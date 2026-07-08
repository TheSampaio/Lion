#pragma once

#include <Lion/Render/Framebuffer.h>

namespace Lion
{
	// OpenGL framebuffer object with an RGBA color attachment and an optional R32I entity-id
	// attachment (used by the editor for pixel-perfect mouse picking).
	class OpenGLFramebuffer : public Framebuffer
	{
	public:
		explicit OpenGLFramebuffer(const FramebufferSpecification& specification);
		~OpenGLFramebuffer() override;

		void Bind() override;
		void Unbind() override;
		void Resize(uint32 width, uint32 height) override;

		uint32 GetColorAttachment() const override { return mColorAttachment; }
		void ClearEntityId(int32 value) override;
		int32 ReadEntityId(uint32 x, uint32 y) override;
		const FramebufferSpecification& GetSpecification() const override { return mSpecification; }

	private:
		uint32 mId = 0;
		uint32 mColorAttachment = 0;
		uint32 mEntityIdAttachment = 0;   // Optional R32I texture (spec.hasEntityId).
		FramebufferSpecification mSpecification;

		// (Re)creates the framebuffer object and its attachments at the current size.
		void Invalidate();
	};
}
