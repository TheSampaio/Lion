#pragma once

namespace Lion
{
	struct Vector;

	// How a component describes its own fields — once.
	//
	// The engine reads that description twice: the scene file saves the fields from it, and the editor's
	// Inspector draws them from it. Two lists of the same fields drift the first time someone adds one to
	// only the list they were looking at; one list cannot.
	//
	// It is abstract for the same reason the Serializer is: whatever implements it — the JSON archive, the
	// Inspector — never crosses into the component's translation unit, and so never crosses the DLL
	// boundary into the game module.
	//
	// A component overrides Component::Reflect and names its fields:
	//
	//     void Movement::Reflect(Reflector& reflector)
	//     {
	//         reflector.Field("Speed", mSpeed);
	//         reflector.Field("Looping", mLooping);
	//     }
	class Reflector
	{
	public:
		virtual ~Reflector() = default;

		virtual void Field(const char8* name, float32& value) = 0;
		virtual void Field(const char8* name, int32& value) = 0;
		virtual void Field(const char8* name, bool& value) = 0;
		virtual void Field(const char8* name, std::string& value) = 0;
		virtual void Field(const char8* name, Vector& value) = 0;

		// A field that names a file rather than holding a value. It is a string either way, but the editor
		// gives it a browse button and takes a drop from the Content Browser, which is the difference
		// between choosing a sprite and spelling one.
		virtual void FieldAsset(const char8* name, std::string& path) = 0;
	};
}
