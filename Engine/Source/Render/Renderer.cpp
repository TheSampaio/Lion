#include "Engine.h"
#include "Renderer.h"

#include "Camera.h"
#include "RenderCommand.h"
#include "SpriteInfo.h"

#include "../Core/Log.h"

namespace Lion
{
    // Vertices's dynamic array
    const std::array<GLfloat, 32> mVertices
    {
        // === Position        // === Color           // === UV
        -0.8f, -0.8f,  0.0f,    1.0f,  1.0f,  1.0f,    0.0f,  0.0f,
         0.8f, -0.8f,  0.0f,    1.0f,  1.0f,  1.0f,    1.0f,  0.0f,
        -0.8f,  0.8f,  0.0f,    1.0f,  1.0f,  1.0f,    0.0f,  1.0f,
         0.8f,  0.8f,  0.0f,    1.0f,  1.0f,  1.0f,    1.0f,  1.0f
    };

    // Indices's array
    const std::array<GLuint, 6> mIndices
    {
        0, 1, 2,
        2, 1, 3
    };

    Renderer* Renderer::sInstance = nullptr;

    struct ShaderSource
    {
        std::string vertex;
        std::string fragment;
    };

    void Renderer::OnWindowResize(uint width, uint height)
    {
        glViewport(0, 0, width, height);
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
        GLuint vertexShader = Compile(GL_VERTEX_SHADER, source.vertex);
        GLuint fragmentShader = Compile(GL_FRAGMENT_SHADER, source.fragment);

        // Attach & Link
        Attacher(vertexShader, fragmentShader);

        if (!Linker(sInstance->mShaderProgram))
        {
            Log::Console(ELogMode::Error, "[Renderer] Failed to link shader program.");
            return false;
        }

        // Generates a VAO
        glGenVertexArrays(1, &sInstance->mVAO);
        glBindVertexArray(sInstance->mVAO);

        // Generates a VBO and set-ups it
        glGenBuffers(1, &sInstance->mVBO);
        glBindBuffer(GL_ARRAY_BUFFER, sInstance->mVBO);
        glBufferData(GL_ARRAY_BUFFER, mVertices.size() * sizeof(GLfloat), mVertices.data(), GL_STATIC_DRAW);

        // Generates a EBO and set-ups it
        glGenBuffers(1, &sInstance->mEBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sInstance->mEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, mIndices.size() * sizeof(GLuint), mIndices.data(), GL_STATIC_DRAW);

        // Set-ups the VAO's layouts
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), nullptr);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), reinterpret_cast<void*>(3 * sizeof(GLfloat)));
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), reinterpret_cast<void*>(6 * sizeof(GLfloat)));

        // Enables the VAO's layouts
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);

        // Unbind VAO and VBO to avoid bugs
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        return true;
    }

    void Renderer::RenderBegin(Camera& camera)
    {
        glUseProgram(Renderer::sInstance->mShaderProgram);
        glBindVertexArray(sInstance->mVAO);

        camera.OnUsage();

        RenderCommand::SetUniformMatrix4fv(sInstance->mShaderProgram, "uView", camera.GetViewMatrix());
        RenderCommand::SetUniformMatrix4fv(sInstance->mShaderProgram, "uProjection", camera.GetProjectionMatrix());
    }

    void Renderer::RenderEnd()
    {
        for (const auto& sprite : sInstance->mSpriteBuffer)
        {
            // Define and send model matrix
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(sprite->x, sprite->y, sprite->depth));            // Apply sprite's translation
            model = glm::rotate(model, glm::radians(sprite->rotation), glm::vec3(0.0f, 0.0f, 1.0f));  // Apply sprite's rotation
            model = glm::scale(model, glm::vec3(sprite->scale));                                      // Apply sprite's scale

            // Enviar a matriz de modelo para o shader
            RenderCommand::SetUniformMatrix4fv(sInstance->mShaderProgram, "uModel", model);

            // Creates a uniform sampler and binds the generated texture
            RenderCommand::SetUniform1i(sInstance->mShaderProgram, "uDiffuseSampler", 0);
            glBindTexture(GL_TEXTURE_2D, sprite->texture);
        }

        // Draw a triangle using the EBO set-up
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(sInstance->mSpriteBuffer.size() * 6), GL_UNSIGNED_INT, nullptr);

        sInstance->mSpriteBuffer.clear();

        glBindTexture(GL_TEXTURE_2D, 0);
        glBindVertexArray(0);
        glUseProgram(0);
    }

    void Renderer::Submit(SpriteInfo* spriteInfo)
    {
        Renderer::sInstance->mSpriteBuffer.push_back(spriteInfo);
    }

    GLuint Renderer::Compile(GLuint type, const std::string& source)
    {
        GLuint id = glCreateShader(type);
        const GLchar* const content = source.c_str();

        glShaderSource(id, 1, &content, nullptr);
        glCompileShader(id);

        // Log errors
        GLint result;
        glGetShaderiv(id, GL_COMPILE_STATUS, &result);

        if (result == GL_FALSE)
        {
            GLint length;
            glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);

            std::vector<GLchar> message(length);
            glGetShaderInfoLog(id, length, &length, message.data());

            Log::Console(ELogMode::Error, LN_LOG_FORMAT("[Renderer] Failed to compile {} shader.", (type == GL_VERTEX_SHADER) ? "vertex" : "fragment"));
            Log::Console(ELogMode::Error, message.data());

            glDeleteShader(id);
            return 0;
        }

        return id;
    }

    GLuint Renderer::Attacher(GLuint vertexShader, GLuint fragmentShader)
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
            Log::Console(ELogMode::Warning, "[Renderer] Failed to locate shader file.");

        return { source[0].str(), source[1].str() };
    }

    bool Renderer::Linker(GLuint program)
    {
        glLinkProgram(program);

        GLint isLinked;
        glGetProgramiv(program, GL_LINK_STATUS, &isLinked);

        if (isLinked == GL_FALSE)
        {
            GLint length;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);

            std::vector<GLchar> message(length);
            glGetProgramInfoLog(program, length, &length, message.data());

            Log::Console(ELogMode::Error, "[Renderer] Failed to link shader program.");
            Log::Console(ELogMode::Error, message.data());

            glDeleteProgram(program);
            return false;
        }

        return true;
    }
}
