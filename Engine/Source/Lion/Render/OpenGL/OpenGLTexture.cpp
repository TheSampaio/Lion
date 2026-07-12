#include "Engine.h"
#include "OpenGLTexture.h"

#include <Lion/Core/Filesystem.h>
#include <Lion/Core/Log.h>

namespace Lion
{
	uint32 OpenGLTexture::sAllocationCount = 0;

	uint32 Texture::GetLiveCount()
	{
		return OpenGLTexture::sAllocationCount;
	}

	OpenGLTexture::OpenGLTexture(const std::string& filePath, TextureFilter filter)
	{
		// A scaled picture asks for the whole mipmap chain on the way down and interpolation on the way up.
		// A sprite asks for neither: one texel, one pixel, and nothing in between it did not draw.
		const bool linear = (filter == TextureFilter::Linear);

		mFilterMin = linear ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST;
		mFilterMag = linear ? GL_LINEAR : GL_NEAREST;

		// Images are stored top-down; flip them to match OpenGL's bottom-up convention.
		stbi_set_flip_vertically_on_load(true);

		const char8* filename = filePath.c_str();
		int32 width = 0, height = 0;
		byte* bytes = stbi_load(ResolveResourcePath(filePath).c_str(), &width, &height, &mChannels, 0);

		if (!bytes)
		{
			Log::Console(LogLevel::Warning, LION_FORMAT_TEXT("[Texture] Could not load '{}'.", filename));
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
				Log::Console(LogLevel::Error, LION_FORMAT_TEXT("[Texture] '{}' has an unsupported pixel format.", filename));
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

		// Not guarded by the build: the log's verbosity is the single thing that decides whether this is
		// seen, so the editor can show it whatever configuration it was compiled in.
		//
		// The line says which texture, and how big it turned out to be — the two things a reader wants
		// when a scene looks wrong. The running total it used to print is a number, not news, and now
		// lives on the Statistics panel where the current value is the only one worth having.
		sAllocationCount++;

		if (Log::IsEnabled(LogLevel::Trace))
			Log::Console(LogLevel::Trace, LION_FORMAT_TEXT("[Texture] Loaded '{}' ({}x{}, {} channels).",
				filename, mSize.width, mSize.height, mChannels));
	}

	OpenGLTexture::~OpenGLTexture()
	{
		glDeleteTextures(1, &mId);

		sAllocationCount--;
	}

	void OpenGLTexture::Bind(uint32 slot) const
	{
		glBindTextureUnit(slot, mId);
	}
}
