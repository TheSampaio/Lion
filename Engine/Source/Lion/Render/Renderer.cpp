#include "Engine.h"
#include "Renderer.h"

#include <Lion/Render/Buffer.h>
#include <Lion/Render/Camera.h>
#include <Lion/Render/RenderCommand.h>
#include <Lion/Render/Shader.h>
#include <Lion/Render/Sprite.h>
#include <Lion/Render/Texture.h>
#include <Lion/Render/VertexArray.h>

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

        self->mShader = Shader::Create("Shader/Lit.glsl");

        // Dynamic vertex buffer streamed once per batch, described by the sprite vertex layout.
        self->mVertexArray = VertexArray::Create();
        self->mVertexBuffer = VertexBuffer::Create(static_cast<uint32>(sizeof(Vertex) * maxVertexCount));
        self->mVertexBuffer->SetLayout({
            { ShaderDataType::Float3, "iPosition" },
            { ShaderDataType::Float4, "iColor" },
            { ShaderDataType::Float2, "iTexCoord" },
            { ShaderDataType::Float,  "iTexId" },
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

        return self->mShader != nullptr;
    }

    void Renderer::Clear(float32 red, float32 green, float32 blue, float32 alpha)
    {
        RenderCommand::SetClearColor(red, green, blue, alpha);
        RenderCommand::Clear();
    }

    void Renderer::RenderBegin(const Reference<Camera>& camera)
    {
        Renderer* self = sInstance;

        self->mShader->Bind();
        self->mVertexArray->Bind();

        camera->OnUsage();

        self->mShader->SetMat4("uView", camera->GetViewMatrix());
        self->mShader->SetMat4("uProjection", camera->GetProjectionMatrix());
    }

    void Renderer::RenderEnd()
    {
        Renderer* self = sInstance;

        if (self->mSpriteBuffer.empty())
            return;

        // Sort sprites by depth (back to front) so transparency blends correctly.
        std::sort(self->mSpriteBuffer.begin(), self->mSpriteBuffer.end(),
            [](const SpriteInfo* a, const SpriteInfo* b) { return a->position.z > b->position.z; });

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
                    continue;  // Texture budget exhausted for this batch; skip the sprite.

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

            self->mShader->SetIntArray("uDiffuseTextureArray", samplers, static_cast<uint32>(maxTextureCount));
        }

        // Bind each used texture to the unit matching its slot index.
        for (uint32 slot = 1; slot < textureSlots.size(); ++slot)
            textureSlots[slot]->Bind(slot);

        RenderCommand::DrawIndexed(indexCount);

        self->mSpriteBuffer.clear();
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
        const Size halfSize = spriteInfo->size * 0.5f;
        const float32 slot = static_cast<float32>(textureSlot);

        constexpr glm::vec4 white = { 1.0f, 1.0f, 1.0f, 1.0f };

        // Top-Left
        target->position = { x - halfSize.width, y + halfSize.height, z };
        target->color = white;
        target->textureCoord = { 0.0f, 1.0f };
        target->texture = slot;
        target++;

        // Bottom-Left
        target->position = { x - halfSize.width, y - halfSize.height, z };
        target->color = white;
        target->textureCoord = { 0.0f, 0.0f };
        target->texture = slot;
        target++;

        // Bottom-Right
        target->position = { x + halfSize.width, y - halfSize.height, z };
        target->color = white;
        target->textureCoord = { 1.0f, 0.0f };
        target->texture = slot;
        target++;

        // Top-Right
        target->position = { x + halfSize.width, y + halfSize.height, z };
        target->color = white;
        target->textureCoord = { 1.0f, 1.0f };
        target->texture = slot;
        target++;

        return target;
    }

    void Renderer::OnWindowResize(uint32 width, uint32 height)
    {
        RenderCommand::SetViewport(0, 0, width, height);
    }
}
