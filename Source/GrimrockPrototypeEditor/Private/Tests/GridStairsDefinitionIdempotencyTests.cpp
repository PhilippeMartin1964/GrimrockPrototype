#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridObjectPaletteAsset.h"
#include "Core/GridRelocationUtils.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "EditorTools/GridLevelEditorActor.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Internationalization/TextInspector.h"
#include "UObject/Package.h"

namespace
{
	FString GetLocalizationKey(const FText& Text)
	{
		const FString* Key = FTextInspector::GetKey(Text);
		return Key ? *Key : FString();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridStairsDefinitionIdempotencyTest, "Grimrock.Editor.Stairs.DefinitionIdempotency",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridStairsDefinitionIdempotencyTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGridWorldObjectDefinitionAsset* Up = LoadObject<UGridWorldObjectDefinitionAsset>(nullptr,
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/GridObjectArchetypeAsset/DA_Stairs_Up.DA_Stairs_Up"));
	UGridWorldObjectDefinitionAsset* Down = LoadObject<UGridWorldObjectDefinitionAsset>(nullptr,
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/GridObjectArchetypeAsset/DA_Stairs_Down.DA_Stairs_Down"));
	if (!TestNotNull(TEXT("Stairs Up asset"), Up) || !TestNotNull(TEXT("Stairs Down asset"), Down) || !GEngine)
	{
		return false;
	}

	TestFalse(TEXT("Up default relocation intentionally unset"), GridRelocation::IsConfigured(Up->DefaultBehavior.Relocation));
	TestFalse(TEXT("Down default relocation intentionally unset"), GridRelocation::IsConfigured(Down->DefaultBehavior.Relocation));

	const FString UpDisplayKey = GetLocalizationKey(Up->DisplayName);
	const FString UpDescriptionKey = GetLocalizationKey(Up->Description);
	const FString DownDisplayKey = GetLocalizationKey(Down->DisplayName);
	const FString DownDescriptionKey = GetLocalizationKey(Down->Description);
	const bool bUpWasDirty = Up->GetOutermost()->IsDirty();
	const bool bDownWasDirty = Down->GetOutermost()->IsDirty();

	const UWorld::InitializationValues Values = UWorld::InitializationValues()
		.AllowAudioPlayback(false).RequiresHitProxies(false).CreatePhysicsScene(false)
		.CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Editor, false, TEXT("StairsDefinitionIdempotencyWorld"),
		nullptr, true, ERHIFeatureLevel::Num, &Values);
	if (!TestNotNull(TEXT("Transient editor world"), World))
	{
		return false;
	}

	FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Editor);
	Context.SetCurrentWorld(World);
	AGridLevelEditorActor* Editor = World->SpawnActor<AGridLevelEditorActor>();
	if (TestNotNull(TEXT("Grid editor actor"), Editor))
	{
		UGridObjectPaletteAsset* Palette = NewObject<UGridObjectPaletteAsset>(Editor);
		Editor->ObjectPalette = Palette;

		const auto AddEntry = [Palette](FName EntryId, UGridWorldObjectDefinitionAsset* Definition)
		{
			FGridObjectPaletteEntry& Entry = Palette->Entries.AddDefaulted_GetRef();
			Entry.EntryId = EntryId;
			Entry.PaletteCategory = FName(TEXT("Navigation"));
			Entry.DefaultWorldObjectDefinition = Definition;
		};
		AddEntry(FName(TEXT("Stairs_Up")), Up);
		AddEntry(FName(TEXT("Stairs_Down")), Down);

		const bool bPaletteWasDirty = Palette->GetOutermost()->IsDirty();
		for (int32 Run = 0; Run < 2; ++Run)
		{
			FString Error;
			const bool bSucceeded = Editor->EnsureStairsRelocationDefinitions(Error);
			TestTrue(*FString::Printf(TEXT("Ensure call %d succeeds: %s"), Run + 1, *Error), bSucceeded);
		}

		TestEqual(TEXT("Up DisplayName key preserved"), GetLocalizationKey(Up->DisplayName), UpDisplayKey);
		TestEqual(TEXT("Up Description key preserved"), GetLocalizationKey(Up->Description), UpDescriptionKey);
		TestEqual(TEXT("Down DisplayName key preserved"), GetLocalizationKey(Down->DisplayName), DownDisplayKey);
		TestEqual(TEXT("Down Description key preserved"), GetLocalizationKey(Down->Description), DownDescriptionKey);
		TestEqual(TEXT("Up package dirty state unchanged"), Up->GetOutermost()->IsDirty(), bUpWasDirty);
		TestEqual(TEXT("Down package dirty state unchanged"), Down->GetOutermost()->IsDirty(), bDownWasDirty);
		TestEqual(TEXT("Palette package dirty state unchanged"), Palette->GetOutermost()->IsDirty(), bPaletteWasDirty);
	}

	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}

#endif
