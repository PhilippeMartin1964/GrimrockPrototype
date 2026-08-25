#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridObjectArchetypeAsset.h"
#include "Core/GridObjectPaletteAsset.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMON2045StoryCompanionPaletteContractTest, "Grimrock.MON20.4.RecruitmentUI.PaletteContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON2045StoryCompanionPaletteContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UScriptStruct* EntryStruct = FGridObjectPaletteEntry::StaticStruct();
	TestNotNull(TEXT("Grid palette entry struct exists"), EntryStruct);
	if (EntryStruct)
	{
		TestNotNull(TEXT("Story companion default is part of the palette entry contract"),
			EntryStruct->FindPropertyByName(GET_MEMBER_NAME_CHECKED(FGridObjectPaletteEntry, DefaultStoryCompanionDefinition)));
	}

	UGridObjectPaletteAsset* Palette = NewObject<UGridObjectPaletteAsset>(GetTransientPackage());
	UGridObjectArchetypeAsset* TriggerArchetype = NewObject<UGridObjectArchetypeAsset>(Palette);
	TriggerArchetype->ArchetypeId = TEXT("Trigger_Test");
	TriggerArchetype->SupportedType = EGridLevelObjectType::Trigger;

	FGridObjectPaletteEntry TriggerEntry;
	TriggerEntry.EntryId = TEXT("Trigger_Test");
	TriggerEntry.DefaultArchetype = TriggerArchetype;
	Palette->Entries.Add(TriggerEntry);

	TArray<FGridArchetypeValidationMessage> Messages;
	TestTrue(TEXT("Non-story palette entries do not require a companion definition"), Palette->ValidatePalette(Messages));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMON2045StoryCompanionPaletteMissingDefinitionTest, "Grimrock.MON20.4.RecruitmentUI.PaletteMissingDefinition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON2045StoryCompanionPaletteMissingDefinitionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGridObjectPaletteAsset* Palette = NewObject<UGridObjectPaletteAsset>(GetTransientPackage());
	UGridObjectArchetypeAsset* CompanionArchetype = NewObject<UGridObjectArchetypeAsset>(Palette);
	CompanionArchetype->ArchetypeId = TEXT("StoryCompanion_Test");
	CompanionArchetype->SupportedType = EGridLevelObjectType::StoryCompanion;

	FGridObjectPaletteEntry CompanionEntry;
	CompanionEntry.EntryId = TEXT("StoryCompanion_Test");
	CompanionEntry.DefaultArchetype = CompanionArchetype;
	CompanionEntry.DefaultStoryCompanionDefinition = nullptr;
	Palette->Entries.Add(CompanionEntry);

	TArray<FGridArchetypeValidationMessage> Messages;
	TestFalse(TEXT("Story companion palette entry rejects a missing default definition"), Palette->ValidatePalette(Messages));

	bool bFoundExpectedError = false;
	for (const FGridArchetypeValidationMessage& Message : Messages)
	{
		if (Message.Severity == EGridArchetypeValidationSeverity::Error && Message.Message.Contains(TEXT("DefaultStoryCompanionDefinition")))
		{
			bFoundExpectedError = true;
			break;
		}
	}
	TestTrue(TEXT("Missing story companion definition is reported explicitly"), bFoundExpectedError);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
