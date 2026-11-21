#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "RPG/RPGClassAsset.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Runtime/GridPartyInventoryComponent.h"

namespace Shield022
{
	struct FWorldFixture
	{
		UWorld* World = nullptr;
		UGameInstance* GI = nullptr;
		UGridItemSubsystem* Registry = nullptr;
		UGridPartyInventoryComponent* Inventory = nullptr;

		FWorldFixture()
		{
			const UWorld::InitializationValues Values = UWorld::InitializationValues()
				.AllowAudioPlayback(false).RequiresHitProxies(false)
				.CreatePhysicsScene(false).CreateNavigation(false)
				.CreateAISystem(false).ShouldSimulatePhysics(false)
				.SetTransactional(false);
			World = UWorld::CreateWorld(EWorldType::Game, false,
				FName(*FString::Printf(TEXT("ARMH35_Shield022_%s"),
					*FGuid::NewGuid().ToString(EGuidFormats::Digits))),
				nullptr, true, ERHIFeatureLevel::Num, &Values);
			if (!World || !GEngine) return;
			GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
			GI = NewObject<UGameInstance>(GetTransientPackage());
			GI->Init();
			World->SetGameInstance(GI);
			Registry = GI->GetSubsystem<UGridItemSubsystem>();
			AGrimrockPartyPawn* Pawn = World->SpawnActor<AGrimrockPartyPawn>();
			Inventory = Pawn ? Pawn->PartyInventoryComponent.Get() : nullptr;
			if (Inventory) Inventory->PartyInventoryState = FGridPartyInventoryState();
		}
		~FWorldFixture()
		{
			if (World) World->SetGameInstance(nullptr);
			if (GI) GI->Shutdown();
			if (World)
			{
				World->DestroyWorld(false);
				if (GEngine) GEngine->DestroyWorldContext(World);
			}
		}

		bool ResolveShield(FGridResolvedItem& Out) const
		{
			return Registry && Registry->ResolveItem(TEXT("Weapon_RoundShield_01"), 1, Out)
				&& Out.bIsArmament && Out.Weapon.PhysicalArmor == 5;
		}
	};

	URPGClassAsset* MakeClass(UObject* Outer, FName Id, int32 Physical, int32 Magical)
	{
		URPGClassAsset* Result = NewObject<URPGClassAsset>(Outer);
		Result->ClassId = Id;
		Result->DisplayName = FText::FromName(Id);
		Result->HealthAtLevelOne = 20;
		Result->BasePhysicalArmor = Physical;
		Result->BaseMagicalArmor = Magical;
		return Result;
	}

	FGridCharacterInventoryState MakeCharacter(URPGClassAsset* Definition, int32 Health)
	{
		FGridCharacterInventoryState Result;
		Result.CharacterId = FGuid::NewGuid();
		Result.ClassId = Definition->ClassId;
		Result.ClassDefinition = Definition;
		Result.Level = 1;
		Result.Resources.CurrentHealth = Health;
		Result.Resources.CurrentMana = 7;
		Result.Resources.CurrentPhysicalArmor = 0;
		Result.Resources.CurrentMagicalArmor = 1;
		Result.DerivedStats.MaxHealth = 20;
		Result.DerivedStats.MaxMana = 12;
		Result.InventorySlots.SetNum(2);
		return Result;
	}

	FGridItemInstance MakeItem(FName ItemId, int32 Spent)
	{
		FGridItemInstance Item;
		Item.RuntimeObjectId = FGuid::NewGuid();
		Item.ItemDefinitionId = ItemId;
		Item.Quantity = 1;
		Item.ItemLevel = 1;
		Item.AbsorbedEquipmentPhysicalArmor = Spent;
		return Item;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShield022VictoryRefillTest,
	"Grimrock.RPG.SHIELD02.2.VictoryRefillsPhysicalAndMagical",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShield022VictoryRefillTest::RunTest(const FString&)
{
	Shield022::FWorldFixture Fixture;
	FGridResolvedItem Shield;
	const FName ShieldId(TEXT("Weapon_RoundShield_01"));
	if (!TestNotNull(TEXT("Canonical game world"), Fixture.World) ||
		!TestNotNull(TEXT("Real party inventory"), Fixture.Inventory) ||
		!TestTrue(TEXT("Real ARM075 N1 resolves from ItemSystem"), Fixture.ResolveShield(Shield)))
		return false;
	TestEqual(TEXT("Round shield grants five physical armor"), Shield.Weapon.PhysicalArmor, 5);
	UGridPartyInventoryComponent* Inventory = Fixture.Inventory;
	URPGClassAsset* Class = Shield022::MakeClass(Inventory, TEXT("Shield022VictoryClass"), 2, 3);
	if (!TestTrue(TEXT("Synthetic class valid"), Class->IsValidDefinition()) ||
		!TestNull(TEXT("No historical shield definition registered"), Inventory->FindItemDefinition(ShieldId)))
		return false;

	Inventory->PartyInventoryState.ActiveCharacters.Add(Shield022::MakeCharacter(Class, 14));
	Inventory->PartyInventoryState.ActiveEquipment.SetNum(1);
	auto& State = Inventory->PartyInventoryState;
	State.ActiveEquipment[0].OffHand = Shield022::MakeItem(ShieldId, 5);
	State.ActiveEquipment[0].OffHand.OwnerType = EGridItemOwnerType::EquipmentSlot;
	State.ActiveEquipment[0].OffHand.OwnerCharacterIndex = 0;
	State.ActiveEquipment[0].OffHand.EquipmentSlot = EGridEquipmentSlot::OffHand;
	State.ActiveCharacters[0].InventorySlots[0].bOccupied = true;
	State.ActiveCharacters[0].InventorySlots[0].Item = Shield022::MakeItem(ShieldId, 4);
	State.ActiveCharacters[0].InventorySlots[0].Item.OwnerType = EGridItemOwnerType::CharacterInventory;
	State.ActiveCharacters[0].InventorySlots[0].Item.OwnerCharacterIndex = 0;
	State.bHasCursorItem = true;
	State.CursorItem = Shield022::MakeItem(ShieldId, 2);
	State.CursorItem.OwnerType = EGridItemOwnerType::Cursor;

	const FGuid EquippedId = State.ActiveEquipment[0].OffHand.RuntimeObjectId;
	const FGuid BagId = State.ActiveCharacters[0].InventorySlots[0].Item.RuntimeObjectId;
	const FGuid CursorId = State.CursorItem.RuntimeObjectId;
	TestEqual(TEXT("Physical armor depleted before Victory"), Inventory->GetEffectiveCurrentPhysicalArmor(0), 0);

	TestEqual(TEXT("One living character changed by refill"), Inventory->RefillArmorReservesAfterVictory(), 1);
	TestEqual(TEXT("Equipped armor fully restored"), State.ActiveEquipment[0].OffHand.AbsorbedEquipmentPhysicalArmor, 0);
	TestEqual(TEXT("Unworn shield in bag fully restored"), State.ActiveCharacters[0].InventorySlots[0].Item.AbsorbedEquipmentPhysicalArmor, 0);
	TestEqual(TEXT("Party-held cursor item fully restored"), State.CursorItem.AbsorbedEquipmentPhysicalArmor, 0);
	TestEqual(TEXT("Class base physical reserve restored"), State.ActiveCharacters[0].Resources.CurrentPhysicalArmor, 2);
	TestEqual(TEXT("Magical armor restored to class reference"), State.ActiveCharacters[0].Resources.CurrentMagicalArmor, 3);
	TestEqual(TEXT("Physical armor sums class 2 and equipped shield 5"), Inventory->GetEffectiveCurrentPhysicalArmor(0), 7);
	TestEqual(TEXT("Victory does not heal HP"), State.ActiveCharacters[0].Resources.CurrentHealth, 14);
	TestEqual(TEXT("Victory does not restore mana"), State.ActiveCharacters[0].Resources.CurrentMana, 7);
	TestEqual(TEXT("Equipped GUID remains"), State.ActiveEquipment[0].OffHand.RuntimeObjectId, EquippedId);
	TestEqual(TEXT("Bag GUID remains"), State.ActiveCharacters[0].InventorySlots[0].Item.RuntimeObjectId, BagId);
	TestEqual(TEXT("Cursor GUID remains"), State.CursorItem.RuntimeObjectId, CursorId);

	FGridInventoryCharacterSummary Summary;
	TestTrue(TEXT("UI summary reflects restored armor"), Inventory->GetCharacterSummary(0, Summary));
	TestEqual(TEXT("UI and combat agree on current physical armor"), Summary.Resources.CurrentPhysicalArmor, 7);
	TestEqual(TEXT("UI and combat agree on current magical armor"), Summary.Resources.CurrentMagicalArmor, 3);

	TestEqual(TEXT("Re-applying full refill changes nobody"), Inventory->RefillArmorReservesAfterVictory(), 0);
	TestEqual(TEXT("Repeated refill cannot exceed physical reference"), Inventory->GetEffectiveCurrentPhysicalArmor(0), 7);
	TestEqual(TEXT("Repeated refill cannot exceed magical reference"), State.ActiveCharacters[0].Resources.CurrentMagicalArmor, 3);

	// Second encounter: no automatic refill until the next authorized Victory.
	Inventory->ApplyPhysicalArmorDamage(0, 4);
	State.ActiveCharacters[0].Resources.CurrentMagicalArmor = 0;
	TestEqual(TEXT("Second encounter spends physical armor"), Inventory->GetEffectiveCurrentPhysicalArmor(0), 3);
	TestEqual(TEXT("Next Victory returns physical armor"), Inventory->RefillArmorReservesAfterVictory(), 1);
	TestEqual(TEXT("Second Victory restores full physical pool"), Inventory->GetEffectiveCurrentPhysicalArmor(0), 7);
	TestEqual(TEXT("Second Victory restores magic"), State.ActiveCharacters[0].Resources.CurrentMagicalArmor, 3);
	TestNull(TEXT("Shield remains pointerless after both victories"), Inventory->FindItemDefinition(ShieldId));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShield022DownedNoRefillTest,
	"Grimrock.RPG.SHIELD02.2.DownedCharactersAreNotRefilled",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShield022DownedNoRefillTest::RunTest(const FString&)
{
	Shield022::FWorldFixture Fixture;
	FGridResolvedItem Shield;
	const FName ShieldId(TEXT("Weapon_RoundShield_01"));
	if (!TestNotNull(TEXT("Canonical game world"), Fixture.World) ||
		!TestNotNull(TEXT("Real party inventory"), Fixture.Inventory) ||
		!TestTrue(TEXT("Real ARM075 N1 shield resolves"), Fixture.ResolveShield(Shield)))
		return false;
	UGridPartyInventoryComponent* Inventory = Fixture.Inventory;
	URPGClassAsset* Class = Shield022::MakeClass(Inventory, TEXT("Shield022DefeatClass"), 1, 4);
	if (!TestTrue(TEXT("Synthetic class valid"), Class->IsValidDefinition()) ||
		!TestNull(TEXT("No legacy shield registered"), Inventory->FindItemDefinition(ShieldId)))
		return false;

	Inventory->PartyInventoryState.ActiveCharacters.Add(Shield022::MakeCharacter(Class, 0));
	Inventory->PartyInventoryState.ActiveEquipment.SetNum(1);
	auto& State = Inventory->PartyInventoryState;
	State.ActiveEquipment[0].OffHand = Shield022::MakeItem(ShieldId, 5);
	State.ActiveCharacters[0].Resources.CurrentPhysicalArmor = 0;
	State.ActiveCharacters[0].Resources.CurrentMagicalArmor = 0;
	State.bHasCursorItem = true;
	State.CursorItem = Shield022::MakeItem(ShieldId, 4);

	TestEqual(TEXT("No living member means no refill"), Inventory->RefillArmorReservesAfterVictory(), 0);
	TestEqual(TEXT("Downed shield remains depleted"), State.ActiveEquipment[0].OffHand.AbsorbedEquipmentPhysicalArmor, 5);
	TestEqual(TEXT("Downed magic remains depleted"), State.ActiveCharacters[0].Resources.CurrentMagicalArmor, 0);
	TestEqual(TEXT("Cursor not recharged without survivor"), State.CursorItem.AbsorbedEquipmentPhysicalArmor, 4);
	TestEqual(TEXT("Refill does not resurrect"), State.ActiveCharacters[0].Resources.CurrentHealth, 0);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
