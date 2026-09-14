#pragma once

#include <Lion/Logic/Component.h>
#include <Lion/Math/Vector.h>

namespace Lion
{
	// Camera-owned full-screen finishing effects. Built-ins are combined in one pass. A custom .lnshader
	// replaces that pass and receives iPosition at location 0, iTexCoord at location 1, uScreenTexture,
	// uResolution, and the reflected built-in effect uniforms.
	class PostProcessingComponent final : public Component
	{
	public:
		LION_REQUIRES("Camera2D");
		LION_API PostProcessingComponent() = default;
		LION_API ~PostProcessingComponent();

		bool HasBloom() const { return mBloom; }
		float32 GetBloomStrength() const { return mBloomStrength; }
		float32 GetBloomThreshold() const { return mBloomThreshold; }
		bool HasColorCorrection() const { return mColorCorrection; }
		float32 GetBrightness() const { return mBrightness; }
		float32 GetContrast() const { return mContrast; }
		float32 GetSaturation() const { return mSaturation; }
		float32 GetGamma() const { return mGamma; }
		const Vector& GetTint() const { return mTint; }
		bool HasVignette() const { return mVignette; }
		float32 GetVignetteStrength() const { return mVignetteStrength; }
		bool HasChromaticAberration() const { return mChromaticAberration; }
		float32 GetChromaticAberration() const { return mChromaticAberrationAmount; }
		bool HasMotionBlur() const { return mMotionBlur; }
		float32 GetMotionBlurStrength() const { return mMotionBlurStrength; }
		int32 GetColorVisionMode() const { return mColorVisionMode; }
		float32 GetFade() const { return mFade; }
		const std::string& GetCustomShaderPath() const { return mCustomShaderPath; }

		void SetBloom(bool enabled) { mBloom = enabled; }
		void SetBloomStrength(float32 strength) { mBloomStrength = std::max(strength, 0.0f); }
		void SetVignette(bool enabled) { mVignette = enabled; }
		void SetVignetteStrength(float32 strength) { mVignetteStrength = std::clamp(strength, 0.0f, 1.0f); }
		void SetChromaticAberration(bool enabled) { mChromaticAberration = enabled; }
		void SetMotionBlur(bool enabled) { mMotionBlur = enabled; }
		void SetMotionBlurStrength(float32 strength) { mMotionBlurStrength = std::clamp(strength, 0.0f, 1.0f); }
		void SetColorVisionMode(int32 mode) { mColorVisionMode = std::clamp(mode, 0, 3); }
		void SetFade(float32 fade) { mFade = std::clamp(fade, 0.0f, 1.0f); }

		LION_API void Reflect(Reflector& reflector) override;

	private:
		bool mBloom = true;
		float32 mBloomStrength = 0.18f;
		float32 mBloomThreshold = 0.72f;
		bool mColorCorrection = true;
		float32 mBrightness = 0.0f;
		float32 mContrast = 1.04f;
		float32 mSaturation = 1.08f;
		float32 mGamma = 1.0f;
		Vector mTint{ 1.0f, 1.0f, 1.0f };
		bool mVignette = true;
		float32 mVignetteStrength = 0.14f;
		bool mChromaticAberration = false;
		float32 mChromaticAberrationAmount = 0.001f;
		bool mMotionBlur = false;
		float32 mMotionBlurStrength = 0.075f;
		int32 mColorVisionMode = 0;
		float32 mFade = 0.0f;
		std::string mCustomShaderPath;
	};
}
