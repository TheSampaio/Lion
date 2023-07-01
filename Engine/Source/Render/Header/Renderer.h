#pragma once

namespace owl
{
	class Renderer
	{
	public:
		// === Friends ======
		friend class Application;
		friend class Sprite;

	protected:
		Renderer();

		// Deletes copy constructor and assigment operator
		Renderer(const Renderer&) = delete;
		Renderer operator=(const Renderer&) = delete;

		// Gets the class's static reference
		static Renderer& GetInstance() { static Renderer s_Instance; return s_Instance; }

	private:
		// Attributes
		ID3D11InputLayout* m_D3D11InputLayout;         // Input layout
		ID3D11VertexShader* m_D3D11VertexShader;       // Vertex shader
		ID3D11PixelShader* m_D3D11PixelShader;         // Pixel shader
		ID3D11RasterizerState* m_D3D11RasterizerState; // Rasterizer state
		ID3D11SamplerState* m_D3D11SamplerState;       // Sampler state
		ID3D11Buffer* m_D3D11VertexBuffer;             // Vertex buffer
		ID3D11Buffer* m_D3D11IndexBuffer;              // Index buffer
		ID3D11Buffer* m_D3D11ConstantBuffer;           // Constant buffer
		uint m_VertexBufferPosition;                   // Current vertex buffer's position

		// Sprite's vector
		std::vector<struct Sinfo*> m_SpriteVector;

		// MAIN methods
		bool Initialize();
		void Finalize();

		void Draw(struct Sinfo* Data);
		void Render();

		// Renders a group of sprites of the same texture
		void RenderBatch(ID3D11ShaderResourceView* Texture, struct Sinfo** Sprites, uint Amount);

		// Static attributes
		static const uint m_MinBatchSize = 128;    // tamanho mínimo do lote de sprites
		static const uint m_MaxBatchSize = 4096;   // tamanho máximo do lote de sprites
		static const uint m_VerticesPerSprite = 4; // número de vértices por sprite
		static const uint m_IndicesPerSprite = 6;  // número de índices por sprite
	};
}
