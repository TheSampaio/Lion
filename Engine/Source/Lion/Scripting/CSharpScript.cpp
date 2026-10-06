#include "Engine.h"
#include "CSharpScript.h"

#include <Lion/Logic/Entity.h>
#include <Lion/Logic/Scene.h>
#include <Lion/Logic/Reflector.h>
#include <Lion/Core/Log.h>
#include <Lion/Math/Vector.h>
#include <Lion/Scripting/CSharpRuntime.h>

namespace Lion
{
	CSharpScript::CSharpScript(const std::string& typeName) : mScriptTypeName(typeName)
	{
		CSharpRuntime::GetFields(typeName, mFields);
	}
	CSharpScript::~CSharpScript() { OnDestroy(); }

	void CSharpScript::OnAwake()
	{
		if (mInstance != 0 || !CSharpRuntime::IsGameplayActive())
			return;

		mInstance = CSharpRuntime::CreateInstance(GetOwner(), mScriptTypeName, mCallbacks);
		if (!CSharpRuntime::WriteFields(mInstance, mFields))
		{
			OnDestroy();
			return;
		}
		Dispatch(0);
	}

	void CSharpScript::OnEnable() { Dispatch(1); }
	void CSharpScript::OnDisable() { Dispatch(2); }
	void CSharpScript::OnUpdateBegin() { Dispatch(3); }
	void CSharpScript::OnUpdate() { Dispatch(4); }
	void CSharpScript::OnUpdateEnd() { Dispatch(5); }
	void CSharpScript::OnCollision(Entity& other) { if ((mCallbacks & (1u << 6)) != 0) CSharpRuntime::Collide(mInstance, other); }
	void CSharpScript::OnRender() { Dispatch(8); }
	bool CSharpScript::UpdatesWhenPaused() const { return CSharpRuntime::UpdatesWhenPaused(mInstance); }

	void CSharpScript::Dispatch(int32 callback)
	{
		if ((mCallbacks & (1u << callback)) == 0)
			return;

		const Reference<Scene> scene = GetOwner().GetScene();
		CSharpRuntime::Invoke(mInstance, callback, scene ? scene->GetDeltaTime() : 0.0f);
	}

	void CSharpScript::Reflect(Reflector& reflector)
	{
		// Inspector edits and undo snapshots can reflect this same component recursively. Keep its
		// native field storage stable until the outer walk finishes.
		struct ReflectionGuard
		{
			bool& flag;
			bool previous;
			~ReflectionGuard() { flag = previous; }
		};
		ReflectionGuard guard { mReflecting, mReflecting };
		if (!mReflecting && mInstance != 0 && !mFields.empty())
			CSharpRuntime::ReadFields(mInstance, mFields);
		mReflecting = true;

		for (auto& field : mFields)
		{
			using ScriptingDetail::FieldKind;
			switch (field.kind)
			{
				case FieldKind::Float: reflector.Field(field.name.c_str(), field.number); break;
				case FieldKind::Int: reflector.Field(field.name.c_str(), field.integer); break;
				case FieldKind::String: reflector.Field(field.name.c_str(), field.text); break;
				case FieldKind::Bool:
				{
					bool value = field.integer != 0;
					reflector.Field(field.name.c_str(), value);
					field.integer = value ? 1 : 0;
					break;
				}
				case FieldKind::Vector2:
				{
					reflector.FieldVector2(field.name.c_str(), field.x, field.y);
					break;
				}
			}
		}

		if (mInstance != 0 && !mFields.empty())
			CSharpRuntime::WriteFields(mInstance, mFields);
	}

	void CSharpScript::Deserialize(const Serializer& serializer)
	{
		const auto defaults = mFields;
		try { Component::Deserialize(serializer); }
		catch (const std::exception& exception)
		{
			mFields = defaults;
			Log::Console(LogLevel::Error, "[C#] Incompatible saved fields for '" + mScriptTypeName
				+ "'; defaults restored: " + exception.what());
		}
	}

	void CSharpScript::OnDestroy()
	{
		if (mInstance != 0)
			CSharpRuntime::DestroyInstance(mInstance);

		mInstance = 0;
		mCallbacks = 0;
	}
}
