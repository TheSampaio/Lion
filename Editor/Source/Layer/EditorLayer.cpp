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

void EditorLayer::OnUpdate()
{
	// Only advance the scene simulation (physics + entity scripts) while in Play mode.
	if (mPlaying)
		mScene->OnUpdate();
}

void EditorLayer::OnDetach()
{
	EditorGui::Shutdown();
}

void EditorLayer::StartPlay()
{
	if (mPlaying)
		return;

	// Save the edited scene, then rebuild it so every component runs OnAwake again (which creates
	// the Box2D bodies/shapes needed for the simulation).
	mPlaySnapshot = SceneSerializer::SerializeToString(mScene);
	mSelectedEntity = nullptr;
	SceneSerializer::DeserializeFromString(mScene, mPlaySnapshot);

	mPlaying = true;
	Log::Console(LogLevel::Information, "[Editor] Play mode started.");
}

void EditorLayer::StopPlay()
{
	if (!mPlaying)
		return;

	// Restore the scene to exactly the edited state captured when Play started.
	mSelectedEntity = nullptr;
	SceneSerializer::DeserializeFromString(mScene, mPlaySnapshot);

	mPlaying = false;
	Log::Console(LogLevel::Information, "[Editor] Play mode stopped.");
}

void EditorLayer::CreateDemoScene()
{
	// Background.
	auto background = MakeReference<Entity>();
	background->SetName("Background");
	background->GetTransform()->SetPosition(Vector(0.0f, 0.0f, Depth::Back));
	background->AddComponent<SpriteRenderer>("Sprite/Brickout/background.jpg");
	mScene->Add(background);

	// A row of bricks.
	for (int32 i = 0; i < 5; i++)
	{
		auto brick = MakeReference<Entity>();
		brick->SetName("Brick " + std::to_string(i + 1));
		brick->GetTransform()->SetPosition(Vector(-160.0f + i * 80.0f, 60.0f, Depth::Middle));
		brick->AddComponent<SpriteRenderer>("Sprite/Brickout/tile-" + std::to_string(i + 1) + ".png");
		mScene->Add(brick);
	}

	// Ball (with physics, so pressing Play drops it under gravity).
	auto ball = MakeReference<Entity>();
	ball->SetName("Ball");
	ball->GetTransform()->SetPosition(Vector(0.0f, 120.0f, Depth::Middle));
	ball->AddComponent<SpriteRenderer>("Sprite/Brickout/ball.png");
	ball->AddComponent<RigidBody2D>(BodyType::Dynamic, false);
	ball->AddComponent<CircleCollider2D>(16.0f);
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
	HandleShortcuts();

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

	// Commit any in-progress continuous edit (gizmo/slider drag) once the mouse is released.
	if (mHasPending && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
		CommitEdit();

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

void EditorLayer::RecordSnapshot()
{
	// Snapshot the current state onto the undo stack (used right before a discrete edit).
	mUndoStack.push_back(SceneSerializer::SerializeToString(mScene));

	if (mUndoStack.size() > kMaxUndo)
		mUndoStack.erase(mUndoStack.begin());

	mRedoStack.clear();
}

void EditorLayer::BeginEdit()
{
	// Capture the pre-edit state once at the start of a continuous edit (gizmo/slider drag).
	if (mHasPending)
		return;

	mPendingSnapshot = SceneSerializer::SerializeToString(mScene);
	mHasPending = true;
}

void EditorLayer::CommitEdit()
{
	if (!mHasPending)
		return;

	mHasPending = false;

	// Only record a step if the edit actually changed the scene.
	if (SceneSerializer::SerializeToString(mScene) == mPendingSnapshot)
		return;

	mUndoStack.push_back(mPendingSnapshot);

	if (mUndoStack.size() > kMaxUndo)
		mUndoStack.erase(mUndoStack.begin());

	mRedoStack.clear();
}

void EditorLayer::Undo()
{
	if (mUndoStack.empty())
		return;

	mRedoStack.push_back(SceneSerializer::SerializeToString(mScene));

	const std::string state = mUndoStack.back();
	mUndoStack.pop_back();

	// The scene is rebuilt from scratch, so any selected-entity pointer becomes stale.
	mSelectedEntity = nullptr;
	SceneSerializer::DeserializeFromString(mScene, state);
}

void EditorLayer::Redo()
{
	if (mRedoStack.empty())
		return;

	mUndoStack.push_back(SceneSerializer::SerializeToString(mScene));

	const std::string state = mRedoStack.back();
	mRedoStack.pop_back();

	mSelectedEntity = nullptr;
	SceneSerializer::DeserializeFromString(mScene, state);
}

void EditorLayer::HandleShortcuts()
{
	// F5 / F8 toggle Play mode (available even while a text field is focused).
	if (ImGui::IsKeyPressed(ImGuiKey_F5, false)) StartPlay();
	if (ImGui::IsKeyPressed(ImGuiKey_F8, false)) StopPlay();

	const ImGuiIO& io = ImGui::GetIO();

	// Undo/redo is an edit-mode action; ignore it while playing or typing in a text field.
	if (mPlaying || io.WantTextInput)
		return;

	if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z, false))
		Undo();

	if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y, false))
		Redo();
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
	if (!mPlaying && !ImGuizmo::IsUsing() && !ImGui::IsAnyItemActive())
	{
		if (ImGui::IsKeyPressed(ImGuiKey_W)) mGizmoOperation = ImGuizmo::TRANSLATE;
		if (ImGui::IsKeyPressed(ImGuiKey_E)) mGizmoOperation = ImGuizmo::ROTATE;
		if (ImGui::IsKeyPressed(ImGuiKey_R)) mGizmoOperation = ImGuizmo::SCALE;
	}

	// The gizmo is an editing tool; hide it while the simulation is running.
	if (mSelectedEntity && !mPlaying)
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

		// Capture the pre-edit state the moment the gizmo is grabbed (before Manipulate mutates it).
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGuizmo::IsOver())
			BeginEdit();

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
		RecordSnapshot();

		auto entity = MakeReference<Entity>();
		mScene->Add(entity);
		mSelectedEntity = entity;
	}

	if (mSelectedEntity)
	{
		ImGui::SameLine();

		if (ImGui::Button("Delete"))
		{
			RecordSnapshot();

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

		const std::string& name = entity->GetName();
		if (ImGui::Selectable(name.empty() ? "(unnamed)" : name.c_str(), entity == mSelectedEntity))
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

	// Editable entity name.
	char nameBuffer[128];
	const std::string& currentName = mSelectedEntity->GetName();
	const size_t copied = currentName.copy(nameBuffer, sizeof(nameBuffer) - 1);
	nameBuffer[copied] = '\0';

	if (ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer)))
		mSelectedEntity->SetName(nameBuffer);
	if (ImGui::IsItemActivated()) BeginEdit();
	if (ImGui::IsItemDeactivatedAfterEdit()) CommitEdit();

	ImGui::Separator();

	if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
	{
		const Reference<Transform> transform = mSelectedEntity->GetTransform();

		Vector position = transform->GetPosition();
		float32 positionValues[3] = { position.x, position.y, position.z };
		if (ImGui::DragFloat3("Position", positionValues, 1.0f))
			transform->SetPosition(Vector(positionValues[0], positionValues[1], positionValues[2]));
		if (ImGui::IsItemActivated()) BeginEdit();
		if (ImGui::IsItemDeactivatedAfterEdit()) CommitEdit();

		Vector rotation = transform->GetRotation();
		float32 rotationValues[3] = { rotation.x, rotation.y, rotation.z };
		if (ImGui::DragFloat3("Rotation", rotationValues, 0.5f))
			transform->SetRotation(Vector(rotationValues[0], rotationValues[1], rotationValues[2]));
		if (ImGui::IsItemActivated()) BeginEdit();
		if (ImGui::IsItemDeactivatedAfterEdit()) CommitEdit();

		Vector scale = transform->GetScale();
		float32 scaleValues[3] = { scale.x, scale.y, scale.z };
		if (ImGui::DragFloat3("Scale", scaleValues, 0.01f))
			transform->SetScale(Vector(scaleValues[0], scaleValues[1], scaleValues[2]));
		if (ImGui::IsItemActivated()) BeginEdit();
		if (ImGui::IsItemDeactivatedAfterEdit()) CommitEdit();
	}

	// Components can request removal here; the removal itself is deferred until after drawing so we
	// never delete a component while its section is still being rendered.
	bool removeSprite = false, removeBody = false, removeBox = false, removeCircle = false;

	if (SpriteRenderer* renderer = mSelectedEntity->GetComponent<SpriteRenderer>())
	{
		if (ImGui::CollapsingHeader("Sprite Renderer", ImGuiTreeNodeFlags_DefaultOpen))
		{
			// Reload the texture only once the user finishes editing (not per keystroke).
			char textureBuffer[256];
			const std::string& path = renderer->GetTexturePath();
			const size_t length = path.copy(textureBuffer, sizeof(textureBuffer) - 1);
			textureBuffer[length] = '\0';

			ImGui::InputText("Texture", textureBuffer, sizeof(textureBuffer));
			if (ImGui::IsItemActivated()) BeginEdit();
			if (ImGui::IsItemDeactivatedAfterEdit()) { renderer->SetTexturePath(textureBuffer); CommitEdit(); }

			if (ImGui::SmallButton("Remove Component##sprite")) removeSprite = true;
		}
	}

	if (RigidBody2D* body = mSelectedEntity->GetComponent<RigidBody2D>())
	{
		if (ImGui::CollapsingHeader("Rigid Body 2D", ImGuiTreeNodeFlags_DefaultOpen))
		{
			static const char8* bodyTypes[] = { "Static", "Kinematic", "Dynamic" };

			int32 typeIndex = static_cast<int32>(body->GetBodyType());
			if (ImGui::Combo("Type", &typeIndex, bodyTypes, IM_ARRAYSIZE(bodyTypes)))
			{
				RecordSnapshot();
				body->SetBodyType(static_cast<BodyType>(typeIndex));
			}

			bool fixedRotation = body->IsFixedRotation();
			if (ImGui::Checkbox("Fixed Rotation", &fixedRotation))
			{
				RecordSnapshot();
				body->SetFixedRotation(fixedRotation);
			}

			if (ImGui::SmallButton("Remove Component##body")) removeBody = true;
		}
	}

	if (BoxCollider2D* collider = mSelectedEntity->GetComponent<BoxCollider2D>())
	{
		if (ImGui::CollapsingHeader("Box Collider 2D", ImGuiTreeNodeFlags_DefaultOpen))
		{
			float32 width = collider->GetWidth();
			if (ImGui::DragFloat("Width", &width, 1.0f, 0.0f, 10000.0f)) collider->SetWidth(width);
			if (ImGui::IsItemActivated()) BeginEdit();
			if (ImGui::IsItemDeactivatedAfterEdit()) CommitEdit();

			float32 height = collider->GetHeight();
			if (ImGui::DragFloat("Height", &height, 1.0f, 0.0f, 10000.0f)) collider->SetHeight(height);
			if (ImGui::IsItemActivated()) BeginEdit();
			if (ImGui::IsItemDeactivatedAfterEdit()) CommitEdit();

			float32 density = collider->GetDensity();
			if (ImGui::DragFloat("Density", &density, 0.05f, 0.0f, 100.0f)) collider->SetDensity(density);
			if (ImGui::IsItemActivated()) BeginEdit();
			if (ImGui::IsItemDeactivatedAfterEdit()) CommitEdit();

			float32 friction = collider->GetFriction();
			if (ImGui::DragFloat("Friction", &friction, 0.01f, 0.0f, 1.0f)) collider->SetFriction(friction);
			if (ImGui::IsItemActivated()) BeginEdit();
			if (ImGui::IsItemDeactivatedAfterEdit()) CommitEdit();

			float32 restitution = collider->GetRestitution();
			if (ImGui::DragFloat("Restitution", &restitution, 0.01f, 0.0f, 1.0f)) collider->SetRestitution(restitution);
			if (ImGui::IsItemActivated()) BeginEdit();
			if (ImGui::IsItemDeactivatedAfterEdit()) CommitEdit();

			if (ImGui::SmallButton("Remove Component##box")) removeBox = true;
		}
	}

	if (CircleCollider2D* collider = mSelectedEntity->GetComponent<CircleCollider2D>())
	{
		if (ImGui::CollapsingHeader("Circle Collider 2D", ImGuiTreeNodeFlags_DefaultOpen))
		{
			float32 radius = collider->GetRadius();
			if (ImGui::DragFloat("Radius", &radius, 1.0f, 0.0f, 10000.0f)) collider->SetRadius(radius);
			if (ImGui::IsItemActivated()) BeginEdit();
			if (ImGui::IsItemDeactivatedAfterEdit()) CommitEdit();

			float32 density = collider->GetDensity();
			if (ImGui::DragFloat("Density", &density, 0.05f, 0.0f, 100.0f)) collider->SetDensity(density);
			if (ImGui::IsItemActivated()) BeginEdit();
			if (ImGui::IsItemDeactivatedAfterEdit()) CommitEdit();

			float32 friction = collider->GetFriction();
			if (ImGui::DragFloat("Friction", &friction, 0.01f, 0.0f, 1.0f)) collider->SetFriction(friction);
			if (ImGui::IsItemActivated()) BeginEdit();
			if (ImGui::IsItemDeactivatedAfterEdit()) CommitEdit();

			float32 restitution = collider->GetRestitution();
			if (ImGui::DragFloat("Restitution", &restitution, 0.01f, 0.0f, 1.0f)) collider->SetRestitution(restitution);
			if (ImGui::IsItemActivated()) BeginEdit();
			if (ImGui::IsItemDeactivatedAfterEdit()) CommitEdit();

			if (ImGui::SmallButton("Remove Component##circle")) removeCircle = true;
		}
	}

	// Apply any requested component removal (one per frame is enough for a click).
	if (removeSprite) { RecordSnapshot(); mSelectedEntity->RemoveComponent<SpriteRenderer>(); }
	if (removeBody)   { RecordSnapshot(); mSelectedEntity->RemoveComponent<RigidBody2D>(); }
	if (removeBox)    { RecordSnapshot(); mSelectedEntity->RemoveComponent<BoxCollider2D>(); }
	if (removeCircle) { RecordSnapshot(); mSelectedEntity->RemoveComponent<CircleCollider2D>(); }

	// "Add Component" lists only the component types the entity doesn't already have.
	ImGui::Separator();

	if (ImGui::Button("Add Component"))
		ImGui::OpenPopup("AddComponentPopup");

	if (ImGui::BeginPopup("AddComponentPopup"))
	{
		if (!mSelectedEntity->HasComponent<SpriteRenderer>() && ImGui::MenuItem("Sprite Renderer"))
		{
			RecordSnapshot();
			mSelectedEntity->AddComponent<SpriteRenderer>("Sprite/Brickout/tile-1.png");
		}

		if (!mSelectedEntity->HasComponent<RigidBody2D>() && ImGui::MenuItem("Rigid Body 2D"))
		{
			RecordSnapshot();
			mSelectedEntity->AddComponent<RigidBody2D>();
		}

		if (!mSelectedEntity->HasComponent<BoxCollider2D>() && ImGui::MenuItem("Box Collider 2D"))
		{
			RecordSnapshot();
			mSelectedEntity->AddComponent<BoxCollider2D>(100.0f, 100.0f);
		}

		if (!mSelectedEntity->HasComponent<CircleCollider2D>() && ImGui::MenuItem("Circle Collider 2D"))
		{
			RecordSnapshot();
			mSelectedEntity->AddComponent<CircleCollider2D>(50.0f);
		}

		ImGui::EndPopup();
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
			{
				RecordSnapshot();
				mScene->Clear();
				mSelectedEntity = nullptr;
			}

			if (ImGui::MenuItem("Open Scene..."))
			{
				const std::string path = FileDialog::Open(sceneFilter);

				if (!path.empty())
				{
					RecordSnapshot();
					mSelectedEntity = nullptr;
					SceneSerializer::Deserialize(mScene, path);
				}
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

		if (ImGui::BeginMenu("Edit"))
		{
			if (ImGui::MenuItem("Undo", "Ctrl+Z", false, !mUndoStack.empty()))
				Undo();

			if (ImGui::MenuItem("Redo", "Ctrl+Y", false, !mRedoStack.empty()))
				Redo();

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("View"))
		{
			ImGui::MenuItem("ImGui Demo Window", nullptr, &mShowDemo);
			ImGui::EndMenu();
		}

		// Play / Stop controls, right-aligned in the menu bar.
		ImGui::SameLine(ImGui::GetWindowWidth() - 110.0f);

		if (mPlaying)
		{
			ImGui::TextColored(ImVec4(0.45f, 0.85f, 0.50f, 1.0f), "PLAYING");
			ImGui::SameLine();
			if (ImGui::SmallButton("Stop (F8)"))
				StopPlay();
		}
		else if (ImGui::SmallButton("Play (F5)"))
		{
			StartPlay();
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
