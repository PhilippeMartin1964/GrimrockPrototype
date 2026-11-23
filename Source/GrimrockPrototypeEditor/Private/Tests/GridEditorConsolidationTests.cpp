#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Core/GridLevelAsset.h"
#include "Editor.h"
#include "Editor/Transactor.h"
#include "EditorTools/GridEditorActorLookup.h"
#include "EditorTools/GridEditorLuaService.h"
#include "EditorTools/GridLuaAuthoringCompiler.h"
#include "EditorTools/GridLuaPersistentCompatibility.h"
#include "Engine/Level.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/GridEditorAssetSave.h"
#include "ScopedTransaction.h"
#include "Selection.h"
#include "Tests/GridAutomationSaveIsolation.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
	struct FPackageSandbox
	{
		FString Root =
			FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("EditorConsolidation") / FGuid::NewGuid().ToString(EGuidFormats::Digits));
		FString Mount = TEXT("/EditorConsolidation_") + FGuid::NewGuid().ToString(EGuidFormats::Digits) + TEXT("/");
		TArray<TStrongObjectPtr<URPGClassAsset>> Assets;

		FPackageSandbox()
		{
			IFileManager::Get().MakeDirectory(*Root, true);
			FPackageName::RegisterMountPoint(Mount, Root + TEXT("/"));
		}

		URPGClassAsset* MakeAsset()
		{
			const FString Name = FString::Printf(TEXT("DA_Fixture_%d"), Assets.Num());
			auto* Package = CreatePackage(*(Mount + Name));
			auto* Asset = NewObject<URPGClassAsset>(Package, *Name, RF_Public | RF_Standalone | RF_Transactional);
			Asset->ClassId = FName(*Name);
			Asset->DisplayName = FText::FromString(Name);
			Package->SetDirtyFlag(false);
			Assets.Emplace(Asset);
			return Asset;
		}

		FString Filename(URPGClassAsset* Asset) const
		{
			return FPackageName::LongPackageNameToFilename(Asset->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension());
		}

		~FPackageSandbox()
		{
			for (auto& Asset : Assets)
			{
				const FString File = Filename(Asset.Get());
				// Only files created in this unique mounted directory may be removed.
				if (FPaths::IsUnderDirectory(File, Root))
				{
					FAssetRegistryModule::AssetDeleted(Asset.Get());
					IFileManager::Get().Delete(*File, false, true);
					for (const TCHAR* Extension : { TEXT("uexp"), TEXT("ubulk") })
						IFileManager::Get().Delete(*FPaths::ChangeExtension(File, Extension), false, true);
				}
				Asset->GetOutermost()->SetDirtyFlag(false);
				Asset->ClearFlags(RF_Public | RF_Standalone);
				Asset->MarkAsGarbage();
			}
			FPackageName::UnRegisterMountPoint(Mount, Root + TEXT("/"));
			IFileManager::Get().DeleteDirectory(*Root, false, false);
		}
	};

	struct FLookupWorld
	{
		UWorld* World = nullptr;
		explicit FLookupWorld(EWorldType::Type Type)
		{
			const auto Values = UWorld::InitializationValues()
									.AllowAudioPlayback(false)
									.RequiresHitProxies(false)
									.CreatePhysicsScene(false)
									.CreateNavigation(false)
									.CreateAISystem(false)
									.ShouldSimulatePhysics(false)
									.SetTransactional(false);
			World = UWorld::CreateWorld(Type, false, FName(*(TEXT("EditorConsolidation_") + FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true,
				ERHIFeatureLevel::Num, &Values);
		}
		~FLookupWorld()
		{
			if (World)
				World->DestroyWorld(false);
		}
	};

	FGridLuaScriptSource Script(FName Id, const TCHAR* Text)
	{
		FGridLuaScriptSource Result;
		Result.ScriptId = Id;
		Result.Source = Text;
		return Result;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridEditorSaveContracts, "Grimrock.Editor.Consolidation.AssetSave", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorSaveContracts::RunTest(const FString&)
{
	if (!GridAutomationSaveIsolation::Require(*this))
		return false;
	struct FSaveCase
	{
		bool ClassDiagnostics;
		TFunction<bool(URPGClassAsset*, FString&)> Save;
	};
	const FSaveCase Cases[] = { { false, GridEditorAssetSave::SaveAuthoredAsset }, { true, GridEditorAssetSave::SaveClassAsset } };
	FString CleanupRoot;
	{
		FPackageSandbox Sandbox;
		CleanupRoot = Sandbox.Root;
		for (const auto& Case : Cases)
		{
			FString Error;
			TestFalse(TEXT("Null object rejected"), Case.Save(nullptr, Error));
			TestEqual(TEXT("Null diagnostic preserved"), Error,
				FString(Case.ClassDiagnostics ? TEXT("Cannot save a null class asset.") : TEXT("Cannot save a null authored asset.")));
			auto* Asset = Sandbox.MakeAsset();
			auto* Package = Asset->GetOutermost();
			const FString File = Sandbox.Filename(Asset);
			TestTrue(TEXT("Destination strictly within isolated mount"), FPaths::IsUnderDirectory(File, Sandbox.Root));
			TestFalse(TEXT("Fresh package has no file"), IFileManager::Get().FileExists(*File));
			Error = TEXT("unchanged-on-success");
			{
				FScopedTransaction Transaction(FText::FromString(TEXT("Private authoring save fixture")));
				Asset->Modify();
				const bool WasActive = GEditor && GEditor->Trans && GEditor->Trans->IsActive();
				TestTrue(TEXT("Save succeeds"), Case.Save(Asset, Error));
				TestEqual(TEXT("Caller transaction state preserved"), GEditor && GEditor->Trans && GEditor->Trans->IsActive(), WasActive);
				Transaction.Cancel();
			}
			TestEqual(TEXT("Success does not reset caller diagnostic"), Error, FString(TEXT("unchanged-on-success")));
			TestTrue(TEXT("Package file written"), IFileManager::Get().FileSize(*File) > 0);
			TestFalse(TEXT("Successful save clears dirty package"), Package->IsDirty());
			TestTrue(TEXT("Transactional object flag preserved"), Asset->HasAnyFlags(RF_Transactional));
			TArray<uint8> Before;
			TestTrue(TEXT("Observe saved bytes"), FFileHelper::LoadFileToArray(Before, *File));

			// Exercise the actual UE permission gate, only for this fixture.
			const auto PreviousPermission = FCoreUObjectDelegates::IsPackageOKToSaveDelegate;
			int32 PermissionCalls = 0;
			FCoreUObjectDelegates::IsPackageOKToSaveDelegate.BindLambda(
				[Package, PreviousPermission, &PermissionCalls](UPackage* Candidate, const FString& Filename, FOutputDevice* Output)
				{
					if (Candidate == Package)
					{
						++PermissionCalls;
						return false;
					}
					return !PreviousPermission.IsBound() || PreviousPermission.Execute(Candidate, Filename, Output);
				});
			bool Denied;
			{
				ON_SCOPE_EXIT
				{
					FCoreUObjectDelegates::IsPackageOKToSaveDelegate = PreviousPermission;
				};
				Denied = !Case.Save(Asset, Error);
			}
			TestTrue(TEXT("SavePackage failure propagated"), Denied);
			TestEqual(TEXT("Native permission delegate consulted"), PermissionCalls, 1);
			const FString Expected = Case.ClassDiagnostics ? FString::Printf(TEXT("Failed to save '%s'."), *Asset->GetPathName())
														   : FString::Printf(TEXT("Failed to save '%s' to '%s'."), *Asset->GetPathName(), *File);
			TestEqual(TEXT("Failure diagnostic exactly preserved"), Error, Expected);
			TestTrue(TEXT("Failed save retains dirty state"), Package->IsDirty());
			TArray<uint8> After;
			TestTrue(TEXT("Existing file still readable"), FFileHelper::LoadFileToArray(After, *File));
			TestTrue(TEXT("Denied write leaves existing file byte-identical"), Before == After);

			Package->MarkAsUnloaded();
			TestFalse(TEXT("Saved fixture can represent a partially loaded package"), Package->IsFullyLoaded());
			TestFalse(TEXT("Partially loaded package rejected by SavePackage"), Case.Save(Asset, Error));
			TestTrue(TEXT("Rejected package remains dirty"), Package->IsDirty());
			TestTrue(TEXT("Invalid package has save failure diagnostic"), Error.StartsWith(TEXT("Failed to save '")));
			TestTrue(TEXT("Original fixture file remains readable"), FFileHelper::LoadFileToArray(After, *File));
			TestTrue(TEXT("Partial package cannot overwrite existing bytes"), Before == After);
			Package->MarkAsFullyLoaded();
		}
	}
	TestFalse(TEXT("Only owned fixture files removed; private directory empty and deleted"), IFileManager::Get().DirectoryExists(*CleanupRoot));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridEditorLuaCompatibility, "Grimrock.Editor.Consolidation.LuaPredicates", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorLuaCompatibility::RunTest(const FString&)
{
	using namespace GridLuaPersistentCompatibility;
	FGridLuaPersistentVariableDefinition A;
	A.VariableId = TEXT("Gate");
	A.Type = EGridLuaPersistentValueType::Bool;
	auto B = A;
	TestTrue(TEXT("Identical definitions"), AreDefinitionsCompatible(A, B));
	B.DefaultInt32Value = 123;
	TestTrue(TEXT("Inactive int field ignored for Bool"), AreDefinitionsCompatible(A, B));
	B.bDefaultBoolValue = !A.bDefaultBoolValue;
	TestFalse(TEXT("Bool defaults differ"), AreDefinitionsCompatible(A, B));
	B = A;
	B.VariableId = TEXT("Other");
	TestFalse(TEXT("Identity differs"), AreDefinitionsCompatible(A, B));
	B = A;
	B.Type = EGridLuaPersistentValueType::Int32;
	TestFalse(TEXT("Type differs"), AreDefinitionsCompatible(A, B));
	A = B;
	A.DefaultInt32Value = MIN_int32;
	B.DefaultInt32Value = MIN_int32;
	TestTrue(TEXT("Minimum integer defaults preserved"), AreDefinitionsCompatible(A, B));
	B.DefaultInt32Value = MAX_int32;
	TestFalse(TEXT("Integer defaults differ"), AreDefinitionsCompatible(A, B));
	A.DefaultInt32Value = B.DefaultInt32Value;
	A.bDefaultBoolValue = !B.bDefaultBoolValue;
	TestTrue(TEXT("Inactive Bool field ignored for Int32"), AreDefinitionsCompatible(A, B));
	FGridLevelVariableDefinition Level;
	Level.VariableId = A.VariableId;
	Level.Type = EGridLevelVariableType::Int32;
	Level.DefaultInt32Value = MAX_int32;
	TestTrue(TEXT("Level-variable projection matches"), MatchesLevelVariable(A, Level));
	Level.DefaultInt32Value = 0;
	TestFalse(TEXT("Level-variable default mismatch"), MatchesLevelVariable(A, Level));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridEditorLuaServiceContracts, "Grimrock.Editor.Consolidation.LuaServices", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorLuaServiceContracts::RunTest(const FString&)
{
	FLookupWorld World(EWorldType::EditorPreview);
	if (!TestNotNull(TEXT("Transient world"), World.World))
		return false;
	auto* Actor = World.World->SpawnActor<AGridLevelEditorActor>();
	auto* Level = NewObject<UGridLevelAsset>(GetTransientPackage(), NAME_None, RF_Transactional);
	Actor->LevelAsset = Level;
	Level->LuaScripts = { Script(TEXT("First"), TEXT("persistent = { Gate = false, Counter = 3 }")), Script(TEXT("Second"), TEXT("local untouched = 0")) };
	FGridLevelVariableDefinition Unrelated;
	Unrelated.VariableId = TEXT("Unrelated");
	Unrelated.Type = EGridLevelVariableType::Int32;
	Unrelated.DefaultInt32Value = 42;
	Level->LevelVariables.Add(Unrelated);
	FString Error;
	TestTrue(TEXT("Compatible duplicate declarations synchronize"),
		GridEditorLuaService::SetScriptSource(*Level, TEXT("Second"), TEXT("persistent = { Counter = 3, Gate = false }"), Error));
	TestEqual(TEXT("Duplicate declarations merged, unrelated variable retained"), Level->LevelVariables.Num(), 3);
	TestEqual(TEXT("Existing order retained"), Level->LevelVariables[0].VariableId, Unrelated.VariableId);
	TestEqual(TEXT("New variables appended in lexical order"), Level->LevelVariables[1].VariableId, FName(TEXT("Counter")));
	TestEqual(TEXT("Second appended variable"), Level->LevelVariables[2].VariableId, FName(TEXT("Gate")));
	FGridLuaCompileResult Result;
	TestTrue(TEXT("Compatible level compiles"), FGridLuaAuthoringCompiler::CompileLevel(*Actor, Result));
	const FString SourceBefore = Level->LuaScripts[1].Source;
	const auto VariablesBefore = Level->LevelVariables;
	for (const TCHAR* Source : { TEXT("persistent = { Counter = true }"), TEXT("persistent = { Counter = 4 }"), TEXT("persistent = { Value = 'invalid' }"),
			 TEXT("persistent = { Value = 1.5 }"), TEXT("persistent = {") })
	{
		TestFalse(TEXT("Incompatible/invalid source refused"), GridEditorLuaService::SetScriptSource(*Level, TEXT("Second"), Source, Error));
		TestFalse(TEXT("Failure has diagnostic"), Error.IsEmpty());
		TestEqual(TEXT("Rejected source leaves script unchanged"), Level->LuaScripts[1].Source, SourceBefore);
		TestEqual(TEXT("Rejected source keeps definition count"), Level->LevelVariables.Num(), VariablesBefore.Num());
		for (int32 Index = 0; Index < VariablesBefore.Num() && Level->LevelVariables.IsValidIndex(Index); ++Index)
			TestTrue(TEXT("Rejected source leaves each persistent definition unchanged"),
				FGridLevelVariableDefinition::StaticStruct()->CompareScriptStruct(&Level->LevelVariables[Index], &VariablesBefore[Index], 0));
		TestFalse(TEXT("Compiler also refuses invalid candidate"),
			FGridLuaAuthoringCompiler::CompileScriptDraft(*Actor, TEXT("Second"), TEXT("Second"), Source, Result));
		TestTrue(TEXT("Compilation has E001/E103 diagnostic"),
			Result.Diagnostics.ContainsByPredicate(
				[](const FGridLuaCompileDiagnostic& D)
				{
					return D.Code == TEXT("E001") || D.Code == TEXT("E103");
				}));
		TestEqual(TEXT("Draft compilation never mutates source"), Level->LuaScripts[1].Source, SourceBefore);
	}
	TestTrue(TEXT("Lua table keeps its existing last-value rule"),
		GridEditorLuaService::SetScriptSource(*Level, TEXT("Second"), TEXT("persistent = { Counter = 8, Counter = 3, Gate = false }"), Error));
	Level->LuaScripts[1].ScriptId = Level->LuaScripts[0].ScriptId;
	TestFalse(TEXT("Duplicate ScriptId is invalid"), FGridLuaAuthoringCompiler::CompileLevel(*Actor, Result));
	TestTrue(TEXT("Duplicate ScriptId diagnostics retained"),
		Result.Diagnostics.ContainsByPredicate(
			[](const FGridLuaCompileDiagnostic& D)
			{
				return D.Code == TEXT("E001");
			}));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridEditorActorLookupContracts, "Grimrock.Editor.Consolidation.ActorLookup", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorActorLookupContracts::RunTest(const FString&)
{
	if (!TestNotNull(TEXT("Editor context exists"), GEditor))
		return false;
	FLookupWorld EditorWorld(EWorldType::Editor), PIEWorld(EWorldType::PIE), OtherWorld(EWorldType::EditorPreview);
	if (!EditorWorld.World || !PIEWorld.World || !OtherWorld.World)
		return false;
	auto& Context = GEditor->GetEditorWorldContext();
	TStrongObjectPtr<UWorld> OriginalWorld(Context.World());
	const auto OriginalPlayWorld = GEditor->PlayWorld;
	TArray<UObject*> Selection;
	GEditor->GetSelectedActors()->GetSelectedObjects(Selection);
	ON_SCOPE_EXIT
	{
		GEditor->SelectNone(false, true, false);
		GEditor->PlayWorld = OriginalPlayWorld;
		Context.SetCurrentWorld(OriginalWorld.Get());
		for (auto* Object : Selection)
			if (auto* Actor = Cast<AActor>(Object); IsValid(Actor))
				GEditor->SelectActor(Actor, true, false);
	};
	Context.SetCurrentWorld(EditorWorld.World);
	GEditor->PlayWorld = PIEWorld.World;
	auto* PIEActor = PIEWorld.World->SpawnActor<AGridLevelEditorActor>();
	auto* OtherActor = OtherWorld.World->SpawnActor<AGridLevelEditorActor>();
	TestNull(TEXT("Null world has no fallback"), GridEditorActorLookup::FindFirstInWorld(nullptr));
	TestNull(TEXT("Absent Editor actor never falls back to PIE/another world"), GridEditorActorLookup::FindInEditorWorld());
	TestEqual(TEXT("Explicit PIE lookup stays in that world"), GridEditorActorLookup::FindFirstInWorld(PIEWorld.World), PIEActor);
	TestEqual(TEXT("Explicit other-world lookup stays in that world"), GridEditorActorLookup::FindFirstInWorld(OtherWorld.World), OtherActor);
	auto* First = EditorWorld.World->SpawnActor<AGridLevelEditorActor>();
	TestEqual(TEXT("One actor is found"), GridEditorActorLookup::FindInEditorWorld(), First);
	auto* SubLevel = NewObject<ULevel>(EditorWorld.World, NAME_None, RF_Transient);
	SubLevel->OwningWorld = EditorWorld.World;
	SubLevel->bIsVisible = true;
	EditorWorld.World->AddLevel(SubLevel);
	FActorSpawnParameters Params;
	Params.OverrideLevel = SubLevel;
	auto* Second = EditorWorld.World->SpawnActor<AGridLevelEditorActor>(AGridLevelEditorActor::StaticClass(), FTransform::Identity, Params);
	// UE iteration order is the existing tie-break, not spawn/selection priority.
	TActorIterator<AGridLevelEditorActor> Iterator(EditorWorld.World);
	auto* Expected = *Iterator;
	auto* NotFirst = Expected == First ? Second : First;
	GEditor->SelectNone(false, true, false);
	GEditor->SelectActor(NotFirst, true, false);
	EditorWorld.World->SetCurrentLevel(NotFirst->GetLevel());
	TestEqual(TEXT("Selection/current level do not override first actor policy"), GridEditorActorLookup::FindInEditorWorld(), Expected);
	TestEqual(TEXT("Lookup never changes selected level"), EditorWorld.World->GetCurrentLevel(), NotFirst->GetLevel());
	TestTrue(TEXT("Selection stays unchanged"), GEditor->GetSelectedActors()->IsSelected(NotFirst));
	GEditor->SelectNone(false, true, false);
	EditorWorld.World->DestroyActor(Expected, true, false);
	TestEqual(TEXT("Destroyed actor skipped; remaining actor returned"), GridEditorActorLookup::FindInEditorWorld(), NotFirst);
	EditorWorld.World->DestroyActor(NotFirst, true, false);
	TestNull(TEXT("Empty Editor world still never falls back to PIE"), GridEditorActorLookup::FindInEditorWorld());
	TestTrue(TEXT("PIE actor was not selected or destroyed"), IsValid(PIEActor) && !GEditor->GetSelectedActors()->IsSelected(PIEActor));
	return true;
}

#endif
