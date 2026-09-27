#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "Engine/Blueprint.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "Runtime/GridDoorActor.h"
#include "UI/GrimrockMainMenuWidget.h"
#include "UI/RPGCharacterCreationWidget.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridEditorCppClean04AssetConfigurationTest,
	"Grimrock.CppCleanup.CPP_CLEAN04.AssetConfiguration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorCppClean04AssetConfigurationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	// Native C++ must not silently manufacture presentation assets.
	const AGridDoorActor* NativeDoorCDO = GetDefault<AGridDoorActor>();
	if (!TestNotNull(TEXT("Native GridDoorActor CDO exists"), NativeDoorCDO))
	{
		return false;
	}
	TestNull(TEXT("Native GridDoorActor has no hard-coded chain support mesh"), NativeDoorCDO->ChainSupportMesh);
	TestNull(TEXT("Native GridDoorActor has no hard-coded chain moving mesh"), NativeDoorCDO->ChainMovingMesh);

	// The canonical runtime Blueprint owns the concrete chain presentation.
	UBlueprint* DoorBlueprint = LoadObject<UBlueprint>(
		nullptr,
		TEXT("/Game/GrimrockPrototype/Blueprints/Runtime/BP_GridDoorActor.BP_GridDoorActor"));
	if (!TestNotNull(TEXT("Canonical BP_GridDoorActor loads"), DoorBlueprint) ||
		!TestTrue(TEXT("Canonical BP_GridDoorActor has a generated class"), DoorBlueprint && DoorBlueprint->GeneratedClass != nullptr))
	{
		return false;
	}

	const AGridDoorActor* BlueprintDoorCDO = Cast<AGridDoorActor>(DoorBlueprint->GeneratedClass->GetDefaultObject());
	if (!TestNotNull(TEXT("Canonical BP_GridDoorActor CDO derives from AGridDoorActor"), BlueprintDoorCDO))
	{
		return false;
	}
	TestNotNull(TEXT("BP_GridDoorActor authors the chain support mesh"), BlueprintDoorCDO->ChainSupportMesh.Get());
	TestNotNull(TEXT("BP_GridDoorActor authors the chain moving mesh"), BlueprintDoorCDO->ChainMovingMesh.Get());

	// Every current definition that enables a chain must resolve to an actor class
	// whose CDO provides the required presentation assets.
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
	AssetRegistry.SearchAllAssets(true);

	FARFilter DoorDefinitionFilter;
	DoorDefinitionFilter.PackagePaths.Add(FName(TEXT("/Game/GrimrockPrototype")));
	DoorDefinitionFilter.ClassPaths.Add(UGridWorldObjectDefinitionAsset::StaticClass()->GetClassPathName());
	DoorDefinitionFilter.bRecursivePaths = true;
	DoorDefinitionFilter.bRecursiveClasses = true;

	TArray<FAssetData> DefinitionAssets;
	AssetRegistry.GetAssets(DoorDefinitionFilter, DefinitionAssets);

	int32 ChainEnabledDefinitionCount = 0;
	for (const FAssetData& AssetData : DefinitionAssets)
	{
		UGridWorldObjectDefinitionAsset* Definition = Cast<UGridWorldObjectDefinitionAsset>(AssetData.GetAsset());
		if (!Definition || Definition->SupportedType != EGridLevelObjectType::Door ||
			!Definition->DefaultBehavior.DoorAnimation.bHasChainMechanism)
		{
			continue;
		}

		++ChainEnabledDefinitionCount;
		TestNotNull(
			*FString::Printf(TEXT("%s has a RuntimeActorClass"), *AssetData.PackageName.ToString()),
			Definition->RuntimeActorClass.Get());

		const AGridDoorActor* DefinitionDoorCDO =
			Definition->RuntimeActorClass ? Cast<AGridDoorActor>(Definition->RuntimeActorClass->GetDefaultObject()) : nullptr;
		if (!TestNotNull(
			*FString::Printf(TEXT("%s resolves to an AGridDoorActor CDO"), *AssetData.PackageName.ToString()),
			DefinitionDoorCDO))
		{
			continue;
		}

		TestNotNull(
			*FString::Printf(TEXT("%s runtime class provides ChainSupportMesh"), *AssetData.PackageName.ToString()),
			DefinitionDoorCDO->ChainSupportMesh.Get());
		TestNotNull(
			*FString::Printf(TEXT("%s runtime class provides ChainMovingMesh"), *AssetData.PackageName.ToString()),
			DefinitionDoorCDO->ChainMovingMesh.Get());
	}
	TestTrue(TEXT("At least one production Door definition exercises chain presentation"), ChainEnabledDefinitionCount > 0);

	// WBP_MainMenu is the sole owner of the character-creation widget class.
	UBlueprint* MainMenuBlueprint = LoadObject<UBlueprint>(
		nullptr,
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/MainMenu/WBP_MainMenu.WBP_MainMenu"));
	if (!TestNotNull(TEXT("Canonical WBP_MainMenu loads"), MainMenuBlueprint) ||
		!TestTrue(TEXT("Canonical WBP_MainMenu has a generated class"), MainMenuBlueprint && MainMenuBlueprint->GeneratedClass != nullptr))
	{
		return false;
	}

	const UGrimrockMainMenuWidget* MainMenuCDO =
		Cast<UGrimrockMainMenuWidget>(MainMenuBlueprint->GeneratedClass->GetDefaultObject());
	if (!TestNotNull(TEXT("Canonical WBP_MainMenu CDO derives from UGrimrockMainMenuWidget"), MainMenuCDO))
	{
		return false;
	}

	const FClassProperty* CharacterCreationClassProperty =
		FindFProperty<FClassProperty>(UGrimrockMainMenuWidget::StaticClass(), TEXT("CharacterCreationWidgetClass"));
	if (!TestNotNull(TEXT("CharacterCreationWidgetClass reflected property exists"), CharacterCreationClassProperty))
	{
		return false;
	}

	UClass* ConfiguredCharacterCreationClass =
		Cast<UClass>(CharacterCreationClassProperty->GetObjectPropertyValue_InContainer(MainMenuCDO));
	TestNotNull(TEXT("WBP_MainMenu explicitly configures CharacterCreationWidgetClass"), ConfiguredCharacterCreationClass);
	if (ConfiguredCharacterCreationClass)
	{
		TestTrue(TEXT("Configured New Game widget derives from URPGCharacterCreationWidget"),
			ConfiguredCharacterCreationClass->IsChildOf(URPGCharacterCreationWidget::StaticClass()));
	}

	// Lock the architectural absence of native asset-path fallbacks.
	FString DoorSource;
	const FString DoorSourcePath = FPaths::Combine(
		FPaths::ProjectDir(), TEXT("Source/GrimrockPrototype/Private/Runtime/GridDoorActor.cpp"));
	TestTrue(TEXT("GridDoorActor source loads"), FFileHelper::LoadFileToString(DoorSource, *DoorSourcePath));
	TestFalse(TEXT("GridDoorActor has no ConstructorHelpers mesh fallback"),
		DoorSource.Contains(TEXT("ConstructorHelpers::FObjectFinder")));
	TestFalse(TEXT("GridDoorActor has no hard-coded chain support asset path"),
		DoorSource.Contains(TEXT("SM_Door_Chain_Support_01")));
	TestFalse(TEXT("GridDoorActor has no hard-coded chain moving asset path"),
		DoorSource.Contains(TEXT("SM_Door_Chain_Moving_01")));

	FString MainMenuSource;
	const FString MainMenuSourcePath = FPaths::Combine(
		FPaths::ProjectDir(), TEXT("Source/GrimrockPrototype/Private/UI/GrimrockMainMenuWidget.cpp"));
	TestTrue(TEXT("GrimrockMainMenuWidget source loads"), FFileHelper::LoadFileToString(MainMenuSource, *MainMenuSourcePath));
	TestFalse(TEXT("Main menu has no hard-coded character-creation LoadClass fallback"),
		MainMenuSource.Contains(TEXT("LoadClass<URPGCharacterCreationWidget>")));
	TestFalse(TEXT("Main menu has no hard-coded WBP_CharacterCreationWizard asset path"),
		MainMenuSource.Contains(TEXT("WBP_CharacterCreationWizard.WBP_CharacterCreationWizard_C")));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
