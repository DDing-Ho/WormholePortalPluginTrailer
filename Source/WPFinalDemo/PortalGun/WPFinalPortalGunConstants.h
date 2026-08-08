// Copyright 2026 Team Beaver. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/** 포탈건에서 바뀌지 않는 최종 튜닝값을 한곳에 모은다. */
namespace WPFinalPortalGunConstants
{
	namespace Asset
	{
		/** 포탈건 스켈레탈 메시 경로. */
		inline constexpr const TCHAR* GunMesh = TEXT("/Game/WormholePortal/Meshes/PortalGun/SKM_PortalGun.SKM_PortalGun");

		/** 포탈건 자체 애니메이션 블루프린트 경로. */
		inline constexpr const TCHAR* GunAnimationBlueprint = TEXT("/Game/WormholePortal/Meshes/PortalGun/Animation/ABP_PortalGun");

		/** 발사 몽타주 경로. */
		inline constexpr const TCHAR* FireMontage = TEXT("/Game/WormholePortal/Meshes/PortalGun/Animation/AM_PortalGun_Fire.AM_PortalGun_Fire");

		/** 발사색을 보여 주는 총 내부 머터리얼 경로. */
		inline constexpr const TCHAR* GunTubeMaterial =
			TEXT("/Game/WormholePortal/Meshes/PortalGun/Materials/M_PortalGun_Tube_Dynamic.M_PortalGun_Tube_Dynamic");

		/** 모든 포탈 에너지 효과가 같이 쓰는 머터리얼 경로. */
		inline constexpr const TCHAR* EnergyMaterial = TEXT("/Game/WormholePortal/VFX/Materials/M_PortalEnergy_Additive.M_PortalEnergy_Additive");

		/** 1인칭 캐릭터 자세를 잡는 애니메이션 블루프린트 경로. */
		inline constexpr const TCHAR* FirstPersonCharacterAnimation = TEXT("/Game/Variant_Shooter/Anims/ABP_FP_Weapon");

		/** 3인칭 캐릭터 자세를 잡는 애니메이션 블루프린트 경로. */
		inline constexpr const TCHAR* ThirdPersonCharacterAnimation = TEXT("/Game/Variant_Shooter/Anims/ABP_TP_Rifle");
	} // namespace Asset

	namespace Color
	{
		/** 파란 포탈의 기본색. */
		inline const FLinearColor Blue(0.02f, 0.14f, 0.65f, 1.0f);

		/** 주황 포탈의 기본색. */
		inline const FLinearColor Orange(0.8f, 0.19f, 0.025f, 1.0f);

		/** 아직 발사하지 않은 총 내부의 옅은 색. */
		inline const FLinearColor IdleTube(0.42f, 0.68f, 0.58f, 1.0f);

		inline const FLinearColor& Select(const bool bBluePortal)
		{
			return bBluePortal ? Blue : Orange;
		}
	} // namespace Color

	namespace Material
	{
		/** 동적 머터리얼에 전달하는 포탈 색 파라미터 이름. */
		inline constexpr const TCHAR* EffectColor = TEXT("EffectColor");

		/** 동적 머터리얼에 전달하는 기본색 파라미터 이름. */
		inline constexpr const TCHAR* DiffuseColor = TEXT("DiffuseColor");

		/** 동적 머터리얼에 전달하는 발광색 파라미터 이름. */
		inline constexpr const TCHAR* EmissiveColor = TEXT("EmissiveColor");

		/** 동적 머터리얼의 밝기 파라미터 이름. */
		inline constexpr const TCHAR* EmissiveStrength = TEXT("EmissiveStrength");

		/** 동적 머터리얼의 투명도 파라미터 이름. */
		inline constexpr const TCHAR* Opacity = TEXT("Opacity");

		/** 기존 색상 맵을 끄는 파라미터 이름. */
		inline constexpr const TCHAR* EmissiveColorMapWeight = TEXT("EmissiveColorMapWeight");
	} // namespace Material

	namespace Weapon
	{
		/** 모델의 총구 뼈 이름. */
		inline constexpr const TCHAR* MuzzleBone = TEXT("weapon_bone");

		/** 총구 뼈를 기준으로 한 실제 총구 위치. 단위는 cm. */
		inline const FVector MuzzleLocation(-2.479562f, 0.000011f, -21.024710f);

		/** 포탈건 모델의 +Y 방향을 언리얼의 +X 발사 방향으로 돌리는 값. */
		inline const FRotator MuzzleRotation(-90.0f, 0.0f, 0.0f);

		/** 발사체 속도. 단위는 cm/s. */
		inline constexpr float ProjectileSpeed = 5000.0f;

		/** 포탈건이 표시하는 탄 수. 파랑과 주황 두 칸이다. */
		inline constexpr int32 MagazineSize = 2;

		/** 발사체가 총과 바로 충돌하지 않도록 총구 앞에서 띄우는 거리. */
		inline constexpr float ProjectileSpawnClearance = 8.0f;

		/** 파란 포탈과 주황 포탈의 최소 발사 간격. 단위는 초. */
		inline constexpr float RefireDelay = 0.25f;

		/** 발사할 때 총이 뒤로 움직이는 최대 거리. 단위는 cm. */
		inline constexpr float RecoilDistance = 24.0f;

		/** 총이 가장 뒤까지 밀리는 시간. 단위는 초. */
		inline constexpr float RecoilKickTime = 0.065f;

		/** 밀린 총이 원래 위치로 돌아오는 시간. 단위는 초. */
		inline constexpr float RecoilRecoveryTime = 0.58f;

		/** 3인칭 반동을 1인칭 반동보다 줄이는 비율. */
		inline constexpr float ThirdPersonRecoilRatio = 0.75f;

		/** 반동이 처음에 빠르게 움직이게 만드는 곡선의 지수. */
		inline constexpr float RecoilEasePower = 3.0f;

		/** 1인칭 총이 손 위치를 따라가는 속도. 낮을수록 더 부드럽지만 반응이 느려진다. */
		inline constexpr float FirstPersonLocationInterpSpeed = 16.0f;

		/** 1인칭 총이 손 회전을 따라가는 속도. 낮을수록 회전이 더 부드럽게 보인다. */
		inline constexpr float FirstPersonRotationInterpSpeed = 18.0f;

		/** 에디터에서만 보이는 총구 화살표의 색과 크기. */
		inline const FColor MuzzleArrowColor(0, 160, 255);
		inline constexpr float MuzzleArrowSize = 0.35f;

		/** 네이티브 클래스만 쓸 때의 1인칭 기본 위치. Blueprint 값이 있으면 그 값이 우선한다. */
		inline const FVector DefaultFirstPersonLocation(12.0f, -18.0f, -32.0f);

		/** 네이티브 클래스만 쓸 때의 1인칭 기본 회전. Blueprint 값이 있으면 그 값이 우선한다. */
		inline const FRotator DefaultFirstPersonRotation(0.0f, 0.0f, -10.0f);
	} // namespace Weapon

	namespace Tube
	{
		/** 색이 바뀌는 총 내부 머터리얼 슬롯 이름. */
		inline constexpr const TCHAR* MaterialSlot = TEXT("Portal_Gun_Glass");

		/** 발사 전 총 내부 밝기. */
		inline constexpr float IdleEmissiveStrength = 0.35f;

		/** 발사 전 총 내부 투명도. */
		inline constexpr float IdleOpacity = 0.09f;

		/** 발사 후 총 내부 밝기. */
		inline constexpr float FiredEmissiveStrength = 4.0f;

		/** 발사 후 총 내부 투명도. */
		inline constexpr float FiredOpacity = 0.42f;
	} // namespace Tube

	namespace Portal
	{
		/** 포탈 중심을 벽 밖으로 옮길 때 반지름에 곱하는 비율. */
		inline constexpr float SurfaceOffsetRatio = 0.5f;

		/** 성장 중 포탈이 완전히 0이 되지 않게 막는 최소 크기. */
		inline constexpr float MinimumGrowthScale = 0.01f;

		/** 포탈이 빠르게 열리고 천천히 멈추게 만드는 곡선의 지수. */
		inline constexpr float GrowthEasePower = 5.0f;
	} // namespace Portal

	namespace Projectile
	{
		/** 날아가는 흰색 코어의 길이와 반지름. 단위는 cm. */
		inline constexpr float CoreLength = 20.0f;
		inline constexpr float CoreRadius = 6.0f;

		/** 흰색 코어를 진행 방향의 뒤쪽으로 옮기는 거리. 단위는 cm. */
		inline constexpr float CoreBackOffset = 5.0f;

		/** 흰색 코어의 밝기와 투명도. */
		inline constexpr float CoreEmissiveStrength = 3.5f;
		inline constexpr float CoreOpacity = 0.07f;

		/** 속도감을 주는 흐린 구체의 길이와 반지름. 단위는 cm. */
		inline constexpr float BlurLength = 55.0f;
		inline constexpr float BlurRadius = 2.75f;

		/** 흐린 구체를 진행 방향의 뒤쪽으로 옮기는 거리. 단위는 cm. */
		inline constexpr float BlurBackOffset = 20.0f;

		/** 흐린 구체의 밝기와 투명도. */
		inline constexpr float BlurEmissiveStrength = 2.5f;
		inline constexpr float BlurOpacity = 0.025f;

		/** 발사체 충돌 반지름. 단위는 cm. */
		inline constexpr float CollisionRadius = 4.0f;

		/** 발사체 조명의 밝기와 범위. */
		inline constexpr float LightIntensity = 120.0f;
		inline constexpr float LightRadius = 70.0f;
		inline constexpr float LightSourceRadius = 2.0f;

		/** 반투명 요소가 겹칠 때 그리는 순서. 숫자가 클수록 앞에 그린다. */
		inline constexpr int32 CoreSortPriority = 8;
		inline constexpr int32 BlurSortPriority = 7;
		inline constexpr int32 TrailSortPriority = 6;

		/** 아무것도 맞히지 못한 발사체가 자동으로 사라지는 시간. */
		inline constexpr float LifeTime = 2.5f;

		/** 에너지 색을 더 선명하게 만드는 발광색 배수. */
		inline constexpr float EmissiveColorMultiplier = 20.0f;

		/** 발사체가 벽을 맞을 때 충돌 지점에서 띄우는 거리. 단위는 cm. */
		inline constexpr float ImpactEffectOffset = 3.0f;

		/** 빠른 발사체의 충돌 누락을 줄이기 위한 최대 시뮬레이션 간격. */
		inline constexpr float MaxSimulationStep = 1.0f / 120.0f;

		/** 한 프레임 안에서 허용하는 최대 발사체 시뮬레이션 횟수. */
		inline constexpr int32 MaxSimulationIterations = 16;
	} // namespace Projectile

	namespace Trail
	{
		/** 꼬리 전체 길이와 앞뒤 반지름. 단위는 cm. */
		inline constexpr float Length = 2000.0f;
		inline constexpr float StartRadius = 2.75f;
		inline constexpr float EndRadius = 1.25f;

		/** 꼬리 앞뒤의 밝기와 투명도. */
		inline constexpr float StartEmissiveStrength = 2.8f;
		inline constexpr float EndEmissiveStrength = 1.0f;
		inline constexpr float StartOpacity = 0.035f;
		inline constexpr float EndOpacity = 0.006f;

		/** 꼬리를 나누는 구체 조각 개수. */
		inline constexpr int32 SegmentCount = 4;

		/** 각 꼬리 조각의 중심 위치 비율. */
		inline constexpr float CenterFractions[SegmentCount] = {0.125f, 0.267857f, 0.446429f, 0.714286f};

		/** 각 꼬리 조각의 길이 비율. */
		inline constexpr float LengthFractions[SegmentCount] = {0.178571f, 0.267857f, 0.392857f, 0.571429f};

		/** 총 안쪽에 꼬리가 생기지 않도록 조각별 표시 시작 거리를 정하는 비율. */
		inline constexpr float RevealFractions[SegmentCount] = {0.071429f, 0.196429f, 0.357143f, 0.571429f};
	} // namespace Trail

	namespace Muzzle
	{
		/** 총구 이펙트 전체를 총구 앞쪽으로 옮기는 거리. 단위는 cm. */
		inline constexpr float SpawnForwardOffset = 2.0f;

		/** 바깥쪽 색상 섬광의 중심, 길이, 반지름. 단위는 cm. */
		inline constexpr float OuterCenter = 14.0f;
		inline constexpr float OuterLength = 100.0f;
		inline constexpr float OuterRadius = 9.0f;

		/** 바깥쪽 색상 섬광의 밝기와 투명도. */
		inline constexpr float OuterEmissiveStrength = 7.0f;
		inline constexpr float OuterOpacity = 0.12f;

		/** 흰색 코어의 중심, 높이, 길이, 반지름. 단위는 cm. */
		inline constexpr float WhiteCoreCenter = 18.0f;
		inline constexpr float WhiteCoreHeight = 0.0f;
		inline constexpr float WhiteCoreLength = 65.0f;
		inline constexpr float WhiteCoreRadius = 5.25f;

		/** 흰색 코어의 밝기, 투명도, 흰색 혼합 비율. */
		inline constexpr float WhiteCoreEmissiveStrength = 8.0f;
		inline constexpr float WhiteCoreOpacity = 0.17f;
		inline constexpr float WhiteCoreBlend = 0.75f;

		/** 만들어 둘 파티클 수와 실제로 보여 줄 파티클 수. */
		inline constexpr int32 ParticlePoolSize = 24;
		inline constexpr int32 VisibleParticleCount = 20;

		/** 총구 파티클의 밝기, 투명도, 흰색 혼합 비율. */
		inline constexpr float ParticleEmissiveStrength = 7.0f;
		inline constexpr float ParticleOpacity = 0.20f;
		inline constexpr float ParticleWhiteBlend = 0.40f;

		/** 총구 파티클이 만들어지는 위치와 범위. 단위는 cm. */
		inline constexpr float ParticleSpawnForwardOffset = -2.0f;
		inline constexpr float ParticleSpawnRadius = 3.0f;

		/** 총구 파티클이 옆으로 퍼지는 비율. */
		inline constexpr float ParticleSpread = 0.75f;

		/** 총구 파티클의 최소·최대 속도. 단위는 cm/s. */
		inline constexpr float ParticleMinSpeed = 320.0f;
		inline constexpr float ParticleMaxSpeed = 320.0f;

		/** 총구 파티클의 최소·최대 길이와 반지름. 단위는 cm. */
		inline constexpr float ParticleMinLength = 7.0f;
		inline constexpr float ParticleMaxLength = 21.0f;
		inline constexpr float ParticleMinRadius = 0.42f;
		inline constexpr float ParticleMaxRadius = 1.12f;

		/** 총구 파티클의 아래 방향 가속도와 공기 저항. */
		inline constexpr float ParticleGravity = 60.0f;
		inline constexpr float ParticleDrag = 3.5f;

		/** 총구 섬광이 유지되는 전체 시간과 빠르게 커지는 시간. */
		inline constexpr float LifeTime = 0.16f;
		inline constexpr float AttackTime = 0.018f;

		/** 총구 섬광이 줄어들기 시작하는 전체 시간의 비율. */
		inline constexpr float CollapseStart = 0.42f;

		/** 총구 조명의 밝기와 범위. */
		inline constexpr float LightIntensity = 650.0f;
		inline constexpr float LightRadius = 320.0f;
	} // namespace Muzzle

	namespace Impact
	{
		/** 충돌 섬광의 위치, 길이, 반지름. 단위는 cm. */
		inline constexpr float FlashForwardOffset = 2.0f;
		inline constexpr float FlashLength = 4.5f;
		inline constexpr float FlashRadius = 3.5f;

		/** 충돌 섬광의 밝기, 투명도, 커지는 배율. */
		inline constexpr float FlashEmissiveStrength = 6.0f;
		inline constexpr float FlashOpacity = 0.13f;
		inline constexpr float FlashExpansion = 2.4f;

		/** 충돌 파동의 위치, 길이, 반지름. 단위는 cm. */
		inline constexpr float WaveForwardOffset = 1.0f;
		inline constexpr float WaveLength = 0.6f;
		inline constexpr float WaveRadius = 6.0f;

		/** 충돌 파동의 밝기, 투명도, 커지는 배율. */
		inline constexpr float WaveEmissiveStrength = 3.0f;
		inline constexpr float WaveOpacity = 0.07f;
		inline constexpr float WaveExpansion = 2.6f;

		/** 충돌 이펙트가 유지되는 시간. 단위는 초. */
		inline constexpr float LifeTime = 0.38f;

		/** 충돌 조명의 밝기와 범위. */
		inline constexpr float LightIntensity = 1100.0f;
		inline constexpr float LightRadius = 320.0f;

		/** 충돌 파편으로 보여 줄 개수. */
		inline constexpr int32 VisibleParticleCount = 16;

		/** 충돌 파편의 밝기와 투명도. */
		inline constexpr float ParticleEmissiveStrength = 5.5f;
		inline constexpr float ParticleOpacity = 0.32f;

		/** 충돌 파편의 최소·최대 속도. 단위는 cm/s. */
		inline constexpr float ParticleMinSpeed = 220.0f;
		inline constexpr float ParticleMaxSpeed = 540.0f;

		/** 충돌 파편의 최소·최대 길이와 반지름. 단위는 cm. */
		inline constexpr float ParticleMinLength = 7.0f;
		inline constexpr float ParticleMaxLength = 16.0f;
		inline constexpr float ParticleMinRadius = 0.2f;
		inline constexpr float ParticleMaxRadius = 0.45f;

		/** 충돌 파편의 아래 방향 가속도와 공기 저항. */
		inline constexpr float ParticleGravity = 520.0f;
		inline constexpr float ParticleDrag = 2.4f;
	} // namespace Impact

	namespace Burst
	{
		/** 기본 구체 메시의 길이와 지름. 단위는 cm. */
		inline constexpr float SphereLength = 100.0f;
		inline constexpr float SphereDiameter = 50.0f;

		/** 반투명 요소가 겹칠 때 그리는 순서. 숫자가 클수록 앞에 그린다. */
		inline constexpr int32 OuterSortPriority = 12;
		inline constexpr int32 InnerSortPriority = 11;
		inline constexpr int32 ParticleSortPriority = 13;

		/** 섬광 조명의 빛이 시작되는 반지름. 단위는 cm. */
		inline constexpr float LightSourceRadius = 6.0f;

		/** 이펙트 수명 뒤 안전하게 액터를 정리하기 위한 여유 시간. */
		inline constexpr float DestroyDelay = 0.05f;

		/** 이펙트가 사라지기 시작하는 전체 시간의 비율. */
		inline constexpr float FadeStart = 0.12f;

		/** 섬광이 처음에 빠르게 변하게 만드는 곡선의 지수. */
		inline constexpr float EaseOutPower = 3.0f;

		/** 충돌 파동의 진행 방향 길이가 마지막에 남는 비율. */
		inline constexpr float ImpactWaveEndLengthRatio = 0.4f;

		/** 총구 바깥 섬광이 처음 나타나고 사라질 때 쓰는 크기 배율. */
		inline constexpr float MuzzleOuterStartLength = 0.72f;
		inline constexpr float MuzzleOuterPeakLength = 1.08f;
		inline constexpr float MuzzleOuterEndLength = 0.82f;
		inline constexpr float MuzzleOuterStartRadius = 0.62f;
		inline constexpr float MuzzleOuterEndRadius = 0.72f;

		/** 총구 흰색 코어가 시작하고 끝날 때 쓰는 크기 배율. */
		inline constexpr float MuzzleInnerStartScale = 0.7f;
		inline constexpr float MuzzleInnerEndScale = 1.25f;

		/** 파티클 길이가 수명 끝에 남는 비율. */
		inline constexpr float ParticleEndLengthRatio = 0.45f;

		/** 파티클이 완전히 납작해지지 않게 남겨 두는 최소 반지름 비율. */
		inline constexpr float ParticleMinimumRadiusRatio = 0.08f;

		/** 충돌 파편이 시작하는 표면 앞 거리. 단위는 cm. */
		inline constexpr float ImpactParticleStart = 3.0f;

		/** 랜덤 모양은 유지하면서 발사 색마다 다른 결과를 만드는 시드 값. */
		inline constexpr int32 RandomSeedMultiplier = 196613;
		inline constexpr int32 BlueRandomSeed = 17;
		inline constexpr int32 OrangeRandomSeed = 31;
	} // namespace Burst
} // namespace WPFinalPortalGunConstants
