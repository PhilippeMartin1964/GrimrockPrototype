#include "RPG/StatusEffects/GridStatusEffectPersistence.h"

#include "Engine/AssetManager.h"
#include "RPG/StatusEffects/GridStatusEffectDefinitionAsset.h"
#include "Runtime/GridInventoryTypes.h"

namespace GridStatusEffectPersistencePrivate
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

	bool ValidateDefinitionForState(
		const FGridStatusEffectRuntimeState& State, const UGridStatusEffectDefinitionAsset* Definition, FString& OutError)
	{
		if (!IsValid(Definition) || !Definition->IsValidDefinition() || Definition->EffectId != State.EffectId)
		{
			OutError = FString::Printf(TEXT("Status effect '%s' cannot resolve its matching valid definition."), *State.EffectId.ToString());
			return false;
		}
		if (Definition->DurationUnit != State.DurationUnit)
		{
			OutError = FString::Printf(TEXT("Status effect '%s' duration unit no longer matches its definition."), *State.EffectId.ToString());
			return false;
		}
		if (State.StackCount > Definition->MaxStacks)
		{
			OutError = FString::Printf(TEXT("Status effect '%s' stack count %d exceeds definition MaxStacks %d."), *State.EffectId.ToString(),
				State.StackCount, Definition->MaxStacks);
			return false;
		}
		return true;
	}

	bool RehydrateCollection(FGridStatusEffectCollection& Collection,
		TFunctionRef<UGridStatusEffectDefinitionAsset*(FName)> DefinitionResolver, FString& OutError)
	{
		if (!FGridStatusEffectPersistence::ValidateDurableCollection(Collection, OutError))
		{
			return false;
		}

		FGridStatusEffectCollection Candidate;
		Candidate.ActiveEffects.Reserve(Collection.ActiveEffects.Num());
		for (const FGridStatusEffectRuntimeState& StableState : Collection.ActiveEffects)
		{
			UGridStatusEffectDefinitionAsset* Definition = DefinitionResolver(StableState.EffectId);
			if (!ValidateDefinitionForState(StableState, Definition, OutError))
			{
				return false;
			}

			FGridStatusEffectRuntimeState RehydratedState;
			FString RehydrateError;
			if (!Definition->BuildRuntimeState(StableState.SourceId, StableState.StackCount, StableState.RemainingDuration, StableState.Potency,
					RehydratedState, RehydrateError))
			{
				OutError = FString::Printf(
					TEXT("Status effect '%s' failed runtime rehydration: %s"), *StableState.EffectId.ToString(), *RehydrateError);
				return false;
			}
			Candidate.ActiveEffects.Add(MoveTemp(RehydratedState));
		}

		Candidate.ActiveEffects.Sort(&RuntimeStateLess);
		Collection = MoveTemp(Candidate);
		OutError.Reset();
		return true;
	}

	bool ValidatePartyCharacters(const TArray<FGridCharacterInventoryState>& Characters, const TCHAR* Location,
		TSet<FGuid>& InOutCharacterIds, bool bRequireRuntimeDefinitions, FString& OutError)
	{
		for (int32 CharacterIndex = 0; CharacterIndex < Characters.Num(); ++CharacterIndex)
		{
			const FGridCharacterInventoryState& Character = Characters[CharacterIndex];
			if (!Character.CharacterId.IsValid() || InOutCharacterIds.Contains(Character.CharacterId))
			{
				OutError = FString::Printf(TEXT("%s[%d] has an invalid or duplicated CharacterId."), Location, CharacterIndex);
				return false;
			}
			InOutCharacterIds.Add(Character.CharacterId);

			const bool bValid = bRequireRuntimeDefinitions
				? FGridStatusEffectPersistence::ValidateRuntimeCollection(Character.StatusEffects, OutError)
				: FGridStatusEffectPersistence::ValidateDurableCollection(Character.StatusEffects, OutError);
			if (!bValid)
			{
				OutError = FString::Printf(TEXT("%s[%d] StatusEffects invalid: %s"), Location, CharacterIndex, *OutError);
				return false;
			}
		}
		return true;
	}
}

using namespace GridStatusEffectPersistencePrivate;

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

bool FGridStatusEffectPersistence::ValidateDurableCollection(const FGridStatusEffectCollection& Collection, FString& OutError)
{
	TSet<FName> EffectIds;
	for (const FGridStatusEffectRuntimeState& State : Collection.ActiveEffects)
	{
		if (!State.IsValid())
		{
			OutError = FString::Printf(TEXT("Durable status effect '%s' has an invalid stable state."), *State.EffectId.ToString());
			return false;
		}
		if (EffectIds.Contains(State.EffectId))
		{
			OutError = FString::Printf(TEXT("Durable status effect '%s' is duplicated."), *State.EffectId.ToString());
			return false;
		}
		EffectIds.Add(State.EffectId);
	}

	OutError.Reset();
	return true;
}

bool FGridStatusEffectPersistence::ValidateRuntimeCollection(const FGridStatusEffectCollection& Collection, FString& OutError)
{
	if (!ValidateDurableCollection(Collection, OutError))
	{
		return false;
	}

	for (const FGridStatusEffectRuntimeState& State : Collection.ActiveEffects)
	{
		if (!ValidateDefinitionForState(State, State.DefinitionAsset.Get(), OutError))
		{
			return false;
		}
	}

	OutError.Reset();
	return true;
}

bool FGridStatusEffectPersistence::ValidatePartyStatusEffects(const FGridPartyInventoryState& PartyState, FString& OutError)
{
	TSet<FGuid> CharacterIds;
	if (!ValidatePartyCharacters(PartyState.ActiveCharacters, TEXT("ActiveCharacter"), CharacterIds, false, OutError) ||
		!ValidatePartyCharacters(PartyState.CharacterPool, TEXT("CharacterPool"), CharacterIds, false, OutError))
	{
		return false;
	}

	OutError.Reset();
	return true;
}

bool FGridStatusEffectPersistence::ValidateRuntimePartyStatusEffects(const FGridPartyInventoryState& PartyState, FString& OutError)
{
	TSet<FGuid> CharacterIds;
	if (!ValidatePartyCharacters(PartyState.ActiveCharacters, TEXT("ActiveCharacter"), CharacterIds, true, OutError) ||
		!ValidatePartyCharacters(PartyState.CharacterPool, TEXT("CharacterPool"), CharacterIds, true, OutError))
	{
		return false;
	}

	OutError.Reset();
	return true;
}

bool FGridStatusEffectPersistence::RehydratePartyStatusEffects(FGridPartyInventoryState& PartyState,
	TFunctionRef<UGridStatusEffectDefinitionAsset*(FName)> DefinitionResolver, FString& OutError)
{
	if (!ValidatePartyStatusEffects(PartyState, OutError))
	{
		return false;
	}

	FGridPartyInventoryState Candidate = PartyState;
	const auto RehydrateCharacters = [&DefinitionResolver, &OutError](TArray<FGridCharacterInventoryState>& Characters) -> bool
	{
		for (FGridCharacterInventoryState& Character : Characters)
		{
			if (!GridStatusEffectPersistencePrivate::RehydrateCollection(Character.StatusEffects, DefinitionResolver, OutError))
			{
				return false;
			}
		}
		return true;
	};

	if (!RehydrateCharacters(Candidate.ActiveCharacters) || !RehydrateCharacters(Candidate.CharacterPool))
	{
		return false;
	}

	PartyState = MoveTemp(Candidate);
	OutError.Reset();
	return true;
}

bool FGridStatusEffectPersistence::RehydratePartyStatusEffects(FGridPartyInventoryState& PartyState, FString& OutError)
{
	return RehydratePartyStatusEffects(
		PartyState,
		[](FName EffectId)
		{
			return FGridStatusEffectPersistence::ResolveDefinitionByEffectId(EffectId);
		},
		OutError);
}

bool FGridStatusEffectPersistence::CaptureCollection(
	const FGridStatusEffectCollection& RuntimeCollection, TArray<FGridStatusEffectSaveState>& OutSavedStates, FString& OutError)
{
	if (!ValidateRuntimeCollection(RuntimeCollection, OutError))
	{
		return false;
	}

	TArray<FGridStatusEffectSaveState> Candidate;
	Candidate.Reserve(RuntimeCollection.ActiveEffects.Num());
	for (const FGridStatusEffectRuntimeState& RuntimeState : RuntimeCollection.ActiveEffects)
	{
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
		FGridStatusEffectRuntimeState StableState;
		StableState.EffectId = SavedState.EffectId;
		StableState.SourceId = SavedState.SourceId;
		StableState.StackCount = SavedState.StackCount;
		StableState.DurationUnit = SavedState.DurationUnit;
		StableState.RemainingDuration = SavedState.RemainingDuration;
		StableState.Potency = SavedState.Potency;
		if (!ValidateDefinitionForState(StableState, Definition, OutError))
		{
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
