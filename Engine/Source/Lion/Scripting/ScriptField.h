#pragma once

namespace Lion::ScriptingDetail
{
	// Native authoring cache. Its layout is not the interop wire format or a saved scene ABI.
	enum class FieldKind : int32 { Float, Int, Bool, String, Vector2 };

	struct ScriptField
	{
		std::string name;
		FieldKind kind = FieldKind::Float;
		float32 number = 0.0f;
		int32 integer = 0;
		std::string text;
		float32 x = 0.0f;
		float32 y = 0.0f;
	};
}
