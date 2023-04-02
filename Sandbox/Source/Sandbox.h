#ifndef SANDBOX_GAME_H
#define SANDBOX_GAME_H

std::vector<Vertex> Vertices
{
    Vertex{ glm::vec4{ -0.5f, -0.5f, 0.0f, 1.0f }, glm::vec4{ 0.0f, 1.0f, 1.0f, 1.0f } },
    Vertex{ glm::vec4{  0.5f, -0.5f, 0.0f, 1.0f }, glm::vec4{ 0.0f, 1.0f, 1.0f, 1.0f } },
    Vertex{ glm::vec4{ -0.5f,  0.5f, 0.0f, 1.0f }, glm::vec4{ 0.0f, 0.0f, 1.0f, 1.0f } },
    Vertex{ glm::vec4{  0.5f,  0.5f, 0.0f, 1.0f }, glm::vec4{ 0.0f, 0.0f, 1.0f, 1.0f } },
};

std::vector<Index> Indices
{
    Index{ 0, 1, 2 },
    Index{ 2, 1, 3 },
};

// Main game's class
class Sandbox : public Application
{
public:
    Sandbox();

    // Main methods
    void Start();
    void Update(float DeltaTime);
    void DrawCall();
    void Finalize();

private:
    // Create shader program
    Shader* ShaderProgram;

    // Create buffers
    VAO* VertexArray;
    VBO* VertexBuffer;
    EBO* ElementBuffer;
};

#endif
