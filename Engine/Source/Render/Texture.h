#pragma once

namespace Lion
{
	class Texture
	{
	public:
		LION_API Texture(const std::string& filePath);
		LION_API ~Texture();

		LION_API uint32 GetId() const { return mId; }
		LION_API std::array<uint32, 2> GetSize() const { return mSize; }
		LION_API std::array<uint32, 2> GetCenter() const { return mCenter; }

		LION_API void Bind(uint32 slot = 0) const;

	private:
		uint32 mId;
		int32 mFilterMin;
		int32 mFilterMag;

		int32 mFormatInternal;
		uint32 mFormatExternal;

		int32 mColumn;

		std::array<uint32, 2> mSize;
		std::array<uint32, 2> mCenter;

#if LN_DEBUG
		static uint32 sAllocationCount;
#endif
	};
}
