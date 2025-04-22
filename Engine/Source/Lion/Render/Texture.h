#pragma once

#include <Lion/Base/Platform.h>
#include <Lion/Type/Size.h>

namespace Lion
{
	class Texture
	{
	public:
		LION_API Texture(const std::string& filePath);
		LION_API ~Texture();

		LION_API uint32 GetId() const { return mId; }
		LION_API Size GetSize() const { return mSize; }
		LION_API Size GetCenter() const { return mCenter; }

	private:
		uint32 mId;
		int32 mFilterMin;
		int32 mFilterMag;

		int32 mFormatInternal;
		uint32 mFormatExternal;

		int32 mColumn;

		Size mSize;
		Size mCenter;

#if LN_DEBUG
		static uint32 sAllocationCount;
#endif
	};
}
