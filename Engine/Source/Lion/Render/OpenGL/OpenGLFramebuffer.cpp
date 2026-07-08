#include "Engine.h"
#include "OpenGLFramebuffer.h"

#include <Lion/Core/Log.h>

namespace Lion
{
	OpenGLFramebuffer::OpenGLFramebuffer(const FramebufferSpecification& specification)
		: mSpecification(specification)
	{
		Invalidate();
	}

	OpenGLFramebuffer::~OpenGLFramebuffer()
	{
		glDeleteFramebuffers(1, &mId);
		glDeleteTextures(1, &mColorAttachment);

		if (mEntityIdAttachment)
			glDeleteTextures(1, &mEntityIdAttachment);
	}

	void OpenGLFramebuffer::Invalidate()
	{
		if (mId)
		{
			glDeleteFramebuffers(1, &mId);
			glDeleteTextures(1, &mColorAttachment);

			if (mEntityIdAttachment)
				glDeleteTextures(1, &mEntityIdAttachment);

			mEntityIdAttachment = 0;
		}

		const int32 width = static_cast<int32>(mSpecification.width);
		const int32 height = static_cast<int32>(mSpecification.height);

		glGenFramebuffers(1, &mId);
		glBindFramebuffer(GL_FRAMEBUFFER, mId);

		// Color texture attachment (displayed in the viewport).
		glGenTextures(1, &mColorAttachment);
		glBindTexture(GL_TEXTURE_2D, mColorAttachment);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mColorAttachment, 0);

		if (mSpecification.hasEntityId)
		{
			// Integer attachment storing the entity id per pixel (nearest filtering: no interpolation).
			glGenTextures(1, &mEntityIdAttachment);
			glBindTexture(GL_TEXTURE_2D, mEntityIdAttachment);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_R32I, width, height, 0, GL_RED_INTEGER, GL_INT, nullptr);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, mEntityIdAttachment, 0);

			const GLenum drawBuffers[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
			glDrawBuffers(2, drawBuffers);
		}

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			Log::Console(LogLevel::Error, "[OpenGLFramebuffer] Framebuffer is incomplete.");

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	void OpenGLFramebuffer::ClearEntityId(int32 value)
	{
		if (!mEntityIdAttachment)
			return;

		glBindFramebuffer(GL_FRAMEBUFFER, mId);
		glClearBufferiv(GL_COLOR, 1, &value);  // Draw buffer 1 == COLOR_ATTACHMENT1.
	}

	int32 OpenGLFramebuffer::ReadEntityId(uint32 x, uint32 y)
	{
		if (!mEntityIdAttachment)
			return -1;

		glBindFramebuffer(GL_FRAMEBUFFER, mId);
		glReadBuffer(GL_COLOR_ATTACHMENT1);

		int32 id = -1;
		glReadPixels(static_cast<int32>(x), static_cast<int32>(y), 1, 1, GL_RED_INTEGER, GL_INT, &id);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		return id;
	}

	void OpenGLFramebuffer::Bind()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, mId);
		glViewport(0, 0, static_cast<GLsizei>(mSpecification.width), static_cast<GLsizei>(mSpecification.height));
	}

	void OpenGLFramebuffer::Unbind()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void OpenGLFramebuffer::Resize(uint32 width, uint32 height)
	{
		if (width == 0 || height == 0)
			return;  // Ignore a collapsed/minimized viewport.

		if (width == mSpecification.width && height == mSpecification.height)
			return;

		mSpecification.width = width;
		mSpecification.height = height;
		Invalidate();
	}
}
