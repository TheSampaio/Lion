#include "Engine.h"
#include "Texture.h"

namespace Lion
{
	Texture::Texture(const std::string& filePath)
	{
        // Attributes
        mFilterMin = GL_LINEAR_MIPMAP_LINEAR;
        mFilterMag = GL_LINEAR;
        mFormatInternal = GL_RGB;
        mFormatExternal = GL_RGB;

        // Flips image on load
        stbi_set_flip_vertically_on_load(true);

        // Loads a image from disk
        uchar* bytes = stbi_load(filePath.c_str(), &mSize[0], &mSize[1], &mColumn, 0);
        mCenter = { mSize[0] / 2, mSize[1] / 2 };

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
	}

	Texture::~Texture()
	{
        glDeleteTextures(1, &mId);
	}

	void Texture::Bind(uint slot) const
	{
		glActiveTexture(GL_TEXTURE0 + slot);
		glBindTexture(GL_TEXTURE_2D, mId);
	}
}
