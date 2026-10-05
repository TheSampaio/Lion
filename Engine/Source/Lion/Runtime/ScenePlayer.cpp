#include "Engine.h"
#include "ScenePlayer.h"

#include <Lion/Core/Application.h>
#include <Lion/Core/Filesystem.h>
#include <Lion/Core/GameModule.h>
#include <Lion/Core/Layer.h>
#include <Lion/Core/Vault.h>
#include <Lion/Core/Window.h>
#include <Lion/Logic/Scene.h>
#include <Lion/Logic/SceneManager.h>
#include <Lion/Render/Camera2D.h>
#include <Lion/Render/CameraOrthographic.h>
#include <Lion/Render/PostProcessingComponent.h>
#include <Lion/Render/Renderer.h>
#include <Lion/Scripting/CSharpRuntime.h>

#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

namespace Lion
{
	namespace
	{
		class ScenePlayerLayer final : public Layer
		{
		public:
			~ScenePlayerLayer() override { OnDetach(); }

			void OnCreate() override
			{
				const std::filesystem::path root = ResourceRootDirectory();
				std::ifstream file(root / kPlayerSettingsFile, std::ios::binary);
				if (!file) throw std::runtime_error("The player configuration is missing.");
				const std::string contents((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
				const auto settings = nlohmann::json::parse(Vault::Unseal(contents));
				Window::SetTitle(settings.at("name").get<std::string>());
				if (settings.value("managed", false))
				{
					const std::string runtimeVersion = settings.at("runtimeVersion").get<std::string>();
					const std::filesystem::path version(runtimeVersion);
					if (version != version.filename() || runtimeVersion.empty() || runtimeVersion == "." || runtimeVersion == "..")
						throw std::runtime_error("The managed runtime version is invalid.");
					std::string error;
					if (!CSharpRuntime::Initialize((root / "Managed").string(),
						(root / "Dotnet" / "host" / "fxr" / version / "hostfxr.dll").string(), error))
						throw std::runtime_error(error);
					mManaged = true;
					CSharpRuntime::SetGameplayActive(true);
					if (!CSharpRuntime::LoadAssembly((root / "Managed" / "lion-scripts.dll").string(), error))
						throw std::runtime_error(error);
				}
				mCamera = MakeReference<CameraOrthographic>();
				if (!SceneManager::LoadScene(settings.at("scene").get<std::string>()))
					throw std::runtime_error("The player's entry scene could not be loaded.");
			}

			void OnUpdate() override { SceneManager::Update(); }

			void OnRender() override
			{
				const Reference<Scene> scene = SceneManager::GetActiveScene();
				if (!scene || !mCamera) return;
				const Size size = Window::GetSize();
				mCamera->OnResize(size.width, size.height);
				Camera2D* camera = scene->FindComponent<Camera2D>();
				PostProcessingComponent* postProcessing = nullptr;
				if (camera && camera->IsEnabled())
				{
					const auto position = camera->GetViewPosition();
					mCamera->SetPosition(glm::vec3(position.x, position.y, 0.0f));
					mCamera->SetZoomLevel(camera->GetZoomForViewportHeight(size.height));
					postProcessing = camera->GetOwner().GetComponent<PostProcessingComponent>();
				}
				const bool processed = Renderer::BeginPostProcessing(postProcessing,
					static_cast<uint32>(size.width), static_cast<uint32>(size.height));
				Renderer::RenderBegin(mCamera);
				scene->OnRender();
				Renderer::RenderEnd();
				if (processed) Renderer::EndPostProcessing();
			}

			void OnDetach() override
			{
				SceneManager::Clear();
				if (mManaged)
				{
					std::string ignored;
					CSharpRuntime::Shutdown(ignored);
					mManaged = false;
				}
			}

		private:
			Reference<CameraOrthographic> mCamera;
			bool mManaged = false;
		};
	}

	Application* CreateScenePlayer()
	{
		auto* application = new Application();
		application->PushLayer(new ScenePlayerLayer());
		return application;
	}
}
