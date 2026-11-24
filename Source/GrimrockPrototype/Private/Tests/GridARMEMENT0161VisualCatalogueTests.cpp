#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Runtime/GridInventoryTypes.h"
#include "Runtime/Items/GridItemCatalogues.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"
#include <limits>

namespace ARMEMENT0161
{
	TStrongObjectPtr<UGridCanonicalWeaponVisualCatalogueAsset> MakeActivePilot()
	{
		TStrongObjectPtr<UGridCanonicalWeaponVisualCatalogueAsset> Catalogue(NewObject<UGridCanonicalWeaponVisualCatalogueAsset>());
		for (const FName Id : { FName(TEXT("VIS_Dagger_A")), FName(TEXT("VIS_Bow_A")), FName(TEXT("VIS_Shield_A")) })
		{
			FGridWeaponVisual& Profile = Catalogue->VisualProfiles.AddDefaulted_GetRef();
			Profile.VisualId = Id;
			Profile.ItemIcon = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/Fixture/T_Icon.T_Icon")));
			Profile.WorldMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Game/Fixture/SM_World.SM_World")));
			Profile.EquippedMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Game/Fixture/SM_Equipped.SM_Equipped")));
			Profile.WorldTransform.SetTranslation(FVector(3.0, -7.0, 11.0));
		}
		for (const auto& Row : { TPair<FName, FName>{ TEXT("ARM001"), TEXT("VIS_Dagger_A") },
			{ TEXT("ARM002"), TEXT("VIS_Dagger_A") }, { TEXT("ARM045"), TEXT("VIS_Bow_A") },
			{ TEXT("ARM075"), TEXT("VIS_Shield_A") } })
		{
			FGridWeaponVisualBinding& Binding = Catalogue->ArchetypeBindings.AddDefaulted_GetRef();
			Binding.ArchetypeId = Row.Key;
			Binding.VisualId = Row.Value;
		}
		return Catalogue;
	}

}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0161SchemaTest,
	"Grimrock.RPG.ARMEMENT01.6.VisualCatalogueSchema",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0161SchemaTest::RunTest(const FString&)
{
	const auto Catalogue = ARMEMENT0161::MakeActivePilot();
	TArray<FString> Errors;
	TestTrue(TEXT("Partial visual catalogue is structurally valid"), Catalogue->Validate(Errors));
	TestEqual(TEXT("Four bindings cover three silhouettes"), Catalogue->ArchetypeBindings.Num(), 4);
	TestEqual(TEXT("Three authored visual records"), Catalogue->VisualProfiles.Num(), 3);
	const FGridWeaponVisual* First = Catalogue->FindForArchetype(TEXT("ARM001"));
	const FGridWeaponVisual* Shield = Catalogue->FindForArchetype(TEXT("ARM075"));
	if (!TestNotNull(TEXT("Dagger visual"), First) || !TestNotNull(TEXT("Shield visual"), Shield))
		return false;
	TestEqual(TEXT("ARM001 maps to dagger"), First->VisualId, FName(TEXT("VIS_Dagger_A")));
	TestEqual(TEXT("ARM075 maps to shield"), Shield->VisualId, FName(TEXT("VIS_Shield_A")));
	TestTrue(TEXT("Two archetypes share one authored profile"), First == Catalogue->FindForArchetype(TEXT("ARM002")));
	TestNull(TEXT("Unmapped identity receives no other silhouette"), Catalogue->FindForArchetype(TEXT("ARM003")));
	TestEqual(TEXT("Independent icon path"), First->ItemIcon.ToSoftObjectPath().ToString(), FString(TEXT("/Game/Fixture/T_Icon.T_Icon")));
	TestEqual(TEXT("Independent world mesh path"), First->WorldMesh.ToSoftObjectPath().ToString(), FString(TEXT("/Game/Fixture/SM_World.SM_World")));
	TestEqual(TEXT("Independent equipped mesh path"), First->EquippedMesh.ToSoftObjectPath().ToString(), FString(TEXT("/Game/Fixture/SM_Equipped.SM_Equipped")));
	TestEqual(TEXT("Independent authored offset"), First->WorldTransform.GetTranslation(), FVector(3.0, -7.0, 11.0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0161DuplicateTest,
	"Grimrock.RPG.ARMEMENT01.6.UniqueBindings",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0161DuplicateTest::RunTest(const FString&)
{
	const auto CheckInvalid = [this](const TCHAR* Label, const UGridCanonicalWeaponVisualCatalogueAsset& Catalogue, const TCHAR* Diagnostic)
	{
		TArray<FString> Errors;
		TestFalse(Label, Catalogue.Validate(Errors));
		TestTrue(TEXT("Specific validation diagnostic"), Errors.Contains(Diagnostic));
	};

	auto Catalogue = ARMEMENT0161::MakeActivePilot();
	FGridWeaponVisualBinding Duplicate = Catalogue->ArchetypeBindings[0];
	Duplicate.VisualId = TEXT("VIS_Bow_A");
	Catalogue->ArchetypeBindings.Add(Duplicate);
	CheckInvalid(TEXT("Conflicting duplicate rejected"), *Catalogue, TEXT("Visual binding invalid: ARM001"));

	Duplicate.VisualId = TEXT("VIS_Dagger_A");
	Catalogue->ArchetypeBindings.Last() = Duplicate;
	CheckInvalid(TEXT("Identical duplicate rejected"), *Catalogue, TEXT("Visual binding invalid: ARM001"));

	Catalogue = ARMEMENT0161::MakeActivePilot();
	const FGridWeaponVisual DuplicateVisual = Catalogue->VisualProfiles[0];
	Catalogue->VisualProfiles.Add(DuplicateVisual);
	CheckInvalid(TEXT("Duplicate visual identity rejected"), *Catalogue, TEXT("Visual catalogue: missing/duplicate VisualId"));

	Catalogue = ARMEMENT0161::MakeActivePilot();
	Catalogue->ArchetypeBindings[0].VisualId = TEXT("VIS_Unknown");
	CheckInvalid(TEXT("Unknown visual binding rejected"), *Catalogue, TEXT("Visual binding invalid: ARM001"));
	TestNull(TEXT("Invalid binding cannot borrow any visual"), Catalogue->FindForArchetype(TEXT("ARM001")));

	Catalogue->ArchetypeBindings.RemoveAt(0);
	TArray<FString> Errors;
	TestTrue(TEXT("Remaining partial mapping is valid"), Catalogue->Validate(Errors));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0161VisualProfileTest,
	"Grimrock.RPG.ARMEMENT01.6.VisualProfileValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0161VisualProfileTest::RunTest(const FString&)
{
	using namespace ARMEMENT0161;
	TestTrue(TEXT("Canonical first archetype"), FGridWeaponArchetype::IsValidArchetypeId(TEXT("ARM001")));
	TestTrue(TEXT("Canonical last archetype"), FGridWeaponArchetype::IsValidArchetypeId(TEXT("ARM082")));
	TestFalse(TEXT("ARM000 is invalid"), FGridWeaponArchetype::IsValidArchetypeId(TEXT("ARM000")));
	TestFalse(TEXT("ARM083 is invalid"), FGridWeaponArchetype::IsValidArchetypeId(TEXT("ARM083")));
	TestFalse(TEXT("Malformed identity rejected"), FGridWeaponArchetype::IsValidArchetypeId(TEXT("Archetype_1")));

    auto Catalogue = MakeActivePilot();
    auto& Profile = Catalogue->VisualProfiles[0];
    Profile.ProjectileAppearance.bEnabled = true;
    TArray<FString> Errors;
    TestFalse(TEXT("Enabled projectile appearance needs a mesh"), Catalogue->Validate(Errors));
    Profile.ProjectileMesh = Profile.WorldMesh;
    Errors.Reset();
    TestTrue(TEXT("Complete projectile appearance is valid"), Catalogue->Validate(Errors));
    Profile.ProjectileAppearance.TumbleDegreesPerSecond = std::numeric_limits<float>::infinity();
    Errors.Reset();
    TestFalse(TEXT("Nonfinite projectile spin rejected"), Catalogue->Validate(Errors));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0161ItemOptInTest,
	"Grimrock.RPG.ARMEMENT01.6.ItemVisualOptIn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0161ItemOptInTest::RunTest(const FString&)
{
    auto Catalogue = ARMEMENT0161::MakeActivePilot();
    TestNotNull(TEXT("A valid archetype uses its bound presentation"), Catalogue->FindForArchetype(TEXT("ARM001")));
    TestNull(TEXT("A valid but unbound archetype has no presentation"), Catalogue->FindForArchetype(TEXT("ARM003")));
    TestNull(TEXT("An unknown archetype has no presentation"), Catalogue->FindForArchetype(TEXT("ARM099")));
    Catalogue->ArchetypeBindings[0].VisualId = TEXT("VIS_Missing");
    TestNull(TEXT("Missing visual profile does not fall back"), Catalogue->FindForArchetype(TEXT("ARM001")));
    TArray<FString> Errors;
    TestFalse(TEXT("A missing visual profile invalidates the catalogue"), Catalogue->Validate(Errors));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0161NoGameplayAuthorityTest,
	"Grimrock.RPG.ARMEMENT01.6.VisualOnlyNoGameplayAuthority",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0161NoGameplayAuthorityTest::RunTest(const FString&)
{
	UScriptStruct* ProfileType = FGridWeaponVisual::StaticStruct();
	TestNotNull(TEXT("Visual profile is a reflected struct"), ProfileType);
	if (!ProfileType) { return false; }

	TestNull(TEXT("Visual profile has no dice authority"), ProfileType->FindPropertyByName(TEXT("DamageDice")));
	TestNull(TEXT("Visual profile has no item-level authority"), ProfileType->FindPropertyByName(TEXT("ItemLevel")));
	TestNull(TEXT("Visual profile has no rarity authority"), ProfileType->FindPropertyByName(TEXT("Rarity")));
	TestNull(TEXT("Visual profile has no forging authority"), ProfileType->FindPropertyByName(TEXT("ForgeRank")));
	TestNull(TEXT("Visual profile has no material override"), ProfileType->FindPropertyByName(TEXT("MaterialOverride")));

	TestNotNull(TEXT("Presentation owns the item icon"), ProfileType->FindPropertyByName(TEXT("ItemIcon")));
	const FProperty* ItemLevel = FGridItemInstance::StaticStruct()->FindPropertyByName(TEXT("ItemLevel"));
	TestNotNull(TEXT("Instance keeps ARMEMENT01.5's level field"), ItemLevel);
	TestNull(TEXT("Instance does not persist presentation VisualId"),
		FGridItemInstance::StaticStruct()->FindPropertyByName(TEXT("VisualId")));
	TestNull(TEXT("Instance does not persist guessed rarity"),
		FGridItemInstance::StaticStruct()->FindPropertyByName(TEXT("Rarity")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
