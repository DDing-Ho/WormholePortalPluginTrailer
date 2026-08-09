// Copyright 2026 Team Beaver. All Rights Reserved.

#include "Passes/WPTemporaryFrontTranslucencyRestorePass.h"

#include "DataDrivenShaderPlatformInfo.h"
#include "GlobalRenderResources.h"
#include "GlobalShader.h"
#include "HAL/IConsoleManager.h"
#include "Passes/WPCompositePass.h"
#include "PixelShaderUtils.h"
#include "ProfilingDebugging/RealtimeGPUProfiler.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphEvent.h"
#include "RenderGraphResources.h"
#include "RenderGraphUtils.h"
#include "RHIStaticStates.h"
#include "SceneView.h"
#include "ScreenPass.h"
#include "ShaderParameterStruct.h"
#include "WormholePortalStats.h"

DECLARE_GPU_STAT_NAMED(
	WPTemporaryFrontTranslucencyRestore,
	TEXT("WP.TemporaryFrontTranslucencyRestore"));
DECLARE_CYCLE_STAT(
	TEXT("Temporary Front Translucency Restore Setup"),
	STAT_WP_TemporaryFrontTranslucencyRestoreSetup,
	STATGROUP_WormholePortal);

namespace
{
	TAutoConsoleVariable<int32> CVarWPTemporaryFrontTranslucencyRestoreEnabled(
		TEXT("wp.TemporaryFrontTranslucencyRestore.Enabled"),
		1,
		TEXT("TEMPORARY workaround for main-view foreground translucency affected by local SceneColor lens or linked cubemap branches.\n")
		TEXT("0: disabled, 1: enabled (default). Requires translucent material Allow Custom Depth Writes, ")
		TEXT("component Render CustomDepth, and matching stencil."),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<int32> CVarWPTemporaryFrontTranslucencyRestoreCustomStencil(
		TEXT("wp.TemporaryFrontTranslucencyRestore.CustomStencil"),
		240,
		TEXT("TEMPORARY foreground translucency CustomDepth stencil value (1..255, default 240)."),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CVarWPTemporaryFrontTranslucencyRestoreDepthBiasCm(
		TEXT("wp.TemporaryFrontTranslucencyRestore.DepthBiasCm"),
		2.0f,
		TEXT("TEMPORARY minimum camera-space separation in cm between tagged CustomDepth and the wormhole proxy."),
		ECVF_RenderThreadSafe);

	class FWPTemporaryFrontTranslucencyRestorePS final : public FGlobalShader
	{
	public:
		DECLARE_GLOBAL_SHADER(FWPTemporaryFrontTranslucencyRestorePS);
		SHADER_USE_PARAMETER_STRUCT(FWPTemporaryFrontTranslucencyRestorePS, FGlobalShader);

		BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
			SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)
			SHADER_PARAMETER_STRUCT(FScreenPassTextureViewportParameters, Input)
			SHADER_PARAMETER_STRUCT(FScreenPassTextureViewportParameters, Base)
			SHADER_PARAMETER_STRUCT(FScreenPassTextureViewportParameters, Output)
			SHADER_PARAMETER_RDG_TEXTURE(Texture2D, InputTexture)
			SHADER_PARAMETER_SAMPLER(SamplerState, InputSampler)
			SHADER_PARAMETER_RDG_TEXTURE(Texture2D, BaseSceneColorTexture)
			SHADER_PARAMETER_SAMPLER(SamplerState, BaseSceneColorSampler)
			SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D, SceneDepthTexture)
			SHADER_PARAMETER_SAMPLER(SamplerState, SceneDepthSampler)
			SHADER_PARAMETER_RDG_TEXTURE(Texture2D, CustomDepthTexture)
			SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D<uint2>, CustomStencilTexture)
			SHADER_PARAMETER_RDG_TEXTURE(Texture3D, RayLUTTexture)
			SHADER_PARAMETER_SAMPLER(SamplerState, RayLUTSampler)
			SHADER_PARAMETER(float, RayLUTZ)
			SHADER_PARAMETER(float, RayLUTEnabled)
			SHADER_PARAMETER(FVector3f, PortalCenterTranslated)
			SHADER_PARAMETER(float, PortalRadiusCm)
			SHADER_PARAMETER(float, ThroatLengthCm)
			SHADER_PARAMETER(float, ProxyRadiusCm)
			SHADER_PARAMETER(float, CompositeDepthBiasCm)
			SHADER_PARAMETER(float, TemporaryFrontDepthBiasCm)
			SHADER_PARAMETER(uint32, TemporaryCustomStencilValue)
			SHADER_PARAMETER(FVector3f, SelfX)
			SHADER_PARAMETER(FVector3f, SelfY)
			SHADER_PARAMETER(FVector3f, SelfZ)
			SHADER_PARAMETER(FVector3f, LinkedX)
			SHADER_PARAMETER(FVector3f, LinkedY)
			SHADER_PARAMETER(FVector3f, LinkedZ)
			RENDER_TARGET_BINDING_SLOTS()
		END_SHADER_PARAMETER_STRUCT()

		static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
		{
			return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
		}
	};

	IMPLEMENT_GLOBAL_SHADER(
		FWPTemporaryFrontTranslucencyRestorePS,
		"/Plugin/WormholePortal/WPTemporaryFrontTranslucencyRestore.usf",
		"MainPS",
		SF_Pixel);

	bool IsWPTemporaryFrontTranslucencyRestoreShaderAvailable(
		const FSceneView& View,
		FGlobalShaderMap*& OutGlobalShaderMap)
	{
		OutGlobalShaderMap = GetGlobalShaderMap(View.GetFeatureLevel());
		if (!OutGlobalShaderMap
			|| !OutGlobalShaderMap->HasShader(
				&FWPTemporaryFrontTranslucencyRestorePS::GetStaticType(), 0))
		{
			return false;
		}

		const TShaderMapRef<FWPTemporaryFrontTranslucencyRestorePS> PixelShader(
			OutGlobalShaderMap);
		return PixelShader.IsValid();
	}

	bool IsWPTemporarySupportedDepthTexture(const FRDGTextureRef Texture)
	{
		return Texture
			&& (Texture->Desc.Dimension == ETextureDimension::Texture2D
				|| Texture->Desc.Dimension == ETextureDimension::Texture2DArray)
			&& (!Texture->Desc.IsTextureArray() || Texture->Desc.ArraySize > 0)
			&& Texture->Desc.NumSamples == 1;
	}
}

FWPTemporaryFrontTranslucencyRestoreSettings
GetWPTemporaryFrontTranslucencyRestoreSettings_RenderThread()
{
	FWPTemporaryFrontTranslucencyRestoreSettings Settings;
	Settings.bEnabled =
		CVarWPTemporaryFrontTranslucencyRestoreEnabled.GetValueOnRenderThread() != 0;
	Settings.CustomStencilValue = static_cast<uint32>(FMath::Clamp(
		CVarWPTemporaryFrontTranslucencyRestoreCustomStencil.GetValueOnRenderThread(),
		1,
		255));
	Settings.FrontDepthBiasCm = FMath::Max(
		CVarWPTemporaryFrontTranslucencyRestoreDepthBiasCm.GetValueOnRenderThread(),
		0.0f);
	return Settings;
}

const TCHAR* GetWPTemporaryFrontTranslucencyRestoreFailureReasonName(
	const EWPTemporaryFrontTranslucencyRestoreFailureReason Reason)
{
	switch (Reason)
	{
	case EWPTemporaryFrontTranslucencyRestoreFailureReason::None: return TEXT("None");
	case EWPTemporaryFrontTranslucencyRestoreFailureReason::Disabled: return TEXT("Disabled");
	case EWPTemporaryFrontTranslucencyRestoreFailureReason::InvalidSceneColor:
		return TEXT("InvalidSceneColor");
	case EWPTemporaryFrontTranslucencyRestoreFailureReason::SceneDepthUnavailable:
		return TEXT("SceneDepthUnavailable");
	case EWPTemporaryFrontTranslucencyRestoreFailureReason::InvalidSceneDepthSlice:
		return TEXT("InvalidSceneDepthSlice");
	case EWPTemporaryFrontTranslucencyRestoreFailureReason::InvalidSceneDepthBinding:
		return TEXT("InvalidSceneDepthBinding");
	case EWPTemporaryFrontTranslucencyRestoreFailureReason::CustomDepthUnavailable:
		return TEXT("CustomDepthUnavailable");
	case EWPTemporaryFrontTranslucencyRestoreFailureReason::InvalidCustomDepthBinding:
		return TEXT("InvalidCustomDepthBinding");
	case EWPTemporaryFrontTranslucencyRestoreFailureReason::CustomStencilUnavailable:
		return TEXT("CustomStencilUnavailable");
	case EWPTemporaryFrontTranslucencyRestoreFailureReason::RayLUTUnavailable:
		return TEXT("RayLUTUnavailable");
	case EWPTemporaryFrontTranslucencyRestoreFailureReason::RayLUTRegistrationFailed:
		return TEXT("RayLUTRegistrationFailed");
	case EWPTemporaryFrontTranslucencyRestoreFailureReason::ShaderUnavailable:
		return TEXT("ShaderUnavailable");
	case EWPTemporaryFrontTranslucencyRestoreFailureReason::OutputUnavailable:
		return TEXT("OutputUnavailable");
	default: return TEXT("Unknown");
	}
}

bool ValidateWPTemporaryFrontTranslucencyRestorePass(
	const FSceneView& View,
	const FScreenPassTexture& SceneColor,
	const FScreenPassTexture& BaseSceneColor,
	const FRDGTextureRef SceneDepthTexture,
	const int32 SceneDepthArraySlice,
	const FRDGTextureRef CustomDepthTexture,
	const FRDGTextureSRVRef CustomStencilTexture,
	const FWPTemporaryFrontTranslucencyRestoreSettings& Settings,
	EWPTemporaryFrontTranslucencyRestoreFailureReason& OutFailureReason)
{
	OutFailureReason = EWPTemporaryFrontTranslucencyRestoreFailureReason::None;
	if (!Settings.bEnabled)
	{
		OutFailureReason = EWPTemporaryFrontTranslucencyRestoreFailureReason::Disabled;
		return false;
	}

	const bool bSceneColorCompatible = SceneColor.IsValid()
		&& SceneColor.Texture->Desc.Dimension == ETextureDimension::Texture2D
		&& SceneColor.Texture->Desc.NumSamples == 1
		&& BaseSceneColor.IsValid()
		&& BaseSceneColor.Texture->Desc.Dimension == ETextureDimension::Texture2D
		&& BaseSceneColor.Texture->Desc.NumSamples == 1;
	if (!bSceneColorCompatible)
	{
		OutFailureReason =
			EWPTemporaryFrontTranslucencyRestoreFailureReason::InvalidSceneColor;
		return false;
	}
	if (!SceneDepthTexture)
	{
		OutFailureReason =
			EWPTemporaryFrontTranslucencyRestoreFailureReason::SceneDepthUnavailable;
		return false;
	}
	if (!IsWPTemporarySupportedDepthTexture(SceneDepthTexture))
	{
		OutFailureReason =
			EWPTemporaryFrontTranslucencyRestoreFailureReason::InvalidSceneDepthBinding;
		return false;
	}
	if (SceneDepthTexture->Desc.Dimension == ETextureDimension::Texture2DArray
		&& (SceneDepthArraySlice < 0
			|| SceneDepthArraySlice >= SceneDepthTexture->Desc.ArraySize))
	{
		OutFailureReason =
			EWPTemporaryFrontTranslucencyRestoreFailureReason::InvalidSceneDepthSlice;
		return false;
	}
	if (!CustomDepthTexture || !HasBeenProduced(CustomDepthTexture))
	{
		OutFailureReason =
			EWPTemporaryFrontTranslucencyRestoreFailureReason::CustomDepthUnavailable;
		return false;
	}
	if (CustomDepthTexture->Desc.Dimension != ETextureDimension::Texture2D
		|| CustomDepthTexture->Desc.NumSamples != 1)
	{
		OutFailureReason =
			EWPTemporaryFrontTranslucencyRestoreFailureReason::InvalidCustomDepthBinding;
		return false;
	}
	if (!CustomStencilTexture)
	{
		OutFailureReason =
			EWPTemporaryFrontTranslucencyRestoreFailureReason::CustomStencilUnavailable;
		return false;
	}

	FGlobalShaderMap* GlobalShaderMap = nullptr;
	if (!IsWPTemporaryFrontTranslucencyRestoreShaderAvailable(View, GlobalShaderMap))
	{
		OutFailureReason =
			EWPTemporaryFrontTranslucencyRestoreFailureReason::ShaderUnavailable;
		return false;
	}
	return true;
}

FScreenPassTexture AddWPTemporaryFrontTranslucencyRestorePass(
	FRDGBuilder& GraphBuilder,
	const FSceneView& View,
	const FScreenPassTexture& SceneColor,
	const FScreenPassTexture& BaseSceneColor,
	const FScreenPassRenderTarget& OverrideOutput,
	const FRDGTextureRef SceneDepthTexture,
	const int32 SceneDepthArraySlice,
	const FRDGTextureRef CustomDepthTexture,
	const FRDGTextureSRVRef CustomStencilTexture,
	const FWPCompositePassParameters& CompositeParameters,
	const FWPTemporaryFrontTranslucencyRestoreSettings& Settings,
	const uint64 PairHandle,
	EWPTemporaryFrontTranslucencyRestoreFailureReason& OutFailureReason)
{
	SCOPE_CYCLE_COUNTER(STAT_WP_TemporaryFrontTranslucencyRestoreSetup);
	if (!ValidateWPTemporaryFrontTranslucencyRestorePass(
		View,
		SceneColor,
		BaseSceneColor,
		SceneDepthTexture,
		SceneDepthArraySlice,
		CustomDepthTexture,
		CustomStencilTexture,
		Settings,
		OutFailureReason))
	{
		return FScreenPassTexture();
	}

	const bool bSceneDepthArray =
		SceneDepthTexture->Desc.Dimension == ETextureDimension::Texture2DArray;
	FRDGTextureSRVRef SceneDepthSRV = bSceneDepthArray
		? GraphBuilder.CreateSRV(
			FRDGTextureSRVDesc::CreateForSlice(SceneDepthTexture, SceneDepthArraySlice))
		: GraphBuilder.CreateSRV(FRDGTextureSRVDesc::Create(SceneDepthTexture));
	if (!SceneDepthSRV)
	{
		OutFailureReason =
			EWPTemporaryFrontTranslucencyRestoreFailureReason::InvalidSceneDepthBinding;
		return FScreenPassTexture();
	}

	FRHITexture* RayLUTBindingTexture = CompositeParameters.bAnalyticNoTransition
		&& GBlackVolumeTexture
		? GBlackVolumeTexture->TextureRHI.GetReference()
		: CompositeParameters.RayLUTTexture;
	if (!RayLUTBindingTexture)
	{
		OutFailureReason =
			EWPTemporaryFrontTranslucencyRestoreFailureReason::RayLUTUnavailable;
		return FScreenPassTexture();
	}
	const FRHITextureDesc& LUTDesc = RayLUTBindingTexture->GetDesc();
	FRDGTextureRef RayLUTRDG = RegisterExternalTexture(
		GraphBuilder,
		RayLUTBindingTexture,
		TEXT("WP.TemporaryFrontTranslucencyRestore.RayLUT"));
	if (!RayLUTRDG)
	{
		OutFailureReason =
			EWPTemporaryFrontTranslucencyRestoreFailureReason::RayLUTRegistrationFailed;
		return FScreenPassTexture();
	}

	FGlobalShaderMap* GlobalShaderMap = nullptr;
	if (!IsWPTemporaryFrontTranslucencyRestoreShaderAvailable(View, GlobalShaderMap))
	{
		OutFailureReason =
			EWPTemporaryFrontTranslucencyRestoreFailureReason::ShaderUnavailable;
		return FScreenPassTexture();
	}
	const TShaderMapRef<FWPTemporaryFrontTranslucencyRestorePS> PixelShader(
		GlobalShaderMap);

	FScreenPassRenderTarget Output = OverrideOutput;
	if (!Output.IsValid())
	{
		Output = FScreenPassRenderTarget::CreateFromInput(
			GraphBuilder,
			SceneColor,
			View.GetOverwriteLoadAction(),
			TEXT("WP.TemporaryFrontTranslucencyRestore.Output"));
	}
	if (!Output.IsValid())
	{
		OutFailureReason =
			EWPTemporaryFrontTranslucencyRestoreFailureReason::OutputUnavailable;
		return FScreenPassTexture();
	}

	const FScreenPassTextureViewport InputViewport(SceneColor);
	const FScreenPassTextureViewport BaseViewport(BaseSceneColor);
	const FScreenPassTextureViewport OutputViewport(Output);
	FWPTemporaryFrontTranslucencyRestorePS::FParameters* PassParameters =
		GraphBuilder.AllocParameters<
			FWPTemporaryFrontTranslucencyRestorePS::FParameters>();
	PassParameters->View = View.ViewUniformBuffer;
	PassParameters->Input = GetScreenPassTextureViewportParameters(InputViewport);
	PassParameters->Base = GetScreenPassTextureViewportParameters(BaseViewport);
	PassParameters->Output = GetScreenPassTextureViewportParameters(OutputViewport);
	PassParameters->InputTexture = SceneColor.Texture;
	PassParameters->InputSampler =
		TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
	PassParameters->BaseSceneColorTexture = BaseSceneColor.Texture;
	PassParameters->BaseSceneColorSampler =
		TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
	PassParameters->SceneDepthTexture = SceneDepthSRV;
	PassParameters->SceneDepthSampler =
		TStaticSamplerState<SF_Point, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
	PassParameters->CustomDepthTexture = CustomDepthTexture;
	PassParameters->CustomStencilTexture = CustomStencilTexture;
	PassParameters->RayLUTTexture = RayLUTRDG;
	PassParameters->RayLUTSampler =
		TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
	PassParameters->RayLUTZ = CompositeParameters.RayLUTZ;
	PassParameters->RayLUTEnabled =
		CompositeParameters.bAnalyticNoTransition ? 0.0f : 1.0f;
	PassParameters->PortalCenterTranslated = CompositeParameters.PortalCenterTranslated;
	PassParameters->PortalRadiusCm = CompositeParameters.PortalRadiusCm;
	PassParameters->ThroatLengthCm = CompositeParameters.ThroatLengthCm;
	PassParameters->ProxyRadiusCm = CompositeParameters.ProxyRadiusCm;
	PassParameters->CompositeDepthBiasCm = CompositeParameters.DepthBiasCm;
	PassParameters->TemporaryFrontDepthBiasCm = Settings.FrontDepthBiasCm;
	PassParameters->TemporaryCustomStencilValue = Settings.CustomStencilValue;
	PassParameters->SelfX = CompositeParameters.SelfX;
	PassParameters->SelfY = CompositeParameters.SelfY;
	PassParameters->SelfZ = CompositeParameters.SelfZ;
	PassParameters->LinkedX = CompositeParameters.LinkedX;
	PassParameters->LinkedY = CompositeParameters.LinkedY;
	PassParameters->LinkedZ = CompositeParameters.LinkedZ;
	PassParameters->RenderTargets[0] = Output.GetRenderTargetBinding();

	ClearUnusedGraphResources(PixelShader, PassParameters);
	RDG_EVENT_SCOPE_STAT(
		GraphBuilder,
		WPTemporaryFrontTranslucencyRestore,
		"WP.TemporaryFrontTranslucencyRestore Pair=%llu View=%dx%d Stencil=%u FrontDepthBiasCm=%.3f LUT=%dx%dx%d/M%u",
		PairHandle,
		Output.ViewRect.Width(),
		Output.ViewRect.Height(),
		Settings.CustomStencilValue,
		Settings.FrontDepthBiasCm,
		LUTDesc.Extent.X,
		LUTDesc.Extent.Y,
		LUTDesc.Depth,
		LUTDesc.NumMips);
	FPixelShaderUtils::AddFullscreenPass(
		GraphBuilder,
		GlobalShaderMap,
		RDG_EVENT_NAME(
			"WP.TemporaryFrontTranslucencyRestore Pair=%llu Stencil=%u",
			PairHandle,
			Settings.CustomStencilValue),
		PixelShader,
		PassParameters,
		Output.ViewRect);

	OutFailureReason = EWPTemporaryFrontTranslucencyRestoreFailureReason::None;
	return MoveTemp(Output);
}
