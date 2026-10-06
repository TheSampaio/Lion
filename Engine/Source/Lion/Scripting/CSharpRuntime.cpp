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
#include <Lion/Render/Camera2D.h>
#include <Lion/Render/ParticleComponent.h>
#include <Lion/Render/PostProcessingComponent.h>
#include <Lion/Physics/RigidBody2D.h>
#include <Lion/Physics/BoxCollider2D.h>
#include <Lion/Physics/CircleCollider2D.h>
#include <Lion/Physics/PhysicsWorld.h>
#include <Lion/Audio/AudioPlayer.h>
#include <Lion/Core/Asset.h>
#include <Lion/Core/Filesystem.h>
#include <Lion/Core/Vault.h>
#include <Lion/Core/Window.h>
#include <Lion/Core/Clock.h>
#include <Lion/Logic/AssemblySerializer.h>
#include <Lion/Logic/Reflector.h>
#include <Lion/UI/Button.h>
#include <Lion/UI/CheckBox.h>
#include <Lion/UI/ComboBox.h>
#include <Lion/UI/ProgressBar.h>
#include <Lion/UI/WidgetAnchor.h>

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
		constexpr uint32 kAbiVersion = 4;
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
		struct PropertyValue
		{
			float32 x, y, z, w;
			int32 integer, kind;
		};
		static_assert(sizeof(PropertyValue) == 24);

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
			int32 (*componentCommand)(uint64, int32, int32, float32*);
			int32 (*componentField)(uint64, int32, const char8*, PropertyValue*, char8*, int32, int32*, int32);
			int32 (*hierarchy)(uint64, int32, uint64*, TransformState*);
			int32 (*sceneCommand)(uint64, int32, uint64*, float32*, const char8*);
			int32 (*inputQuery)(uint64, int32, int32, int32, float32*);
			int32 (*behaviour)(uint64, const char8*, int32, uint64*);
			int32 (*hostCommand)(int32, float32*, char8*, int32, int32*);
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
			int32 (*collide)(uint64, uint64) = nullptr;
			int32 (*pausedUpdates)(uint64) = nullptr;
		};
		static_assert(sizeof(NativeFunctions) == 240);
		static_assert(sizeof(ManagedFunctions) == 96);

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
			std::vector<std::pair<uint64, Component*>> pendingComponentRemoval;
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
				else if (property == 3 && entity.HasComponent<AudioPlayer>()) text = &entity.GetComponent<AudioPlayer>()->GetClipPath();
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
				else if (property == 3 && entity.HasComponent<AudioPlayer>()) entity.GetComponent<AudioPlayer>()->SetClipPath(text);
				else if (property == 4 && entity.HasComponent<ComboBox>()) entity.GetComponent<ComboBox>()->SetPrefix(text);
				else return kFailure;
				return kSuccess;
			});
		}
		Component* NativeComponent(Entity& entity, int32 kind)
		{
			switch (kind)
			{
				case 0: return entity.GetComponent<TextRenderer>();
				case 1: return entity.GetComponent<SpriteRenderer>();
				case 2: return entity.GetComponent<Camera2D>();
				case 3: return entity.GetComponent<RigidBody2D>();
				case 4: return entity.GetComponent<BoxCollider2D>();
				case 5: return entity.GetComponent<CircleCollider2D>();
				case 6: return entity.GetComponent<AudioPlayer>();
				case 7: return entity.GetComponent<ParticleComponent>();
				case 8: return entity.GetComponent<PostProcessingComponent>();
				case 9: return entity.GetComponent<Button>();
				case 10: return entity.GetComponent<CheckBox>();
				case 11: return entity.GetComponent<ComboBox>();
				case 12: return entity.GetComponent<ProgressBar>();
				case 13: return entity.GetComponent<WidgetAnchor>();
				default: return nullptr;
			}
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

		// Reflected fields remain defined by the component, not by a second scripting schema.
		class PropertyReflector final : public Reflector
		{
		public:
			PropertyReflector(const char8* name, PropertyValue& value, bool writing,
				char8* text, int32 capacity, int32& required)
				: mName(name), mValue(value), mWriting(writing), mText(text), mCapacity(capacity), mRequired(required) {}

			bool found = false;
			void Field(const char8* name, float32& value) override
			{
				if (!Matches(name, 0)) return;
				if (mWriting) value = mValue.x; else mValue.x = value;
			}
			void Field(const char8* name, int32& value) override
			{
				if (!Matches(name, 1)) return;
				if (mWriting) value = mValue.integer; else mValue.integer = value;
			}
			void Field(const char8* name, bool& value) override
			{
				if (!Matches(name, 2)) return;
				if (mWriting) value = mValue.integer != 0; else mValue.integer = value;
			}
			void Field(const char8* name, std::string& value) override
			{
				if (!Matches(name, 3)) return;
				if (mWriting) { if (!mText) throw std::invalid_argument("Missing field text."); value = mText; }
				else
				{
					mRequired = static_cast<int32>(value.size()) + 1;
					if (mText)
					{
						if (mCapacity < mRequired) throw std::invalid_argument("Field text buffer is too small.");
						std::memcpy(mText, value.c_str(), mRequired);
					}
				}
			}
			void Field(const char8* name, Vector& value) override
			{
				if (!Matches(name, 4)) return;
				if (mWriting) value = Vector(mValue.x, mValue.y, mValue.z);
				else { mValue.x = value.x; mValue.y = value.y; mValue.z = value.z; }
			}
			void FieldVector2(const char8* name, float32& x, float32& y) override
			{
				if (!Matches(name, 5)) return;
				if (mWriting) { x = mValue.x; y = mValue.y; } else { mValue.x = x; mValue.y = y; }
			}
			void FieldAsset(const char8* name, std::string& path) override
			{
				if (mWriting && std::strcmp(name, mName) == 0 && mText && mText[0] != '\0')
				{
					const std::string candidate(mText);
					const std::filesystem::path asset(candidate);
					if (asset.is_absolute() || candidate.find('\\') != std::string::npos || candidate.find(':') != std::string::npos)
						throw std::invalid_argument("Asset fields require resource-relative paths.");
					for (const auto& part : asset) if (part == ".." || part == ".") throw std::invalid_argument("Asset traversal is not supported.");
				}
				Field(name, path);
			}

		private:
			bool Matches(const char8* name, int32 kind)
			{
				if (std::strcmp(name, mName) != 0) return false;
				if (mValue.kind != kind) throw std::invalid_argument("Reflected field type mismatch.");
				found = true;
				return true;
			}
			const char8* mName;
			PropertyValue& mValue;
			bool mWriting;
			char8* mText;
			int32 mCapacity;
			int32& mRequired;
		};

		int32 ComponentField(uint64 token, int32 kind, const char8* name, PropertyValue* value,
			char8* text, int32 capacity, int32* required, int32 writing)
		{
			if (!name || !value || !required || capacity < 0 || !std::isfinite(value->x)
				|| !std::isfinite(value->y) || !std::isfinite(value->z)) return kFailure;
			return AccessEntity(token, [&](Entity& entity)
			{
				Component* component = NativeComponent(entity, kind);
				if (!component) return kInvalidLifetime;
				PropertyReflector reflector(name, *value, writing != 0, text, capacity, *required);
				component->Reflect(reflector);
				return reflector.found ? kSuccess : kFailure;
			});
		}

		int32 ComponentCommand(uint64 token, int32 kind, int32 operation, float32* values)
		{
			if (!values || kind < 0 || kind > 13) return kFailure;
			for (int32 index = 0; index < 10; index++) if (!std::isfinite(values[index])) return kFailure;
			return AccessEntity(token, [&](Entity& entity)
			{
				Component* component = NativeComponent(entity, kind);
				if (operation == 0)
				{
					static constexpr const char8* names[] = { "TextRenderer", "SpriteRenderer", "Camera2D",
						"RigidBody2D", "BoxCollider2D", "CircleCollider2D", "AudioPlayer", "ParticleComponent",
						"PostProcessingComponent", "Button", "CheckBox", "ComboBox", "ProgressBar", "WidgetAnchor" };
					values[0] = component ? 0.0f : 1.0f;
					return component || entity.AddComponentByName(names[kind]) ? kSuccess : kFailure;
				}
				if (!component) return kInvalidLifetime;
				if (operation == 1) { component->OnAwake(); return kSuccess; }
				if (operation == 2) { State().pendingComponentRemoval.emplace_back(token, component); return kSuccess; }
				if (auto* body = dynamic_cast<RigidBody2D*>(component))
				{
					switch (operation)
					{
						case 10: { const auto v = body->GetLinearVelocity(); values[0] = v.x; values[1] = v.y; break; }
						case 11: body->SetLinearVelocity({ values[0], values[1] }); break;
						case 12: body->SetPosition({ values[0], values[1] }); break;
						case 13: values[0] = static_cast<float32>(body->GetBodyType()); values[1] = body->IsFixedRotation(); break;
						case 14:
							if (values[0] < 0 || values[0] > 2) return kFailure;
							body->SetBodyType(static_cast<BodyType>(static_cast<int32>(values[0]))); body->SetFixedRotation(values[1] != 0); break;
						default: return kFailure;
					}
					return kSuccess;
				}
				if (auto* sprite = dynamic_cast<SpriteRenderer*>(component))
				{
					if (sprite->GetTexturePath().empty()) return kUnavailable;
					if (operation == 20) sprite->GetSprite().SetRegion({ values[0], values[1], 0.0f }, { values[2], values[3], 0.0f }, { values[4], values[5] });
					else if (operation == 21) sprite->GetSprite().SetColor({ values[0], values[1], values[2] });
					else if (operation == 22) { const auto size = sprite->GetSprite().GetRegionSize(); values[0] = size.width; values[1] = size.height; }
					else return kFailure;
					return kSuccess;
				}
				if (auto* audio = dynamic_cast<AudioPlayer*>(component))
				{
					if (operation == 30) values[0] = audio->Play();
					else if (operation == 31) audio->Stop();
					else if (operation == 32) values[0] = audio->IsPlaying();
					else if (operation == 33) values[0] = static_cast<float32>(audio->GetBus());
					else if (operation == 34 && values[0] >= 0 && values[0] <= 2) audio->SetBus(static_cast<AudioBus>(static_cast<int32>(values[0])));
					else if (operation == 35) { values[0] = audio->GetVolume(); values[1] = audio->GetPitch(); }
					else if (operation == 36) audio->SetVolume(values[0]);
					else if (operation == 37) audio->SetPitch(values[0]);
					else return kFailure;
					return kSuccess;
				}
				if (auto* emitter = dynamic_cast<ParticleComponent*>(component); emitter && operation == 40)
				{
					if (values[0] < 0 || values[0] > 65536) return kFailure;
					if (values[3] != 0) emitter->EmitAt({ values[1], values[2] }, static_cast<int32>(values[0]));
					else emitter->Emit(static_cast<int32>(values[0]));
					return kSuccess;
				}
				if (auto* emitter = dynamic_cast<ParticleComponent*>(component); emitter && operation == 41)
				{
					emitter->SetEmitterPosition({ values[0], values[1] }); return kSuccess;
				}
				if (auto* button = dynamic_cast<Button*>(component))
				{
					if (operation == 50) { values[0] = button->WasClicked(); values[1] = button->IsHovered(); values[2] = button->IsSelected(); }
					else if (operation == 51) button->SetSelected(values[0] != 0);
					else if (operation == 52) button->SetInteractable(values[0] != 0);
					else return kFailure;
					return kSuccess;
				}
				if (auto* check = dynamic_cast<CheckBox*>(component))
				{
					if (operation == 53) { values[0] = check->IsChecked(); values[1] = check->WasChanged(); values[2] = check->IsHovered(); }
					else if (operation == 54) check->SetChecked(values[0] != 0);
					else if (operation == 52) check->SetInteractable(values[0] != 0);
					else return kFailure;
					return kSuccess;
				}
				if (auto* combo = dynamic_cast<ComboBox*>(component))
				{
					if (operation == 55) { values[0] = static_cast<float32>(combo->GetSelectedIndex()); values[1] = combo->WasChanged(); values[2] = combo->IsOpen(); }
					else if (operation == 56) combo->SetSelectedIndex(static_cast<int32>(values[0]));
					else if (operation == 57) combo->SetOpen(values[0] != 0);
					else if (operation == 58) combo->SelectRelative(static_cast<int32>(values[0]));
					else return kFailure;
					return kSuccess;
				}
				if (auto* bar = dynamic_cast<ProgressBar*>(component))
				{
					if (operation == 59) { values[0] = bar->GetValue(); values[1] = bar->WasChanged(); }
					else if (operation == 60) bar->SetValue(values[0]);
					else if (operation == 61) bar->SetRange(values[0], values[1]);
					else return kFailure;
					return kSuccess;
				}
				if (auto* camera = dynamic_cast<Camera2D*>(component); camera && operation == 70)
				{
					const auto position = camera->GetViewPosition(), size = camera->GetViewSize();
					values[0] = position.x; values[1] = position.y; values[2] = camera->GetViewRotation(); values[3] = size.x; values[4] = size.y;
					return kSuccess;
				}
				if (auto* box = dynamic_cast<BoxCollider2D*>(component))
				{
					if (operation == 71) box->RefreshShape();
					else if (operation == 72) { values[0] = box->GetWidth(); values[1] = box->GetHeight(); values[2] = box->GetDensity(); values[3] = box->GetFriction(); values[4] = box->GetRestitution(); }
					else if (operation == 73) { box->SetWidth(values[0]); box->SetHeight(values[1]); box->SetDensity(values[2]); box->SetFriction(values[3]); box->SetRestitution(values[4]); }
					else return kFailure;
					return kSuccess;
				}
				if (auto* circle = dynamic_cast<CircleCollider2D*>(component))
				{
					if (operation == 71) circle->RefreshShape();
					else if (operation == 74) { values[0] = circle->GetRadius(); values[1] = circle->GetDensity(); values[2] = circle->GetFriction(); values[3] = circle->GetRestitution(); }
					else if (operation == 75) { circle->SetRadius(values[0]); circle->SetDensity(values[1]); circle->SetFriction(values[2]); circle->SetRestitution(values[3]); }
					else return kFailure;
					return kSuccess;
				}
				return kFailure;
			});
		}

		Reference<Entity> SharedEntity(Entity* entity)
		{
			if (!entity || !entity->GetScene()) return nullptr;
			for (const auto& candidate : entity->GetScene()->GetEntities()) if (candidate.get() == entity) return candidate;
			return nullptr;
		}
		int32 Hierarchy(uint64 token, int32 operation, uint64* other, TransformState* value)
		{
			if (!other || !value) return kFailure;
			return AccessEntity(token, [&](Entity& entity)
			{
				if (operation == 0) *other = EntityToken(SharedEntity(entity.GetParent()));
				else if (operation == 1 || operation == 2)
				{
					const auto parent = *other == 0 ? nullptr : FindEntity(*other);
					if (*other != 0 && !parent) return kInvalidLifetime;
					if (parent && (parent->GetScene() != entity.GetScene() || parent.get() == &entity || parent->IsDescendantOf(&entity))) return kFailure;
					entity.SetParent(parent.get(), operation == 1);
				}
				else if (operation == 3) *other = entity.GetChildren().size();
				else if (operation == 4)
				{
					if (*other >= entity.GetChildren().size()) return kFailure;
					*other = EntityToken(SharedEntity(entity.GetChildren()[*other]));
				}
				else if (operation == 5)
				{
					const auto p = entity.GetWorldPosition(), s = entity.GetWorldScale();
					*value = { p.x, p.y, entity.GetWorldRotation(), s.x, s.y };
				}
				else if (operation == 6)
				{
					if (!std::isfinite(value->positionX) || !std::isfinite(value->positionY) || !std::isfinite(value->rotation)
						|| !std::isfinite(value->scaleX) || !std::isfinite(value->scaleY)) return kFailure;
					entity.SetWorldPosition({ value->positionX, value->positionY }); entity.SetWorldRotation(value->rotation); entity.SetWorldScale({ value->scaleX, value->scaleY });
				}
				else if (operation == 7)
				{
					const auto before = *other == 0 ? nullptr : FindEntity(*other);
					if (*other != 0 && (!before || before->GetScene() != entity.GetScene())) return kInvalidLifetime;
					entity.GetScene()->Reorder(SharedEntity(&entity), before.get());
				}
				else if (operation == 8 || operation == 9)
				{
					const auto target = FindEntity(*other);
					if (!target) return kInvalidLifetime;
					if (operation == 8)
					{
						const auto source = entity.GetComponent<TextRenderer>(), destination = target->GetComponent<TextRenderer>();
						if (!source || !destination) return kFailure;
						source->CopyStyleTo(*destination);
					}
					else
					{
						const auto source = entity.GetComponent<Button>(), destination = target->GetComponent<Button>();
						if (!source || !destination) return kFailure;
						source->CopyVisualStyleTo(*destination);
					}
				}
				else return kFailure;
				return kSuccess;
			});
		}
		int32 SceneCommand(uint64 token, int32 operation, uint64* other, float32* values, const char8* text)
		{
			if (!other || !values) return kFailure;
			for (int32 index = 0; index < 10; index++) if (!std::isfinite(values[index])) return kFailure;
			return AccessScene(token, [&](Scene& scene)
			{
				if (operation == 0) { const auto gravity = scene.GetGravity(); values[0] = gravity.x; values[1] = gravity.y; }
				else if (operation == 1) scene.SetGravity({ values[0], values[1] });
				else if (operation == 2) *other = scene.GetEntities().size();
				else if (operation == 3)
				{
					if (*other >= scene.GetEntities().size()) return kFailure;
					auto found = scene.GetEntities().begin(); std::advance(found, *other); *other = EntityToken(*found);
				}
				else if (operation == 4 && text) *other = EntityToken(AssemblySerializer::Instantiate(scene.shared_from_this(), text));
				else if (operation == 5) values[0] = SceneManager::IsPaused();
				else if (operation == 6 && SceneManager::GetActiveScene().get() == &scene) SceneManager::SetPaused(values[0] != 0);
				else if (operation == 7)
				{
					const auto ignore = *other == 0 ? nullptr : FindEntity(*other);
					PhysicsRaycastHit hit;
					const bool found = scene.GetPhysicsWorld()->Raycast({ values[0], values[1] }, { values[2], values[3] }, hit, ignore.get());
					*other = found ? EntityToken(SharedEntity(hit.entity)) : 0;
					values[0] = hit.position.x; values[1] = hit.position.y; values[2] = hit.normal.x; values[3] = hit.normal.y; values[4] = hit.fraction;
				}
				else if (operation == 10 && values[0] >= 0 && values[0] <= 2) Audio::SetBusVolume(static_cast<AudioBus>(static_cast<int32>(values[0])), values[1]);
				else if (operation == 11 && text && values[3] >= 0 && values[3] <= 2)
				{
					const auto clip = Asset::LoadAudio(text, text);
					*other = Audio::Play(clip, { values[0], values[1], values[2] != 0, static_cast<AudioBus>(static_cast<int32>(values[3])) });
				}
				else if (operation == 12) Audio::Stop(*other);
				else if (operation == 13) values[0] = Audio::IsPlaying(*other);
				else if (operation == 14) Audio::SetVolume(*other, values[0]);
				else if (operation == 15) Audio::SetPitch(*other, values[0]);
				else if (operation == 16) values[0] = Audio::IsAvailable();
				else if (operation == 17) Audio::StopAll();
				else if (operation == 18 && values[0] >= 0 && values[0] <= 2) values[0] = Audio::GetBusVolume(static_cast<AudioBus>(static_cast<int32>(values[0])));
				else if (operation == 20 || operation == 21)
				{
					const int32 kind = static_cast<int32>(values[0]);
					if (kind < 0 || kind > 13) return kFailure;
					*other = 0; values[0] = 0;
					for (const auto& entity : scene.GetEntities())
						if (NativeComponent(*entity, kind))
						{
							if (operation == 20) { *other = EntityToken(entity); break; }
							if (entity->IsActive()) values[0]++;
						}
				}
				else return kFailure;
				return kSuccess;
			});
		}
		int32 InputQuery(uint64 action, int32 operation, int32 code, int32 gamepad, float32* values)
		{
			const int32 status = CheckRuntime();
			if (status != kSuccess || !values) return status == kSuccess ? kFailure : status;
			if (operation != 0 && operation != 2 && !Window::IsAvailable()) return kUnavailable;
			if (operation == 0)
			{
				const auto found = State().actions.find(action);
				if (found == State().actions.end()) return kInvalidLifetime;
				values[0] = Input::GetActionTap(found->second);
			}
			else if (operation == 1) { const auto pointer = Input::GetPointerPosition(); values[0] = pointer.x; values[1] = pointer.y; }
			else if (operation == 2) values[0] = static_cast<float32>(Input::GetLastInputMethod());
			else if (operation == 3 && gamepad >= 0 && gamepad < 16) values[0] = Input::IsGamepadConnected(gamepad);
			else if (operation == 4 && gamepad >= 0 && gamepad < 16 && code >= 0 && code <= 14) values[0] = Input::GetGamepadButtonPress(static_cast<GamepadButton>(code), gamepad);
			else if (operation == 5 && gamepad >= 0 && gamepad < 16 && code >= 0 && code <= 5) values[0] = Input::GetGamepadAxis(static_cast<GamepadAxis>(code), gamepad);
			else if (operation == 6 && code >= 0 && code <= GLFW_KEY_LAST) values[0] = Input::GetKeyPress(static_cast<KeyCode>(code));
			else if (operation == 7 && code >= 0 && code <= GLFW_KEY_LAST) values[0] = Input::GetKeyTap(static_cast<KeyCode>(code));
			else if (operation == 8 && code >= 0 && code <= GLFW_MOUSE_BUTTON_LAST) values[0] = Input::GetMouseButtonPress(code);
			else return kFailure;
			return kSuccess;
		}

		int32 BehaviourCommand(uint64 token, const char8* name, int32 operation, uint64* instance)
		{
			if (!name || !instance) return kFailure;
			return AccessEntity(token, [&](Entity& entity)
			{
				CSharpScript* script = nullptr;
				for (const auto& component : entity.GetComponents())
					if (component->GetTypeName() == name) script = dynamic_cast<CSharpScript*>(component.get());
				if (operation == 1)
				{
					if (script) return kFailure;
					script = dynamic_cast<CSharpScript*>(entity.AddComponentByName(name));
					if (!script) return kFailure;
					script->OnAwake();
				}
				else if (operation == 2 && script) State().pendingComponentRemoval.emplace_back(token, script);
				else if (operation != 0 && operation != 2) return kFailure;
				*instance = script ? script->GetInstance() : 0;
				return kSuccess;
			});
		}

		int32 HostCommand(int32 operation, float32* values, char8* text, int32 capacity, int32* required)
		{
			const int32 status = CheckRuntime();
			if (status != kSuccess || !values || !required || capacity < 0) return status == kSuccess ? kFailure : status;
			try
			{
				for (int32 index = 0; index < 10; index++) if (!std::isfinite(values[index])) return kFailure;
				if (operation == 0) { values[0] = Application::IsEditor(); return kSuccess; }
				if (operation == 13)
				{
					if (!text) return kFailure;
					const auto path = ResolveResourcePath(text);
					if (std::filesystem::file_size(path) > 96 * 1024 * 1024) return kFailure;
					std::ifstream file(path, std::ios::binary);
					if (!file) return kFailure;
					const std::string contents((std::istreambuf_iterator<char8>(file)), std::istreambuf_iterator<char8>());
					const std::string data = Vault::Unseal(contents);
					if (data.size() > 64 * 1024 * 1024) return kFailure;
					*required = static_cast<int32>(data.size());
					if (values[0] != 0)
					{
						// The path has already been consumed, so the managed buffer can now receive decoded bytes.
						if (capacity < *required) return kFailure;
						std::memcpy(text, data.data(), data.size());
					}
					return kSuccess;
				}
				if (!Window::IsAvailable()) return kUnavailable;
				const bool mutating = operation == 2 || operation == 4 || operation == 5 || operation == 6 || operation == 7 || operation == 9 || operation == 11 || operation == 12;
				if (mutating && Application::IsEditor()) return kUnavailable;
				if (operation == 1) { const auto size = Window::GetSize(); values[0] = size.width; values[1] = size.height; }
				else if (operation == 2 && values[0] >= 1 && values[0] <= 16384 && values[1] >= 1 && values[1] <= 16384) Window::SetSize(static_cast<uint32>(values[0]), static_cast<uint32>(values[1]));
				else if (operation == 3) { const auto color = Window::GetBackgroundColor(); values[0] = color[0]; values[1] = color[1]; values[2] = color[2]; }
				else if (operation == 4) Window::SetBackgroundColor(values[0], values[1], values[2]);
				else if (operation == 5) Window::SetResizable(values[0] != 0);
				else if (operation == 6) Window::SetMaximized(values[0] != 0);
				else if (operation == 7) Window::Minimize();
				else if (operation == 8) values[0] = Clock::GetShowFrameStats();
				else if (operation == 9) Clock::SetShowFrameStats(values[0] != 0);
				else if (operation == 10)
				{
					const std::string title = Window::GetTitle(); *required = static_cast<int32>(title.size()) + 1;
					if (text) { if (capacity < *required) return kFailure; std::memcpy(text, title.c_str(), *required); }
				}
				else if (operation == 11 && text) Window::SetTitle(text);
				else if (operation == 12 && text) Window::SetIcon(text);
				else if (operation == 14) values[0] = Window::IsMaximized();
				else return kFailure;
				return kSuccess;
			}
			catch (...) { return kFailure; }
		}

		const NativeFunctions kNativeFunctions = { kAbiVersion, sizeof(NativeFunctions), ValidateEntity,
			GetTransform, SetTransform, ResolveAction, ReadAction, IsLogEnabled, WriteLog, ReportError,
			SceneOf, ValidateScene, FindSceneEntity, CreateSceneEntity, DestroyEntity, GetEntityState,
			SetEntityState, ReadText, WriteText, HasNativeComponent, GetComponentState, SetComponentState, RequestScene, Quit,
			ComponentCommand, ComponentField, Hierarchy, SceneCommand, InputQuery, BehaviourCommand, HostCommand };
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
			|| !functions.describe || !functions.readFields || !functions.writeField || !functions.collide || !functions.pausedUpdates)
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

	void CSharpRuntime::Collide(uint64 instance, Entity& other)
	{
		if (instance != 0 && CheckRuntime() == kSuccess && State().gameplayActive)
			State().managed.collide(instance, EntityToken(SharedEntity(&other)));
	}

	bool CSharpRuntime::UpdatesWhenPaused(uint64 instance)
	{
		return instance != 0 && CheckRuntime() == kSuccess && State().managed.pausedUpdates(instance) != 0;
	}

	void CSharpRuntime::FlushComponentRemovals()
	{
		if (CheckRuntime() != kSuccess) return;
		std::vector<std::pair<uint64, Component*>> pending;
		pending.swap(State().pendingComponentRemoval);
		for (const auto& [token, component] : pending)
		{
			const auto entity = FindEntity(token);
			if (!entity) continue;
			const auto& components = entity->GetComponents();
			if (std::any_of(components.begin(), components.end(), [&](const auto& current) { return current.get() == component; }))
				entity->RemoveComponent(component);
		}
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
		state.pendingComponentRemoval.clear();
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
