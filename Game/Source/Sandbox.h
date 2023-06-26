#pragma once

#include <Owl.h>
#include "Resources.h"

class Sandbox : public owl::Game
{
public:
	Sandbox();

	void OnStart();
	void OnUpdate();
	void OnDraw();
	void OnFinish();
};

