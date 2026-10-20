#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Runtime/Combat/GridWeaponVisualCatalogueAsset.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridInventoryTypes.h"
#include "UObject/UnrealType.h"

namespace ARMEMENT0161
{
	FGridWeaponVisualProfile MakeProfile(FName Id, FName Family, bool bWorldMesh = true)
	{
		FGridWeaponVisualProfile Profile;
		Profile.VisualId = Id;
		Profile.FamilyTag = Family;
		Profile.BaseIcon = TSoftObjectPtr<UTexture2D>(
			FSoftObjectPath(TEXT("/Game/GrimrockPrototype/UI/Icons/Weapons/T_VisualPilot.T_VisualPilot")));
		if (bWorldMesh)
		{
			Profile.WorldMesh = TSoftObjectPtr<UStaticMesh>(
				FSoftObjectPath(TEXT("/Game/GrimrockPrototype/Meshes/Weapons/SM_VisualPilot.SM_VisualPilot")));
		}
		return Profile;
	}

	FGridWeaponVisualArchetypeBinding MakeBinding(FName Archetype, FName Visual)
	{
		FGridWeaponVisualArchetypeBinding Binding;
		Binding.ArchetypeId = Archetype;
		Binding.VisualId = Visual;
		return Binding;
	}

	UGridWeaponVisualCatalogueAsset* MakePilot()
	{
		UGridWeaponVisualCatalogueAsset* Catalogue = NewObject<UGridWeaponVisualCatalogueAsset>();
		Catalogue->VisualProfiles.Add(MakeProfile(TEXT("VIS_Dagger_A"), TEXT("Light")));
		Catalogue->VisualProfiles.Add(MakeProfile(TEXT("VIS_Bow_A"), TEXT("Ranged")));
		Catalogue->VisualProfiles.Add(MakeProfile(TEXT("VIS_Shield_A"), TEXT("Shield")));
		Catalogue->ArchetypeBindings.Add(MakeBinding(TEXT("ARM001"), TEXT("VIS_Dagger_A")));
		Catalogue->ArchetypeBindings.Add(MakeBinding(TEXT("ARM002"), TEXT("VIS_Dagger_A")));
		Catalogue->ArchetypeBindings.Add(MakeBinding(TEXT("ARM045"), TEXT("VIS_Bow_A")));
		Catalogue->ArchetypeBindings.Add(MakeBinding(TEXT("ARM075"), TEXT("VIS_Shield_A")));
		return Catalogue;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0161SchemaTest,
	"Grimrock.RPG.ARMEMENT01.6.VisualCatalogueSchema",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0161SchemaTest::RunTest(const FString&)
{
	UGridWeaponVisualCatalogueAsset* Catalogue = ARMEMENT0161::MakePilot();
	TestTrue(TEXT("Partially mapped pilot catalogue is structurally valid"), Catalogue->IsValidCatalogue());
	TestEqual(TEXT("Four bindings cover three reusable silhouettes"), Catalogue->ArchetypeBindings.Num(), 4);
	TestEqual(TEXT("Only three visual profiles are authored"), Catalogue->VisualProfiles.Num(), 3);
	TestEqual(TEXT("ARM001 maps to dagger"), Catalogue->FindVisualId(TEXT("ARM001")), FName(TEXT("VIS_Dagger_A")));
	TestEqual(TEXT("ARM002 shares the same dagger visual"), Catalogue->FindVisualId(TEXT("ARM002")), FName(TEXT("VIS_Dagger_A")));
	TestEqual(TEXT("ARM075 maps to passive shield visual"), Catalogue->FindVisualId(TEXT("ARM075")), FName(TEXT("VIS_Shield_A")));
	TestTrue(TEXT("Unmapped archetypes do not inherit another silhouette"), Catalogue->FindVisualId(TEXT("ARM003")).IsNone());
	TestNull(TEXT("Unmapped archetype returns no profile"), Catalogue->FindProfileForArchetype(TEXT("ARM003")));
	const FGridWeaponVisualProfile* First = Catalogue->FindProfileForArchetype(TEXT("ARM001"));
	const FGridWeaponVisualProfile* Shared = Catalogue->FindProfileForArchetype(TEXT("ARM002"));
	TestNotNull(TEXT("Shared source profile is available"), First);
	TestTrue(TEXT("Both archetypes share exactly one visual record"), First != nullptr && First == Shared);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0161DuplicateTest,
	"Grimrock.RPG.ARMEMENT01.6.UniqueBindings",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0161DuplicateTest::RunTest(const FString&)
{
	UGridWeaponVisualCatalogueAsset* Catalogue = ARMEMENT0161::MakePilot();
	Catalogue->ArchetypeBindings.Add(ARMEMENT0161::MakeBinding(TEXT("ARM001"), TEXT("VIS_Bow_A")));
	TestFalse(TEXT("Duplicate archetype with conflicting visual rejected"), Catalogue->IsValidCatalogue());

	Catalogue = ARMEMENT0161::MakePilot();
	Catalogue->ArchetypeBindings.Add(ARMEMENT0161::MakeBinding(TEXT("ARM001"), TEXT("VIS_Dagger_A")));
	TestFalse(TEXT("Even identical duplicate archetype rejected"), Catalogue->IsValidCatalogue());

	Catalogue = ARMEMENT0161::MakePilot();
	Catalogue->VisualProfiles.Add(ARMEMENT0161::MakeProfile(TEXT("VIS_Dagger_A"), TEXT("Light")));
	TestFalse(TEXT("Duplicate visual identity rejected"), Catalogue->IsValidCatalogue());

	Catalogue = ARMEMENT0161::MakePilot();
	Catalogue->ArchetypeBindings[0].VisualId = TEXT("VIS_Unknown");
	TestFalse(TEXT("Binding pointing to unknown silhouette rejected"), Catalogue->IsValidCatalogue());

	Catalogue = ARMEMENT0161::MakePilot();
	Catalogue->ArchetypeBindings.RemoveAt(0);
	TestTrue(TEXT("Pilot catalogue may be only partially mapped"), Catalogue->IsValidCatalogue());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0161VisualProfileTest,
	"Grimrock.RPG.ARMEMENT01.6.VisualProfileValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0161VisualProfileTest::RunTest(const FString&)
{
	using namespace ARMEMENT0161;
	TestTrue(TEXT("Canonical first archetype"), UGridWeaponVisualCatalogueAsset::IsCanonicalArchetypeId(TEXT("ARM001")));
	TestTrue(TEXT("Canonical last archetype"), UGridWeaponVisualCatalogueAsset::IsCanonicalArchetypeId(TEXT("ARM082")));
	TestFalse(TEXT("ARM000 is invalid"), UGridWeaponVisualCatalogueAsset::IsCanonicalArchetypeId(TEXT("ARM000")));
	TestFalse(TEXT("ARM083 is invalid"), UGridWeaponVisualCatalogueAsset::IsCanonicalArchetypeId(TEXT("ARM083")));
	TestFalse(TEXT("Malformed identity rejected"), UGridWeaponVisualCatalogueAsset::IsCanonicalArchetypeId(TEXT("Archetype_1")));

	FGridWeaponVisualProfile Profile = MakeProfile(TEXT("VIS_Test"), TEXT("Light"), false);
	TestTrue(TEXT("Icon-only profile is allowed"), Profile.IsValidProfile());
	Profile.BaseIcon.Reset();
	TestFalse(TEXT("Completely empty silhouette is rejected"), Profile.IsValidProfile());

	Profile = MakeProfile(TEXT("VIS_Test"), TEXT("Light"));
	Profile.BaseIcon.Reset();
	TestTrue(TEXT("Mesh-only profile is allowed"), Profile.IsValidProfile());

	Profile.OrnamentMesh = TSoftObjectPtr<UStaticMesh>(
		FSoftObjectPath(TEXT("/Game/GrimrockPrototype/Meshes/Weapons/SM_Ornament.SM_Ornament")));
	TestTrue(TEXT("Ornament with world mesh allowed"), Profile.IsValidProfile());
	Profile.WorldMesh.Reset();
	TestFalse(TEXT("Ornament with no base mesh rejected"), Profile.IsValidProfile());

	Profile = MakeProfile(TEXT("VIS_Test"), TEXT("Light"));
	Profile.WorldVisualTransform.SetScale3D(FVector::ZeroVector);
	TestFalse(TEXT("Degenerate world transform rejected"), Profile.IsValidProfile());

	Profile = MakeProfile(TEXT("VIS_Test"), NAME_None);
	TestFalse(TEXT("Family group required for authoring"), Profile.IsValidProfile());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0161ItemOptInTest,
	"Grimrock.RPG.ARMEMENT01.6.ItemVisualOptIn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0161ItemOptInTest::RunTest(const FString&)
{
	UGridItemDefinitionAsset* Item = NewObject<UGridItemDefinitionAsset>();
	Item->ItemDefinitionId = TEXT("ARM0161_Prototype");
	TestFalse(TEXT("All prior item definitions remain visual-Legacy by default"), Item->bUseWeaponVisualCatalogue);
	TestTrue(TEXT("Legacy item without any visual catalogue stays valid"), Item->IsValidDefinition());

	Item->bUseWeaponVisualCatalogue = true;
	TestFalse(TEXT("Explicit visual opt-in requires an archetype and catalogue"), Item->IsValidDefinition());

	Item->WeaponArchetypeId = TEXT("ARM001");
	Item->WeaponVisualCatalogue = ARMEMENT0161::MakePilot();
	TestTrue(TEXT("Visually linked item definition is structurally valid"), Item->IsValidDefinition());
	TestFalse(TEXT("Visual opt-in does not auto-enable combat catalogue"), Item->bUseWeaponCatalogue);

	Item->WeaponArchetypeId = TEXT("ARM099");
	TestFalse(TEXT("Unknown weapon ID is rejected"), Item->IsValidDefinition());
	Item->bUseWeaponVisualCatalogue = false;
	TestTrue(TEXT("Disabling visual opt-in preserves Legacy validity"), Item->IsValidDefinition());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0161NoGameplayAuthorityTest,
	"Grimrock.RPG.ARMEMENT01.6.VisualOnlyNoGameplayAuthority",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0161NoGameplayAuthorityTest::RunTest(const FString&)
{
	UScriptStruct* ProfileType = FGridWeaponVisualProfile::StaticStruct();
	TestNotNull(TEXT("Visual profile is a reflected struct"), ProfileType);
	if (!ProfileType) { return false; }

	TestNull(TEXT("Visual profile has no dice authority"), ProfileType->FindPropertyByName(TEXT("DamageDice")));
	TestNull(TEXT("Visual profile has no item-level authority"), ProfileType->FindPropertyByName(TEXT("ItemLevel")));
	TestNull(TEXT("Visual profile has no rarity authority"), ProfileType->FindPropertyByName(TEXT("Rarity")));
	TestNull(TEXT("Visual profile has no forging authority"), ProfileType->FindPropertyByName(TEXT("ForgeRank")));
	TestNull(TEXT("Visual profile has no material override"), ProfileType->FindPropertyByName(TEXT("MaterialOverride")));

	const FProperty* OutputIcon = FGridResolvedWeaponVisual::StaticStruct()->FindPropertyByName(TEXT("Icon"));
	TestNotNull(TEXT("Visual read model exposes an icon"), OutputIcon);
	TestTrue(TEXT("Resolved visual icon is transient and never saved"),
		OutputIcon && OutputIcon->HasAnyPropertyFlags(CPF_Transient));

	const FProperty* ItemLevel = FGridItemInstance::StaticStruct()->FindPropertyByName(TEXT("ItemLevel"));
	TestNotNull(TEXT("Instance keeps ARMEMENT01.5's level field"), ItemLevel);
	TestNull(TEXT("Instance does not persist presentation VisualId"),
		FGridItemInstance::StaticStruct()->FindPropertyByName(TEXT("VisualId")));
	TestNull(TEXT("Instance does not persist guessed rarity"),
		FGridItemInstance::StaticStruct()->FindPropertyByName(TEXT("Rarity")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
