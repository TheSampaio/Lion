#pragma once

#include <Lion/Logic/Component.h>
#include <Lion/Scripting/ScriptField.h>

namespace Lion
{
	// Named managed lifecycle adapter. Authored fields stay native while gameplay is dormant.
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
		LION_API void Reflect(Reflector& reflector) override;
		LION_API void Deserialize(const Serializer& serializer) override;

	private:
		std::string mScriptTypeName;
		uint64 mInstance = 0;
		uint32 mCallbacks = 0;
		std::vector<ScriptingDetail::ScriptField> mFields;
		bool mReflecting = false;

		void Dispatch(int32 callback);
	};
}
