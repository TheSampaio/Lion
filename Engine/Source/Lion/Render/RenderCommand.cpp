#include "Engine.h"
#include "RenderCommand.h"

#include <Lion/Render/RendererAPI.h>

namespace Lion
{
	Scope<RendererAPI> RenderCommand::sRendererAPI = nullptr;

	void RenderCommand::Init()
	{
		sRendererAPI = RendererAPI::Create();
		sRendererAPI->Init();
	}

	void RenderCommand::Shutdown()
	{
		sRendererAPI.reset();
	}

	void RenderCommand::SetViewport(int32 x, int32 y, uint32 width, uint32 height)
	{
		sRendererAPI->SetViewport(x, y, width, height);
	}

	void RenderCommand::SetClearColor(float32 red, float32 green, float32 blue, float32 alpha)
	{
		sRendererAPI->SetClearColor(red, green, blue, alpha);
	}

	void RenderCommand::Clear()
	{
		sRendererAPI->Clear();
	}

	void RenderCommand::DrawIndexed(uint32 indexCount)
	{
		sRendererAPI->DrawIndexed(indexCount);
	}
}
