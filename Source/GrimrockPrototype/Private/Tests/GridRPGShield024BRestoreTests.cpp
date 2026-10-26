#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "RPG/RPGClassAsset.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"

namespace Shield024B
{
	struct FTestWorld
	{
		UWorld* World = nullptr;
		FTestWorld()
		{
			const UWorld::InitializationValues Values = UWorld::InitializationValues()
				.AllowAudioPlayback(false).RequiresHitProxies(false)
				.CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false)
				.ShouldSimulatePhysics(false).SetTransactional(false);
			World = UWorld::CreateWorld(EWorldType::Game, false,
				FName(*FString::Printf(TEXT("Shield024B_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
				nullptr, true, ERHIFeatureLevel::Num, &Values);
			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
				Context.SetCurrentWorld(World);
			}
		}
		~FTestWorld()
		{
			if (World)
			{
				World->DestroyWorld(false);
				if (GEngine)
				{
					GEngine->DestroyWorldContext(World);
				}
			}
		}
	};

	FGridCombatActionDefinition RestoreAction(FName Id, EGridCombatTargetingPolicy Targeting)
	{
		FGridCombatActionDefinition Action;
		Action.ActionId = Id;
		Action.DisplayName = FText::FromName(Id);
		Action.Description = FText::FromName(Id);
		Action.ActionType = EGridCombatActionType::Ability;
		Action.SourcePolicy = EGridCombatActionSourcePolicy::Ability;
		Action.TargetingPolicy = Targeting;
		Action.ResolutionProfile = EGridCombatActionResolutionProfile::Effect;
		Action.ActionPointCost = 1;

		FGridCombatArmorEffectProfile Physical;
		Physical.Pool = EGridCombatArmorPool::Physical;
		Physical.Operation = EGridCombatArmorEffectOperation::Restore;
		Physical.Magnitude = EGridCombatArmorEffectMagnitude::Flat;
		Physical.Trigger = EGridCombatArmorEffectTrigger::AfterResolution;
		Physical.Amount = 4;
		Action.ArmorEffects.Add(Physical);

		FGridCombatArmorEffectProfile Magical = Physical;
		Magical.Pool = EGridCombatArmorPool::Magical;
		Magical.Amount = 2;
		Action.ArmorEffects.Add(Magical);
		return Action;
	}

	struct FFixture
	{
		FTestWorld TestWorld;
		AGridLevelRuntimeActor* Runtime = nullptr;
		AGrimrockPartyPawn* Party = nullptr;
		UGridTurnManagerComponent* Manager = nullptr;
		URPGClassAsset* Class = nullptr;
		UGridItemDefinitionAsset* ShieldDef = nullptr;

		bool Initialize(EGridCombatTargetingPolicy Targeting)
		{
			if (!TestWorld.World)
				return false;
			Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
			Party = TestWorld.World->SpawnActor<AGrimrockPartyPawn>();
			AActor* Owner = TestWorld.World->SpawnActor<AActor>();
			if (!Runtime || !Party || !Owner || !Party->PartyInventoryComponent)
				return false;

			UGridLevelAsset* Level = NewObject<UGridLevelAsset>(Runtime);
			Level->Width = 4;
			Level->Height = 4;
			Level->EnsureCellCount();
			for (FGridLevelCellData& Cell : Level->Cells)
			{
				Cell.CellType = EGridCellType::Floor;
				Cell.bBlocksOccupancy = false;
			}
			Runtime->LevelAsset = Level;
			Party->LevelRuntimeActor = Runtime;
			Party->CurrentCellX = 1;
			Party->CurrentCellY = 1;
			Party->Facing = EGridEdge::North;
			Party->SnapToCurrentCell();

			Class = NewObject<URPGClassAsset>(GetTransientPackage());
			Class->ClassId = TEXT("Shield024BClass");
			Class->DisplayName = FText::FromString(TEXT("Shield restorative"));
			Class->HealthAtLevelOne = 20;
			Class->BasePhysicalArmor = 2;
			Class->BaseMagicalArmor = 3;
			Class->CombatActions = {RestoreAction(TEXT("Shield024BRestore"), Targeting)};
			if (!Class->IsValidDefinition() || !Class->CombatActions[0].IsValid())
				return false;

			auto& State = Party->PartyInventoryComponent->PartyInventoryState;
			for (int32 Index = 0; Index < 3; ++Index)
			{
				FGridCharacterInventoryState Char;
				Char.CharacterId = FGuid::NewGuid();
				Char.ClassId = Class->ClassId;
				Char.ClassDefinition = Class;
				Char.DisplayName = FText::FromString(FString::Printf(TEXT("Member %d"), Index));
				Char.Level = 1;
				Char.DerivedStats.MaxHealth = 20;
				Char.DerivedStats.MaxMana = 10;
				Char.Resources.CurrentHealth = Index == 2 ? 0 : 15 + Index;
				Char.Resources.CurrentMana = 7;
				Char.Resources.CurrentPhysicalArmor = 0;
				Char.Resources.CurrentMagicalArmor = 0;
				Char.InventorySlots.SetNum(2);
				State.ActiveCharacters.Add(Char);
			}
			State.ActiveEquipment.SetNum(3);

			ShieldDef = LoadObject<UGridItemDefinitionAsset>(nullptr,
				TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_RoundShield_01.DA_Weapon_RoundShield_01"));
			if (!ShieldDef || !Party->PartyInventoryComponent->RegisterItemDefinition(ShieldDef))
				return false;
			FGridItemInstance Shield;
			Shield.RuntimeObjectId = FGuid::NewGuid();
			Shield.ItemDefinitionId = ShieldDef->ItemDefinitionId;
			Shield.ItemLevel = 1;
			Shield.Quantity = 1;
			Shield.OwnerType = EGridItemOwnerType::EquipmentSlot;
			Shield.OwnerCharacterIndex = 1;
			Shield.EquipmentSlot = EGridEquipmentSlot::OffHand;
			Shield.AbsorbedEquipmentPhysicalArmor = 5;
			State.ActiveEquipment[1].OffHand = Shield;

			Manager = NewObject<UGridTurnManagerComponent>(Owner, TEXT("Shield024BManager"));
			if (!Manager || !Manager->InitializeTurnManager(Runtime, Party))
				return false;
			Manager->bCombatActive = true;
			Manager->CurrentPhase = EGridCombatPhase::PlayerPhase;
			Manager->RoundNumber = 1;

			FGridCombatantInitiativeEntry Entry;
			Entry.CombatantId = State.ActiveCharacters[0].CharacterId;
			Entry.Side = EGridCombatantSide::Party;
			Entry.CharacterIndex = 0;
			Entry.DisplayName = State.ActiveCharacters[0].DisplayName;
			Entry.InitiativeBase = 10;
			Entry.InitiativeRoll = 10;
			Entry.InitiativeTotal = 20;
			Entry.CurrentHealth = 15;
			Entry.MaximumHealth = 20;
			Entry.State = EGridCombatantTurnState::Active;
			Manager->InitiativeOrder = {Entry};
			Manager->CurrentInitiativeIndex = 0;

			FGridPlayerCharacterTurnState TurnState;
			TurnState.CharacterIndex = 0;
			TurnState.CharacterId = Entry.CombatantId;
			TurnState.State = EGridCombatantTurnState::Active;
			TurnState.MaximumActionPoints = 4;
			TurnState.RemainingActionPoints = 4;
			Manager->PlayerCharacterTurnStates = {TurnState};
			return true;
		}

		bool CastParty()
		{
			FGridCombatActionRequestResult Result;
			return Manager->RequestCharacterCombatAction(
				0, TEXT("Shield024BRestore"), EGridCombatActionSourcePolicy::Ability,
				Class->ClassId, EGridEquipmentSlot::None, Result) && Result.bAccepted;
		}

		bool CastOn(int32 Target)
		{
			FGridCombatActionRequestResult Result;
			return Manager->RequestCharacterCombatActionOnPartyTargets(
				0, TEXT("Shield024BRestore"), EGridCombatActionSourcePolicy::Ability,
				Class->ClassId, EGridEquipmentSlot::None, {Target}, Result) && Result.bAccepted;
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShield024BPartyRestorationTest,
	"Grimrock.RPG.SHIELD02.4B.PartyRestorationUsesEquippedArmorAndCaps",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShield024BPartyRestorationTest::RunTest(const FString&)
{
	Shield024B::FFixture Fixture;
	if (!TestTrue(TEXT("Party-class C3 fixture is valid"), Fixture.Initialize(EGridCombatTargetingPolicy::Party)))
		return false;
	UGridPartyInventoryComponent* Inv = Fixture.Party->PartyInventoryComponent;
	auto& State = Inv->PartyInventoryState;
	auto& Target = State.ActiveCharacters[1];
	auto& Shield = State.ActiveEquipment[1].OffHand;
	const FGuid Id = Shield.RuntimeObjectId;
	const bool bAssetDirty = Fixture.ShieldDef->GetOutermost()->IsDirty();
	const int32 SourceHP = State.ActiveCharacters[0].Resources.CurrentHealth;
	const int32 TargetHP = Target.Resources.CurrentHealth;
	TestEqual(TEXT("Shield N1 grants five physical armor"), Fixture.ShieldDef->EquipmentStatBonus.ArmorBonus, 5);
	TestEqual(TEXT("Initially target shield depleted"), Inv->GetEffectiveCurrentPhysicalArmor(1), 0);

	TestTrue(TEXT("Party restoration action accepted"), Fixture.CastParty());
	TestEqual(TEXT("Target shield regained four of five points"), Shield.AbsorbedEquipmentPhysicalArmor, 1);
	TestEqual(TEXT("Target base not falsely credited"), Target.Resources.CurrentPhysicalArmor, 0);
	TestEqual(TEXT("Target physical read model is four"), Inv->GetEffectiveCurrentPhysicalArmor(1), 4);
	TestEqual(TEXT("Target magical armor restored two"), Target.Resources.CurrentMagicalArmor, 2);
	TestEqual(TEXT("Caster physical base restored to reference two"), State.ActiveCharacters[0].Resources.CurrentPhysicalArmor, 2);
	TestEqual(TEXT("Caster magical armor restored two"), State.ActiveCharacters[0].Resources.CurrentMagicalArmor, 2);
	TestEqual(TEXT("Downed party member physical remains zero"), State.ActiveCharacters[2].Resources.CurrentPhysicalArmor, 0);
	TestEqual(TEXT("Downed party member magical remains zero"), State.ActiveCharacters[2].Resources.CurrentMagicalArmor, 0);
	TestEqual(TEXT("Source HP unchanged"), State.ActiveCharacters[0].Resources.CurrentHealth, SourceHP);
	TestEqual(TEXT("Target HP unchanged"), Target.Resources.CurrentHealth, TargetHP);
	TestEqual(TEXT("Target mana unchanged"), Target.Resources.CurrentMana, 7);

	TestTrue(TEXT("Second group action can restore remaining armor"), Fixture.CastParty());
	TestEqual(TEXT("Target shield fully refilled"), Shield.AbsorbedEquipmentPhysicalArmor, 0);
	TestEqual(TEXT("Target base refilled without exceeding class max"), Target.Resources.CurrentPhysicalArmor, 2);
	TestEqual(TEXT("Target total physical armor capped at seven"), Inv->GetEffectiveCurrentPhysicalArmor(1), 7);
	TestEqual(TEXT("Target magic capped at three"), Target.Resources.CurrentMagicalArmor, 3);
	TestEqual(TEXT("Caster physical remains at two"), State.ActiveCharacters[0].Resources.CurrentPhysicalArmor, 2);
	TestEqual(TEXT("Caster magic capped at three"), State.ActiveCharacters[0].Resources.CurrentMagicalArmor, 3);
	TestEqual(TEXT("Item instance ID unchanged"), Shield.RuntimeObjectId, Id);

	FGridInventoryCharacterSummary Summary;
	TestTrue(TEXT("Post-restore UI summary exists"), Inv->GetCharacterSummary(1, Summary));
	TestEqual(TEXT("UI sees complete target physical pool"), Summary.Resources.CurrentPhysicalArmor, 7);
	TestEqual(TEXT("UI sees restored target magic"), Summary.Resources.CurrentMagicalArmor, 3);
	TestEqual(TEXT("Production shield dataasset stays unmodified"), Fixture.ShieldDef->GetOutermost()->IsDirty(), bAssetDirty);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShield024BAllyTargetTest,
	"Grimrock.RPG.SHIELD02.4B.ExplicitAllyRestoresOnlySelectedCharacter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShield024BAllyTargetTest::RunTest(const FString&)
{
	Shield024B::FFixture Fixture;
	if (!TestTrue(TEXT("Targeted C3 fixture initialized"), Fixture.Initialize(EGridCombatTargetingPolicy::Ally)))
		return false;
	UGridPartyInventoryComponent* Inv = Fixture.Party->PartyInventoryComponent;
	auto& State = Inv->PartyInventoryState;
	auto& Shield = State.ActiveEquipment[1].OffHand;
	TestTrue(TEXT("Targeted Ally restoration accepted"), Fixture.CastOn(1));
	TestEqual(TEXT("Selected ally shield restored four"), Shield.AbsorbedEquipmentPhysicalArmor, 1);
	TestEqual(TEXT("Selected ally base stays empty until equipment restores"), State.ActiveCharacters[1].Resources.CurrentPhysicalArmor, 0);
	TestEqual(TEXT("Selected ally magic restored"), State.ActiveCharacters[1].Resources.CurrentMagicalArmor, 2);
	TestEqual(TEXT("Unselected caster base unchanged"), State.ActiveCharacters[0].Resources.CurrentPhysicalArmor, 0);
	TestEqual(TEXT("Unselected caster magic unchanged"), State.ActiveCharacters[0].Resources.CurrentMagicalArmor, 0);
	TestEqual(TEXT("Unselected downed character unchanged"), State.ActiveCharacters[2].Resources.CurrentPhysicalArmor, 0);
	TestEqual(TEXT("No HP repair on ally"), State.ActiveCharacters[1].Resources.CurrentHealth, 16);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
