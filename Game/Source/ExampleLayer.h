#pragma once

#include <Lion/Core.h>

class ExampleLayer : public Lion::Layer
{
public:
	void OnAttach() override;
	void OnUpdate() override;
	void OnRender() override;
};
