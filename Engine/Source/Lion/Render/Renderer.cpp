#include "Engine.h"
#include "Renderer.h"

#include <Lion/Core/Log.h>

#include <Lion/Render/Camera.h>
#include <Lion/Render/RenderCommand.h>
#include <Lion/Render/Sprite.h>

namespace Lion
{
    static const size_t maxQuadCount = 1000;
    static const size_t maxVertexCount = maxQuadCount * 4;
    static const size_t maxIndexCount = maxQuadCount * 6;
    static const size_t maxTextureCount = 32;

#ifdef LN_DEBUG
    static uint32 sDrawCallCount = 0;

#endif // LN_DEBUG

    struct Vertex
    {
        glm::vec3 position;
        glm::vec4 color;
        glm::vec2 textureCoord;
        float32 texture;
    };

    Renderer* Renderer::sInstance = nullptr;

    struct ShaderSource
    {
        std::string vertex;
        std::string fragment;
    };

    void Renderer::OnWindowResize(uint32 width, uint32 height)
    {
        RenderCommand::CreateViewport(0, 0, width, height);
    }

    void Renderer::New()
    {
        sInstance = new Renderer();
    }

    void Renderer::Delete()
    {
        delete sInstance;
        sInstance = nullptr;
    }

    Renderer::Renderer()
    {
        mShaderProgram = 0;
        mVAO = 0;
        mVBO = 0;
        mEBO = 0;
    }

    bool Renderer::Initialize()
    {
        auto* self = sInstance;

        {
            // Parse & Compile
            ShaderSource source = Parse("Resource/Shader/Lit.glsl");
            uint32 vertexShader = Compile(GL_VERTEX_SHADER, source.vertex);
            uint32 fragmentShader = Compile(GL_FRAGMENT_SHADER, source.fragment);

            // Attach & Link
            Attacher(vertexShader, fragmentShader);

            if (!Linker(self->mShaderProgram))
            {
                Log::Console(LogLevel::Error, "[Renderer] Failed to link shader program.");
                return false;
            }
        }
       
        // Generates a VAO
        glGenVertexArrays(1, &self->mVAO);
        glBindVertexArray(self->mVAO);

        // Generates a VBO and set-ups it
        glGenBuffers(1, &self->mVBO);
        glBindBuffer(GL_ARRAY_BUFFER, self->mVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * maxVertexCount, nullptr, GL_DYNAMIC_DRAW);

        // Set-ups the VAO's layouts
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, Vertex::position)));
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, Vertex::color)));
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, Vertex::textureCoord)));
        glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, Vertex::texture)));

        // Enables the VAO's layouts
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);
        glEnableVertexAttribArray(3);

        {
            // Generates an EBO and set-ups it
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

            glGenBuffers(1, &self->mEBO);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, self->mEBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32) * indices.size(), indices.data(), GL_STATIC_DRAW);
        }

        self->mSpriteBuffer.reserve(maxQuadCount);
        self->mTextureSlots.reserve(maxTextureCount);
        self->mVertexBuffer.resize(maxVertexCount);

#ifdef LN_DEBUG
        // Unbind VAO and VBO to avoid bugs
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

#endif // LN_DEBUG

        // Enable transparency
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        return true;
    }

    void Renderer::RenderBegin(const Reference<Camera> camera)
    {
#ifdef LN_DEBUG
        sDrawCallCount = 0;

#endif // LN_DEBUG

        auto* self = sInstance;

        glUseProgram(self->mShaderProgram);
        glBindVertexArray(self->mVAO);

        camera->OnUsage();

        RenderCommand::SetShaderMatrix4(self->mShaderProgram, "uView", camera->GetViewMatrix());
        RenderCommand::SetShaderMatrix4(self->mShaderProgram, "uProjection", camera->GetProjectionMatrix());
    }

    void Renderer::RenderEnd()
    {
        auto* self = sInstance;

        if (self->mSpriteBuffer.empty())
            return;

        // Sort sprites by depth (back to front)
        std::sort(self->mSpriteBuffer.begin(), self->mSpriteBuffer.end(),
            [](const auto& a, const auto& b)
            {
                return a->position.z > b->position.z;
            });

        uint32 indexCount = 0;

        // Rebuild the texture-slot table for this frame. Index 0 is reserved, so real
        // sprites always map to a slot >= 1. Keeping this per frame avoids stale bindings.
        auto& textureSlots = self->mTextureSlots;
        textureSlots.assign(1, 0);  // Slot 0 is reserved and never sampled.

        Vertex* buffer = self->mVertexBuffer.data();

        for (const SpriteInfo* sprite : self->mSpriteBuffer)
        {
            const uint32 textureId = sprite->texture;

            // Find the slot already assigned to this texture, if any (linear scan over <= 32).
            int32 slot = -1;
            for (uint32 i = 1; i < textureSlots.size(); ++i)
            {
                if (textureSlots[i] == textureId)
                {
                    slot = static_cast<int32>(i);
                    break;
                }
            }

            // Assign a new slot when the texture is seen for the first time this frame.
            if (slot < 0)
            {
                if (textureSlots.size() >= maxTextureCount)
                    continue;  // Texture budget exhausted for this batch; skip the sprite.

                slot = static_cast<int32>(textureSlots.size());
                textureSlots.push_back(textureId);
            }

            buffer = CreateQuad(buffer, sprite, slot);
            indexCount += 6;
        }

        // Upload only the vertices actually produced this frame.
        const size_t vertexCount = static_cast<size_t>(buffer - self->mVertexBuffer.data());
        glBindBuffer(GL_ARRAY_BUFFER, self->mVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vertex) * vertexCount, self->mVertexBuffer.data());

        {
            // Sampler i reads from texture unit i.
            int32 samplers[maxTextureCount] = { 0 };

            for (int32 i = 0; i < static_cast<int32>(maxTextureCount); ++i)
                samplers[i] = i;

            glUniform1iv(glGetUniformLocation(self->mShaderProgram, "uDiffuseTextureArray"), maxTextureCount, samplers);
        }

        // Bind each used texture to the unit matching its slot index.
        for (uint32 slot = 1; slot < textureSlots.size(); ++slot)
            glBindTextureUnit(slot, textureSlots[slot]);

        // Draw
        RenderCommand::DrawIndexedQuads(indexCount);

        self->mSpriteBuffer.clear();

#ifdef LN_DEBUG
        sDrawCallCount++;

        glBindVertexArray(0);
        glUseProgram(0);
#endif
    }

    void Renderer::Submit(SpriteInfo* spriteInfo)
    {
        Renderer::sInstance->mSpriteBuffer.push_back(spriteInfo);
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

    uint32 Renderer::Compile(uint32 type, const std::string& source)
    {
        uint32 id = glCreateShader(type);
        const GLchar* const content = source.c_str();

        glShaderSource(id, 1, &content, nullptr);
        glCompileShader(id);

        // Log errors
        int32 result;
        glGetShaderiv(id, GL_COMPILE_STATUS, &result);

        if (result == GL_FALSE)
        {
            int32 length;
            glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);

            std::vector<GLchar> message(length);
            glGetShaderInfoLog(id, length, &length, message.data());

            Log::Console(LogLevel::Error, LION_FORMAT_TEXT("[Renderer] Failed to compile {} shader:\n{}", (type == GL_VERTEX_SHADER) ? "vertex" : "fragment", message.data()));

            glDeleteShader(id);
            return 0;
        }

        return id;
    }

    uint32 Renderer::Attacher(uint32 vertexShader, uint32 fragmentShader)
    {
        auto* self = sInstance;

        self->mShaderProgram = glCreateProgram();
        glAttachShader(self->mShaderProgram, vertexShader);
        glAttachShader(self->mShaderProgram, fragmentShader);
        return self->mShaderProgram;
    }

    ShaderSource Renderer::Parse(const std::string& filepath)
    {
        enum class ShaderType
        {
            None = -1,
            Vertex = 0,
            Fragment = 1,
        };

        std::ifstream file(filepath);
        std::string line;
        std::stringstream source[2];
        ShaderType shader = ShaderType::None;

        if (file.is_open())
        {
            while (std::getline(file, line))
            {
                if (line.find("#shader") != std::string::npos)
                {
                    if (line.find("vertex") != std::string::npos)
                        shader = ShaderType::Vertex;

                    else if (line.find("fragment") != std::string::npos)
                        shader = ShaderType::Fragment;
                }

                else if (shader != ShaderType::None)
                {
                    source[static_cast<int32>(shader)] << line << '\n';
                }
            }
        }

        else
            Log::Console(LogLevel::Warning, "[Renderer] Failed to locate shader file.");

        return { source[0].str(), source[1].str() };
    }

    bool Renderer::Linker(uint32 program)
    {
        glLinkProgram(program);

        int32 isLinked;
        glGetProgramiv(program, GL_LINK_STATUS, &isLinked);

        if (isLinked == GL_FALSE)
        {
            int32 length;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);

            std::vector<GLchar> message(length);
            glGetProgramInfoLog(program, length, &length, message.data());

            Log::Console(LogLevel::Error, LION_FORMAT_TEXT("[Renderer] Failed to link shader program: {}", message.data()));
            glDeleteProgram(program);
            return false;
        }

        return true;
    }
}
