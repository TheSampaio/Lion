#pragma once

struct ImFont;

// Dear ImGui lifecycle for the editor (GLFW + OpenGL 3 backends).
//
// ImGui lives only in the editor, never in the engine or the shipped game. The editor owns the
// full lifecycle and draws one UI frame per rendered frame.
class EditorGui
{
public:
	// Creates the ImGui context, style and backends (requires a live graphics context).
	static void Init();

	// Releases the backends and context (call while the window/context are still alive).
	static void Shutdown();

	// Starts a new UI frame.
	static void BeginFrame();

	// Renders the accumulated UI for this frame.
	static void EndFrame();

	// Bold UI font, used for emphasis (e.g. the Transform's axis buttons). Null when unavailable,
	// in which case callers should fall back to the default font.
	static ImFont* GetBoldFont();
};
