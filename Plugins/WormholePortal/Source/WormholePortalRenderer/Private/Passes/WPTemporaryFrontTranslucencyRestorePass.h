// Copyright 2026 Team Beaver. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RenderGraphFwd.h"

class FRDGBuilder;
class FSceneView;
struct FScreenPassRenderTarget;
struct FScreenPassTexture;
struct FWPCompositePassParameters;

/**
 * TEMPORARY workaround controls for foreground translucency affected by any
 * wormhole optical branch, including local SceneColor lens and linked cubemap.
 */
struct FWPTemporaryFrontTranslucencyRestoreSettings
{
	bool bEnabled = false;
	uint32 CustomStencilValue = 240;
	float FrontDepthBiasCm = 2.0f;
};

/** TEMPORARY pass diagnostics. These names intentionally remain explicit in logs. */
enum class EWPTemporaryFrontTranslucencyRestoreFailureReason : uint8
{
	None = 0,
	Disabled,
	InvalidSceneColor,
	SceneDepthUnavailable,
	InvalidSceneDepthSlice,
	InvalidSceneDepthBinding,
	CustomDepthUnavailable,
	InvalidCustomDepthBinding,
	CustomStencilUnavailable,
	RayLUTUnavailable,
	RayLUTRegistrationFailed,
	ShaderUnavailable,
	OutputUnavailable
};

FWPTemporaryFrontTranslucencyRestoreSettings
GetWPTemporaryFrontTranslucencyRestoreSettings_RenderThread();

const TCHAR* GetWPTemporaryFrontTranslucencyRestoreFailureReasonName(
	EWPTemporaryFrontTranslucencyRestoreFailureReason Reason);

/**
 * Side-effect-free TEMPORARY pass preflight. A false result means the normal
 * composite should keep its original output path and this workaround is skipped.
 */
bool ValidateWPTemporaryFrontTranslucencyRestorePass(
	const FSceneView& View,
	const FScreenPassTexture& SceneColor,
	const FScreenPassTexture& BaseSceneColor,
	FRDGTextureRef SceneDepthTexture,
	int32 SceneDepthArraySlice,
	FRDGTextureRef CustomDepthTexture,
	FRDGTextureSRVRef CustomStencilTexture,
	const FWPTemporaryFrontTranslucencyRestoreSettings& Settings,
	EWPTemporaryFrontTranslucencyRestoreFailureReason& OutFailureReason);

/**
 * TEMPORARY post-composite restore. Only CustomStencilValue pixels whose custom
 * depth is in front of this endpoint's proxy are restored from immutable BaseSceneColor.
 */
FScreenPassTexture AddWPTemporaryFrontTranslucencyRestorePass(
	FRDGBuilder& GraphBuilder,
	const FSceneView& View,
	const FScreenPassTexture& SceneColor,
	const FScreenPassTexture& BaseSceneColor,
	const FScreenPassRenderTarget& OverrideOutput,
	FRDGTextureRef SceneDepthTexture,
	int32 SceneDepthArraySlice,
	FRDGTextureRef CustomDepthTexture,
	FRDGTextureSRVRef CustomStencilTexture,
	const FWPCompositePassParameters& CompositeParameters,
	const FWPTemporaryFrontTranslucencyRestoreSettings& Settings,
	uint64 PairHandle,
	EWPTemporaryFrontTranslucencyRestoreFailureReason& OutFailureReason);
