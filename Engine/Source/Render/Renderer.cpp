#include "Engine.h"
#include "Renderer.h"

#include "Camera.h"
#include "RenderCommand.h"
#include "SpriteInfo.h"

#include "../Core/Log.h"

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
        self->mTextureBuffer.resize(maxTextureCount, -1);
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
                return a->position.mData.z > b->position.mData.z;
            });

        uint32 indexCount = 0;

        {
            std::vector<uint32> boundTextures;
            boundTextures.reserve(static_cast<uint32>(self->mSpriteBuffer.size()));

            Vertex* buffer = self->mVertexBuffer.data();

            {
                // Start from 1 (0 is usually reserved)
                uint64 nextSlot = 1;

                for (const auto& sprite : self->mSpriteBuffer)
                {
                    uint32 textureId = sprite->texture;

                    if (self->mTextureBuffer[textureId] == -1)
                    {
                        if (nextSlot >= maxTextureCount)
                            continue;

                        self->mTextureBuffer[textureId] = static_cast<int32>(nextSlot++);
                        boundTextures.push_back(textureId);
                    }

                    // Assign texIndex to sprite
                    sprite->texture = self->mTextureBuffer[textureId];

                    // Create new quads dynamically
                    buffer = CreateQuad(buffer, sprite);
                    indexCount += 6;
                }
            }

            // Bind vertex buffer
            glBindBuffer(GL_ARRAY_BUFFER, self->mVBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vertex) * self->mVertexBuffer.size(), self->mVertexBuffer.data());

            {
                // Setup texture array uniforms
                int samplers[maxTextureCount] = { 0 };

                for (int i = 0; i < maxTextureCount; ++i)
                    samplers[i] = i;

                glUniform1iv(glGetUniformLocation(self->mShaderProgram, "uDiffuseTextureArray"), maxTextureCount, samplers);
            }

            // Bind the unique textures
            for (uint32 i = 0; i < boundTextures.size(); ++i)
                glBindTextureUnit(i + 1, boundTextures[i]);
        }

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

    Vertex* Renderer::CreateQuad(Vertex* target, SpriteInfo* spriteInfo)
    {
        const float32 x = spriteInfo->position.mData.x;
        const float32 y = spriteInfo->position.mData.y;
        const float32 z = spriteInfo->position.mData.z;
        const float32 halfWidth = spriteInfo->width * 0.5f;
        const float32 halfHeight = spriteInfo->height * 0.5f;

        // Top-Left
        target->position = { x - halfWidth, y + halfHeight, z };
        target->color = { 1.0f, 1.0f, 1.0f, 1.0f };
        target->textureCoord = { 0.0f, 1.0f };
        target->texture = static_cast<float32>(spriteInfo->texture);
        target++;

        // Bottom-Left
        target->position = { x - halfWidth, y - halfHeight, z };
        target->color = { 1.0f, 1.0f, 1.0f, 1.0f };
        target->textureCoord = { 0.0f, 0.0f };
        target->texture = static_cast<float32>(spriteInfo->texture);
        target++;

        // Bottom-Right
        target->position = { x + halfWidth, y - halfHeight, z };
        target->color = { 1.0f, 1.0f, 1.0f, 1.0f };
        target->textureCoord = { 1.0f, 0.0f };
        target->texture = static_cast<float32>(spriteInfo->texture);
        target++;

        // Top-Right
        target->position = { x + halfWidth, y + halfHeight, z };
        target->color = { 1.0f, 1.0f, 1.0f, 1.0f };
        target->textureCoord = { 1.0f, 1.0f };
        target->texture = static_cast<float32>(spriteInfo->texture);
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

            Log::Console(LogLevel::Error, LN_LOG_FORMAT("[Renderer] Failed to compile {} shader:\n{}", (type == GL_VERTEX_SHADER) ? "vertex" : "fragment", message.data()));

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

                else
                {
                    source[(int)shader] << line << '\n';
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

            Log::Console(LogLevel::Error, LN_LOG_FORMAT("[Renderer] Failed to link shader program: {}", message.data()));
            glDeleteProgram(program);
            return false;
        }

        return true;
    }
}
