#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Core/GridLevelAsset.h"
#include "Core/GridObjectPaletteAsset.h"
#include "EditorTools/GridLevelEditorActor.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "../EditorTools/Widgets/GridEditorCanonicalPaletteReadModel.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridEditorClean064X28PointerlessPalette,
	"Grimrock.RPG.ARM.CLEAN06_4X28.PointerlessCanonicalPalettePlacement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorClean064X28PointerlessPalette::RunTest(const FString&)
{
	const UWorld::InitializationValues Values = UWorld::InitializationValues()
		.AllowAudioPlayback(false).RequiresHitProxies(false)
		.CreatePhysicsScene(false).CreateNavigation(false)
		.CreateAISystem(false).ShouldSimulatePhysics(false)
		.SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::EditorPreview, false,
		FName(*FString::Printf(TEXT("ARM_CLEAN064X28_%s"),
			*FGuid::NewGuid().ToString(EGuidFormats::Digits))),
		nullptr, true, ERHIFeatureLevel::Num, &Values);
	if (!TestNotNull(TEXT("Transient editor preview world"), World)) return false;
	if (GEngine)
	{
		GEngine->CreateNewWorldContext(EWorldType::EditorPreview)
			.SetCurrentWorld(World);
	}

	bool bOk = true;
	AGridLevelEditorActor* Editor = World->SpawnActor<AGridLevelEditorActor>();
	if (!TestNotNull(TEXT("Editor actor"), Editor))
	{
		World->DestroyWorld(false);
		if (GEngine) GEngine->DestroyWorldContext(World);
		return false;
	}
	UGridLevelAsset* Level = NewObject<UGridLevelAsset>(Editor);
	Level->Width = 4;
	Level->Height = 4;
	Level->EnsureCellCount();
	for (FGridLevelCellData& Cell : Level->Cells)
	{
		Cell.CellType = EGridCellType::Floor;
		Cell.bBlocksOccupancy = false;
	}
	Editor->LevelAsset = Level;
	Editor->SelectedCellX = 1;
	Editor->SelectedCellY = 1;
	Editor->SelectedEdge = EGridEdge::None;
	UGridObjectPaletteAsset* Palette = NewObject<UGridObjectPaletteAsset>(Editor);
	Editor->ObjectPalette = Palette;

	Palette->Entries.SetNum(2);
	FGridObjectPaletteEntry& Arm = Palette->Entries[0];
	Arm.EntryId = TEXT("X28_Arm");
	Arm.PaletteCategory = TEXT("Items");
	Arm.CanonicalItemId = TEXT("Weapon_Dagger_01");
	Arm.CanonicalDefaultItemLevel = 20;
	FGridObjectPaletteEntry& Ammo = Palette->Entries[1];
	Ammo.EntryId = TEXT("X28_Ammo");
	Ammo.PaletteCategory = TEXT("Items");
	Ammo.CanonicalItemId = TEXT("Item_Arrows_01");
	Ammo.CanonicalDefaultItemLevel = 0;

	TArray<FGridWorldObjectDefinitionValidationMessage> Messages;
	bOk &= TestTrue(TEXT("Pointerless ARM is a direct item entry"), Arm.IsDirectItemEntry());
	bOk &= TestTrue(TEXT("Pointerless ARM has structural validity"), Arm.IsValidEntry());
	bOk &= TestTrue(TEXT("Pointerless ammo is a direct item entry"), Ammo.IsDirectItemEntry());
	bOk &= TestTrue(TEXT("Both pointerless entries pass structural palette validation"),
		Palette->ValidatePalette(Messages));
	bOk &= TestEqual(TEXT("Pointerless palette has exactly two entries"),
		Palette->Entries.Num(), 2);
	const UGridItemSubsystem* Registry = Editor->ResolveEditorItemRegistry();
	bOk &= TestNotNull(TEXT("Configured canonical editor registry"), Registry);
	if (!bOk)
	{
		World->DestroyWorld(false);
		if (GEngine) GEngine->DestroyWorldContext(World);
		return false;
	}
	FGridResolvedItem ExpectedArm, ExpectedAmmo;
	if (!TestTrue(TEXT("Registered N20 ARM exists"),
			Registry->ResolveItem(Arm.CanonicalItemId, Arm.CanonicalDefaultItemLevel, ExpectedArm)) ||
		!TestTrue(TEXT("Registered N0 ammo exists"),
			Registry->ResolveItem(Ammo.CanonicalItemId, Ammo.CanonicalDefaultItemLevel, ExpectedAmmo)))
	{
		World->DestroyWorld(false);
		if (GEngine) GEngine->DestroyWorldContext(World);
		return false;
	}
	const FGridEditorCanonicalPaletteReadView ArmTile =
		FGridEditorCanonicalPaletteReadModel::Resolve(Arm, Registry);
	bOk &= TestTrue(TEXT("Pointerless palette tile resolves from ItemSystem"),
		ArmTile.bCanonicalSource && ArmTile.bResolved);
	bOk &= TestEqual(TEXT("Tile uses registered ID"), ArmTile.ItemId,
		Arm.CanonicalItemId);
	bOk &= TestEqual(TEXT("Tile uses canonical icon"), ArmTile.Icon.ToSoftObjectPath().ToString(),
		ExpectedArm.ItemIcon.ToSoftObjectPath().ToString());

	bOk &= TestTrue(TEXT("Select pointerless ARM from palette"),
		Editor->ApplyPaletteEntry(Arm.EntryId));
	bOk &= TestNull(TEXT("Selected ARM has no legacy definition"),
		FGridItemBehaviorParams::StaticStruct()->FindPropertyByName(TEXT("ItemDefinitionAsset")));
	Editor->PlaceSelectedObject();
	bOk &= TestEqual(TEXT("Place one pointerless ARM"), Level->LooseItemInstances.Num(), 1);
	if (Level->LooseItemInstances.Num() == 1)
	{
		const FGridLooseItemInstance& Placed = Level->LooseItemInstances[0];
		bOk &= TestEqual(TEXT("Placed ARM ItemId"), Placed.CanonicalItemId,
			Arm.CanonicalItemId);
		bOk &= TestEqual(TEXT("Placed ARM level N20"), Placed.ItemLevel, 20);
		bOk &= TestNull(TEXT("Placed ARM does not store an old definition"),
			FGridLooseItemInstance::StaticStruct()->FindPropertyByName(TEXT("ItemDefinition")));
		bOk &= TestTrue(TEXT("Placed ARM has stable GUID"), Placed.InstanceId.IsValid());
	}

	Editor->SelectedCellX = 2;
	Editor->SelectedCellY = 1;
	bOk &= TestTrue(TEXT("Select pointerless non-ARM from palette"),
		Editor->ApplyPaletteEntry(Ammo.EntryId));
	Editor->PlaceSelectedObject();
	bOk &= TestEqual(TEXT("Place second pointerless item"), Level->LooseItemInstances.Num(), 2);
	if (Level->LooseItemInstances.Num() == 2)
	{
		const FGridLooseItemInstance& Placed = Level->LooseItemInstances[1];
		bOk &= TestEqual(TEXT("Placed ammo is Item_Arrows_01"),
			Placed.CanonicalItemId, Ammo.CanonicalItemId);
		bOk &= TestEqual(TEXT("Placed ammo remains N0"), Placed.ItemLevel, 0);
		bOk &= TestNull(TEXT("Placed ammo is pointerless"), FGridLooseItemInstance::StaticStruct()->FindPropertyByName(TEXT("ItemDefinition")));

		// Label/notes edits must not reset independently authored physical levels.
		Editor->LastSelectedObjectId = Level->LooseItemInstances[0].InstanceId;
		bOk &= TestTrue(TEXT("Re-select same canonical ARM"),
			Editor->ApplyPaletteEntry(Arm.EntryId));
		Level->LooseItemInstances[0].ItemLevel = 12;
		bOk &= TestTrue(TEXT("Edit selected canonical item"),
			Editor->ApplyEditedSelectedObject());
		bOk &= TestEqual(TEXT("Authored N12 survived the edit"),
			Level->LooseItemInstances[0].ItemLevel, 12);
		bOk &= TestNull(TEXT("Edited ARM is still pointerless"),
			FGridLooseItemInstance::StaticStruct()->FindPropertyByName(TEXT("ItemDefinition")));
		const FName Sword(TEXT("Weapon_StraightSword_01"));
		bOk &= TestTrue(TEXT("Selected object changes ID without changing source palette"),
			Editor->SetSelectedObjectItemDefinitionId(Sword));
		bOk &= TestTrue(TEXT("Apply subsequent metadata edit"), Editor->ApplyEditedSelectedObject());
		bOk &= TestEqual(TEXT("Metadata cannot restore stale palette identity"),
			Level->LooseItemInstances[0].CanonicalItemId, Sword);
		bOk &= TestEqual(TEXT("Metadata preserves N12 after explicit ID change"),
			Level->LooseItemInstances[0].ItemLevel, 12);
		bOk &= TestTrue(TEXT("Restore test item ID explicitly"),
			Editor->SetSelectedObjectItemDefinitionId(Arm.CanonicalItemId));
	}

	const FGridObjectPaletteEntry OriginalArm = Arm;
	// The native palette and level no longer need the old authoring bridge.
	UGridItemDefinitionAsset* OldMatchingArm = NewObject<UGridItemDefinitionAsset>(Palette);
	OldMatchingArm->ItemDefinitionId = OriginalArm.CanonicalItemId;
	FGridObjectPaletteEntry OldBackedEntry = OriginalArm;
	OldBackedEntry.EntryId = TEXT("X44_MatchingLegacyBackedArm");
	OldBackedEntry.DefaultItemDefinition = OldMatchingArm;
	Palette->Entries.Add(OldBackedEntry);
	bOk &= TestFalse(TEXT("Even matching old palette pointer is rejected"),
		Editor->ApplyPaletteEntry(OldBackedEntry.EntryId));
	const int32 BeforeReject = Level->LooseItemInstances.Num();
	Editor->SelectedPaletteEntryId = OldBackedEntry.EntryId;
	Editor->PlaceSelectedObject();
	bOk &= TestEqual(TEXT("Bypassing palette selection cannot place legacy item"),
		Level->LooseItemInstances.Num(), BeforeReject);
	if (BeforeReject == 2)
	{
		FGridLooseItemInstance& First = Level->LooseItemInstances[0];
		Editor->LastSelectedObjectId = First.InstanceId;
		bOk &= TestTrue(TEXT("Reselect pointerless ARM"),
			Editor->ApplyPaletteEntry(OriginalArm.EntryId));
		Editor->SelectedPaletteEntryId = OldBackedEntry.EntryId;
		const FGuid PreviousId = First.InstanceId;
		bOk &= TestFalse(TEXT("Metadata apply rejects old-backed palette source"),
			Editor->ApplyEditedSelectedObject());
		bOk &= TestEqual(TEXT("Rejection preserves GUID"), First.InstanceId, PreviousId);
		bOk &= TestEqual(TEXT("Rejection preserves authored N12"), First.ItemLevel, 12);
		bOk &= TestTrue(TEXT("Restore valid palette selection"), Editor->ApplyPaletteEntry(OriginalArm.EntryId));
		bOk &= TestFalse(TEXT("Old pointer cannot be reattached"),
			Editor->SetSelectedObjectItemDefinitionAsset(OldMatchingArm));
		bOk &= TestNull(TEXT("Rejected reattachment leaves pointerless placement"),
			FGridLooseItemInstance::StaticStruct()->FindPropertyByName(TEXT("ItemDefinition")));
	}
	const int32 PlacedCount = Level->LooseItemInstances.Num();
	FGridObjectPaletteEntry Bad = OriginalArm;
	Bad.EntryId = TEXT("X28_InvalidN0");
	Bad.CanonicalDefaultItemLevel = 0;
	Palette->Entries.Add(Bad);
	bOk &= TestFalse(TEXT("Persisted registered ARM N0 cannot be selected"),
		Editor->ApplyPaletteEntry(Bad.EntryId));
	bOk &= TestEqual(TEXT("Rejected selection cannot add loose items"),
		Level->LooseItemInstances.Num(), PlacedCount);

	Bad.EntryId = TEXT("X28_Unknown");
	Bad.CanonicalItemId = TEXT("X28_UnregisteredItem");
	Bad.CanonicalDefaultItemLevel = 1;
	Palette->Entries.Add(Bad);
	bOk &= TestFalse(TEXT("Unknown ItemId cannot pass editor source registry"),
		Editor->ApplyPaletteEntry(Bad.EntryId));

	UGridItemDefinitionAsset* Wrong = NewObject<UGridItemDefinitionAsset>(Palette);
	Wrong->ItemDefinitionId = TEXT("Weapon_StraightSword_01");
	Bad = OriginalArm;
	Bad.EntryId = TEXT("X28_Contradictory");
	Bad.DefaultItemDefinition = Wrong;
	Palette->Entries.Add(Bad);
	bOk &= TestFalse(TEXT("Contradictory old item cannot be selected"),
		Editor->ApplyPaletteEntry(Bad.EntryId));

	World->DestroyWorld(false);
	if (GEngine) GEngine->DestroyWorldContext(World);
	return bOk && !HasAnyErrors();
}
#endif
