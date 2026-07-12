#include "Engine.h"
#include "Component.h"

#include <Lion/Logic/Entity.h>
#include <Lion/Logic/Reflector.h>
#include <Lion/Logic/Serializer.h>
#include <Lion/Math/Vector.h>

namespace Lion
{
	namespace
	{
		// The two halves of "a described field is a saved field": one Reflector hands each field to the
		// archive, the other takes it back out. The component never sees either of them — it names its
		// fields and the direction is somebody else's problem, which is the whole point of describing them.
		class WritingReflector : public Reflector
		{
		public:
			explicit WritingReflector(Serializer& serializer) : mSerializer(serializer) {}

			void Field(const char8* name, float32& value) override     { mSerializer.Write(name, value); }
			void Field(const char8* name, int32& value) override       { mSerializer.Write(name, value); }
			void Field(const char8* name, bool& value) override        { mSerializer.Write(name, value); }
			void Field(const char8* name, std::string& value) override { mSerializer.Write(name, value); }
			void FieldAsset(const char8* name, std::string& path) override { mSerializer.Write(name, path); }

			// A vector is three numbers, and it is stored as three numbers: the archive is a flat map of
			// keys, so the shape of a field lives in the suffix rather than in a nested node.
			void Field(const char8* name, Vector& value) override
			{
				mSerializer.Write(std::string(name) + ".x", value.x);
				mSerializer.Write(std::string(name) + ".y", value.y);
				mSerializer.Write(std::string(name) + ".z", value.z);
			}

		private:
			Serializer& mSerializer;
		};

		class ReadingReflector : public Reflector
		{
		public:
			explicit ReadingReflector(const Serializer& serializer) : mSerializer(serializer) {}

			// The field's current value is its own fallback: a key the file never carried leaves whatever
			// the component was constructed with, which is what a default is for.
			void Field(const char8* name, float32& value) override     { value = mSerializer.ReadFloat(name, value); }
			void Field(const char8* name, int32& value) override       { value = mSerializer.ReadInt(name, value); }
			void Field(const char8* name, bool& value) override        { value = mSerializer.ReadBool(name, value); }
			void Field(const char8* name, std::string& value) override { value = mSerializer.ReadString(name, value); }
			void FieldAsset(const char8* name, std::string& path) override { path = mSerializer.ReadString(name, path); }

			void Field(const char8* name, Vector& value) override
			{
				value.x = mSerializer.ReadFloat(std::string(name) + ".x", value.x);
				value.y = mSerializer.ReadFloat(std::string(name) + ".y", value.y);
				value.z = mSerializer.ReadFloat(std::string(name) + ".z", value.z);
			}

		private:
			const Serializer& mSerializer;
		};
	}

	Reference<Transform> Component::GetTransform() const
	{
		return mOwner->GetTransform();
	}

	void Component::Serialize(Serializer& serializer) const
	{
		// Describing a field hands out a reference to it, so a const walk is not one Reflect can do. The
		// cast is safe and stays here: writing a field's value out does not change it, and this is the one
		// place that knows that.
		WritingReflector reflector(serializer);
		const_cast<Component*>(this)->Reflect(reflector);
	}

	void Component::Deserialize(const Serializer& serializer)
	{
		ReadingReflector reflector(serializer);
		Reflect(reflector);
	}
}
