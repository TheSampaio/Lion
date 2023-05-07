#include "Core.h"
#include "Vector.h"

glm::vec3 Vector::Zero =    glm::vec3{ 0.0f,  0.0f,  0.0f, };
glm::vec3 Vector::Uniform = glm::vec3{ 1.0f,  1.0f,  1.0f, };

glm::vec3 Vector::Right = glm::vec3{ 1.0f,  0.0f,  0.0f, };
glm::vec3 Vector::Left =  glm::vec3{-1.0f,  0.0f,  0.0f, };

glm::vec3 Vector::Up =   glm::vec3{ 0.0f,  1.0f,  0.0f, };
glm::vec3 Vector::Down = glm::vec3{ 0.0f, -1.0f,  0.0f, };

glm::vec3 Vector::Forward =  glm::vec3{ 0.0f,  0.0f,  1.0f, };
glm::vec3 Vector::Backward = glm::vec3{ 0.0f,  0.0f, -1.0f, };
