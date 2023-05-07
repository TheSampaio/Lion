#ifndef OWL_VECTOR_H
#define OWL_VECTOR_H

struct OWL_API Vector
{
	static glm::vec3 Zero;
	static glm::vec3 Uniform;

	static glm::vec3 Right;
	static glm::vec3 Left;

	static glm::vec3 Up;
	static glm::vec3 Down;

	static glm::vec3 Forward;
	static glm::vec3 Backward;
};

#endif // !OWL_VECTOR_H
