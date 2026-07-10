#include "Engine.h"
#include "ScriptComponent.h"

#include <Lion/Core/Log.h>
#include <Lion/Logic/Entity.h>
#include <Lion/Logic/Script.h>
#include <Lion/Logic/ScriptRegistry.h>

namespace Lion
{
	ScriptComponent::ScriptComponent(const std::string& scriptName)
		: mScriptName(scriptName)
	{
	}

	ScriptComponent::~ScriptComponent() = default;

	void ScriptComponent::OnAwake()
	{
		if (mScriptName.empty())
			return;

		mInstance = ScriptRegistry::Create(mScriptName);

		if (!mInstance)
		{
			Log::Console(LogLevel::Error, LION_FORMAT_TEXT("[ScriptComponent] Unknown script: '{}'.", mScriptName));
			return;
		}

		mInstance->mOwner = &GetOwner();
		mInstance->OnAwake();
	}

	void ScriptComponent::OnUpdate()
	{
		if (mInstance)
			mInstance->OnUpdate();
	}

	void ScriptComponent::OnDestroy()
	{
		if (!mInstance)
			return;

		mInstance->OnDestroy();
		mInstance.reset();
	}

	void ScriptComponent::SetScriptName(const std::string& name)
	{
		if (name == mScriptName)
			return;

		// Drop any running instance; a new one is created the next time the entity wakes.
		OnDestroy();
		mScriptName = name;
	}
}
