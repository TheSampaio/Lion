#pragma once

#include <Lion/Logic/Component.h>
#include <Lion/Math/Vector.h>

namespace Lion
{
	// Places an entity in the game's logical screen frame. Widget Assemblies use this component on
	// their visual entities so authored UI follows a normalized anchor instead of a world location.
	class WidgetAnchor final : public Component
	{
	public:
		LION_API const Vector& GetAnchor() const { return mAnchor; }
		LION_API const Vector& GetOffset() const { return mOffset; }
		LION_API void SetAnchor(const Vector& anchor) { mAnchor = anchor; }
		LION_API void SetOffset(const Vector& offset) { mOffset = offset; }

		LION_API void OnAwake() override;
		LION_API void OnUpdateBegin() override;
		LION_API void Reflect(Reflector& reflector) override;

	private:
		Vector mAnchor{ 0.5f, 0.5f, 0.0f };
		Vector mOffset;

		void ApplyLayout();
	};
}
