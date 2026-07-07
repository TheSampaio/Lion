#pragma once

namespace Lion
{
	// Size of an off-screen render target.
	struct FramebufferSpecification
	{
		uint32 width = 0;
		uint32 height = 0;
	};

	// Off-screen render target.
	//
	// Binding a framebuffer redirects rendering into its color texture instead of the window.
	// The editor renders the scene into one and displays its color attachment in a viewport panel.
	class Framebuffer
	{
	public:
		virtual ~Framebuffer() = default;

		// Redirects rendering into this framebuffer (and sets a matching viewport).
		virtual void Bind() = 0;

		// Restores rendering to the default (window) framebuffer.
		virtual void Unbind() = 0;

		// Recreates the attachments at a new size (no-op for a zero size).
		virtual void Resize(uint32 width, uint32 height) = 0;

		// Native handle of the color texture, usable as an ImGui image id.
		virtual uint32 GetColorAttachment() const = 0;

		virtual const FramebufferSpecification& GetSpecification() const = 0;

		// Creates a framebuffer for the selected backend.
		static LION_API Reference<Framebuffer> Create(const FramebufferSpecification& specification);
	};
}
