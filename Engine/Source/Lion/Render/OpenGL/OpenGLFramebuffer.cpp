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
	}

	void OpenGLFramebuffer::Invalidate()
	{
		if (mId)
		{
			glDeleteFramebuffers(1, &mId);
			glDeleteTextures(1, &mColorAttachment);
		}

		glGenFramebuffers(1, &mId);
		glBindFramebuffer(GL_FRAMEBUFFER, mId);

		// Color texture attachment.
		glGenTextures(1, &mColorAttachment);
		glBindTexture(GL_TEXTURE_2D, mColorAttachment);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
			static_cast<int32>(mSpecification.width), static_cast<int32>(mSpecification.height),
			0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mColorAttachment, 0);

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			Log::Console(LogLevel::Error, "[OpenGLFramebuffer] Framebuffer is incomplete.");

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glBindTexture(GL_TEXTURE_2D, 0);
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
