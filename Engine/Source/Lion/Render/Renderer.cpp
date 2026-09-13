#include "Engine.h"
#include "Renderer.h"

#include <Lion/Render/Buffer.h>
#include <Lion/Render/Camera.h>
#include <Lion/Render/Framebuffer.h>
#include <Lion/Render/PostProcessingComponent.h>
#include <Lion/Render/RenderCommand.h>
#include <Lion/Render/Shader.h>
#include <Lion/Render/Sprite.h>
#include <Lion/Render/Texture.h>
#include <Lion/Render/VertexArray.h>
#include <Lion/Core/Window.h>

namespace Lion
{
    static const size_t maxQuadCount = 1000;
    static const size_t maxVertexCount = maxQuadCount * 4;
    static const size_t maxIndexCount = maxQuadCount * 6;
    static const size_t maxTextureCount = 32;

    struct Vertex
    {
        glm::vec3 position;
        glm::vec4 color;
        glm::vec2 textureCoord;
        float32 texture;
        float32 entityId;  // Owner entity id (as float; exact for ids well under 2^24), for editor picking.
    };

	struct ScreenVertex
	{
		glm::vec2 position;
		glm::vec2 textureCoord;
	};

    Renderer* Renderer::sInstance = nullptr;

    void Renderer::New()
    {
        sInstance = new Renderer();
    }

    void Renderer::Delete()
    {
        delete sInstance;
        sInstance = nullptr;
    }

    bool Renderer::Initialize()
    {
        Renderer* self = sInstance;

        self->mShader = Shader::Create("Shaders/Lit.lnshader");
        self->mWireframeShader = Shader::Create("Shaders/Wireframe.lnshader");
		self->mPostShader = Shader::Create("Shaders/PostProcessing.lnshader");

        // Dynamic vertex buffer streamed once per batch, described by the sprite vertex layout.
        self->mVertexArray = VertexArray::Create();
        self->mVertexBuffer = VertexBuffer::Create(static_cast<uint32>(sizeof(Vertex) * maxVertexCount));
        self->mVertexBuffer->SetLayout({
            { ShaderDataType::Float3, "iPosition" },
            { ShaderDataType::Float4, "iColor" },
            { ShaderDataType::Float2, "iTexCoord" },
            { ShaderDataType::Float,  "iTexId" },
            { ShaderDataType::Float,  "iEntityId" },
        });
        self->mVertexArray->AddVertexBuffer(self->mVertexBuffer);

        // Static index buffer: two triangles per quad, precomputed for the whole capacity.
        {
            std::vector<uint32> indices(maxIndexCount);
            uint32 offset = 0;

            for (size_t i = 0; i < maxIndexCount; i += 6)
            {
                indices[i + 0] = 0 + offset;
                indices[i + 1] = 1 + offset;
                indices[i + 2] = 2 + offset;

                indices[i + 3] = 0 + offset;
                indices[i + 4] = 2 + offset;
                indices[i + 5] = 3 + offset;

                offset += 4;
            }

            self->mIndexBuffer = IndexBuffer::Create(indices.data(), static_cast<uint32>(maxIndexCount));
            self->mVertexArray->SetIndexBuffer(self->mIndexBuffer);
        }

        self->mSpriteBuffer.reserve(maxQuadCount);
        self->mTextureSlots.reserve(maxTextureCount);
        self->mVertexData.resize(maxVertexCount);

		const ScreenVertex screenVertices[] = {
			{ { -1.0f,  1.0f }, { 0.0f, 1.0f } },
			{ { -1.0f, -1.0f }, { 0.0f, 0.0f } },
			{ {  1.0f, -1.0f }, { 1.0f, 0.0f } },
			{ {  1.0f,  1.0f }, { 1.0f, 1.0f } },
		};
		const uint32 screenIndices[] = { 0, 1, 2, 0, 2, 3 };
		self->mPostVertexArray = VertexArray::Create();
		self->mPostVertexBuffer = VertexBuffer::Create(screenVertices, sizeof(screenVertices));
		self->mPostVertexBuffer->SetLayout({
			{ ShaderDataType::Float2, "iPosition" },
			{ ShaderDataType::Float2, "iTexCoord" },
		});
		self->mPostVertexArray->AddVertexBuffer(self->mPostVertexBuffer);
		self->mPostIndexBuffer = IndexBuffer::Create(screenIndices, 6);
		self->mPostVertexArray->SetIndexBuffer(self->mPostIndexBuffer);

        return self->mShader != nullptr && self->mPostShader != nullptr;
    }

    void Renderer::Clear(float32 red, float32 green, float32 blue, float32 alpha)
    {
        RenderCommand::SetClearColor(red, green, blue, alpha);
        RenderCommand::Clear();
    }

    void Renderer::SetWireframe(bool enabled)
    {
        Renderer* self = sInstance;
        self->mWireframe = enabled;
        RenderCommand::SetWireframe(enabled);
    }

    void Renderer::RenderBegin(const Reference<Camera>& camera)
    {
        Renderer* self = sInstance;

        const Reference<Shader>& shader = self->mWireframe ? self->mWireframeShader : self->mShader;
        shader->Bind();
        self->mVertexArray->Bind();

        camera->OnUsage();

        shader->SetMat4("uView", camera->GetViewMatrix());
        shader->SetMat4("uProjection", camera->GetProjectionMatrix());
    }

    const RenderStats& Renderer::GetStats()
    {
        return sInstance->mStats;
    }

    void Renderer::RenderEnd()
    {
        Renderer* self = sInstance;

        // Counted whether or not anything is drawn: a frame that drew nothing is a fact about the frame,
        // not a reason to leave the last one's numbers on screen.
        self->mStats = RenderStats();
        self->mStats.sprites = static_cast<uint32>(self->mSpriteBuffer.size());

        if (self->mSpriteBuffer.empty())
            return;

        // Sort by draw order, low to high, so the highest lands on top and transparency blends over what
        // is behind it. A *stable* sort is the whole point of the rest: sprites sharing an order keep the
        // order they were submitted in, which is the order the scene holds its entities — the order the
        // Hierarchy shows. Moving a row down the list therefore moves the sprite in front of the ones
        // above it, and setting an order by hand is overriding exactly that and nothing else.
        std::stable_sort(self->mSpriteBuffer.begin(), self->mSpriteBuffer.end(),
            [](const SpriteInfo* a, const SpriteInfo* b) { return a->order < b->order; });

        // Rebuild the texture-slot table for this frame. Index 0 is reserved and never sampled.
        auto& textureSlots = self->mTextureSlots;
        textureSlots.assign(1, nullptr);

        Vertex* buffer = self->mVertexData.data();
        uint32 indexCount = 0;

        for (const SpriteInfo* sprite : self->mSpriteBuffer)
        {
            Texture* texture = sprite->texture;

            // Find the slot already assigned to this texture, if any (linear scan over <= 32).
            int32 slot = -1;
            for (uint32 i = 1; i < textureSlots.size(); ++i)
            {
                if (textureSlots[i] == texture)
                {
                    slot = static_cast<int32>(i);
                    break;
                }
            }

            // Assign a new slot the first time a texture is seen this frame.
            if (slot < 0)
            {
                if (textureSlots.size() >= maxTextureCount)
                {
                    self->mStats.spritesDropped++;  // Texture budget exhausted for this batch.
                    continue;
                }

                slot = static_cast<int32>(textureSlots.size());
                textureSlots.push_back(texture);
            }

            buffer = CreateQuad(buffer, sprite, slot);
            indexCount += 6;
        }

        // Upload only the vertices produced this frame.
        const uint32 vertexCount = static_cast<uint32>(buffer - self->mVertexData.data());
        self->mVertexBuffer->SetData(self->mVertexData.data(), static_cast<uint32>(sizeof(Vertex) * vertexCount));

        // Sampler i reads from texture unit i.
        {
            int32 samplers[maxTextureCount];

            for (int32 i = 0; i < static_cast<int32>(maxTextureCount); ++i)
                samplers[i] = i;

            if (!self->mWireframe)
                self->mShader->SetIntArray("uDiffuseTextureArray", samplers, static_cast<uint32>(maxTextureCount));
        }

        // Bind each used texture to the unit matching its slot index.
        if (!self->mWireframe)
            for (uint32 slot = 1; slot < textureSlots.size(); ++slot)
                textureSlots[slot]->Bind(slot);

        RenderCommand::DrawIndexed(indexCount);

        self->mStats.drawCalls = 1;   // The whole point of the batch: one call, however many sprites.
        self->mStats.indices = indexCount;
        self->mStats.vertices = indexCount / 6 * 4;
        self->mStats.textureSlots = static_cast<uint32>(textureSlots.size() - 1);  // Slot 0 is reserved.

        self->mSpriteBuffer.clear();
    }

	bool Renderer::BeginPostProcessing(PostProcessingComponent* component, uint32 width, uint32 height)
	{
		Renderer* self = sInstance;

		if (!component || !component->IsEnabled() || width == 0 || height == 0)
			return false;

		if (!self->mPostFramebuffer)
			self->mPostFramebuffer = Framebuffer::Create({ width, height, false });
		else
			self->mPostFramebuffer->Resize(width, height);

		self->mActivePostProcessing = component;
		self->mPostFramebuffer->Bind();
		RenderCommand::Clear();
		return true;
	}

	void Renderer::EndPostProcessing(const Reference<Framebuffer>& target)
	{
		Renderer* self = sInstance;

		if (!self->mActivePostProcessing || !self->mPostFramebuffer)
			return;

		if (target)
			target->Bind();
		else
		{
			self->mPostFramebuffer->Unbind();
			const Size windowSize = Window::GetSize();
			RenderCommand::SetViewport(0, 0, static_cast<uint32>(windowSize.width), static_cast<uint32>(windowSize.height));
		}

		PostProcessingComponent& effect = *self->mActivePostProcessing;
		const std::string& customPath = effect.GetCustomShaderPath();
		if (!customPath.empty() && customPath != self->mCustomPostShaderPath)
		{
			self->mCustomPostShaderPath = customPath;
			self->mCustomPostShader = Shader::Create(customPath);
		}
		else if (customPath.empty())
		{
			self->mCustomPostShaderPath.clear();
			self->mCustomPostShader.reset();
		}

		const Reference<Shader>& shader = self->mCustomPostShader ? self->mCustomPostShader : self->mPostShader;
		shader->Bind();
		shader->SetInt("uScreenTexture", 0);
		const FramebufferSpecification& source = self->mPostFramebuffer->GetSpecification();
		shader->SetFloat2("uResolution", glm::vec2(source.width, source.height));
		shader->SetFloat("uBloomEnabled", effect.HasBloom() ? 1.0f : 0.0f);
		shader->SetFloat("uBloomStrength", effect.GetBloomStrength());
		shader->SetFloat("uBloomThreshold", effect.GetBloomThreshold());
		shader->SetFloat("uColorCorrectionEnabled", effect.HasColorCorrection() ? 1.0f : 0.0f);
		shader->SetFloat("uBrightness", effect.GetBrightness());
		shader->SetFloat("uContrast", effect.GetContrast());
		shader->SetFloat("uSaturation", effect.GetSaturation());
		shader->SetFloat("uGamma", effect.GetGamma());
		shader->SetFloat3("uTint", glm::vec3(effect.GetTint().x, effect.GetTint().y, effect.GetTint().z));
		shader->SetFloat("uVignetteEnabled", effect.HasVignette() ? 1.0f : 0.0f);
		shader->SetFloat("uVignetteStrength", effect.GetVignetteStrength());
		shader->SetFloat("uChromaticAberrationEnabled", effect.HasChromaticAberration() ? 1.0f : 0.0f);
		shader->SetFloat("uChromaticAberration", effect.GetChromaticAberration());

		self->mPostFramebuffer->BindColorAttachment(0);
		self->mPostVertexArray->Bind();
		RenderCommand::SetWireframe(false);
		RenderCommand::DrawIndexed(6);
		RenderCommand::SetWireframe(self->mWireframe);
		self->mActivePostProcessing = nullptr;
	}

    void Renderer::Submit(SpriteInfo* spriteInfo)
    {
        sInstance->mSpriteBuffer.push_back(spriteInfo);
    }

    Vertex* Renderer::CreateQuad(Vertex* target, const SpriteInfo* spriteInfo, int32 textureSlot)
    {
        const float32 x = spriteInfo->position.x;
        const float32 y = spriteInfo->position.y;
        const float32 z = spriteInfo->position.z;
        const float32 halfWidth = spriteInfo->size.width * 0.5f * spriteInfo->scale.x;
        const float32 halfHeight = spriteInfo->size.height * 0.5f * spriteInfo->scale.y;
        const float32 slot = static_cast<float32>(textureSlot);
        const float32 entityId = static_cast<float32>(spriteInfo->entityId);

        // Rotate the corner offsets around the sprite center (rotation.z is stored in degrees).
        const float32 angle = glm::radians(spriteInfo->rotation.z);
        const float32 cosAngle = std::cos(angle);
        const float32 sinAngle = std::sin(angle);

        const auto corner = [&](float32 offsetX, float32 offsetY) -> glm::vec3
        {
            return { x + offsetX * cosAngle - offsetY * sinAngle, y + offsetX * sinAngle + offsetY * cosAngle, z };
        };

        const glm::vec4 color = {
            spriteInfo->color.x,
            spriteInfo->color.y,
            spriteInfo->color.z,
            1.0f
        };

        // Mirroring is the texture read backwards: the quad keeps its shape, its winding and its size,
        // and only the corner each texel is fetched from changes.
        const float32 left = spriteInfo->flipX ? spriteInfo->uvMaximum.x : spriteInfo->uvMinimum.x;
        const float32 right = spriteInfo->flipX ? spriteInfo->uvMinimum.x : spriteInfo->uvMaximum.x;
        const float32 bottom = spriteInfo->flipY ? spriteInfo->uvMaximum.y : spriteInfo->uvMinimum.y;
        const float32 top = spriteInfo->flipY ? spriteInfo->uvMinimum.y : spriteInfo->uvMaximum.y;

        // Top-Left
        target->position = corner(-halfWidth, halfHeight);
        target->color = color;
        target->textureCoord = { left, top };
        target->texture = slot;
        target->entityId = entityId;
        target++;

        // Bottom-Left
        target->position = corner(-halfWidth, -halfHeight);
        target->color = color;
        target->textureCoord = { left, bottom };
        target->texture = slot;
        target->entityId = entityId;
        target++;

        // Bottom-Right
        target->position = corner(halfWidth, -halfHeight);
        target->color = color;
        target->textureCoord = { right, bottom };
        target->texture = slot;
        target->entityId = entityId;
        target++;

        // Top-Right
        target->position = corner(halfWidth, halfHeight);
        target->color = color;
        target->textureCoord = { right, top };
        target->texture = slot;
        target->entityId = entityId;
        target++;

        return target;
    }

    void Renderer::OnWindowResize(uint32 width, uint32 height)
    {
        RenderCommand::SetViewport(0, 0, width, height);
    }
}
