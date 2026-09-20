#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/TextBlock.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "UI/GridCharacterSheetWidget.h"
#include "UI/GridInventoryBagWidget.h"
#include "UI/GridInventoryWidget.h"

namespace
{
	struct FGridUISplit01TestWorld
	{
		UWorld* World = nullptr;

		FGridUISplit01TestWorld()
		{
			const UWorld::InitializationValues InitializationValues = UWorld::InitializationValues()
				.AllowAudioPlayback(false)
				.RequiresHitProxies(false)
				.CreatePhysicsScene(false)
				.CreateNavigation(false)
				.CreateAISystem(false)
				.ShouldSimulatePhysics(false)
				.SetTransactional(false);

			World = UWorld::CreateWorld(EWorldType::Game, false,
				FName(*FString::Printf(TEXT("UISplit01World_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true,
				ERHIFeatureLevel::Num, &InitializationValues);
			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
				Context.SetCurrentWorld(World);
			}
		}

		~FGridUISplit01TestWorld()
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUISplit01SemanticWindowsTest, "Grimrock.UI.Split01.SemanticWindows",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUISplit01SemanticWindowsTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	TestTrue(TEXT("Character sheet reuses canonical inventory UI routing"),
		UGridCharacterSheetWidget::StaticClass()->IsChildOf(UGridInventoryWidget::StaticClass()));
	TestTrue(TEXT("Inventory bag reuses canonical inventory UI routing"),
		UGridInventoryBagWidget::StaticClass()->IsChildOf(UGridInventoryWidget::StaticClass()));

	FGridUISplit01TestWorld TestWorld;
	AGrimrockPartyPawn* Party = TestWorld.World ? TestWorld.World->SpawnActor<AGrimrockPartyPawn>() : nullptr;
	if (!TestNotNull(TEXT("Party pawn exists"), Party))
	{
		return false;
	}

	Party->CharacterSheetWidgetClass = UGridCharacterSheetWidget::StaticClass();
	Party->InventoryBagWidgetClass = UGridInventoryBagWidget::StaticClass();
	TestTrue(TEXT("Two configured split classes enable the split workspace"), Party->IsSplitInventoryWorkspaceConfigured());

	Party->InventoryBagWidgetClass = nullptr;
	TestFalse(TEXT("A missing split class keeps legacy fallback available"), Party->IsSplitInventoryWorkspaceConfigured());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUISplit01SharedAuthorityTest, "Grimrock.UI.Split01.SharedAuthority",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUISplit01SharedAuthorityTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridUISplit01TestWorld TestWorld;
	AGrimrockPartyPawn* Party = TestWorld.World ? TestWorld.World->SpawnActor<AGrimrockPartyPawn>() : nullptr;
	if (!TestNotNull(TEXT("Party pawn exists"), Party) ||
		!TestNotNull(TEXT("Canonical party inventory exists"), Party ? Party->PartyInventoryComponent.Get() : nullptr))
	{
		return false;
	}

	UGridPartyInventoryComponent* Inventory = Party->PartyInventoryComponent;
	Inventory->PartyInventoryState.ActiveCharacters.SetNum(2);
	Inventory->PartyInventoryState.ActiveEquipment.SetNum(2);
	Inventory->PartyInventoryState.SelectedCharacterIndex = 0;

	FGridCharacterInventoryState& First = Inventory->PartyInventoryState.ActiveCharacters[0];
	First.CharacterId = FGuid::NewGuid();
	First.DisplayName = FText::FromString(TEXT("Ariadne"));
	First.InventorySlots.SetNum(4);

	FGridCharacterInventoryState& Second = Inventory->PartyInventoryState.ActiveCharacters[1];
	Second.CharacterId = FGuid::NewGuid();
	Second.DisplayName = FText::FromString(TEXT("Borin"));
	Second.InventorySlots.SetNum(7);

	UGridCharacterSheetWidget* Sheet = NewObject<UGridCharacterSheetWidget>();
	UGridInventoryBagWidget* Bag = NewObject<UGridInventoryBagWidget>();
	if (!TestNotNull(TEXT("Character sheet exists"), Sheet) || !TestNotNull(TEXT("Inventory bag exists"), Bag))
	{
		return false;
	}

	Sheet->Text_CharacterName = NewObject<UTextBlock>(Sheet);
	Bag->Text_InventoryBagTitle = NewObject<UTextBlock>(Bag);
	Sheet->InitializeInventoryWidget(Party);
	Bag->InitializeInventoryWidget(Party);

	TestTrue(TEXT("Both windows share the same inventory component"), Sheet->InventoryComponent == Bag->InventoryComponent);
	TestTrue(TEXT("Both windows point at the canonical party inventory"), Sheet->InventoryComponent == Inventory);
	TestEqual(TEXT("Sheet initially projects selected character 0"), Sheet->Text_CharacterName->GetText().ToString(), FString(TEXT("Ariadne")));
	TestEqual(TEXT("Bag initially projects selected character 0"), Bag->Text_InventoryBagTitle->GetText().ToString(), FString(TEXT("Ariadne")));

	TestTrue(TEXT("Selecting character 1 through the sheet succeeds"), Sheet->SelectCharacter(1));
	TestEqual(TEXT("Canonical selection changes once"), Inventory->GetSelectedCharacterIndex(), 1);
	TestEqual(TEXT("Sheet refreshes from the shared selection"), Sheet->Text_CharacterName->GetText().ToString(), FString(TEXT("Borin")));
	TestEqual(TEXT("Bag refreshes from the same shared selection event"), Bag->Text_InventoryBagTitle->GetText().ToString(), FString(TEXT("Borin")));

	Party->InventoryBagWidgetInstance = Bag;
	TestTrue(TEXT("Pawn canonical inventory UI accessor prefers the split bag"), Party->GetInventoryWidget() == Bag);

	return true;
}

#endif
