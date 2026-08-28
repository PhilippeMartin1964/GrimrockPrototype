#include "Modules/ModuleManager.h"
#include "EditorModeRegistry.h"
#include "EditorTools/GridLevelEdMode.h"
#include "EditorTools/GridLevelEditorActor.h"
#include "EditorTools/Widgets/SGridEditorLuaScriptsPanel.h"
#include "EditorTools/Widgets/SGridEditorWorkspaceTab.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPIEPlaytestRequest.h"

#include "Editor.h"
#include "EngineUtils.h"
#include "Framework/Docking/TabManager.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"

namespace
{
	const FName GridDungeonLevelsTabName(TEXT("GrimrockGridDungeonLevels"));
	const FName GridPlaytestValidationTabName(TEXT("GrimrockGridPlaytestValidation"));
	const FName GridToolsPaletteTabName(TEXT("GrimrockGridToolsPalette"));
	const FName GridSelectedObjectTabName(TEXT("GrimrockGridSelectedObject"));
	const FName GridLuaEditorTabName(TEXT("GrimrockLuaEditor"));
}

class FGrimrockPrototypeEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		FEditorModeRegistry::Get().RegisterMode<FGridLevelEdMode>(
			FGridLevelEdMode::EM_GridLevelEdModeId, FText::FromString(TEXT("Grimrock Grid Editor")), FSlateIcon(), true);

		RegisterGridWorkspaceTab(GridDungeonLevelsTabName, FText::FromString(TEXT("Dungeon Levels")),
			FText::FromString(TEXT("Navigate dungeon levels and inspect the level overview.")), EGridEditorWorkspaceTab::DungeonLevels);
		RegisterGridWorkspaceTab(GridPlaytestValidationTabName, FText::FromString(TEXT("PlayTest & Validation")),
			FText::FromString(TEXT("Validate the current level and prepare the playtest workflow.")), EGridEditorWorkspaceTab::PlaytestValidation);
		RegisterGridWorkspaceTab(GridToolsPaletteTabName, FText::FromString(TEXT("Tools & Palette")),
			FText::FromString(TEXT("Choose Grid Editor tools and object palette entries.")), EGridEditorWorkspaceTab::ToolsPalette);
		RegisterGridWorkspaceTab(GridSelectedObjectTabName, FText::FromString(TEXT("Selected Object")),
			FText::FromString(TEXT("Inspect the selected object and its connectors.")), EGridEditorWorkspaceTab::SelectedObject);

		// StartupModule can be reached again during editor module reloads.
		// Remove any stale registration first so the Window menu exposes one
		// canonical "Grimrock Lua Scripts" entry.
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(GridLuaEditorTabName);
		FGlobalTabmanager::Get()
			->RegisterNomadTabSpawner(GridLuaEditorTabName, FOnSpawnTab::CreateRaw(this, &FGrimrockPrototypeEditorModule::SpawnLuaEditorTab))
			.SetDisplayName(FText::FromString(TEXT("Grimrock Lua Scripts")))
			.SetTooltipText(FText::FromString(TEXT("Edit and validate level Lua scripts and Lua event bindings.")))
			.SetMenuType(ETabSpawnerMenuType::Enabled)
			.SetAutoGenerateMenuEntry(false);

		UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FGrimrockPrototypeEditorModule::RegisterMenus));

		PreBeginPIEHandle = FEditorDelegates::PreBeginPIE.AddRaw(this, &FGrimrockPrototypeEditorModule::HandlePreBeginPIE);
		BeginPIEHandle = FEditorDelegates::BeginPIE.AddRaw(this, &FGrimrockPrototypeEditorModule::HandleBeginPIE);
		CancelPIEHandle = FEditorDelegates::CancelPIE.AddRaw(this, &FGrimrockPrototypeEditorModule::HandleCancelPIE);
		WorldCleanupHandle = FWorldDelegates::OnWorldCleanup.AddRaw(this, &FGrimrockPrototypeEditorModule::HandleWorldCleanup);
	}

	virtual void ShutdownModule() override
	{
		FEditorDelegates::PreBeginPIE.Remove(PreBeginPIEHandle);
		FEditorDelegates::BeginPIE.Remove(BeginPIEHandle);
		FEditorDelegates::CancelPIE.Remove(CancelPIEHandle);
		FWorldDelegates::OnWorldCleanup.Remove(WorldCleanupHandle);
		GridPIEPlaytestRequest::Clear(TEXT("EditorModuleShutdown"));

		UToolMenus::UnRegisterStartupCallback(this);
		UToolMenus::UnregisterOwner(this);

		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(GridDungeonLevelsTabName);
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(GridPlaytestValidationTabName);
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(GridToolsPaletteTabName);
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(GridSelectedObjectTabName);
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(GridLuaEditorTabName);

		if (FModuleManager::Get().IsModuleLoaded("UnrealEd"))
		{
			FEditorModeRegistry::Get().UnregisterMode(FGridLevelEdMode::EM_GridLevelEdModeId);
		}
	}

private:
	void RegisterGridWorkspaceTab(const FName& TabName, const FText& DisplayName, const FText& Tooltip, EGridEditorWorkspaceTab WorkspaceTab)
	{
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(TabName);
		FGlobalTabmanager::Get()
			->RegisterNomadTabSpawner(TabName, FOnSpawnTab::CreateRaw(this, &FGrimrockPrototypeEditorModule::SpawnGridWorkspaceTab, WorkspaceTab))
			.SetDisplayName(DisplayName)
			.SetTooltipText(Tooltip)
			.SetMenuType(ETabSpawnerMenuType::Enabled)
			.SetAutoGenerateMenuEntry(false);
	}

	void AddWindowMenuEntry(FToolMenuSection& Section, const FName& EntryName, const FName& TabName, const FText& DisplayName, const FText& Tooltip)
	{
		TSharedPtr<FTabSpawnerEntry> TabSpawner = FGlobalTabmanager::Get()->FindTabSpawnerFor(TabName);
		if (!TabSpawner.IsValid())
		{
			return;
		}

		Section.AddMenuEntry(EntryName, DisplayName, Tooltip, FSlateIcon(),
			FGlobalTabmanager::Get()->GetUIActionForTabSpawnerMenuEntry(TabSpawner), EUserInterfaceActionType::ToggleButton, NAME_None);
	}

	void RegisterMenus()
	{
		FToolMenuOwnerScoped OwnerScoped(this);

		UToolMenu* WindowMenu = UToolMenus::Get()->ExtendMenu(TEXT("LevelEditor.MainMenu.Window"));
		if (!WindowMenu)
		{
			return;
		}

		FToolMenuSection& Section = WindowMenu->FindOrAddSection(TEXT("WindowLayout"));

		AddWindowMenuEntry(Section, FName(TEXT("GrimrockGridDungeonLevels")), GridDungeonLevelsTabName, FText::FromString(TEXT("Dungeon Levels")),
			FText::FromString(TEXT("Navigate dungeon levels and inspect the level overview.")));
		AddWindowMenuEntry(Section, FName(TEXT("GrimrockGridPlaytestValidation")), GridPlaytestValidationTabName,
			FText::FromString(TEXT("PlayTest & Validation")), FText::FromString(TEXT("Validate the current level and prepare the playtest workflow.")));
		AddWindowMenuEntry(Section, FName(TEXT("GrimrockGridToolsPalette")), GridToolsPaletteTabName, FText::FromString(TEXT("Tools & Palette")),
			FText::FromString(TEXT("Choose Grid Editor tools and object palette entries.")));
		AddWindowMenuEntry(Section, FName(TEXT("GrimrockGridSelectedObject")), GridSelectedObjectTabName, FText::FromString(TEXT("Selected Object")),
			FText::FromString(TEXT("Inspect the selected object and its connectors.")));
		AddWindowMenuEntry(Section, FName(TEXT("GrimrockLuaScripts")), GridLuaEditorTabName, FText::FromString(TEXT("Grimrock Lua Scripts")),
			FText::FromString(TEXT("Edit and validate level Lua scripts and Lua event bindings.")));
	}

	TSharedRef<SDockTab> SpawnGridWorkspaceTab(const FSpawnTabArgs& SpawnTabArgs, EGridEditorWorkspaceTab WorkspaceTab)
	{
		(void)SpawnTabArgs;
		return SNew(SDockTab).TabRole(ETabRole::NomadTab)[SNew(SGridEditorWorkspaceTab).WorkspaceTab(WorkspaceTab)];
	}

	TSharedRef<SDockTab> SpawnLuaEditorTab(const FSpawnTabArgs& SpawnTabArgs)
	{
		(void)SpawnTabArgs;
		return SNew(SDockTab).TabRole(ETabRole::NomadTab)[SNew(SGridEditorLuaScriptsPanel)];
	}

	AGridLevelEditorActor* FindEditorActorForPIEPreparation() const
	{
		if (!GEditor)
		{
			return nullptr;
		}

		UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();
		if (!EditorWorld)
		{
			return nullptr;
		}

		for (TActorIterator<AGridLevelEditorActor> It(EditorWorld); It; ++It)
		{
			return *It;
		}

		return nullptr;
	}

	void HandlePreBeginPIE(bool bIsSimulating)
	{
		bRequestStopPIEAfterBegin = false;
		GridPIEPlaytestRequest::Clear(TEXT("HandlePreBeginPIE"));

		AGridLevelEditorActor* EditorActor = FindEditorActorForPIEPreparation();
		if (!EditorActor || !EditorActor->bAutoPreparePIE)
		{
			return;
		}

		UE_LOG(LogTemp, Log, TEXT("Auto PIE preparation started for %s."), *EditorActor->GetName());

		FString Error;
		if (!EditorActor->PreparePIETestFromStartInternal(Error))
		{
			UE_LOG(LogTemp, Error, TEXT("Auto PIE preparation failed: %s"), *Error);

			if (EditorActor->bAbortPIEOnPreparationError)
			{
				bRequestStopPIEAfterBegin = true;
				UE_LOG(LogTemp, Error, TEXT("PIE aborted because bAbortPIEOnPreparationError is true."));
			}
			GridPIEPlaytestRequest::Clear(TEXT("PIEPreparationFailed"));
			return;
		}

		GridPIEPlaytestRequest::BeginFreshPlaytest(EditorActor->PreviewRuntimeActor.Get());

		const UGridLevelAsset* LevelAsset = EditorActor->LevelAsset;
		UE_LOG(LogTemp, Log, TEXT("Auto PIE preparation OK. LevelAsset=%s, DungeonAsset=%s, CurrentDungeonLevelId=%s, StartCell=(%d,%d), Facing=%s."),
			LevelAsset ? *LevelAsset->GetPathName() : TEXT("None"), EditorActor->DungeonAsset ? *EditorActor->DungeonAsset->GetPathName() : TEXT("None"),
			*EditorActor->CurrentDungeonLevelId.ToString(), LevelAsset ? LevelAsset->StartCellX : INDEX_NONE, LevelAsset ? LevelAsset->StartCellY : INDEX_NONE,
			LevelAsset ? *StaticEnum<EGridEdge>()->GetNameStringByValue(static_cast<int64>(LevelAsset->StartFacing)) : TEXT("None"));
	}

	void HandleBeginPIE(bool bIsSimulating)
	{
		if (!bRequestStopPIEAfterBegin || !GEditor)
		{
			return;
		}

		bRequestStopPIEAfterBegin = false;
		GEditor->RequestEndPlayMap();
	}

	void HandleCancelPIE()
	{
		if (!GEditor || !GEditor->PlayWorld)
		{
			GridPIEPlaytestRequest::Clear(TEXT("HandleCancelPIEBeforeWorld"));
		}
	}

	void HandleWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources)
	{
		if (World && World->WorldType == EWorldType::PIE)
		{
			GridPIEPlaytestRequest::Clear(TEXT("PIEWorldCleanup"));
		}
	}

private:
	FDelegateHandle PreBeginPIEHandle;
	FDelegateHandle BeginPIEHandle;
	FDelegateHandle CancelPIEHandle;
	FDelegateHandle WorldCleanupHandle;
	bool bRequestStopPIEAfterBegin = false;
};

IMPLEMENT_MODULE(FGrimrockPrototypeEditorModule, GrimrockPrototypeEditor)
