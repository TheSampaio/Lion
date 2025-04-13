#include "Engine.h"
#include "Texture.h"

#include "../Core/Log.h"

namespace Lion
{
#if LN_DEBUG
    uint32 Texture::sAllocationCount = 0;
#endif

	Texture::Texture(const std::string& filePath)
        : mId(0),
        mFilterMin(GL_NEAREST),
        mFilterMag(GL_NEAREST),
        mSize{ 0, 0 },
        mCenter{ 0, 0 },
        mFormatInternal(0),
        mFormatExternal(0),
        mColumn(0)
	{
        // Flips image on load
        stbi_set_flip_vertically_on_load(true);

        // Loads a image from disk
        const char8* filename = filePath.c_str();
        int32 width = 0, height = 0;
        byte* bytes = stbi_load(filename, &width, &height, &mColumn, 0);

        if (!bytes)
        {
            Log::Console(ELogMode::Warning, LN_LOG_FORMAT("[Texture] Failed to load image: '{}'.", filename));
            return;
        }

        mSize = { static_cast<uint32>(width), static_cast<uint32>(height) };
        mCenter = { mSize[0] / 2, mSize[1] / 2 };

        // Set texture formats based on number of channels
        switch (mColumn)
        {
        case 4:
            mFormatInternal = GL_RGBA;
            mFormatExternal = GL_RGBA;
            break;

        case 3:
            mFormatInternal = GL_RGB;
            mFormatExternal = GL_RGB;
            break;

        case 1:
            mFormatInternal = GL_RED;
            mFormatExternal = GL_RED;
            break;

        default:
            Log::Console(ELogMode::Error, LN_LOG_FORMAT("[Texture] Unsupported image format: '{}'.", filename));
            return;
        }

        // Generates a texture
        glGenTextures(1, &mId);

        // Bind a 2D texture
        glBindTexture(GL_TEXTURE_2D, mId);

        // Set-ups the minification and magnification filters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mFilterMin);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mFilterMag);

        // Set-ups the texture's wrap behavior
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        // Set-ups the texture data
        glTexImage2D(GL_TEXTURE_2D, 0, mFormatInternal, mSize[0], mSize[1], 0, mFormatExternal, GL_UNSIGNED_BYTE, bytes);

        // Generates a mipmap for the 2D texture
        glGenerateMipmap(GL_TEXTURE_2D);

        // Free the data loaded from disk
        stbi_image_free(bytes);

        // Unbinds the 2D texture to avoid bugs
        glBindTexture(GL_TEXTURE_2D, 0);

#if LN_DEBUG
        sAllocationCount++;
        Log::Console(ELogMode::Trace, LN_LOG_FORMAT("[Texture] Allocated: 1 (Total: {})", sAllocationCount));
#endif
	}

	Texture::~Texture()
	{
        glDeleteTextures(1, &mId);

#if LN_DEBUG
        sAllocationCount--;
        Log::Console(ELogMode::Trace, LN_LOG_FORMAT("[Texture] Released:  1 (Remaining: {})", sAllocationCount));
#endif
	}

	void Texture::Bind(uint32 slot) const
	{
		glActiveTexture(GL_TEXTURE0 + slot);
		glBindTexture(GL_TEXTURE_2D, mId);
	}
}
