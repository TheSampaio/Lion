#pragma once

#include <Lion/Base/Platform.h>

namespace Lion
{
	class Scene;

	class Entity
	{
	public:
		LION_API Entity() = default;
		virtual LION_API ~Entity() = default;

		virtual LION_API Reference<Scene> GetScene() const { return mScene; }

		virtual void OnAwake() {};
		virtual void OnDestroy() {};

		virtual void OnUpdateBegin() {};
		virtual void OnUpdate() {};
		virtual void OnUpdateEnd() {};

		virtual void OnRender() {};

		friend Scene;

	private:
		Reference<Scene> mScene;
	};
}
