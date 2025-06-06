#pragma once

namespace Lion
{
	class Camera
	{
	public:
		LION_API Camera();

		virtual const LION_API glm::mat4& GetViewMatrix() const { return mView; }
		virtual const LION_API glm::mat4& GetProjectionMatrix() const { return mProjection; }

		virtual LION_API void OnResize(float32 width, float32 height) = 0;
		virtual LION_API void OnUsage() = 0;

	protected:
		glm::mat4 mView;
		glm::mat4 mProjection;

		glm::vec3 mPosition;
		glm::vec3 mRotation;
		const glm::vec3 mUp = glm::vec3{ 0.0f, 1.0f, 0.0f };

		float32 mAspectRatio;
		float32 mNearPlane;
		float32 mFarPlane;
		float32 mFieldOfView;
	};
}
