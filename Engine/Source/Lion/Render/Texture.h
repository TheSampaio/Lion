#pragma once

#include <Lion/Type/Size.h>

namespace Lion
{
	// How a texture is sampled when it is not drawn at its own size.
	//
	// A sprite is drawn one texel to one pixel and wants none of it — smoothing pixel art is how pixel art
	// stops being pixel art. A picture that is scaled to fit something (an icon in a toolbar, a mark in a
	// title bar) wants all of it, or it arrives at its new size in ruins.
	enum class TextureFilter
	{
		Nearest,
		Linear,
	};

	// 2D texture abstraction, decoupled from the underlying graphics backend.
	class Texture
	{
	public:
		virtual ~Texture() = default;

		// Pixel size of the texture.
		virtual Size GetSize() const = 0;

		// Half the pixel size (texture center), useful for sprite pivots.
		virtual Size GetCenter() const = 0;

		// Binds the texture to the given sampler slot.
		virtual void Bind(uint32 slot = 0) const = 0;

		// The backend's own handle for this texture. The editor needs it to hand a texture to ImGui, which
		// is the one place outside the backend that has to name one.
		virtual uint32 GetNativeHandle() const = 0;

		// Loads a texture from an image file for the selected backend.
		static LION_API Reference<Texture> Create(const std::string& filePath, TextureFilter filter = TextureFilter::Nearest);

		// How many textures are alive right now. It used to be a line in the console every time one was
		// created or destroyed, which is a number pretending to be news: a count belongs on a panel that
		// shows the current one, not in a log that repeats every value it ever had.
		static LION_API uint32 GetLiveCount();
	};
}
