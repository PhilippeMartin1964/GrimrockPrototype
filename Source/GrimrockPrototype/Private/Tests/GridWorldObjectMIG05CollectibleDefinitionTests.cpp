#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/StaticMeshComponent.h"
#include "Core/GridLevelAsset.h"
#include "Core/GridObjectPaletteAsset.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Runtime/GridItemActor.h"
#include "Runtime/GridItemDefinitionAsset.h"
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
	FGridWorldObjectMIG05DirectCollectibleDefinitionTest,
	"Grimrock.WorldObjects.MIG05.DirectCollectibleDefinition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridWorldObjectMIG05DirectCollectibleDefinitionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGridItemDefinitionAsset* Definition = NewObject<UGridItemDefinitionAsset>();
	Definition->ItemDefinitionId = TEXT("BlueGem");
	Definition->DisplayName = FText::FromString(TEXT("Blue Gem"));
	Definition->ItemType = EGridItemType::Gem;
	UStaticMesh* WorldMesh = NewObject<UStaticMesh>(Definition);
	Definition->WorldMesh = WorldMesh;
	UTexture2D* DefinitionIcon = NewObject<UTexture2D>(Definition);
	Definition->Icon = DefinitionIcon;

	FGridObjectPaletteEntry Entry;
	Entry.EntryId = TEXT("BlueGem");
	Entry.DefaultItemDefinition = Definition;

	TestTrue(TEXT("Direct collectible palette entry is valid without an object archetype"), Entry.IsValidEntry());
	TestTrue(TEXT("Direct collectible entry owns no companion archetype"), Entry.DefaultArchetype == nullptr);
	TestTrue(TEXT("Direct collectible entry does not duplicate the definition icon"), Entry.Icon == nullptr);
	TestEqual(TEXT("Direct collectible entry resolves to Item"), Entry.GetEffectiveObjectType(), EGridLevelObjectType::Item);
	TestTrue(TEXT("Direct collectible entry has no effective ArchetypeId"), Entry.GetEffectiveArchetypeId().IsNone());
	TestEqual(TEXT("Direct collectible entry uses Items category"), Entry.GetEffectiveCategory(), FName(TEXT("Items")));
	TestEqual(TEXT("Direct collectible entry uses ItemDefinition display name"), Entry.GetEffectiveDisplayName().ToString(), FString(TEXT("Blue Gem")));
	TestTrue(TEXT("Collectible icon lives on the ItemDefinition"), Definition->Icon.Get() == DefinitionIcon);

	UGridObjectPaletteAsset* Palette = NewObject<UGridObjectPaletteAsset>();
	Palette->Entries.Add(Entry);
	TArray<FGridArchetypeValidationMessage> ValidationMessages;
	TestTrue(TEXT("Palette accepts a direct ItemDefinition collectible"), Palette->ValidatePalette(ValidationMessages));
	TestEqual(TEXT("Direct collectible palette validation has no messages"), ValidationMessages.Num(), 0);

	FProperty* ItemArchetypeBridge = AGridItemActor::StaticClass()->FindPropertyByName(TEXT("ArchetypeId"));
	TestNotNull(TEXT("Legacy item ArchetypeId bridge remains until MIG09"), ItemArchetypeBridge);
	if (ItemArchetypeBridge)
	{
		TestTrue(TEXT("Legacy item ArchetypeId bridge is transient"), ItemArchetypeBridge->HasAnyPropertyFlags(CPF_Transient));
		TestTrue(TEXT("Legacy item ArchetypeId bridge is read-only in Details"), ItemArchetypeBridge->HasAnyPropertyFlags(CPF_EditConst));
	}

	FMIG05ItemTestWorld TestWorld;
	TestNotNull(TEXT("MIG05 runtime world exists"), TestWorld.World);
	if (!TestWorld.World)
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

	FGridLevelObjectData ItemData;
	ItemData.ObjectId = FGuid::NewGuid();
	ItemData.Type = EGridLevelObjectType::Item;
	ItemData.CellX = 0;
	ItemData.CellY = 0;
	ItemData.ArchetypeId = NAME_None;
	ItemData.ItemDefinitionAsset = Definition;

	FTransform PlacementTransform;
	TestTrue(TEXT("Loose item placement transform resolves without any ObjectArchetype"), Runtime->GetObjectPlacementTransform(ItemData, PlacementTransform));
	TestTrue(TEXT("Direct loose item remains centered on its cell"),
		PlacementTransform.GetLocation().Equals(Runtime->GetCellCenterWorld(0, 0, 12.0f), KINDA_SMALL_NUMBER));

	AGridItemActor* ItemActor = TestWorld.World->SpawnActor<AGridItemActor>();
	TestNotNull(TEXT("Generic item actor exists"), ItemActor);
	if (ItemActor)
	{
		ItemActor->InitializeFromItemDefinition(Definition, ItemData.ObjectId);
		TestTrue(TEXT("Generic item actor keeps the canonical definition asset"), ItemActor->GetItemDefinitionAsset() == Definition);
		TestEqual(TEXT("Generic item actor identity is ItemDefinitionId"), ItemActor->GetItemDefinitionId(), FName(TEXT("BlueGem")));
		TestTrue(TEXT("Generic item actor presentation uses ItemDefinition WorldMesh"), ItemActor->MeshComponent->GetStaticMesh() == WorldMesh);
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

	UGridItemDefinitionAsset* Definition = NewObject<UGridItemDefinitionAsset>();
	Definition->ItemDefinitionId = TEXT("CopperKey");
	Definition->DisplayName = FText::FromString(TEXT("Copper Key"));

	UGridObjectArchetypeAsset* LegacyArchetype = NewObject<UGridObjectArchetypeAsset>();
	LegacyArchetype->ArchetypeId = TEXT("Item_CopperKey_Legacy");
	LegacyArchetype->SupportedType = EGridLevelObjectType::Item;

	FGridObjectPaletteEntry Entry;
	Entry.EntryId = TEXT("CopperKey");
	Entry.DefaultItemDefinition = Definition;
	Entry.DefaultArchetype = LegacyArchetype;

	UGridObjectPaletteAsset* Palette = NewObject<UGridObjectPaletteAsset>();
	Palette->Entries.Add(Entry);
	TArray<FGridArchetypeValidationMessage> Messages;
	TestFalse(TEXT("Palette rejects two authoring definitions for one collectible"), Palette->ValidatePalette(Messages));
	TestTrue(TEXT("Dual-definition palette entry emits an error"),
		Messages.ContainsByPredicate([](const FGridArchetypeValidationMessage& Message)
		{
			return Message.Severity == EGridArchetypeValidationSeverity::Error;
		}));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridWorldObjectMIG05PaletteIconAuthorityTest,
	"Grimrock.WorldObjects.MIG05.PaletteRejectsCollectibleIconDuplication",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridWorldObjectMIG05PaletteIconAuthorityTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGridItemDefinitionAsset* Definition = NewObject<UGridItemDefinitionAsset>();
	Definition->ItemDefinitionId = TEXT("SilverKey");
	Definition->DisplayName = FText::FromString(TEXT("Silver Key"));
	Definition->Icon = NewObject<UTexture2D>(Definition);

	FGridObjectPaletteEntry Entry;
	Entry.EntryId = TEXT("SilverKey");
	Entry.DefaultItemDefinition = Definition;
	Entry.Icon = NewObject<UTexture2D>();

	TestFalse(TEXT("Direct collectible entry is invalid when Palette Icon duplicates ItemDefinition.Icon"), Entry.IsValidEntry());

	UGridObjectPaletteAsset* Palette = NewObject<UGridObjectPaletteAsset>();
	Palette->Entries.Add(Entry);
	TArray<FGridArchetypeValidationMessage> Messages;
	TestFalse(TEXT("Palette rejects duplicated collectible icon authority"), Palette->ValidatePalette(Messages));
	TestTrue(TEXT("Collectible icon duplication emits a specific validation error"),
		Messages.ContainsByPredicate([](const FGridArchetypeValidationMessage& Message)
		{
			return Message.Severity == EGridArchetypeValidationSeverity::Error && Message.Message.Contains(TEXT("DefaultItemDefinition.Icon"));
		}));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS