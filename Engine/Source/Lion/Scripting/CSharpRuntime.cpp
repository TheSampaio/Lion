#include "Engine.h"
#include "CSharpRuntime.h"

#include <Lion/Core/Input.h>
#include <Lion/Core/Log.h>
#include <Lion/Logic/ComponentRegistry.h>
#include <Lion/Scripting/CSharpScript.h>
#include <Lion/Logic/SceneManager.h>
#include <Lion/Logic/Entity.h>
#include <Lion/Logic/Scene.h>
#include <Lion/Core/Application.h>
#include <Lion/Render/TextRenderer.h>
#include <Lion/Render/SpriteRenderer.h>
#include <Lion/Render/Sprite.h>

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
		constexpr uint32 kAbiVersion = 3;
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
			int32 (*sceneOf)(uint64, uint64*);
			int32 (*validateScene)(uint64);
			int32 (*findSceneEntity)(uint64, const char8*, uint64*);
			int32 (*createSceneEntity)(uint64, const char8*, uint64*);
			int32 (*destroyEntity)(uint64);
			int32 (*getEntityState)(uint64, int32, int32*);
			int32 (*setEntityState)(uint64, int32, int32);
			int32 (*readText)(uint64, int32, char8*, int32, int32*);
			int32 (*writeText)(uint64, int32, const char8*);
			int32 (*hasComponent)(uint64, int32, int32*);
			int32 (*getComponentState)(uint64, int32, int32, int32*);
			int32 (*setComponentState)(uint64, int32, int32, int32);
			int32 (*requestScene)(uint64, const char8*);
			int32 (*quit)();
		};

		struct FieldValue
		{
			int32 kind;
			float32 number;
			int32 integer;
			float32 x, y;
			const char8* text;
		};
		static_assert(sizeof(FieldValue) == 32);
		using ReceiveField = int32 (*)(void*, const char8*, const FieldValue*);
		using ReceiveType = int32 (*)(void*, const char8*);

		struct ManagedFunctions
		{
			uint32 version = kAbiVersion;
			uint32 size = sizeof(ManagedFunctions);
			int32 (*loadAssembly)(const char8*, int32 (*)(const char8*)) = nullptr;
			int32 (*create)(uint64, const char8*, uint64*, uint32*) = nullptr;
			int32 (*invoke)(uint64, int32, float32) = nullptr;
			int32 (*destroy)(uint64) = nullptr;
			int32 (*shutdown)() = nullptr;
			int32 (*enumerateTypes)(void*, ReceiveType) = nullptr;
			int32 (*describe)(const char8*, void*, ReceiveField) = nullptr;
			int32 (*readFields)(uint64, void*, ReceiveField) = nullptr;
			int32 (*writeField)(uint64, const char8*, const FieldValue*) = nullptr;
		};
		static_assert(sizeof(NativeFunctions) == 184);
		static_assert(sizeof(ManagedFunctions) == 80);

		struct RuntimeState
		{
			std::atomic<bool> initialized = false;
			bool gameplayActive = true;
			std::thread::id thread;
			ManagedFunctions managed;
			std::string error;
			std::unordered_map<uint64, std::weak_ptr<Entity>> entities;
			std::unordered_map<Entity*, uint64> entityHandles;
			std::unordered_map<uint64, std::string> actions;
			std::unordered_map<std::string, uint64> actionHandles;
			uint64 nextEntity = 0;
			uint64 nextAction = 0;
			uint64 nextScene = 0;
			std::unordered_map<uint64, std::weak_ptr<Scene>> scenes;
			std::unordered_map<Scene*, uint64> sceneHandles;
			std::vector<std::string> scriptNames;
			std::unordered_map<std::string, std::vector<ScriptingDetail::ScriptField>> fieldDefaults;
		};

		RuntimeState& State()
		{
			static RuntimeState state;
			return state;
		}

		int32 CanRegister(const char8* name)
		{
			try { return name && !ComponentRegistry::Contains(name) ? kSuccess : kFailure; }
			catch (...) { return kFailure; }
		}

		int32 CollectType(void* context, const char8* name)
		{
			try
			{
				if (!context || !name)
					return kFailure;
				static_cast<std::vector<std::string>*>(context)->emplace_back(name);
				return kSuccess;
			}
			catch (...) { return kFailure; }
		}

		int32 CollectField(void* context, const char8* name, const FieldValue* value)
		{
			try
			{
				if (!context || !name || !value || value->kind < 0 || value->kind > 4
					|| !std::isfinite(value->number) || !std::isfinite(value->x) || !std::isfinite(value->y))
					return kFailure;
				ScriptingDetail::ScriptField field;
				field.name = name;
				field.kind = static_cast<ScriptingDetail::FieldKind>(value->kind);
				field.number = value->number;
				field.integer = value->integer;
				field.x = value->x;
				field.y = value->y;
				field.text = value->text ? value->text : "";
				static_cast<std::vector<ScriptingDetail::ScriptField>*>(context)->push_back(std::move(field));
				return kSuccess;
			}
			catch (...) { return kFailure; }
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

		uint64 EntityToken(const Reference<Entity>& entity)
		{
			if (!entity) return 0;
			auto& state = State();
			const auto found = state.entityHandles.find(entity.get());
			if (found != state.entityHandles.end()) return found->second;
			const uint64 token = ++state.nextEntity;
			state.entityHandles.emplace(entity.get(), token);
			state.entities.emplace(token, entity);
			return token;
		}

		template<typename Operation>
		int32 AccessEntity(uint64 token, Operation&& operation)
		{
			const int32 status = CheckRuntime();
			if (status != kSuccess) return status;
			try
			{
				const auto entity = FindEntity(token);
				return entity ? operation(*entity) : kInvalidLifetime;
			}
			catch (const std::exception& error) { ReportError(token, error.what()); return kFailure; }
			catch (...) { return kFailure; }
		}

		template<typename Operation>
		int32 AccessScene(uint64 token, Operation&& operation)
		{
			const int32 status = CheckRuntime();
			if (status != kSuccess) return status;
			try
			{
				const auto found = State().scenes.find(token);
				const auto scene = found == State().scenes.end() ? nullptr : found->second.lock();
				return scene ? operation(*scene) : kInvalidLifetime;
			}
			catch (const std::exception& error) { ReportError(0, error.what()); return kFailure; }
			catch (...) { return kFailure; }
		}

		int32 SceneOf(uint64 token, uint64* result)
		{
			if (!result) return kFailure;
			return AccessEntity(token, [&](Entity& entity)
			{
				const auto scene = entity.GetScene();
				if (!scene) return kInvalidLifetime;
				auto& state = State();
				auto found = state.sceneHandles.find(scene.get());
				if (found == state.sceneHandles.end())
				{
					const uint64 handle = ++state.nextScene;
					state.scenes.emplace(handle, scene);
					found = state.sceneHandles.emplace(scene.get(), handle).first;
				}
				*result = found->second;
				return kSuccess;
			});
		}

		int32 ValidateScene(uint64 token) { return AccessScene(token, [](Scene&) { return kSuccess; }); }
		int32 FindSceneEntity(uint64 token, const char8* name, uint64* result)
		{
			if (!name || !result) return kFailure;
			return AccessScene(token, [&](Scene& scene) { *result = EntityToken(scene.FindEntity(name)); return kSuccess; });
		}
		int32 CreateSceneEntity(uint64 token, const char8* name, uint64* result)
		{
			if (!name || !result) return kFailure;
			return AccessScene(token, [&](Scene& scene)
			{
				auto entity = MakeReference<Entity>();
				entity->SetName(name);
				scene.Add(entity);
				*result = EntityToken(entity);
				return kSuccess;
			});
		}
		int32 DestroyEntity(uint64 token)
		{
			return AccessEntity(token, [](Entity& entity) { entity.RemoveFromScene(); return kSuccess; });
		}
		int32 GetEntityState(uint64 token, int32 property, int32* result)
		{
			if (!result) return kFailure;
			return AccessEntity(token, [&](Entity& entity)
			{
				switch (property)
				{
					case 0: *result = entity.IsEnabled(); break;
					case 1: *result = entity.IsVisible(); break;
					case 2: *result = entity.IsActive(); break;
					default: return kFailure;
				}
				return kSuccess;
			});
		}
		int32 SetEntityState(uint64 token, int32 property, int32 value)
		{
			return AccessEntity(token, [&](Entity& entity)
			{
				if (property == 0) entity.SetEnabled(value != 0);
				else if (property == 1) entity.SetVisible(value != 0);
				else return kFailure;
				return kSuccess;
			});
		}
		int32 ReadText(uint64 token, int32 property, char8* buffer, int32 capacity, int32* required)
		{
			if (!required || capacity < 0) return kFailure;
			return AccessEntity(token, [&](Entity& entity)
			{
				const std::string* text = nullptr;
				if (property == 0) text = &entity.GetName();
				else if (property == 1 && entity.HasComponent<TextRenderer>()) text = &entity.GetComponent<TextRenderer>()->GetText();
				else if (property == 2 && entity.HasComponent<SpriteRenderer>()) text = &entity.GetComponent<SpriteRenderer>()->GetTexturePath();
				if (!text || text->size() >= static_cast<size_t>(INT_MAX)) return kFailure;
				*required = static_cast<int32>(text->size()) + 1;
				if (!buffer) return kSuccess;
				if (capacity < *required) return kFailure;
				std::memcpy(buffer, text->c_str(), *required);
				return kSuccess;
			});
		}
		int32 WriteText(uint64 token, int32 property, const char8* text)
		{
			if (!text) return kFailure;
			return AccessEntity(token, [&](Entity& entity)
			{
				if (property == 0) entity.SetName(text);
				else if (property == 1 && entity.HasComponent<TextRenderer>()) entity.GetComponent<TextRenderer>()->SetText(text);
				else if (property == 2 && entity.HasComponent<SpriteRenderer>()) entity.GetComponent<SpriteRenderer>()->SetTexturePath(text);
				else return kFailure;
				return kSuccess;
			});
		}
		Component* NativeComponent(Entity& entity, int32 kind)
		{
			if (kind == 0) return entity.GetComponent<TextRenderer>();
			if (kind == 1) return entity.GetComponent<SpriteRenderer>();
			return nullptr;
		}
		int32 HasNativeComponent(uint64 token, int32 kind, int32* result)
		{
			if (!result) return kFailure;
			return AccessEntity(token, [&](Entity& entity) { *result = NativeComponent(entity, kind) != nullptr; return kSuccess; });
		}
		int32 GetComponentState(uint64 token, int32 kind, int32 property, int32* result)
		{
			if (!result) return kFailure;
			return AccessEntity(token, [&](Entity& entity)
			{
				Component* component = NativeComponent(entity, kind);
				if (!component) return kInvalidLifetime;
				if (property == 0) *result = component->IsEnabled();
				else if (kind == 1 && property == 1) *result = static_cast<SpriteRenderer*>(component)->GetOrder();
				else if (kind == 1 && property == 2) *result = static_cast<SpriteRenderer*>(component)->IsFlippedX();
				else if (kind == 1 && property == 3) *result = static_cast<SpriteRenderer*>(component)->IsFlippedY();
				else return kFailure;
				return kSuccess;
			});
		}
		int32 SetComponentState(uint64 token, int32 kind, int32 property, int32 value)
		{
			return AccessEntity(token, [&](Entity& entity)
			{
				Component* component = NativeComponent(entity, kind);
				if (!component) return kInvalidLifetime;
				if (property == 0) component->SetEnabled(value != 0);
				else if (kind == 1 && property == 1) static_cast<SpriteRenderer*>(component)->SetOrder(value);
				else if (kind == 1 && property == 2) static_cast<SpriteRenderer*>(component)->SetFlipX(value != 0);
				else if (kind == 1 && property == 3) static_cast<SpriteRenderer*>(component)->SetFlipY(value != 0);
				else return kFailure;
				return kSuccess;
			});
		}
		int32 RequestScene(uint64 token, const char8* path)
		{
			if (!path) return kFailure;
			return AccessScene(token, [&](Scene& scene)
			{
				if (SceneManager::GetActiveScene().get() != &scene) return kUnavailable;
				return SceneManager::LoadScene(path) ? kSuccess : kFailure;
			});
		}
		int32 Quit()
		{
			const int32 status = CheckRuntime();
			if (status != kSuccess) return status;
			Application::RequestQuit();
			return kSuccess;
		}

		const NativeFunctions kNativeFunctions = { kAbiVersion, sizeof(NativeFunctions), ValidateEntity,
			GetTransform, SetTransform, ResolveAction, ReadAction, IsLogEnabled, WriteLog, ReportError,
			SceneOf, ValidateScene, FindSceneEntity, CreateSceneEntity, DestroyEntity, GetEntityState,
			SetEntityState, ReadText, WriteText, HasNativeComponent, GetComponentState, SetComponentState, RequestScene, Quit };
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

		struct InitializeParameters
		{
			size_t size;
			const wchar_t* hostPath;
			const wchar_t* dotnetRoot;
		};
		const std::wstring dotnetRoot = std::filesystem::path(hostfxrPath).parent_path().parent_path().parent_path().parent_path().wstring();
		const InitializeParameters parameters { sizeof(InitializeParameters), nullptr, dotnetRoot.c_str() };
		void* context = nullptr;
		int32 status = initialize(configuration.c_str(), &parameters, &context);
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
			|| !functions.invoke || !functions.destroy || !functions.shutdown || !functions.enumerateTypes
			|| !functions.describe || !functions.readFields || !functions.writeField)
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
		if (State().managed.loadAssembly(assemblyPath.c_str(), CanRegister) != kSuccess)
		{
			error = State().error;
			return false;
		}

		std::vector<std::string> names;
		if (State().managed.enumerateTypes(&names, CollectType) != kSuccess)
		{
			error = State().error;
			return false;
		}
		for (const std::string& name : names)
		{
			if (std::find(State().scriptNames.begin(), State().scriptNames.end(), name) != State().scriptNames.end())
				continue;
			if (!ComponentRegistry::RegisterNamed(name, [name]() -> Scope<Component> { return MakeScope<CSharpScript>(name); }))
			{
				error = "Could not register script '" + name + "'.";
				return false;
			}
			State().scriptNames.push_back(name);
		}
		error.clear();
		return true;
	}

	uint64 CSharpRuntime::CreateInstance(Entity& entity, const std::string& typeName, uint32& callbacks)
	{
		if (CheckRuntime() != kSuccess || !State().gameplayActive || !entity.GetScene())
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
		if (instance != 0 && CheckRuntime() == kSuccess && State().gameplayActive)
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
		{
			error.clear();
			return true;
		}

		if (CheckRuntime() != kSuccess)
		{
			error = "Runtime shutdown requires the engine thread.";
			return false;
		}

		RuntimeState& state = State();
		const bool succeeded = state.managed.shutdown() == kSuccess;
		for (const std::string& name : state.scriptNames)
			ComponentRegistry::UnregisterNamed(name);
		state.scriptNames.clear();
		state.fieldDefaults.clear();
		state.entities.clear();
		state.entityHandles.clear();
		state.actions.clear();
		state.scenes.clear();
		state.sceneHandles.clear();
		state.actionHandles.clear();
		state.initialized.store(false);
		error = succeeded ? std::string() : state.error;
		return succeeded;
	}

	bool CSharpRuntime::GetFields(const std::string& typeName, std::vector<ScriptingDetail::ScriptField>& fields)
	{
		if (CheckRuntime() != kSuccess)
			return false;
		RuntimeState& state = State();
		const auto found = state.fieldDefaults.find(typeName);
		if (found != state.fieldDefaults.end())
		{
			fields = found->second;
			return true;
		}
		std::vector<ScriptingDetail::ScriptField> defaults;
		if (state.managed.describe(typeName.c_str(), &defaults, CollectField) != kSuccess)
			return false;
		fields = defaults;
		state.fieldDefaults.emplace(typeName, std::move(defaults));
		return true;
	}

	bool CSharpRuntime::ReadFields(uint64 instance, std::vector<ScriptingDetail::ScriptField>& fields)
	{
		if (instance == 0 || CheckRuntime() != kSuccess)
			return false;
		std::vector<ScriptingDetail::ScriptField> values;
		if (State().managed.readFields(instance, &values, CollectField) != kSuccess)
			return false;
		fields = std::move(values);
		return true;
	}

	bool CSharpRuntime::WriteFields(uint64 instance, const std::vector<ScriptingDetail::ScriptField>& fields)
	{
		if (instance == 0 || CheckRuntime() != kSuccess)
			return false;
		for (const auto& field : fields)
		{
			const FieldValue value = { static_cast<int32>(field.kind), field.number, field.integer,
				field.x, field.y, field.text.c_str() };
			if (State().managed.writeField(instance, field.name.c_str(), &value) != kSuccess)
				return false;
		}
		return true;
	}

	bool CSharpRuntime::SetGameplayActive(bool active)
	{
		if (State().thread != std::thread::id() && State().thread != std::this_thread::get_id())
			return false;
		State().gameplayActive = active;
		return true;
	}

	bool CSharpRuntime::IsGameplayActive() { return State().gameplayActive; }
	void CSharpRuntime::InvalidateScene(Scene& scene)
	{
		if (!IsInitialized()) return;
		auto& state = State();
		const auto found = state.sceneHandles.find(&scene);
		if (found == state.sceneHandles.end()) return;
		state.scenes.erase(found->second);
		state.sceneHandles.erase(found);
	}
	bool CSharpRuntime::IsInitialized() { return State().initialized.load(); }
	const std::string& CSharpRuntime::GetLastError() { return State().error; }
}
