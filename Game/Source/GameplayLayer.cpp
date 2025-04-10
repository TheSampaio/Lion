#include "GameplayLayer.h"

using namespace Lion;

void GameplayLayer::OnAttach()
{
	mTexture = nullptr;
}

void GameplayLayer::OnCreate()
{
    // Generates a VAO
    glGenVertexArrays(1, &mVAO);
    glBindVertexArray(mVAO);

    // Generates a VBO and set-ups it
    glGenBuffers(1, &mVBO);
    glBindBuffer(GL_ARRAY_BUFFER, mVBO);
    glBufferData(GL_ARRAY_BUFFER, mVertices.size() * sizeof(GLfloat), mVertices.data(), GL_STATIC_DRAW);

    // Generates a EBO and set-ups it
    glGenBuffers(1, &mEBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mIndices.size() * sizeof(GLuint), mIndices.data(), GL_STATIC_DRAW);

    // Set-ups the VAO's layouts
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), nullptr);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), reinterpret_cast<void*>(3 * sizeof(GLfloat)));
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), reinterpret_cast<void*>(6 * sizeof(GLfloat)));

    // Enables the VAO's layouts
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    // Unbind VAO and VBO to avoid bugs
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	mTexture = new Texture("Resource/Sprite/showcase-alex-kidd-remastered.jpg");
}

void GameplayLayer::OnRender()
{
    // Informs OpenGL which shader program and VAO we want to use
    const auto shaderProgram = Renderer::GetShaderProgram();
    glUseProgram(shaderProgram);
    glBindVertexArray(mVAO);

    // Creates a uniform sampler and binds the generated texture
    glUniform1i(glGetUniformLocation(shaderProgram, "uDiffuseSampler"), 0);
    mTexture->Bind();

    // Draw a triangle using the EBO set-up
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(mIndices.size()), GL_UNSIGNED_INT, nullptr);

    // Unbind everything binded to avoid bugs
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindVertexArray(0);
    glUseProgram(0);
}

void GameplayLayer::OnDetach()
{
	delete mTexture;

    glDeleteBuffers(1, &mVBO);
    glDeleteBuffers(1, &mEBO);
    glDeleteVertexArrays(1, &mVAO);
}
