#pragma once

#include <Lion/Core.h>

class GameplayLayer : public Lion::Layer
{
public:
	void OnAttach() override;
	void OnUpdate() override;
};
