#ifndef SANDBOX_GAME_H
#define SANDBOX_GAME_H

// Main game's class
class Sandbox : public Application
{
public:
    Sandbox();

    // Main methods
    void Start();
    void Update(float DeltaTime);
    void Draw();
    void Finalize();

private:
    // Create static meshes
    Mesh* Quads;
};

#endif

// === Vertices (Temporary) =====
static const std::vector<Vertex> Vertices
{
	Vertex{ glm::vec4{ -1.0f,  0.0f, 0.0f, 1.0f }, glm::vec4{ 1.0f, 0.0f, 0.0f, 1.0f } }, // 0
	Vertex{ glm::vec4{  0.0f,  0.0f, 0.0f, 1.0f }, glm::vec4{ 1.0f, 0.0f, 0.0f, 1.0f } }, // 1
	Vertex{ glm::vec4{ -1.0f,  1.0f, 0.0f, 1.0f }, glm::vec4{ 1.0f, 0.8f, 0.0f, 1.0f } }, // 2
	Vertex{ glm::vec4{  0.0f,  1.0f, 0.0f, 1.0f }, glm::vec4{ 1.0f, 0.8f, 0.0f, 1.0f } }, // 3

	Vertex{ glm::vec4{  0.0f,  0.0f, 0.0f, 1.0f }, glm::vec4{ 0.0f, 1.0f, 0.0f, 1.0f } }, // 4
	Vertex{ glm::vec4{  1.0f,  0.0f, 0.0f, 1.0f }, glm::vec4{ 0.0f, 1.0f, 0.0f, 1.0f } }, // 5
	Vertex{ glm::vec4{ -0.0f,  1.0f, 0.0f, 1.0f }, glm::vec4{ 0.6f, 1.0f, 0.6f, 1.0f } }, // 6
	Vertex{ glm::vec4{  1.0f,  1.0f, 0.0f, 1.0f }, glm::vec4{ 0.6f, 1.0f, 0.6f, 1.0f } }, // 7

	Vertex{ glm::vec4{ -1.0f, -1.0f, 0.0f, 1.0f }, glm::vec4{ 0.0f, 0.0f, 1.0f, 1.0f } }, // 8
	Vertex{ glm::vec4{  0.0f, -1.0f, 0.0f, 1.0f }, glm::vec4{ 0.0f, 0.0f, 1.0f, 1.0f } }, // 9
	Vertex{ glm::vec4{ -1.0f,  0.0f, 0.0f, 1.0f }, glm::vec4{ 0.0f, 0.8f, 1.0f, 1.0f } }, // 10
	Vertex{ glm::vec4{  0.0f,  0.0f, 0.0f, 1.0f }, glm::vec4{ 0.0f, 0.8f, 1.0f, 1.0f } }, // 11

	Vertex{ glm::vec4{  0.0f, -1.0f, 0.0f, 1.0f }, glm::vec4{ 1.0f, 1.0f, 0.0f, 1.0f } }, // 12
	Vertex{ glm::vec4{  1.0f, -1.0f, 0.0f, 1.0f }, glm::vec4{ 1.0f, 1.0f, 0.0f, 1.0f } }, // 13
	Vertex{ glm::vec4{  0.0f,  0.0f, 0.0f, 1.0f }, glm::vec4{ 1.0f, 1.0f, 0.6f, 1.0f } }, // 14
	Vertex{ glm::vec4{  1.0f,  0.0f, 0.0f, 1.0f }, glm::vec4{ 1.0f, 1.0f, 0.6f, 1.0f } }, // 15
};

// === Indices (Temporary) =====
static const std::vector<Index> Indices
{
	Index{ 0, 1, 2 },
	Index{ 2, 1, 3 },

	Index{ 4, 5, 6 },
	Index{ 6, 5, 7 },

	Index{  8, 9, 10 },
	Index{ 10, 9, 11 },

	Index{ 12, 13, 14 },
	Index{ 14, 13, 15 },
};