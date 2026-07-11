#pragma once

namespace Lion
{
	// Abstract key/value archive used to persist and restore a Component's fields.
	//
	// The engine backs this with JSON, but components — including user-defined ones that live in the
	// game module — only ever see this interface. Keeping the concrete serialization library on the
	// engine side of the boundary means no third-party type ever crosses into the game DLL.
	//
	// Serialize() writes with the Write overloads; Deserialize() reads with the typed readers, each
	// returning the supplied fallback when the key is absent.
	class Serializer
	{
	public:
		virtual ~Serializer() = default;

		virtual void Write(const std::string& key, float32 value) = 0;
		virtual void Write(const std::string& key, int32 value) = 0;
		virtual void Write(const std::string& key, bool value) = 0;
		virtual void Write(const std::string& key, const std::string& value) = 0;

		virtual float32 ReadFloat(const std::string& key, float32 fallback = 0.0f) const = 0;
		virtual int32 ReadInt(const std::string& key, int32 fallback = 0) const = 0;
		virtual bool ReadBool(const std::string& key, bool fallback = false) const = 0;
		virtual std::string ReadString(const std::string& key, const std::string& fallback = std::string()) const = 0;
	};
}
