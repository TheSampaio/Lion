#pragma once

#include <Lion/Render/Framebuffer.h>

namespace Lion
{
	// OpenGL framebuffer object with a single RGBA color texture attachment.
	class OpenGLFramebuffer : public Framebuffer
	{
	public:
		explicit OpenGLFramebuffer(const FramebufferSpecification& specification);
		~OpenGLFramebuffer() override;

		void Bind() override;
		void Unbind() override;
		void Resize(uint32 width, uint32 height) override;

		uint32 GetColorAttachment() const override { return mColorAttachment; }
		const FramebufferSpecification& GetSpecification() const override { return mSpecification; }

	private:
		uint32 mId = 0;
		uint32 mColorAttachment = 0;
		FramebufferSpecification mSpecification;

		// (Re)creates the framebuffer object and its color texture at the current size.
		void Invalidate();
	};
}
