#include "Core.h"
#include "Include/Mesh.h"

#include "../Core/Include/Application.h"

#include "../Graphics/Include/VAO.h"
#include "../Graphics/Include/VBO.h"
#include "../Graphics/Include/EBO.h"

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
	Application::GetRenderer()->Draw(m_Indices);
}
