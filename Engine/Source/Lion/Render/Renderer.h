#pragma once

namespace Lion
{
    class Application;
    class Camera;
    class Sprite;

    class Shader;
    class Texture;
    class VertexArray;
    class VertexBuffer;
    class IndexBuffer;

    struct SpriteInfo;
    struct Vertex;

    // High-level 2D sprite renderer.
    //
    // Batches submitted sprites into a single dynamic vertex buffer, resolves per-frame texture
    // slots and issues one indexed draw call. It is fully backend-agnostic: it only talks to the
    // RHI abstractions (Shader, VertexArray, Buffer, Texture) and RenderCommand.
    // What one frame cost the renderer. A batcher's whole job is to turn many sprites into few draw
    // calls, and none of that is visible from the outside — so it says so, and the editor reads it.
    struct RenderStats
    {
        uint32 drawCalls = 0;      // How many times the GPU was actually asked to draw.
        uint32 sprites = 0;        // Sprites submitted, before batching.
        uint32 spritesDropped = 0; // Sprites the batch had no texture slot left for.
        uint32 textureSlots = 0;   // Distinct textures bound for the batch.
        uint32 vertices = 0;
        uint32 indices = 0;
    };

    class Renderer
    {
    public:
        // What the last completed frame cost. Reset when a batch begins, filled when it flushes.
        static LION_API const RenderStats& GetStats();

        // Clears the currently bound framebuffer with the given color.
        static LION_API void Clear(float32 red, float32 green, float32 blue, float32 alpha = 1.0f);

        // Begins a batch: binds the shared shader/geometry and uploads the camera matrices.
        static LION_API void RenderBegin(const Reference<Camera>& camera);

        // Ends a batch: builds geometry, binds textures and flushes a single draw call.
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
        Reference<Shader> mShader;
        Reference<VertexArray> mVertexArray;
        Reference<VertexBuffer> mVertexBuffer;
        Reference<IndexBuffer> mIndexBuffer;

        std::vector<SpriteInfo*> mSpriteBuffer;   // Sprites submitted this frame.
        std::vector<Texture*> mTextureSlots;      // Slot index -> texture, rebuilt every frame.
        std::vector<Vertex> mVertexData;          // CPU-side staging for the vertex buffer.
        RenderStats mStats;                       // What the last flush cost (see GetStats).

        Renderer() = default;

        static bool Initialize();
        static void Submit(SpriteInfo* spriteInfo);

        // Writes the four vertices of a textured quad and returns the advanced buffer cursor.
        static Vertex* CreateQuad(Vertex* target, const SpriteInfo* spriteInfo, int32 textureSlot);

        static void OnWindowResize(uint32 width, uint32 height);
    };
}
