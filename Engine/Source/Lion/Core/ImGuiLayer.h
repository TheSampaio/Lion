#pragma once

#include <imgui/imgui.h>

namespace Lion
{
	// Immediate-mode UI system backed by Dear ImGui (GLFW + OpenGL 3 backends).
	//
	// The Application owns one ImGuiLayer and drives a single UI frame per rendered frame:
	// Begin() opens the frame, every layer's OnRenderUI() submits its widgets, and End()
	// renders them on top of the scene. Docking is enabled for the future editor panels.
	class ImGuiLayer
	{
	public:
		// Creates the ImGui context and style (no graphics context required yet).
		void CreateContext();

		// Initializes the GLFW/OpenGL backends (requires a live graphics context).
		void InitBackend();

		// Shuts the backends down and destroys the context.
		void Shutdown();

		// Starts a new UI frame.
		void Begin();

		// Renders the accumulated UI for this frame.
		void End();

	private:
		void SetDarkTheme();
	};

	// Dear ImGui keeps its context and allocator as per-module globals. When the engine is a DLL
	// and the client (game/editor) links ImGui too, the client must adopt the engine's context and
	// allocator so both talk to the same instance. These accessors expose them; the launcher wires
	// them up transparently (see Launcher.h), so client code just calls ImGui:: normally.
	LION_API ImGuiContext* GetImGuiContext();
	LION_API void GetImGuiAllocatorFunctions(ImGuiMemAllocFunc* allocFunc, ImGuiMemFreeFunc* freeFunc, void** userData);
}
