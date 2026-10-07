#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "UI/GridCombatActionPanelWidget.h"
#include "UI/GridCombatHudWidget.h"
#include "UI/GridMapWidget.h"
#include "UI/GridSkillsWidget.h"
#include "UI/GridPersistentHudWidget.h"
#include "UI/GrimrockMenuWidget.h"
#include "UObject/UnrealType.h"

namespace
{
	struct FGridUIGlobalHud01World
	{
		UWorld* World = nullptr;

		FGridUIGlobalHud01World()
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
				FName(*FString::Printf(TEXT("UIGlobalHud01World_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true,
				ERHIFeatureLevel::Num, &Values);
			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
				Context.SetCurrentWorld(World);
			}
		}

		~FGridUIGlobalHud01World()
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

	bool IsSelectionVisible(const UImage* Image)
	{
		return IsValid(Image) && Image->GetVisibility() == ESlateVisibility::HitTestInvisible;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUICombatUnify01ContractsTest, "Grimrock.UI.CombatUnify01.Contracts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUICombatUnify01ContractsTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const UGridCombatActionPanelWidget* PanelDefaults = GetDefault<UGridCombatActionPanelWidget>();
	const UGridCombatHudWidget* HudDefaults = GetDefault<UGridCombatHudWidget>();
	if (!TestNotNull(TEXT("Combat action panel defaults exist"), PanelDefaults) || !TestNotNull(TEXT("Combat HUD defaults exist"), HudDefaults))
	{
		return false;
	}

	TestEqual(TEXT("Disabled panel opacity keeps the canonical default"), PanelDefaults->DisabledOpacity, 0.45f);
	TestEqual(TEXT("Party-panel spacing defaults to no artificial gap"), HudDefaults->PartyMemberPanelSpacing, 0.0f);
	TestEqual(TEXT("Initiative keeps eight visible slots by default"), HudDefaults->VisibleInitiativeSlotCount, 8);

	const UClass* PanelClass = UGridCombatActionPanelWidget::StaticClass();
	const UClass* HudClass = UGridCombatHudWidget::StaticClass();
	const FName RequiredPanelBindings[] = { TEXT("Text_StatusEffects"), TEXT("Text_StatusFeedback") };
	for (const FName PropertyName : RequiredPanelBindings)
	{
		TestTrue(FString::Printf(TEXT("Combat action panel exposes %s for authored UMG presentation"), *PropertyName.ToString()),
			PanelClass->FindPropertyByName(PropertyName) != nullptr);
	}

	const FName DefaultsOnlyProperties[] = {
		TEXT("PartyMemberPanelWidgetClass"), TEXT("InitiativeSlotWidgetClass"), TEXT("VisibleInitiativeSlotCount"),
		TEXT("PartyMemberPanelSpacing")
	};
	for (const FName PropertyName : DefaultsOnlyProperties)
	{
		const FProperty* Property = HudClass->FindPropertyByName(PropertyName);
		TestNotNull(FString::Printf(TEXT("Combat HUD exposes configurable %s"), *PropertyName.ToString()), Property);
		if (Property)
		{
			TestTrue(FString::Printf(TEXT("%s is editable on widget defaults"), *PropertyName.ToString()), Property->HasAnyPropertyFlags(CPF_Edit));
			TestTrue(FString::Printf(TEXT("%s is not a per-instance authority"), *PropertyName.ToString()),
				Property->HasAnyPropertyFlags(CPF_DisableEditOnInstance));
		}
	}

	TestTrue(TEXT("Combat HUD exposes no party-member runtime position offset"),
		HudClass->FindPropertyByName(TEXT("PartyMembersPositionOffset")) == nullptr);
	TestTrue(TEXT("Combat HUD exposes no combat-controls runtime position offset"),
		HudClass->FindPropertyByName(TEXT("CombatControlsPositionOffset")) == nullptr);
	TestTrue(TEXT("Combat HUD exposes no C++ persistent-HUD bottom-clearance authority"),
		HudClass->FindPropertyByName(TEXT("PersistentHudBottomClearance")) == nullptr);

	const FProperty* DisabledOpacityProperty = PanelClass->FindPropertyByName(TEXT("DisabledOpacity"));
	TestNotNull(TEXT("Combat action panel exposes DisabledOpacity"), DisabledOpacityProperty);
	if (DisabledOpacityProperty)
	{
		TestTrue(TEXT("DisabledOpacity is editable on widget defaults"), DisabledOpacityProperty->HasAnyPropertyFlags(CPF_Edit));
		TestTrue(TEXT("DisabledOpacity is not a per-instance authority"), DisabledOpacityProperty->HasAnyPropertyFlags(CPF_DisableEditOnInstance));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUIGlobalHud01NavigationSelectionTest, "Grimrock.UI.GlobalHud01.NavigationSelection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIGlobalHud01NavigationSelectionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridUIGlobalHud01World TestWorld;
	if (!TestNotNull(TEXT("Transient world is created"), TestWorld.World))
	{
		return false;
	}

	AGrimrockPartyPawn* Party = TestWorld.World->SpawnActor<AGrimrockPartyPawn>();
	UGridPersistentHudWidget* Hud = NewObject<UGridPersistentHudWidget>(Party);
	if (!TestNotNull(TEXT("Party is created"), Party) || !TestNotNull(TEXT("Persistent HUD presenter is created"), Hud))
	{
		return false;
	}

	Hud->Image_NavEscapeSelectionFrame = NewObject<UImage>(Hud);
	Hud->Image_NavInventorySelectionFrame = NewObject<UImage>(Hud);
	Hud->Image_NavSkillsSelectionFrame = NewObject<UImage>(Hud);
	Hud->Image_NavCraftingSelectionFrame = NewObject<UImage>(Hud);
	Hud->Image_NavMapSelectionFrame = NewObject<UImage>(Hud);
	Hud->Image_NavJournalSelectionFrame = NewObject<UImage>(Hud);
	Hud->Image_NavHelpSelectionFrame = NewObject<UImage>(Hud);
	Hud->InitializePersistentHud(Party);

	Party->bInventoryWorkspaceVisible = true;
	Party->bInventoryWidgetVisible = true;
	Hud->RefreshFromSources();
	TestTrue(TEXT("Inventory navigation frame is selected for the split inventory workspace"), IsSelectionVisible(Hud->Image_NavInventorySelectionFrame));
	TestFalse(TEXT("ESC does not fabricate a selected state"), IsSelectionVisible(Hud->Image_NavEscapeSelectionFrame));

	Party->bInventoryWorkspaceVisible = false;
	Party->SkillsWidgetInstance = NewObject<UGridSkillsWidget>(Party);
	Party->SkillsWidgetInstance->SetVisibility(ESlateVisibility::Visible);
	Party->bInventoryWidgetVisible = true;
	Hud->RefreshFromSources();
	TestTrue(TEXT("Skills navigation frame follows the standalone Skills window"), IsSelectionVisible(Hud->Image_NavSkillsSelectionFrame));
	TestFalse(TEXT("Inventory frame clears when the inventory workspace closes"), IsSelectionVisible(Hud->Image_NavInventorySelectionFrame));

	Party->SkillsWidgetInstance->SetVisibility(ESlateVisibility::Collapsed);
	Party->MapWidgetInstance = NewObject<UGridMapWidget>(Party);
	Party->MapWidgetInstance->SetVisibility(ESlateVisibility::Visible);
	Party->bInventoryWidgetVisible = true;
	Hud->RefreshFromSources();
	TestTrue(TEXT("Map navigation frame follows the standalone Map window"), IsSelectionVisible(Hud->Image_NavMapSelectionFrame));
	TestFalse(TEXT("Previous navigation selection is cleared"), IsSelectionVisible(Hud->Image_NavSkillsSelectionFrame));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUIGlobalHud01CombatChromeSplitTest, "Grimrock.UI.GlobalHud01.CombatChromeSplit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIGlobalHud01CombatChromeSplitTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridUIGlobalHud01World TestWorld;
	if (!TestNotNull(TEXT("Transient world is created"), TestWorld.World))
	{
		return false;
	}

	AGrimrockPartyPawn* Party = TestWorld.World->SpawnActor<AGrimrockPartyPawn>();
	UGridPersistentHudWidget* PersistentHud = NewObject<UGridPersistentHudWidget>(Party);
	UGridCombatHudWidget* CombatHud = NewObject<UGridCombatHudWidget>(Party);
	if (!Party || !PersistentHud || !CombatHud)
	{
		return false;
	}

	Party->PersistentHudWidgetInstance = PersistentHud;
	Party->CombatHudWidgetInstance = CombatHud;

	const UClass* CombatHudClass = UGridCombatHudWidget::StaticClass();
	const UClass* PersistentHudClass = UGridPersistentHudWidget::StaticClass();
	TestTrue(TEXT("Combat HUD exposes no legacy global navigation panel"),
		CombatHudClass->FindPropertyByName(TEXT("Panel_GlobalNavigation")) == nullptr);
	TestTrue(TEXT("Combat HUD exposes no legacy action-bar panel"),
		CombatHudClass->FindPropertyByName(TEXT("Panel_Actions")) == nullptr);
	TestTrue(TEXT("Combat HUD exposes no action-widget class authority"),
		CombatHudClass->FindPropertyByName(TEXT("ActionWidgetClass")) == nullptr);
	TestTrue(TEXT("Combat HUD exposes no obsolete targeting presentation panel"),
		CombatHudClass->FindPropertyByName(TEXT("Panel_Targeting")) == nullptr);
	TestTrue(TEXT("Combat HUD exposes no obsolete targeting instructions text"),
		CombatHudClass->FindPropertyByName(TEXT("Text_TargetingInstructions")) == nullptr);
	TestTrue(TEXT("Combat HUD exposes no obsolete targeting cell text"),
		CombatHudClass->FindPropertyByName(TEXT("Text_TargetingCell")) == nullptr);
	TestTrue(TEXT("Persistent HUD owns global navigation"),
		PersistentHudClass->FindPropertyByName(TEXT("Panel_GlobalNavigation")) != nullptr);
	TestTrue(TEXT("Persistent HUD owns the action bar"),
		PersistentHudClass->FindPropertyByName(TEXT("Panel_ActionBar")) != nullptr);
	TestTrue(TEXT("Persistent HUD owns the action-widget class"),
		PersistentHudClass->FindPropertyByName(TEXT("ActionWidgetClass")) != nullptr);

	const FName NavigationButtonProperties[] = {
		TEXT("Button_NavEscape"), TEXT("Button_NavInventory"), TEXT("Button_NavSkills"), TEXT("Button_NavCrafting"),
		TEXT("Button_NavMap"), TEXT("Button_NavJournal"), TEXT("Button_NavHelp")
	};
	for (const FName PropertyName : NavigationButtonProperties)
	{
		TestTrue(FString::Printf(TEXT("Combat HUD exposes no legacy %s"), *PropertyName.ToString()),
			CombatHudClass->FindPropertyByName(PropertyName) == nullptr);
		TestTrue(FString::Printf(TEXT("Persistent HUD owns %s"), *PropertyName.ToString()),
			PersistentHudClass->FindPropertyByName(PropertyName) != nullptr);
	}

	const FName CombatProperties[] = {
		TEXT("Panel_CombatHud"), TEXT("Panel_PartyMembers"), TEXT("Panel_Initiative"), TEXT("Panel_CombatBottomRight"),
		TEXT("Text_MobilityActionPoints"), TEXT("Button_EndTurn"), TEXT("Text_EndTurnDisabledReason")
	};
	for (const FName PropertyName : CombatProperties)
	{
		TestTrue(FString::Printf(TEXT("Combat HUD retains %s"), *PropertyName.ToString()),
			CombatHudClass->FindPropertyByName(PropertyName) != nullptr);
	}
	CombatHud->Panel_CombatBottomRight = NewObject<UVerticalBox>(CombatHud);
	CombatHud->Text_MobilityActionPoints = NewObject<UTextBlock>(CombatHud);
	CombatHud->Button_EndTurn = NewObject<UButton>(CombatHud);
	CombatHud->Text_EndTurnDisabledReason = NewObject<UTextBlock>(CombatHud);
	CombatHud->InitializeCombatHud(Party, nullptr);

	TestTrue(TEXT("Combat HUD layout is authored in UMG, not through a C++ bottom-clearance property"),
		CombatHudClass->FindPropertyByName(TEXT("PersistentHudBottomClearance")) == nullptr);
	TestTrue(TEXT("Combat HUD layout is authored in UMG, not through a party-member position offset"),
		CombatHudClass->FindPropertyByName(TEXT("PartyMembersPositionOffset")) == nullptr);
	TestTrue(TEXT("Combat HUD layout is authored in UMG, not through a combat-controls position offset"),
		CombatHudClass->FindPropertyByName(TEXT("CombatControlsPositionOffset")) == nullptr);

	TestEqual(TEXT("The persistent action bar keeps twelve minimum keyboard-addressable slots"), FGridCombatHotbarBinding::MinimumSlotCount, 12);
	TestEqual(TEXT("Twelve action slots have keyboard shortcuts"), FGridCombatHotbarBinding::KeyboardShortcutSlotCount, 12);
	TestEqual(TEXT("A 1600-wide viewport fits twenty-seven 50px slots after 210px navigation"),
		UGridPersistentHudWidget::CalculateVisibleActionSlotCount(1600.0f, 210.0f, 50.0f), 27);
	TestEqual(TEXT("A narrow viewport never drops below the twelve keyboard slots"),
		UGridPersistentHudWidget::CalculateVisibleActionSlotCount(640.0f, 210.0f, 50.0f), 12);

	Party->PartyInventoryComponent->InitializeDefaultPartyIfNeeded();
	const int32 CharacterIndex = Party->PartyInventoryComponent->GetSelectedCharacterIndex();
	TestTrue(TEXT("Dynamic storage can grow to the fitted slot count"),
		Party->PartyInventoryComponent->EnsureCharacterCombatHotbarCapacity(CharacterIndex, 27));
	TestEqual(TEXT("Dynamic storage grows without inventing a fixed upper presentation count"),
		Party->PartyInventoryComponent->GetCharacterCombatHotbarSlotCount(CharacterIndex), 27);
	TestTrue(TEXT("A smaller later requirement is accepted without truncating storage"),
		Party->PartyInventoryComponent->EnsureCharacterCombatHotbarCapacity(CharacterIndex, 18));
	TestEqual(TEXT("Dynamic storage never shrinks implicitly"),
		Party->PartyInventoryComponent->GetCharacterCombatHotbarSlotCount(CharacterIndex), 27);

	return true;
}

#endif
