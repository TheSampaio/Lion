#include "Engine.h"
#include "ParticleComponent.h"

#include <Lion/Core/Asset.h>
#include <Lion/Core/Clock.h>
#include <Lion/Logic/ComponentRegistry.h>
#include <Lion/Logic/Entity.h>
#include <Lion/Logic/Reflector.h>
#include <Lion/Render/Sprite.h>
#include <Lion/Render/Texture.h>

namespace Lion
{
	ParticleComponent::~ParticleComponent() = default;

	void ParticleComponent::OnAwake()
	{
		mEmitterPosition = GetOwner().GetWorldPosition();
		mParticles.reserve(static_cast<size_t>(std::max(mMaxParticles, 0)));
		LoadTexture();
	}

	void ParticleComponent::Emit(int32 count)
	{
		LoadTexture();
		const int32 available = std::max(mMaxParticles - static_cast<int32>(mParticles.size()), 0);
		count = std::clamp(count, 0, available);

		for (int32 index = 0; index < count; ++index)
			Spawn();
	}

	void ParticleComponent::EmitAt(const Vector2& worldPosition, int32 count)
	{
		mEmitterPosition = worldPosition;
		Emit(count);
	}

	void ParticleComponent::OnUpdate()
	{
		LoadTexture();
		const float32 deltaTime = Clock::GetDeltaTime();

		if (mEmissionRate > 0.0f)
		{
			mEmissionAccumulator += deltaTime * mEmissionRate;
			const int32 count = static_cast<int32>(mEmissionAccumulator);
			mEmissionAccumulator -= static_cast<float32>(count);
			mEmitterPosition = GetOwner().GetWorldPosition();
			Emit(count);
		}

		for (Particle& particle : mParticles)
		{
			particle.age += deltaTime;
			particle.position.x += particle.velocity.x * deltaTime;
			particle.position.y += particle.velocity.y * deltaTime;
			particle.velocity = particle.velocity * std::max(0.0f, 1.0f - deltaTime * 2.5f);
			particle.rotation += 180.0f * deltaTime;
		}

		mParticles.erase(std::remove_if(mParticles.begin(), mParticles.end(),
			[](const Particle& particle) { return particle.age >= particle.lifetime; }), mParticles.end());
	}

	void ParticleComponent::OnRender()
	{
		for (Particle& particle : mParticles)
		{
			if (!particle.sprite)
				continue;

			const float32 progress = std::clamp(particle.age / particle.lifetime, 0.0f, 1.0f);
			const float32 size = mStartSize + (mEndSize - mStartSize) * progress;
			const Vector color(
				mStartColor.x + (mEndColor.x - mStartColor.x) * progress,
				mStartColor.y + (mEndColor.y - mStartColor.y) * progress,
				mStartColor.z + (mEndColor.z - mStartColor.z) * progress);
			const Size textureSize = particle.sprite->GetSize();

			particle.sprite->SetOrder(mOrder);
			particle.sprite->SetColor(color);
			particle.sprite->Draw(
				particle.position,
				Vector(0.0f, 0.0f, particle.rotation),
				Vector(size / textureSize.width, size / textureSize.height, 1.0f),
				GetOwner().GetId());
		}
	}

	void ParticleComponent::Reflect(Reflector& reflector)
	{
		reflector.FieldAsset("Texture", mTexturePath);
		reflector.Field("Max Particles", mMaxParticles);
		reflector.Field("Emission Rate", mEmissionRate);
		reflector.Field("Lifetime", mLifetime);
		reflector.Field("Speed", mSpeed);
		reflector.Field("Direction", mDirection);
		reflector.Field("Spread", mSpread);
		reflector.Field("Start Size", mStartSize);
		reflector.Field("End Size", mEndSize);
		reflector.Field("Start Color", mStartColor);
		reflector.Field("End Color", mEndColor);
		reflector.Field("Order", mOrder);
	}

	void ParticleComponent::LoadTexture()
	{
		if (mTexturePath == mLoadedTexturePath)
			return;

		mLoadedTexturePath = mTexturePath;
		mTexture = mTexturePath.empty() ? nullptr : Asset::LoadTexture(mTexturePath, mTexturePath);
		mParticles.clear();
	}

	void ParticleComponent::Spawn()
	{
		if (!mTexture)
			return;

		const float32 angle = glm::radians(mDirection + (NextRandom() - 0.5f) * mSpread);
		const float32 speed = mSpeed * (0.7f + NextRandom() * 0.6f);
		Particle particle;
		particle.position = mEmitterPosition;
		particle.velocity = Vector2(std::cos(angle) * speed, std::sin(angle) * speed);
		particle.lifetime = std::max(mLifetime * (0.75f + NextRandom() * 0.5f), 0.01f);
		particle.rotation = NextRandom() * 360.0f;
		particle.sprite = MakeScope<Sprite>(mTexture);
		mParticles.push_back(std::move(particle));
	}

	float32 ParticleComponent::NextRandom()
	{
		mRandomState = mRandomState * 1664525u + 1013904223u;
		return static_cast<float32>(mRandomState & 0x00FFFFFFu) / static_cast<float32>(0x01000000u);
	}

	LION_REGISTER_COMPONENT(ParticleComponent)
}
