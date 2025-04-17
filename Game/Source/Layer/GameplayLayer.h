#pragma once

#include <Lion/Core.h>

class GameplayLayer : public Lion::Layer
{
public:
	void OnCreate() override;
	void OnUpdate() override;
	void OnRender() override;

protected:
	void OnEvent(Lion::Event& event) override;
	bool OnEventWindowResize(const Lion::EventWindowResize& event);

private:
	Lion::Reference<Lion::CameraOrthographic> mCamera;
	Lion::Reference<Lion::Scene> mScene;
};
