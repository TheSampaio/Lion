#include <Lion/Lion.h>

using namespace Lion;

// Sample native script: spins its entity around the z axis.
//
// It exists so the editor's script list is not empty and the whole path (registry -> component ->
// play mode) can be exercised. Games register their own scripts the same way.
class Spinner : public Script
{
public:
	void OnUpdate() override
	{
		const Reference<Transform> transform = GetTransform();
		const Vector rotation = transform->GetRotation();

		transform->SetRotation(Vector(rotation.x, rotation.y, rotation.z + kDegreesPerSecond * Clock::GetDeltaTime()));
	}

private:
	static constexpr float32 kDegreesPerSecond = 90.0f;
};

LION_REGISTER_SCRIPT(Spinner)
