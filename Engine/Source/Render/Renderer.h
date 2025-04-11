#pragma once

namespace Lion
{
    class Application;
    class Camera;
    class Sprite;

    struct ShaderSource;
    struct SpriteInfo;

    class Renderer
    {
    public:
        static LION_API GLuint GetShaderProgram() { return sInstance->mShaderProgram; }

        static void OnWindowResize(uint width, uint height);

        friend Application;
        friend Sprite;

    protected:
        static Renderer* sInstance;

        static void New();
        static void Delete();

        Renderer(const Renderer&) = delete;
        Renderer& operator=(const Renderer&) = delete;

    private:
        GLuint mShaderProgram;

        Renderer() = default;

        static bool Initialize();
        static void RenderBegin(const Camera& camera);
        static void RenderEnd();
        static void Submit(const SpriteInfo& spriteInfo);

        static ShaderSource Parse(const std::string& filepath);
        static GLuint Compile(GLuint type, const std::string& source);
        static GLuint Attacher(GLuint vertexShader, GLuint fragmentShader);
        static bool Linker(GLuint program);
    };
}
