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
		ID3D11InputLayout* m_D3D11InputLayout;         // input layout
		ID3D11VertexShader* m_D3D11VertexShader;       // vertex shader
		ID3D11PixelShader* m_D3D11PixelShader;         // pixel shader
		ID3D11RasterizerState* m_D3D11RasterizerState; // estado do rasterizador
		ID3D11SamplerState* m_D3D11SamplerState;       // estado do amostrador de textura
		ID3D11Buffer* m_D3D11VertexBuffer;             // buffer de vértices
		ID3D11Buffer* m_D3D11IndexBuffer;              // buffer de índices
		ID3D11Buffer* m_D3D11ConstantBuffer;           // buffer para o shader
		uint m_VertexBufferPosition;                   // posição atual do vertex buffer

		// Sprite's vector
		std::vector<struct Sinfo*> m_SpriteVector;

		// MAIN methods
		bool Initialize();
		void Finalize();

		void Draw(struct Sinfo* Data);
		void Render();

		// renderiza um grupo de sprites de mesma textura
		void RenderBatch(ID3D11ShaderResourceView* Texture, struct Sinfo** Sprites, uint Amount);

		// Static attributes
		static const uint m_MinBatchSize = 128;    // tamanho mínimo do lote de sprites
		static const uint m_MaxBatchSize = 4096;   // tamanho máximo do lote de sprites
		static const uint m_VerticesPerSprite = 4; // número de vértices por sprite
		static const uint m_IndicesPerSprite = 6;  // número de índices por sprite
	};
}
