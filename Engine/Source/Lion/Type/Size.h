#pragma once

#include <Lion/Base/Platform.h>

namespace Lion
{
	struct Size
	{
		float32 width, height;

		LION_API Size() : width(0.0f), height(0.0f) {}

		LION_API Size(float32 value) : width(value), height(value) {}
		LION_API Size(int32 value) : width(static_cast<float32>(value)), height(static_cast<float32>(value)) {}
		LION_API Size(uint32 value) : width(static_cast<float32>(value)), height(static_cast<float32>(value)) {}

		LION_API Size(float32 width, float32 height) : width(width), height(height) {}
		LION_API Size(int32 width, int32 height) : width(static_cast<float32>(width)), height(static_cast<float32>(height)) {}
		LION_API Size(uint32 width, uint32 height) : width(static_cast<float32>(width)), height(static_cast<float32>(height)) {}

		LION_API Size ToInt32() const { return { static_cast<int32>(width), static_cast<int32>(height) }; }
		LION_API Size ToUint32() const { return { static_cast<uint32>(width), static_cast<uint32>(height) } ; }

		LION_API Size& operator=(const Size& other)
		{
			if (this != &other)
			{
				std::swap(width, const_cast<Size&>(other).width);
				std::swap(height, const_cast<Size&>(other).height);
			}

			return *this;
		}

		LION_API Size operator*(float32 scalar) const { return { width * scalar, height * scalar }; }
		LION_API Size operator*(int32 scalar) const { return { static_cast<int32>(width) * scalar, static_cast<int32>(height) * scalar }; }
		LION_API Size operator*(uint32 scalar) const { return { static_cast<uint32>(width) * scalar, static_cast<uint32>(height) * scalar }; }
	};
}
