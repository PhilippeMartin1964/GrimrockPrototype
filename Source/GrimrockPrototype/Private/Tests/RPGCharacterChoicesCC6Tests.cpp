#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/Texture2D.h"
#include "RPG/RPGCharacterPortraitSetAsset.h"
#include "RPG/RPGCharacterRulesLibrary.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGClassVisualAsset.h"
#include "RPG/RPGRaceAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "UObject/SoftObjectPath.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	URPGRaceAsset* CreateCC6Race(FName RaceId, const TCHAR* DisplayName, const FRPGAttributes& Bonuses)
	{
		URPGRaceAsset* Race = NewObject<URPGRaceAsset>();
		Race->RaceId = RaceId;
		Race->DisplayName = FText::FromString(DisplayName);
		Race->AttributeBonuses = Bonuses;
		return Race;
	}

	URPGClassAsset* CreateCC6Class(FName ClassId, const TCHAR* DisplayName, const FRPGAttributes& BaseAttributes, int32 HealthAtLevelOne, int32 HealthPerLevel,
		int32 ManaAtLevelOne, int32 ManaPerLevel)
	{
		URPGClassAsset* CharacterClass = NewObject<URPGClassAsset>();
		CharacterClass->ClassId = ClassId;
		CharacterClass->DisplayName = FText::FromString(DisplayName);
		CharacterClass->BaseAttributes = BaseAttributes;
		CharacterClass->HealthAtLevelOne = HealthAtLevelOne;
		CharacterClass->HealthPerLevel = HealthPerLevel;
		CharacterClass->ManaAtLevelOne = ManaAtLevelOne;
		CharacterClass->ManaPerLevel = ManaPerLevel;
		return CharacterClass;
	}

	URPGClassVisualAsset* CreateClassVisual(FName ClassId, const TCHAR* DisplayName, const TCHAR* TexturePath)
	{
		URPGClassVisualAsset* ClassVisual = NewObject<URPGClassVisualAsset>();
		ClassVisual->ClassId = ClassId;
		ClassVisual->DisplayName = FText::FromString(DisplayName);
		ClassVisual->ClassIcon = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TexturePath));
		ClassVisual->AccentColor = FLinearColor::White;
		return ClassVisual;
	}

	FRPGCharacterPortraitVariant CreatePortraitVariant(FName VariantId, const TCHAR* DisplayName, const TCHAR* TexturePath)
	{
		FRPGCharacterPortraitVariant Variant;
		Variant.VariantId = VariantId;
		Variant.DisplayName = FText::FromString(DisplayName);
		Variant.Portrait = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TexturePath));
		Variant.Description = FText::FromString(DisplayName);
		return Variant;
	}

	URPGCharacterPortraitSetAsset* CreatePortraitSet(FName RaceId)
	{
		URPGCharacterPortraitSetAsset* PortraitSet = NewObject<URPGCharacterPortraitSetAsset>();
		PortraitSet->RaceId = RaceId;
		PortraitSet->DisplayName = FText::FromName(RaceId);
		PortraitSet->MalePortraits.Add(CreatePortraitVariant(
			TEXT("Male_01"), TEXT("Masculin 01"), TEXT("/Game/GrimrockPrototype/UI/Portraits/Races/T_Portrait_Elf_Male_01.T_Portrait_Elf_Male_01")));
		PortraitSet->FemalePortraits.Add(CreatePortraitVariant(
			TEXT("Female_01"), TEXT("Feminin 01"), TEXT("/Game/GrimrockPrototype/UI/Portraits/Races/T_Portrait_Elf_Female_01.T_Portrait_Elf_Female_01")));
		return PortraitSet;
	}

	TArray<URPGRaceAsset*> CreateCC6Races()
	{
		return { CreateCC6Race(TEXT("Human"), TEXT("Humain"), FRPGAttributes{ 1, 1, 1, 1, 1, 1 }),
			CreateCC6Race(TEXT("Dwarf"), TEXT("Nain"), FRPGAttributes{ 1, 0, 2, 0, 0, 0 }),
			CreateCC6Race(TEXT("Elf"), TEXT("Elfe"), FRPGAttributes{ 0, 2, 0, 1, 0, 0 }),
			CreateCC6Race(TEXT("Halfling"), TEXT("Halfelin"), FRPGAttributes{ 0, 2, 0, 0, 0, 1 }),
			CreateCC6Race(TEXT("Gnome"), TEXT("Gnome"), FRPGAttributes{ 0, 0, 1, 2, 0, 0 }),
			CreateCC6Race(TEXT("HalfOrc"), TEXT("Demi-orc"), FRPGAttributes{ 2, 0, 1, 0, 0, 0 }) };
	}

	TArray<URPGClassAsset*> CreateCC6Classes()
	{
		return { CreateCC6Class(TEXT("Warrior"), TEXT("Guerrier"), FRPGAttributes{ 15, 11, 13, 9, 9, 9 }, 18, 8, 0, 0),
			CreateCC6Class(TEXT("Rogue"), TEXT("Voleur"), FRPGAttributes{ 9, 15, 10, 13, 9, 10 }, 14, 6, 0, 0),
			CreateCC6Class(TEXT("Ranger"), TEXT("Rodeur"), FRPGAttributes{ 11, 15, 12, 9, 11, 8 }, 16, 7, 0, 0),
			CreateCC6Class(TEXT("Mage"), TEXT("Mage"), FRPGAttributes{ 8, 12, 10, 15, 12, 9 }, 8, 4, 18, 8),
			CreateCC6Class(TEXT("Priest"), TEXT("Pretre"), FRPGAttributes{ 10, 9, 13, 9, 15, 10 }, 12, 6, 16, 7),
			CreateCC6Class(TEXT("Alchemist"), TEXT("Alchimiste"), FRPGAttributes{ 9, 13, 12, 15, 9, 8 }, 12, 5, 10, 5) };
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGAllRaceClassCombinationsCC6Test, "Grimrock.CharacterCreation.CC6.AllRaceClassCombinations",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGAllRaceClassCombinationsCC6Test::RunTest(const FString& Parameters)
{
	const TArray<URPGRaceAsset*> Races = CreateCC6Races();
	const TArray<URPGClassAsset*> Classes = CreateCC6Classes();
	int32 ValidCombinationCount = 0;

	TestEqual(TEXT("Six prototype races are configured"), Races.Num(), 6);
	TestEqual(TEXT("Six prototype classes are configured"), Classes.Num(), 6);

	for (const URPGRaceAsset* Race : Races)
	{
		TestTrue(TEXT("Each race definition is valid"), Race && Race->IsValidDefinition());
		for (const URPGClassAsset* CharacterClass : Classes)
		{
			TestTrue(TEXT("Each class definition is valid"), CharacterClass && CharacterClass->IsValidDefinition());
			if (!Race || !CharacterClass)
			{
				continue;
			}

			const FRPGAttributes Attributes = URPGCharacterRulesLibrary::AddAttributes(CharacterClass->BaseAttributes, Race->AttributeBonuses);
			if (URPGCharacterRulesLibrary::AreAttributesInRange(Attributes))
			{
				++ValidCombinationCount;
			}
		}
	}

	TestEqual(TEXT("All 36 race and class combinations are valid"), ValidCombinationCount, 36);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGElfMageCreationCC6Test, "Grimrock.CharacterCreation.CC6.ElfMageCreation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGElfMageCreationCC6Test::RunTest(const FString& Parameters)
{
	const TArray<URPGRaceAsset*> Races = CreateCC6Races();
	const TArray<URPGClassAsset*> Classes = CreateCC6Classes();
	URPGRaceAsset* Elf = Races[2];
	URPGClassAsset* Mage = Classes[3];

	FRPGCharacterCreationRequest Request;
	Request.DisplayName = FText::FromString(TEXT("Aelwen"));
	Request.RaceDefinition = Elf;
	Request.ClassDefinition = Mage;

	UGridPartyInventoryComponent* Component = NewObject<UGridPartyInventoryComponent>();
	Component->InitializeDefaultPartyIfNeeded();

	FText Error;
	TestTrue(TEXT("An Elf Mage can be created"), Component->CreateInitialCharacter(Request, Error));
	if (!Component->HasCompletedInitialCharacterCreation())
	{
		return false;
	}

	const FGridCharacterInventoryState& Character = Component->PartyInventoryState.ActiveCharacters[0];
	TestTrue(TEXT("The race id is Elf"), Character.RaceId == FName(TEXT("Elf")));
	TestTrue(TEXT("The class id is Mage"), Character.ClassId == FName(TEXT("Mage")));
	TestEqual(TEXT("Elf Mage Strength"), Character.Attributes.Strength, 8);
	TestEqual(TEXT("Elf Mage Dexterity"), Character.Attributes.Dexterity, 14);
	TestEqual(TEXT("Elf Mage Constitution"), Character.Attributes.Constitution, 10);
	TestEqual(TEXT("Elf Mage Intelligence"), Character.Attributes.Intelligence, 16);
	TestEqual(TEXT("Elf Mage Wisdom"), Character.Attributes.Wisdom, 12);
	TestEqual(TEXT("Elf Mage Charisma"), Character.Attributes.Charisma, 9);
	TestEqual(TEXT("Elf Mage maximum health"), Character.DerivedStats.MaxHealth, 8);
	TestEqual(TEXT("Elf Mage maximum mana"), Character.DerivedStats.MaxMana, 18);
	FGridInventoryCharacterSummary Summary;
	TestTrue(TEXT("Elf Mage summary resolves"), Component->GetCharacterSummary(0, Summary));
	TestTrue(TEXT("Elf Mage maximum carry weight"), FMath::IsNearlyEqual(Summary.BaseMaxWeight, 40.0f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGPortraitSelectionPersistsCC6Test, "Grimrock.CharacterCreation.CC6.PortraitSelectionPersists",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGPortraitSelectionPersistsCC6Test::RunTest(const FString& Parameters)
{
	const TArray<URPGRaceAsset*> Races = CreateCC6Races();
	const TArray<URPGClassAsset*> Classes = CreateCC6Classes();
	const FSoftObjectPath PortraitPath(TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/Portraits/T_Portrait_ElfMage.T_Portrait_ElfMage"));

	FRPGCharacterCreationRequest Request;
	Request.DisplayName = FText::FromString(TEXT("Aelwen"));
	Request.RaceDefinition = Races[2];
	Request.ClassDefinition = Classes[3];
	Request.Portrait = TSoftObjectPtr<UTexture2D>(PortraitPath);

	UGridPartyInventoryComponent* Component = NewObject<UGridPartyInventoryComponent>();
	Component->InitializeDefaultPartyIfNeeded();

	FText Error;
	TestTrue(TEXT("A character can be created with a portrait"), Component->CreateInitialCharacter(Request, Error));
	if (!Component->HasCompletedInitialCharacterCreation())
	{
		return false;
	}

	const FGridCharacterInventoryState& Character = Component->PartyInventoryState.ActiveCharacters[0];
	TestEqual(TEXT("The portrait path is stored on the character"), Character.Portrait.ToSoftObjectPath().ToString(), PortraitPath.ToString());

	FGridInventoryCharacterSummary Summary;
	TestTrue(TEXT("The character summary can be read"), Component->GetCharacterSummary(0, Summary));
	TestEqual(TEXT("The portrait path is exposed to the inventory summary"), Summary.Portrait.ToSoftObjectPath().ToString(), PortraitPath.ToString());

	UGridPartyInventoryComponent* RestoredComponent = NewObject<UGridPartyInventoryComponent>();
	RestoredComponent->InitializeDefaultPartyIfNeeded();
	TestTrue(TEXT("The saved party inventory state restores with the portrait"),
		RestoredComponent->RestorePartyInventoryState(Component->PartyInventoryState, Error));

	FGridInventoryCharacterSummary RestoredSummary;
	TestTrue(TEXT("The restored character summary can be read"), RestoredComponent->GetCharacterSummary(0, RestoredSummary));
	TestEqual(TEXT("The restored summary keeps the portrait path"), RestoredSummary.Portrait.ToSoftObjectPath().ToString(), PortraitPath.ToString());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGPortraitSetFiltersByGenderCC6Test, "Grimrock.CharacterCreation.CC6.PortraitSetFiltersByGender",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGPortraitSetFiltersByGenderCC6Test::RunTest(const FString& Parameters)
{
	const URPGCharacterPortraitSetAsset* PortraitSet = CreatePortraitSet(TEXT("Elf"));
	TestTrue(TEXT("Portrait set is valid"), PortraitSet->IsValidDefinition());
	TestTrue(TEXT("Portrait set matches Elf"), PortraitSet->IsValidForRace(TEXT("Elf")));
	TestFalse(TEXT("Portrait set rejects Human"), PortraitSet->IsValidForRace(TEXT("Human")));

	TArray<FRPGCharacterPortraitVariant> MalePortraits;
	TArray<FRPGCharacterPortraitVariant> FemalePortraits;
	PortraitSet->GetPortraitsForGender(ERPGCharacterPortraitGender::Male, MalePortraits);
	PortraitSet->GetPortraitsForGender(ERPGCharacterPortraitGender::Female, FemalePortraits);

	TestEqual(TEXT("Male portraits are filtered"), MalePortraits.Num(), 1);
	TestEqual(TEXT("Female portraits are filtered"), FemalePortraits.Num(), 1);
	TestEqual(TEXT("Male portrait id"), MalePortraits[0].VariantId, FName(TEXT("Male_01")));
	TestEqual(TEXT("Female portrait id"), FemalePortraits[0].VariantId, FName(TEXT("Female_01")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGPortraitSetFallbackCC6Test, "Grimrock.CharacterCreation.CC6.PortraitSetFallbackByGender",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGPortraitSetFallbackCC6Test::RunTest(const FString& Parameters)
{
	const URPGCharacterPortraitSetAsset* PortraitSet = CreatePortraitSet(TEXT("Elf"));

	FRPGCharacterPortraitVariant MaleFallback;
	FRPGCharacterPortraitVariant FemaleFallback;
	TestTrue(TEXT("Male fallback is available"), PortraitSet->GetFirstValidPortrait(ERPGCharacterPortraitGender::Male, MaleFallback));
	TestTrue(TEXT("Female fallback is available"), PortraitSet->GetFirstValidPortrait(ERPGCharacterPortraitGender::Female, FemaleFallback));

	TestEqual(TEXT("Male fallback id"), MaleFallback.VariantId, FName(TEXT("Male_01")));
	TestEqual(TEXT("Female fallback id"), FemaleFallback.VariantId, FName(TEXT("Female_01")));

	FRPGCharacterPortraitVariant FoundVariant;
	TestTrue(TEXT("Female variant can be found"), PortraitSet->FindPortraitVariant(ERPGCharacterPortraitGender::Female, TEXT("Female_01"), FoundVariant));
	TestEqual(TEXT("Found female variant id"), FoundVariant.VariantId, FName(TEXT("Female_01")));
	TestFalse(
		TEXT("Female lookup rejects male variant id"), PortraitSet->FindPortraitVariant(ERPGCharacterPortraitGender::Female, TEXT("Male_01"), FoundVariant));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGClassVisualMatchesClassCC6Test, "Grimrock.CharacterCreation.CC6.ClassVisualMatchesClass",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGClassVisualMatchesClassCC6Test::RunTest(const FString& Parameters)
{
	const URPGClassVisualAsset* MageVisual =
		CreateClassVisual(TEXT("Mage"), TEXT("Mage"), TEXT("/Game/GrimrockPrototype/UI/Portraits/ClassIcons/T_ClassIcon_Mage.T_ClassIcon_Mage"));

	TestTrue(TEXT("Class visual is valid"), MageVisual->IsValidDefinition());
	TestTrue(TEXT("Class visual matches Mage"), MageVisual->IsValidForClass(TEXT("Mage")));
	TestFalse(TEXT("Class visual rejects Warrior"), MageVisual->IsValidForClass(TEXT("Warrior")));
	return true;
}

#endif
