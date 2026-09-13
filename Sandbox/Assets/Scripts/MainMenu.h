#pragma once

#include <Lion/Lion.h>

// Starts the authored game flow from the main menu after any keyboard, mouse or gamepad button.
class MainMenu final : public Lion::Component
{
public:
	void OnUpdate() override;
	void Reflect(Lion::Reflector& reflector) override;

private:
	std::string mStartScene = "Scenes/Level01.lnscene";
};
