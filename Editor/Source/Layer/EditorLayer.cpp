#include "EditorLayer.h"

#include <imgui/imgui_internal.h> // DockBuilder API for the default layout.

using namespace Lion;

void EditorLayer::OnAttach()
{
	Window::SetSize(1280, 720);
	Window::SetTitle("Lion Editor");
	Window::SetBackgroundColor(0.10f, 0.10f, 0.11f);
	Window::SetResizable(true);
}

void EditorLayer::OnCreate()
{
	mCamera = MakeReference<CameraOrthographic>();
	mScene = MakeReference<Scene>();

	FramebufferSpecification spec;
	spec.width = 1280;
	spec.height = 720;
	mFramebuffer = Framebuffer::Create(spec);

	CreateDemoScene();
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

void EditorLayer::OnRenderUI()
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

	ImGui::Begin("Scene Hierarchy");
	ImGui::TextDisabled("Demo scene");
	ImGui::BulletText("Background");
	ImGui::BulletText("Bricks x5");
	ImGui::BulletText("Ball");
	ImGui::End();

	ImGui::Begin("Properties");
	ImGui::TextDisabled("Select an entity to edit its components.");
	ImGui::End();

	ImGui::Begin("Statistics");
	const ImGuiIO& io = ImGui::GetIO();
	ImGui::Text("FPS:   %.1f", io.Framerate);
	ImGui::Text("Frame: %.3f ms", 1000.0f / io.Framerate);
	ImGui::Separator();
	ImGui::Text("Viewport: %.0f x %.0f", mViewportSize.x, mViewportSize.y);
	ImGui::End();

	if (mShowDemo)
		ImGui::ShowDemoWindow(&mShowDemo);
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

	ImGui::End();
	ImGui::PopStyleVar();
}

void EditorLayer::DrawMenuBar()
{
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
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
	ImGui::DockBuilderDockWindow("Statistics", bottom);
	ImGui::DockBuilderDockWindow("Viewport", center);

	ImGui::DockBuilderFinish(dockspaceId);
}
