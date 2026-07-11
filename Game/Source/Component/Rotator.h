#pragma once

#include <Lion/Lion.h>
#include <Lion/Logic/Serializer.h>

// Sample user-defined component, compiled only into the game module.
//
// It spins its entity around the z axis while the scene is simulating. Its only purpose is to
// exercise the module -> registry -> editor path: because Game.dll is loaded by both the standalone
// launcher and the editor, this component shows up in the editor's Add Component list even though
// the engine and editor know nothing about it. A game defines its own components exactly like this,
// and the editor will later generate the boilerplate for new ones.
class Rotator : public Lion::Component
{
public:
	void OnUpdate() override;

	void Serialize(Lion::Serializer& serializer) const override;
	void Deserialize(const Lion::Serializer& serializer) override;

private:
	Lion::float32 mDegreesPerSecond = 90.0f;
};
