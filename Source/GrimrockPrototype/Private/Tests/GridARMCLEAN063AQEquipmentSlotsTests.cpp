#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Blueprint/UserWidget.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Runtime/GridItemContextActionLibrary.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "UI/GridInventoryWidget.h"
#include "UI/GridInventorySlotWidget.h"
#include "UI/GridEquipmentTooltipGlobalProjection.h"
#include "UI/UILocalizationText.h"
#include "../Runtime/GridCanonicalEquipmentSlotsReadModel.h"

namespace ARMCLEAN063AQ
{
	struct FWorld
	{
		UWorld* World = nullptr;
		UGameInstance* GameInstance = nullptr;
		AGrimrockPartyPawn* Pawn = nullptr;
		UGridPartyInventoryComponent* Inventory = nullptr;
		const UGridItemSubsystem* Registry = nullptr;

		FWorld()
		{
			const UWorld::InitializationValues Values = UWorld::InitializationValues()
				.AllowAudioPlayback(false).RequiresHitProxies(false)
				.CreatePhysicsScene(false).CreateNavigation(false)
				.CreateAISystem(false).ShouldSimulatePhysics(false)
				.SetTransactional(false);
			World = UWorld::CreateWorld(EWorldType::Game, false,
				FName(*FString::Printf(TEXT("ARM063AQ_%s"),
					*FGuid::NewGuid().ToString(EGuidFormats::Digits))),
				nullptr, true, ERHIFeatureLevel::Num, &Values);
			if (!World || !GEngine) return;
			FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
			Context.SetCurrentWorld(World);
			GameInstance = NewObject<UGameInstance>(GetTransientPackage());
			if (!GameInstance) return;
			GameInstance->Init();
			World->SetGameInstance(GameInstance);
			Registry = GameInstance->GetSubsystem<UGridItemSubsystem>();
			Pawn = World->SpawnActor<AGrimrockPartyPawn>();
			if (Pawn)
			{
				Inventory = Pawn->PartyInventoryComponent.Get();
				if (Inventory) Inventory->InitializeDefaultPartyIfNeeded();
			}
		}

		~FWorld()
		{
			if (World) World->SetGameInstance(nullptr);
			if (GameInstance) GameInstance->Shutdown();
			if (World)
			{
				World->DestroyWorld(false);
				if (GEngine) GEngine->DestroyWorldContext(World);
			}
		}

		bool IsReady() const
		{
			return World && Pawn && Inventory && Registry &&
				Registry->HasCanonicalItem(TEXT("Weapon_GreatAxe_01")) &&
				Inventory->IsValidCharacterIndex(0);
		}
	};

	FGridItemInstance MakeItem(FName Id, int32 Level, FGuid CharacterId)
	{
		FGridItemInstance Item;
		Item.RuntimeObjectId = FGuid::NewGuid();
		Item.ItemDefinitionId = Id;
		Item.ItemLevel = Level;
		Item.Quantity = 1;
		Item.OwnerType = EGridItemOwnerType::CharacterInventory;
		Item.OwnerGuid = CharacterId;
		Item.OwnerCharacterIndex = 0;
		Item.EquipmentSlot = EGridEquipmentSlot::None;
		return Item;
	}

	UGridItemDefinitionAsset* MakeContradictoryDefinition(
		UGridPartyInventoryComponent* Inventory, FName Id, EGridItemHandUsage Usage,
		EGridEquipmentSlot WrongSlot)
	{
		UGridItemDefinitionAsset* Definition = NewObject<UGridItemDefinitionAsset>(Inventory);
		Definition->ItemDefinitionId = Id;
		Definition->DisplayName = FText::FromName(Id);
		Definition->ItemType = EGridItemType::Weapon;
		Definition->HandUsage = Usage;
		Definition->CompatibleEquipmentSlots.Add(WrongSlot);
		return Definition;
	}

	TArray<EGridEquipmentSlot> MenuSlots(AGrimrockPartyPawn* Pawn,
		const FGridItemInstance& Item, UGridItemDefinitionAsset* Definition,
		int32& OutTwoHandLabels)
	{
		FGridItemActionContext Context;
		Context.PartyPawn = Pawn;
		Context.CharacterIndex = 0;
		Context.InventorySlotIndex = 0;
		Context.Item = Item;
		Context.ItemDefinition = Definition;
		FGridFacingTargetContext Facing;
		TArray<FGridItemContextAction> Actions;
		UGridItemContextActionLibrary::BuildItemContextActions(Context, Facing, Actions);
		OutTwoHandLabels = 0;
		TArray<EGridEquipmentSlot> Slots;
		for (const FGridItemContextAction& Action : Actions)
		{
			if (Action.ActionType != EGridItemActionType::Equip) continue;
			Slots.Add(Action.EquipmentSlot);
			if (Action.Label.EqualTo(NSLOCTEXT("GridItemActions", "EquipTwoHands", "S'équiper : deux mains")))
				++OutTwoHandLabels;
		}
		return Slots;
	}
}

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FGridClean063AQEquipmentSlots,
	"Grimrock.RPG.ARM.CLEAN06_3.CanonicalContextAndTooltipEquipmentSlots",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

void FGridClean063AQEquipmentSlots::GetTests(
	TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
{
	for (const TCHAR* Stage : { TEXT("Policy"), TEXT("ContextMenu"), TEXT("TooltipModel"), TEXT("TooltipWidget") })
	{
		OutBeautifiedNames.Add(Stage);
		OutTestCommands.Add(Stage);
	}
}

bool FGridClean063AQEquipmentSlots::RunTest(const FString& Stage)
{
	ARMCLEAN063AQ::FWorld Fixture;
	if (!TestTrue(TEXT("Real world, GI registry and inventory are initialized"), Fixture.IsReady()))
		return false;
	const FGuid CharacterId =
		Fixture.Inventory->PartyInventoryState.ActiveCharacters[0].CharacterId;
	if (!TestTrue(TEXT("Character GUID is valid"), CharacterId.IsValid())) return false;

	// Genuine canonical weapons with intentionally FALSE obsolete hand flags.
	UGridItemDefinitionAsset* Axe = ARMCLEAN063AQ::MakeContradictoryDefinition(
		Fixture.Inventory, TEXT("Weapon_GreatAxe_01"),
		EGridItemHandUsage::OneHanded, EGridEquipmentSlot::OffHand);
	UGridItemDefinitionAsset* Shield = ARMCLEAN063AQ::MakeContradictoryDefinition(
		Fixture.Inventory, TEXT("Weapon_RoundShield_01"),
		EGridItemHandUsage::TwoHanded, EGridEquipmentSlot::MainHand);
	UGridItemDefinitionAsset* Sword = ARMCLEAN063AQ::MakeContradictoryDefinition(
		Fixture.Inventory, TEXT("Weapon_StraightSword_01"),
		EGridItemHandUsage::TwoHanded, EGridEquipmentSlot::Head);
	UGridItemDefinitionAsset* Ammo = ARMCLEAN063AQ::MakeContradictoryDefinition(
		Fixture.Inventory, TEXT("Item_Arrows_01"),
		EGridItemHandUsage::OneHanded, EGridEquipmentSlot::MainHand);
	Ammo->bUseWeaponCatalogue = true;
	const FGridItemInstance AxeN12 =
		ARMCLEAN063AQ::MakeItem(Axe->ItemDefinitionId, 12, CharacterId);
	const FGridItemInstance ShieldN20 =
		ARMCLEAN063AQ::MakeItem(Shield->ItemDefinitionId, 20, CharacterId);
	const FGridItemInstance SwordN4 =
		ARMCLEAN063AQ::MakeItem(Sword->ItemDefinitionId, 4, CharacterId);
	const FGridItemInstance AmmoN0 =
		ARMCLEAN063AQ::MakeItem(Ammo->ItemDefinitionId, 0, CharacterId);

	const FGridCanonicalEquipmentSlotsReadView AxeSlots =
		FGridCanonicalEquipmentSlotsReadModel::Resolve(Fixture.Registry, Axe, AxeN12);
	TestTrue(TEXT("Canonical great axe overrides obsolete one-handed flag"),
		AxeSlots.bCanonicalIdentity && AxeSlots.bResolved && AxeSlots.bTwoHanded);
	TestEqual(TEXT("Great axe has one physical storage action"), AxeSlots.AllowedSlots.Num(), 1);
	if (!AxeSlots.AllowedSlots.IsEmpty())
		TestEqual(TEXT("Two-handed action targets MainHand"), AxeSlots.AllowedSlots[0], EGridEquipmentSlot::MainHand);

	const auto ShieldSlots = FGridCanonicalEquipmentSlotsReadModel::Resolve(Fixture.Registry, Shield, ShieldN20);
	TestTrue(TEXT("Canonical shield is not two-handed despite its old asset"),
		ShieldSlots.bCanonicalIdentity && ShieldSlots.bResolved && !ShieldSlots.bTwoHanded);
	TestEqual(TEXT("Canonical shield has just one allowed hand"), ShieldSlots.AllowedSlots.Num(), 1);
	if (!ShieldSlots.AllowedSlots.IsEmpty())
		TestEqual(TEXT("Shield is OffHand-only"), ShieldSlots.AllowedSlots[0], EGridEquipmentSlot::OffHand);

	FGridResolvedItem RealSword;
	if (!TestTrue(TEXT("Real sword N4 resolves"), Fixture.Registry->ResolveItem(
		SwordN4.ItemDefinitionId, SwordN4.ItemLevel, RealSword))) return false;
	const auto SwordSlots = FGridCanonicalEquipmentSlotsReadModel::Resolve(Fixture.Registry, Sword, SwordN4);
	TestTrue(TEXT("Sword hand policy comes from canonical root"), SwordSlots.bCanonicalIdentity && SwordSlots.bResolved);
	TestFalse(TEXT("One-hand sword is not mislabeled two hands"), SwordSlots.bTwoHanded);
	TestEqual(TEXT("Sword MainHand follows canonical policy"),
		SwordSlots.AllowedSlots.Contains(EGridEquipmentSlot::MainHand),
		RealSword.Weapon.CanEquipInHand(EGridEquipmentSlot::MainHand));
	TestEqual(TEXT("Sword OffHand follows canonical policy"),
		SwordSlots.AllowedSlots.Contains(EGridEquipmentSlot::OffHand),
		RealSword.Weapon.CanEquipInHand(EGridEquipmentSlot::OffHand));
	TestFalse(TEXT("Old Head slot never leaks into registered ARM"), SwordSlots.AllowedSlots.Contains(EGridEquipmentSlot::Head));

	const auto AmmoSlots = FGridCanonicalEquipmentSlotsReadModel::Resolve(Fixture.Registry, Ammo, AmmoN0);
	TestTrue(TEXT("Registered ammo N0 is resolved as non-armament"),
		AmmoSlots.bCanonicalIdentity && AmmoSlots.bResolved);
	TestTrue(TEXT("Ammo cannot become wearable via stale old flags"), AmmoSlots.AllowedSlots.IsEmpty());
	FGridItemInstance InvalidAxe = AxeN12;
	InvalidAxe.ItemLevel = 0;
	const auto InvalidSlots = FGridCanonicalEquipmentSlotsReadModel::Resolve(Fixture.Registry, Axe, InvalidAxe);
	TestTrue(TEXT("Invalid ARM N0 is still a known identity"), InvalidSlots.bCanonicalIdentity);
	TestFalse(TEXT("Registered ARM N0 cannot fall back to old slots"), InvalidSlots.bResolved);
	TestTrue(TEXT("Registered ARM N0 has no actions"), InvalidSlots.AllowedSlots.IsEmpty());

	// Transitional unknown IDs keep the existing one/two-hand semantics.
	UGridItemDefinitionAsset* Legacy = ARMCLEAN063AQ::MakeContradictoryDefinition(
		Fixture.Inventory, TEXT("ARM063AQ_Legacy"),
		EGridItemHandUsage::OneHanded, EGridEquipmentSlot::Head);
	FGridItemInstance LegacyItem =
		ARMCLEAN063AQ::MakeItem(Legacy->ItemDefinitionId, 0, CharacterId);
	const auto OldSlots = FGridCanonicalEquipmentSlotsReadModel::Resolve(Fixture.Registry, Legacy, LegacyItem);
	TestFalse(TEXT("Unmigrated ID is not canonical"), OldSlots.bCanonicalIdentity);
	TestTrue(TEXT("Unmigrated ID uses old allowed hands"), OldSlots.AllowedSlots.Contains(EGridEquipmentSlot::MainHand) &&
		OldSlots.AllowedSlots.Contains(EGridEquipmentSlot::OffHand));
	TestTrue(TEXT("Unmigrated explicit Head remains available"), OldSlots.AllowedSlots.Contains(EGridEquipmentSlot::Head));
	TestEqual(TEXT("No modification of canonical item state"), AxeN12.ItemLevel, 12);
	if (Stage == TEXT("Policy")) return !HasAnyErrors();

	// Actual context menu uses the same canonical read model.
	int32 TwoHandLabels = 0;
	const auto AxeMenu = ARMCLEAN063AQ::MenuSlots(Fixture.Pawn, AxeN12, Axe, TwoHandLabels);
	TestEqual(TEXT("Axe contextual action count"), AxeMenu.Num(), 1);
	TestEqual(TEXT("Axe contextual two-handed label count"), TwoHandLabels, 1);
	if (!AxeMenu.IsEmpty()) TestEqual(TEXT("Axe menu targets MainHand"), AxeMenu[0], EGridEquipmentSlot::MainHand);
	const auto ShieldMenu = ARMCLEAN063AQ::MenuSlots(Fixture.Pawn, ShieldN20, Shield, TwoHandLabels);
	TestEqual(TEXT("Shield contextual action count"), ShieldMenu.Num(), 1);
	TestEqual(TEXT("Shield contextual two-handed label count"), TwoHandLabels, 0);
	if (!ShieldMenu.IsEmpty()) TestEqual(TEXT("Shield menu targets OffHand"), ShieldMenu[0], EGridEquipmentSlot::OffHand);
	const auto SwordMenu = ARMCLEAN063AQ::MenuSlots(Fixture.Pawn, SwordN4, Sword, TwoHandLabels);
	TestEqual(TEXT("Sword menu matches canonical slot choices"), SwordMenu.Num(), SwordSlots.AllowedSlots.Num());
	for (const EGridEquipmentSlot Slot : SwordMenu)
		TestTrue(TEXT("Sword menu has no invented slot"), SwordSlots.AllowedSlots.Contains(Slot));
	TestTrue(TEXT("Registered ammo has no equip menu action"),
		ARMCLEAN063AQ::MenuSlots(Fixture.Pawn, AmmoN0, Ammo, TwoHandLabels).IsEmpty());
	TestTrue(TEXT("Registered invalid ARM has no equip menu action"),
		ARMCLEAN063AQ::MenuSlots(Fixture.Pawn, InvalidAxe, Axe, TwoHandLabels).IsEmpty());
	TestEqual(TEXT("Canonical sword works with NO legacy definition"),
		ARMCLEAN063AQ::MenuSlots(Fixture.Pawn, SwordN4, nullptr, TwoHandLabels).Num(),
		SwordSlots.AllowedSlots.Num());

	if (Stage == TEXT("ContextMenu")) return !HasAnyErrors();

	// The shared tooltip scenario list must match the menu and runtime.
	TArray<EGridEquipmentSlot> TooltipSlots;
	FGridEquipmentTooltipGlobalProjection::GatherAllowedSlots(
		Fixture.Registry, Shield, ShieldN20, TooltipSlots);
	TestEqual(TEXT("Tooltip shield slot parity"), TooltipSlots, ShieldSlots.AllowedSlots);
	FGridEquipmentTooltipGlobalProjection::GatherAllowedSlots(
		Fixture.Registry, Axe, AxeN12, TooltipSlots);
	TestEqual(TEXT("Tooltip axe slot parity"), TooltipSlots, AxeSlots.AllowedSlots);
	for (const EGridEquipmentSlot Slot : SwordSlots.AllowedSlots)
		TestTrue(TEXT("Every context slot is equippable by the actual inventory"),
			Fixture.Inventory->CanEquipItemToSlot(0, SwordN4, Slot));

	// Presentation uses the pre-existing ST_UI key; no authoring or WBP change.
	TestTrue(TEXT("The existing two-hands translation is materialized"),
		!FUILocalizationText::Get(TEXT("Inventory.Tooltip.Equipment.TwoHands")).IsEmpty());
	if (Stage == TEXT("TooltipModel")) return !HasAnyErrors();
	if (!TestTrue(TEXT("Register contradictory axe as a legacy-compatible UI lookup"),
			Fixture.Inventory->RegisterItemDefinition(Axe))) return false;
	UGridInventoryWidget* ParentWidget =
		CreateWidget<UGridInventoryWidget>(Fixture.World, UGridInventoryWidget::StaticClass());
	UGridInventorySlotWidget* SlotWidget =
		CreateWidget<UGridInventorySlotWidget>(Fixture.World, UGridInventorySlotWidget::StaticClass());
	if (!TestNotNull(TEXT("Real parent widget"), ParentWidget) ||
		!TestNotNull(TEXT("Real slot widget"), SlotWidget)) return false;
	ParentWidget->InventoryComponent = Fixture.Inventory;
	SlotWidget->SetOwnerInventoryWidget(ParentWidget);
	SlotWidget->InitializeInventorySlot(EGridInventoryUiSlotType::Inventory, INDEX_NONE);
	SlotWidget->SetItem(AxeN12);
	const FGridItemTooltipView AxeTooltip = SlotWidget->GetTooltipView();
	TestTrue(TEXT("Canonical tooltip view is valid"), AxeTooltip.bValid);
	TestTrue(TEXT("Canonical axe remains equippable"), AxeTooltip.bEquippable);
	TestTrue(TEXT("Axe tooltip says two hands"),
		AxeTooltip.CompatibleSlotsText.EqualTo(
			FUILocalizationText::Get(TEXT("Inventory.Tooltip.Equipment.TwoHands"))));


	return true;
}
#endif
