#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Runtime/Combat/GridWeaponVisualCatalogueAsset.h"
#include "Runtime/Combat/GridWeaponVisualResolver.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"

namespace ARMEMENT0163
{
	const TCHAR* CataloguePath = TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponVisualCatalogue.DA_WeaponVisualCatalogue");

	struct FPilot
	{
		const TCHAR* ArchetypeId;
		const TCHAR* VisualId;
		const TCHAR* IconName;
		const TCHAR* MeshName;
	};
	const FPilot Pilots[3] =
	{
		{TEXT("ARM001"), TEXT("VIS_Dagger_A"), TEXT("T_Weapon_Dagger_A"), TEXT("SM_Weapon_Dagger_A")},
		{TEXT("ARM045"), TEXT("VIS_ShortBow_A"), TEXT("T_Weapon_ShortBow_A"), TEXT("SM_Weapon_ShortBow_A")},
		{TEXT("ARM075"), TEXT("VIS_RoundShield_A"), TEXT("T_Weapon_RoundShield_A"), TEXT("SM_Weapon_RoundShield_A")}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0163MaterializedAssetTest,
	"Grimrock.RPG.ARMEMENT01.6.Pilot.MaterializedAssets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0163MaterializedAssetTest::RunTest(const FString&)
{
	UGridWeaponVisualCatalogueAsset* Catalogue =
		LoadObject<UGridWeaponVisualCatalogueAsset>(nullptr, ARMEMENT0163::CataloguePath);
	if (!TestNotNull(TEXT("Materialized visual catalogue"), Catalogue)) { return false; }
	TestTrue(TEXT("Pilot visual catalogue passes strict structure checks"), Catalogue->IsValidCatalogue());
	TestEqual(TEXT("Exactly three pilot visual profiles"), Catalogue->VisualProfiles.Num(), 3);
	TestEqual(TEXT("Exactly three real-archtype bindings"), Catalogue->ArchetypeBindings.Num(), 3);
	for (const ARMEMENT0163::FPilot& Pilot : ARMEMENT0163::Pilots)
	{
		const FGridWeaponVisualProfile* P = Catalogue->FindProfileForArchetype(FName(Pilot.ArchetypeId));
		if (!TestNotNull(*FString::Printf(TEXT("Profile for %s"), Pilot.ArchetypeId), P))
		{
			continue;
		}
		TestEqual(TEXT("Canonical visual mapping"), P->VisualId, FName(Pilot.VisualId));
		const UTexture2D* Icon = P->BaseIcon.LoadSynchronous();
		const UStaticMesh* Mesh = P->WorldMesh.LoadSynchronous();
		if (!TestNotNull(TEXT("Actual RGBA icon imports"), Icon) ||
			!TestNotNull(TEXT("Actual OBJ static mesh imports"), Mesh))
		{
			continue;
		}
		// Check the imported source resolution, not platform/RHI texture dimensions.
		// Under -NullRHI the latter may be 0 despite a valid PNG source.
		const FIntPoint ImportedSize = Icon->GetImportedSize();
		TestEqual(TEXT("Pilot imported icon width 512px"), ImportedSize.X, 512);
		TestEqual(TEXT("Pilot imported icon height 512px"), ImportedSize.Y, 512);
		TestTrue(TEXT("Pilot mesh has render geometry"), Mesh->GetNumLODs() > 0);
		TestTrue(TEXT("Pilot mesh has nonzero bounds"), Mesh->GetBounds().SphereRadius > 1.0f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0163ResolverTest,
	"Grimrock.RPG.ARMEMENT01.6.Pilot.ProductionResolver",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0163ResolverTest::RunTest(const FString&)
{
	UGridWeaponVisualCatalogueAsset* Catalogue =
		LoadObject<UGridWeaponVisualCatalogueAsset>(nullptr, ARMEMENT0163::CataloguePath);
	if (!TestNotNull(TEXT("Pilot catalogue"), Catalogue)) { return false; }
	for (const ARMEMENT0163::FPilot& Pilot : ARMEMENT0163::Pilots)
	{
		UGridItemDefinitionAsset* Definition = NewObject<UGridItemDefinitionAsset>();
		Definition->ItemDefinitionId = TEXT("ARM0163_TestItem");
		Definition->WeaponArchetypeId = FName(Pilot.ArchetypeId);
		Definition->bUseWeaponVisualCatalogue = true;
		Definition->WeaponVisualCatalogue = Catalogue;

		FGridResolvedWeaponVisual View;
		EGridWeaponVisualResolutionError Error = EGridWeaponVisualResolutionError::CatalogueUnavailable;
		TestTrue(TEXT("Real pilot profile resolves"),
			FGridWeaponVisualResolver::TryResolve(*Definition, nullptr, View, Error));
		TestTrue(TEXT("No catalogue lookup error"), Error == EGridWeaponVisualResolutionError::None);
		TestEqual(TEXT("Resolver keeps real VisualId"), View.VisualId, FName(Pilot.VisualId));
		TestFalse(TEXT("Preview without item instance has no fabricated level"), View.bHasItemLevel);
		TestFalse(TEXT("Pilot profile needs no Legacy fallback"), View.bLegacyFallback);
		TestTrue(TEXT("Same world mesh can represent equipped weapon"),
			View.EquippedMesh.ToSoftObjectPath() == View.WorldMesh.ToSoftObjectPath());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0163PassiveTest,
	"Grimrock.RPG.ARMEMENT01.6.Pilot.PassiveShieldNoAttack",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0163PassiveTest::RunTest(const FString&)
{
	UGridWeaponVisualCatalogueAsset* Visuals =
		LoadObject<UGridWeaponVisualCatalogueAsset>(nullptr, ARMEMENT0163::CataloguePath);
	if (!TestNotNull(TEXT("Visual catalogue available"), Visuals)) { return false; }
	const FGridWeaponVisualProfile* Shield = Visuals->FindProfileForArchetype(TEXT("ARM075"));
	TestNotNull(TEXT("Round shield is a separate passive silhouette"), Shield);
	if (!Shield) { return false; }

	UGridItemDefinitionAsset* Definition = NewObject<UGridItemDefinitionAsset>();
	Definition->ItemDefinitionId = TEXT("ARM0163_RoundShield");
	Definition->ItemType = EGridItemType::Shield;
	Definition->WeaponArchetypeId = TEXT("ARM075");
	Definition->bUseWeaponVisualCatalogue = true;
	Definition->WeaponVisualCatalogue = Visuals;

	TestTrue(TEXT("Visually linked shield is a valid item"), Definition->IsValidDefinition());
	TestFalse(TEXT("Visual opt-in never switches on catalogue combat"), Definition->bUseWeaponCatalogue);
	TestFalse(TEXT("Visual-only passive shield has no attack"), Definition->CanProvideAttackFromSlot(EGridEquipmentSlot::MainHand));
	TestTrue(TEXT("No attack action is generated"), Definition->CombatActions.IsEmpty());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
