#include "Engine.h"
#include "RigidBody2D.h"

#include <box2d/box2d.h>

#include <Lion/Core/Log.h>
#include <Lion/Logic/Entity.h>
#include <Lion/Logic/Scene.h>
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

		const Vector position = GetTransform()->GetPosition();
		const Vector rotation = GetTransform()->GetRotation();

		b2BodyDef bodyDef = b2DefaultBodyDef();
		bodyDef.type = ToBox2DBodyType(mType);
		bodyDef.position = { position.x * PhysicsWorld::MetersPerPixel, position.y * PhysicsWorld::MetersPerPixel };
		bodyDef.rotation = b2MakeRot(glm::radians(rotation.z));  // Transform rotation is stored in degrees.
		bodyDef.fixedRotation = mFixedRotation;
		bodyDef.linearVelocity = { mPendingVelocity.x, mPendingVelocity.y };
		bodyDef.userData = &GetOwner();

		mBodyId = b2CreateBody(mWorld->GetWorldId(), &bodyDef);
		mHasBody = true;

		mWorld->Register(this);
		return mBodyId;
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

		const Reference<Transform> transform = GetTransform();
		const Vector current = transform->GetPosition();

		// Preserve the sprite depth (z) that physics does not manage; rotation is stored in degrees.
		transform->SetPosition(Vector(position.x * PhysicsWorld::PixelsPerMeter, position.y * PhysicsWorld::PixelsPerMeter, current.z));
		transform->SetRotation(Vector(0.0f, 0.0f, angle));
	}
}
