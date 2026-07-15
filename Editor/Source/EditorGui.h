#pragma once

struct ImFont;
struct ImVec4;
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

	// The bold cut of the UI font, or null when the machine has no bold to give — which ImGui reads as "the
	// default font", so a caller pushing it is never left holding nothing.
	static ImFont* GetBoldFont();

	// The engine's orange, which is what the editor highlights with. One source, because a brand colour
	// spelled out in two places is a brand colour that will one day be two colours.
	static ImVec4 GetAccent();

	// The icon font on its own, baked at atlas resolution, and that resolution. Icons the editor draws by
	// hand use this rather than the copy merged into the text, so they stay sharp at any size down from it.
	static ImFont* GetIconFont();
	static Lion::float32 GetIconAtlasSize();
};
