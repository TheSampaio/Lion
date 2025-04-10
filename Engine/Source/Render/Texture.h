#pragma once

namespace Lion
{
	class Texture
	{
	public:
		LION_API Texture(const std::string& filePath);
		LION_API ~Texture();

		LION_API uint GetId() const { return mId; }
		LION_API std::array<int, 2> GetSize() const { return mSize; }
		LION_API std::array<int, 2> GetCenter() const { return mCenter; }

		LION_API void Bind(uint slot = 0) const;

	private:
		uint mId;
		int mFilterMin;
		int mFilterMag;

		int mFormatInternal;
		uint mFormatExternal;

		int mColumn;

		std::array<int, 2> mSize;
		std::array<int, 2> mCenter;
	};
}
