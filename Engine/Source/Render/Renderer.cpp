#include "Engine.h"
#include "Renderer.h"

#include "Camera.h"
#include "RenderCommand.h"
#include "SpriteInfo.h"

#include "../Core/Log.h"

namespace Lion
{
    // Vertices's array
    const std::array<float32, 32> mVertices
    {
        // === Position         // === Color         // === UV
        -0.5f, -0.5f, 0.0f,     1.0f, 1.0f, 1.0f,    0.0f, 0.0f,  // P0
         0.5f, -0.5f, 0.0f,     1.0f, 1.0f, 1.0f,    1.0f, 0.0f,  // P1
        -0.5f,  0.5f, 0.0f,     1.0f, 1.0f, 1.0f,    0.0f, 1.0f,  // P2
         0.5f,  0.5f, 0.0f,     1.0f, 1.0f, 1.0f,    1.0f, 1.0f   // P3
    };

    // Indices's array
    const std::array<uint32, 6> mIndices
    {
        0, 2, 1,
        2, 3, 1
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
        // Parse & Compile
        ShaderSource source = Parse("Resource/Shader/Lit.glsl");
        uint32 vertexShader = Compile(GL_VERTEX_SHADER, source.vertex);
        uint32 fragmentShader = Compile(GL_FRAGMENT_SHADER, source.fragment);

        // Attach & Link
        Attacher(vertexShader, fragmentShader);

        if (!Linker(sInstance->mShaderProgram))
        {
            Log::Console(LogLevel::Error, "[Renderer] Failed to link shader program.");
            return false;
        }

        // Generates a VAO
        glGenVertexArrays(1, &sInstance->mVAO);
        glBindVertexArray(sInstance->mVAO);

        // Generates a VBO and set-ups it
        glGenBuffers(1, &sInstance->mVBO);
        glBindBuffer(GL_ARRAY_BUFFER, sInstance->mVBO);
        glBufferData(GL_ARRAY_BUFFER, mVertices.size() * sizeof(float32), mVertices.data(), GL_STATIC_DRAW);

        // Generates a EBO and set-ups it
        glGenBuffers(1, &sInstance->mEBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sInstance->mEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, mIndices.size() * sizeof(uint32), mIndices.data(), GL_STATIC_DRAW);

        // Set-ups the VAO's layouts
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float32), nullptr);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float32), reinterpret_cast<void*>(3 * sizeof(float32)));
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float32), reinterpret_cast<void*>(6 * sizeof(float32)));

        // Enables the VAO's layouts
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);

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
        glUseProgram(Renderer::sInstance->mShaderProgram);
        glBindVertexArray(sInstance->mVAO);

        camera->OnUsage();

        RenderCommand::SetShaderMatrix4(sInstance->mShaderProgram, "uView", camera->GetViewMatrix());
        RenderCommand::SetShaderMatrix4(sInstance->mShaderProgram, "uProjection", camera->GetProjectionMatrix());
    }

    void Renderer::RenderEnd()
    {
        if (sInstance->mSpriteBuffer.empty())
            return;

        // Sort the sprites by depth
        std::sort(sInstance->mSpriteBuffer.begin(), sInstance->mSpriteBuffer.end(),
            [](const auto& a, const auto& b)
            {
                //Sort from furthest (largest) to closest (smallest)
                return a->depth > b->depth;
            });

        for (const auto& sprite : sInstance->mSpriteBuffer)
        {
            // Define and setup the model matrix
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(sprite->x, sprite->y, sprite->depth));
            model = glm::rotate(model, glm::radians(sprite->rotation), glm::vec3(0.0f, 0.0f, 1.0f));
            model = glm::scale(model, glm::vec3(sprite->width * sprite->scale, sprite->height * sprite->scale, 1.0f));

            // Send the matrix to the shader
            RenderCommand::SetShaderMatrix4(sInstance->mShaderProgram, "uModel", model);

            // Creates a uniform sampler and binds the generated texture
            RenderCommand::SetShaderInt(sInstance->mShaderProgram, "uDiffuseSampler", 0);
            RenderCommand::BindTexture2D(sprite->texture);

            // Draw a triangle using the EBO set-up
            RenderCommand::DrawIndexedQuads(static_cast<int32>(sInstance->mSpriteBuffer.size()));
        }

        sInstance->mSpriteBuffer.clear();

#ifdef LN_DEBUG
        RenderCommand::BindTexture2D(0);
        glBindVertexArray(0);
        glUseProgram(0);

#endif // LN_DEBUG
    }

    void Renderer::Submit(SpriteInfo* spriteInfo)
    {
        Renderer::sInstance->mSpriteBuffer.push_back(spriteInfo);
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

            Log::Console(LogLevel::Error, LN_LOG_FORMAT("[Renderer] Failed to compile {} shader.", (type == GL_VERTEX_SHADER) ? "vertex" : "fragment"));
            Log::Console(LogLevel::Error, message.data());

            glDeleteShader(id);
            return 0;
        }

        return id;
    }

    uint32 Renderer::Attacher(uint32 vertexShader, uint32 fragmentShader)
    {
        sInstance->mShaderProgram = glCreateProgram();
        glAttachShader(sInstance->mShaderProgram, vertexShader);
        glAttachShader(sInstance->mShaderProgram, fragmentShader);
        return sInstance->mShaderProgram;
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
