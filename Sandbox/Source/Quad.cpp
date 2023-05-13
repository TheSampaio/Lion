#include "PCH.h"
#include "Quad.h"

Quad::Quad()
{
	std::vector<Vertex> Vertices
	{
		Vertex{ -Vector::Right + -Vector::Up, Colour::Blue + Colour::Red * 0.2f },
		Vertex{  Vector::Right + -Vector::Up, Colour::Blue + Colour::Red * 0.2f },
		Vertex{ -Vector::Right +  Vector::Up, Colour::Blue + Colour::Red * 0.8f },
		Vertex{  Vector::Right +  Vector::Up, Colour::Blue + Colour::Red * 0.8f },
	};

	std::vector<Index> Indices
	{
		Index { 0, 1, 2 },
		Index { 2, 1, 3 }
	};

	StaticMesh = new Mesh(Vertices, Indices);
}

Quad::~Quad()
{
	delete StaticMesh;

	Debug::Log(Information, "Quad destroyed!");
}

void Quad::Start()
{
	Debug::Log(Information, "Quad spawned!");
}

void Quad::Update(float DeltaTime)
{
}

void Quad::Draw()
{
	StaticMesh->Draw();
}
