#include "Engine.h"
#include "OpenGLTexture.h"

#include <Lion/Core/Filesystem.h>
#include <Lion/Core/Log.h>

namespace Lion
{
#if LN_DEBUG
	uint32 OpenGLTexture::sAllocationCount = 0;
#endif

	OpenGLTexture::OpenGLTexture(const std::string& filePath)
		: mFilterMin(GL_NEAREST),
		mFilterMag(GL_NEAREST)
	{
		// Images are stored top-down; flip them to match OpenGL's bottom-up convention.
		stbi_set_flip_vertically_on_load(true);

		const char8* filename = filePath.c_str();
		int32 width = 0, height = 0;
		byte* bytes = stbi_load(ResolveResourcePath(filePath).c_str(), &width, &height, &mChannels, 0);

		if (!bytes)
		{
			Log::Console(LogLevel::Warning, LION_FORMAT_TEXT("[OpenGLTexture] Failed to load image: '{}'.", filename));
			return;
		}

		mSize = { width, height };
		mCenter = { mSize.width / 2, mSize.height / 2 };

		// Select the pixel format from the number of channels.
		switch (mChannels)
		{
			case 4: mFormatInternal = GL_RGBA; mFormatExternal = GL_RGBA; break;
			case 3: mFormatInternal = GL_RGB;  mFormatExternal = GL_RGB;  break;
			case 1: mFormatInternal = GL_RED;  mFormatExternal = GL_RED;  break;

			default:
				Log::Console(LogLevel::Error, LION_FORMAT_TEXT("[OpenGLTexture] Unsupported image format: '{}'.", filename));
				stbi_image_free(bytes);
				return;
		}

		glGenTextures(1, &mId);
		glBindTexture(GL_TEXTURE_2D, mId);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mFilterMin);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mFilterMag);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		glTexImage2D(GL_TEXTURE_2D, 0, mFormatInternal,
			static_cast<int32>(mSize.width), static_cast<int32>(mSize.height),
			0, mFormatExternal, GL_UNSIGNED_BYTE, bytes);

		glGenerateMipmap(GL_TEXTURE_2D);

		stbi_image_free(bytes);
		glBindTexture(GL_TEXTURE_2D, 0);

#if LN_DEBUG
		sAllocationCount++;
		Log::Console(LogLevel::Trace, LION_FORMAT_TEXT("[OpenGLTexture] Allocated: 1 (Total: {})", sAllocationCount));
#endif
	}

	OpenGLTexture::~OpenGLTexture()
	{
		glDeleteTextures(1, &mId);

#if LN_DEBUG
		sAllocationCount--;
		Log::Console(LogLevel::Trace, LION_FORMAT_TEXT("[OpenGLTexture] Released:  1 (Remaining: {})", sAllocationCount));
#endif
	}

	void OpenGLTexture::Bind(uint32 slot) const
	{
		glBindTextureUnit(slot, mId);
	}
}
