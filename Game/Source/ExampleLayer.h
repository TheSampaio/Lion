#pragma once

#include <Lion/Core.h>

class ExampleLayer : public Lion::Layer
{
public:
	void OnAttach() override;
	void OnEvent(Lion::Event& event) override;

private:
	bool OnEventInputMouseMove(const Lion::EventInputMouseMove& event);
};
