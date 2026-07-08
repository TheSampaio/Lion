#include "EditorGui.h"

#include <Lion/Lion.h>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>

using namespace Lion;

static void SetDarkTheme()
{
	ImGui::StyleColorsDark();

	ImGuiStyle& style = ImGui::GetStyle();
	style.WindowRounding = 4.0f;
	style.FrameRounding = 3.0f;
	style.GrabRounding = 3.0f;
	style.WindowPadding = ImVec2(8.0f, 8.0f);

	ImVec4* colors = style.Colors;
	colors[ImGuiCol_WindowBg]       = ImVec4(0.10f, 0.10f, 0.11f, 1.00f);
	colors[ImGuiCol_Header]         = ImVec4(0.20f, 0.20f, 0.22f, 1.00f);
	colors[ImGuiCol_HeaderHovered]  = ImVec4(0.30f, 0.30f, 0.33f, 1.00f);
	colors[ImGuiCol_HeaderActive]   = ImVec4(0.25f, 0.25f, 0.28f, 1.00f);
	colors[ImGuiCol_Button]         = ImVec4(0.20f, 0.20f, 0.22f, 1.00f);
	colors[ImGuiCol_ButtonHovered]  = ImVec4(0.30f, 0.30f, 0.33f, 1.00f);
	colors[ImGuiCol_ButtonActive]   = ImVec4(0.25f, 0.25f, 0.28f, 1.00f);
	colors[ImGuiCol_FrameBg]        = ImVec4(0.16f, 0.16f, 0.18f, 1.00f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.22f, 0.22f, 0.24f, 1.00f);
	colors[ImGuiCol_FrameBgActive]  = ImVec4(0.26f, 0.26f, 0.28f, 1.00f);
	colors[ImGuiCol_TitleBg]        = ImVec4(0.13f, 0.13f, 0.14f, 1.00f);
	colors[ImGuiCol_TitleBgActive]  = ImVec4(0.16f, 0.16f, 0.18f, 1.00f);
	colors[ImGuiCol_Tab]            = ImVec4(0.14f, 0.14f, 0.16f, 1.00f);
	colors[ImGuiCol_TabHovered]     = ImVec4(0.30f, 0.30f, 0.33f, 1.00f);
}

void EditorGui::Init()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	SetDarkTheme();

	auto* window = static_cast<GLFWwindow*>(Window::GetNativeHandle());
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 460");
}

void EditorGui::Shutdown()
{
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

void EditorGui::BeginFrame()
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

void EditorGui::EndFrame()
{
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
