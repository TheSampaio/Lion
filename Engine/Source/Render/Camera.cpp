#include "Engine.h"
#include "Camera.h"

#include "../Core/Window.h"

namespace Lion
{
	Camera::Camera()
	{
		mView = glm::mat4(1.0f);
		mProjection = glm::mat4(1.0f);

		mPosition = glm::vec3(0.0f);
		mRotation = glm::vec3(0.0f);

		mAspectRatio = 1.0f;
		mNearPlane = 0.1f;
		mFarPlane = 1000.0f;
		mFieldOfView = 60.0f;
	}
}
