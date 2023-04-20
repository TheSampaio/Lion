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
    Mesh* Quad01;
    Mesh* Quad02;
    Mesh* Quad03;
    Mesh* Quad04;
};

#endif

// === Temporary data bellow =====
static const std::vector<Vertex> Vertices01
{
	Vertex{ glm::vec4{ -1.0f,  0.0f, 0.0f, 1.0f }, glm::vec4{ 1.0f, 0.0f, 0.0f, 1.0f } },
	Vertex{ glm::vec4{  0.0f,  0.0f, 0.0f, 1.0f }, glm::vec4{ 1.0f, 0.0f, 0.0f, 1.0f } },
	Vertex{ glm::vec4{ -1.0f,  1.0f, 0.0f, 1.0f }, glm::vec4{ 1.0f, 0.8f, 0.0f, 1.0f } },
	Vertex{ glm::vec4{  0.0f,  1.0f, 0.0f, 1.0f }, glm::vec4{ 1.0f, 0.8f, 0.0f, 1.0f } },
};

static const std::vector<Vertex> Vertices02
{
	Vertex{ glm::vec4{  0.0f,  0.0f, 0.0f, 1.0f }, glm::vec4{ 0.0f, 1.0f, 0.0f, 1.0f } },
	Vertex{ glm::vec4{  1.0f,  0.0f, 0.0f, 1.0f }, glm::vec4{ 0.0f, 1.0f, 0.0f, 1.0f } },
	Vertex{ glm::vec4{ -0.0f,  1.0f, 0.0f, 1.0f }, glm::vec4{ 0.6f, 1.0f, 0.6f, 1.0f } },
	Vertex{ glm::vec4{  1.0f,  1.0f, 0.0f, 1.0f }, glm::vec4{ 0.6f, 1.0f, 0.6f, 1.0f } },
};

static const std::vector<Vertex> Vertices03
{
	Vertex{ glm::vec4{ -1.0f, -1.0f, 0.0f, 1.0f }, glm::vec4{ 0.0f, 0.0f, 1.0f, 1.0f } },
	Vertex{ glm::vec4{  0.0f, -1.0f, 0.0f, 1.0f }, glm::vec4{ 0.0f, 0.0f, 1.0f, 1.0f } },
	Vertex{ glm::vec4{ -1.0f,  0.0f, 0.0f, 1.0f }, glm::vec4{ 0.0f, 0.8f, 1.0f, 1.0f } },
	Vertex{ glm::vec4{  0.0f,  0.0f, 0.0f, 1.0f }, glm::vec4{ 0.0f, 0.8f, 1.0f, 1.0f } },
};

static const std::vector<Vertex> Vertices04
{
	Vertex{ glm::vec4{  0.0f, -1.0f, 0.0f, 1.0f }, glm::vec4{ 1.0f, 1.0f, 0.0f, 1.0f } },
	Vertex{ glm::vec4{  1.0f, -1.0f, 0.0f, 1.0f }, glm::vec4{ 1.0f, 1.0f, 0.0f, 1.0f } },
	Vertex{ glm::vec4{  0.0f,  0.0f, 0.0f, 1.0f }, glm::vec4{ 1.0f, 1.0f, 0.6f, 1.0f } },
	Vertex{ glm::vec4{  1.0f,  0.0f, 0.0f, 1.0f }, glm::vec4{ 1.0f, 1.0f, 0.6f, 1.0f } },
};

static const std::vector<Index> Indices
{
	Index{ 0, 1, 2 },
	Index{ 2, 1, 3 },
};