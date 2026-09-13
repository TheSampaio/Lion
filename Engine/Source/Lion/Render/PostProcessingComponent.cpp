#include "Engine.h"
#include "PostProcessingComponent.h"

#include <Lion/Logic/ComponentRegistry.h>
#include <Lion/Logic/Reflector.h>

namespace Lion
{
	PostProcessingComponent::~PostProcessingComponent() = default;

	void PostProcessingComponent::Reflect(Reflector& reflector)
	{
		reflector.Field("Bloom", mBloom);
		reflector.Field("Bloom Strength", mBloomStrength);
		reflector.Field("Bloom Threshold", mBloomThreshold);
		reflector.Field("Color Correction", mColorCorrection);
		reflector.Field("Brightness", mBrightness);
		reflector.Field("Contrast", mContrast);
		reflector.Field("Saturation", mSaturation);
		reflector.Field("Gamma", mGamma);
		reflector.Field("Tint", mTint);
		reflector.Field("Vignette", mVignette);
		reflector.Field("Vignette Strength", mVignetteStrength);
		reflector.Field("Chromatic Aberration", mChromaticAberration);
		reflector.Field("Chromatic Aberration Amount", mChromaticAberrationAmount);
		reflector.FieldAsset("Custom Shader", mCustomShaderPath);
	}

	LION_REGISTER_COMPONENT(PostProcessingComponent)
}
