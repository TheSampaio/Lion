#include "Rotator.h"

#include <Lion/Logic/ComponentRegistry.h>

using namespace Lion;

void Rotator::OnUpdate()
{
	const Reference<Transform> transform = GetTransform();
	const Vector rotation = transform->GetRotation();

	transform->SetRotation(Vector(rotation.x, rotation.y, rotation.z + mDegreesPerSecond * Clock::GetDeltaTime()));
}

void Rotator::Serialize(Serializer& serializer) const
{
	serializer.Write("degreesPerSecond", mDegreesPerSecond);
}

void Rotator::Deserialize(const Serializer& serializer)
{
	mDegreesPerSecond = serializer.ReadFloat("degreesPerSecond", 90.0f);
}

LION_REGISTER_COMPONENT(Rotator)
