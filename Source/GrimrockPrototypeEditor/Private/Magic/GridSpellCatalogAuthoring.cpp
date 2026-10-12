#include "Magic/GridSpellCatalogAuthoring.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "HAL/FileManager.h"
#include "Magic/GridProductionSpellLibrary.h"
#include "Magic/GridSpellDefinitionAsset.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

namespace GridSpellCatalogAuthoringPrivate
{
	constexpr int32 ProductionSpellCount = 4;
	const TCHAR* ProductionFolderPath = TEXT("/Game/GrimrockPrototype/Core/DataAssets/Magic/Spells");

	FString MakeAssetName(FName SpellId)
	{
		return FString::Printf(TEXT("DA_%s"), *SpellId.ToString());
	}

	bool EffectsEqual(
		const TArray<FGridSpellEffectDefinition>& Left,
		const TArray<FGridSpellEffectDefinition>& Right)
	{
		if (Left.Num() != Right.Num())
		{
			return false;
		}

		for (int32 Index = 0; Index < Left.Num(); ++Index)
		{
			if (Left[Index].Type != Right[Index].Type ||
				Left[Index].Magnitude != Right[Index].Magnitude ||
				Left[Index].StatusEffectId != Right[Index].StatusEffectId)
			{
				return false;
			}
		}

		return true;
	}

	UGridSpellDefinitionAsset* FindOrCreateSpellAsset(
		const FGridProductionSpellAuthoringSeed& Seed,
		FString& OutError)
	{
		const FString AssetName = MakeAssetName(Seed.Definition.SpellId);
		const FString PackageName = FString::Printf(
			TEXT("%s/%s"),
			ProductionFolderPath,
			*AssetName);
		const FString ObjectPath = FString::Printf(TEXT("%s.%s"), *PackageName, *AssetName);

		if (UGridSpellDefinitionAsset* Existing =
				LoadObject<UGridSpellDefinitionAsset>(nullptr, *ObjectPath))
		{
			return Existing;
		}

		UPackage* Package = CreatePackage(*PackageName);
		if (!Package)
		{
			OutError = FString::Printf(
				TEXT("Impossible de créer le package Spell '%s'."),
				*PackageName);
			return nullptr;
		}

		UGridSpellDefinitionAsset* Created = NewObject<UGridSpellDefinitionAsset>(
			Package,
			FName(*AssetName),
			RF_Public | RF_Standalone);
		if (!Created)
		{
			OutError = FString::Printf(
				TEXT("Impossible de créer l'asset Spell '%s'."),
				*ObjectPath);
			return nullptr;
		}

		FAssetRegistryModule::AssetCreated(Created);
		return Created;
	}

	bool SaveSpellAsset(UGridSpellDefinitionAsset& SpellAsset, FString& OutError)
	{
		UPackage* Package = SpellAsset.GetOutermost();
		if (!Package)
		{
			OutError = FString::Printf(
				TEXT("Le Spell '%s' ne possède aucun package."),
				*GetNameSafe(&SpellAsset));
			return false;
		}

		Package->MarkPackageDirty();
		const FString Filename = FPackageName::LongPackageNameToFilename(
			Package->GetName(),
			FPackageName::GetAssetPackageExtension());
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true);

		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		SaveArgs.SaveFlags = SAVE_NoError;
		if (!UPackage::SavePackage(Package, &SpellAsset, *Filename, SaveArgs))
		{
			OutError = FString::Printf(
				TEXT("Impossible de sauvegarder le Spell '%s'."),
				*Filename);
			return false;
		}

		return true;
	}
}

const TCHAR* FGridSpellCatalogAuthoring::ProductionFolder()
{
	return GridSpellCatalogAuthoringPrivate::ProductionFolderPath;
}

bool FGridSpellCatalogAuthoring::BuildProductionSeeds(
	TArray<FGridProductionSpellAuthoringSeed>& OutSeeds,
	FString& OutError)
{
	OutSeeds.Reset();
	OutError.Reset();

	TArray<FGridSpellDefinition> Definitions;
	FGridProductionSpellLibrary::BuildAll(Definitions);
	if (Definitions.Num() != GridSpellCatalogAuthoringPrivate::ProductionSpellCount)
	{
		OutError = FString::Printf(
			TEXT("Le catalogue Spell C++ doit contenir exactement %d définitions, mais en contient %d."),
			GridSpellCatalogAuthoringPrivate::ProductionSpellCount,
			Definitions.Num());
		return false;
	}

	TSet<FName> SeenSpellIds;
	for (const FGridSpellDefinition& Definition : Definitions)
	{
		if (FGridSpellContract::ValidateDefinition(Definition) !=
				EGridSpellValidationError::None ||
			SeenSpellIds.Contains(Definition.SpellId))
		{
			OutError = FString::Printf(
				TEXT("Définition Spell C++ invalide ou dupliquée : '%s'."),
				*Definition.SpellId.ToString());
			return false;
		}

		FGridSpellPresentationProfile Presentation;
		if (!FGridProductionSpellLibrary::TryBuildPresentationProfile(
				Definition.SpellId,
				Presentation) ||
			!Presentation.IsValid())
		{
			OutError = FString::Printf(
				TEXT("Profil de présentation Spell invalide ou absent : '%s'."),
				*Definition.SpellId.ToString());
			return false;
		}

		FGridProductionSpellAuthoringSeed Seed;
		Seed.Definition = Definition;
		Seed.Presentation = Presentation;
		OutSeeds.Add(MoveTemp(Seed));
		SeenSpellIds.Add(Definition.SpellId);
	}

	return true;
}

bool FGridSpellCatalogAuthoring::ConfigureSpellAsset(
	UGridSpellDefinitionAsset& SpellAsset,
	const FGridProductionSpellAuthoringSeed& Seed,
	FString& OutError)
{
	OutError.Reset();
	if (FGridSpellContract::ValidateDefinition(Seed.Definition) !=
			EGridSpellValidationError::None ||
		!Seed.Presentation.IsValid())
	{
		OutError = FString::Printf(
			TEXT("Seed Spell invalide : '%s'."),
			*Seed.Definition.SpellId.ToString());
		return false;
	}

	const bool bPreserveAuthoredPresentation =
		!SpellAsset.Definition.SpellId.IsNone() &&
		SpellAsset.Definition.SpellId == Seed.Definition.SpellId;

	const FText ExistingDisplayName = SpellAsset.Definition.DisplayName;
	const FText ExistingDescription = SpellAsset.Definition.Description;
	const FGridSpellPresentationProfile ExistingPresentation = SpellAsset.Presentation;

	SpellAsset.Definition = Seed.Definition;
	SpellAsset.Presentation = Seed.Presentation;

	if (bPreserveAuthoredPresentation)
	{
		SpellAsset.Definition.DisplayName = ExistingDisplayName;
		SpellAsset.Definition.Description = ExistingDescription;
		SpellAsset.Presentation = ExistingPresentation;
	}

	const EGridSpellValidationError ValidationError = SpellAsset.ValidateDefinition();
	if (ValidationError != EGridSpellValidationError::None)
	{
		OutError = FString::Printf(
			TEXT("Le Spell matérialisé '%s' est invalide après préservation (erreur %d)."),
			*Seed.Definition.SpellId.ToString(),
			static_cast<int32>(ValidationError));
		return false;
	}

	if (!SpellAsset.HasValidPresentation())
	{
		OutError = FString::Printf(
			TEXT("La présentation matérialisée '%s' est invalide après préservation."),
			*Seed.Definition.SpellId.ToString());
		return false;
	}

	return true;
}

bool FGridSpellCatalogAuthoring::IsCanonicalMechanics(
	const UGridSpellDefinitionAsset& SpellAsset,
	const FGridProductionSpellAuthoringSeed& Seed)
{
	const FGridSpellDefinition& Actual = SpellAsset.Definition;
	const FGridSpellDefinition& Expected = Seed.Definition;

	return Actual.SpellId == Expected.SpellId &&
		Actual.School == Expected.School &&
		Actual.ManaCost == Expected.ManaCost &&
		Actual.ActionPointCost == Expected.ActionPointCost &&
		Actual.MinRangeCells == Expected.MinRangeCells &&
		Actual.MaxRangeCells == Expected.MaxRangeCells &&
		Actual.TargetingPolicy == Expected.TargetingPolicy &&
		Actual.bRequiresLineOfSight == Expected.bRequiresLineOfSight &&
		Actual.CooldownRounds == Expected.CooldownRounds &&
		GridSpellCatalogAuthoringPrivate::EffectsEqual(
			Actual.Effects,
			Expected.Effects);
}

bool FGridSpellCatalogAuthoring::AuthorProductionAssets(FString& OutError)
{
	OutError.Reset();

	TArray<FGridProductionSpellAuthoringSeed> Seeds;
	if (!BuildProductionSeeds(Seeds, OutError))
	{
		return false;
	}

	for (const FGridProductionSpellAuthoringSeed& Seed : Seeds)
	{
		UGridSpellDefinitionAsset* SpellAsset =
			GridSpellCatalogAuthoringPrivate::FindOrCreateSpellAsset(Seed, OutError);
		if (!IsValid(SpellAsset))
		{
			return false;
		}

		SpellAsset->Modify();
		if (!ConfigureSpellAsset(*SpellAsset, Seed, OutError) ||
			!IsCanonicalMechanics(*SpellAsset, Seed))
		{
			if (OutError.IsEmpty())
			{
				OutError = FString::Printf(
					TEXT("La mécanique Spell matérialisée '%s' n'est pas canonique."),
					*Seed.Definition.SpellId.ToString());
			}
			return false;
		}

		if (!GridSpellCatalogAuthoringPrivate::SaveSpellAsset(*SpellAsset, OutError))
		{
			return false;
		}
	}

	return true;
}
