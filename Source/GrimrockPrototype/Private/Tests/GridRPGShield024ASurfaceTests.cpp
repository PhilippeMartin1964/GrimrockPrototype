#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "RPG/RPGClassAsset.h"
#include "Runtime/Combat/GridCombatModifierResolver.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"

namespace Shield024A
{
	struct FTestWorld
	{
		UWorld* World = nullptr;
		FTestWorld()
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
				FName(*FString::Printf(TEXT("Shield024A_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
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

	struct FFixture
	{
		FTestWorld TestWorld;
		AGridLevelRuntimeActor* Runtime = nullptr;
		AGrimrockPartyPawn* Party = nullptr;
		UGridTurnManagerComponent* TurnManager = nullptr;
		UGridPartyInventoryComponent* Inventory = nullptr;
		UGridItemDefinitionAsset* ShieldDefinition = nullptr;

		bool Initialize(int32 BasePhysical = 2, int32 BaseMagical = 3, bool bEquipShield = true)
		{
			if (!TestWorld.World)
				return false;

			Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
			Party = TestWorld.World->SpawnActor<AGrimrockPartyPawn>();
			if (!Runtime || !Party || !Party->PartyInventoryComponent)
				return false;
			Inventory = Party->PartyInventoryComponent;

			UGridLevelAsset* Level = NewObject<UGridLevelAsset>(Runtime);
			Level->Width = 4;
			Level->Height = 4;
			Level->EnsureCellCount();
			for (FGridLevelCellData& Cell : Level->Cells)
			{
				Cell.CellType = EGridCellType::Floor;
				Cell.bBlocksOccupancy = false;
			}
			Runtime->bApplyLevelStartOnBeginPlay = false;
			Runtime->LevelAsset = Level;
			Runtime->CurrentDungeonLevelId = TEXT("Shield024ALevel");
			Party->LevelRuntimeActor = Runtime;
			Party->CurrentCellX = 1;
			Party->CurrentCellY = 1;

			URPGClassAsset* Class = NewObject<URPGClassAsset>(Runtime);
			Class->ClassId = TEXT("Shield024AClass");
			Class->DisplayName = FText::FromString(TEXT("Shield024A"));
			Class->HealthAtLevelOne = 20;
			Class->BasePhysicalArmor = BasePhysical;
			Class->BaseMagicalArmor = BaseMagical;
			if (!Class->IsValidDefinition())
				return false;

			FGridCharacterInventoryState Character;
			Character.CharacterId = FGuid::NewGuid();
			Character.ClassId = Class->ClassId;
			Character.ClassDefinition = Class;
			Character.DisplayName = FText::FromString(TEXT("Elias"));
			Character.Resources.CurrentHealth = 20;
			Character.Resources.CurrentMana = 7;
			Character.Resources.CurrentPhysicalArmor = BasePhysical;
			Character.Resources.CurrentMagicalArmor = BaseMagical;
			Character.DerivedStats.MaxHealth = 20;
			Character.DerivedStats.MaxMana = 12;
			Inventory->PartyInventoryState.ActiveCharacters = {Character};
			Inventory->PartyInventoryState.ActiveEquipment.SetNum(1);

			if (bEquipShield)
			{
				ShieldDefinition = LoadObject<UGridItemDefinitionAsset>(nullptr,
					TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_RoundShield_01.DA_Weapon_RoundShield_01"));
				if (!ShieldDefinition || !Inventory->RegisterItemDefinition(ShieldDefinition))
					return false;
				FGridItemInstance Shield;
				Shield.RuntimeObjectId = FGuid::NewGuid();
				Shield.ItemDefinitionId = ShieldDefinition->ItemDefinitionId;
				Shield.ItemLevel = 1;
				Shield.Quantity = 1;
				Shield.OwnerType = EGridItemOwnerType::EquipmentSlot;
				Shield.OwnerCharacterIndex = 0;
				Shield.EquipmentSlot = EGridEquipmentSlot::OffHand;
				Inventory->PartyInventoryState.ActiveEquipment[0].OffHand = Shield;
			}

			TurnManager = NewObject<UGridTurnManagerComponent>(Runtime, TEXT("Shield024ATurnManager"));
			TurnManager->bAutoInitialize = false;
			Runtime->AddInstanceComponent(TurnManager);
			TurnManager->RegisterComponent();
			if (!TurnManager->InitializeTurnManager(Runtime, Party))
				return false;
			TurnManager->bCombatActive = true;
			return true;
		}

		bool AddSurface(EGridCombatSurfaceType SurfaceType, EGridDamageType DamageType,
			int32 Damage, int32 CellX = 1, int32 CellY = 1)
		{
			FGridCombatSurfaceEffectProfile Profile;
			Profile.SurfaceType = SurfaceType;
			Profile.DurationRounds = 3;
			Profile.PeriodicDamageType = DamageType;
			Profile.PeriodicDamagePerRound = Damage;
			return Runtime->ApplyCombatSurfaceAtCell(CellX, CellY, Profile,
				FGuid::NewGuid(), TEXT("Shield024ASurface"), FGridResolvedCombatModifiers());
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridShield024APhysicalEquipmentSurfaceTest,
	"Grimrock.RPG.SHIELD02.4A.PhysicalSurfaceConsumesEquippedArmorBeforeHP",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridShield024APhysicalEquipmentSurfaceTest::RunTest(const FString&)
{
	Shield024A::FFixture Fixture;
	if (!TestTrue(TEXT("Surface runtime with real ARM075 N1 shield"), Fixture.Initialize()))
		return false;
	auto& Char = Fixture.Inventory->PartyInventoryState.ActiveCharacters[0];
	auto& Shield = Fixture.Inventory->PartyInventoryState.ActiveEquipment[0].OffHand;
	const FGuid ShieldId = Shield.RuntimeObjectId;
	const bool bShieldInitiallyDirty = Fixture.ShieldDefinition->GetOutermost()->IsDirty();
	TestEqual(TEXT("Production round shield N1 has 5 physical armor"), Fixture.ShieldDefinition->EquipmentStatBonus.ArmorBonus, 5);
	TestEqual(TEXT("Before surface: base 2 + shield 5"), Fixture.Inventory->GetEffectiveCurrentPhysicalArmor(0), 7);
	if (!TestTrue(TEXT("Physical periodic surface placed on party cell"),
		Fixture.AddSurface(EGridCombatSurfaceType::Oil, EGridDamageType::Physical, 4)))
		return false;

	Fixture.TurnManager->ResolveCombatSurfaceRound();
	TestEqual(TEXT("First tick: physical shield spends four"), Shield.AbsorbedEquipmentPhysicalArmor, 4);
	TestEqual(TEXT("First tick: base reserve stays intact"), Char.Resources.CurrentPhysicalArmor, 2);
	TestEqual(TEXT("First tick: only three physical armor remain"), Fixture.Inventory->GetEffectiveCurrentPhysicalArmor(0), 3);
	TestEqual(TEXT("First tick: HP protected"), Char.Resources.CurrentHealth, 20);
	TestEqual(TEXT("First tick: magical armor untouched"), Char.Resources.CurrentMagicalArmor, 3);

	// Replace the active surface with a second physical tick, 5 raw damage.
	if (!TestTrue(TEXT("Second physical tick replaces prior surface"),
		Fixture.AddSurface(EGridCombatSurfaceType::Oil, EGridDamageType::Physical, 5)))
		return false;
	Fixture.TurnManager->ResolveCombatSurfaceRound();
	TestEqual(TEXT("Second tick: shield exhausted"), Shield.AbsorbedEquipmentPhysicalArmor, 5);
	TestEqual(TEXT("Second tick: remaining base reserve exhausted"), Char.Resources.CurrentPhysicalArmor, 0);
	TestEqual(TEXT("Second tick: both pools add to zero"), Fixture.Inventory->GetEffectiveCurrentPhysicalArmor(0), 0);
	TestEqual(TEXT("Second tick: only the two excess damage reduce HP"), Char.Resources.CurrentHealth, 18);
	TestEqual(TEXT("Second tick: magical armor still untouched"), Char.Resources.CurrentMagicalArmor, 3);
	TestEqual(TEXT("Item instance was never replaced"), Shield.RuntimeObjectId, ShieldId);

	FGridInventoryCharacterSummary Summary;
	TestTrue(TEXT("UI read model generated"), Fixture.Inventory->GetCharacterSummary(0, Summary));
	TestEqual(TEXT("UI sees depleted effective armor"), Summary.Resources.CurrentPhysicalArmor, 0);
	TestEqual(TEXT("Production shield asset remains clean"), Fixture.ShieldDefinition->GetOutermost()->IsDirty(), bShieldInitiallyDirty);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridShield024AMagicalSurfaceTest,
	"Grimrock.RPG.SHIELD02.4A.MagicalSurfaceUsesOnlyMagicPool",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridShield024AMagicalSurfaceTest::RunTest(const FString&)
{
	Shield024A::FFixture Fixture;
	if (!TestTrue(TEXT("Surface runtime with equipped round shield"), Fixture.Initialize()))
		return false;
	auto& Char = Fixture.Inventory->PartyInventoryState.ActiveCharacters[0];
	auto& Shield = Fixture.Inventory->PartyInventoryState.ActiveEquipment[0].OffHand;
	if (!TestTrue(TEXT("Fire surface placed"),
		Fixture.AddSurface(EGridCombatSurfaceType::Fire, EGridDamageType::Fire, 2)))
		return false;

	Fixture.TurnManager->ResolveCombatSurfaceRound();
	TestEqual(TEXT("First fire tick: magic 3 -> 1"), Char.Resources.CurrentMagicalArmor, 1);
	TestEqual(TEXT("First fire tick: HP still 20"), Char.Resources.CurrentHealth, 20);
	TestEqual(TEXT("Fire does not spend shield"), Shield.AbsorbedEquipmentPhysicalArmor, 0);
	TestEqual(TEXT("Fire does not spend physical base"), Char.Resources.CurrentPhysicalArmor, 2);
	if (!TestTrue(TEXT("Second fire surface placed"),
		Fixture.AddSurface(EGridCombatSurfaceType::Fire, EGridDamageType::Fire, 4)))
		return false;
	Fixture.TurnManager->ResolveCombatSurfaceRound();
	TestEqual(TEXT("Second fire tick: magic exhausted"), Char.Resources.CurrentMagicalArmor, 0);
	TestEqual(TEXT("Second fire tick: excess three to HP"), Char.Resources.CurrentHealth, 17);
	TestEqual(TEXT("Magical surface never consumes shield"), Shield.AbsorbedEquipmentPhysicalArmor, 0);
	TestEqual(TEXT("Physical armor still base 2 plus shield 5"), Fixture.Inventory->GetEffectiveCurrentPhysicalArmor(0), 7);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridShield024AZeroArmorSurfaceTest,
	"Grimrock.RPG.SHIELD02.4A.ZeroArmorAndOffCellDoNotInventProtection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridShield024AZeroArmorSurfaceTest::RunTest(const FString&)
{
	Shield024A::FFixture Fixture;
	if (!TestTrue(TEXT("Runtime without armor or shield"), Fixture.Initialize(0, 0, false)))
		return false;
	auto& Char = Fixture.Inventory->PartyInventoryState.ActiveCharacters[0];
	if (!TestTrue(TEXT("Off-cell surface placed"),
		Fixture.AddSurface(EGridCombatSurfaceType::Oil, EGridDamageType::Physical, 3, 2, 1)))
		return false;
	Fixture.TurnManager->ResolveCombatSurfaceRound();
	TestEqual(TEXT("Off-cell surface does not damage party"), Char.Resources.CurrentHealth, 20);
	TestEqual(TEXT("No physical armor is invented"), Fixture.Inventory->GetEffectiveCurrentPhysicalArmor(0), 0);
	if (!TestTrue(TEXT("Zero-armor party cell surface placed"),
		Fixture.AddSurface(EGridCombatSurfaceType::Oil, EGridDamageType::Physical, 3)))
		return false;
	Fixture.TurnManager->ResolveCombatSurfaceRound();
	TestEqual(TEXT("All three physical damage to HP without armor"), Char.Resources.CurrentHealth, 17);
	TestEqual(TEXT("Physical pool stays zero"), Char.Resources.CurrentPhysicalArmor, 0);
	TestEqual(TEXT("Magical pool stays zero"), Char.Resources.CurrentMagicalArmor, 0);
	TestEqual(TEXT("Mana unchanged"), Char.Resources.CurrentMana, 7);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
