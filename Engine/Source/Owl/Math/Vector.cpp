#include "Core.h"
#include "Include/Vector.h"

// Initializes static attributes
glm::vec3 Vector::Zero =     glm::vec3{ 0.0f,  0.0f,  0.0f, };
glm::vec3 Vector::Uniform =  glm::vec3{ 1.0f,  1.0f,  1.0f, };
glm::vec3 Vector::Right =    glm::vec3{ 1.0f,  0.0f,  0.0f, };
glm::vec3 Vector::Up =       glm::vec3{ 0.0f,  1.0f,  0.0f, };
glm::vec3 Vector::Forward =  glm::vec3{ 0.0f,  0.0f,  1.0f, };
