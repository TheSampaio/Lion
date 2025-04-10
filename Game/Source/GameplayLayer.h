#pragma once

#include <Lion/Core.h>

class GameplayLayer : public Lion::Layer
{
public:
	void OnAttach() override;
	void OnCreate() override;
	void OnRender() override;
	void OnDetach() override;

private:
	GLuint mVAO = 0, mVBO = 0, mEBO = 0;
	Lion::Texture* mTexture;

    // Vertices's dynamic array
    const std::array<GLfloat, 32> mVertices
    {
        // === Position        // === Color           // === UV
        -0.8f, -0.8f,  0.0f,    1.0f,  1.0f,  1.0f,    0.0f,  0.0f,
         0.8f, -0.8f,  0.0f,    1.0f,  1.0f,  1.0f,    1.0f,  0.0f,
        -0.8f,  0.8f,  0.0f,    1.0f,  1.0f,  1.0f,    0.0f,  1.0f,
         0.8f,  0.8f,  0.0f,    1.0f,  1.0f,  1.0f,    1.0f,  1.0f
    };

    // Indices's array
    const std::array<GLuint, 6> mIndices
    {
        0, 1, 2,
        2, 1, 3
    };
};
