#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Core/GridLevelAsset.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGRaceAsset.h"
#include "RPG/RPGSkillAsset.h"
#include "RPG/RPGSkillPointService.h"
#include "RPG/RPGSkillService.h"
#include "RPG/RPGStartingEquipmentPresetCatalog.h"
#include "Runtime/GridGenericObjectActor.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Save/GrimrockPartySaveGame.h"

namespace ARMEMENT01163
{
    static URPGClassAsset* RogueClass()
    {
        return LoadObject<URPGClassAsset>(nullptr,
            TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Rogue.DA_Class_Rogue"));
    }

    static URPGRaceAsset* HumanRace()
    {
        return LoadObject<URPGRaceAsset>(nullptr,
            TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Race_Human.DA_Race_Human"));
    }

    static UGridItemDefinitionAsset* DisarmTools()
    {
        return LoadObject<UGridItemDefinitionAsset>(nullptr,
            TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_DisarmTools_01.DA_Item_DisarmTools_01"));
    }

    static URPGSkillAsset* TrapSkill()
    {
        return LoadObject<URPGSkillAsset>(nullptr,
            TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/Skills/DA_Skill_Traps.DA_Skill_Traps"));
    }

    static const FGridItemInstance* FindBagItem(const FGridCharacterInventoryState& Hero, FName Id)
    {
        for (const FGridInventorySlot& Slot : Hero.InventorySlots)
        {
            if (!Slot.IsEmpty() && Slot.Item.ItemDefinitionId == Id)
            {
                return &Slot.Item;
            }
        }
        return nullptr;
    }

    struct FWorldFixture
    {
        UWorld* World = nullptr;
        FWorldFixture()
        {
            const UWorld::InitializationValues Values = UWorld::InitializationValues()
                .AllowAudioPlayback(false).RequiresHitProxies(false)
                .CreatePhysicsScene(false).CreateNavigation(false)
                .CreateAISystem(false).ShouldSimulatePhysics(false)
                .SetTransactional(false);
            World = UWorld::CreateWorld(EWorldType::Game, false,
                FName(*FString::Printf(TEXT("ARM01163_%s"),
                    *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
                nullptr, true, ERHIFeatureLevel::Num, &Values);
            if (World && GEngine)
            {
                FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
                Context.SetCurrentWorld(World);
            }
            if (World) World->InitializeActorsForPlay(FURL());
        }
        ~FWorldFixture()
        {
            if (!World) return;
            World->DestroyWorld(false);
            if (GEngine) GEngine->DestroyWorldContext(World);
        }
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT01163SaboteurKitOnly,
    "Grimrock.RPG.ARMEMENT01.16.3.SaboteurKitOnly",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT01163SaboteurKitOnly::RunTest(const FString&)
{
    URPGClassAsset* Rogue = ARMEMENT01163::RogueClass();
    UGridItemDefinitionAsset* Tool = ARMEMENT01163::DisarmTools();
    if (!TestNotNull(TEXT("Production Rogue class"), Rogue) ||
        !TestNotNull(TEXT("Published disarm tool DataAsset"), Tool))
    {
        return false;
    }
    TestEqual(TEXT("Canonical reusable tool identity"), Tool->ItemDefinitionId,
        FName(TEXT("Item_DisarmTools_01")));
    TestTrue(TEXT("Tool has its real disarm gameplay tag"),
        Tool->ItemTags.Contains(FName(TEXT("Tool.Disarm"))));
    TestEqual(TEXT("Tool is not equipable in the hands"), Tool->HandUsage,
        EGridItemHandUsage::NotHandHeld);
    TestFalse(TEXT("One reusable tool, never a stack"), Tool->bStackable);
    TestTrue(TEXT("Real tool has a valid definition"), Tool->IsValidDefinition());

    const int32 OriginalCount = Rogue->StartingEquipmentKits.Num();
    const FName OriginalDefault = Rogue->DefaultStartingEquipmentKitId;
    URPGClassAsset* Preview = DuplicateObject<URPGClassAsset>(Rogue, GetTransientPackage());
    FString Error;
    if (!TestTrue(TEXT("Three production Rogue kits can be previewed"),
        Preview && FRPGStartingEquipmentPresetCatalog::TryPopulateApprovedTransientClass(*Preview, Error)))
    {
        AddError(Error);
        return false;
    }
    TestEqual(TEXT("Rogue production class remains untouched"),
        Rogue->StartingEquipmentKits.Num(), OriginalCount);
    TestEqual(TEXT("Rogue default remains untouched"),
        Rogue->DefaultStartingEquipmentKitId, OriginalDefault);
    TestEqual(TEXT("Rogue still has three approved orientations"), Preview->StartingEquipmentKits.Num(), 3);
    for (const FRPGStartingEquipmentKit& Kit : Preview->StartingEquipmentKits)
    {
        int32 ToolCount = 0;
        for (const FRPGStartingEquipmentEntry& Entry : Kit.Items)
        {
            if (Entry.ItemDefinition && Entry.ItemDefinition->ItemDefinitionId == Tool->ItemDefinitionId)
            {
                ++ToolCount;
                TestTrue(TEXT("Saboteur receives the published tool asset, not a clone"),
                    Entry.ItemDefinition.Get() == Tool);
                TestEqual(TEXT("Tool is in bag, not an equipment hand"),
                    Entry.TargetSlot, EGridEquipmentSlot::None);
                TestEqual(TEXT("One actual disarm tool is granted"), Entry.Quantity, 1);
                TestEqual(TEXT("Tool has no weapon item level"), Entry.ItemLevel, 0);
            }
        }
        const bool bSaboteur = Kit.KitId == FName(TEXT("Kit_Rogue_Saboteur"));
        TestEqual(TEXT("Only Saboteur receives disarm tools"), ToolCount, bSaboteur ? 1 : 0);
        if (bSaboteur)
        {
            TestEqual(TEXT("Saboteur keeps stiletto and receives one support item"), Kit.Items.Num(), 2);
            TestEqual(TEXT("Main hand retains the canonical stiletto"),
                Kit.Items[0].ItemDefinition->ItemDefinitionId, FName(TEXT("Weapon_Stiletto_01")));
            TestEqual(TEXT("Stiletto stays equipped"),
                Kit.Items[0].TargetSlot, EGridEquipmentSlot::MainHand);
        }
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT01163NewGameDisarmSave,
    "Grimrock.RPG.ARMEMENT01.16.3.N1DisarmAndSaveContinue",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT01163NewGameDisarmSave::RunTest(const FString&)
{
    URPGClassAsset* Rogue = ARMEMENT01163::RogueClass();
    URPGRaceAsset* Human = ARMEMENT01163::HumanRace();
    URPGSkillAsset* Traps = ARMEMENT01163::TrapSkill();
    UGridItemDefinitionAsset* Tool = ARMEMENT01163::DisarmTools();
    if (!TestNotNull(TEXT("Production Rogue"), Rogue) ||
        !TestNotNull(TEXT("Production Human"), Human) ||
        !TestNotNull(TEXT("Production Traps skill"), Traps) ||
        !TestNotNull(TEXT("Published physical tool"), Tool)) return false;
    if (!TestTrue(TEXT("Traps skill is valid"), Traps->IsValidDefinition())) return false;
    TestFalse(TEXT("N1 cannot disarm without learned Skill_Traps"),
        Traps->bAllowUntrainedChecks);

    ARMEMENT01163::FWorldFixture Fixture;
    if (!TestNotNull(TEXT("Transient game world"), Fixture.World)) return false;
    AGridLevelRuntimeActor* Runtime = Fixture.World->SpawnActor<AGridLevelRuntimeActor>();
    AGrimrockPartyPawn* Pawn = Fixture.World->SpawnActor<AGrimrockPartyPawn>();
    if (!TestNotNull(TEXT("Runtime"), Runtime) ||
        !TestNotNull(TEXT("Playable party pawn"), Pawn) ||
        !TestNotNull(TEXT("Real party inventory"), Pawn ? Pawn->PartyInventoryComponent.Get() : nullptr))
    {
        return false;
    }

    UGridPartyInventoryComponent* Inventory = Pawn->PartyInventoryComponent;
    Inventory->InitializeDefaultPartyIfNeeded();
    FRPGCharacterCreationRequest Request;
    Request.DisplayName = FText::FromString(TEXT("Saboteur N1"));
    Request.ClassDefinition = Rogue;
    Request.CombatActionSourceClassDefinition = Rogue;
    Request.RaceDefinition = Human;
    Request.StartingEquipmentKitId = TEXT("Kit_Rogue_Saboteur");
    FText CreationError;
    if (!TestTrue(TEXT("New Game builds a real Saboteur"),
        Inventory->CreateInitialCharacter(Request, CreationError)))
    {
        AddError(CreationError.ToString());
        return false;
    }
    const FGridCharacterInventoryState& Hero = Inventory->PartyInventoryState.ActiveCharacters[0];
    const FGridItemInstance* Tools = ARMEMENT01163::FindBagItem(Hero, Tool->ItemDefinitionId);
    const FGridItemInstance& Stiletto = Inventory->PartyInventoryState.ActiveEquipment[0].MainHand;
    if (!TestNotNull(TEXT("Tool is physically granted in the Saboteur bag"), Tools)) return false;
    const FGuid ToolGuid = Tools->RuntimeObjectId;
    const FGuid HeroGuid = Hero.CharacterId;
    TestTrue(TEXT("Tool GUID is valid"), ToolGuid.IsValid());
    TestEqual(TEXT("Physical tool is owned by created Saboteur"), Tools->OwnerGuid, HeroGuid);
    TestEqual(TEXT("Tool belongs to inventory, not equipment"),
        Tools->OwnerType, EGridItemOwnerType::CharacterInventory);
    TestEqual(TEXT("Tool retains quantity one"), Tools->Quantity, 1);
    TestEqual(TEXT("Starter tool definition is registered"),
        Inventory->FindItemDefinition(Tool->ItemDefinitionId), Tool);
    TestEqual(TEXT("Main-hand weapon unchanged"), Stiletto.ItemDefinitionId,
        FName(TEXT("Weapon_Stiletto_01")));
    TestEqual(TEXT("Kit ID saved on hero"), Hero.StartingEquipmentKitId,
        FName(TEXT("Kit_Rogue_Saboteur")));
    TestEqual(TEXT("No free trained Traps rank granted by the kit"),
        FRPGSkillService::GetSkillRank(Hero, Traps->SkillId), 0);

    UGridLevelAsset* Level = NewObject<UGridLevelAsset>(Runtime);
    Level->EnsureCellCount();
    Level->GetCellMutable(1, 1).CellType = EGridCellType::Floor;
    Runtime->LevelAsset = Level;
    Pawn->LevelRuntimeActor = Runtime;
    Pawn->CurrentCellX = 1;
    Pawn->CurrentCellY = 1;

    UGridWorldObjectDefinitionAsset* Definition =
        NewObject<UGridWorldObjectDefinitionAsset>(Runtime);
    Definition->DefinitionId = TEXT("ARM01163_Disarmable");
    Definition->SupportedType = EGridLevelObjectType::Trigger;
    Definition->DungeonTrap.bIsDungeonTrap = true;
    Definition->bCanBeDisarmed = true;
    Definition->DisarmDifficulty = 1; // Guaranteed once the trained d20 is allowed.
    Definition->RuntimeActorClass = AGridGenericObjectActor::StaticClass();
    Definition->StaticPart.Mesh = LoadObject<UStaticMesh>(nullptr,
        TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (!TestNotNull(TEXT("Visible trap mesh available"), Definition->StaticPart.Mesh.Get()))
        return false;
    Runtime->WorldObjectDefinitions.Add(Definition);

    FGridWorldObjectInstance Trap;
    Trap.InstanceId = FGuid::NewGuid();
    Trap.Type = EGridLevelObjectType::Trigger;
    Trap.WorldObjectDefinitionId = Definition->DefinitionId;
    Trap.CellX = 1;
    Trap.CellY = 1;
    Trap.InstanceConfig.DungeonTrap.bNoDamage = true;
    Level->WorldObjectInstances.Add(Trap);
    if (!TestTrue(TEXT("Disarming is only offered after real detection"),
        Runtime->TryRecordDungeonTrapDetection(Trap.InstanceId))) return false;

    FRandomStream Dice(163);
    FRPGSkillCheckResult Check;
    const int32 SeedBefore = Dice.GetCurrentSeed();
    TestFalse(TEXT("Untrained N1 Saboteur cannot roll a disarm check"),
        Runtime->TryDisarmTriggerTrap(Pawn, Trap.InstanceId, 0, Traps, Dice, Check));
    TestEqual(TEXT("Untrained rejection consumes no RNG"),
        Dice.GetCurrentSeed(), SeedBefore);
    TestEqual(TEXT("Refused attempt preserves actual tool"),
        Inventory->CountItemDefinitionInCharacterInventory(0, Tool->ItemDefinitionId), 1);

    // Spend one of the FOUR level-one skill points through the actual RPG
    // skill economy, not by silently adding rank/talent to the starting kit.
    FRPGSkillPointMutationResult Purchase;
    if (!TestTrue(TEXT("Saboteur can train Traps at level one"),
        FRPGSkillPointService::TryPurchaseNextRank(Inventory, 0, Traps, Purchase)))
        return false;
    TestEqual(TEXT("Purchased rank is one"), Purchase.NewRank, 1);
    TestEqual(TEXT("Three starting skill points remain"), Purchase.RemainingPoints, 3);
    TestTrue(TEXT("Rank-one trained Saboteur resolves and succeeds at DC1"),
        Runtime->TryDisarmTriggerTrap(Pawn, Trap.InstanceId, 0, Traps, Dice, Check));
    TestTrue(TEXT("Disarm skill check succeeds"), Check.bSuccess);
    TestEqual(TEXT("Disarm uses Dexterity"), Check.GoverningAttribute,
        ERPGSkillGoverningAttribute::Dexterity);
    TestTrue(TEXT("Trap is persistently disarmed"),
        Runtime->IsTriggerTrapDisarmed(Trap.InstanceId));
    TestEqual(TEXT("Tool is reusable after success"),
        Inventory->CountItemDefinitionInCharacterInventory(0, Tool->ItemDefinitionId), 1);

    UGrimrockPartySaveGame* Save = NewObject<UGrimrockPartySaveGame>();
    Save->PartyInventoryState = Inventory->PartyInventoryState;
    Save->DungeonRuntimeState = Runtime->DungeonRuntimeState;
    TArray<uint8> Bytes;
    if (!TestTrue(TEXT("A real SaveGame binary can be serialized"),
        UGameplayStatics::SaveGameToMemory(Save, Bytes))) return false;
    UGrimrockPartySaveGame* Loaded =
        Cast<UGrimrockPartySaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
    if (!TestNotNull(TEXT("SaveGame reload succeeds"), Loaded)) return false;
    const FGridCharacterInventoryState& SavedHero = Loaded->PartyInventoryState.ActiveCharacters[0];
    const FGridItemInstance* SavedTool = ARMEMENT01163::FindBagItem(SavedHero, Tool->ItemDefinitionId);
    if (!TestNotNull(TEXT("Tool survives real SaveGame binary roundtrip"), SavedTool)) return false;
    TestEqual(TEXT("Same tool GUID survives Save/Continue"), SavedTool->RuntimeObjectId, ToolGuid);
    TestEqual(TEXT("Same hero GUID survives Save/Continue"), SavedHero.CharacterId, HeroGuid);
    TestEqual(TEXT("Kit selection survives Save/Continue"), SavedHero.StartingEquipmentKitId,
        FName(TEXT("Kit_Rogue_Saboteur")));
    TestEqual(TEXT("Learned rank survives Save/Continue"),
        FRPGSkillService::GetSkillRank(SavedHero, Traps->SkillId), 1);

    UGridPartyInventoryComponent* Continued = NewObject<UGridPartyInventoryComponent>();
    Continued->InitializeDefaultPartyIfNeeded();
    FText RestoreError;
    if (!TestTrue(TEXT("Real party restore accepts saved Saboteur inventory"),
        Continued->RestorePartyInventoryState(Loaded->PartyInventoryState, RestoreError)))
    {
        AddError(RestoreError.ToString());
        return false;
    }
    TestEqual(TEXT("Continue leaves exactly one physical disarm tool"),
        Continued->CountItemDefinitionInCharacterInventory(0, Tool->ItemDefinitionId), 1);
    AGridLevelRuntimeActor* ContinuedRuntime = Fixture.World->SpawnActor<AGridLevelRuntimeActor>();
    if (!TestNotNull(TEXT("Continued runtime"), ContinuedRuntime)) return false;
    ContinuedRuntime->LevelAsset = Level;
    ContinuedRuntime->WorldObjectDefinitions.Add(Definition);
    ContinuedRuntime->DungeonRuntimeState = Loaded->DungeonRuntimeState;
    TestTrue(TEXT("Disarmed trap persists after SaveGame reload"),
        ContinuedRuntime->IsTriggerTrapDisarmed(Trap.InstanceId));
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
