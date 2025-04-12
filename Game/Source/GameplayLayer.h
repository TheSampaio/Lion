#pragma once

#include <Lion/Core.h>

class GameplayLayer : public Lion::Layer
{
public:
	void OnAttach() override;
	void OnCreate() override;
	void OnRender() override;
	void OnDetach() override;

	void OnEvent(Lion::Event& event) override;

private:
	Lion::CameraOrthographic* mCamera;
	Lion::Sprite* mSprite;

	bool OnEventWindowResize(const Lion::EventWindowResize& event);
};
