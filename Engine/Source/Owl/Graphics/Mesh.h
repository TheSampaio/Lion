#ifndef OWL_MESH_H
#define OWL_MESH_H

#include "../Graphics/VAO.h"
#include "../Graphics/VBO.h"
#include "../Graphics/EBO.h"

class OWL_API Mesh
{
public:
	// TODO: Constructor with geometry
	Mesh(std::vector<Vertex> Vertices, std::vector<Index> Indices);
	~Mesh();

	// Main methods
	virtual void Draw();

private:
	// Attributes
	VAO* m_VertexArray;
	VBO* m_VertexBuffer;
	EBO* m_ElementBuffer;

	std::vector<Vertex> m_Vertices;
	std::vector<Index> m_Indices;

	// Static attributs
	static class Renderer& s_Renderer;
};

#endif // !OWL_MESH_H
