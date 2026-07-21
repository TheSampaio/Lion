#include "Engine.h"
#include "RigidBody2D.h"

#include <box2d/box2d.h>

#include <Lion/Core/Log.h>
#include <Lion/Logic/ComponentRegistry.h>
#include <Lion/Logic/Entity.h>
#include <Lion/Logic/Scene.h>
#include <Lion/Logic/Serializer.h>
#include <Lion/Math/Transform.h>
#include <Lion/Physics/PhysicsWorld.h>

namespace Lion
{
	static b2BodyType ToBox2DBodyType(BodyType type)
	{
		switch (type)
		{
			case BodyType::Static:    return b2_staticBody;
			case BodyType::Kinematic: return b2_kinematicBody;
			case BodyType::Dynamic:   return b2_dynamicBody;
		}

		return b2_staticBody;
	}

	static const char8* BodyTypeToString(BodyType type)
	{
		switch (type)
		{
			case BodyType::Kinematic: return "Kinematic";
			case BodyType::Dynamic:   return "Dynamic";
			case BodyType::Static:    return "Static";
		}

		return "Static";
	}

	static BodyType BodyTypeFromString(const std::string& value)
	{
		if (value == "Kinematic") return BodyType::Kinematic;
		if (value == "Dynamic")   return BodyType::Dynamic;
		return BodyType::Static;
	}

	RigidBody2D::RigidBody2D(BodyType type, bool fixedRotation)
		: mType(type), mFixedRotation(fixedRotation)
	{
	}

	void RigidBody2D::OnAwake()
	{
		EnsureBody();
	}

	b2BodyId RigidBody2D::EnsureBody()
	{
		if (mHasBody)
			return mBodyId;

		const Reference<Scene> scene = GetOwner().GetScene();

		if (!scene)
		{
			Log::Console(LogLevel::Error, "[RigidBody2D] Owner is not part of a scene; body not created.");
			return mBodyId;
		}

		mWorld = scene->GetPhysicsWorld();

		if (!mWorld)
			return mBodyId;

		// Bodies live in world space, so resolve the owner's transform through its parent chain.
		const Vector position = GetOwner().GetWorldPosition();
		const float32 rotation = GetOwner().GetWorldRotation();

		b2BodyDef bodyDef = b2DefaultBodyDef();
		bodyDef.type = ToBox2DBodyType(mType);
		bodyDef.position = { position.x * PhysicsWorld::MetersPerPixel, position.y * PhysicsWorld::MetersPerPixel };
		bodyDef.rotation = b2MakeRot(glm::radians(rotation));  // Transform rotation is stored in degrees.
		bodyDef.fixedRotation = mFixedRotation;
		bodyDef.linearVelocity = { mPendingVelocity.x, mPendingVelocity.y };
		bodyDef.userData = &GetOwner();

		bodyDef.isEnabled = GetOwner().IsActive();  // A body born to a disabled entity is born disabled.

		mBodyId = b2CreateBody(mWorld->GetWorldId(), &bodyDef);
		mHasBody = true;

		mWorld->Register(this);
		return mBodyId;
	}

	void RigidBody2D::OnEnable()
	{
		if (mHasBody)
			b2Body_Enable(mBodyId);
	}

	void RigidBody2D::OnDisable()
	{
		if (mHasBody)
			b2Body_Disable(mBodyId);
	}

	void RigidBody2D::OnDestroy()
	{
		if (!mHasBody)
			return;

		if (mWorld)
			mWorld->Unregister(this);

		b2DestroyBody(mBodyId);
		mHasBody = false;
	}

	void RigidBody2D::SetLinearVelocity(const glm::vec2& pixelsPerSecond)
	{
		const glm::vec2 metersPerSecond = pixelsPerSecond * PhysicsWorld::MetersPerPixel;

		if (mHasBody)
			b2Body_SetLinearVelocity(mBodyId, { metersPerSecond.x, metersPerSecond.y });
		else
			mPendingVelocity = metersPerSecond;
	}

	glm::vec2 RigidBody2D::GetLinearVelocity() const
	{
		if (!mHasBody)
			return mPendingVelocity * PhysicsWorld::PixelsPerMeter;

		const b2Vec2 velocity = b2Body_GetLinearVelocity(mBodyId);
		return { velocity.x * PhysicsWorld::PixelsPerMeter, velocity.y * PhysicsWorld::PixelsPerMeter };
	}

	void RigidBody2D::SetPosition(const glm::vec2& pixels)
	{
		if (!mHasBody)
			return;

		const b2Vec2 meters = { pixels.x * PhysicsWorld::MetersPerPixel, pixels.y * PhysicsWorld::MetersPerPixel };
		b2Body_SetTransform(mBodyId, meters, b2Body_GetRotation(mBodyId));
	}

	void RigidBody2D::SyncTransform()
	{
		if (!mHasBody)
			return;

		const b2Vec2 position = b2Body_GetPosition(mBodyId);
		const float32 angle = glm::degrees(b2Rot_GetAngle(b2Body_GetRotation(mBodyId)));

		// The simulation works in world space; writing it back rebases the entity's local transform.
		Entity& owner = GetOwner();

		owner.SetWorldPosition(Vector2(
			position.x * PhysicsWorld::PixelsPerMeter,
			position.y * PhysicsWorld::PixelsPerMeter));

		owner.SetWorldRotation(angle);
	}

	void RigidBody2D::Serialize(Serializer& serializer) const
	{
		serializer.Write("bodyType", std::string(BodyTypeToString(mType)));
		serializer.Write("fixedRotation", mFixedRotation);
	}

	void RigidBody2D::Deserialize(const Serializer& serializer)
	{
		mType = BodyTypeFromString(serializer.ReadString("bodyType", "Static"));
		mFixedRotation = serializer.ReadBool("fixedRotation");
	}

	LION_REGISTER_COMPONENT(RigidBody2D)
}
