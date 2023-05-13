#ifndef OWL_ENTITY_H
#define OWL_ENTITY_H

#include "../../Graphics/Include/Mesh.h"
#include "../../Math/Include/Vector.h"
#include "../../Math/Include/Colour.h"

class Entity
{
public:
	Entity() {};
	virtual ~Entity() {};

	virtual void Start() = 0;
	virtual void Update(float DeltaTime) = 0;
	virtual void Draw() = 0;
};

#endif // !OWL_ENTITY_H
