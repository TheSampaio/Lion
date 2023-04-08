#include "Core.h"
#include "Mesh.h"

#include "../Core/Application.h"
#include "../Core/Renderer.h"

#include "../Graphics/VAO.h"
#include "../Graphics/VBO.h"
#include "../Graphics/EBO.h"

// Initialize static attributes
Renderer& Mesh::s_Renderer = *Application::s_Renderer;

Mesh::Mesh(std::vector<Vertex> Vertices, std::vector<Index> Indices)
{
	m_Vertices = Vertices;
	m_Indices = Indices;

	// Create buffers
	m_VertexArray = new VAO;
	m_VertexBuffer = new VBO(Vertices);
	m_ElementBuffer = new EBO(Indices);

	m_VertexArray->AttribPointer(0, 4, sizeof(Vertex), 0);
	m_VertexArray->AttribPointer(1, 4, sizeof(Vertex), offsetof(Vertex, Vertex::Color));
}

Mesh::~Mesh()
{
	delete m_VertexBuffer;
	delete m_VertexArray;
	delete m_ElementBuffer;
}

void Mesh::Draw()
{
	m_VertexArray->Bind();
	s_Renderer.Draw(m_Indices);
}
