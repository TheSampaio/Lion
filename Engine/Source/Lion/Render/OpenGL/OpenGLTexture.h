#pragma once

#include <Lion/Render/Texture.h>

namespace Lion
{
	// OpenGL implementation of the Texture abstraction, loaded from disk through stb_image.
	class OpenGLTexture : public Texture
	{
	public:
		explicit OpenGLTexture(const std::string& filePath);
		~OpenGLTexture() override;

		Size GetSize() const override { return mSize; }
		Size GetCenter() const override { return mCenter; }

		void Bind(uint32 slot = 0) const override;

	private:
		uint32 mId = 0;

		int32 mFilterMin = 0;
		int32 mFilterMag = 0;

		int32 mFormatInternal = 0;
		uint32 mFormatExternal = 0;

		int32 mChannels = 0;

		Size mSize{ 0, 0 };
		Size mCenter{ 0, 0 };

#if LN_DEBUG
		static uint32 sAllocationCount;
#endif
	};
}
