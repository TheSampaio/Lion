#include "Engine.h"
#include "CSharpScript.h"

#include <Lion/Logic/Entity.h>
#include <Lion/Logic/Scene.h>
#include <Lion/Scripting/CSharpRuntime.h>

namespace Lion
{
	CSharpScript::CSharpScript(const std::string& typeName) : mScriptTypeName(typeName) {}
	CSharpScript::~CSharpScript() { OnDestroy(); }

	void CSharpScript::OnAwake()
	{
		if (mInstance != 0)
			return;

		mInstance = CSharpRuntime::CreateInstance(GetOwner(), mScriptTypeName, mCallbacks);
		Dispatch(0);
	}

	void CSharpScript::OnEnable() { Dispatch(1); }
	void CSharpScript::OnDisable() { Dispatch(2); }
	void CSharpScript::OnUpdateBegin() { Dispatch(3); }
	void CSharpScript::OnUpdate() { Dispatch(4); }
	void CSharpScript::OnUpdateEnd() { Dispatch(5); }

	void CSharpScript::Dispatch(int32 callback)
	{
		if ((mCallbacks & (1u << callback)) == 0)
			return;

		const Reference<Scene> scene = GetOwner().GetScene();
		CSharpRuntime::Invoke(mInstance, callback, scene ? scene->GetDeltaTime() : 0.0f);
	}

	void CSharpScript::OnDestroy()
	{
		if (mInstance != 0)
			CSharpRuntime::DestroyInstance(mInstance);

		mInstance = 0;
		mCallbacks = 0;
	}
}
