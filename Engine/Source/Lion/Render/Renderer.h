#pragma once

namespace Lion
{
    class Application;
    class Camera;
    class Sprite;

    struct ShaderSource;
    struct SpriteInfo;
    struct Vertex;

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
        std::vector<uint32> mTextureSlots;  // Slot index -> GL texture id, rebuilt every frame.
        std::vector<Vertex> mVertexBuffer;

        Renderer();

        static bool Initialize();
        static void Submit(SpriteInfo* spriteInfo);

        // Writes the four vertices of a textured quad and returns the advanced buffer cursor.
        static Vertex* CreateQuad(Vertex* target, const SpriteInfo* spriteInfo, int32 textureSlot);

        static ShaderSource Parse(const std::string& filepath);
        static uint32 Compile(uint32 type, const std::string& source);
        static uint32 Attacher(uint32 vertexShader, uint32 fragmentShader);
        static bool Linker(uint32 program);

        static void OnWindowResize(uint32 width, uint32 height);
    };
}
