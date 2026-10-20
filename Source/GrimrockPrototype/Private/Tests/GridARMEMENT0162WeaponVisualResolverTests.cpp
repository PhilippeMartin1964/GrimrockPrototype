#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Runtime/Combat/GridWeaponVisualResolver.h"
#include "Runtime/GridInventoryTypes.h"
#include "Runtime/GridItemDefinitionAsset.h"

namespace ARMEMENT0162
{
	TSoftObjectPtr<UTexture2D> Icon(const TCHAR* Name)
	{
		return TSoftObjectPtr<UTexture2D>(FSoftObjectPath(Name));
	}

	TSoftObjectPtr<UStaticMesh> Mesh(const TCHAR* Name)
	{
		return TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(Name));
	}

	FGridWeaponVisualProfile MakeProfile(FName VisualId)
	{
		FGridWeaponVisualProfile Profile;
		Profile.VisualId = VisualId;
		Profile.FamilyTag = TEXT("Light");
		Profile.BaseIcon = Icon(TEXT("/Game/GrimrockPrototype/UI/Icons/Weapons/T_TestVisual.T_TestVisual"));
		Profile.WorldMesh = Mesh(TEXT("/Game/GrimrockPrototype/Meshes/Weapons/SM_TestVisual.SM_TestVisual"));
		Profile.WorldVisualTransform.SetLocation(FVector(2.0f, 3.0f, 4.0f));
		return Profile;
	}

	UGridWeaponVisualCatalogueAsset* MakeCatalogue()
	{
		UGridWeaponVisualCatalogueAsset* Catalogue = NewObject<UGridWeaponVisualCatalogueAsset>();
		Catalogue->VisualProfiles.Add(MakeProfile(TEXT("VIS_Dagger_A")));

		FGridWeaponVisualArchetypeBinding First;
		First.ArchetypeId = TEXT("ARM001");
		First.VisualId = TEXT("VIS_Dagger_A");
		Catalogue->ArchetypeBindings.Add(First);

		FGridWeaponVisualArchetypeBinding Shared = First;
		Shared.ArchetypeId = TEXT("ARM002");
		Catalogue->ArchetypeBindings.Add(Shared);
		return Catalogue;
	}

	UGridItemDefinitionAsset* MakeDefinition(
		UGridWeaponVisualCatalogueAsset* Catalogue, FName Archetype = TEXT("ARM001"))
	{
		UGridItemDefinitionAsset* Definition = NewObject<UGridItemDefinitionAsset>();
		Definition->ItemDefinitionId = TEXT("ARM0162_Weapon");
		Definition->ItemType = EGridItemType::Weapon;
		Definition->WeaponArchetypeId = Archetype;
		Definition->bUseWeaponVisualCatalogue = true;
		Definition->WeaponVisualCatalogue = Catalogue;
		Definition->Icon = Icon(TEXT("/Game/GrimrockPrototype/UI/Icons/Weapons/T_Legacy.T_Legacy"));
		Definition->WorldMesh = Mesh(TEXT("/Game/GrimrockPrototype/Meshes/Weapons/SM_Legacy.SM_Legacy"));
		Definition->EquippedMesh = Mesh(TEXT("/Game/GrimrockPrototype/Meshes/Weapons/SM_LegacyEquipped.SM_LegacyEquipped"));
		return Definition;
	}

	FGridItemInstance MakeInstance(const UGridItemDefinitionAsset* Definition, int32 Level)
	{
		FGridItemInstance Item;
		Item.RuntimeObjectId = FGuid::NewGuid();
		Item.ItemDefinitionId = Definition->ItemDefinitionId;
		Item.ItemLevel = Level;
		Item.Quantity = 1;
		Item.Weight = 4.25f;
		return Item;
	}

	bool SamePath(const TSoftObjectPtr<UTexture2D>& A, const TSoftObjectPtr<UTexture2D>& B)
	{
		return A.ToSoftObjectPath() == B.ToSoftObjectPath();
	}
	bool SamePath(const TSoftObjectPtr<UStaticMesh>& A, const TSoftObjectPtr<UStaticMesh>& B)
	{
		return A.ToSoftObjectPath() == B.ToSoftObjectPath();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0162SharedVisualTest,
	"Grimrock.RPG.ARMEMENT01.6.Resolver.SharedSilhouette",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0162SharedVisualTest::RunTest(const FString&)
{
	UGridWeaponVisualCatalogueAsset* Catalogue = ARMEMENT0162::MakeCatalogue();
	if (!TestTrue(TEXT("Pilot catalogue valid"), Catalogue->IsValidCatalogue())) { return false; }

	UGridItemDefinitionAsset* Definition = ARMEMENT0162::MakeDefinition(Catalogue);
	const FGridItemInstance Item = ARMEMENT0162::MakeInstance(Definition, 7);
	FGridResolvedWeaponVisual First;
	EGridWeaponVisualResolutionError Error = EGridWeaponVisualResolutionError::CatalogueUnavailable;
	TestTrue(TEXT("Fixed-level item resolves its silhouette"),
		FGridWeaponVisualResolver::TryResolve(*Definition, &Item, First, Error));
	TestTrue(TEXT("Output bResolved is set"), First.bResolved);
	TestTrue(TEXT("No visual lookup error"), Error == EGridWeaponVisualResolutionError::None &&
		First.Error == EGridWeaponVisualResolutionError::None);
	TestFalse(TEXT("No unintended Legacy fallback"), First.bLegacyFallback);
	TestEqual(TEXT("Resolved VisualId"), First.VisualId, FName(TEXT("VIS_Dagger_A")));
	TestTrue(TEXT("Visual icon comes from profile"),
		ARMEMENT0162::SamePath(First.Icon, Catalogue->VisualProfiles[0].BaseIcon));
	TestTrue(TEXT("World mesh comes from profile"),
		ARMEMENT0162::SamePath(First.WorldMesh, Catalogue->VisualProfiles[0].WorldMesh));
	TestTrue(TEXT("Unassigned equipped mesh falls back to resolved world mesh"),
		ARMEMENT0162::SamePath(First.EquippedMesh, First.WorldMesh));
	TestTrue(TEXT("World visual offset is projected"),
		First.WorldVisualTransform.GetLocation().Equals(FVector(2.0f, 3.0f, 4.0f)));
	TestTrue(TEXT("Real item level is exposed"), First.bHasItemLevel);
	TestEqual(TEXT("Real item level stays seven"), First.ItemLevel, 7);

	UGridItemDefinitionAsset* Other = ARMEMENT0162::MakeDefinition(Catalogue, TEXT("ARM002"));
	FGridResolvedWeaponVisual Shared;
	TestTrue(TEXT("Another archetype resolves the same silhouette"),
		FGridWeaponVisualResolver::TryResolve(*Other, nullptr, Shared, Error));
	TestEqual(TEXT("Shared VisualId without duplicate profile"), Shared.VisualId, First.VisualId);
	TestFalse(TEXT("Definition-only preview never invents a level"), Shared.bHasItemLevel);
	TestEqual(TEXT("Definition-only preview has no level"), Shared.ItemLevel, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0162LegacyFallbackTest,
	"Grimrock.RPG.ARMEMENT01.6.Resolver.LegacyExactFallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0162LegacyFallbackTest::RunTest(const FString&)
{
	UGridItemDefinitionAsset* Definition = ARMEMENT0162::MakeDefinition(nullptr);
	Definition->bUseWeaponVisualCatalogue = false;
	const FGridItemInstance Item = ARMEMENT0162::MakeInstance(Definition, 0);
	FGridResolvedWeaponVisual Out;
	EGridWeaponVisualResolutionError Error;
	TestTrue(TEXT("Legacy visual is still usable"),
		FGridWeaponVisualResolver::TryResolve(*Definition, &Item, Out, Error));
	TestTrue(TEXT("Legacy path is explicitly indicated"), Out.bLegacyFallback);
	TestTrue(TEXT("Legacy icon is identical"),
		ARMEMENT0162::SamePath(Out.Icon, Definition->Icon));
	TestTrue(TEXT("Legacy world mesh is identical"),
		ARMEMENT0162::SamePath(Out.WorldMesh, Definition->WorldMesh));
	TestTrue(TEXT("Legacy equipped mesh remains distinct"),
		ARMEMENT0162::SamePath(Out.EquippedMesh, Definition->EquippedMesh));
	TestTrue(TEXT("No lookup error when opt-in is disabled"),
		Error == EGridWeaponVisualResolutionError::None);
	TestFalse(TEXT("Legacy level zero does not become level 1"), Out.bHasItemLevel);
	TestTrue(TEXT("Legacy visual has no new ornament"), Out.OrnamentMesh.IsNull());
	TestTrue(TEXT("Legacy offset is identity"), Out.WorldVisualTransform.Equals(FTransform::Identity));

	Definition->Icon.Reset();
	Definition->WorldMesh.Reset();
	Definition->EquippedMesh.Reset();
	TestFalse(TEXT("Legacy item with no assigned art has no renderable reference"),
		FGridWeaponVisualResolver::TryResolve(*Definition, &Item, Out, Error));
	TestTrue(TEXT("Completely missing artwork reports MissingVisualResource"),
		Error == EGridWeaponVisualResolutionError::MissingVisualResource);
	TestFalse(TEXT("No-art read model is not falsely marked resolved"), Out.bResolved);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0162PartialResourceTest,
	"Grimrock.RPG.ARMEMENT01.6.Resolver.PartialResourceFallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0162PartialResourceTest::RunTest(const FString&)
{
	UGridWeaponVisualCatalogueAsset* Catalogue = ARMEMENT0162::MakeCatalogue();
	UGridItemDefinitionAsset* Definition = ARMEMENT0162::MakeDefinition(Catalogue);
	FGridResolvedWeaponVisual Out;
	EGridWeaponVisualResolutionError Error;

	Catalogue->VisualProfiles[0].WorldMesh.Reset();
	TestTrue(TEXT("Icon-only visual remains valid"), Catalogue->IsValidCatalogue());
	TestTrue(TEXT("Icon-only visual resolved with Legacy world mesh"),
		FGridWeaponVisualResolver::TryResolve(*Definition, nullptr, Out, Error));
	TestTrue(TEXT("Legacy world component fallback is reported"), Out.bLegacyFallback);
	TestTrue(TEXT("New icon is kept"), ARMEMENT0162::SamePath(Out.Icon, Catalogue->VisualProfiles[0].BaseIcon));
	TestTrue(TEXT("Legacy world mesh fills missing channel"),
		ARMEMENT0162::SamePath(Out.WorldMesh, Definition->WorldMesh));
	TestTrue(TEXT("Fallback world mesh used for equipped presentation"),
		ARMEMENT0162::SamePath(Out.EquippedMesh, Definition->WorldMesh));
	TestEqual(TEXT("Successful partial profile has no error"),
		static_cast<int32>(Error), static_cast<int32>(EGridWeaponVisualResolutionError::None));

	Catalogue->VisualProfiles[0].WorldMesh = ARMEMENT0162::Mesh(
		TEXT("/Game/GrimrockPrototype/Meshes/Weapons/SM_TestVisual.SM_TestVisual"));
	Catalogue->VisualProfiles[0].BaseIcon.Reset();
	TestTrue(TEXT("Mesh-only visual remains valid"), Catalogue->IsValidCatalogue());
	TestTrue(TEXT("Mesh-only visual resolved with Legacy icon"),
		FGridWeaponVisualResolver::TryResolve(*Definition, nullptr, Out, Error));
	TestTrue(TEXT("Legacy icon fills missing channel"), ARMEMENT0162::SamePath(Out.Icon, Definition->Icon));
	TestTrue(TEXT("New world mesh is used"), ARMEMENT0162::SamePath(Out.WorldMesh, Catalogue->VisualProfiles[0].WorldMesh));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0162LookupFailuresTest,
	"Grimrock.RPG.ARMEMENT01.6.Resolver.MissingCatalogueAndBindings",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0162LookupFailuresTest::RunTest(const FString&)
{
	UGridWeaponVisualCatalogueAsset* Catalogue = ARMEMENT0162::MakeCatalogue();
	UGridItemDefinitionAsset* Definition = ARMEMENT0162::MakeDefinition(Catalogue);
	FGridResolvedWeaponVisual Out;
	EGridWeaponVisualResolutionError Error;

	Definition->WeaponVisualCatalogue.Reset();
	TestTrue(TEXT("Missing catalogue still displays Legacy icon"),
		FGridWeaponVisualResolver::TryResolve(*Definition, nullptr, Out, Error));
	TestTrue(TEXT("CatalogueUnavailable is surfaced despite a usable fallback"),
		Error == EGridWeaponVisualResolutionError::CatalogueUnavailable &&
		Out.Error == Error && Out.bLegacyFallback);
	TestTrue(TEXT("Missing catalogue never borrows another archetype's visual"),
		Out.VisualId.IsNone());

	// A declared but unloaded soft reference must not be LoadSynchronous'd.
	Definition->WeaponVisualCatalogue = TSoftObjectPtr<UGridWeaponVisualCatalogueAsset>(
		FSoftObjectPath(TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_MissingVisual.DA_MissingVisual")));
	TestNull(TEXT("Broken visual catalogue path stays unloaded before resolving"),
		Definition->WeaponVisualCatalogue.Get());
	TestTrue(TEXT("Unloaded catalogue falls back without synchronous loading"),
		FGridWeaponVisualResolver::TryResolve(*Definition, nullptr, Out, Error));
	TestTrue(TEXT("Unloaded soft asset produces CatalogueUnavailable"),
		Error == EGridWeaponVisualResolutionError::CatalogueUnavailable);
	TestNull(TEXT("Broken visual catalogue remains unloaded after resolving"),
		Definition->WeaponVisualCatalogue.Get());

	Definition->WeaponVisualCatalogue = Catalogue;
	Definition->WeaponArchetypeId = TEXT("ARM003");
	TestTrue(TEXT("Unmapped archetype gets its own Legacy fallback"),
		FGridWeaponVisualResolver::TryResolve(*Definition, nullptr, Out, Error));
	TestTrue(TEXT("MissingBinding is explicit"), Error == EGridWeaponVisualResolutionError::MissingBinding);

	Definition->WeaponArchetypeId = TEXT("ARM001");
	Catalogue->ArchetypeBindings[0].VisualId = TEXT("VIS_Ghost");
	TestTrue(TEXT("Unknown VisualId still has legacy presentation"),
		FGridWeaponVisualResolver::TryResolve(*Definition, nullptr, Out, Error));
	TestTrue(TEXT("UnknownVisualId is explicit"), Error == EGridWeaponVisualResolutionError::UnknownVisualId);
	TestTrue(TEXT("No invalid silhouette is surfaced"), Out.VisualId.IsNone());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0162MalformedTest,
	"Grimrock.RPG.ARMEMENT01.6.Resolver.InvalidInstanceAndProfile",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0162MalformedTest::RunTest(const FString&)
{
	UGridWeaponVisualCatalogueAsset* Catalogue = ARMEMENT0162::MakeCatalogue();
	UGridItemDefinitionAsset* Definition = ARMEMENT0162::MakeDefinition(Catalogue);
	FGridResolvedWeaponVisual Out;
	EGridWeaponVisualResolutionError Error;

	FGridItemInstance Wrong = ARMEMENT0162::MakeInstance(Definition, 9);
	Wrong.ItemDefinitionId = TEXT("AnotherItem");
	TestTrue(TEXT("Invalid instance still shows neutral Legacy icon"),
		FGridWeaponVisualResolver::TryResolve(*Definition, &Wrong, Out, Error));
	TestTrue(TEXT("Mismatched item identity reported"),
		Error == EGridWeaponVisualResolutionError::InstanceContextUnavailable);
	TestFalse(TEXT("Wrong instance never leaks an item-level badge"), Out.bHasItemLevel);
	TestEqual(TEXT("Wrong instance has zero projected level"), Out.ItemLevel, 0);

	Wrong.ItemDefinitionId = Definition->ItemDefinitionId;
	Wrong.ItemLevel = 21;
	TestTrue(TEXT("Out-of-range item retains safe Legacy fallback"),
		FGridWeaponVisualResolver::TryResolve(*Definition, &Wrong, Out, Error));
	TestTrue(TEXT("Invalid level reported"), Error == EGridWeaponVisualResolutionError::InstanceContextUnavailable);
	TestFalse(TEXT("Level 21 not displayed"), Out.bHasItemLevel);

	Catalogue->VisualProfiles[0].BaseIcon.Reset();
	Catalogue->VisualProfiles[0].WorldMesh.Reset();
	TestTrue(TEXT("Invalid visual profile safely falls back"),
		FGridWeaponVisualResolver::TryResolve(*Definition, nullptr, Out, Error));
	TestTrue(TEXT("Missing visual resource reported"),
		Error == EGridWeaponVisualResolutionError::MissingVisualResource);
	TestTrue(TEXT("Fallback does not borrow invalid VisualId"), Out.VisualId.IsNone());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0162NoMutationTest,
	"Grimrock.RPG.ARMEMENT01.6.Resolver.ReadOnlyGameplay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0162NoMutationTest::RunTest(const FString&)
{
	UGridWeaponVisualCatalogueAsset* Catalogue = ARMEMENT0162::MakeCatalogue();
	UGridItemDefinitionAsset* Definition = ARMEMENT0162::MakeDefinition(Catalogue);
	FGridItemInstance Item = ARMEMENT0162::MakeInstance(Definition, 11);
	Item.Quantity = 3;
	const FGridItemInstance Original = Item;
	const FName OriginalArchetype = Definition->WeaponArchetypeId;
	const int32 OriginalDefaultLevel = Definition->DefaultWeaponItemLevel;

	FGridResolvedWeaponVisual First;
	FGridResolvedWeaponVisual Second;
	EGridWeaponVisualResolutionError Error;
	TestTrue(TEXT("First projection"), FGridWeaponVisualResolver::TryResolve(*Definition, &Item, First, Error));
	TestTrue(TEXT("Second projection"), FGridWeaponVisualResolver::TryResolve(*Definition, &Item, Second, Error));
	TestEqual(TEXT("Same VisualId on repeated projection"), Second.VisualId, First.VisualId);
	TestTrue(TEXT("Repeated soft icon path is identical"),
		ARMEMENT0162::SamePath(Second.Icon, First.Icon));
	TestEqual(TEXT("Repeated presentation has same level"), Second.ItemLevel, First.ItemLevel);
	TestEqual(TEXT("RuntimeObjectId remains unchanged"), Item.RuntimeObjectId, Original.RuntimeObjectId);
	TestEqual(TEXT("ItemDefinitionId remains unchanged"), Item.ItemDefinitionId, Original.ItemDefinitionId);
	TestEqual(TEXT("ItemLevel remains unchanged"), Item.ItemLevel, Original.ItemLevel);
	TestEqual(TEXT("Quantity remains unchanged"), Item.Quantity, Original.Quantity);
	TestTrue(TEXT("Weight remains unchanged"), FMath::IsNearlyEqual(Item.Weight, Original.Weight));
	TestEqual(TEXT("Definition archetype remains unchanged"), Definition->WeaponArchetypeId, OriginalArchetype);
	TestEqual(TEXT("Default item level remains unchanged"), Definition->DefaultWeaponItemLevel, OriginalDefaultLevel);
	TestFalse(TEXT("Visual-only opt-in does not enable combat catalogue"), Definition->bUseWeaponCatalogue);
	TestTrue(TEXT("No forced icon load"), First.Icon.Get() == nullptr);
	TestTrue(TEXT("No forced world mesh load"), First.WorldMesh.Get() == nullptr);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
