#pragma once

#include <Lion/Logic/Component.h>

namespace Lion
{
	// Opt-in native lifecycle adapter. Authoring registration and field persistence are a later
	// increment; attaching this explicitly does not alter existing native component registration.
	class CSharpScript final : public Component
	{
	public:
		LION_API explicit CSharpScript(const std::string& typeName);
		LION_API ~CSharpScript();

		LION_API void OnAwake() override;
		LION_API void OnEnable() override;
		LION_API void OnDisable() override;
		LION_API void OnUpdateBegin() override;
		LION_API void OnUpdate() override;
		LION_API void OnUpdateEnd() override;
		LION_API void OnDestroy() override;

	private:
		std::string mScriptTypeName;
		uint64 mInstance = 0;
		uint32 mCallbacks = 0;

		void Dispatch(int32 callback);
	};
}
