#ifndef OWL_MESH_H
#define OWL_MESH_H

#include "../../Math/Include/Vertex.h"
#include "../../Math/Include/Index.h"

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
	class VAO* m_VertexArray;
	class VBO* m_VertexBuffer;
	class EBO* m_ElementBuffer;

	std::vector<Vertex> m_Vertices;
	std::vector<Index> m_Indices;
};

#endif // !OWL_MESH_H
