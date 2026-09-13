#pragma once

#include <Lion/Logic/Component.h>
#include <Lion/Math/Vector.h>
#include <Lion/Math/Vector2.h>

namespace Lion
{
	class Sprite;
	class Texture;

	// A compact CPU particle emitter for 2D effects. Storage is reserved up front and sprites share one
	// cached texture, keeping bursts predictable while still exposing a simple authored component.
	class ParticleComponent final : public Component
	{
	public:
		LION_API ParticleComponent() = default;
		LION_API ~ParticleComponent();

		LION_API void Emit(int32 count = 1);
		LION_API void EmitAt(const Vector2& worldPosition, int32 count = 1);
		LION_API void SetEmitterPosition(const Vector2& worldPosition) { mEmitterPosition = worldPosition; }

		void OnAwake() override;
		void OnUpdate() override;
		void OnRender() override;
		void Reflect(Reflector& reflector) override;

	private:
		struct Particle
		{
			Vector2 position;
			Vector2 velocity;
			float32 age = 0.0f;
			float32 lifetime = 0.35f;
			float32 rotation = 0.0f;
			Scope<Sprite> sprite;
		};

		std::string mTexturePath;
		int32 mMaxParticles = 128;
		float32 mEmissionRate = 0.0f;
		float32 mLifetime = 0.35f;
		float32 mSpeed = 150.0f;
		float32 mSpread = 360.0f;
		float32 mDirection = 90.0f;
		float32 mStartSize = 18.0f;
		float32 mEndSize = 2.0f;
		Vector mStartColor{ 1.0f, 1.0f, 1.0f };
		Vector mEndColor{ 0.25f, 0.75f, 1.0f };
		int32 mOrder = 60;

		Reference<Texture> mTexture;
		std::string mLoadedTexturePath;
		std::vector<Particle> mParticles;
		Vector2 mEmitterPosition;
		float32 mEmissionAccumulator = 0.0f;
		uint32 mRandomState = 0xA341316Cu;

		void LoadTexture();
		void Spawn();
		float32 NextRandom();
	};
}
