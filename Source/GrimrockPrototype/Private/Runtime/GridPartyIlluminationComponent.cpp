#include "Runtime/GridPartyIlluminationComponent.h"

#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

namespace
{
	float GridPartyIlluminationGetScore(const FGridLightEmitterConfig& Config)
	{
		return Config.bUsePointLight ? Config.LightIntensity : -1.0f;
	}
}

UGridPartyIlluminationComponent::UGridPartyIlluminationComponent()
{
	SetPointLightCastShadows(true);
}

void UGridPartyIlluminationComponent::BeginPlay()
{
	Super::BeginPlay();
	SetPointLightCastShadows(bCastShadows);
}

FGridLightEmitterConfig UGridPartyIlluminationComponent::BuildPartyConfig(const FGridLightEmitterConfig& SourceConfig) const
{
	FGridLightEmitterConfig PartyConfig = SourceConfig;

	// VisualEffects remains on the item. This config only contains PointLight data.
	// Placement is controlled by this component's transform under the camera,
	// not by the source item's physical point-light offset.
	PartyConfig.PointLightRelativeLocation = FVector::ZeroVector;
	PartyConfig.PointLightRelativeRotation = FRotator::ZeroRotator;
	PartyConfig.bDefaultEnabled = false;

	const float SafeIntensityMultiplier = FMath::Max(0.0f, IntensityMultiplier);
	const float SafeRadiusMultiplier = FMath::Max(0.0f, RadiusMultiplier);

	PartyConfig.LightIntensity *= SafeIntensityMultiplier;
	PartyConfig.FlickerIntensityAmount *= SafeIntensityMultiplier;

	PartyConfig.LightRadius *= SafeRadiusMultiplier;
	PartyConfig.FlickerRadiusAmount *= SafeRadiusMultiplier;

	return PartyConfig;
}

void UGridPartyIlluminationComponent::ApplyIlluminationSource(const FGridLightEmitterConfig& SourceConfig, FName SourceId)
{
	if (SourceId.IsNone() || !SourceConfig.bUsePointLight)
	{
		ClearIlluminationSource();
		return;
	}

	ActiveSourceId = SourceId;
	SetPointLightCastShadows(bCastShadows);
	ApplyConfig(BuildPartyConfig(SourceConfig));
	SetLightEnabled(true);
}

void UGridPartyIlluminationComponent::ClearIlluminationSource()
{
	ActiveSourceId = NAME_None;
	SetLightEnabled(false);
	ApplyConfig(FGridLightEmitterConfig());
	SetPointLightCastShadows(bCastShadows);
}

void UGridPartyIlluminationComponent::RefreshFromEquipment(UGridPartyInventoryComponent* Inventory, AGridLevelRuntimeActor* LevelRuntimeActor)
{
	if (!Inventory)
	{
		ClearIlluminationSource();
		return;
	}

	FGridLightEmitterConfig BestConfig;
	bool bHasBestConfig = false;
	FName BestSourceId = NAME_None;
	float BestScore = -1.0f;
	const UWorld* OwningWorld = GetWorld();
	UGameInstance* GameInstance = OwningWorld ? OwningWorld->GetGameInstance() : nullptr;
	const UGridItemSubsystem* Registry = GameInstance
		? GameInstance->GetSubsystem<UGridItemSubsystem>() : nullptr;

	const int32 ActiveCharacterCount = Inventory->GetActiveCharacterCount();
	const EGridEquipmentSlot HandSlots[] = {EGridEquipmentSlot::MainHand, EGridEquipmentSlot::OffHand};
	for (int32 CharacterIndex = 0; CharacterIndex < ActiveCharacterCount; ++CharacterIndex)
	{
		for (EGridEquipmentSlot HandSlot : HandSlots)
		{
			FGridItemInstance CandidateItem;
			if (!Inventory->GetEquippedItem(CharacterIndex, HandSlot, CandidateItem) || !CandidateItem.bLightsEnabled)
			{
				continue;
			}

				FGridLightEmitterConfig CandidateConfig;
			bool bHasConfig = false;
			const bool bCanonicalRegistered = Registry &&
				Registry->HasCanonicalItem(CandidateItem.ItemDefinitionId);
			if (bCanonicalRegistered)
			{
				FGridResolvedItem Canonical;
				if (!Registry->ResolveItem(CandidateItem.ItemDefinitionId,
					CandidateItem.ItemLevel, Canonical) ||
					(!Canonical.bIsArmament && CandidateItem.ItemLevel != 0))
				{
					continue; // Invalid physical N0 ARM or non-ARM N1: no fallback.
				}
				// X.41: the native X.38 ARM visual parity covers LightEmitter.
				// All registered families use the same canonical authority even
				// if a stale DA_Weapon_* still exists in inventory.
				CandidateConfig = Canonical.LightEmitter;
				bHasConfig = true;
			}
			else
			{
				// Genuine out-of-registry items remain temporarily historical.
				UGridItemDefinitionAsset* CandidateDefinition =
					Inventory->FindItemDefinition(CandidateItem.ItemDefinitionId);
				if (!CandidateDefinition && LevelRuntimeActor)
				{
					CandidateDefinition = LevelRuntimeActor->ResolveRuntimeItemDefinition(
						CandidateItem.ItemDefinitionId);
				}
				if (CandidateDefinition)
				{
					CandidateConfig = CandidateDefinition->LightEmitter;
					bHasConfig = true;
				}
			}

			const float CandidateScore = bHasConfig
				? GridPartyIlluminationGetScore(CandidateConfig) : -1.0f;
			if (CandidateScore > BestScore)
			{
				BestScore = CandidateScore;
				BestConfig = CandidateConfig;
				bHasBestConfig = true;
				BestSourceId = CandidateItem.ItemDefinitionId;
			}
		}
	}

	if (bHasBestConfig)
	{
		ApplyIlluminationSource(BestConfig, BestSourceId);
	}
	else
	{
		ClearIlluminationSource();
	}
}
