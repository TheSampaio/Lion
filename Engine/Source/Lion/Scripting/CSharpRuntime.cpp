#include "Engine.h"
#include "CSharpRuntime.h"

#include <Lion/Core/Input.h>
#include <Lion/Core/Log.h>
#include <Lion/Core/Filesystem.h>
#include <Lion/Logic/SceneManager.h>
#include <Lion/Logic/Entity.h>
#include <Lion/Logic/Scene.h>

#include <atomic>
#include <filesystem>
#include <thread>

#ifdef LN_PLATFORM_WIN
	#define WIN32_LEAN_AND_MEAN
	#define NOMINMAX
	#include <Windows.h>
#endif

namespace Lion
{
	namespace
	{
		constexpr uint32 kAbiVersion = 1;
		constexpr int32 kSuccess = 0;
		constexpr int32 kInvalidLifetime = 1;
		constexpr int32 kWrongThread = 2;
		constexpr int32 kUnavailable = 3;
		constexpr int32 kFailure = 4;

		struct TransformState
		{
			float32 positionX, positionY, rotation, scaleX, scaleY;
		};
		static_assert(sizeof(TransformState) == 20);

		struct NativeFunctions
		{
			uint32 version = kAbiVersion;
			uint32 size = sizeof(NativeFunctions);
			int32 (*validateEntity)(uint64);
			int32 (*getTransform)(uint64, TransformState*);
			int32 (*setTransform)(uint64, const TransformState*);
			int32 (*resolveAction)(const char8*, uint64*);
			int32 (*readAction)(uint64, float32*);
			int32 (*isLogEnabled)(int32);
			void (*writeLog)(int32, const char8*);
			void (*reportError)(uint64, const char8*);
		};

		struct ManagedFunctions
		{
			uint32 version = kAbiVersion;
			uint32 size = sizeof(ManagedFunctions);
			int32 (*loadAssembly)(const char8*) = nullptr;
			int32 (*create)(uint64, const char8*, uint64*, uint32*) = nullptr;
			int32 (*invoke)(uint64, int32, float32) = nullptr;
			int32 (*destroy)(uint64) = nullptr;
			int32 (*shutdown)() = nullptr;
		};
		static_assert(sizeof(NativeFunctions) == 72);
		static_assert(sizeof(ManagedFunctions) == 48);

		struct RuntimeState
		{
			std::atomic<bool> initialized = false;
			std::thread::id thread;
			ManagedFunctions managed;
			std::string error;
			std::unordered_map<uint64, std::weak_ptr<Entity>> entities;
			std::unordered_map<Entity*, uint64> entityHandles;
			std::unordered_map<uint64, std::string> actions;
			std::unordered_map<std::string, uint64> actionHandles;
			uint64 nextEntity = 0;
			uint64 nextAction = 0;
		};

		RuntimeState& State()
		{
			static RuntimeState state;
			return state;
		}

		int32 CheckRuntime()
		{
			RuntimeState& state = State();

			if (!state.initialized.load())
				return kUnavailable;

			return state.thread == std::this_thread::get_id() ? kSuccess : kWrongThread;
		}

		Reference<Entity> FindEntity(uint64 handle)
		{
			const auto found = State().entities.find(handle);
			return found == State().entities.end() ? nullptr : found->second.lock();
		}

		int32 ValidateEntity(uint64 handle)
		{
			const int32 status = CheckRuntime();
			return status != kSuccess ? status : (FindEntity(handle) ? kSuccess : kInvalidLifetime);
		}

		int32 GetTransform(uint64 handle, TransformState* result)
		{
			const int32 status = CheckRuntime();
			if (status != kSuccess)
				return status;

			const Reference<Entity> entity = FindEntity(handle);
			if (!entity || !result)
				return kInvalidLifetime;

			const Reference<Transform> transform = entity->GetTransform();
			const Vector2 position = transform->GetPosition();
			const Vector2 scale = transform->GetScale();
			*result = { position.x, position.y, transform->GetRotation(), scale.x, scale.y };
			return kSuccess;
		}

		int32 SetTransform(uint64 handle, const TransformState* value)
		{
			const int32 status = CheckRuntime();
			if (status != kSuccess)
				return status;

			const Reference<Entity> entity = FindEntity(handle);
			if (!entity || !value)
				return kInvalidLifetime;

			if (!std::isfinite(value->positionX) || !std::isfinite(value->positionY)
				|| !std::isfinite(value->rotation) || !std::isfinite(value->scaleX) || !std::isfinite(value->scaleY))
				return kFailure;

			const Reference<Transform> transform = entity->GetTransform();
			transform->SetPosition({ value->positionX, value->positionY });
			transform->SetRotation(value->rotation);
			transform->SetScale({ value->scaleX, value->scaleY });
			return kSuccess;
		}

		int32 ResolveAction(const char8* name, uint64* result)
		{
			const int32 status = CheckRuntime();
			if (status != kSuccess)
				return status;

			if (!name || !result)
				return kFailure;

			try
			{
				RuntimeState& state = State();
				const auto found = state.actionHandles.find(name);
				if (found != state.actionHandles.end())
				{
					*result = found->second;
					return kSuccess;
				}
				const uint64 handle = ++state.nextAction;
				state.actions.emplace(handle, name);
				state.actionHandles.emplace(name, handle);
				*result = handle;
				return kSuccess;
			}
			catch (...) { return kFailure; }
		}

		int32 ReadAction(uint64 handle, float32* result)
		{
			const int32 status = CheckRuntime();
			if (status != kSuccess)
				return status;

			const auto found = State().actions.find(handle);
			if (found == State().actions.end() || !result)
				return kInvalidLifetime;

			*result = Input::GetActionStrength(found->second);
			return kSuccess;
		}

		int32 IsLogEnabled(int32 level)
		{
			return CheckRuntime() == kSuccess && level >= 0 && level <= 5
				&& Log::IsEnabled(static_cast<LogLevel>(level));
		}

		void WriteLog(int32 level, const char8* text)
		{
			if (!text || !IsLogEnabled(level))
				return;

			try { Log::Console(static_cast<LogLevel>(level), text); }
			catch (...) {}
		}

		void ReportError(uint64 handle, const char8* text)
		{
			if (CheckRuntime() != kSuccess || !text)
				return;

			try
			{
				RuntimeState& state = State();
				const Reference<Entity> entity = FindEntity(handle);
				state.error = text;
				if (entity)
					state.error += std::format(" [Entity: '{}' ({}), Scene: '{}']", entity->GetName(),
						entity->GetId(), SceneManager::GetActivePath());

				Log::Console(LogLevel::Error, state.error);
			}
			catch (...) {}
		}

		const NativeFunctions kNativeFunctions = { kAbiVersion, sizeof(NativeFunctions), ValidateEntity,
			GetTransform, SetTransform, ResolveAction, ReadAction, IsLogEnabled, WriteLog, ReportError };
	}

	bool CSharpRuntime::Initialize(const std::string& managedDirectory,
		const std::string& hostfxrPath, std::string& error)
	{
		RuntimeState& state = State();
		if (state.thread != std::thread::id() && state.thread != std::this_thread::get_id())
		{
			error = "Runtime initialization requires the original engine thread.";
			return false;
		}

		if (state.initialized.load())
		{
			error = "The C# runtime is already initialized.";
			return false;
		}

#ifdef LN_PLATFORM_WIN
		// Only these private declarations depend on the hostfxr Windows x64 ABI. Gameplay tables use
		// cdecl and fixed-width values; the public C++ headers do not depend on the .NET SDK.
		using InitializeHost = int32 (__cdecl*)(const wchar_t*, const void*, void**);
		using GetDelegate = int32 (__cdecl*)(void*, int32, void**);
		using CloseHost = int32 (__cdecl*)(void*);
		using LoadAssembly = int32 (__stdcall*)(const wchar_t*, const wchar_t*, const wchar_t*,
			const wchar_t*, void*, void**);
		using InitializeManaged = int32 (*)(const NativeFunctions*, ManagedFunctions*);
		constexpr int32 kLoadAssemblyDelegate = 5;

		const std::filesystem::path directory(managedDirectory);
		const std::filesystem::path hostPath(hostfxrPath);
		const std::filesystem::path configuration = directory / "Lion.Engine.runtimeconfig.json";
		const std::filesystem::path assembly = directory / "Lion.Engine.dll";
		std::error_code code;

		if (!directory.is_absolute() || !hostPath.is_absolute()
			|| !std::filesystem::is_regular_file(configuration, code)
			|| !std::filesystem::is_regular_file(assembly, code)
			|| !std::filesystem::is_regular_file(hostPath, code))
		{
			error = "C# hosting requires absolute paths to hostfxr and a built Lion.Engine runtime directory.";
			return false;
		}

		// The hosting module and CoreCLR stay resident for the process; script shutdown is not runtime unload.
		static HMODULE host = nullptr;
		static std::filesystem::path loadedHostPath;
		if (host && loadedHostPath != hostPath)
		{
			error = "The process cannot switch to another hostfxr library after initialization.";
			return false;
		}

		if (!host)
		{
			host = LoadLibraryExW(hostPath.c_str(), nullptr, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
			if (!host)
			{
				error = std::format("Could not load hostfxr (Windows error {}).", ::GetLastError());
				return false;
			}
			loadedHostPath = hostPath;
		}

		const auto initialize = reinterpret_cast<InitializeHost>(GetProcAddress(host, "hostfxr_initialize_for_runtime_config"));
		const auto getDelegate = reinterpret_cast<GetDelegate>(GetProcAddress(host, "hostfxr_get_runtime_delegate"));
		const auto close = reinterpret_cast<CloseHost>(GetProcAddress(host, "hostfxr_close"));
		if (!initialize || !getDelegate || !close)
		{
			error = "The selected hostfxr does not expose the required hosting API.";
			return false;
		}

		void* context = nullptr;
		int32 status = initialize(configuration.c_str(), nullptr, &context);
		if (status < 0 || !context)
		{
			if (context)
				close(context);
			error = std::format("Could not initialize .NET for Lion.Engine (host status {}).", status);
			return false;
		}

		LoadAssembly load = nullptr;
		status = getDelegate(context, kLoadAssemblyDelegate, reinterpret_cast<void**>(&load));
		close(context);
		if (status < 0 || !load)
		{
			error = std::format("Could not obtain the .NET assembly loader (host status {}).", status);
			return false;
		}

		InitializeManaged bind = nullptr;
		status = load(assembly.c_str(), L"Lion.Engine.Internal.ScriptRuntime, Lion.Engine", L"Initialize",
			reinterpret_cast<const wchar_t*>(static_cast<intptr_t>(-1)), nullptr, reinterpret_cast<void**>(&bind));
		if (status < 0 || !bind)
		{
			error = std::format("Could not load the Lion managed bootstrap (host status {}).", status);
			return false;
		}

		ManagedFunctions functions;
		if (bind(&kNativeFunctions, &functions) != kSuccess || functions.version != kAbiVersion
			|| functions.size != sizeof(ManagedFunctions) || !functions.loadAssembly || !functions.create
			|| !functions.invoke || !functions.destroy || !functions.shutdown)
		{
			error = "The Lion native and managed scripting ABIs are incompatible.";
			return false;
		}

		state.managed = functions;
		state.thread = std::this_thread::get_id();
		state.error.clear();
		state.initialized.store(true);
		error.clear();
		return true;
#else
		error = "C# hosting is currently supported only on Windows x64.";
		return false;
#endif
	}

	bool CSharpRuntime::LoadAssembly(const std::string& assemblyPath, std::string& error)
	{
		if (CheckRuntime() != kSuccess)
		{
			error = "Assembly loading requires an initialized runtime on the engine thread.";
			return false;
		}

		State().error.clear();
		if (State().managed.loadAssembly(assemblyPath.c_str()) != kSuccess)
		{
			error = State().error;
			return false;
		}
		error.clear();
		return true;
	}

	uint64 CSharpRuntime::CreateInstance(Entity& entity, const std::string& typeName, uint32& callbacks)
	{
		if (CheckRuntime() != kSuccess || !entity.GetScene())
			return 0;

		RuntimeState& state = State();
		auto found = state.entityHandles.find(&entity);
		uint64 handle = found == state.entityHandles.end() ? 0 : found->second;
		if (handle == 0)
		{
			for (const Reference<Entity>& candidate : entity.GetScene()->GetEntities())
			{
				if (candidate.get() == &entity)
				{
					handle = ++state.nextEntity;
					state.entities.emplace(handle, candidate);
					state.entityHandles.emplace(&entity, handle);
					break;
				}
			}
		}

		uint64 instance = 0;
		if (handle != 0 && state.managed.create(handle, typeName.c_str(), &instance, &callbacks) == kSuccess)
			return instance;

		return 0;
	}

	void CSharpRuntime::Invoke(uint64 instance, int32 callback, float32 deltaTime)
	{
		if (instance != 0 && CheckRuntime() == kSuccess)
			State().managed.invoke(instance, callback, deltaTime);
	}

	void CSharpRuntime::DestroyInstance(uint64 instance)
	{
		if (instance != 0 && CheckRuntime() == kSuccess)
			State().managed.destroy(instance);
	}

	void CSharpRuntime::InvalidateEntity(Entity& entity)
	{
		if (CheckRuntime() != kSuccess)
			return;

		RuntimeState& state = State();
		const auto found = state.entityHandles.find(&entity);
		if (found == state.entityHandles.end())
			return;

		state.entities.erase(found->second);
		state.entityHandles.erase(found);
	}

	bool CSharpRuntime::Shutdown(std::string& error)
	{
		if (!IsInitialized())
			return true;

		if (CheckRuntime() != kSuccess)
		{
			error = "Runtime shutdown requires the engine thread.";
			return false;
		}

		RuntimeState& state = State();
		const bool succeeded = state.managed.shutdown() == kSuccess;
		state.entities.clear();
		state.entityHandles.clear();
		state.actions.clear();
		state.actionHandles.clear();
		state.initialized.store(false);
		error = succeeded ? std::string() : state.error;
		return succeeded;
	}

	bool CSharpRuntime::IsInitialized() { return State().initialized.load(); }
	const std::string& CSharpRuntime::GetLastError() { return State().error; }
}
