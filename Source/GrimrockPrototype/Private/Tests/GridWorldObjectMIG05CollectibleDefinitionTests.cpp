#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Runtime/GridPlacementTransformResolver.h"

#include "Components/StaticMeshComponent.h"
#include "Core/GridLevelAsset.h"
#include "Core/GridObjectPaletteAsset.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Runtime/GridItemActor.h"
#include "GridCanonicalLootTestScope.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "UObject/UnrealType.h"

namespace
{
	struct FMIG05ItemTestWorld
	{
		UWorld* World = nullptr;

		FMIG05ItemTestWorld()
		{
			const UWorld::InitializationValues Values = UWorld::InitializationValues()
				.AllowAudioPlayback(false)
				.RequiresHitProxies(false)
				.CreatePhysicsScene(false)
				.CreateNavigation(false)
				.CreateAISystem(false)
				.ShouldSimulatePhysics(false)
				.SetTransactional(false);

			World = UWorld::CreateWorld(EWorldType::Game, false,
				FName(*FString::Printf(TEXT("MIG05ItemWorld_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true,
				ERHIFeatureLevel::Num, &Values);
			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
				Context.SetCurrentWorld(World);
			}
		}

		~FMIG05ItemTestWorld()
		{
			if (!World)
			{
				return;
			}
			World->DestroyWorld(false);
			if (GEngine)
			{
				GEngine->DestroyWorldContext(World);
			}
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridWorldObjectPaletteDisplayNameAuthorityTest,
	"Grimrock.WorldObjects.PALETTE_CLEAN01.DisplayNameAuthority",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridWorldObjectPaletteDisplayNameAuthorityTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	TestNull(TEXT("Palette entry has no display-name override property"),
		FGridObjectPaletteEntry::StaticStruct()->FindPropertyByName(TEXT("DisplayName") TEXT("Override")));

	UGridWorldObjectDefinitionAsset* WorldObjectDefinition = NewObject<UGridWorldObjectDefinitionAsset>();
	WorldObjectDefinition->DefinitionId = TEXT("Door_Test");
	WorldObjectDefinition->DisplayName = FText::FromString(TEXT("Door Alpha"));

	FGridObjectPaletteEntry WorldObjectEntry;
	WorldObjectEntry.EntryId = TEXT("Door_Entry");
	WorldObjectEntry.DefaultWorldObjectDefinition = WorldObjectDefinition;
	TestEqual(TEXT("World-object display name comes from its definition"), WorldObjectEntry.GetEffectiveDisplayName().ToString(), FString(TEXT("Door Alpha")));

	WorldObjectDefinition->DisplayName = FText::FromString(TEXT("Door Beta"));
	TestEqual(TEXT("World-object display name immediately follows its definition"), WorldObjectEntry.GetEffectiveDisplayName().ToString(), FString(TEXT("Door Beta")));

	WorldObjectDefinition->DisplayName = FText::GetEmpty();
	TestEqual(TEXT("World-object display name falls back to DefinitionId"), WorldObjectEntry.GetEffectiveDisplayName().ToString(), FString(TEXT("Door_Test")));

	const FName ItemId(TEXT("Item_Test"));

	FGridObjectPaletteEntry ItemEntry;
	ItemEntry.EntryId = TEXT("Item_Entry");
	ItemEntry.ItemId = ItemId;
	TestEqual(TEXT("Direct-item context-free name is stable ID"), ItemEntry.GetEffectiveDisplayName().ToString(), FString(TEXT("Item_Test")));

	TestEqual(TEXT("Legacy label changes cannot affect direct entry"), ItemEntry.GetEffectiveDisplayName().ToString(), FString(TEXT("Item_Test")));

	TestEqual(TEXT("Direct-item display name falls back to ItemDefinitionId"), ItemEntry.GetEffectiveDisplayName().ToString(), FString(TEXT("Item_Test")));

	const UGridObjectPaletteAsset* ProductionPalette = LoadObject<UGridObjectPaletteAsset>(
		nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/DA_ObjectPalette_Default.DA_ObjectPalette_Default"));
	TestNotNull(TEXT("Production palette loads"), ProductionPalette);
	if (ProductionPalette)
	{
		TestTrue(TEXT("Production palette contains entries"), !ProductionPalette->Entries.IsEmpty());
		for (const FGridObjectPaletteEntry& ProductionEntry : ProductionPalette->Entries)
		{
			if (!ProductionEntry.IsValidEntry())
			{
				continue;
			}

			// Pointerless direct entries are displayed by the canonical Slate read model
			// (X.14/X.55); this raw helper has only the stable identity fallback.
			if (ProductionEntry.IsDirectItemEntry())
			{
				TestFalse(TEXT("Pointerless direct entry has a stable ID"), ProductionEntry.ItemId.IsNone());
				TestEqual(TEXT("Raw context-free helper returns the stable ID"),
					ProductionEntry.GetEffectiveDisplayName().ToString(), ProductionEntry.ItemId.ToString());
				continue;
			}

			const FText DefinitionDisplayName = ProductionEntry.DefaultWorldObjectDefinition->DisplayName;
			const FName DefinitionId = ProductionEntry.DefaultWorldObjectDefinition->DefinitionId;
			const FText ExpectedDisplayName = !DefinitionDisplayName.IsEmpty()
				? DefinitionDisplayName
				: FText::FromName(!DefinitionId.IsNone() ? DefinitionId : ProductionEntry.EntryId);

			TestFalse(*FString::Printf(TEXT("Production entry %s has a display name"), *ProductionEntry.EntryId.ToString()),
				ProductionEntry.GetEffectiveDisplayName().IsEmpty());
			TestEqual(*FString::Printf(TEXT("Production entry %s uses its definition display name"), *ProductionEntry.EntryId.ToString()),
				ProductionEntry.GetEffectiveDisplayName().ToString(), ExpectedDisplayName.ToString());
		}
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridWorldObjectMIG05DirectCollectibleDefinitionTest,
	"Grimrock.WorldObjects.MIG05.DirectCollectibleDefinition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridWorldObjectMIG05DirectCollectibleDefinitionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const FName ItemId(TEXT("BlueGem"));
	UStaticMesh* WorldMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (!TestNotNull(TEXT("Fixture world mesh"), WorldMesh)) return false;

	FGridObjectPaletteEntry Entry;
	Entry.EntryId = TEXT("BlueGem");
	Entry.ItemId = ItemId;

	TestTrue(TEXT("Direct collectible palette entry is valid without an object definition"), Entry.IsValidEntry());
	TestTrue(TEXT("Direct collectible entry owns no companion definition"), Entry.DefaultWorldObjectDefinition == nullptr);
	TestTrue(TEXT("Direct collectible entry does not duplicate the definition icon"), Entry.Icon == nullptr);
	TestEqual(TEXT("Direct collectible entry resolves to Item"), Entry.GetEffectiveObjectType(), EGridLevelObjectType::Item);
	TestTrue(TEXT("Direct collectible entry has no effective WorldObjectDefinitionId"), Entry.GetEffectiveWorldObjectDefinitionId().IsNone());
	Entry.PaletteCategory = FName(TEXT("Items"));
	TestEqual(TEXT("Direct collectible entry uses its palette category"), Entry.GetPaletteCategory(), FName(TEXT("Items")));
	TestEqual(TEXT("Direct collectible context-free name uses ItemId"), Entry.GetEffectiveDisplayName().ToString(), ItemId.ToString());

	UGridObjectPaletteAsset* Palette = NewObject<UGridObjectPaletteAsset>();
	Palette->Entries.Add(Entry);
	TArray<FGridWorldObjectDefinitionValidationMessage> ValidationMessages;
	TestTrue(TEXT("Palette accepts a direct ItemDefinition collectible"), Palette->ValidatePalette(ValidationMessages));
	TestEqual(TEXT("Direct collectible palette validation has no messages"), ValidationMessages.Num(), 0);

	FProperty* LegacyItemIdentity = AGridItemActor::StaticClass()->FindPropertyByName(TEXT("ArchetypeId"));
	TestNull(TEXT("MIG09 physically removes duplicate item identity"), LegacyItemIdentity);
	TestNull(TEXT("MIG09 removes legacy GetItemArchetypeId API"), AGridItemActor::StaticClass()->FindFunctionByName(TEXT("GetItemArchetypeId")));
	TestNull(TEXT("MIG09 removes legacy InitializeItem API"), AGridItemActor::StaticClass()->FindFunctionByName(TEXT("InitializeItem")));

	FMIG05ItemTestWorld TestWorld;
	TestNotNull(TEXT("MIG05 runtime world exists"), TestWorld.World);
	if (!TestWorld.World)
	{
		return false;
	}

	FGridCanonicalLootTestScope Scope;
	if (!TestTrue(TEXT("Isolated item system registered"), Scope.AttachWithWeights(TestWorld.World, {},
		[&](UGridItemSystemAsset& Root)
		{
			auto* Definition = NewObject<UGridNonArmamentDefinitionAsset>(&Root);
			Definition->ItemId = ItemId;
			Definition->DisplayName = FText::FromString(TEXT("Blue Gem"));
			Definition->Icon = LoadObject<UTexture2D>(nullptr, TEXT("/Engine/EngineResources/WhiteSquareTexture.WhiteSquareTexture"));
			Definition->WorldMesh = WorldMesh;
			Root.ItemDefinitions.Add(Definition);
		})))
	{
		return false;
	}

	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	TestNotNull(TEXT("MIG05 runtime actor exists"), Runtime);
	if (!Runtime)
	{
		return false;
	}
	Runtime->LevelAsset = NewObject<UGridLevelAsset>(Runtime);
	Runtime->LevelAsset->Width = 1;
	Runtime->LevelAsset->Height = 1;
	Runtime->LevelAsset->CellSize = 200.0f;
	Runtime->LevelAsset->EnsureCellCount();

	FGridLooseItemInstance ItemData;
	ItemData.InstanceId = FGuid::NewGuid();
	ItemData.CellX = 0;
	ItemData.CellY = 0;
	ItemData.ItemId = ItemId;

	FTransform PlacementTransform;
	TestTrue(TEXT("Loose item placement transform resolves without any WorldObjectDefinition"), GridPlacementTransformResolver::ResolveLooseItem(*Runtime, ItemData, PlacementTransform));
	TestTrue(TEXT("Direct loose item remains centered on its cell"),
		PlacementTransform.GetLocation().Equals(Runtime->GetCellCenterWorld(0, 0, 12.0f), KINDA_SMALL_NUMBER));

	AGridItemActor* ItemActor = TestWorld.World->SpawnActor<AGridItemActor>();
	TestNotNull(TEXT("Generic item actor exists"), ItemActor);
	if (ItemActor)
	{
		FGridItemInstance Instance;
		Instance.ItemDefinitionId = ItemId;
		Instance.RuntimeObjectId = ItemData.InstanceId;
		Instance.Quantity = 1;
		Instance.OwnerType = EGridItemOwnerType::World;
		if (!TestTrue(TEXT("Physical gem initializes"), ItemActor->InitializeFromItemInstance(Instance, Scope.Registry))) return false;
		FGridResolvedItem Resolved;
		TestTrue(TEXT("Collectible icon lives in the registry"),
			Scope.Registry->ResolveItem(ItemId, 0, Resolved) &&
			Resolved.ItemIcon.ToSoftObjectPath().ToString() == TEXT("/Engine/EngineResources/WhiteSquareTexture.WhiteSquareTexture"));
		TestEqual(TEXT("Generic item actor identity is ItemDefinitionId"), ItemActor->GetItemDefinitionId(), FName(TEXT("BlueGem")));
		TestTrue(TEXT("Generic item actor presentation uses ItemDefinition WorldMesh"), ItemActor->MeshComponent->GetStaticMesh() == WorldMesh);

		ItemActor->ItemTags.Add(TEXT("LegacyTag"));
		ItemActor->InitializeFromItemDefinitionId(TEXT("LegacyOnly"), FGuid());
		TestEqual(TEXT("ID-only initialization owns the canonical identity"), ItemActor->GetItemDefinitionId(), FName(TEXT("LegacyOnly")));
		TestEqual(TEXT("ID-only initialization clears stale definition tags"), ItemActor->ItemTags.Num(), 0);
		TestTrue(TEXT("ID-only initialization preserves Blueprint/item presentation when no definition mesh is available"),
			ItemActor->MeshComponent->GetStaticMesh() == WorldMesh);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridWorldObjectMIG05PaletteAmbiguityTest,
	"Grimrock.WorldObjects.MIG05.PaletteRejectsDualDefinition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridWorldObjectMIG05PaletteAmbiguityTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const FName ItemId(TEXT("CopperKey"));

	UGridWorldObjectDefinitionAsset* LegacyDefinition = NewObject<UGridWorldObjectDefinitionAsset>();
	LegacyDefinition->DefinitionId = TEXT("Item_CopperKey_Legacy");
	LegacyDefinition->SupportedType = EGridLevelObjectType::Item;

	FGridObjectPaletteEntry Entry;
	Entry.EntryId = TEXT("CopperKey");
	Entry.ItemId = ItemId;
	Entry.DefaultWorldObjectDefinition = LegacyDefinition;

	UGridObjectPaletteAsset* Palette = NewObject<UGridObjectPaletteAsset>();
	Palette->Entries.Add(Entry);
	TArray<FGridWorldObjectDefinitionValidationMessage> Messages;
	TestFalse(TEXT("Palette rejects two authoring definitions for one collectible"), Palette->ValidatePalette(Messages));
	TestTrue(TEXT("Dual-definition palette entry emits an error"),
		Messages.ContainsByPredicate([](const FGridWorldObjectDefinitionValidationMessage& Message)
		{
			return Message.Severity == EGridWorldObjectDefinitionValidationSeverity::Error;
		}));

	TestFalse(TEXT("Dual-definition entry is invalid"), Entry.IsValidEntry());
	Palette->Entries[0].ItemId = NAME_None;
	TestFalse(TEXT("Item WorldObjectDefinition alone is not a valid palette entry"), Palette->Entries[0].IsValidEntry());
	TestFalse(TEXT("Palette rejects Item WorldObjectDefinition without a direct ItemDefinition"), Palette->ValidatePalette(Messages));
	TestTrue(TEXT("Legacy collectible palette rejection directs authors to ItemId"),
		Messages.ContainsByPredicate([](const FGridWorldObjectDefinitionValidationMessage& Message)
		{
			return Message.Severity == EGridWorldObjectDefinitionValidationSeverity::Error && Message.Message.Contains(TEXT("Use ItemId instead"));
		}));

	TestFalse(TEXT("WorldObjectDefinition rejects collectible Item type"), LegacyDefinition->ValidateDefinition(Messages));
	TestTrue(TEXT("Collectible type rejection explicitly names the typed replacement"),
		Messages.ContainsByPredicate([](const FGridWorldObjectDefinitionValidationMessage& Message)
		{
			return Message.Severity == EGridWorldObjectDefinitionValidationSeverity::Error && Message.Message.Contains(TEXT("cannot represent a collectible Item"));
		}));

	// The duplicate identity is now impossible to encode, even on a non-Item world definition.
	TestNull(TEXT("Historical item asset property removed"), FGridItemBehaviorParams::StaticStruct()->FindPropertyByName(TEXT("ItemDefinitionAsset")));
	TestNull(TEXT("Historical item ID property removed"), FGridItemBehaviorParams::StaticStruct()->FindPropertyByName(TEXT("ItemDefinitionId")));
	LegacyDefinition->SupportedType = EGridLevelObjectType::Decoration;

	FMIG05ItemTestWorld TestWorld;
	if (!TestNotNull(TEXT("Item resolution rejection test world exists"), TestWorld.World)) return false;
	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	if (!TestNotNull(TEXT("Item resolution rejection runtime exists"), Runtime)) return false;
	Runtime->WorldObjectDefinitions.Add(LegacyDefinition);
	UGridLevelAsset* EmptyLevel = NewObject<UGridLevelAsset>(Runtime);
	EmptyLevel->Width = 2;
	EmptyLevel->Height = 2;
	EmptyLevel->EnsureCellCount();
	for (FGridLevelCellData& Cell : EmptyLevel->Cells)
	{
		Cell.CellType = EGridCellType::Floor;
		Cell.bBlocksOccupancy = false;
	}
	Runtime->LevelAsset = EmptyLevel;
	FGridItemInstance Unknown;
	Unknown.RuntimeObjectId = FGuid::NewGuid();
	Unknown.ItemDefinitionId = ItemId;
	Unknown.Quantity = 1;
	Unknown.ItemLevel = 0;
	TestFalse(TEXT("Runtime cannot drop an item through WorldObjectDefinition alone"),
		Runtime->TryDropItemInstanceAtCell(Unknown, 0, 0, EGridEdge::None, FVector::ZeroVector));

	LegacyDefinition->SupportedType = EGridLevelObjectType::Receptacle;
	LegacyDefinition->DefaultBehavior.Receptacle.InitialContent.AddDefaulted_GetRef().ItemId = ItemId;
	TestFalse(TEXT("Receptacle canonical ID cannot supply an obsolete runtime item definition"),
		Runtime->TryDropItemInstanceAtCell(Unknown, 0, 0, EGridEdge::None, FVector::ZeroVector));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridWorldObjectMIG05PaletteIconAuthorityTest,
	"Grimrock.WorldObjects.MIG05.PaletteRejectsCollectibleIconDuplication",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridWorldObjectMIG05PaletteIconAuthorityTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const FName ItemId(TEXT("SilverKey"));

	FGridObjectPaletteEntry Entry;
	Entry.EntryId = TEXT("SilverKey");
	Entry.ItemId = ItemId;
	Entry.Icon = NewObject<UTexture2D>();

	TestFalse(TEXT("Direct collectible entry is invalid when Palette Icon duplicates ItemDefinition.Icon"), Entry.IsValidEntry());

	UGridObjectPaletteAsset* Palette = NewObject<UGridObjectPaletteAsset>();
	Palette->Entries.Add(Entry);
	TArray<FGridWorldObjectDefinitionValidationMessage> Messages;
	TestFalse(TEXT("Palette rejects duplicated collectible icon authority"), Palette->ValidatePalette(Messages));
	TestTrue(TEXT("Collectible icon duplication emits a specific validation error"),
		Messages.ContainsByPredicate([](const FGridWorldObjectDefinitionValidationMessage& Message)
		{
			return Message.Severity == EGridWorldObjectDefinitionValidationSeverity::Error && Message.Message.Contains(TEXT("item icon from ItemSystem"));
		}));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
