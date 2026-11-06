using System.Linq.Expressions;
using System.Numerics;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Text;

namespace Lion.Engine.Internal;

[StructLayout(LayoutKind.Sequential)]
internal unsafe struct FieldValue
{
	internal int Kind;
	internal float Number;
	internal int Integer;
	internal float X;
	internal float Y;
	internal byte* Text;
}

internal sealed unsafe class FieldMetadata
{
	internal readonly string Name;
	private readonly int _kind;
	private readonly Func<Behaviour, object?> _get;
	private readonly Action<Behaviour, object?> _set;

	private FieldMetadata(FieldInfo field, int kind)
	{
		Name = field.Name;
		_kind = kind;
		var owner = Expression.Parameter(typeof(Behaviour));
		var value = Expression.Parameter(typeof(object));
		var member = Expression.Field(Expression.Convert(owner, field.DeclaringType!), field);
		_get = Expression.Lambda<Func<Behaviour, object?>>(Expression.Convert(member, typeof(object)), owner).Compile();
		_set = Expression.Lambda<Action<Behaviour, object?>>(
			Expression.Assign(member, Expression.Convert(value, field.FieldType)), owner, value).Compile();
	}

	internal static Dictionary<string, FieldMetadata> Discover(Type type)
	{
		var fields = new Dictionary<string, FieldMetadata>(StringComparer.Ordinal);
		for (Type? current = type; current != typeof(Behaviour) && current != null; current = current.BaseType)
		{
			foreach (var field in current.GetFields(BindingFlags.Instance | BindingFlags.Static
				| BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.DeclaredOnly))
			{
				if (!field.IsDefined(typeof(EditableAttribute))) { continue; }
				int kind = field.FieldType == typeof(float) ? 0 : field.FieldType == typeof(int) ? 1
					: field.FieldType == typeof(bool) ? 2 : field.FieldType == typeof(string) ? 3
					: field.FieldType == typeof(Vector2) ? 4 : -1;
				if (field.IsStatic || field.IsInitOnly || kind < 0 || fields.ContainsKey(field.Name))
				{
					throw new InvalidOperationException($"Editable field '{type.FullName}.{field.Name}' must have a unique name, be mutable and use a supported instance-field type.");
				}
				fields.Add(field.Name, new FieldMetadata(field, kind));
			}
		}
		return fields;
	}

	internal void Emit(Behaviour owner, void* context,
		delegate* unmanaged[Cdecl]<void*, byte*, FieldValue*, int> receive)
	{
		object? value = _get(owner);
		var state = new FieldValue { Kind = _kind };
		switch (_kind)
		{
			case 0: state.Number = (float)value!; break;
			case 1: state.Integer = (int)value!; break;
			case 2: state.Integer = (bool)value! ? 1 : 0; break;
			case 4: var vector = (Vector2)value!; state.X = vector.X; state.Y = vector.Y; break;
		}
		string text = _kind == 3 ? (string?)value ?? "" : "";
		if (text.Contains('\0')) { throw new InvalidOperationException($"Editable string '{Name}' cannot contain null characters."); }
		fixed (byte* name = Encoding.UTF8.GetBytes(Name + '\0'))
		fixed (byte* content = Encoding.UTF8.GetBytes(text + '\0'))
		{
			state.Text = content;
			NativeApi.CheckStatus(receive(context, name, &state));
		}
	}

	internal void Apply(Behaviour owner, FieldValue* state)
	{
		if (state == null || state->Kind != _kind) { throw new InvalidOperationException($"Editable field '{Name}' has an incompatible type."); }
		if (!float.IsFinite(state->Number) || !float.IsFinite(state->X) || !float.IsFinite(state->Y))
		{
			throw new InvalidOperationException($"Editable field '{Name}' must contain finite values.");
		}
		object value = _kind switch
		{
			0 => state->Number, 1 => state->Integer, 2 => state->Integer != 0,
			3 => Marshal.PtrToStringUTF8((nint)state->Text) ?? "",
			4 => new Vector2(state->X, state->Y), _ => throw new InvalidOperationException("Unknown field kind.")
		};
		_set(owner, value);
	}
}
