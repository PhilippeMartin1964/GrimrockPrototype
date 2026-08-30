#include "Runtime/Monsters/GridMonsterDeathComponent.h"

#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogGridMonsterDeathDissolve, Log, All);

namespace
{
	constexpr float DeathDissolveUpdateInterval = 1.0f / 30.0f;
	constexpr float CorpseHoldDelaySeconds = 2.0f;
	constexpr float CorpseDissolveDurationSeconds = 1.5f;
	const FName CorpseDissolveParameterName(TEXT("DissolveAmount"));
}

void UGridMonsterDeathComponent::BeginPlay()
{
	Super::BeginPlay();

	if (InitializeDeathComponent(RuntimeActor) && OwnerMonster)
	{
		OwnerMonster->OnMonsterDied.AddUniqueDynamic(this, &UGridMonsterDeathComponent::HandleOwnerMonsterDied);
	}
}

void UGridMonsterDeathComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (OwnerMonster)
	{
		OwnerMonster->OnMonsterDied.RemoveDynamic(this, &UGridMonsterDeathComponent::HandleOwnerMonsterDied);
	}

	ResetDeathDissolvePresentation(true, false);
	Super::EndPlay(EndPlayReason);
}

void UGridMonsterDeathComponent::HandleOwnerMonsterDied(AGridMonsterActor* Monster, FIntPoint InDeathCell)
{
	(void)InDeathCell;

	if (!OwnerMonster || Monster != OwnerMonster || !bDeathCommitted)
	{
		return;
	}

	ScheduleDeathDissolve();
}

void UGridMonsterDeathComponent::ScheduleDeathDissolve()
{
	if (!OwnerMonster || !OwnerMonster->MonsterDefinition)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	ClearDeathDissolveTimers();

	const UGridMonsterDefinitionAsset* Definition = OwnerMonster->MonsterDefinition;
	const float PresentationDuration = Definition->DeathMontage.IsNull() ? 0.0f : FMath::Max(0.01f, Definition->DeathExpectedDuration);
	const float StartDelay = PresentationDuration + CorpseHoldDelaySeconds;

	UE_LOG(LogGridMonsterDeathDissolve, Log, TEXT("[GridMonsterDeathDissolve] Scheduled Monster=%s Presentation=%.3f Hold=%.3f StartAfter=%.3f"),
		*GetNameSafe(OwnerMonster), PresentationDuration, CorpseHoldDelaySeconds, StartDelay);

	if (StartDelay <= KINDA_SMALL_NUMBER)
	{
		StartDeathDissolve();
		return;
	}

	World->GetTimerManager().SetTimer(DeathDissolveDelayTimerHandle, this, &UGridMonsterDeathComponent::StartDeathDissolve, StartDelay, false);
}

void UGridMonsterDeathComponent::StartDeathDissolve()
{
	if (!OwnerMonster || !OwnerMonster->MonsterDefinition || !bDeathCommitted)
	{
		return;
	}

	UWorld* World = GetWorld();
	USkeletalMeshComponent* Mesh = OwnerMonster->SkeletalMeshComponent;
	if (!World || !Mesh)
	{
		return;
	}

	World->GetTimerManager().ClearTimer(DeathDissolveDelayTimerHandle);

	DeathOriginalMaterials.Reset();
	DeathDissolveMaterials.Reset();

	const FName ParameterName = CorpseDissolveParameterName;
	const int32 MaterialCount = Mesh->GetNumMaterials();
	DeathOriginalMaterials.Reserve(MaterialCount);
	DeathDissolveMaterials.Reserve(MaterialCount);

	for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
	{
		UMaterialInterface* SourceMaterial = Mesh->GetMaterial(MaterialIndex);
		DeathOriginalMaterials.Add(SourceMaterial);

		UMaterialInstanceDynamic* DynamicMaterial = Mesh->CreateDynamicMaterialInstance(MaterialIndex, SourceMaterial);
		DeathDissolveMaterials.Add(DynamicMaterial);

		if (DynamicMaterial)
		{
			DynamicMaterial->SetScalarParameterValue(ParameterName, 0.0f);
		}
	}

	bDeathDissolveActive = true;
	DeathDissolveAlpha = 0.0f;
	DeathDissolveStartWorldTime = World->GetTimeSeconds();

	UE_LOG(LogGridMonsterDeathDissolve, Log, TEXT("[GridMonsterDeathDissolve] Started Monster=%s Materials=%d Parameter=%s Duration=%.3f"),
		*GetNameSafe(OwnerMonster), MaterialCount, *ParameterName.ToString(), CorpseDissolveDurationSeconds);

	World->GetTimerManager().SetTimer(DeathDissolveStepTimerHandle, this, &UGridMonsterDeathComponent::UpdateDeathDissolve, DeathDissolveUpdateInterval, true);

	UpdateDeathDissolve();
}

void UGridMonsterDeathComponent::UpdateDeathDissolve()
{
	if (!bDeathDissolveActive || !OwnerMonster || !OwnerMonster->MonsterDefinition)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const float Duration = CorpseDissolveDurationSeconds;
	DeathDissolveAlpha = FMath::Clamp((World->GetTimeSeconds() - DeathDissolveStartWorldTime) / Duration, 0.0f, 1.0f);

	const FName ParameterName = CorpseDissolveParameterName;
	for (UMaterialInstanceDynamic* DynamicMaterial : DeathDissolveMaterials)
	{
		if (DynamicMaterial)
		{
			DynamicMaterial->SetScalarParameterValue(ParameterName, DeathDissolveAlpha);
		}
	}

	if (DeathDissolveAlpha >= 1.0f - KINDA_SMALL_NUMBER)
	{
		FinishDeathDissolve();
	}
}

void UGridMonsterDeathComponent::FinishDeathDissolve()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DeathDissolveStepTimerHandle);
	}

	if (OwnerMonster && OwnerMonster->MonsterDefinition)
	{
		const FName ParameterName = CorpseDissolveParameterName;
		for (UMaterialInstanceDynamic* DynamicMaterial : DeathDissolveMaterials)
		{
			if (DynamicMaterial)
			{
				DynamicMaterial->SetScalarParameterValue(ParameterName, 1.0f);
			}
		}
	}

	DeathDissolveAlpha = 1.0f;
	bDeathDissolveActive = false;

	if (OwnerMonster && OwnerMonster->SkeletalMeshComponent)
	{
		OwnerMonster->SkeletalMeshComponent->SetVisibility(false, true);
	}

	UE_LOG(LogGridMonsterDeathDissolve, Log, TEXT("[GridMonsterDeathDissolve] Completed Monster=%s MeshHidden=true ActorRetained=true"),
		*GetNameSafe(OwnerMonster));
}

void UGridMonsterDeathComponent::ResetDeathDissolvePresentation(bool bRestoreOriginalMaterials, bool bRestoreVisibility)
{
	ClearDeathDissolveTimers();

	if (bRestoreOriginalMaterials && OwnerMonster && OwnerMonster->SkeletalMeshComponent)
	{
		USkeletalMeshComponent* Mesh = OwnerMonster->SkeletalMeshComponent;
		for (int32 MaterialIndex = 0; MaterialIndex < DeathOriginalMaterials.Num(); ++MaterialIndex)
		{
			Mesh->SetMaterial(MaterialIndex, DeathOriginalMaterials[MaterialIndex]);
		}
	}

	DeathOriginalMaterials.Reset();
	DeathDissolveMaterials.Reset();
	DeathDissolveStartWorldTime = 0.0f;
	DeathDissolveAlpha = 0.0f;
	bDeathDissolveActive = false;

	if (bRestoreVisibility && OwnerMonster)
	{
		OwnerMonster->SetActorHiddenInGame(false);
		if (OwnerMonster->SkeletalMeshComponent)
		{
			OwnerMonster->SkeletalMeshComponent->SetVisibility(true, true);
		}
	}
}

void UGridMonsterDeathComponent::ClearDeathDissolveTimers()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DeathDissolveDelayTimerHandle);
		World->GetTimerManager().ClearTimer(DeathDissolveStepTimerHandle);
	}
}
