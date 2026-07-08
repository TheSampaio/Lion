#include "EditorLayer.h"

#include "../EditorGui.h"

#include <imgui/imgui_internal.h> // DockBuilder API for the default layout.

using namespace Lion;

void EditorLayer::OnAttach()
{
	Window::SetSize(1280, 720);
	Window::SetTitle("Lion Editor");
	Window::SetBackgroundColor(0.10f, 0.10f, 0.11f);
	Window::SetResizable(true);
	Window::SetMaximized(true);
}

void EditorLayer::OnCreate()
{
	EditorGui::Init();

	mCamera = MakeReference<CameraOrthographic>();
	mScene = MakeReference<Scene>();

	FramebufferSpecification spec;
	spec.width = 1280;
	spec.height = 720;
	mFramebuffer = Framebuffer::Create(spec);

	CreateDemoScene();
}

void EditorLayer::OnDetach()
{
	EditorGui::Shutdown();
}

void EditorLayer::CreateDemoScene()
{
	// Background.
	auto background = MakeReference<Entity>();
	background->GetTransform()->SetPosition(Vector(0.0f, 0.0f, Depth::Back));
	background->AddComponent<SpriteRenderer>("Sprite/Brickout/background.jpg");
	mScene->Add(background);

	// A row of bricks.
	for (int32 i = 0; i < 5; i++)
	{
		auto brick = MakeReference<Entity>();
		brick->GetTransform()->SetPosition(Vector(-160.0f + i * 80.0f, 60.0f, Depth::Middle));
		brick->AddComponent<SpriteRenderer>("Sprite/Brickout/tile-" + std::to_string(i + 1) + ".png");
		mScene->Add(brick);
	}

	// Ball.
	auto ball = MakeReference<Entity>();
	ball->GetTransform()->SetPosition(Vector(0.0f, -80.0f, Depth::Middle));
	ball->AddComponent<SpriteRenderer>("Sprite/Brickout/ball.png");
	mScene->Add(ball);
}

void EditorLayer::OnRender()
{
	// The scene is rendered into the framebuffer first, then the editor UI is drawn on top of
	// the window with the framebuffer shown inside the Viewport panel.
	RenderScene();

	EditorGui::BeginFrame();
	ImGuizmo::BeginFrame();

	DrawUI();

	EditorGui::EndFrame();
}

void EditorLayer::RenderScene()
{
	// Match the framebuffer/camera to the viewport panel size measured last frame.
	const FramebufferSpecification& spec = mFramebuffer->GetSpecification();
	const uint32 targetWidth = static_cast<uint32>(mViewportSize.x);
	const uint32 targetHeight = static_cast<uint32>(mViewportSize.y);

	if (targetWidth > 0 && targetHeight > 0 && (targetWidth != spec.width || targetHeight != spec.height))
	{
		mFramebuffer->Resize(targetWidth, targetHeight);
		mCamera->OnResize(mViewportSize.x, mViewportSize.y);
	}

	// Render the scene into the framebuffer instead of the window.
	mFramebuffer->Bind();
	Renderer::Clear(0.12f, 0.12f, 0.15f, 1.0f);

	Renderer::RenderBegin(mCamera);
	mScene->OnRender();
	Renderer::RenderEnd();

	mFramebuffer->Unbind();
}

void EditorLayer::DrawUI()
{
	DrawMenuBar();

	// Fullscreen, borderless host window that holds the dockspace (below the main menu bar).
	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);
	ImGui::SetNextWindowViewport(viewport->ID);

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

	const ImGuiWindowFlags hostFlags =
		ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
		ImGuiWindowFlags_NoNavFocus;

	ImGui::Begin("LionEditorDockHost", nullptr, hostFlags);
	ImGui::PopStyleVar(3);

	const ImGuiID dockspaceId = ImGui::GetID("LionEditorDockspace");

	if (!mLayoutInitialized)
	{
		mLayoutInitialized = true;

		if (ImGui::DockBuilderGetNode(dockspaceId) == nullptr)
			BuildDefaultLayout(dockspaceId);
	}

	ImGui::DockSpace(dockspaceId);
	ImGui::End();

	// --- Panels -----------------------------------------------------------------------------

	DrawViewport();
	DrawHierarchy();
	DrawProperties();

	ImGui::Begin("Statistics");
	const ImGuiIO& io = ImGui::GetIO();
	ImGui::Text("FPS:   %.1f", io.Framerate);
	ImGui::Text("Frame: %.3f ms", 1000.0f / io.Framerate);
	ImGui::Separator();
	ImGui::Text("Viewport: %.0f x %.0f", mViewportSize.x, mViewportSize.y);
	ImGui::End();

	DrawConsole();

	if (mShowDemo)
		ImGui::ShowDemoWindow(&mShowDemo);
}

namespace
{
	// Display color for each log severity, mirroring the spdlog console coloring.
	ImVec4 LogLevelColor(LogLevel level)
	{
		switch (level)
		{
			case LogLevel::Error:       return ImVec4(0.94f, 0.35f, 0.35f, 1.0f);
			case LogLevel::Fatal:       return ImVec4(1.00f, 0.30f, 0.55f, 1.0f);
			case LogLevel::Warning:     return ImVec4(0.95f, 0.80f, 0.35f, 1.0f);
			case LogLevel::Success:     return ImVec4(0.45f, 0.85f, 0.50f, 1.0f);
			case LogLevel::Information: return ImVec4(0.60f, 0.75f, 0.95f, 1.0f);
			case LogLevel::Trace:       return ImVec4(0.65f, 0.65f, 0.65f, 1.0f);
		}

		return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
	}
}

void EditorLayer::DrawConsole()
{
	ImGui::Begin("Console");

	// Toolbar: clear the history, follow new output, and filter by substring.
	if (ImGui::Button("Clear"))
		Log::ClearHistory();

	ImGui::SameLine();
	ImGui::Checkbox("Auto-scroll", &mConsoleAutoScroll);

	ImGui::SameLine();
	static ImGuiTextFilter filter;
	filter.Draw("Filter", 180.0f);

	ImGui::Separator();

	ImGui::BeginChild("ConsoleOutput", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_HorizontalScrollbar);

	for (const LogEntry& entry : Log::GetHistory())
	{
		const std::string line = "[" + entry.time + "] " + entry.message;

		if (!filter.PassFilter(line.c_str()))
			continue;

		ImGui::PushStyleColor(ImGuiCol_Text, LogLevelColor(entry.level));
		ImGui::TextUnformatted(line.c_str());
		ImGui::PopStyleColor();
	}

	// Keep following the tail while the view is already scrolled to the bottom.
	if (mConsoleAutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
		ImGui::SetScrollHereY(1.0f);

	ImGui::EndChild();
	ImGui::End();
}

void EditorLayer::DrawViewport()
{
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin("Viewport");

	const ImVec2 available = ImGui::GetContentRegionAvail();
	mViewportSize = { available.x, available.y };

	// Display the framebuffer's color texture, flipped vertically (OpenGL is bottom-up).
	const auto textureId = static_cast<ImTextureID>(mFramebuffer->GetColorAttachment());
	ImGui::Image(textureId, available, ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));

	const ImVec2 imageMin = ImGui::GetItemRectMin();
	const ImVec2 imageSize = ImGui::GetItemRectSize();

	// W/E/R switch the gizmo between move / rotate / scale (unless typing or dragging).
	if (!ImGuizmo::IsUsing() && !ImGui::IsAnyItemActive())
	{
		if (ImGui::IsKeyPressed(ImGuiKey_W)) mGizmoOperation = ImGuizmo::TRANSLATE;
		if (ImGui::IsKeyPressed(ImGuiKey_E)) mGizmoOperation = ImGuizmo::ROTATE;
		if (ImGui::IsKeyPressed(ImGuiKey_R)) mGizmoOperation = ImGuizmo::SCALE;
	}

	if (mSelectedEntity)
	{
		ImGuizmo::SetOrthographic(true);
		ImGuizmo::SetDrawlist();
		ImGuizmo::SetRect(imageMin.x, imageMin.y, imageSize.x, imageSize.y);

		const glm::mat4 view = mCamera->GetViewMatrix();
		const glm::mat4 projection = mCamera->GetProjectionMatrix();

		const Reference<Transform> transform = mSelectedEntity->GetTransform();
		const Vector position = transform->GetPosition();
		const Vector rotation = transform->GetRotation();
		const Vector scale = transform->GetScale();

		// Build the entity's model matrix (rotation in degrees, matching the Transform).
		float32 translationValues[3] = { position.x, position.y, position.z };
		float32 rotationValues[3] = { 0.0f, 0.0f, rotation.z };
		float32 scaleValues[3] = { scale.x, scale.y, 1.0f };

		float32 model[16];
		ImGuizmo::RecomposeMatrixFromComponents(translationValues, rotationValues, scaleValues, model);

		ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(projection), mGizmoOperation, ImGuizmo::LOCAL, model);

		if (ImGuizmo::IsUsing())
		{
			float32 newTranslation[3], newRotation[3], newScale[3];
			ImGuizmo::DecomposeMatrixToComponents(model, newTranslation, newRotation, newScale);

			transform->SetPosition(Vector(newTranslation[0], newTranslation[1], position.z));
			transform->SetRotation(Vector(0.0f, 0.0f, newRotation[2]));
			transform->SetScale(Vector(newScale[0], newScale[1], 1.0f));
		}
	}

	ImGui::End();
	ImGui::PopStyleVar();
}

void EditorLayer::DrawHierarchy()
{
	ImGui::Begin("Scene Hierarchy");

	if (ImGui::Button("Add Entity"))
	{
		auto entity = MakeReference<Entity>();
		mScene->Add(entity);
		mSelectedEntity = entity;
	}

	if (mSelectedEntity)
	{
		ImGui::SameLine();

		if (ImGui::Button("Delete"))
		{
			mScene->Remove(mSelectedEntity);
			mScene->FlushRemovals();
			mSelectedEntity = nullptr;
		}
	}

	ImGui::Separator();

	int32 index = 0;
	for (const auto& entity : mScene->GetEntities())
	{
		ImGui::PushID(index);

		const std::string label = "Entity " + std::to_string(index);
		if (ImGui::Selectable(label.c_str(), entity == mSelectedEntity))
			mSelectedEntity = entity;

		ImGui::PopID();
		index++;
	}

	if (index == 0)
		ImGui::TextDisabled("Empty scene");

	// Click empty space inside the panel to deselect.
	if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
		mSelectedEntity = nullptr;

	ImGui::End();
}

void EditorLayer::DrawProperties()
{
	ImGui::Begin("Properties");

	if (!mSelectedEntity)
	{
		ImGui::TextDisabled("Select an entity to edit its components.");
		ImGui::End();
		return;
	}

	if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
	{
		const Reference<Transform> transform = mSelectedEntity->GetTransform();

		Vector position = transform->GetPosition();
		float32 positionValues[3] = { position.x, position.y, position.z };
		if (ImGui::DragFloat3("Position", positionValues, 1.0f))
			transform->SetPosition(Vector(positionValues[0], positionValues[1], positionValues[2]));

		Vector rotation = transform->GetRotation();
		float32 rotationValues[3] = { rotation.x, rotation.y, rotation.z };
		if (ImGui::DragFloat3("Rotation", rotationValues, 0.5f))
			transform->SetRotation(Vector(rotationValues[0], rotationValues[1], rotationValues[2]));

		Vector scale = transform->GetScale();
		float32 scaleValues[3] = { scale.x, scale.y, scale.z };
		if (ImGui::DragFloat3("Scale", scaleValues, 0.01f))
			transform->SetScale(Vector(scaleValues[0], scaleValues[1], scaleValues[2]));
	}

	if (const SpriteRenderer* renderer = mSelectedEntity->GetComponent<SpriteRenderer>())
	{
		if (ImGui::CollapsingHeader("Sprite Renderer", ImGuiTreeNodeFlags_DefaultOpen))
			ImGui::Text("Texture: %s", renderer->GetTexturePath().c_str());
	}

	if (const RigidBody2D* body = mSelectedEntity->GetComponent<RigidBody2D>())
	{
		if (ImGui::CollapsingHeader("Rigid Body 2D", ImGuiTreeNodeFlags_DefaultOpen))
		{
			static const char8* bodyTypes[] = { "Static", "Kinematic", "Dynamic" };
			ImGui::Text("Type: %s", bodyTypes[static_cast<int32>(body->GetBodyType())]);
			ImGui::Text("Fixed Rotation: %s", body->IsFixedRotation() ? "true" : "false");
		}
	}

	if (const BoxCollider2D* collider = mSelectedEntity->GetComponent<BoxCollider2D>())
	{
		if (ImGui::CollapsingHeader("Box Collider 2D", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Text("Size: %.1f x %.1f", collider->GetWidth(), collider->GetHeight());
			ImGui::Text("Density: %.2f  Friction: %.2f  Restitution: %.2f",
				collider->GetDensity(), collider->GetFriction(), collider->GetRestitution());
		}
	}

	if (const CircleCollider2D* collider = mSelectedEntity->GetComponent<CircleCollider2D>())
	{
		if (ImGui::CollapsingHeader("Circle Collider 2D", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Text("Radius: %.1f", collider->GetRadius());
			ImGui::Text("Density: %.2f  Friction: %.2f  Restitution: %.2f",
				collider->GetDensity(), collider->GetFriction(), collider->GetRestitution());
		}
	}

	ImGui::End();
}

void EditorLayer::DrawMenuBar()
{
	static const char8* sceneFilter = "Lion Scene (*.json)\0*.json\0";

	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("New Scene"))
				mScene->Clear();

			if (ImGui::MenuItem("Open Scene..."))
			{
				const std::string path = FileDialog::Open(sceneFilter);

				if (!path.empty())
					SceneSerializer::Deserialize(mScene, path);
			}

			if (ImGui::MenuItem("Save Scene As..."))
			{
				const std::string path = FileDialog::Save(sceneFilter, "json");

				if (!path.empty())
					SceneSerializer::Serialize(mScene, path);
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Exit", "Alt+F4"))
				Window::RequestClose();

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("View"))
		{
			ImGui::MenuItem("ImGui Demo Window", nullptr, &mShowDemo);
			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}
}

void EditorLayer::BuildDefaultLayout(unsigned int dockspaceId)
{
	ImGui::DockBuilderRemoveNode(dockspaceId);
	ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
	ImGui::DockBuilderSetNodeSize(dockspaceId, ImGui::GetMainViewport()->WorkSize);

	ImGuiID center = dockspaceId;
	const ImGuiID left   = ImGui::DockBuilderSplitNode(center, ImGuiDir_Left,  0.20f, nullptr, &center);
	const ImGuiID right  = ImGui::DockBuilderSplitNode(center, ImGuiDir_Right, 0.25f, nullptr, &center);
	const ImGuiID bottom = ImGui::DockBuilderSplitNode(center, ImGuiDir_Down,  0.25f, nullptr, &center);

	ImGui::DockBuilderDockWindow("Scene Hierarchy", left);
	ImGui::DockBuilderDockWindow("Properties", right);
	ImGui::DockBuilderDockWindow("Console", bottom);
	ImGui::DockBuilderDockWindow("Statistics", bottom);
	ImGui::DockBuilderDockWindow("Viewport", center);

	ImGui::DockBuilderFinish(dockspaceId);
}
