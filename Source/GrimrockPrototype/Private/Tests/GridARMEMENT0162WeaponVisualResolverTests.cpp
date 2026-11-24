#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Runtime/GridItemMeshResolver.h"
#include "UI/GridWeaponVisualUiResolver.h"
#include "UObject/StrongObjectPtr.h"

namespace ARMEMENT0162
{
	const FName Sword(TEXT("Weapon_StraightSword_01"));
	const FName Shield(TEXT("Weapon_RoundShield_01"));
	const FName VisualId(TEXT("VIS_0162_Shared"));
	const FSoftObjectPath Icon(TEXT("/Game/Fixture/T_Visual.T_Visual"));
	const FSoftObjectPath WorldMesh(TEXT("/Game/Fixture/SM_World.SM_World"));
	const FSoftObjectPath EquippedMesh(TEXT("/Game/Fixture/SM_Equipped.SM_Equipped"));
	const FTransform Offset(FVector(2.0, 3.0, 4.0));

	struct FFixture
	{
		TStrongObjectPtr<UGameInstance> Game{NewObject<UGameInstance>()};
		TStrongObjectPtr<UGridItemSubsystem> Registry{NewObject<UGridItemSubsystem>(Game.Get())};
		TStrongObjectPtr<UGridItemSystemAsset> Root;
		FGridWeaponVisual* Visual = nullptr;
		FName SwordArchetype;

		bool Prepare(FAutomationTestBase& Test)
		{
			UGridItemSystemAsset* Source = GetDefault<UGridItemSubsystem>()->LoadConfiguredItemSystemForPreview();
			if (!Test.TestTrue(TEXT("Configured source and visual catalogue exist"),
				IsValid(Source) && IsValid(Source->WeaponVisualCatalogue.Get())))
				return false;

			Root.Reset(DuplicateObject<UGridItemSystemAsset>(Source, GetTransientPackage()));
			Root->WeaponVisualCatalogue = DuplicateObject<UGridWeaponVisualCatalogueAsset>(Source->WeaponVisualCatalogue, Root.Get());
			auto* Catalogue = Root->WeaponVisualCatalogue.Get();
			int32 Redirected = 0;
			for (const UGridItemDefinitionAsset* Definition : Root->ItemDefinitions)
			{
				const auto* Arm = Cast<UGridArmamentDefinitionAsset>(Definition);
				if (!Arm || (Arm->ItemId != Sword && Arm->ItemId != Shield))
					continue;
				const auto* Rule = Root->WeaponCatalogue->Find(Arm->ArchetypeId);
				if (!Test.TestTrue(TEXT("Fixture item has no projectile visual requirement"), Rule && !Rule->Projectile.bEnabled))
					return false;
				auto* Binding = Catalogue->ArchetypeBindings.FindByPredicate([Arm](const FGridWeaponVisualBinding& Row)
				{
					return Row.ArchetypeId == Arm->ArchetypeId;
				});
				if (!Test.TestNotNull(TEXT("Fixture armament has a binding"), Binding))
					return false;
				Binding->VisualId = VisualId;
				if (Arm->ItemId == Sword)
					SwordArchetype = Arm->ArchetypeId;
				++Redirected;
			}
			Visual = &Catalogue->VisualProfiles.AddDefaulted_GetRef();
			Visual->VisualId = VisualId;
			Visual->ItemIcon = TSoftObjectPtr<UTexture2D>(Icon);
			Visual->WorldMesh = TSoftObjectPtr<UStaticMesh>(WorldMesh);
			Visual->WorldTransform = Offset;
			return Test.TestEqual(TEXT("Both intended identities enrolled"), Redirected, 2);
		}

		bool Register(FAutomationTestBase& Test)
		{
			TArray<FString> Errors;
			if (Registry->RegisterItemSystem(Root.Get(), Errors))
				return true;
			for (const FString& Error : Errors)
				Test.AddError(Error);
			return false;
		}
	};

	FGridItemInstance MakeInstance(FName Id, int32 Level)
	{
		FGridItemInstance Item;
		Item.RuntimeObjectId = FGuid::NewGuid();
		Item.ItemDefinitionId = Id;
		Item.ItemLevel = Level;
		Item.Quantity = 1;
		return Item;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0162SharedVisualTest,
	"Grimrock.RPG.ARMEMENT01.6.Resolver.SharedSilhouette",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FARMEMENT0162SharedVisualTest::RunTest(const FString&)
{
	using namespace ARMEMENT0162;
	FFixture F;
	if (!F.Prepare(*this) || !F.Register(*this))
		return false;

	for (const FName Id : { Sword, Shield })
	{
		const auto Item = MakeInstance(Id, 7);
		const auto Mesh = FGridItemMeshResolver::Resolve(F.Registry.Get(), Item);
		const auto Ui = FGridWeaponVisualUiResolver::ResolveInventoryItemIcon(F.Registry.Get(), Item);
		TestTrue(TEXT("Registered armament resolves mesh and icon"), Mesh.bResolved && Mesh.bRegisteredIdentity && Ui.bRegisteredIdentity);
		TestEqual(TEXT("Both archetypes use the authored fixture silhouette"), Mesh.VisualId, VisualId);
		TestEqual(TEXT("UI resolves the same explicit silhouette"), Ui.VisualId, VisualId);
		TestEqual(TEXT("Fixture icon"), Ui.Icon.ToSoftObjectPath(), Icon);
		TestEqual(TEXT("Fixture world mesh"), Mesh.WorldMesh.ToSoftObjectPath(), WorldMesh);
		TestEqual(TEXT("Absent equipped mesh uses the authored world mesh"), Mesh.EquippedMesh.ToSoftObjectPath(), WorldMesh);
		TestTrue(TEXT("World transform is preserved"), Mesh.WorldTransform.Equals(Offset));
		TestEqual(TEXT("Item keeps its physical level"), Item.ItemLevel, 7);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0162LegacyFallbackTest,
	"Grimrock.RPG.ARMEMENT01.6.Resolver.LegacyExactFallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FARMEMENT0162LegacyFallbackTest::RunTest(const FString&)
{
	using namespace ARMEMENT0162;
	FFixture F;
	if (!F.Prepare(*this) || !F.Register(*this))
		return false;

	const auto Unknown = MakeInstance(TEXT("ARM0162_NoRegisteredDefinition"), 7);
	FGridResolvedItem Rejected;
	TestFalse(TEXT("Unknown identity cannot resolve any definition"), F.Registry->ResolveItem(Unknown.ItemDefinitionId, 7, Rejected));
	TestTrue(TEXT("Explicit unknown-identity diagnostic"), Rejected.Error == EGridItemResolveError::UnknownItemId);
	const auto IconView = FGridWeaponVisualUiResolver::ResolveInventoryItemIcon(F.Registry.Get(), Unknown);
	const auto Mesh = FGridItemMeshResolver::Resolve(F.Registry.Get(), Unknown);
	TestTrue(TEXT("Unknown identity receives neither icon nor visual ID"), !IconView.bRegisteredIdentity && IconView.Icon.IsNull() && IconView.VisualId.IsNone());
	TestTrue(TEXT("Unknown identity receives no mesh, level projection or authored transform"),
		!Mesh.bResolved && !Mesh.bRegisteredIdentity && Mesh.WorldMesh.IsNull() && Mesh.EquippedMesh.IsNull() &&
		Mesh.WorldTransform.Equals(FTransform::Identity));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0162PartialResourceTest,
	"Grimrock.RPG.ARMEMENT01.6.Resolver.PartialResourceFallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FARMEMENT0162PartialResourceTest::RunTest(const FString&)
{
	using namespace ARMEMENT0162;
	FFixture F;
	if (!F.Prepare(*this))
		return false;
	F.Visual->WorldMesh.Reset();
	F.Visual->EquippedMesh = TSoftObjectPtr<UStaticMesh>(EquippedMesh);
	if (!F.Register(*this))
		return false;

	const auto Item = MakeInstance(Sword, 7);
	const auto Mesh = FGridItemMeshResolver::Resolve(F.Registry.Get(), Item);
	const auto Ui = FGridWeaponVisualUiResolver::ResolveInventoryItemIcon(F.Registry.Get(), Item);
	TestTrue(TEXT("Known presentation can lack a world mesh"), Mesh.bResolved && Mesh.WorldMesh.IsNull());
	TestEqual(TEXT("Explicit equipped mesh stays distinct"), Mesh.EquippedMesh.ToSoftObjectPath(), EquippedMesh);
	TestEqual(TEXT("Authored icon remains"), Ui.Icon.ToSoftObjectPath(), Icon);

	F.Visual->ItemIcon.Reset();
	F.Visual->WorldMesh = TSoftObjectPtr<UStaticMesh>(WorldMesh);
	TestTrue(TEXT("Missing icon cannot be supplied by another silhouette"),
		FGridWeaponVisualUiResolver::ResolveInventoryItemIcon(F.Registry.Get(), Item).Icon.IsNull());
	TestEqual(TEXT("Mesh remains independently available"),
		FGridItemMeshResolver::Resolve(F.Registry.Get(), Item).WorldMesh.ToSoftObjectPath(), WorldMesh);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0162LookupFailuresTest,
	"Grimrock.RPG.ARMEMENT01.6.Resolver.MissingCatalogueAndBindings",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FARMEMENT0162LookupFailuresTest::RunTest(const FString&)
{
	using namespace ARMEMENT0162;
	for (const int32 Mode : {0, 1, 2})
	{
		FFixture F;
		if (!F.Prepare(*this))
			return false;
		auto* Catalogue = F.Root->WeaponVisualCatalogue.Get();
		const int32 Index = Catalogue->ArchetypeBindings.IndexOfByPredicate([&F](const FGridWeaponVisualBinding& Row)
		{
			return Row.ArchetypeId == F.SwordArchetype;
		});
		if (!TestTrue(TEXT("Sword visual binding exists before fault injection"), Index != INDEX_NONE))
			return false;
		if (Mode == 0)
			F.Root->WeaponVisualCatalogue = nullptr;
		else if (Mode == 1)
			Catalogue->ArchetypeBindings.RemoveAt(Index);
		else
			Catalogue->ArchetypeBindings[Index].VisualId = TEXT("VIS_0162_Invalid");

		TArray<FString> Errors;
		TestFalse(TEXT("Incomplete visual authority is refused before registration"), F.Registry->RegisterItemSystem(F.Root.Get(), Errors));
		TestTrue(TEXT("Refusal emits a diagnostic"), !Errors.IsEmpty());
		const auto Item = MakeInstance(Sword, 7);
		TestFalse(TEXT("Refused registration exposes no newly indexed identity"), F.Registry->ContainsItem(Sword));
		TestFalse(TEXT("Refused registration exposes no world mesh"), FGridItemMeshResolver::Resolve(F.Registry.Get(), Item).bResolved);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0162MalformedTest,
	"Grimrock.RPG.ARMEMENT01.6.Resolver.InvalidInstanceAndProfile",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FARMEMENT0162MalformedTest::RunTest(const FString&)
{
	using namespace ARMEMENT0162;
	FFixture F;
	if (!F.Prepare(*this) || !F.Register(*this))
		return false;

	for (const int32 Level : { -1, 0, 21 })
	{
		FGridResolvedItem Rejected;
		TestFalse(TEXT("Invalid physical level cannot resolve"), F.Registry->ResolveItem(Sword, Level, Rejected));
		TestTrue(TEXT("Explicit invalid-level diagnostic"), Rejected.Error == EGridItemResolveError::InvalidItemLevel);
		TestFalse(TEXT("Invalid level exposes no mesh"), FGridItemMeshResolver::Resolve(F.Registry.Get(), MakeInstance(Sword, Level)).bResolved);
		TestTrue(TEXT("Invalid level exposes no icon"),
			FGridWeaponVisualUiResolver::ResolveInventoryItemIcon(F.Registry.Get(), MakeInstance(Sword, Level)).Icon.IsNull());
	}
	auto Invalid = MakeInstance(Sword, 7);
	Invalid.RuntimeObjectId.Invalidate();
	TestFalse(TEXT("Malformed physical identity is rejected by the mesh view"), FGridItemMeshResolver::Resolve(F.Registry.Get(), Invalid).bResolved);
	TestFalse(TEXT("Malformed physical identity never reaches icon lookup"),
		FGridWeaponVisualUiResolver::ResolveInventoryItemIcon(F.Registry.Get(), Invalid).bRegisteredIdentity);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0162NoMutationTest,
	"Grimrock.RPG.ARMEMENT01.6.Resolver.ReadOnlyGameplay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FARMEMENT0162NoMutationTest::RunTest(const FString&)
{
	using namespace ARMEMENT0162;
	FFixture F;
	if (!F.Prepare(*this) || !F.Register(*this))
		return false;

	auto Item = MakeInstance(Sword, 7);
	Item.OwnerType = EGridItemOwnerType::CharacterInventory;
	Item.OwnerCharacterIndex = 2;
	const auto Before = Item;
	const FGridWeaponVisual BeforeVisual = *F.Visual;
	for (int32 Read = 0; Read < 2; ++Read)
	{
		TestEqual(TEXT("World read uses fixture mesh"),
			FGridItemMeshResolver::Resolve(F.Registry.Get(), Item).WorldMesh.ToSoftObjectPath(), WorldMesh);
		TestEqual(TEXT("Icon read uses fixture image"),
			FGridWeaponVisualUiResolver::ResolveInventoryItemIcon(F.Registry.Get(), Item).Icon.ToSoftObjectPath(), Icon);
	}
	TestTrue(TEXT("Visual reads preserve every physical instance field"),
		FGridItemInstance::StaticStruct()->CompareScriptStruct(&Before, &Item, 0));
	TestTrue(TEXT("Visual reads preserve authored values"),
		FGridWeaponVisual::StaticStruct()->CompareScriptStruct(&BeforeVisual, F.Visual, 0));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
