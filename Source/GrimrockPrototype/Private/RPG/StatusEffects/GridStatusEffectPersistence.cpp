#include "RPG/StatusEffects/GridStatusEffectPersistence.h"

#include "Engine/AssetManager.h"
#include "RPG/StatusEffects/GridStatusEffectDefinitionAsset.h"

namespace
{
	const FPrimaryAssetType StatusEffectPrimaryAssetType(TEXT("GridStatusEffect"));

	bool StatusSaveStateLess(const FGridStatusEffectSaveState& Left, const FGridStatusEffectSaveState& Right)
	{
		return Left.EffectId.ToString().Compare(Right.EffectId.ToString(), ESearchCase::CaseSensitive) < 0;
	}

	bool RuntimeStateLess(const FGridStatusEffectRuntimeState& Left, const FGridStatusEffectRuntimeState& Right)
	{
		return Left.EffectId.ToString().Compare(Right.EffectId.ToString(), ESearchCase::CaseSensitive) < 0;
	}
}

bool FGridStatusEffectPersistence::ValidateSavedCollection(const TArray<FGridStatusEffectSaveState>& SavedStates, FString& OutError)
{
	TSet<FName> EffectIds;
	for (const FGridStatusEffectSaveState& SavedState : SavedStates)
	{
		if (!SavedState.IsStructurallyValid())
		{
			OutError = FString::Printf(TEXT("Saved status effect '%s' has an invalid runtime snapshot."), *SavedState.EffectId.ToString());
			return false;
		}
		if (EffectIds.Contains(SavedState.EffectId))
		{
			OutError = FString::Printf(TEXT("Saved status effect '%s' is duplicated."), *SavedState.EffectId.ToString());
			return false;
		}
		EffectIds.Add(SavedState.EffectId);
	}

	OutError.Reset();
	return true;
}

bool FGridStatusEffectPersistence::CaptureCollection(
	const FGridStatusEffectCollection& RuntimeCollection, TArray<FGridStatusEffectSaveState>& OutSavedStates, FString& OutError)
{
	TArray<FGridStatusEffectSaveState> Candidate;
	Candidate.Reserve(RuntimeCollection.ActiveEffects.Num());

	for (const FGridStatusEffectRuntimeState& RuntimeState : RuntimeCollection.ActiveEffects)
	{
		if (!RuntimeState.IsValid())
		{
			OutError = FString::Printf(TEXT("Runtime status effect '%s' is invalid and cannot be saved."), *RuntimeState.EffectId.ToString());
			return false;
		}

		const UGridStatusEffectDefinitionAsset* Definition = RuntimeState.DefinitionAsset.Get();
		if (!IsValid(Definition) || !Definition->IsValidDefinition() || Definition->EffectId != RuntimeState.EffectId ||
			Definition->DurationUnit != RuntimeState.DurationUnit || RuntimeState.StackCount > Definition->MaxStacks)
		{
			OutError = FString::Printf(TEXT("Runtime status effect '%s' has no matching valid definition for persistence."), *RuntimeState.EffectId.ToString());
			return false;
		}

		FGridStatusEffectSaveState SavedState;
		SavedState.EffectId = RuntimeState.EffectId;
		SavedState.SourceId = RuntimeState.SourceId;
		SavedState.StackCount = RuntimeState.StackCount;
		SavedState.DurationUnit = RuntimeState.DurationUnit;
		SavedState.RemainingDuration = RuntimeState.RemainingDuration;
		SavedState.Potency = RuntimeState.Potency;
		Candidate.Add(MoveTemp(SavedState));
	}

	Candidate.Sort(&StatusSaveStateLess);
	if (!ValidateSavedCollection(Candidate, OutError))
	{
		return false;
	}

	OutSavedStates = MoveTemp(Candidate);
	OutError.Reset();
	return true;
}

bool FGridStatusEffectPersistence::RestoreCollection(const TArray<FGridStatusEffectSaveState>& SavedStates,
	TFunctionRef<UGridStatusEffectDefinitionAsset*(FName)> DefinitionResolver, FGridStatusEffectCollection& OutRuntimeCollection, FString& OutError)
{
	if (!ValidateSavedCollection(SavedStates, OutError))
	{
		return false;
	}

	FGridStatusEffectCollection Candidate;
	Candidate.ActiveEffects.Reserve(SavedStates.Num());

	for (const FGridStatusEffectSaveState& SavedState : SavedStates)
	{
		UGridStatusEffectDefinitionAsset* Definition = DefinitionResolver(SavedState.EffectId);
		if (!IsValid(Definition) || !Definition->IsValidDefinition() || Definition->EffectId != SavedState.EffectId)
		{
			OutError = FString::Printf(TEXT("Saved status effect '%s' cannot resolve its canonical definition."), *SavedState.EffectId.ToString());
			return false;
		}
		if (Definition->DurationUnit != SavedState.DurationUnit)
		{
			OutError = FString::Printf(TEXT("Saved status effect '%s' duration unit no longer matches its definition."), *SavedState.EffectId.ToString());
			return false;
		}
		if (SavedState.StackCount > Definition->MaxStacks)
		{
			OutError = FString::Printf(TEXT("Saved status effect '%s' stack count %d exceeds definition MaxStacks %d."), *SavedState.EffectId.ToString(),
				SavedState.StackCount, Definition->MaxStacks);
			return false;
		}

		FGridStatusEffectRuntimeState RuntimeState;
		FString RehydrateError;
		if (!Definition->BuildRuntimeState(
				SavedState.SourceId, SavedState.StackCount, SavedState.RemainingDuration, SavedState.Potency, RuntimeState, RehydrateError))
		{
			OutError = FString::Printf(TEXT("Saved status effect '%s' failed runtime rehydration: %s"), *SavedState.EffectId.ToString(), *RehydrateError);
			return false;
		}

		Candidate.ActiveEffects.Add(MoveTemp(RuntimeState));
	}

	Candidate.ActiveEffects.Sort(&RuntimeStateLess);
	OutRuntimeCollection = MoveTemp(Candidate);
	OutError.Reset();
	return true;
}

bool FGridStatusEffectPersistence::RestoreCollection(
	const TArray<FGridStatusEffectSaveState>& SavedStates, FGridStatusEffectCollection& OutRuntimeCollection, FString& OutError)
{
	return RestoreCollection(
		SavedStates,
		[](FName EffectId)
		{
			return FGridStatusEffectPersistence::ResolveDefinitionByEffectId(EffectId);
		},
		OutRuntimeCollection, OutError);
}

UGridStatusEffectDefinitionAsset* FGridStatusEffectPersistence::ResolveDefinitionByEffectId(FName EffectId)
{
	if (EffectId.IsNone())
	{
		return nullptr;
	}

	UAssetManager& AssetManager = UAssetManager::Get();
	const FPrimaryAssetId PrimaryAssetId(StatusEffectPrimaryAssetType, EffectId);

	UGridStatusEffectDefinitionAsset* Definition = AssetManager.GetPrimaryAssetObject<UGridStatusEffectDefinitionAsset>(PrimaryAssetId);
	if (!IsValid(Definition))
	{
		FSoftObjectPath AssetPath = AssetManager.GetPrimaryAssetPath(PrimaryAssetId);
		if (!AssetPath.IsValid())
		{
			TArray<FString> SearchPaths;
			SearchPaths.Add(TEXT("/Game"));
			AssetManager.ScanPathsForPrimaryAssets(
				StatusEffectPrimaryAssetType, SearchPaths, UGridStatusEffectDefinitionAsset::StaticClass(), false, false, true);
			AssetPath = AssetManager.GetPrimaryAssetPath(PrimaryAssetId);
		}
		if (AssetPath.IsValid())
		{
			Definition = Cast<UGridStatusEffectDefinitionAsset>(AssetPath.TryLoad());
		}
	}

	if (!IsValid(Definition) || !Definition->IsValidDefinition() || Definition->GetPrimaryAssetId() != PrimaryAssetId)
	{
		return nullptr;
	}
	return Definition;
}
