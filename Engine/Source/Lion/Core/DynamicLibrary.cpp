#include "Engine.h"
#include "DynamicLibrary.h"

#include <Lion/Core/Log.h>

namespace Lion
{
	DynamicLibrary::~DynamicLibrary()
	{
		Unload();
	}

	DynamicLibrary::DynamicLibrary(DynamicLibrary&& other) noexcept
		: mHandle(other.mHandle)
	{
		other.mHandle = nullptr;
	}

	DynamicLibrary& DynamicLibrary::operator=(DynamicLibrary&& other) noexcept
	{
		if (this != &other)
		{
			Unload();
			mHandle = other.mHandle;
			other.mHandle = nullptr;
		}

		return *this;
	}

	bool DynamicLibrary::Load(const std::string& path)
	{
		Unload();

#if LN_PLATFORM_WIN
		mHandle = ::LoadLibraryA(path.c_str());

		if (!mHandle)
			Log::Console(LogLevel::Error, LION_FORMAT_TEXT("[DynamicLibrary] Failed to load '{}' (error {}).", path, ::GetLastError()));
#endif

		return mHandle != nullptr;
	}

	void DynamicLibrary::Unload()
	{
		if (!mHandle)
			return;

#if LN_PLATFORM_WIN
		::FreeLibrary(static_cast<HMODULE>(mHandle));
#endif

		mHandle = nullptr;
	}

	void* DynamicLibrary::GetSymbol(const std::string& name) const
	{
		if (!mHandle)
			return nullptr;

#if LN_PLATFORM_WIN
		return reinterpret_cast<void*>(::GetProcAddress(static_cast<HMODULE>(mHandle), name.c_str()));
#else
		return nullptr;
#endif
	}
}
