#include "Magic/GridProductionSpellLibrary.h"

#include "Magic/GridSpellDefinitionAsset.h"
#include "UObject/UObjectGlobals.h"

namespace GridProductionSpellLibraryPrivate
{
	struct FProductionSpellAssetSpec
	{
		FName SpellId;
		const TCHAR* ObjectPath;
	};

	const FProductionSpellAssetSpec ProductionSpells[] = {
		{ FGridProductionSpellLibrary::ArcaneBoltId(),
		  TEXT("/Game/GrimrockPrototype/Core/DataAssets/Magic/Spells/DA_Spell_ArcaneBolt.DA_Spell_ArcaneBolt") },
		{ FGridProductionSpellLibrary::LesserHealId(),
		  TEXT("/Game/GrimrockPrototype/Core/DataAssets/Magic/Spells/DA_Spell_LesserHeal.DA_Spell_LesserHeal") },
		{ FGridProductionSpellLibrary::HasteId(),
		  TEXT("/Game/GrimrockPrototype/Core/DataAssets/Magic/Spells/DA_Spell_Haste.DA_Spell_Haste") },
		{ FGridProductionSpellLibrary::CurePoisonId(),
		  TEXT("/Game/GrimrockPrototype/Core/DataAssets/Magic/Spells/DA_Spell_CurePoison.DA_Spell_CurePoison") }
	};

	const FProductionSpellAssetSpec* FindSpec(FName SpellId)
	{
		for (const FProductionSpellAssetSpec& Spec : ProductionSpells)
		{
			if (Spec.SpellId == SpellId)
			{
				return &Spec;
			}
		}
		return nullptr;
	}

	const UGridSpellDefinitionAsset* LoadValidatedAsset(const FProductionSpellAssetSpec& Spec)
	{
		const UGridSpellDefinitionAsset* Asset =
			LoadObject<UGridSpellDefinitionAsset>(nullptr, Spec.ObjectPath);
		if (!IsValid(Asset))
		{
			UE_LOG(LogTemp, Error, TEXT("Production Spell asset missing: %s (%s)."),
				*Spec.SpellId.ToString(), Spec.ObjectPath);
			return nullptr;
		}

		if (Asset->Definition.SpellId != Spec.SpellId)
		{
			UE_LOG(LogTemp, Error,
				TEXT("Production Spell asset '%s' has mismatched SpellId '%s' (expected '%s')."),
				Spec.ObjectPath,
				*Asset->Definition.SpellId.ToString(),
				*Spec.SpellId.ToString());
			return nullptr;
		}

		if (Asset->ValidateDefinition() != EGridSpellValidationError::None)
		{
			UE_LOG(LogTemp, Error, TEXT("Production Spell asset '%s' has an invalid Definition."), Spec.ObjectPath);
			return nullptr;
		}

		if (!Asset->HasValidPresentation())
		{
			UE_LOG(LogTemp, Error, TEXT("Production Spell asset '%s' has an invalid Presentation."), Spec.ObjectPath);
			return nullptr;
		}

		return Asset;
	}

	const UGridSpellDefinitionAsset* LoadValidatedAsset(FName SpellId)
	{
		const FProductionSpellAssetSpec* Spec = FindSpec(SpellId);
		return Spec ? LoadValidatedAsset(*Spec) : nullptr;
	}

	FGridSpellDefinition LoadDefinitionOrTechnicalFallback(FName SpellId)
	{
		if (const UGridSpellDefinitionAsset* Asset = LoadValidatedAsset(SpellId))
		{
			return Asset->Definition;
		}

		FGridSpellDefinition Fallback;
		Fallback.SpellId = SpellId;
		return Fallback;
	}

	FGridSpellPresentationProfile LoadPresentationOrEmpty(FName SpellId)
	{
		if (const UGridSpellDefinitionAsset* Asset = LoadValidatedAsset(SpellId))
		{
			return Asset->Presentation;
		}
		return FGridSpellPresentationProfile();
	}
}

FGridSpellDefinition FGridProductionSpellLibrary::MakeArcaneBolt()
{
	return GridProductionSpellLibraryPrivate::LoadDefinitionOrTechnicalFallback(ArcaneBoltId());
}

FGridSpellDefinition FGridProductionSpellLibrary::MakeLesserHeal()
{
	return GridProductionSpellLibraryPrivate::LoadDefinitionOrTechnicalFallback(LesserHealId());
}

FGridSpellDefinition FGridProductionSpellLibrary::MakeHaste()
{
	return GridProductionSpellLibraryPrivate::LoadDefinitionOrTechnicalFallback(HasteId());
}

FGridSpellDefinition FGridProductionSpellLibrary::MakeCurePoison()
{
	return GridProductionSpellLibraryPrivate::LoadDefinitionOrTechnicalFallback(CurePoisonId());
}

FGridSpellPresentationProfile FGridProductionSpellLibrary::MakeArcaneBoltPresentation()
{
	return GridProductionSpellLibraryPrivate::LoadPresentationOrEmpty(ArcaneBoltId());
}

FGridSpellPresentationProfile FGridProductionSpellLibrary::MakeLesserHealPresentation()
{
	return GridProductionSpellLibraryPrivate::LoadPresentationOrEmpty(LesserHealId());
}

FGridSpellPresentationProfile FGridProductionSpellLibrary::MakeHastePresentation()
{
	return GridProductionSpellLibraryPrivate::LoadPresentationOrEmpty(HasteId());
}

FGridSpellPresentationProfile FGridProductionSpellLibrary::MakeCurePoisonPresentation()
{
	return GridProductionSpellLibraryPrivate::LoadPresentationOrEmpty(CurePoisonId());
}

bool FGridProductionSpellLibrary::TryBuildPresentationProfile(
	FName SpellId,
	FGridSpellPresentationProfile& OutProfile)
{
	OutProfile = FGridSpellPresentationProfile();

	const UGridSpellDefinitionAsset* Asset =
		GridProductionSpellLibraryPrivate::LoadValidatedAsset(SpellId);
	if (!Asset)
	{
		return false;
	}

	OutProfile = Asset->Presentation;
	return true;
}

void FGridProductionSpellLibrary::BuildAll(TArray<FGridSpellDefinition>& OutDefinitions)
{
	OutDefinitions.Reset();
	OutDefinitions.Reserve(UE_ARRAY_COUNT(GridProductionSpellLibraryPrivate::ProductionSpells));

	for (const GridProductionSpellLibraryPrivate::FProductionSpellAssetSpec& Spec :
		GridProductionSpellLibraryPrivate::ProductionSpells)
	{
		if (const UGridSpellDefinitionAsset* Asset =
				GridProductionSpellLibraryPrivate::LoadValidatedAsset(Spec))
		{
			OutDefinitions.Add(Asset->Definition);
		}
	}
}
