#pragma once

#include <Lion/Type/Size.h>

namespace Lion
{
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

		// Loads a texture from an image file for the selected backend.
		static Reference<Texture> Create(const std::string& filePath);
	};
}
