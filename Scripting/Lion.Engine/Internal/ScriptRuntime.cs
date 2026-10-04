using System.Linq.Expressions;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Runtime.Loader;
using System.Text;

namespace Lion.Engine.Internal;

[StructLayout(LayoutKind.Sequential)]
internal unsafe struct ManagedFunctions
{
	internal uint Version;
	internal uint Size;
	internal delegate* unmanaged[Cdecl]<byte*, delegate* unmanaged[Cdecl]<byte*, int>, int> LoadAssembly;
	internal delegate* unmanaged[Cdecl]<ulong, byte*, ulong*, uint*, int> Create;
	internal delegate* unmanaged[Cdecl]<ulong, int, float, int> Invoke;
	internal delegate* unmanaged[Cdecl]<ulong, int> Destroy;
	internal delegate* unmanaged[Cdecl]<int> Shutdown;
	internal delegate* unmanaged[Cdecl]<void*, delegate* unmanaged[Cdecl]<void*, byte*, int>, int> EnumerateTypes;
	internal delegate* unmanaged[Cdecl]<byte*, void*, delegate* unmanaged[Cdecl]<void*, byte*, FieldValue*, int>, int> Describe;
	internal delegate* unmanaged[Cdecl]<ulong, void*, delegate* unmanaged[Cdecl]<void*, byte*, FieldValue*, int>, int> ReadFields;
	internal delegate* unmanaged[Cdecl]<ulong, byte*, FieldValue*, int> WriteField;
}

internal static unsafe class ScriptRuntime
{
	private sealed class Instance(Behaviour behaviour)
	{
		internal readonly Behaviour Behaviour = behaviour;
		internal bool Faulted;
	}

	private sealed class GameContext(string assemblyPath) : AssemblyLoadContext(isCollectible: true)
	{
		private readonly AssemblyDependencyResolver _resolver = new(assemblyPath);
		protected override Assembly? Load(AssemblyName assemblyName)
		{
			var api = typeof(Behaviour).Assembly;
			if (assemblyName.Name == api.GetName().Name)
			{
				if (!AssemblyName.ReferenceMatchesDefinition(assemblyName, api.GetName()))
				{
					throw new FileLoadException("The game references an incompatible Lion.Engine assembly.");
				}
				return api;
			}
			string? path = _resolver.ResolveAssemblyToPath(assemblyName);
			return path == null ? null : LoadFromAssemblyPath(path);
		}
	}

	private sealed record ScriptType(Func<Behaviour> Create, uint Callbacks, Dictionary<string, FieldMetadata> Fields);
	private static readonly string[] CallbackNames = ["OnAwake", "OnEnable", "OnDisable", "OnUpdateBegin", "OnUpdate", "OnUpdateEnd"];
	private static readonly Dictionary<string, ScriptType> Factories = new(StringComparer.Ordinal);
	private static readonly Dictionary<ulong, Instance> Instances = [];
	private static readonly Dictionary<string, GameContext> Contexts = new(StringComparer.OrdinalIgnoreCase);
	private static ulong _nextInstance;

	[UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
	public static int Initialize(NativeFunctions* native, ManagedFunctions* managed)
	{
		try
		{
			if (managed == null || managed->Version != 2 || managed->Size != sizeof(ManagedFunctions)
				|| !NativeApi.Bind(native))
			{
				return 1;
			}
			managed->LoadAssembly = &LoadAssembly;
			managed->Create = &Create;
			managed->Invoke = &Invoke;
			managed->Destroy = &Destroy;
			managed->Shutdown = &Shutdown;
			managed->EnumerateTypes = &EnumerateTypes;
			managed->Describe = &Describe;
			managed->ReadFields = &ReadFields;
			managed->WriteField = &WriteField;
			return 0;
		}
		catch { return 1; }
	}

	[UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
	private static int LoadAssembly(byte* path, delegate* unmanaged[Cdecl]<byte*, int> canRegister)
	{
		GameContext? pending = null;
		try
		{
			NativeApi.CheckThread();
			string assemblyPath = Marshal.PtrToStringUTF8((nint)path) ?? throw new ArgumentNullException(nameof(path));
			assemblyPath = Path.GetFullPath(assemblyPath);
			if (Contexts.ContainsKey(assemblyPath))
			{
				throw new InvalidOperationException($"Assembly '{assemblyPath}' is already loaded.");
			}
			pending = new GameContext(assemblyPath);
			var assembly = pending.LoadFromAssemblyPath(assemblyPath);
			var discovered = new Dictionary<string, ScriptType>(StringComparer.Ordinal);
			foreach (var type in assembly.GetTypes())
			{
				if (type.IsAbstract || type.ContainsGenericParameters || !type.IsPublic || !type.IsSubclassOf(typeof(Behaviour)))
				{
					continue;
				}
				var constructor = type.GetConstructor(Type.EmptyTypes)
					?? throw new InvalidOperationException($"Behaviour '{type.FullName}' needs a public parameterless constructor.");
				string name = type.FullName!;
				if (Factories.ContainsKey(name))
				{
					throw new InvalidOperationException($"Behaviour '{name}' is already registered.");
				}
				fixed (byte* nativeName = Encoding.UTF8.GetBytes(name + '\0'))
				{
					if (canRegister == null || canRegister(nativeName) != 0)
					{
						throw new InvalidOperationException($"Behaviour '{name}' conflicts with an existing native component.");
					}
				}
				uint callbacks = 0;
				for (int index = 0; index < CallbackNames.Length; index++)
				{
					var method = type.GetMethod(CallbackNames[index], index < 3 ? Type.EmptyTypes : [typeof(float)]);
					if (method?.DeclaringType != typeof(Behaviour)) { callbacks |= 1u << index; }
				}
				discovered.Add(name, new ScriptType(Expression.Lambda<Func<Behaviour>>(Expression.New(constructor)).Compile(), callbacks,
					FieldMetadata.Discover(type)));
			}
			foreach (var entry in discovered)
			{
				Factories.Add(entry.Key, entry.Value);
			}
			Contexts.Add(assemblyPath, pending);
			pending = null;
			return 0;
		}
		catch (Exception exception) { pending?.Unload(); return Report(0, "Assembly discovery", exception); }
	}

	[UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
	private static int Create(ulong entity, byte* name, ulong* result, uint* callbacks)
	{
		try
		{
			NativeApi.CheckThread();
			if (result == null || callbacks == null) { throw new ArgumentNullException(nameof(result)); }
			*result = 0;
			*callbacks = 0;
			string typeName = Marshal.PtrToStringUTF8((nint)name) ?? throw new ArgumentNullException(nameof(name));
			if (!Factories.TryGetValue(typeName, out var factory))
			{
				throw new InvalidOperationException($"Behaviour '{typeName}' was not discovered.");
			}
			NativeApi.CheckStatus(NativeStatus(entity));
			var behaviour = factory.Create();
			behaviour.Entity = new Entity(entity);
			ulong id = checked(++_nextInstance);
			Instances.Add(id, new Instance(behaviour));
			*result = id;
			*callbacks = factory.Callbacks;
			return 0;
		}
		catch (Exception exception) { return Report(entity, "Script creation", exception); }
	}

	private static int NativeStatus(ulong entity) => NativeApi.IsEntityValid(entity) ? 0 : 1;

	[UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
	private static int EnumerateTypes(void* context, delegate* unmanaged[Cdecl]<void*, byte*, int> receive)
	{
		try
		{
			NativeApi.CheckThread();
			if (receive == null) { throw new ArgumentNullException(nameof(receive)); }
			foreach (string name in Factories.Keys)
			{
				fixed (byte* text = Encoding.UTF8.GetBytes(name + '\0')) { NativeApi.CheckStatus(receive(context, text)); }
			}
			return 0;
		}
		catch (Exception exception) { return Report(0, "Script catalog", exception); }
	}

	[UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
	private static int Describe(byte* name, void* context,
		delegate* unmanaged[Cdecl]<void*, byte*, FieldValue*, int> receive)
	{
		try
		{
			NativeApi.CheckThread();
			var type = Factories[Marshal.PtrToStringUTF8((nint)name)!];
			if (type.Fields.Count == 0) { return 0; }
			var defaults = type.Create();
			foreach (var field in type.Fields.Values) { field.Emit(defaults, context, receive); }
			return 0;
		}
		catch (Exception exception) { return Report(0, "Script field defaults", exception); }
	}

	[UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
	private static int ReadFields(ulong id, void* context,
		delegate* unmanaged[Cdecl]<void*, byte*, FieldValue*, int> receive)
	{
		try
		{
			NativeApi.CheckThread();
			var owner = Instances[id].Behaviour;
			foreach (var field in Factories[owner.GetType().FullName!].Fields.Values) { field.Emit(owner, context, receive); }
			return 0;
		}
		catch (Exception exception) { return Report(0, "Script field read", exception); }
	}

	[UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
	private static int WriteField(ulong id, byte* name, FieldValue* value)
	{
		try
		{
			NativeApi.CheckThread();
			var owner = Instances[id].Behaviour;
			Factories[owner.GetType().FullName!].Fields[Marshal.PtrToStringUTF8((nint)name)!].Apply(owner, value);
			return 0;
		}
		catch (Exception exception) { return Report(0, "Script field write", exception); }
	}

	[UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
	private static int Invoke(ulong id, int callback, float deltaTime)
	{
		float previousDeltaTime = Time.CallbackDeltaTime;
		Instance? instance = null;
		try
		{
			NativeApi.CheckThread();
			if (!Instances.TryGetValue(id, out instance) || instance.Faulted) { return 1; }
			NativeApi.CheckStatus(NativeStatus(instance.Behaviour.Entity.Handle));
			Time.CallbackDeltaTime = callback is >= 3 and <= 5 ? deltaTime : 0;
			switch (callback)
			{
				case 0: instance.Behaviour.OnAwake(); break;
				case 1: instance.Behaviour.OnEnable(); break;
				case 2: instance.Behaviour.OnDisable(); break;
				case 3: instance.Behaviour.OnUpdateBegin(deltaTime); break;
				case 4: instance.Behaviour.OnUpdate(deltaTime); break;
				case 5: instance.Behaviour.OnUpdateEnd(deltaTime); break;
				default: throw new ArgumentOutOfRangeException(nameof(callback));
			}
			return 0;
		}
		catch (Exception exception)
		{
			if (instance != null) { instance.Faulted = true; }
			return Report(instance?.Behaviour.Entity.Handle ?? 0,
				$"{instance?.Behaviour.GetType().FullName}, {CallbackName(callback)}", exception);
		}
		finally { Time.CallbackDeltaTime = previousDeltaTime; }
	}

	private static string CallbackName(int callback) => callback switch
	{
		0 => "OnAwake", 1 => "OnEnable", 2 => "OnDisable", 3 => "OnUpdateBegin",
		4 => "OnUpdate", 5 => "OnUpdateEnd", _ => "Unknown callback"
	};

	[UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
	private static int Destroy(ulong id)
	{
		try { NativeApi.CheckThread(); return DestroyInstance(id); }
		catch (Exception exception) { return Report(0, "Script destruction", exception); }
	}

	private static int DestroyInstance(ulong id)
	{
		if (!Instances.Remove(id, out var instance)) { return 0; }
		try { instance.Behaviour.OnDestroy(); return 0; }
		catch (Exception exception)
		{
			return Report(instance.Behaviour.Entity.Handle,
				$"{instance.Behaviour.GetType().FullName}, OnDestroy", exception);
		}
	}

	[UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
	private static int Shutdown()
	{
		try
		{
			NativeApi.CheckThread();
			bool failed = false;
			foreach (ulong id in Instances.Keys.ToArray()) { failed |= DestroyInstance(id) != 0; }
			Factories.Clear();
			foreach (var context in Contexts.Values) { context.Unload(); }
			Contexts.Clear();
			return failed ? 1 : 0;
		}
		catch (Exception exception) { return Report(0, "Runtime shutdown", exception); }
	}

	private static int Report(ulong entity, string callback, Exception exception)
	{
		try { NativeApi.ReportError(entity, $"[C#] {callback}: {exception}"); }
		catch { /* Reporting must never unwind into the native host. */ }
		return 1;
	}
}
