#include "Engine.h"
#include "Renderer.h"

#include "Camera.h"
#include "SpriteInfo.h"

#include "../Core/Log.h"

namespace Lion
{
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

    bool Renderer::Initialize()
    {
        // Parse & Compile
        ShaderSource source = Parse("Resource/Shader/Lit.glsl");
        GLuint vertexShader = Compile(GL_VERTEX_SHADER, source.vertex);
        GLuint fragmentShader = Compile(GL_FRAGMENT_SHADER, source.fragment);

        // Attach & Link
        Attacher(vertexShader, fragmentShader);
        return Linker(sInstance->mShaderProgram);
    }

    void Renderer::RenderBegin(const Camera& camera)
    {
    }

    void Renderer::RenderEnd()
    {
    }

    void Renderer::Submit(const SpriteInfo& spriteInfo)
    {
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
