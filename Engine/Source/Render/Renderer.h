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
        static LION_API uint32 GetShaderProgram() { return sInstance->mShaderProgram; }

        static LION_API void RenderBegin(const Reference<Camera> camera);

        static LION_API void RenderEnd();

        friend Application;
        friend Sprite;

    protected:
        static Renderer* sInstance;

        static void New();
        static void Delete();

        Renderer(const Renderer&) = delete;
        Renderer& operator=(const Renderer&) = delete;

    private:
        uint32 mShaderProgram;
        uint32 mVAO;
        uint32 mVBO;
        uint32 mEBO;

        std::vector<SpriteInfo*> mSpriteBuffer;

        Renderer();

        static bool Initialize();
        static void Submit(SpriteInfo* spriteInfo);

        static ShaderSource Parse(const std::string& filepath);
        static uint32 Compile(uint32 type, const std::string& source);
        static uint32 Attacher(uint32 vertexShader, uint32 fragmentShader);
        static bool Linker(uint32 program);

        static void OnWindowResize(uint32 width, uint32 height);
    };
}
