#include "Modules/ModuleManager.h"
#include "EditorModeRegistry.h"
#include "EditorTools/GridLevelEdMode.h"
#include "EditorTools/GridLevelEditorActor.h"

#include "Editor.h"
#include "EngineUtils.h"

class FGrimrockPrototypeEditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        FEditorModeRegistry::Get().RegisterMode<FGridLevelEdMode>(
            FGridLevelEdMode::EM_GridLevelEdModeId,
            FText::FromString(TEXT("Grimrock Grid Editor")),
            FSlateIcon(),
            true);

        PreBeginPIEHandle = FEditorDelegates::PreBeginPIE.AddRaw (this, &FGrimrockPrototypeEditorModule::HandlePreBeginPIE);
        BeginPIEHandle = FEditorDelegates::BeginPIE.AddRaw (this, &FGrimrockPrototypeEditorModule::HandleBeginPIE);
    }

    virtual void ShutdownModule() override
    {
        FEditorDelegates::PreBeginPIE.Remove (PreBeginPIEHandle);
        FEditorDelegates::BeginPIE.Remove (BeginPIEHandle);

        if (FModuleManager::Get().IsModuleLoaded("UnrealEd"))
        {
            FEditorModeRegistry::Get().UnregisterMode(FGridLevelEdMode::EM_GridLevelEdModeId);
        }
    }

private:
    AGridLevelEditorActor* FindEditorActorForPIEPreparation () const
    {
        if (!GEditor)
        {
            return nullptr;
        }

        UWorld* EditorWorld = GEditor->GetEditorWorldContext ().World ();
        if (!EditorWorld)
        {
            return nullptr;
        }

        for (TActorIterator<AGridLevelEditorActor> It (EditorWorld); It; ++It)
        {
            return *It;
        }

        return nullptr;
    }

    void HandlePreBeginPIE (bool bIsSimulating)
    {
        bRequestStopPIEAfterBegin = false;

        AGridLevelEditorActor* EditorActor = FindEditorActorForPIEPreparation ();
        if (!EditorActor || !EditorActor->bAutoPreparePIE)
        {
            return;
        }

        UE_LOG (LogTemp, Log, TEXT ("Auto PIE preparation started for %s."), *EditorActor->GetName ());

        FString Error;
        if (!EditorActor->PreparePIETestFromStartInternal (Error))
        {
            UE_LOG (LogTemp, Error, TEXT ("Auto PIE preparation failed: %s"), *Error);

            if (EditorActor->bAbortPIEOnPreparationError)
            {
                bRequestStopPIEAfterBegin = true;
                UE_LOG (LogTemp, Error, TEXT ("PIE aborted because bAbortPIEOnPreparationError is true."));
            }
            return;
        }

        const UGridLevelAsset* LevelAsset = EditorActor->LevelAsset;
        UE_LOG (
            LogTemp,
            Log,
            TEXT ("Auto PIE preparation OK. LevelAsset=%s, DungeonAsset=%s, CurrentDungeonLevelId=%s, StartCell=(%d,%d), Facing=%s."),
            LevelAsset ? *LevelAsset->GetPathName () : TEXT ("None"),
            EditorActor->DungeonAsset ? *EditorActor->DungeonAsset->GetPathName () : TEXT ("None"),
            *EditorActor->CurrentDungeonLevelId.ToString (),
            LevelAsset ? LevelAsset->StartCellX : INDEX_NONE,
            LevelAsset ? LevelAsset->StartCellY : INDEX_NONE,
            LevelAsset ? *StaticEnum<EGridEdge> ()->GetNameStringByValue (static_cast<int64> (LevelAsset->StartFacing)) : TEXT ("None"));
    }

    void HandleBeginPIE (bool bIsSimulating)
    {
        if (!bRequestStopPIEAfterBegin || !GEditor)
        {
            return;
        }

        bRequestStopPIEAfterBegin = false;
        GEditor->RequestEndPlayMap ();
    }

private:
    FDelegateHandle PreBeginPIEHandle;
    FDelegateHandle BeginPIEHandle;
    bool bRequestStopPIEAfterBegin = false;
};

IMPLEMENT_MODULE(FGrimrockPrototypeEditorModule, GrimrockPrototypeEditor)
