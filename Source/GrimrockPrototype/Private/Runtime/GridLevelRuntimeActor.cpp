#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPIEPlaytestRequest.h"
#include "Runtime/GridMonsterEncounterComponent.h"
#include "Core/GridTypes.h"
#include "Core/GridDirectionUtils.h"
#include "Core/GridObjectArchetypeAsset.h"
#include "Runtime/GridRuntimeObjectActor.h"
#include "Runtime/GridActivationComponent.h"
#include "Runtime/GridDoorSystemComponent.h"
#include "Runtime/GridEditorPreviewComponent.h"
#include "Runtime/GrimrockGameMode.h"
#include "Runtime/GridItemActor.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLeverActor.h"
#include "Runtime/GridMechanismActor.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"
#include "Runtime/Combat/GridPlayerAttackPresentationComponent.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterBehaviorComponent.h"
#include "Runtime/Monsters/GridMonsterCombatComponent.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "Runtime/Monsters/GridMonsterIdleVariationComponent.h"
#include "Runtime/Monsters/GridMonsterMovementComponent.h"
#include "Runtime/Monsters/GridMonsterOccupancySubsystem.h"
#include "Runtime/GridPressurePlateActor.h"
#include "Runtime/GridReceptacleActor.h"
#include "Runtime/GridThrownItemActor.h"
#include "Runtime/GridWallLockActor.h"
#include "UI/ReadableMessageWidget.h"
#include "Blueprint/UserWidget.h"
#include "EngineUtils.h"
#include "GameFramework/GameModeBase.h"
#include "Kismet/GameplayStatics.h"

namespace
{
    const FName SingleLevelRuntimeStateId (TEXT ("SingleLevel"));

    bool IsCardinalMonsterSpawnFacing (
        EGridEdge Facing)
    {
        return Facing == EGridEdge::North ||
            Facing == EGridEdge::East ||
            Facing == EGridEdge::South ||
            Facing == EGridEdge::West;
    }

    FName ResolveRuntimeStateLevelId (const UGridDungeonAsset* DungeonAsset, FName CurrentDungeonLevelId)
    {
        if (DungeonAsset && !CurrentDungeonLevelId.IsNone ())
        {
            return CurrentDungeonLevelId;
        }

        return SingleLevelRuntimeStateId;
    }

    FString GetRuntimeWorldTypeText (const UWorld* World)
    {
        if (!World)
        {
            return TEXT ("None");
        }

        switch (World->WorldType)
        {
            case EWorldType::Game:
                return TEXT ("Game");

            case EWorldType::PIE:
                return TEXT ("PIE");

            case EWorldType::Editor:
                return TEXT ("Editor");

            case EWorldType::EditorPreview:
                return TEXT ("EditorPreview");

            case EWorldType::GamePreview:
                return TEXT ("GamePreview");

            case EWorldType::Inactive:
                return TEXT ("Inactive");

            default:
                return FString::Printf (TEXT ("Unknown(%d)"), static_cast<int32> (World->WorldType));
        }
    }

    FString GetRuntimeEdgeText (EGridEdge Edge)
    {
        if (const UEnum* EdgeEnum = StaticEnum<EGridEdge> ())
        {
            return EdgeEnum->GetNameStringByValue (static_cast<int64> (Edge));
        }

        return FString::Printf (TEXT ("%d"), static_cast<int32> (Edge));
    }

    FString GetRuntimeBoolText (bool bValue)
    {
        return bValue ? TEXT ("true") : TEXT ("false");
    }

    UGridItemDefinitionAsset* ResolveObjectItemDefinitionAsset (
        const FGridLevelObjectData& ObjectData,
        const UGridObjectArchetypeAsset* Archetype)
    {
        if (ObjectData.ItemDefinitionAsset)
        {
            return ObjectData.ItemDefinitionAsset;
        }
        if (Archetype && Archetype->DefaultBehavior.Item.ItemDefinitionAsset)
        {
            return Archetype->DefaultBehavior.Item.ItemDefinitionAsset;
        }
        return nullptr;
    }

    FName ResolveObjectItemDefinitionId (
        const FGridLevelObjectData& ObjectData,
        const UGridObjectArchetypeAsset* Archetype)
    {
        if (ObjectData.ItemDefinitionAsset && !ObjectData.ItemDefinitionAsset->ItemDefinitionId.IsNone ())
        {
            return ObjectData.ItemDefinitionAsset->ItemDefinitionId;
        }
        if (!ObjectData.ItemDefinitionId.IsNone ())
        {
            return ObjectData.ItemDefinitionId;
        }
        if (Archetype && Archetype->DefaultBehavior.Item.ItemDefinitionAsset &&
            !Archetype->DefaultBehavior.Item.ItemDefinitionAsset->ItemDefinitionId.IsNone ())
        {
            return Archetype->DefaultBehavior.Item.ItemDefinitionAsset->ItemDefinitionId;
        }
        if (Archetype && !Archetype->DefaultBehavior.Item.ItemDefinitionId.IsNone ())
        {
            return Archetype->DefaultBehavior.Item.ItemDefinitionId;
        }
        return ObjectData.ArchetypeId;
    }

    FName ResolvePickupItemDefinitionId (const AGridItemActor* ItemActor, FName FallbackArchetypeId)
    {
        if (!ItemActor)
        {
            return FallbackArchetypeId;
        }
        if (const UGridItemDefinitionAsset* Definition = ItemActor->GetItemDefinitionAsset ())
        {
            if (!Definition->ItemDefinitionId.IsNone ())
            {
                return Definition->ItemDefinitionId;
            }
        }
        if (!ItemActor->GetItemDefinitionId ().IsNone ())
        {
            return ItemActor->GetItemDefinitionId ();
        }
        if (!FallbackArchetypeId.IsNone ())
        {
            return FallbackArchetypeId;
        }
        return ItemActor->GetItemArchetypeId ();
    }

    const FGridLevelObjectData* FindLevelObjectDataById (const UGridLevelAsset* LevelAsset, FGuid ObjectId)
    {
        if (!LevelAsset || !ObjectId.IsValid ())
        {
            return nullptr;
        }

        for (const FGridLevelObjectData& ObjectData : LevelAsset->Objects)
        {
            if (ObjectData.ObjectId == ObjectId)
            {
                return &ObjectData;
            }
        }

        return nullptr;
    }

    int32 CountRuntimeTransitionObjects (const UGridLevelAsset* InLevelAsset)
    {
        if (!InLevelAsset)
        {
            return 0;
        }

        int32 TransitionCount = 0;
        for (const FGridLevelObjectData& ObjectData : InLevelAsset->Objects)
        {
            if (ObjectData.Behavior.Transition.bIsTransition)
            {
                ++TransitionCount;
            }
        }

        return TransitionCount;
    }

    int32 CountRemovedRuntimeObjects (const FGridLevelRuntimeState* RuntimeState)
    {
        if (!RuntimeState)
        {
            return 0;
        }

        int32 RemovedCount = 0;
        for (const TPair<FGuid, FGridRuntimeObjectPresenceState>& Pair : RuntimeState->ObjectPresence)
        {
            if (Pair.Value.bRemovedFromInitialPlacement)
            {
                ++RemovedCount;
            }
        }
        return RemovedCount;
    }

    int32 CountHiddenFloorCells (const UGridLevelAsset* InLevelAsset, const AGridLevelRuntimeActor* RuntimeActor)
    {
        if (!InLevelAsset || !RuntimeActor)
        {
            return 0;
        }

        int32 HiddenFloorCells = 0;
        for (int32 Y = 0; Y < InLevelAsset->Height; ++Y)
        {
            for (int32 X = 0; X < InLevelAsset->Width; ++X)
            {
                if (RuntimeActor->ShouldHideCellFloor (X, Y))
                {
                    ++HiddenFloorCells;
                }
            }
        }

        return HiddenFloorCells;
    }

    void GetWorldMonsters (
        const UWorld* World,
        TArray<AGridMonsterActor*>& OutMonsters)
    {
        OutMonsters.Reset ();
        if (!World)
        {
            return;
        }

        for (TActorIterator<AGridMonsterActor> It (
            const_cast<UWorld*> (World)); It; ++It)
        {
            if (IsValid (*It))
            {
                OutMonsters.Add (*It);
            }
        }

        OutMonsters.Sort ([] (
            const AGridMonsterActor& Left,
            const AGridMonsterActor& Right)
        {
            const FGuid LeftId = Left.ResolvePersistenceId ();
            const FGuid RightId = Right.ResolvePersistenceId ();
            if (LeftId.IsValid () != RightId.IsValid ())
            {
                return LeftId.IsValid ();
            }
            if (LeftId != RightId)
            {
                return LeftId.ToString (EGuidFormats::Digits) <
                    RightId.ToString (EGuidFormats::Digits);
            }
            return Left.GetPathName () < Right.GetPathName ();
        });
    }
}

bool AGridLevelRuntimeActor::IsSafeRuntimeRenderTransform (const FTransform& Transform)
{
    constexpr float MinAbsScale = 0.001f;
    const FVector Scale = Transform.GetScale3D ();

    return Transform.IsValid () &&
        !Transform.GetLocation ().ContainsNaN () &&
        !Scale.ContainsNaN () &&
        Transform.GetRotation ().IsNormalized () &&
        FMath::Abs (Scale.X) >= MinAbsScale &&
        FMath::Abs (Scale.Y) >= MinAbsScale &&
        FMath::Abs (Scale.Z) >= MinAbsScale;
}

void AGridLevelRuntimeActor::LogUnsafeInstanceTransform (const TCHAR* FunctionName, const UInstancedStaticMeshComponent* Component,
    int32 X, int32 Y, EGridEdge Edge, const FTransform& Transform) const
{
    UE_LOG (LogTemp, Error,
        TEXT ("Unsafe runtime render transform skipped: Function=%s Component=%s StaticMesh=%s Cell=(%d,%d) Edge=%d Location=%s Rotation=%s Scale=%s"),
        FunctionName,
        *GetNameSafe (Component),
        *GetNameSafe (Component ? Component->GetStaticMesh () : nullptr),
        X,
        Y,
        static_cast<int32> (Edge),
        *Transform.GetLocation ().ToCompactString (),
        *Transform.GetRotation ().ToString (),
        *Transform.GetScale3D ().ToCompactString ());
}

void AGridLevelRuntimeActor::LogUnsafeObjectTransform (const TCHAR* FunctionName, const FGridLevelObjectData& ObjectData,
    const UStaticMesh* StaticMesh, const FTransform& Transform) const
{
    UE_LOG (LogTemp, Error,
        TEXT ("Unsafe runtime render transform skipped: Function=%s ObjectId=%s ArchetypeId=%s Tag=%s Cell=(%d,%d) Edge=%d StaticMesh=%s Location=%s Rotation=%s Scale=%s"),
        FunctionName,
        *ObjectData.ObjectId.ToString (),
        *ObjectData.ArchetypeId.ToString (),
        *ObjectData.Tag.ToString (),
        ObjectData.CellX,
        ObjectData.CellY,
        static_cast<int32> (ObjectData.Edge),
        *GetNameSafe (StaticMesh),
        *Transform.GetLocation ().ToCompactString (),
        *Transform.GetRotation ().ToString (),
        *Transform.GetScale3D ().ToCompactString ());
}

void AGridLevelRuntimeActor::LogUnsafeItemTransform (const TCHAR* FunctionName, FName ArchetypeId, const AActor* OwnerActor,
    const USceneComponent* AttachParent, const UStaticMesh* StaticMesh, const FTransform& Transform) const
{
    UE_LOG (LogTemp, Error,
        TEXT ("Unsafe runtime item transform skipped: Function=%s ArchetypeId=%s Owner=%s AttachParent=%s StaticMesh=%s Location=%s Rotation=%s Scale=%s"),
        FunctionName,
        *ArchetypeId.ToString (),
        *GetNameSafe (OwnerActor),
        *GetNameSafe (AttachParent),
        *GetNameSafe (StaticMesh),
        *Transform.GetLocation ().ToCompactString (),
        *Transform.GetRotation ().ToString (),
        *Transform.GetScale3D ().ToCompactString ());
}

AGridLevelRuntimeActor::AGridLevelRuntimeActor ()
{
    PrimaryActorTick.bCanEverTick = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent> (TEXT ("Root"));
    SetRootComponent (SceneRoot);

    FloorISM = CreateDefaultSubobject<UInstancedStaticMeshComponent> (TEXT ("FloorISM"));
    FloorISM->SetupAttachment (SceneRoot);

    WallISM = CreateDefaultSubobject<UInstancedStaticMeshComponent> (TEXT ("WallISM"));
    WallISM->SetupAttachment (SceneRoot);

    CeilingISM = CreateDefaultSubobject<UInstancedStaticMeshComponent> (TEXT ("CeilingISM"));
    CeilingISM->SetupAttachment (SceneRoot);

    ActivationComponent = CreateDefaultSubobject<UGridActivationComponent> (TEXT ("ActivationComponent"));

    MonsterEncounterComponent =
        CreateDefaultSubobject<UGridMonsterEncounterComponent> (
            TEXT ("MonsterEncounterComponent"));

    DoorSystemComponent = CreateDefaultSubobject<UGridDoorSystemComponent> (TEXT ("DoorSystemComponent"));

    EditorPreviewComponent = CreateDefaultSubobject<UGridEditorPreviewComponent> (TEXT ("EditorPreviewComponent"));

    PlayerAttackPresentationComponent =
        CreateDefaultSubobject<
            UGridPlayerAttackPresentationComponent> (
                TEXT ("PlayerAttackPresentationComponent"));
}

void AGridLevelRuntimeActor::ShowReadableMessage (const FText& MessageText)
{
    if (MessageText.IsEmpty ())
    {
        return;
    }
    UWorld* World = GetWorld ();
    if (!World || !ReadableMessageWidgetClass)
    {
        UE_LOG (LogTemp, Warning, TEXT ("ShowReadableMessage failed: missing world or widget class."));
        return;
    }
    APlayerController* PlayerController = World->GetFirstPlayerController ();
    if (!PlayerController)
    {
        UE_LOG (LogTemp, Warning, TEXT ("ShowReadableMessage failed: missing player controller."));
        return;
    }
    if (!ActiveReadableMessageWidget)
    {
        ActiveReadableMessageWidget = CreateWidget<UReadableMessageWidget> (PlayerController, ReadableMessageWidgetClass);

        if (!ActiveReadableMessageWidget)
        {
            UE_LOG (LogTemp, Warning, TEXT ("ShowReadableMessage failed: widget creation failed."));
            return;
        }
        ActiveReadableMessageWidget->AddToViewport (50);
    }
    ActiveReadableMessageWidget->SetReadableText (MessageText);
    GetWorldTimerManager ().ClearTimer (ReadableMessageTimerHandle);
    if (bReadableMessageAutoHide)
    {
        GetWorldTimerManager ().SetTimer (
            ReadableMessageTimerHandle,
            this,
            &AGridLevelRuntimeActor::HideReadableMessage,
            ReadableMessageDuration,
            false);
    }
    if (!ActiveReadableMessageWidget->IsInViewport ())
    {
        ActiveReadableMessageWidget->AddToViewport (50);
    }
}

bool AGridLevelRuntimeActor::HasActiveReadableMessage () const
{
    return ActiveReadableMessageWidget &&
        ActiveReadableMessageWidget->IsInViewport ();
}

bool AGridLevelRuntimeActor::DismissReadableMessage ()
{
    if (!HasActiveReadableMessage ())
    {
        return false;
    }

    HideReadableMessage ();
    return true;
}

void AGridLevelRuntimeActor::HideReadableMessage ()
{
    GetWorldTimerManager ().ClearTimer (ReadableMessageTimerHandle);

    if (ActiveReadableMessageWidget)
    {
        ActiveReadableMessageWidget->RemoveFromParent ();
        ActiveReadableMessageWidget = nullptr;
    }
}

void AGridLevelRuntimeActor::ShowInteractionFeedback (const FText& MessageText, float DurationSeconds)
{
    if (MessageText.IsEmpty ())
    {
        return;
    }

    UWorld* World = GetWorld ();
    const TSubclassOf<UReadableMessageWidget> WidgetClass =
        InteractionFeedbackWidgetClass ? InteractionFeedbackWidgetClass : ReadableMessageWidgetClass;
    if (!World || !WidgetClass)
    {
        UE_LOG (LogTemp, Verbose, TEXT ("ShowInteractionFeedback skipped: missing world or widget class."));
        return;
    }

    APlayerController* PlayerController = World->GetFirstPlayerController ();
    if (!PlayerController)
    {
        return;
    }

    if (!ActiveInteractionFeedbackWidget)
    {
        ActiveInteractionFeedbackWidget = CreateWidget<UReadableMessageWidget> (PlayerController, WidgetClass);
        if (!ActiveInteractionFeedbackWidget)
        {
            return;
        }
    }

    ActiveInteractionFeedbackWidget->SetReadableText (MessageText);
    if (!ActiveInteractionFeedbackWidget->IsInViewport ())
    {
        ActiveInteractionFeedbackWidget->AddToViewport (60);
    }

    GetWorldTimerManager ().ClearTimer (InteractionFeedbackTimerHandle);
    GetWorldTimerManager ().SetTimer (
        InteractionFeedbackTimerHandle,
        this,
        &AGridLevelRuntimeActor::HideInteractionFeedback,
        FMath::Max (0.1f, DurationSeconds),
        false);
}

void AGridLevelRuntimeActor::HideInteractionFeedback ()
{
    GetWorldTimerManager ().ClearTimer (InteractionFeedbackTimerHandle);

    if (ActiveInteractionFeedbackWidget)
    {
        ActiveInteractionFeedbackWidget->RemoveFromParent ();
        ActiveInteractionFeedbackWidget = nullptr;
    }
}

void AGridLevelRuntimeActor::OnConstruction (const FTransform& Transform)
{
    Super::OnConstruction (Transform);
    if (!bRebuildInConstruction)
    {
        return;
    }
    UWorld* World = GetWorld ();
    if (!World || World->IsGameWorld ())
    {
        return;
    }
    RebuildLevel (EGridRuntimeRebuildMode::GeometryOnly);
}

void AGridLevelRuntimeActor::BeginPlay ()
{
    Super::BeginPlay ();

    if (GridPIEPlaytestRequest::Matches (this))
    {
        DungeonRuntimeState = FGridDungeonRuntimeState ();
        UE_LOG (LogTemp, Log,
            TEXT ("GridLevelRuntimeActor: fresh PIE dungeon state initialized without modifying save data."));
    }

    if (DungeonAsset)
    {
        if (!CurrentDungeonLevelId.IsNone ())
        {
            if (UGridLevelAsset* ConfiguredDungeonLevel = DungeonAsset->GetLevelAssetById (CurrentDungeonLevelId))
            {
                LevelAsset = ConfiguredDungeonLevel;
            }
            else
            {
                UE_LOG (
                    LogTemp,
                    Warning,
                    TEXT ("GridLevelRuntimeActor: CurrentDungeonLevelId %s is not valid in DungeonAsset %s; keeping configured LevelAsset."),
                    *CurrentDungeonLevelId.ToString (),
                    *DungeonAsset->GetPathName ());
            }
        }

        if (CurrentDungeonLevelId.IsNone ())
        {
            for (const FGridDungeonLevelEntry& Entry : DungeonAsset->Levels)
            {
                if (Entry.bEnabled && Entry.LevelAsset == LevelAsset)
                {
                    CurrentDungeonLevelId = Entry.LevelId;
                    break;
                }
            }

            if (CurrentDungeonLevelId.IsNone ())
            {
                if (DungeonAsset->IsValidLevelId (DungeonAsset->DefaultLevelId))
                {
                    CurrentDungeonLevelId = DungeonAsset->DefaultLevelId;
                    LevelAsset = DungeonAsset->GetLevelAssetById (DungeonAsset->DefaultLevelId);
                    UE_LOG (
                        LogTemp,
                        Log,
                        TEXT ("GridLevelRuntimeActor: using DungeonAsset DefaultLevelId %s at BeginPlay."),
                        *CurrentDungeonLevelId.ToString ());
                }
                else
                {
                    UE_LOG (
                        LogTemp,
                        Warning,
                        TEXT ("GridLevelRuntimeActor: could not resolve CurrentDungeonLevelId from DungeonAsset %s; keeping configured LevelAsset %s."),
                        *DungeonAsset->GetPathName (),
                        LevelAsset ? *LevelAsset->GetPathName () : TEXT ("None"));
                }
            }
        }
    }

    if (ActivationComponent)
    {
        ActivationComponent->Initialize (this);
        ActivationComponent->ResetRuntimeState ();
    }
    if (DoorSystemComponent)
    {
        DoorSystemComponent->Initialize (this);
        DoorSystemComponent->ResetRuntimeState ();
    }
    if (EditorPreviewComponent)
    {
        EditorPreviewComponent->Initialize (this);
    }
    RebuildLevel ();
    ApplyCurrentLevelRuntimeState ();
    if (ActivationComponent)
    {
        ActivationComponent->RefreshAllPressurePlates ();
    }
}

FGridLevelRuntimeState* AGridLevelRuntimeActor::GetOrCreateRuntimeStateForCurrentLevel ()
{
    const FName RuntimeLevelId = ResolveRuntimeStateLevelId (DungeonAsset, CurrentDungeonLevelId);
    FGridLevelRuntimeState& State = DungeonRuntimeState.LevelStates.FindOrAdd (RuntimeLevelId);
    State.LevelId = RuntimeLevelId;
    return &State;
}

const FGridLevelRuntimeState* AGridLevelRuntimeActor::FindRuntimeStateForCurrentLevel () const
{
    const FName RuntimeLevelId = ResolveRuntimeStateLevelId (DungeonAsset, CurrentDungeonLevelId);
    return DungeonRuntimeState.LevelStates.Find (RuntimeLevelId);
}

void AGridLevelRuntimeActor::ShowCombatFeedback (
    const FGridPlayerAttackFeedbackRequest& Feedback)
{
    if (Feedback.PrimaryText.IsEmpty ())
    {
        return;
    }
    UWorld* World = GetWorld ();
    const TSubclassOf<UReadableMessageWidget> WidgetClass =
        CombatFeedbackWidgetClass
            ? CombatFeedbackWidgetClass
            : InteractionFeedbackWidgetClass
                ? InteractionFeedbackWidgetClass
                : ReadableMessageWidgetClass;
    if (!World || !WidgetClass)
    {
        UE_LOG (
            LogTemp,
            Verbose,
            TEXT ("ShowCombatFeedback skipped: missing world or widget class."));
        return;
    }
    APlayerController* PlayerController =
        World->GetFirstPlayerController ();
    if (!PlayerController)
    {
        return;
    }
    if (!ActiveCombatFeedbackWidget)
    {
        ActiveCombatFeedbackWidget =
            CreateWidget<UReadableMessageWidget> (
                PlayerController,
                WidgetClass);
        if (!ActiveCombatFeedbackWidget)
        {
            return;
        }
    }
    const FText DisplayText = Feedback.DetailText.IsEmpty ()
        ? Feedback.PrimaryText
        : FText::Format (
            NSLOCTEXT (
                "GridPlayerAttackPresentation",
                "FeedbackWithDetail",
                "{0}\n{1}"),
            Feedback.PrimaryText,
            Feedback.DetailText);
    ActiveCombatFeedbackWidget->SetReadableText (DisplayText);
    if (!ActiveCombatFeedbackWidget->IsInViewport ())
    {
        ActiveCombatFeedbackWidget->AddToViewport (65);
    }
    GetWorldTimerManager ().ClearTimer (
        CombatFeedbackTimerHandle);
    GetWorldTimerManager ().SetTimer (
        CombatFeedbackTimerHandle,
        this,
        &AGridLevelRuntimeActor::HideCombatFeedback,
        FMath::Clamp (
            Feedback.DurationSeconds,
            0.1f,
            10.0f),
        false);
}

void AGridLevelRuntimeActor::HideCombatFeedback ()
{
    GetWorldTimerManager ().ClearTimer (
        CombatFeedbackTimerHandle);
    if (ActiveCombatFeedbackWidget)
    {
        ActiveCombatFeedbackWidget->RemoveFromParent ();
        ActiveCombatFeedbackWidget = nullptr;
    }
}

void AGridLevelRuntimeActor::EndPlay (
    const EEndPlayReason::Type EndPlayReason)
{
    HideCombatFeedback ();
    Super::EndPlay (EndPlayReason);
}

void AGridLevelRuntimeActor::AbortActiveCombatAndMonsterActions ()
{
    if (UGridTurnManagerComponent* TurnManager =
        FindComponentByClass<UGridTurnManagerComponent> ())
    {
        TurnManager->AbortCombat ();
    }

    TArray<AGridMonsterActor*> Monsters;
    GetWorldMonsters (GetWorld (), Monsters);
    for (AGridMonsterActor* Monster : Monsters)
    {
        if (Monster->CombatComponent)
        {
            Monster->CombatComponent->CancelAttackPresentation ();
        }

        if (UGridMonsterMovementComponent* Movement =
            Monster->FindComponentByClass<UGridMonsterMovementComponent> ())
        {
            Movement->CancelCurrentAction ();
        }
    }

    if (UGridMonsterOccupancySubsystem* Occupancy =
        GetWorld ()
            ? GetWorld ()->GetSubsystem<UGridMonsterOccupancySubsystem> ()
            : nullptr)
    {
        for (AGridMonsterActor* Monster : Monsters)
        {
            Occupancy->CancelReservation (Monster);
        }
    }
}

void AGridLevelRuntimeActor::SetMonsterRuntimeLevelActive (
    AGridMonsterActor* Monster,
    bool bActive)
{
    if (!IsValid (Monster))
    {
        return;
    }

    UGridMonsterMovementComponent* Movement =
        Monster->FindComponentByClass<UGridMonsterMovementComponent> ();
    UGridMonsterBehaviorComponent* Behavior =
        Monster->FindComponentByClass<UGridMonsterBehaviorComponent> ();
    UGridMonsterOccupancySubsystem* Occupancy =
        GetWorld ()
            ? GetWorld ()->GetSubsystem<UGridMonsterOccupancySubsystem> ()
            : nullptr;

    if (!bActive)
    {
        if (Monster->IdleVariationComponent)
        {
            Monster->IdleVariationComponent->
                StopIdleVariations ();
        }
        if (Monster->VFXComponent)
        {
            Monster->VFXComponent->StopAllMonsterVFX ();
        }
        if (Monster->AudioComponent)
        {
            Monster->AudioComponent->StopAllMonsterAudio ();
        }
        if (Monster->CombatComponent)
        {
            Monster->CombatComponent->CancelAttackPresentation ();
            Monster->CombatComponent->Deactivate ();
        }
        if (Movement)
        {
            Movement->CancelCurrentAction ();
            Movement->ReleaseOccupancy ();
            Movement->Deactivate ();
        }
        else if (Occupancy)
        {
            Occupancy->UnregisterMonster (Monster);
        }
        if (Behavior)
        {
            Behavior->Deactivate ();
        }

        Monster->ResetAnimationSignals ();
        Monster->bRuntimeLevelActive = false;
        Monster->SetActorEnableCollision (false);
        if (Monster->CollisionComponent)
        {
            Monster->CollisionComponent->SetCollisionEnabled (
                ECollisionEnabled::NoCollision);
        }
        Monster->SetActorHiddenInGame (true);
        if (Monster->SkeletalMeshComponent)
        {
            Monster->SkeletalMeshComponent->SetVisibility (
                false,
                true);
        }

        UE_LOG (LogGridMonsterState, Log,
            TEXT ("[GridMonsterState] DeactivateLevel Level=%s Monster=%s PersistenceId=%s"),
            *CurrentDungeonLevelId.ToString (),
            *GetNameSafe (Monster),
            *Monster->ResolvePersistenceId ().ToString ());
        return;
    }

    Monster->bRuntimeLevelActive = true;
    if (Monster->VFXComponent)
    {
        Monster->VFXComponent->InitializeMonsterVFX ();
    }
    Monster->SetActorHiddenInGame (false);
    if (Monster->SkeletalMeshComponent)
    {
        Monster->SkeletalMeshComponent->SetVisibility (true, true);
    }

    const bool bStatsWereInitialized =
        Monster->bCombatStatsInitialized;
    Monster->EnsureInitialCombatState ();
    const bool bInitializedStats =
        !bStatsWereInitialized &&
        Monster->bCombatStatsInitialized;

    if (Monster->IsDead ())
    {
        if (IsValidCell (
                Monster->CurrentCell.X,
                Monster->CurrentCell.Y) &&
            IsWalkableCell (
                Monster->CurrentCell.X,
                Monster->CurrentCell.Y))
        {
            Monster->SetActorLocation (GetCellCenterWorld (
                Monster->CurrentCell.X,
                Monster->CurrentCell.Y));
            Monster->ApplyFacingRotation ();
        }
        else
        {
            UE_LOG (LogGridMonsterState, Error,
                TEXT ("[GridMonsterState] ActivateLevel Level=%s Monster=%s PersistenceId=%s Cell=(%d,%d) Result=InvalidDeadCell"),
                *CurrentDungeonLevelId.ToString (),
                *GetNameSafe (Monster),
                *Monster->ResolvePersistenceId ().ToString (),
                Monster->CurrentCell.X,
                Monster->CurrentCell.Y);
        }

        if (Monster->DeathComponent)
        {
            Monster->DeathComponent->InitializeDeathComponent (this);
            Monster->DeathComponent->RestoreCommittedDeathState (
                Monster->CurrentCell,
                true);
        }
        else
        {
            Monster->SetActorEnableCollision (false);
        }
    }
    else if (Monster->bMonsterEnabled)
    {
        Monster->SetActorEnableCollision (true);
        if (Monster->CollisionComponent)
        {
            Monster->CollisionComponent->SetCollisionEnabled (
                ECollisionEnabled::QueryOnly);
        }

        bool bRegistered = false;
        if (Movement)
        {
            Movement->Activate ();
            bRegistered = Movement->InitializeMovement (this);
        }
        else
        {
            int32 ResolvedX = INDEX_NONE;
            int32 ResolvedY = INDEX_NONE;
            FVector LocalOffset = FVector::ZeroVector;
            const FVector WorldLocation =
                Monster->GetActorLocation ();
            if (TryResolveWorldCellFromImpactPoint (
                    WorldLocation,
                    ResolvedX,
                    ResolvedY,
                    LocalOffset))
            {
                Monster->CurrentCell =
                    FIntPoint (ResolvedX, ResolvedY);
                Monster->SetActorLocation (GetCellCenterWorld (
                    ResolvedX,
                    ResolvedY));
                Monster->ApplyFacingRotation ();
                bRegistered = Occupancy &&
                    Occupancy->RegisterMonster (
                        Monster,
                        Monster->CurrentCell);
            }
            else
            {
                UE_LOG (LogGridMonsterState, Error,
                    TEXT ("[GridMonsterState] ActivateLevel Level=%s Monster=%s PersistenceId=%s WorldLocation=%s Result=CellInferenceFailed"),
                    *CurrentDungeonLevelId.ToString (),
                    *GetNameSafe (Monster),
                    *Monster->ResolvePersistenceId ().ToString (),
                    *WorldLocation.ToCompactString ());
            }
        }

        if (!bRegistered)
        {
            if (Monster->CollisionComponent)
            {
                Monster->CollisionComponent->SetCollisionEnabled (
                    ECollisionEnabled::NoCollision);
            }
            UE_LOG (LogGridMonsterState, Error,
                TEXT ("[GridMonsterState] ActivateLevel Level=%s Monster=%s PersistenceId=%s Cell=(%d,%d) Result=MovementInitializationOrOccupancyFailed"),
                *CurrentDungeonLevelId.ToString (),
                *GetNameSafe (Monster),
                *Monster->ResolvePersistenceId ().ToString (),
                Monster->CurrentCell.X,
                Monster->CurrentCell.Y);
        }

        if (Behavior)
        {
            Behavior->Activate ();
            Behavior->InitializeBehavior (this, nullptr);
        }
        if (Monster->CombatComponent)
        {
            Monster->CombatComponent->Activate ();
            Monster->CombatComponent->InitializeCombat (nullptr);
        }
    }
    else
    {
        Monster->SetActorEnableCollision (false);
        if (Monster->CollisionComponent)
        {
            Monster->CollisionComponent->SetCollisionEnabled (
                ECollisionEnabled::NoCollision);
        }
    }

    UE_LOG (LogGridMonsterState, Log,
        TEXT ("[GridMonsterState] ActivateLevel Level=%s Monster=%s PersistenceId=%s Dead=%s Enabled=%s InitializedStats=%s CombatStatsInitialized=%s DeathCommitted=%s"),
        *CurrentDungeonLevelId.ToString (),
        *GetNameSafe (Monster),
        *Monster->ResolvePersistenceId ().ToString (),
        Monster->IsDead () ? TEXT ("true") : TEXT ("false"),
        Monster->bMonsterEnabled ? TEXT ("true") : TEXT ("false"),
        bInitializedStats ? TEXT ("true") : TEXT ("false"),
        Monster->bCombatStatsInitialized
            ? TEXT ("true")
            : TEXT ("false"),
        Monster->DeathComponent &&
            Monster->DeathComponent->bDeathCommitted
            ? TEXT ("true")
            : TEXT ("false"));
    if (Monster->AudioComponent)
    {
        Monster->AudioComponent->InitializeMonsterAudio ();
        Monster->AudioComponent->RefreshIdleAmbienceScheduling ();
    }
    if (Monster->IdleVariationComponent)
    {
        Monster->IdleVariationComponent->
            InitializeIdleVariations ();
        Monster->IdleVariationComponent->
            RefreshIdleVariationScheduling ();
    }
}

void AGridLevelRuntimeActor::ApplyInitialMonsterStateForCurrentLevel ()
{
    AbortActiveCombatAndMonsterActions ();

    TArray<AGridMonsterActor*> Monsters;
    GetWorldMonsters (GetWorld (), Monsters);
    for (AGridMonsterActor* Monster : Monsters)
    {
        SetMonsterRuntimeLevelActive (Monster, false);
    }

    if (UGridMonsterOccupancySubsystem* Occupancy =
        GetWorld ()
            ? GetWorld ()->GetSubsystem<UGridMonsterOccupancySubsystem> ()
            : nullptr)
    {
        Occupancy->ResetRegistry ();
    }

    const FName RuntimeLevelId =
        ResolveRuntimeStateLevelId (DungeonAsset, CurrentDungeonLevelId);
    for (AGridMonsterActor* Monster : Monsters)
    {
        if (Monster->ResolveRuntimeDungeonLevelId (RuntimeLevelId) ==
            RuntimeLevelId)
        {
            Monster->EnsureInitialCombatState ();
            SetMonsterRuntimeLevelActive (Monster, true);
        }
    }
}

bool AGridLevelRuntimeActor::CaptureCurrentLevelRuntimeState ()
{
    if (!LevelAsset)
    {
        return false;
    }

    FGridLevelRuntimeState* State = GetOrCreateRuntimeStateForCurrentLevel ();
    if (!State)
    {
        return false;
    }

    State->Doors.Reset ();
    State->InteractiveObjects.Reset ();
    State->ObjectPresence.Reset ();
    State->Items.Reset ();
    State->Receptacles.Reset ();
    State->Monsters.Reset ();
    State->bHasBeenVisited = true;

    for (const FGridLevelObjectData& ObjectData : LevelAsset->Objects)
    {
        if (!ObjectData.ObjectId.IsValid ())
        {
            continue;
        }

        if (ObjectData.Type == EGridLevelObjectType::Door && DoorSystemComponent)
        {
            bool bDoorOpen = false;
            bool bDoorMoving = false;
            bool bDoorBlocked = true;
            if (DoorSystemComponent->GetDoorState (ObjectData.ObjectId, bDoorOpen, bDoorMoving, bDoorBlocked))
            {
                FGridRuntimeDoorState DoorState;
                DoorState.ObjectId = ObjectData.ObjectId;
                DoorState.bIsOpen = bDoorOpen;
                DoorState.bBlocksMovement = bDoorBlocked;
                State->Doors.Add (DoorState.ObjectId, DoorState);
            }
        }
    }

    TSet<FGuid> ActiveObjectIds;
    if (ActivationComponent)
    {
        ActiveObjectIds = ActivationComponent->GetActiveObjectIds ();
        for (const FGridLevelObjectData& ObjectData : LevelAsset->Objects)
        {
            if (!ObjectData.ObjectId.IsValid ())
            {
                continue;
            }

            const bool bIsInteractiveObject =
                ObjectData.Type == EGridLevelObjectType::Button ||
                ObjectData.Type == EGridLevelObjectType::Lever ||
                ObjectData.Type == EGridLevelObjectType::PressurePlate ||
                ObjectData.Type == EGridLevelObjectType::Receptacle ||
                ObjectData.Type == EGridLevelObjectType::Trigger;
            if (!bIsInteractiveObject)
            {
                continue;
            }

            const bool bIsActive = ActiveObjectIds.Contains (ObjectData.ObjectId);
            FGridRuntimeInteractiveState InteractiveState;
            InteractiveState.ObjectId = ObjectData.ObjectId;
            InteractiveState.bIsActivated = bIsActive;
            InteractiveState.bIsPressed = bIsActive;
            InteractiveState.bIsOn = bIsActive;
            State->InteractiveObjects.Add (ObjectData.ObjectId, InteractiveState);
        }
    }

    TSet<FGuid> ExistingPlacedItemObjectIds;
    for (const FGridSpawnedItemRuntimeEntry& Entry : SpawnedItemEntries)
    {
        AGridItemActor* ItemActor = Entry.ItemActor.Get ();
        if (!IsValid (ItemActor))
        {
            continue;
        }
        ExistingPlacedItemObjectIds.Add (Entry.ObjectId);
        FGridRuntimeItemState ItemState;
        ItemState.ObjectId = Entry.ObjectId;
        ItemState.ArchetypeId = Entry.ItemArchetypeId;
        ItemState.ItemDefinitionId = !Entry.ItemDefinitionId.IsNone ()
            ? Entry.ItemDefinitionId
            : ResolvePickupItemDefinitionId (ItemActor, Entry.ItemArchetypeId);
        ItemState.Quantity = FMath::Max (1, Entry.Quantity);
        ItemState.CellX = Entry.Cell.X;
        ItemState.CellY = Entry.Cell.Y;
        ItemState.Edge = Entry.Edge;

        if (ItemState.ItemDefinitionId.IsNone ())
        {
            UE_LOG (LogTemp, Warning,
                TEXT ("GridRuntimeState Capture skipped item: ObjectId=%s Actor=%s no ItemDefinitionId or legacy ArchetypeId resolved."),
                *Entry.ObjectId.ToString (),
                *GetNameSafe (ItemActor));
            continue;
        }
        ItemState.Transform = ItemActor->GetActorTransform ();
        ItemState.bIsSimulatingPhysics = ItemActor->MeshComponent ? ItemActor->MeshComponent->IsSimulatingPhysics () : false;
        ItemState.bIsContainedInReceptacle = false;
        ItemState.bLightsEnabled = ItemActor->AreItemLightsEnabled ();
        ItemState.ReadableContentAsset = ItemActor->ReadableContentAsset;
        ItemState.ReadableContentId = ItemActor->ReadableContentId;
        ItemState.ReadTitleOverride = ItemActor->ReadTitleOverride;
        ItemState.ReadTextOverride = ItemActor->ReadTextOverride;
        State->Items.Add (Entry.ObjectId, ItemState);
    }
    for (const FGridLevelObjectData& ObjectData : LevelAsset->Objects)
    {
        if (ObjectData.Type != EGridLevelObjectType::Item || !ObjectData.ObjectId.IsValid ())
        {
            continue;
        }

        FGridRuntimeObjectPresenceState PresenceState;
        PresenceState.ObjectId = ObjectData.ObjectId;
        PresenceState.bRemovedFromInitialPlacement = !ExistingPlacedItemObjectIds.Contains (ObjectData.ObjectId);
        State->ObjectPresence.Add (ObjectData.ObjectId, PresenceState);
    }

    for (const TPair<FGuid, TObjectPtr<AGridRuntimeObjectActor>>& Pair : SpawnedRuntimeObjectActors)
    {
        AGridReceptacleActor* ReceptacleActor = Cast<AGridReceptacleActor> (Pair.Value.Get ());
        if (!IsValid (ReceptacleActor))
        {
            continue;
        }
        FGridRuntimeReceptacleState ReceptacleState;
        ReceptacleActor->CaptureRuntimeReceptacleState (ReceptacleState);
        State->Receptacles.Add (Pair.Key, ReceptacleState);
    }

    TArray<AGridMonsterActor*> Monsters;
    GetWorldMonsters (GetWorld (), Monsters);
    TMap<FGuid, int32> PersistenceIdCounts;
    TSet<FGuid> CapturedMonsterSpawnIds;
    for (AGridMonsterActor* Monster : Monsters)
    {
        if (Monster->ResolveRuntimeDungeonLevelId (State->LevelId) !=
            State->LevelId)
        {
            continue;
        }

        const FGuid PersistenceId =
            Monster->ResolvePersistenceId ();
        if (!PersistenceId.IsValid ())
        {
            UE_LOG (LogGridMonsterState, Error,
                TEXT ("[GridMonsterState] Capture skipped Level=%s Monster=%s Reason=InvalidPersistenceId"),
                *State->LevelId.ToString (),
                *GetNameSafe (Monster));
            continue;
        }
        ++PersistenceIdCounts.FindOrAdd (PersistenceId);
    }

    for (AGridMonsterActor* Monster : Monsters)
    {
        if (Monster->ResolveRuntimeDungeonLevelId (State->LevelId) !=
            State->LevelId)
        {
            continue;
        }

        const FGuid PersistenceId =
            Monster->ResolvePersistenceId ();
        const int32 DuplicateCount =
            PersistenceIdCounts.FindRef (PersistenceId);
        if (DuplicateCount > 1)
        {
            UE_LOG (LogGridMonsterState, Error,
                TEXT ("[GridMonsterState] DuplicatePersistenceId Level=%s Monster=%s PersistenceId=%s Count=%d"),
                *State->LevelId.ToString (),
                *GetNameSafe (Monster),
                *PersistenceId.ToString (),
                DuplicateCount);
            continue;
        }

        FGridRuntimeMonsterState MonsterState;
        if (Monster->CaptureRuntimeMonsterState (
            MonsterState,
            State->LevelId))
        {
            State->Monsters.Add (
                MonsterState.PersistenceId,
                MonsterState);
            if (MonsterState.SpawnObjectId.IsValid () &&
                LevelAsset->FindMonsterSpawnById (
                    MonsterState.SpawnObjectId))
            {
                FGridRuntimeMonsterPlacementState& PlacementState =
                    State->MonsterPlacements.FindOrAdd (
                        MonsterState.SpawnObjectId);
                PlacementState.SpawnId =
                    MonsterState.SpawnObjectId;
                PlacementState.bIsSpawned = true;
                PlacementState.bHasMonsterState = true;
                PlacementState.MonsterState = MonsterState;
                CapturedMonsterSpawnIds.Add (
                    MonsterState.SpawnObjectId);
            }
        }
    }

    for (const FGridLevelObjectData& ObjectData : LevelAsset->Objects)
    {
        if (ObjectData.Type != EGridLevelObjectType::MonsterSpawn ||
            !ObjectData.ObjectId.IsValid () ||
            CapturedMonsterSpawnIds.Contains (ObjectData.ObjectId))
        {
            continue;
        }

        FGridRuntimeMonsterPlacementState* PlacementState =
            State->MonsterPlacements.Find (
                ObjectData.ObjectId);
        if (!PlacementState)
        {
            FGridRuntimeMonsterPlacementState InitialPlacementState;
            InitialPlacementState.SpawnId = ObjectData.ObjectId;
            InitialPlacementState.bIsSpawned =
                ObjectData.bInitiallyEnabled;
            State->MonsterPlacements.Add (
                ObjectData.ObjectId,
                InitialPlacementState);
        }
    }

    int32 DeadMonsterCount = 0;
    for (const TPair<FGuid, FGridRuntimeMonsterState>& Pair :
        State->Monsters)
    {
        DeadMonsterCount += Pair.Value.bIsDead ? 1 : 0;
    }

    UE_LOG (LogTemp, Log,
        TEXT ("GridRuntimeState Capture Level=%s Doors=%d RemovedObjects=%d Items=%d Receptacles=%d Interactives=%d Monsters=%d MonsterPlacements=%d DeadMonsters=%d"),
        *State->LevelId.ToString (),
        State->Doors.Num (),
        CountRemovedRuntimeObjects (State),
        State->Items.Num (),
        State->Receptacles.Num (),
        State->InteractiveObjects.Num (),
        State->Monsters.Num (),
        State->MonsterPlacements.Num (),
        DeadMonsterCount);

    return true;
}

bool AGridLevelRuntimeActor::ApplyCurrentLevelRuntimeState ()
{
    if (!LevelAsset)
    {
        return false;
    }

    const FGridLevelRuntimeState* State = FindRuntimeStateForCurrentLevel ();
    if (!State || !State->bHasBeenVisited)
    {
        ApplyInitialMonsterStateForCurrentLevel ();
        return false;
    }

    AbortActiveCombatAndMonsterActions ();

    TArray<AGridMonsterActor*> WorldMonsters;
    GetWorldMonsters (GetWorld (), WorldMonsters);
    for (AGridMonsterActor* Monster : WorldMonsters)
    {
        SetMonsterRuntimeLevelActive (Monster, false);
    }

    if (UGridMonsterOccupancySubsystem* Occupancy =
        GetWorld ()
            ? GetWorld ()->GetSubsystem<UGridMonsterOccupancySubsystem> ()
            : nullptr)
    {
        Occupancy->ResetRegistry ();
    }

    if (DoorSystemComponent)
    {
        for (const TPair<FGuid, FGridRuntimeDoorState>& Pair : State->Doors)
        {
            DoorSystemComponent->ApplyDoorState (Pair.Key, Pair.Value.bIsOpen, Pair.Value.bBlocksMovement);
        }
    }

    if (ActivationComponent)
    {
        TSet<FGuid> ActiveObjectIds;
        for (const TPair<FGuid, FGridRuntimeInteractiveState>& Pair : State->InteractiveObjects)
        {
            if (Pair.Value.bIsActivated || Pair.Value.bIsOn || Pair.Value.bIsPressed)
            {
                ActiveObjectIds.Add (Pair.Key);
            }
        }
        ActivationComponent->SetActiveObjectIds (ActiveObjectIds);

        for (const TPair<FGuid, FGridRuntimeInteractiveState>& Pair : State->InteractiveObjects)
        {
            const bool bIsActive = Pair.Value.bIsActivated || Pair.Value.bIsOn || Pair.Value.bIsPressed;
            if (AGridLeverActor* LeverActor = FindRuntimeObjectActor<AGridLeverActor> (Pair.Key))
            {
                LeverActor->SetLeverState (bIsActive);
            }
            if (AGridPressurePlateActor* PlateActor = FindRuntimeObjectActor<AGridPressurePlateActor> (Pair.Key))
            {
                PlateActor->SetPressed (bIsActive);
            }
        }
    }

    auto RemoveSpawnedItemEntry = [this] (int32 EntryIndex)
    {
        if (!SpawnedItemEntries.IsValidIndex (EntryIndex))
        {
            return;
        }

        AGridItemActor* ItemActor = SpawnedItemEntries[EntryIndex].ItemActor.Get ();
        if (IsValid (ItemActor))
        {
            ItemActor->OnRemovedFromWorld ();
            ItemActor->Destroy ();
        }
        SpawnedItemActors.RemoveAllSwap ([ItemActor] (const TObjectPtr<AGridItemActor>& SpawnedItemActor)
        {
            return SpawnedItemActor.Get () == ItemActor;
        });
        SpawnedItemEntries.RemoveAtSwap (EntryIndex);
    };

    for (const TPair<FGuid, FGridRuntimeObjectPresenceState>& Pair : State->ObjectPresence)
    {
        const FGridRuntimeObjectPresenceState& PresenceState = Pair.Value;
        if (!PresenceState.bRemovedFromInitialPlacement)
        {
            continue;
        }

        UE_LOG (LogTemp, Log,
            TEXT ("GridRuntimeState Apply RemovedInitialObject ObjectId=%s"),
            *PresenceState.ObjectId.ToString ());

        for (int32 EntryIndex = SpawnedItemEntries.Num () - 1; EntryIndex >= 0; --EntryIndex)
        {
            if (SpawnedItemEntries[EntryIndex].ObjectId == PresenceState.ObjectId)
            {
                RemoveSpawnedItemEntry (EntryIndex);
            }
        }
    }

    for (const TPair<FGuid, FGridRuntimeItemState>& Pair : State->Items)
    {
        const FGridRuntimeItemState& ItemState = Pair.Value;
        if (ItemState.bIsContainedInReceptacle)
        {
            continue;
        }

        bool bFoundExistingItem = false;
        for (FGridSpawnedItemRuntimeEntry& Entry : SpawnedItemEntries)
        {
            if (Entry.ObjectId != Pair.Key)
            {
                continue;
            }

            if (AGridItemActor* ItemActor = Entry.ItemActor.Get ())
            {
                ItemActor->SetActorTransform (ItemState.Transform, false, nullptr, ETeleportType::TeleportPhysics);
                ItemActor->SetRuntimeObjectId (Pair.Key);
                ItemActor->SetRuntimeCell (ItemState.CellX, ItemState.CellY);
                ItemActor->SetItemLightsEnabled (ItemState.bLightsEnabled);
                ItemActor->InitializeReadableContent (
                    ItemState.ReadableContentAsset,
                    ItemState.ReadableContentId,
                    ItemState.ReadTitleOverride,
                    ItemState.ReadTextOverride);
                Entry.Cell = FIntPoint (ItemState.CellX, ItemState.CellY);
                Entry.Edge = ItemState.Edge;
                Entry.Quantity = FMath::Max (1, ItemState.Quantity);
                bFoundExistingItem = true;
            }
            break;
        }

        const FName RuntimeItemDefinitionId = !ItemState.ItemDefinitionId.IsNone () ? ItemState.ItemDefinitionId : ItemState.ArchetypeId;
        if (!bFoundExistingItem && !RuntimeItemDefinitionId.IsNone ())
        {
            UGridItemDefinitionAsset* ItemDefinition = ResolveRuntimeItemDefinition (RuntimeItemDefinitionId);
            AGridItemActor* ItemActor = SpawnItemActorForDefinition (ItemDefinition, RuntimeItemDefinitionId, this, nullptr);
            if (ItemActor)
            {
                const FGridLevelObjectData* ItemObjectData = FindLevelObjectDataById (LevelAsset, Pair.Key);
                const FIntPoint RuntimeCell = ItemObjectData
                    ? FIntPoint (ItemObjectData->CellX, ItemObjectData->CellY)
                    : FIntPoint (ItemState.CellX, ItemState.CellY);
                const EGridEdge RuntimeEdge = ItemObjectData ? ItemObjectData->Edge : ItemState.Edge;

                ItemActor->SetActorTransform (ItemState.Transform, false, nullptr, ETeleportType::TeleportPhysics);
                ItemActor->SetRuntimeObjectId (Pair.Key);
                ItemActor->SetRuntimeCell (RuntimeCell.X, RuntimeCell.Y);
                ItemActor->ConfigureAsWorldPickup ();
                ItemActor->OnPlacedInWorld ();
                ItemActor->SetItemLightsEnabled (ItemState.bLightsEnabled);
                ItemActor->InitializeReadableContent (
                    ItemState.ReadableContentAsset,
                    ItemState.ReadableContentId,
                    ItemState.ReadTitleOverride,
                    ItemState.ReadTextOverride);
                SpawnedItemActors.Add (ItemActor);

                FGridSpawnedItemRuntimeEntry Entry;
                Entry.Cell = RuntimeCell;
                Entry.Edge = RuntimeEdge;
                Entry.ItemActor = ItemActor;
                Entry.ObjectId = Pair.Key;
                Entry.ItemArchetypeId = !ItemState.ArchetypeId.IsNone () ? ItemState.ArchetypeId : RuntimeItemDefinitionId;
                Entry.ItemDefinitionAsset = ItemDefinition;
                Entry.ItemDefinitionId = RuntimeItemDefinitionId;
                Entry.Quantity = FMath::Max (1, ItemState.Quantity);
                SpawnedItemEntries.Add (Entry);
            }
        }
    }
    for (const TPair<FGuid, FGridRuntimeReceptacleState>& Pair : State->Receptacles)
    {
        AGridReceptacleActor* ReceptacleActor = FindRuntimeObjectActor<AGridReceptacleActor> (Pair.Key);
        if (!ReceptacleActor)
        {
            continue;
        }
        const int32 ClearedItemCount = ReceptacleActor->ForceClearRuntimeContents (false);
        const FGridLevelObjectData* ReceptacleObjectData = FindLevelObjectDataById (LevelAsset, Pair.Key);
        const UGridObjectArchetypeAsset* ReceptacleArchetype = ReceptacleObjectData
            ? FindObjectArchetype (ReceptacleObjectData->ArchetypeId)
            : nullptr;
        const TSubclassOf<AGridItemActor> PreferredItemActorClass =
            ReceptacleActor->ContainedItemActorClass
            ? ReceptacleActor->ContainedItemActorClass
            : (ReceptacleArchetype ? ReceptacleArchetype->ItemActorClass : nullptr);
        for (const FGridRuntimeItemState& ItemState : Pair.Value.ContainedItems)
        {
            const FName RuntimeItemDefinitionId = !ItemState.ItemDefinitionId.IsNone ()
                ? ItemState.ItemDefinitionId
                : ItemState.ArchetypeId;
            if (RuntimeItemDefinitionId.IsNone ())
            {
                UE_LOG (LogTemp, Warning,
                    TEXT ("GridRuntimeState Apply skipped receptacle item: ReceptacleId=%s RuntimeId=%s no ItemDefinitionId or legacy ArchetypeId resolved."),
                    *Pair.Key.ToString (),
                    *ItemState.ObjectId.ToString ());
                continue;
            }
            UGridItemDefinitionAsset* ItemDefinition = ResolveRuntimeItemDefinition (RuntimeItemDefinitionId);
            AGridItemActor* ItemActor = SpawnItemActorForDefinition (ItemDefinition, RuntimeItemDefinitionId, ReceptacleActor, ReceptacleActor->ItemAttachPoint.Get (), PreferredItemActorClass);
            if (ItemActor)
            {
                ItemActor->SetRuntimeObjectId (ItemState.ObjectId);

                if (ItemDefinition)
                {
                    ItemActor->InitializeFromItemDefinition (ItemDefinition, ItemState.ObjectId);
                } else
                {
                    ItemActor->InitializeFromItemDefinitionId (RuntimeItemDefinitionId, ItemState.ObjectId);
                }
                ItemActor->SetItemLightsEnabled (ItemState.bLightsEnabled);
            }
            FGridRuntimeItemState ResolvedItemState = ItemState;
            ResolvedItemState.ItemDefinitionId = RuntimeItemDefinitionId;
            if (ResolvedItemState.ArchetypeId.IsNone ())
            {
                ResolvedItemState.ArchetypeId = RuntimeItemDefinitionId;
            }
            ReceptacleActor->RestoreRuntimeContainedItem (ResolvedItemState, ItemActor);
        }
        UE_LOG (LogTemp, Verbose,
            TEXT ("GridRuntimeState Apply Receptacle Final ObjectId=%s HasItem=%s Count=%d"),
            *Pair.Key.ToString (),
            ReceptacleActor->HasItem () ? TEXT ("true") : TEXT ("false"),
            ReceptacleActor->GetContainedItemCount ());
    }

    TMap<FGuid, TArray<AGridMonsterActor*>> MonstersByPersistenceId;
    TArray<AGridMonsterActor*> CurrentLevelMonsters;
    for (AGridMonsterActor* Monster : WorldMonsters)
    {
        if (Monster->ResolveRuntimeDungeonLevelId (State->LevelId) !=
            State->LevelId)
        {
            continue;
        }

        CurrentLevelMonsters.Add (Monster);
        const FGuid PersistenceId =
            Monster->ResolvePersistenceId ();
        if (PersistenceId.IsValid ())
        {
            MonstersByPersistenceId.FindOrAdd (
                PersistenceId).Add (Monster);
        }
        else
        {
            UE_LOG (LogGridMonsterState, Error,
                TEXT ("[GridMonsterState] ActivateLevel skipped Level=%s Monster=%s Reason=InvalidPersistenceId"),
                *State->LevelId.ToString (),
                *GetNameSafe (Monster));
        }
    }

    for (const TPair<FGuid, TArray<AGridMonsterActor*>>& Pair :
        MonstersByPersistenceId)
    {
        if (Pair.Value.Num () > 1)
        {
            UE_LOG (LogGridMonsterState, Error,
                TEXT ("[GridMonsterState] DuplicatePersistenceId Level=%s PersistenceId=%s Count=%d"),
                *State->LevelId.ToString (),
                *Pair.Key.ToString (),
                Pair.Value.Num ());
        }
    }

    TArray<FGuid> SavedMonsterIds;
    State->Monsters.GetKeys (SavedMonsterIds);
    SavedMonsterIds.Sort ([] (const FGuid& Left, const FGuid& Right)
    {
        return Left.ToString (EGuidFormats::Digits) <
            Right.ToString (EGuidFormats::Digits);
    });

    TSet<FGuid> AppliedMonsterIds;
    for (const FGuid& SavedMonsterId : SavedMonsterIds)
    {
        const FGridRuntimeMonsterState* SavedMonsterState =
            State->Monsters.Find (SavedMonsterId);
        if (!SavedMonsterState)
        {
            continue;
        }

        const TArray<AGridMonsterActor*>* MatchingActors =
            MonstersByPersistenceId.Find (SavedMonsterId);
        if (!MatchingActors || MatchingActors->IsEmpty ())
        {
            UE_LOG (LogGridMonsterState, Warning,
                TEXT ("[GridMonsterState] MissingActor Level=%s PersistenceId=%s Definition=%s Cell=(%d,%d)"),
                *State->LevelId.ToString (),
                *SavedMonsterId.ToString (),
                *SavedMonsterState->MonsterDefinitionId.ToString (),
                SavedMonsterState->CellX,
                SavedMonsterState->CellY);
            continue;
        }
        if (MatchingActors->Num () != 1)
        {
            continue;
        }

        AGridMonsterActor* Monster = (*MatchingActors)[0];
        Monster->RestoreRuntimeMonsterState (
            *SavedMonsterState,
            this);
        AppliedMonsterIds.Add (SavedMonsterId);
    }

    for (AGridMonsterActor* Monster : CurrentLevelMonsters)
    {
        const FGuid PersistenceId =
            Monster->ResolvePersistenceId ();
        const TArray<AGridMonsterActor*>* MatchingActors =
            MonstersByPersistenceId.Find (PersistenceId);
        if (!PersistenceId.IsValid () ||
            (MatchingActors && MatchingActors->Num () > 1) ||
            AppliedMonsterIds.Contains (PersistenceId))
        {
            continue;
        }

        // Version 1 and partial legacy states intentionally preserve the
        // actor's initial runtime values.
        Monster->EnsureInitialCombatState ();
        SetMonsterRuntimeLevelActive (Monster, true);
    }

    for (AGridMonsterActor* Monster : CurrentLevelMonsters)
    {
        if (!Monster->IsRuntimeLevelActive () ||
            Monster->IsDead () ||
            !Monster->bMonsterEnabled)
        {
            continue;
        }

        if (UGridMonsterBehaviorComponent* Behavior =
            Monster->FindComponentByClass<UGridMonsterBehaviorComponent> ())
        {
            Behavior->RefreshPerception ();
        }
    }

    if (ActivationComponent)
    {
        ActivationComponent->RefreshAllPressurePlates ();
    }

    int32 DeadMonsterCount = 0;
    for (const TPair<FGuid, FGridRuntimeMonsterState>& Pair :
        State->Monsters)
    {
        DeadMonsterCount += Pair.Value.bIsDead ? 1 : 0;
    }

    UE_LOG (LogTemp, Log,
        TEXT ("GridRuntimeState Apply Level=%s Doors=%d RemovedObjects=%d Items=%d Receptacles=%d Interactives=%d Monsters=%d DeadMonsters=%d"),
        *State->LevelId.ToString (),
        State->Doors.Num (),
        CountRemovedRuntimeObjects (State),
        State->Items.Num (),
        State->Receptacles.Num (),
        State->InteractiveObjects.Num (),
        State->Monsters.Num (),
        DeadMonsterCount);

    return true;
}

void AGridLevelRuntimeActor::ClearVisuals (EGridRuntimeRebuildMode RebuildMode)
{
    if (FloorISM) FloorISM->ClearInstances ();
    if (WallISM) WallISM->ClearInstances ();
    if (CeilingISM)
    {
        CeilingISM->ClearInstances ();
        CeilingISM->EmptyOverrideMaterials ();
    }

    const bool bClearObjects = RebuildMode != EGridRuntimeRebuildMode::GeometryOnly;
    if (bClearObjects)
    {
        RuntimeMonsterSpawnFailureCount = 0;
        ClearRuntimeObjectActors ();
        if (EditorPreviewComponent) EditorPreviewComponent->ClearPreviewObjects ();
        if (DoorSystemComponent) DoorSystemComponent->ResetRuntimeState ();
        if (ActivationComponent) ActivationComponent->ResetRuntimeState ();
    }
}

bool AGridLevelRuntimeActor::IsValidCell (int32 X, int32 Y) const
{
    return LevelAsset && LevelAsset->IsValidCoord (X, Y);
}

const FGridLevelCellData& AGridLevelRuntimeActor::GetCell (int32 X, int32 Y) const
{
    check (LevelAsset);
    return LevelAsset->GetCell (X, Y);
}

FVector AGridLevelRuntimeActor::CellToWorld (int32 X, int32 Y, float ZOffset) const
{
    const float Size = LevelAsset ? LevelAsset->CellSize : 200.f;
    return GridOrigin + FVector (X * Size, Y * Size, ZOffset);
}

FVector AGridLevelRuntimeActor::GetCellCenterWorld (int32 X, int32 Y, float ZOffset) const
{
    const float CellSize = LevelAsset ? LevelAsset->CellSize : 200.f;
    return GetActorLocation () + GridOrigin + FVector ((X * CellSize) + (CellSize * 0.5f), (Y * CellSize) + (CellSize * 0.5f), ZOffset);
}

void AGridLevelRuntimeActor::AddFloor (int32 X, int32 Y, float CellSize)
{
    const FVector Base = CellToWorld (X, Y, 0.f);
    const FVector CenterOffset (CellSize * 0.5f, CellSize * 0.5f, 0.f);
    const FTransform T (FRotator::ZeroRotator, Base + CenterOffset, FVector::OneVector);
    if (!IsSafeRuntimeRenderTransform (T))
    {
        LogUnsafeInstanceTransform (TEXT ("AddFloor"), FloorISM, X, Y, EGridEdge::None, T);
        return;
    }
    FloorISM->AddInstance (T);
}

void AGridLevelRuntimeActor::AddCeiling (int32 X, int32 Y, float CellSize)
{
    const FVector Base = CellToWorld (X, Y, 200.f);
    const FVector CenterOffset (CellSize * 0.5f, CellSize * 0.5f, 0.f);
    const FTransform T (FRotator::ZeroRotator, Base + CenterOffset, FVector (1.f, 1.f, 1.f));
    if (!IsSafeRuntimeRenderTransform (T))
    {
        LogUnsafeInstanceTransform (TEXT ("AddCeiling"), CeilingISM, X, Y, EGridEdge::None, T);
        return;
    }
    CeilingISM->AddInstance (T);
}

bool AGridLevelRuntimeActor::ShouldSuppressStandardWallForEdge (int32 X, int32 Y, EGridEdge Edge) const
{
    if (!LevelAsset || Edge == EGridEdge::None)
    {
        return false;
    }

    for (const FGridLevelObjectData& ObjectData : LevelAsset->Objects)
    {
        if (ObjectData.CellX == X &&
            ObjectData.CellY == Y &&
            ObjectData.Edge == Edge)
        {
            const UGridObjectArchetypeAsset* Archetype = FindObjectArchetype (ObjectData.ArchetypeId);
            if (Archetype && Archetype->bReplacesStandardWall)
            {
                return true;
            }
        }
    }

    return false;
}

bool AGridLevelRuntimeActor::ShouldHideCellFloor (int32 CellX, int32 CellY) const
{
    if (!LevelAsset)
    {
        return false;
    }

    for (const FGridLevelObjectData& ObjectData : LevelAsset->Objects)
    {
        if (ObjectData.CellX != CellX || ObjectData.CellY != CellY)
        {
            continue;
        }

        const UGridObjectArchetypeAsset* Archetype = FindObjectArchetype (ObjectData.ArchetypeId);
        if (Archetype && Archetype->bHideCellFloor)
        {
            return true;
        }
    }

    return false;
}

void AGridLevelRuntimeActor::AddEdgeInstance (UInstancedStaticMeshComponent* TargetISM, int32 X, int32 Y, EGridEdge Edge, float CellSize)
{
    if (!TargetISM)
    {
        return;
    }
    const FVector Base = CellToWorld (X, Y, 0.f);
    FVector Pos = Base;
    FRotator Rot = FRotator::ZeroRotator;
    switch (Edge)
    {
        case EGridEdge::North:
        {
            Pos = Base + FVector (CellSize * 0.5f, CellSize, 0.f);
            Rot = FRotator (0.f, 0.f, 0.f);
            break;
        }
        case EGridEdge::South:
        {
            Pos = Base + FVector (CellSize * 0.5f, 0.f, 0.f);
            Rot = FRotator (0.f, 180.f, 0.f);
            break;
        }
        case EGridEdge::East:
        {
            Pos = Base + FVector (CellSize, CellSize * 0.5f, 0.f);
            Rot = FRotator (0.f, -90.f, 0.f);
            break;
        }
        case EGridEdge::West:
        {
            Pos = Base + FVector (0.f, CellSize * 0.5f, 0.f);
            Rot = FRotator (0.f, 90.f, 0.f);
            break;
        }
        default:
        {
            return;
        }
    }
    const FTransform T (Rot, Pos, FVector::OneVector); 
    if (!IsSafeRuntimeRenderTransform (T))
    {
        LogUnsafeInstanceTransform (TEXT ("AddEdgeInstance"), TargetISM, X, Y, Edge, T);
        return;
    }
    TargetISM->AddInstance (T);
}

void AGridLevelRuntimeActor::RebuildLevel (EGridRuntimeRebuildMode RebuildMode)
{
    ClearVisuals (RebuildMode);

    if (!LevelAsset || !FloorISM || !WallISM || !CeilingISM)
    {
        return;
    }

    LevelAsset->EnsureCellCount ();
#if WITH_EDITOR
    for (const UGridObjectArchetypeAsset* Archetype : ObjectArchetypes)
    {
        if (!Archetype)
        {
            continue;
        }

        TArray<FGridArchetypeValidationMessage> ValidationMessages;
        Archetype->ValidateArchetype (ValidationMessages);

        bool bHasWarningOrError = false;
        for (const FGridArchetypeValidationMessage& Message : ValidationMessages)
        {
            if (Message.Severity == EGridArchetypeValidationSeverity::Warning ||
                Message.Severity == EGridArchetypeValidationSeverity::Error)
            {
                bHasWarningOrError = true;
                break;
            }
        }

        if (bHasWarningOrError)
        {
            UE_LOG (LogTemp, Warning, TEXT ("%s"), *Archetype->GetValidationSummary ());
        }
    }
#endif
    if (ActivationComponent)
    {
        ActivationComponent->Initialize (this);
        ActivationComponent->RebuildIndexes ();
    }
    if (MonsterEncounterComponent)
    {
        MonsterEncounterComponent->Initialize (this);
    }

    if (DoorSystemComponent)
    {
        DoorSystemComponent->Initialize (this);
        DoorSystemComponent->RebuildIndexes ();
    }
    if (EditorPreviewComponent)
    {
        EditorPreviewComponent->Initialize (this);
    }
    FloorISM->SetStaticMesh (FloorMesh);
    WallISM->SetStaticMesh (WallMesh);
    CeilingISM->SetStaticMesh (CeilingMesh);

    const bool bIsGameWorld = GetWorld () && GetWorld ()->IsGameWorld ();

    if (!bIsGameWorld && CeilingEditorMaterial && CeilingMesh)
    {
        const int32 MaterialCount = CeilingMesh->GetStaticMaterials ().Num ();

        for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
        {
            CeilingISM->SetMaterial (MaterialIndex, CeilingEditorMaterial);
        }
    }
    const float CellSize = LevelAsset->CellSize;
    for (int32 Y = 0; Y < LevelAsset->Height; ++Y)
    {
        for (int32 X = 0; X < LevelAsset->Width; ++X)
        {
            const FGridLevelCellData& Cell = LevelAsset->GetCell (X, Y);
            if (Cell.CellType == EGridCellType::Empty)
            {
                continue;
            }
            if (!ShouldHideCellFloor (X, Y))
            {
                AddFloor (X, Y, CellSize);
            }

            if (Cell.bHasCeiling)
            {
                AddCeiling (X, Y, CellSize);
            }
            auto DrawEdgeIfNeeded =
                [&] (EGridEdge Edge, EGridWallType WallType, bool bShouldDraw)
            {
                if (!bShouldDraw || ShouldSuppressStandardWallForEdge (X, Y, Edge))
                {
                    return;
                }
                switch (WallType)
                {
                    case EGridWallType::Solid:
                    AddEdgeInstance (WallISM, X, Y, Edge, CellSize);
                    break;

                    default:
                    break;
                }
            };
            // Each stored cell edge is rendered independently. Opposite neighbor edges are not merged.
            DrawEdgeIfNeeded (EGridEdge::North, Cell.NorthWall, Cell.NorthWall != EGridWallType::None);
            DrawEdgeIfNeeded (EGridEdge::East, Cell.EastWall, Cell.EastWall != EGridWallType::None);
            DrawEdgeIfNeeded (EGridEdge::South, Cell.SouthWall, Cell.SouthWall != EGridWallType::None);
            DrawEdgeIfNeeded (EGridEdge::West, Cell.WestWall, Cell.WestWall != EGridWallType::None);
        }
    }
    if (RebuildMode != EGridRuntimeRebuildMode::GeometryOnly)
    {
        if (!GetWorld () || !GetWorld ()->IsGameWorld ())
        {
            if (EditorPreviewComponent)
            {
                EditorPreviewComponent->RebuildPreviewObjects ();
            }
        }
    }    
    if (RebuildMode != EGridRuntimeRebuildMode::GeometryOnly)
    {
        if (bIsGameWorld)
        {
            ++RuntimeObjectRebuildGeneration;
            RebuildRuntimeObjects ();
        }
    }
    if (bEnableRuntimeDebugLog)
    {
        LogRuntimeDebugSummary ();
    }

    if (bEnableRuntimeDebugScreen)
    {
        ShowRuntimeDebugSummary ();
    }
}

bool AGridLevelRuntimeActor::IsWalkableCell (int32 X, int32 Y) const
{
    if (!LevelAsset || !LevelAsset->IsValidCoord (X, Y))
    {
        return false;
    }
    const FGridLevelCellData& Cell = LevelAsset->GetCell (X, Y);
    if (Cell.CellType == EGridCellType::Empty)
    {
        return false;
    }
    if (Cell.bBlocksOccupancy)
    {
        return false;
    }
    return true;
}

bool AGridLevelRuntimeActor::TryGetNeighborCell (int32 X, int32 Y, EGridEdge Direction, int32& OutX, int32& OutY) const
{
    OutX = X;
    OutY = Y;

    if (!LevelAsset)
    {
        return false;
    }

    switch (Direction)
    {
        case EGridEdge::North:
            OutY = Y + 1;
            break;

        case EGridEdge::East:
            OutX = X + 1;
            break;

        case EGridEdge::South:
            OutY = Y - 1;
            break;

        case EGridEdge::West:
            OutX = X - 1;
            break;

        default:
            return false;
    }

    return LevelAsset->IsValidCoord (OutX, OutY);
}

EGridWallType AGridLevelRuntimeActor::GetWallOnEdge (int32 X, int32 Y, EGridEdge Edge) const
{
    if (!LevelAsset || !LevelAsset->IsValidCoord (X, Y))
    {
        return EGridWallType::Solid;
    }

    const FGridLevelCellData& Cell = LevelAsset->GetCell (X, Y);

    // Shared edges are intentionally directional: only the requested cell edge is authoritative.
    // The editor does not mirror this value to the neighboring cell.
    switch (Edge)
    {
        case EGridEdge::North:
            return Cell.NorthWall;

        case EGridEdge::East:
            return Cell.EastWall;

        case EGridEdge::South:
            return Cell.SouthWall;

        case EGridEdge::West:
            return Cell.WestWall;

        default:
            return EGridWallType::Solid;
    }
}

bool AGridLevelRuntimeActor::CanMove (int32 FromX, int32 FromY, EGridEdge Direction) const
{
    if (!IsWalkableCell (FromX, FromY))
    {
        return false;
    }

    int32 ToX = INDEX_NONE;
    int32 ToY = INDEX_NONE;

    if (!TryGetNeighborCell (FromX, FromY, Direction, ToX, ToY))
    {
        return false;
    }

    if (!IsWalkableCell (ToX, ToY))
    {
        return false;
    }
    if (DoorSystemComponent && DoorSystemComponent->IsDoorPassageBlocked (FromX, FromY, Direction))
    {
        return false;
    }
    // Movement follows the same directional wall convention as painting and rendering.
    const EGridWallType Wall = GetWallOnEdge (FromX, FromY, Direction);

    switch (Wall)
    {
        case EGridWallType::None:
            return true;

        case EGridWallType::Solid:
        default:
            return false;
    }
}

void AGridLevelRuntimeActor::GetEdgeTransform (int32 X, int32 Y, EGridEdge Edge, float CellSize, FVector& OutWorldLocation, FRotator& OutWorldRotation) const
{
    const FVector Base = GetActorLocation () + CellToWorld (X, Y, 0.f);

    switch (Edge)
    {
        case EGridEdge::North:
        OutWorldLocation = Base + FVector (CellSize * 0.5f, CellSize, 0.f);
        OutWorldRotation = FRotator (0.f, 0.f, 0.f);
        break;

        case EGridEdge::East:
        OutWorldLocation = Base + FVector (CellSize, CellSize * 0.5f, 0.f);
        OutWorldRotation = FRotator (0.f, -90.f, 0.f);
        break;

        case EGridEdge::South:
        OutWorldLocation = Base + FVector (CellSize * 0.5f, 0.f, 0.f);
        OutWorldRotation = FRotator (0.f, 180.f, 0.f);
        break;

        case EGridEdge::West:
        OutWorldLocation = Base + FVector (0.f, CellSize * 0.5f, 0.f);
        OutWorldRotation = FRotator (0.f, 90.f, 0.f);
        break;

        default:
        OutWorldLocation = Base;
        OutWorldRotation = FRotator::ZeroRotator;
        break;
    }
}

bool AGridLevelRuntimeActor::HasDoorOnEdge (int32 X, int32 Y, EGridEdge Edge) const
{
    int32 ResolvedX = INDEX_NONE;
    int32 ResolvedY = INDEX_NONE;
    EGridEdge ResolvedEdge = EGridEdge::None;
    bool bResolvedOpposite = false;
    return TryResolveDoorEdge (X, Y, Edge, ResolvedX, ResolvedY, ResolvedEdge, bResolvedOpposite);
}

bool AGridLevelRuntimeActor::TryGetOppositeEdge (int32 X, int32 Y, EGridEdge Edge, int32& OutX, int32& OutY, EGridEdge& OutEdge) const
{
    OutX = X;
    OutY = Y;
    OutEdge = EGridEdge::None;

    if (Edge == EGridEdge::None)
    {
        return false;
    }
    if (!TryGetNeighborCell (X, Y, Edge, OutX, OutY))
    {
        return false;
    }
    OutEdge = GridDirectionUtils::GetBackward (Edge);
    return OutEdge != EGridEdge::None;
}

bool AGridLevelRuntimeActor::TryResolveDoorEdge (int32 X, int32 Y, EGridEdge Edge, int32& OutX, int32& OutY, EGridEdge& OutEdge, bool& bOutResolvedOpposite) const
{
    OutX = X;
    OutY = Y;
    OutEdge = Edge;
    bOutResolvedOpposite = false;

    if (!DoorSystemComponent)
    {
        return false;
    }
    if (DoorSystemComponent->HasDoorOnEdge (X, Y, Edge))
    {
        return true;
    }
    if (!TryGetOppositeEdge (X, Y, Edge, OutX, OutY, OutEdge))
    {
        return false;
    }
    bOutResolvedOpposite = DoorSystemComponent->HasDoorOnEdge (OutX, OutY, OutEdge);
    return bOutResolvedOpposite;
}

bool AGridLevelRuntimeActor::IsDoorOpenOnEdge (int32 X, int32 Y, EGridEdge Edge) const
{
    int32 ResolvedX = INDEX_NONE;
    int32 ResolvedY = INDEX_NONE;
    EGridEdge ResolvedEdge = EGridEdge::None;
    bool bResolvedOpposite = false;
    return TryResolveDoorEdge (X, Y, Edge, ResolvedX, ResolvedY, ResolvedEdge, bResolvedOpposite) &&
        DoorSystemComponent->IsDoorOpenOnEdge (ResolvedX, ResolvedY, ResolvedEdge);
}

bool AGridLevelRuntimeActor::ToggleDoorOnEdge (int32 X, int32 Y, EGridEdge Edge)
{
    int32 ResolvedX = INDEX_NONE;
    int32 ResolvedY = INDEX_NONE;
    EGridEdge ResolvedEdge = EGridEdge::None;
    bool bResolvedOpposite = false;
    if (TryResolveDoorEdge (X, Y, Edge, ResolvedX, ResolvedY, ResolvedEdge, bResolvedOpposite) &&
        DoorSystemComponent->ToggleDoorOnEdge (ResolvedX, ResolvedY, ResolvedEdge))
    {
        if (bResolvedOpposite)
        {
            UE_LOG (LogTemp, Log, TEXT ("Grid Use: toggled door on opposite edge (%d,%d,%d)."),
                ResolvedX, ResolvedY, static_cast<int32> (ResolvedEdge));
        }
        return true;
    }
    return false;
}

bool AGridLevelRuntimeActor::OpenDoorOnEdge (int32 X, int32 Y, EGridEdge Edge)
{
    int32 ResolvedX = INDEX_NONE;
    int32 ResolvedY = INDEX_NONE;
    EGridEdge ResolvedEdge = EGridEdge::None;
    bool bResolvedOpposite = false;
    if (TryResolveDoorEdge (X, Y, Edge, ResolvedX, ResolvedY, ResolvedEdge, bResolvedOpposite) &&
        DoorSystemComponent->OpenDoorOnEdge (ResolvedX, ResolvedY, ResolvedEdge))
    {
        if (bResolvedOpposite)
        {
            UE_LOG (LogTemp, Log, TEXT ("Grid Use: opened door on opposite edge (%d,%d,%d)."),
                ResolvedX, ResolvedY, static_cast<int32> (ResolvedEdge));
        }
        return true;
    }
    return false;
}

bool AGridLevelRuntimeActor::CloseDoorOnEdge (int32 X, int32 Y, EGridEdge Edge)
{
    int32 ResolvedX = INDEX_NONE;
    int32 ResolvedY = INDEX_NONE;
    EGridEdge ResolvedEdge = EGridEdge::None;
    bool bResolvedOpposite = false;
    if (TryResolveDoorEdge (X, Y, Edge, ResolvedX, ResolvedY, ResolvedEdge, bResolvedOpposite) &&
        DoorSystemComponent->CloseDoorOnEdge (ResolvedX, ResolvedY, ResolvedEdge))
    {
        if (bResolvedOpposite)
        {
            UE_LOG (LogTemp, Log, TEXT ("Grid Use: closed door on opposite edge (%d,%d,%d)."),
                ResolvedX, ResolvedY, static_cast<int32> (ResolvedEdge));
        }
        return true;
    }
    return false;
}

bool AGridLevelRuntimeActor::TryInteractAtEdge (int32 FromCellX, int32 FromCellY, EGridEdge Edge, AGrimrockPartyPawn* PartyPawn)
{
    if (!ActivationComponent || !CanPartyInteractWithEdgeObject (FromCellX, FromCellY, Edge, PartyPawn))
    {
        return false;
    }
    if (ActivationComponent->TryInteractAtEdge (FromCellX, FromCellY, Edge, PartyPawn))
    {
        return true;
    }
    int32 OppositeX = INDEX_NONE;
    int32 OppositeY = INDEX_NONE;
    EGridEdge OppositeEdge = EGridEdge::None;
    if (TryGetOppositeEdge (FromCellX, FromCellY, Edge, OppositeX, OppositeY, OppositeEdge) &&
        ActivationComponent->TryInteractAtEdge (OppositeX, OppositeY, OppositeEdge, PartyPawn))
    {
        UE_LOG (LogTemp, Log, TEXT ("Grid Use: interacted with object on opposite edge (%d,%d,%d)."),
            OppositeX, OppositeY, static_cast<int32> (OppositeEdge));
        return true;
    }
    return false;
}

bool AGridLevelRuntimeActor::CanPartyInteractWithEdgeObject (
    int32 ObjectCellX,
    int32 ObjectCellY,
    EGridEdge ObjectEdge,
    const AGrimrockPartyPawn* PartyPawn) const
{
    if (!PartyPawn || PartyPawn->LevelRuntimeActor != this || ObjectEdge == EGridEdge::None ||
        PartyPawn->Facing == EGridEdge::None)
    {
        UE_LOG (LogTemp, Verbose,
            TEXT ("Grid edge interaction refused Reason=EdgeNotFacingParty PartyCell=(%d,%d) PartyFacing=%s ObjectCell=(%d,%d) ObjectEdge=%s"),
            PartyPawn ? PartyPawn->CurrentCellX : INDEX_NONE,
            PartyPawn ? PartyPawn->CurrentCellY : INDEX_NONE,
            *GetRuntimeEdgeText (PartyPawn ? PartyPawn->Facing : EGridEdge::None),
            ObjectCellX,
            ObjectCellY,
            *GetRuntimeEdgeText (ObjectEdge));
        return false;
    }

    const FIntPoint PartyCell (PartyPawn->CurrentCellX, PartyPawn->CurrentCellY);
    if (FIntPoint (ObjectCellX, ObjectCellY) == PartyCell && ObjectEdge == PartyPawn->Facing)
    {
        return true;
    }

    int32 FrontCellX = PartyCell.X;
    int32 FrontCellY = PartyCell.Y;
    const bool bIsFrontOppositeEdge =
        TryGetNeighborCell (PartyCell.X, PartyCell.Y, PartyPawn->Facing, FrontCellX, FrontCellY) &&
        ObjectCellX == FrontCellX &&
        ObjectCellY == FrontCellY &&
        ObjectEdge == GridDirectionUtils::GetOpposite (PartyPawn->Facing);
    if (bIsFrontOppositeEdge)
    {
        return true;
    }

    UE_LOG (LogTemp, Verbose,
        TEXT ("Grid edge interaction refused Reason=EdgeNotFacingParty PartyCell=(%d,%d) PartyFacing=%s ObjectCell=(%d,%d) ObjectEdge=%s"),
        PartyCell.X,
        PartyCell.Y,
        *GetRuntimeEdgeText (PartyPawn->Facing),
        ObjectCellX,
        ObjectCellY,
        *GetRuntimeEdgeText (ObjectEdge));
    return false;
}

AGridReceptacleActor* AGridLevelRuntimeActor::FindReceptacleAtEdge (int32 FromCellX, int32 FromCellY, EGridEdge Edge) const
{
    if (!ActivationComponent)
    {
        return nullptr;
    }

    if (AGridReceptacleActor* ReceptacleActor = ActivationComponent->FindReceptacleAtEdge (FromCellX, FromCellY, Edge))
    {
        return ReceptacleActor;
    }

    int32 OppositeX = INDEX_NONE;
    int32 OppositeY = INDEX_NONE;
    EGridEdge OppositeEdge = EGridEdge::None;
    if (TryGetOppositeEdge (FromCellX, FromCellY, Edge, OppositeX, OppositeY, OppositeEdge))
    {
        return ActivationComponent->FindReceptacleAtEdge (OppositeX, OppositeY, OppositeEdge);
    }

    return nullptr;
}

AGridWallLockActor* AGridLevelRuntimeActor::FindWallLockAtEdge (
    int32 FromCellX,
    int32 FromCellY,
    EGridEdge Edge) const
{
    if (!LevelAsset || Edge == EGridEdge::None)
    {
        return nullptr;
    }

    const auto FindAtExactEdge = [this] (int32 CellX, int32 CellY, EGridEdge CandidateEdge)
        -> AGridWallLockActor*
    {
        for (const FGridLevelObjectData& ObjectData : LevelAsset->Objects)
        {
            if (ObjectData.Type == EGridLevelObjectType::Receptacle &&
                ObjectData.CellX == CellX &&
                ObjectData.CellY == CellY &&
                ObjectData.Edge == CandidateEdge)
            {
                if (AGridWallLockActor* WallLockActor =
                    FindRuntimeObjectActor<AGridWallLockActor> (ObjectData.ObjectId))
                {
                    return WallLockActor;
                }
            }
        }
        return nullptr;
    };

    if (AGridWallLockActor* WallLockActor = FindAtExactEdge (FromCellX, FromCellY, Edge))
    {
        return WallLockActor;
    }

    int32 OppositeX = INDEX_NONE;
    int32 OppositeY = INDEX_NONE;
    EGridEdge OppositeEdge = EGridEdge::None;
    return TryGetOppositeEdge (
        FromCellX,
        FromCellY,
        Edge,
        OppositeX,
        OppositeY,
        OppositeEdge)
        ? FindAtExactEdge (OppositeX, OppositeY, OppositeEdge)
        : nullptr;
}

bool AGridLevelRuntimeActor::ExecuteLinksFromRuntimeObject (FGuid SourceObjectId, EGridObjectEvent SourceEvent)
{
    return ActivationComponent ? ActivationComponent->ExecuteLinksFromObjectForEvent (SourceObjectId, SourceEvent) : false;
}

void AGridLevelRuntimeActor::HandlePartyCellChanged (int32 OldCellX, int32 OldCellY, int32 NewCellX, int32 NewCellY)
{
    if (ActivationComponent)
    {
        ActivationComponent->HandlePartyCellChanged (OldCellX, OldCellY, NewCellX, NewCellY);
    }
}

void AGridLevelRuntimeActor::NotifyPawnEnteredCell (int32 CellX, int32 CellY)
{
    if (ActivationComponent)
    {
        ActivationComponent->NotifyPawnEnteredCell (CellX, CellY);
    }
}

void AGridLevelRuntimeActor::NotifyPawnExitedCell (int32 CellX, int32 CellY)
{
    if (ActivationComponent)
    {
        ActivationComponent->NotifyPawnExitedCell (CellX, CellY);
    }
}

bool AGridLevelRuntimeActor::FindTransitionAtCell (
    int32 CellX,
    int32 CellY,
    bool bTriggeredByUseAction,
    FGridObjectTransitionParams& OutTransition) const
{
    if (!LevelAsset)
    {
        UE_LOG (LogTemp, Warning, TEXT ("Dungeon transition lookup failed: LevelAsset is null."));
        return false;
    }

    int32 TransitionCountAtCell = 0;
    bool bFoundUsableTransition = false;

    for (const FGridLevelObjectData& Obj : LevelAsset->Objects)
    {
        const FGridObjectTransitionParams& Transition = Obj.Behavior.Transition;
        if (Obj.CellX != CellX || Obj.CellY != CellY || !Transition.bIsTransition)
        {
            continue;
        }

        ++TransitionCountAtCell;

        if (!bTriggeredByUseAction && Transition.bRequireUseAction)
        {
            UE_LOG (
                LogTemp,
                Log,
                TEXT ("Dungeon transition ignored at Cell=(%d,%d): object %s requires Use action."),
                CellX,
                CellY,
                *Obj.ObjectId.ToString ());
            continue;
        }

        if (!bFoundUsableTransition)
        {
            OutTransition = Transition;
            bFoundUsableTransition = true;
            UE_LOG (
                LogTemp,
                Log,
                TEXT ("Dungeon transition found at Cell=(%d,%d): TargetLevelId=%s TargetCell=(%d,%d) Facing=%s."),
                CellX,
                CellY,
                *Transition.TargetLevelId.ToString (),
                Transition.TargetCellX,
                Transition.TargetCellY,
                *GetRuntimeEdgeText (Transition.TargetFacing));
        }
    }

    if (TransitionCountAtCell > 1)
    {
        UE_LOG (
            LogTemp,
            Warning,
            TEXT ("Dungeon transition: multiple transition objects found at Cell=(%d,%d); using the first valid transition."),
            CellX,
            CellY);
    }

    return bFoundUsableTransition;
}

bool AGridLevelRuntimeActor::TryExecuteTransitionAtCell (
    int32 CellX,
    int32 CellY,
    AGrimrockPartyPawn* PartyPawn,
    bool bTriggeredByUseAction)
{
    FGridObjectTransitionParams Transition;
    if (!FindTransitionAtCell (CellX, CellY, bTriggeredByUseAction, Transition))
    {
        return false;
    }

    return TravelToDungeonLevel (
        Transition.TargetLevelId,
        Transition.TargetCellX,
        Transition.TargetCellY,
        Transition.TargetFacing,
        PartyPawn);
}

bool AGridLevelRuntimeActor::TravelToDungeonLevel (
    FName TargetLevelId,
    int32 TargetCellX,
    int32 TargetCellY,
    EGridEdge TargetFacing,
    AGrimrockPartyPawn* PartyPawn)
{
    if (bIsExecutingDungeonTransition)
    {
        UE_LOG (LogTemp, Warning, TEXT ("Dungeon transition ignored: another transition is already executing."));
        return false;
    }

    struct FScopedDungeonTransitionGuard
    {
        bool& bGuard;

        explicit FScopedDungeonTransitionGuard (bool& InGuard)
            : bGuard (InGuard)
        {
            bGuard = true;
        }

        ~FScopedDungeonTransitionGuard ()
        {
            bGuard = false;
        }
    };

    FScopedDungeonTransitionGuard TransitionGuard (bIsExecutingDungeonTransition);

    if (!DungeonAsset)
    {
        UE_LOG (LogTemp, Error, TEXT ("Dungeon transition failed: DungeonAsset is null."));
        return false;
    }

    if (TargetLevelId.IsNone ())
    {
        UE_LOG (LogTemp, Error, TEXT ("Dungeon transition failed: TargetLevelId is None."));
        return false;
    }

    const FGridDungeonLevelEntry* TargetEntry = DungeonAsset->FindLevelEntry (TargetLevelId);
    if (!TargetEntry)
    {
        UE_LOG (
            LogTemp,
            Error,
            TEXT ("Dungeon transition failed: TargetLevelId %s was not found in DungeonAsset %s."),
            *TargetLevelId.ToString (),
            *DungeonAsset->GetPathName ());
        return false;
    }

    if (!TargetEntry->bEnabled)
    {
        UE_LOG (
            LogTemp,
            Error,
            TEXT ("Dungeon transition failed: TargetLevelId %s is disabled."),
            *TargetLevelId.ToString ());
        return false;
    }

    UGridLevelAsset* TargetLevelAsset = TargetEntry->LevelAsset.Get ();
    if (!TargetLevelAsset)
    {
        UE_LOG (
            LogTemp,
            Error,
            TEXT ("Dungeon transition failed: TargetLevelId %s has no LevelAsset."),
            *TargetLevelId.ToString ());
        return false;
    }

    if (!TargetLevelAsset->IsValidCoord (TargetCellX, TargetCellY))
    {
        UE_LOG (
            LogTemp,
            Error,
            TEXT ("Dungeon transition failed: Target cell (%d,%d) is outside LevelAsset %s."),
            TargetCellX,
            TargetCellY,
            *TargetLevelAsset->GetPathName ());
        return false;
    }

    const FGridLevelCellData& TargetCell = TargetLevelAsset->GetCell (TargetCellX, TargetCellY);
    if (TargetCell.CellType == EGridCellType::Empty || TargetCell.bBlocksOccupancy)
    {
        UE_LOG (
            LogTemp,
            Error,
            TEXT ("Dungeon transition failed: Target cell (%d,%d) is not walkable in LevelAsset %s. CellType=%d BlocksOccupancy=%s."),
            TargetCellX,
            TargetCellY,
            *TargetLevelAsset->GetPathName (),
            static_cast<int32> (TargetCell.CellType),
            *GetRuntimeBoolText (TargetCell.bBlocksOccupancy));
        return false;
    }

    if (TargetFacing == EGridEdge::None)
    {
        UE_LOG (LogTemp, Error, TEXT ("Dungeon transition failed: TargetFacing is None."));
        return false;
    }

    if (!PartyPawn)
    {
        UE_LOG (LogTemp, Error, TEXT ("Dungeon transition failed: PartyPawn is null."));
        return false;
    }

    UE_LOG (
        LogTemp,
        Log,
        TEXT ("Dungeon transition: %s -> %s, Cell=(%d,%d), Facing=%s."),
        *CurrentDungeonLevelId.ToString (),
        *TargetLevelId.ToString (),
        TargetCellX,
        TargetCellY,
        *GetRuntimeEdgeText (TargetFacing));

    AbortActiveCombatAndMonsterActions ();

    const FName OldLevelId = ResolveRuntimeStateLevelId (DungeonAsset, CurrentDungeonLevelId);
    CaptureCurrentLevelRuntimeState ();
    if (const FGridLevelRuntimeState* StoredState = DungeonRuntimeState.LevelStates.Find (OldLevelId))
    {
        UE_LOG (LogTemp, Log,
            TEXT ("GridRuntimeState Stored Level=%s Receptacles=%d Items=%d Doors=%d Monsters=%d"),
            *OldLevelId.ToString (),
            StoredState->Receptacles.Num (),
            StoredState->Items.Num (),
            StoredState->Doors.Num (),
            StoredState->Monsters.Num ());
    }

    TArray<AGridMonsterActor*> Monsters;
    GetWorldMonsters (GetWorld (), Monsters);
    for (AGridMonsterActor* Monster : Monsters)
    {
        if (Monster->ResolveRuntimeDungeonLevelId (OldLevelId) ==
            OldLevelId)
        {
            SetMonsterRuntimeLevelActive (Monster, false);
        }
    }

    CurrentDungeonLevelId = TargetLevelId;
    LevelAsset = TargetLevelAsset;

    RebuildLevel ();
    ApplyCurrentLevelRuntimeState ();
    PartyPawn->SetGridStart (this, TargetCellX, TargetCellY, TargetFacing);
    if (ActivationComponent)
    {
        ActivationComponent->RefreshAllPressurePlates ();
    }

    UE_LOG (
        LogTemp,
        Log,
        TEXT ("Dungeon transition complete: CurrentDungeonLevelId=%s LevelAsset=%s PartyCell=(%d,%d) Facing=%s."),
        *CurrentDungeonLevelId.ToString (),
        LevelAsset ? *LevelAsset->GetPathName () : TEXT ("None"),
        TargetCellX,
        TargetCellY,
        *GetRuntimeEdgeText (TargetFacing));
    return true;
}

void AGridLevelRuntimeActor::SetEditorHoveredObject (FGuid ObjectId)
{
    if (EditorPreviewComponent)
    {
        EditorPreviewComponent->SetHoveredObject (ObjectId);
    }
}

void AGridLevelRuntimeActor::SetEditorSelectedObject (FGuid ObjectId)
{
    if (EditorPreviewComponent)
    {
        EditorPreviewComponent->SetSelectedObject (ObjectId);
    }
}

void AGridLevelRuntimeActor::CleanupOrphanEditorPreviewObjects ()
{
    if (EditorPreviewComponent)
    {
		EditorPreviewComponent->CleanupOrphanPreviewObjects ();
    }
}

const UGridObjectArchetypeAsset* AGridLevelRuntimeActor::FindObjectArchetype (FName ArchetypeId) const
{
    if (ArchetypeId.IsNone ())
    {
        return nullptr;
    }
    for (const UGridObjectArchetypeAsset* Archetype : ObjectArchetypes)
    {
        if (!Archetype)
        {
            continue;
        }
        if (Archetype->ArchetypeId == ArchetypeId)
        {
            return Archetype;
        }
    }
    return nullptr;
}

UGridItemDefinitionAsset* AGridLevelRuntimeActor::ResolveRuntimeItemDefinition (FName ItemDefinitionId) const
{
    if (ItemDefinitionId.IsNone ())
    {
        return nullptr;
    }

    if (LevelAsset)
    {
        for (const FGridLevelObjectData& ObjectData : LevelAsset->Objects)
        {
            if (ObjectData.ItemDefinitionAsset &&
                ObjectData.ItemDefinitionAsset->ItemDefinitionId == ItemDefinitionId)
            {
                return ObjectData.ItemDefinitionAsset;
            }

            const FGridReceptacleBehaviorParams& Receptacle = ObjectData.Behavior.Receptacle;
            for (const FGridReceptacleInitialItemConfig& InitialItem : Receptacle.InitialContent)
            {
                if (InitialItem.ItemDefinition &&
                    InitialItem.ItemDefinition->ItemDefinitionId == ItemDefinitionId)
                {
                    return InitialItem.ItemDefinition;
                }
            }
        }
    }

    for (const UGridObjectArchetypeAsset* Archetype : ObjectArchetypes)
    {
        if (!Archetype)
        {
            continue;
        }

        const auto& ItemParams = Archetype->DefaultBehavior.Item;
        if (ItemParams.ItemDefinitionAsset &&
            ItemParams.ItemDefinitionAsset->ItemDefinitionId == ItemDefinitionId)
        {
            return ItemParams.ItemDefinitionAsset;
        }

        const FGridReceptacleBehaviorParams& ReceptacleParams = Archetype->DefaultBehavior.Receptacle;
        for (const FGridReceptacleInitialItemConfig& InitialItem : ReceptacleParams.InitialContent)
        {
            if (InitialItem.ItemDefinition &&
                InitialItem.ItemDefinition->ItemDefinitionId == ItemDefinitionId)
            {
                return InitialItem.ItemDefinition;
            }
        }
    }

    return nullptr;
}

UStaticMesh* AGridLevelRuntimeActor::GetObjectMesh (const FGridLevelObjectData& ObjectData) const
{
    const UGridObjectArchetypeAsset* Archetype = FindObjectArchetype (ObjectData.ArchetypeId);
    if (!Archetype)
    {
        return nullptr;
    }

    if (Archetype->PreviewMesh)
    {
        return Archetype->PreviewMesh.Get ();
    }

    if (Archetype->MovingMesh)
    {
        return Archetype->MovingMesh.Get ();
    }

    if (Archetype->FixedMesh)
    {
        return Archetype->FixedMesh.Get ();
    }

    return nullptr;
}

UMaterialInterface* AGridLevelRuntimeActor::GetObjectMaterial (const FGridLevelObjectData& ObjectData) const
{
    const UGridObjectArchetypeAsset* Archetype = FindObjectArchetype (ObjectData.ArchetypeId);
    if (!Archetype)
    {
        return nullptr;
    }

    if (Archetype->PreviewMaterial)
    {
        return Archetype->PreviewMaterial.Get ();
    }

    if (Archetype->MovingMaterial)
    {
        return Archetype->MovingMaterial.Get ();
    }

    if (Archetype->FixedMaterial)
    {
        return Archetype->FixedMaterial.Get ();
    }

    return nullptr;
}

AGridItemActor* AGridLevelRuntimeActor::SpawnItemActorForDefinition (UGridItemDefinitionAsset* ItemDefinition, FName ItemDefinitionId,
    AActor* OwnerActor, USceneComponent* AttachParent, TSubclassOf<AGridItemActor> PreferredItemActorClass) const
{
    ItemDefinitionId = ItemDefinition && !ItemDefinition->ItemDefinitionId.IsNone ()
        ? ItemDefinition->ItemDefinitionId : ItemDefinitionId;
    if (!ItemDefinition && ItemDefinitionId.IsNone ())
    {
        UE_LOG (LogTemp, Warning, TEXT ("Grid item spawn failed: missing ItemDefinition and ItemDefinitionId."));
        return nullptr;
    }
    UWorld* World = GetWorld ();
    if (!World)
    {
        return nullptr;
    }
    TSubclassOf<AGridItemActor> ItemClass = PreferredItemActorClass;
    if (!ItemClass)
    {
        ItemClass = AGridItemActor::StaticClass ();
    }
    UStaticMesh* ItemMesh = ItemDefinition ? ItemDefinition->WorldMesh.LoadSynchronous () : nullptr;
    const FTransform SpawnTransform (
        AttachParent ? AttachParent->GetComponentRotation () : FRotator::ZeroRotator,
        AttachParent ? AttachParent->GetComponentLocation () : GetActorLocation (),
        FVector::OneVector);
    if (!IsSafeRuntimeRenderTransform (SpawnTransform))
    {
        UE_LOG (LogTemp, Warning,
            TEXT ("Grid item spawn failed: unsafe transform Item=%s Owner=%s AttachParent=%s."),
            *ItemDefinitionId.ToString (),
            OwnerActor ? *OwnerActor->GetName () : TEXT ("None"),
            AttachParent ? *AttachParent->GetName () : TEXT ("None"));
        return nullptr;
    }
    FActorSpawnParameters Params;
    Params.Owner = OwnerActor ? OwnerActor : const_cast<AGridLevelRuntimeActor*>(this);
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AGridItemActor* ItemActor = World->SpawnActor<AGridItemActor> (ItemClass, SpawnTransform.GetLocation (), SpawnTransform.GetRotation ().Rotator (), Params);

    if (!ItemActor)
    {
        return nullptr;
    }
    // Optional mesh. InitializeItem must not overwrite BP components
    // si ItemMesh est nullptr.
    ItemActor->InitializeItem (ItemDefinitionId, TArray<FName> (), ItemMesh, nullptr);
    if (ItemDefinition)
    {
        ItemActor->InitializeFromItemDefinition (ItemDefinition, FGuid ());
    } else
    {
        ItemActor->InitializeFromItemDefinitionId (ItemDefinitionId, FGuid ());
    }
    if (AttachParent)
    {
        ItemActor->ConfigureAsAttachedItem ();
        ItemActor->AttachToComponent (AttachParent, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
        ItemActor->SetActorRelativeTransform (FTransform::Identity);
    }
    return ItemActor;
}

bool AGridLevelRuntimeActor::GetFloorEdgeObjectTransform (const FGridLevelObjectData& ObjectData, float ZOffset, float EdgeInset,
    FTransform& OutTransform) const
{
    if (!LevelAsset || ObjectData.Edge == EGridEdge::None)
    {
        return false;
    }

    const float CellSize = LevelAsset->CellSize;
    const FVector Base = GetActorLocation () + CellToWorld (ObjectData.CellX, ObjectData.CellY, ZOffset);
    FVector Pos = Base + FVector (CellSize * 0.5f, CellSize * 0.5f, 0.f);
    FRotator Rot = FRotator::ZeroRotator;

    switch (ObjectData.Edge)
    {
        case EGridEdge::North:
            Pos = Base + FVector (CellSize * 0.5f, CellSize - EdgeInset, 0.f);
            Rot = FRotator (0.f, 0.f, 0.f);
            break;

        case EGridEdge::South:
            Pos = Base + FVector (CellSize * 0.5f, EdgeInset, 0.f);
            Rot = FRotator (0.f, 180.f, 0.f);
            break;

        case EGridEdge::East:
            Pos = Base + FVector (CellSize - EdgeInset, CellSize * 0.5f, 0.f);
            Rot = FRotator (0.f, 90.f, 0.f);
            break;

        case EGridEdge::West:
            Pos = Base + FVector (EdgeInset, CellSize * 0.5f, 0.f);
            Rot = FRotator (0.f, -90.f, 0.f);
            break;

        default:
            return false;
    }

    Rot.Yaw += ObjectData.LocalYaw;
    OutTransform = FTransform (Rot, Pos, FVector::OneVector);
    return true;
}

bool AGridLevelRuntimeActor::GetWallMountedObjectTransform (const FGridLevelObjectData& ObjectData, float ZOffset, float WallInset,
    float LocalOffsetAlongWall, float LocalOffsetVertical, FTransform& OutTransform) const
{
    if (!LevelAsset || ObjectData.Edge == EGridEdge::None)
    {
        return false;
    }

    const float CellSize = LevelAsset->CellSize;
    const float FinalZ = ZOffset + LocalOffsetVertical;
    const FVector Base = GetActorLocation () + CellToWorld (ObjectData.CellX, ObjectData.CellY, FinalZ);
    FVector Pos = Base;
    FRotator Rot = FRotator::ZeroRotator;

    switch (ObjectData.Edge)
    {
        case EGridEdge::North:
            Pos = Base + FVector ((CellSize * 0.5f) + LocalOffsetAlongWall, CellSize - WallInset, 0.f);
            Rot = FRotator (0.f, 90.f, 0.f);
            break;

        case EGridEdge::South:
            Pos = Base + FVector ((CellSize * 0.5f) - LocalOffsetAlongWall, WallInset, 0.f);
            Rot = FRotator (0.f, -90.f, 0.f);
            break;

        case EGridEdge::East:
            Pos = Base + FVector (CellSize - WallInset, (CellSize * 0.5f) - LocalOffsetAlongWall, 0.f);
            Rot = FRotator (0.f, 0.f, 0.f);
            break;

        case EGridEdge::West:
            Pos = Base + FVector (WallInset, (CellSize * 0.5f) + LocalOffsetAlongWall, 0.f);
            Rot = FRotator (0.f, 180.f, 0.f);
            break;
    }
    OutTransform = FTransform (Rot, Pos, FVector::OneVector);
    return true;
}

bool AGridLevelRuntimeActor::GetCenteredObjectTransform (const FGridLevelObjectData& ObjectData, float ZOffset, FTransform& OutTransform) const
{
    if (!LevelAsset)
    {
        return false;
    }
    const FVector Pos = GetActorLocation () +
        CellToWorld (ObjectData.CellX, ObjectData.CellY, ZOffset) +
        FVector (LevelAsset->CellSize * 0.5f, LevelAsset->CellSize * 0.5f, 0.f);

    FRotator Rotation = FRotator::ZeroRotator;
    Rotation.Yaw = ObjectData.LocalYaw;

    OutTransform = FTransform (Rotation, Pos, FVector::OneVector);
    return true;
}

bool AGridLevelRuntimeActor::GetObjectPlacementTransform (const FGridLevelObjectData& ObjectData, FTransform& OutTransform) const
{
    if (!LevelAsset)
    {
        return false;
    }
    const UGridObjectArchetypeAsset* Archetype = FindObjectArchetype (ObjectData.ArchetypeId);
    if (!Archetype)
    {
        if (ObjectData.Type == EGridLevelObjectType::Item)
        {
            if (ObjectData.Edge != EGridEdge::None)
            {
                return GetFloorEdgeObjectTransform (ObjectData, 12.f, 18.f, OutTransform);
            }
            return GetCenteredObjectTransform (ObjectData, 12.f, OutTransform);
        }
        return false;
    }
    if (ObjectData.Type == EGridLevelObjectType::Door)
    {
        FVector Pos = FVector::ZeroVector;
        FRotator Rot = FRotator::ZeroRotator;

        GetEdgeTransform (ObjectData.CellX, ObjectData.CellY, ObjectData.Edge, LevelAsset->CellSize, Pos, Rot);

        OutTransform = FTransform (Rot, Pos, FVector::OneVector);
        return true;
    }
    if (ObjectData.Type == EGridLevelObjectType::Item && ObjectData.Edge != EGridEdge::None)
    {
        const float EdgeInset = FMath::Max (Archetype->WallInset, 18.f);
        return GetFloorEdgeObjectTransform (ObjectData, Archetype->PlacementZOffset, EdgeInset, OutTransform);
    }
    if (Archetype->IsEdgePlaced ())
    {
        return GetWallMountedObjectTransform (ObjectData, Archetype->PlacementZOffset, Archetype->WallInset,
            Archetype->LocalOffsetAlongWall, Archetype->LocalOffsetVertical, OutTransform);
    }
    if (Archetype->IsCenterPlaced ())
    {
        return GetCenteredObjectTransform (ObjectData, Archetype->PlacementZOffset, OutTransform);
    }
    return false;
}

void AGridLevelRuntimeActor::RegisterRuntimeObjectActor (const FGuid& ObjectId, AGridRuntimeObjectActor* Actor)
{
    if (!ObjectId.IsValid () || !IsValid (Actor))
    {
        return;
    }
    SpawnedRuntimeObjectActors.Add (ObjectId, Actor);
}

void AGridLevelRuntimeActor::ClearRuntimeObjectActors ()
{
    ClearSpawnedMonsterActors ();

    for (AGridItemActor* ItemActor : SpawnedItemActors)
    {
        if (IsValid (ItemActor))
        {
            ItemActor->OnRemovedFromWorld ();
            ItemActor->Destroy ();
        }
    }
    SpawnedItemActors.Reset ();
    SpawnedItemEntries.Reset ();

    for (TPair<FGuid, TObjectPtr<AGridRuntimeObjectActor>>& Pair : SpawnedRuntimeObjectActors)
    {
        if (IsValid (Pair.Value))
        {
            if (AGridReceptacleActor* ReceptacleActor = Cast<AGridReceptacleActor> (Pair.Value.Get ()))
            {
                ReceptacleActor->ForceClearRuntimeContents (false);
            }
            Pair.Value->Destroy ();
        }
    }
    SpawnedRuntimeObjectActors.Empty ();
}

void AGridLevelRuntimeActor::ClearSpawnedMonsterActors ()
{
    if (SpawnedMonsterActors.IsEmpty ())
    {
        return;
    }

    AbortActiveCombatAndMonsterActions ();
    for (TPair<FGuid, TObjectPtr<AGridMonsterActor>>& Pair :
        SpawnedMonsterActors)
    {
        AGridMonsterActor* Monster = Pair.Value.Get ();
        if (!IsValid (Monster))
        {
            continue;
        }

        SetMonsterRuntimeLevelActive (Monster, false);
        Monster->Destroy ();
    }
    SpawnedMonsterActors.Empty ();
}

bool AGridLevelRuntimeActor::CanPartyPickupItemEntry (
    const FGridSpawnedItemRuntimeEntry& Entry,
    const AGrimrockPartyPawn* PartyPawn,
    bool bLogRejection) const
{
    if (!PartyPawn)
    {
        if (bLogRejection)
        {
            UE_LOG (LogTemp, Warning, TEXT ("Grid item pickup rejected: missing party pawn."));
        }
        return false;
    }

    if (PartyPawn->LevelRuntimeActor != this)
    {
        if (bLogRejection)
        {
            UE_LOG (LogTemp, Warning,
                TEXT ("Grid item pickup rejected: party runtime actor does not match item runtime actor. ItemCell=(%d,%d)."),
                Entry.Cell.X,
                Entry.Cell.Y);
        }
        return false;
    }

    const FIntPoint PartyCell (PartyPawn->CurrentCellX, PartyPawn->CurrentCellY);
    if (Entry.Cell == PartyCell)
    {
        if (Entry.Edge == EGridEdge::None)
        {
            return true;
        }

        const bool bFacesItemEdge =
            PartyPawn->Facing != EGridEdge::None &&
            Entry.Edge == PartyPawn->Facing;
        if (!bFacesItemEdge && bLogRejection)
        {
            UE_LOG (LogTemp, Warning,
                TEXT ("Grid item pickup rejected: item in party cell is not on the edge currently faced by the party. PartyCell=(%d,%d) Facing=%s ItemEdge=%s."),
                PartyCell.X,
                PartyCell.Y,
                *GetRuntimeEdgeText (PartyPawn->Facing),
                *GetRuntimeEdgeText (Entry.Edge));
        }
        return bFacesItemEdge;
    }

    if (PartyPawn->Facing == EGridEdge::None)
    {
        if (bLogRejection)
        {
            UE_LOG (LogTemp, Warning,
                TEXT ("Grid item pickup rejected: party facing is None. PartyCell=(%d,%d) ItemCell=(%d,%d) ItemEdge=%s."),
                PartyCell.X,
                PartyCell.Y,
                Entry.Cell.X,
                Entry.Cell.Y,
                *GetRuntimeEdgeText (Entry.Edge));
        }
        return false;
    }

    int32 FrontCellX = PartyCell.X;
    int32 FrontCellY = PartyCell.Y;
    if (!TryGetNeighborCell (PartyCell.X, PartyCell.Y, PartyPawn->Facing, FrontCellX, FrontCellY) ||
        Entry.Cell != FIntPoint (FrontCellX, FrontCellY))
    {
        if (bLogRejection)
        {
            UE_LOG (LogTemp, Warning,
                TEXT ("Grid item pickup rejected: item is not in the party cell or the cell directly ahead. PartyCell=(%d,%d) Facing=%s ItemCell=(%d,%d) ItemEdge=%s."),
                PartyCell.X,
                PartyCell.Y,
                *GetRuntimeEdgeText (PartyPawn->Facing),
                Entry.Cell.X,
                Entry.Cell.Y,
                *GetRuntimeEdgeText (Entry.Edge));
        }
        return false;
    }

    const EGridEdge RequiredItemEdge = GridDirectionUtils::GetOpposite (PartyPawn->Facing);
    if (Entry.Edge != RequiredItemEdge)
    {
        if (bLogRejection)
        {
            UE_LOG (LogTemp, Warning,
                TEXT ("Grid item pickup rejected: item in front cell is not on the edge facing the party. PartyCell=(%d,%d) Facing=%s ItemCell=(%d,%d) ItemEdge=%s RequiredEdge=%s."),
                PartyCell.X,
                PartyCell.Y,
                *GetRuntimeEdgeText (PartyPawn->Facing),
                Entry.Cell.X,
                Entry.Cell.Y,
                *GetRuntimeEdgeText (Entry.Edge),
                *GetRuntimeEdgeText (RequiredItemEdge));
        }
        return false;
    }

    return true;
}

bool AGridLevelRuntimeActor::CanPartyPickupItemActor (
    const AGridItemActor* ItemActor,
    const AGrimrockPartyPawn* PartyPawn) const
{
    if (!IsValid (ItemActor) || !PartyPawn)
    {
        return false;
    }

    for (const FGridSpawnedItemRuntimeEntry& Entry : SpawnedItemEntries)
    {
        if (Entry.ItemActor.Get () == ItemActor)
        {
            return CanPartyPickupItemEntry (Entry, PartyPawn, false);
        }
    }

    return false;
}

bool AGridLevelRuntimeActor::TryPickupItemAtCell (int32 CellX, int32 CellY, AGrimrockPartyPawn* PartyPawn)
{
    if (!PartyPawn)
    {
        return false;
    }

    const FIntPoint TargetCell (CellX, CellY);
    for (int32 EntryIndex = 0; EntryIndex < SpawnedItemEntries.Num (); ++EntryIndex)
    {
        FGridSpawnedItemRuntimeEntry& Entry = SpawnedItemEntries[EntryIndex];
        if (Entry.Cell != TargetCell)
        {
            continue;
        }

        AGridItemActor* ItemActor = Entry.ItemActor.Get ();
        if (!IsValid (ItemActor))
        {
            const FIntPoint RemovedCell = Entry.Cell;
            SpawnedItemEntries.RemoveAtSwap (EntryIndex);
            SpawnedItemActors.RemoveAllSwap ([] (const TObjectPtr<AGridItemActor>& SpawnedItemActor)
            {
                return !IsValid (SpawnedItemActor.Get ());
            });
            if (ActivationComponent)
            {
                ActivationComponent->RefreshPressurePlatesAtCell (RemovedCell.X, RemovedCell.Y);
            }
            return false;
        }

        if (!CanPartyPickupItemEntry (Entry, PartyPawn))
        {
            continue;
        }

        const FName ItemDefinitionId = ResolvePickupItemDefinitionId (ItemActor, Entry.ItemDefinitionId.IsNone () ? Entry.ItemArchetypeId : Entry.ItemDefinitionId);
        if (ItemDefinitionId.IsNone ())
        {
            UE_LOG (LogTemp, Warning, TEXT ("Item pickup failed at cell %d,%d: missing item definition id."), CellX, CellY);
            return false;
        }

        FGridItemInstance ItemInstance;
        ItemInstance.RuntimeObjectId = ItemActor->GetRuntimeObjectId ();
        if (!ItemInstance.RuntimeObjectId.IsValid ())
        {
            ItemInstance.RuntimeObjectId = FGuid::NewGuid ();
        }
        ItemInstance.ItemDefinitionId = ItemDefinitionId;
        ItemInstance.Quantity = FMath::Max (1, Entry.Quantity);
        ItemInstance.Weight = 0.0f;
        ItemInstance.bLightsEnabled = ItemActor->AreItemLightsEnabled ();
        ItemInstance.ReadableContentAsset = ItemActor->ReadableContentAsset;
        ItemInstance.ReadableContentId = ItemActor->ReadableContentId;
        ItemInstance.ReadTitleOverride = ItemActor->ReadTitleOverride;
        ItemInstance.ReadTextOverride = ItemActor->ReadTextOverride;
        ItemInstance.LastWorldTransform = ItemActor->GetActorTransform ();

        if (!PartyPawn->AddItemInstanceToSelectedCharacterInventory (ItemInstance))
        {
            UE_LOG (LogTemp, Warning, TEXT ("GridInventory Pickup Failed InventoryFull Item=%s RuntimeId=%s"),
                *ItemDefinitionId.ToString (),
                *ItemInstance.RuntimeObjectId.ToString ());
            ShowInteractionFeedback (FText::FromString (TEXT ("Inventaire plein.")));
            return false;
        }

        if (IsValid (ItemActor))
        {
            ItemActor->OnRemovedFromWorld ();
            ItemActor->Destroy ();
        }
        SpawnedItemActors.RemoveAllSwap ([ItemActor] (const TObjectPtr<AGridItemActor>& SpawnedItemActor)
        {
            return SpawnedItemActor.Get () == ItemActor;
        });

        const FIntPoint PickedCell = Entry.Cell;
        SpawnedItemEntries.RemoveAtSwap (EntryIndex);
        if (ActivationComponent)
        {
            ActivationComponent->RefreshPressurePlatesAtCell (PickedCell.X, PickedCell.Y);
        }

        UE_LOG (LogTemp, Log, TEXT ("Picked up item %s from cell %d,%d."), *ItemDefinitionId.ToString (), CellX, CellY);
        return true;
    }

    return false;
}

bool AGridLevelRuntimeActor::TryPickupItemActor (AGridItemActor* ItemActor, AGrimrockPartyPawn* PartyPawn)
{
    if (!IsValid (ItemActor) || !PartyPawn)
    {
        return false;
    }

    for (int32 EntryIndex = 0; EntryIndex < SpawnedItemEntries.Num (); ++EntryIndex)
    {
        FGridSpawnedItemRuntimeEntry& Entry = SpawnedItemEntries[EntryIndex];
        if (Entry.ItemActor.Get () != ItemActor)
        {
            continue;
        }

        if (!CanPartyPickupItemEntry (Entry, PartyPawn))
        {
            return false;
        }

        const FName ItemDefinitionId = ResolvePickupItemDefinitionId (ItemActor, Entry.ItemDefinitionId.IsNone () ? Entry.ItemArchetypeId : Entry.ItemDefinitionId);
        if (ItemDefinitionId.IsNone ())
        {
            UE_LOG (LogTemp, Warning, TEXT ("Item pickup failed for actor %s: missing item definition id."), *ItemActor->GetName ());
            return false;
        }

        FGridItemInstance ItemInstance;
        ItemInstance.RuntimeObjectId = ItemActor->GetRuntimeObjectId ();
        if (!ItemInstance.RuntimeObjectId.IsValid ())
        {
            ItemInstance.RuntimeObjectId = FGuid::NewGuid ();
        }
        ItemInstance.ItemDefinitionId = ItemDefinitionId;
        ItemInstance.Quantity = FMath::Max (1, Entry.Quantity);
        ItemInstance.Weight = 0.0f;
        ItemInstance.bLightsEnabled = ItemActor->AreItemLightsEnabled ();
        ItemInstance.ReadableContentAsset = ItemActor->ReadableContentAsset;
        ItemInstance.ReadableContentId = ItemActor->ReadableContentId;
        ItemInstance.ReadTitleOverride = ItemActor->ReadTitleOverride;
        ItemInstance.ReadTextOverride = ItemActor->ReadTextOverride;
        ItemInstance.LastWorldTransform = ItemActor->GetActorTransform ();

        if (!PartyPawn->AddItemInstanceToSelectedCharacterInventory (ItemInstance))
        {
            UE_LOG (LogTemp, Warning, TEXT ("GridInventory Pickup Failed InventoryFull Item=%s RuntimeId=%s"),
                *ItemDefinitionId.ToString (),
                *ItemInstance.RuntimeObjectId.ToString ());
            ShowInteractionFeedback (FText::FromString (TEXT ("Inventaire plein.")));
            return false;
        }

        ItemActor->OnRemovedFromWorld ();
        ItemActor->Destroy ();

        SpawnedItemActors.RemoveAllSwap ([ItemActor] (const TObjectPtr<AGridItemActor>& SpawnedItemActor)
        {
            return SpawnedItemActor.Get () == ItemActor;
        });

        const FIntPoint PickedCell = Entry.Cell;
        SpawnedItemEntries.RemoveAtSwap (EntryIndex);
        if (ActivationComponent)
        {
            ActivationComponent->RefreshPressurePlatesAtCell (PickedCell.X, PickedCell.Y);
        }

        UE_LOG (LogTemp, Log, TEXT ("Picked up item %s from clicked actor at cell %d,%d."), *ItemDefinitionId.ToString (), PickedCell.X, PickedCell.Y);
        return true;
    }

    return false;
}

bool AGridLevelRuntimeActor::TryDropItemInstanceAtCell (
    const FGridItemInstance& ItemInstance,
    int32 CellX,
    int32 CellY,
    EGridEdge Edge,
    const FVector& LocalOffset)
{
    return TryDropItemInstanceAtCell (
        ItemInstance,
        nullptr,
        CellX,
        CellY,
        Edge,
        LocalOffset);
}

bool AGridLevelRuntimeActor::TryDropItemInstanceAtCell (
    const FGridItemInstance& ItemInstance,
    UGridItemDefinitionAsset* ItemDefinitionAsset,
    int32 CellX,
    int32 CellY,
    EGridEdge Edge,
    const FVector& LocalOffset)
{
    if (!LevelAsset || !ItemInstance.IsValid () || !LevelAsset->IsValidCoord (CellX, CellY))
    {
        return false;
    }

    const FGridLevelCellData& Cell = LevelAsset->GetCell (CellX, CellY);
    if (Cell.CellType == EGridCellType::Empty || Cell.bBlocksOccupancy)
    {
        return false;
    }

    UGridItemDefinitionAsset* ItemDefinition = IsValid (ItemDefinitionAsset)
        ? ItemDefinitionAsset
        : ResolveRuntimeItemDefinition (ItemInstance.ItemDefinitionId);
    if (!ItemDefinition)
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridInventory WorldDrop Failed Item=%s Reason=DefinitionNotResolved"),
            *ItemInstance.ItemDefinitionId.ToString ());
        return false;
    }

    FTransform DropTransform;
    if (Edge == EGridEdge::None)
    {
        DropTransform = FTransform (
            FRotator::ZeroRotator,
            GetCellCenterWorld (CellX, CellY, 12.f) + LocalOffset,
            FVector::OneVector);
    }
    else
    {
        FGridLevelObjectData PlacementData;
        PlacementData.Type = EGridLevelObjectType::Item;
        PlacementData.CellX = CellX;
        PlacementData.CellY = CellY;
        PlacementData.Edge = Edge;
        if (!GetObjectPlacementTransform (PlacementData, DropTransform))
        {
            return false;
        }
        DropTransform.AddToTranslation (LocalOffset);
    }

    if (!IsSafeRuntimeRenderTransform (DropTransform))
    {
        return false;
    }

    AGridItemActor* ItemActor = SpawnItemActorForDefinition (
        ItemDefinition,
        ItemInstance.ItemDefinitionId,
        this,
        nullptr);
    if (!ItemActor)
    {
        return false;
    }

    const FGuid RuntimeObjectId = ItemInstance.RuntimeObjectId.IsValid ()
        ? ItemInstance.RuntimeObjectId
        : FGuid::NewGuid ();
    ItemActor->SetActorTransform (DropTransform);
    ItemActor->InitializeFromItemDefinition (ItemDefinition, RuntimeObjectId);
    ItemActor->SetRuntimeObjectId (RuntimeObjectId);
    ItemActor->SetRuntimeCell (CellX, CellY);
    ItemActor->ConfigureAsWorldPickup ();
    ItemActor->OnPlacedInWorld ();
    ItemActor->SetItemLightsEnabled (ItemInstance.bLightsEnabled);
    ItemActor->InitializeReadableContent (
        ItemInstance.ReadableContentAsset,
        ItemInstance.ReadableContentId,
        ItemInstance.ReadTitleOverride,
        ItemInstance.ReadTextOverride);

    FGridSpawnedItemRuntimeEntry Entry;
    Entry.Cell = FIntPoint (CellX, CellY);
    Entry.Edge = Edge;
    Entry.ItemActor = ItemActor;
    Entry.ObjectId = RuntimeObjectId;
    Entry.ItemArchetypeId = ItemInstance.ItemDefinitionId;
    Entry.ItemDefinitionAsset = ItemDefinition;
    Entry.ItemDefinitionId = ItemInstance.ItemDefinitionId;
    Entry.Quantity = FMath::Max (1, ItemInstance.Quantity);

    SpawnedItemActors.Add (ItemActor);
    SpawnedItemEntries.Add (Entry);
    if (ActivationComponent)
    {
        ActivationComponent->RefreshPressurePlatesAtCell (CellX, CellY);
    }

    UE_LOG (LogTemp, Log,
        TEXT ("GridInventory WorldDrop Item=%s RuntimeId=%s Quantity=%d Cell=(%d,%d) Edge=%d Result=true"),
        *Entry.ItemDefinitionId.ToString (),
        *Entry.ObjectId.ToString (),
        Entry.Quantity,
        CellX,
        CellY,
        static_cast<int32> (Edge));
    return true;
}

bool AGridLevelRuntimeActor::TrySpawnThrownItemProjectile (
    const FGridItemInstance& ItemInstance,
    const FVector& StartWorldLocation,
    const FVector& LaunchVelocity,
    int32 SourceCellX,
    int32 SourceCellY)
{
    UGridItemDefinitionAsset* ItemDefinition = ResolveRuntimeItemDefinition (ItemInstance.ItemDefinitionId);
    return SpawnThrownItemProjectile (
        ItemInstance,
        ItemDefinition,
        StartWorldLocation,
        LaunchVelocity,
        SourceCellX,
        SourceCellY) != nullptr;
}

AGridThrownItemActor*
AGridLevelRuntimeActor::SpawnThrownItemProjectile (
    const FGridItemInstance& ItemInstance,
    UGridItemDefinitionAsset* ItemDefinition,
    const FVector& StartWorldLocation,
    const FVector& LaunchVelocity,
    int32 SourceCellX,
    int32 SourceCellY)
{
    if (!ItemInstance.IsValid () ||
        !ItemDefinition ||
        ItemDefinition->ItemDefinitionId !=
            ItemInstance.ItemDefinitionId ||
        !ItemDefinition->bThrowable ||
        LaunchVelocity.IsNearlyZero () ||
        !IsWalkableCell (SourceCellX, SourceCellY))
    {
        return nullptr;
    }

    const FTransform SpawnTransform (
        LaunchVelocity.Rotation (),
        StartWorldLocation,
        FVector::OneVector);
    if (!IsSafeRuntimeRenderTransform (SpawnTransform))
    {
        return nullptr;
    }

    UWorld* World = GetWorld ();
    if (!World)
    {
        return nullptr;
    }

    FActorSpawnParameters Params;
    Params.Owner = this;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AGridThrownItemActor* ThrownActor = World->SpawnActor<AGridThrownItemActor> (
        AGridThrownItemActor::StaticClass (),
        SpawnTransform,
        Params);
    if (!ThrownActor)
    {
        return nullptr;
    }

    ThrownActor->InitializeThrownItem (
        this,
        ItemInstance,
        ItemDefinition,
        LaunchVelocity,
        SourceCellX,
        SourceCellY);

    UE_LOG (LogTemp, Log,
        TEXT ("GridInventory Throw Spawn Item=%s RuntimeId=%s SourceCell=(%d,%d) Speed=%.2f Result=true"),
        *ItemInstance.ItemDefinitionId.ToString (),
        *ItemInstance.RuntimeObjectId.ToString (),
        SourceCellX,
        SourceCellY,
        LaunchVelocity.Size ());
    return ThrownActor;
}

bool AGridLevelRuntimeActor::TryResolveWorldCellFromImpactPoint (
    const FVector& WorldPoint,
    int32& OutCellX,
    int32& OutCellY,
    FVector& OutLocalOffset) const
{
    OutCellX = INDEX_NONE;
    OutCellY = INDEX_NONE;
    OutLocalOffset = FVector::ZeroVector;
    if (!LevelAsset || LevelAsset->CellSize <= KINDA_SMALL_NUMBER)
    {
        return false;
    }

    const FVector GridLocalPoint = WorldPoint - GetActorLocation () - GridOrigin;
    OutCellX = FMath::FloorToInt (GridLocalPoint.X / LevelAsset->CellSize);
    OutCellY = FMath::FloorToInt (GridLocalPoint.Y / LevelAsset->CellSize);
    if (!IsWalkableCell (OutCellX, OutCellY))
    {
        return false;
    }

    const FVector CellCenter = GetCellCenterWorld (OutCellX, OutCellY, 12.0f);
    const FVector RawOffset = WorldPoint - CellCenter;
    const float MaxOffset = LevelAsset->CellSize * 0.35f;
    OutLocalOffset = FVector (
        FMath::Clamp (RawOffset.X, -MaxOffset, MaxOffset),
        FMath::Clamp (RawOffset.Y, -MaxOffset, MaxOffset),
        0.0f);
    return true;
}

float AGridLevelRuntimeActor::GetWorldItemWeightAtCell (int32 CellX, int32 CellY, bool bIncludeEdgeItems) const
{
    const FIntPoint TargetCell (CellX, CellY);
    float TotalWeight = 0.0f;

    for (const FGridSpawnedItemRuntimeEntry& Entry : SpawnedItemEntries)
    {
        if (Entry.Cell != TargetCell ||
            (!bIncludeEdgeItems && Entry.Edge != EGridEdge::None) ||
            !IsValid (Entry.ItemActor.Get ()))
        {
            continue;
        }

        UGridItemDefinitionAsset* ItemDefinition = Entry.ItemDefinitionAsset.Get ();
        if (!ItemDefinition)
        {
            const FName DefinitionId = !Entry.ItemDefinitionId.IsNone ()
                ? Entry.ItemDefinitionId
                : Entry.ItemArchetypeId;
            ItemDefinition = ResolveRuntimeItemDefinition (DefinitionId);
        }
        if (!ItemDefinition)
        {
            continue;
        }

        const int32 Quantity = FMath::Max (1, Entry.Quantity);
        const float Contribution = ItemDefinition->Weight * Quantity;
        TotalWeight += Contribution;
        UE_LOG (LogTemp, Verbose,
            TEXT ("GridPressurePlate WeightScan Cell=(%d,%d) Item=%s Quantity=%d UnitWeight=%.2f TotalContribution=%.2f"),
            CellX,
            CellY,
            *ItemDefinition->ItemDefinitionId.ToString (),
            Quantity,
            ItemDefinition->Weight,
            Contribution);
    }

    return TotalWeight;
}

bool AGridLevelRuntimeActor::IsPartyOnCell (int32 CellX, int32 CellY) const
{
    UWorld* World = GetWorld ();
    if (!World)
    {
        return false;
    }

    for (TActorIterator<AGrimrockPartyPawn> It (World); It; ++It)
    {
        const AGrimrockPartyPawn* PartyPawn = *It;
        if (PartyPawn &&
            PartyPawn->LevelRuntimeActor == this &&
            PartyPawn->CurrentCellX == CellX &&
            PartyPawn->CurrentCellY == CellY)
        {
            return true;
        }
    }

    return false;
}

void AGridLevelRuntimeActor::ApplyMonsterPlacementMetadata (
    AGridMonsterActor* Monster) const
{
    if (!LevelAsset || !IsValid (Monster) || !Monster->SpawnObjectId.IsValid ())
    {
        return;
    }

    const FGridLevelObjectData* Placement = LevelAsset->Objects.FindByPredicate (
        [Monster] (const FGridLevelObjectData& ObjectData)
        {
            return ObjectData.Type == EGridLevelObjectType::MonsterSpawn &&
                ObjectData.ObjectId == Monster->SpawnObjectId;
        });
    if (Placement)
    {
        Monster->EncounterGroupId = Placement->EncounterGroupId;
        if (Monster->HomeDungeonLevelId.IsNone ())
        {
            Monster->HomeDungeonLevelId =
                ResolveRuntimeStateLevelId (
                    DungeonAsset,
                    CurrentDungeonLevelId);
        }
    }
}

bool AGridLevelRuntimeActor::ResolveMonsterSpawn (
    const FGridLevelObjectData& ObjectData,
    UGridMonsterDefinitionAsset*& OutDefinition,
    TSubclassOf<AGridMonsterActor>& OutActorClass,
    FString& OutError) const
{
    OutDefinition = nullptr;
    OutActorClass = nullptr;
    OutError.Empty ();

    TArray<FString> Errors;
    if (!LevelAsset)
    {
        Errors.Add (TEXT ("LevelAsset is missing."));
    }
    if (ObjectData.Type != EGridLevelObjectType::MonsterSpawn)
    {
        Errors.Add (TEXT ("Object type is not MonsterSpawn."));
    }
    if (!ObjectData.ObjectId.IsValid ())
    {
        Errors.Add (TEXT ("SpawnId/ObjectId is invalid."));
    }
    if (!LevelAsset ||
        !LevelAsset->IsValidCoord (
            ObjectData.CellX,
            ObjectData.CellY))
    {
        Errors.Add (FString::Printf (
            TEXT ("Cell=(%d,%d) is outside the level."),
            ObjectData.CellX,
            ObjectData.CellY));
    }
    else if (!IsWalkableCell (
            ObjectData.CellX,
            ObjectData.CellY))
    {
        Errors.Add (FString::Printf (
            TEXT ("Cell=(%d,%d) does not allow monster occupancy."),
            ObjectData.CellX,
            ObjectData.CellY));
    }
    if (ObjectData.Edge != EGridEdge::None)
    {
        Errors.Add (TEXT ("MonsterSpawn requires Edge=None."));
    }
    if (!IsCardinalMonsterSpawnFacing (
            ObjectData.InitialFacing))
    {
        Errors.Add (TEXT ("InitialFacing is not cardinal."));
    }

    UGridMonsterDefinitionAsset* Definition =
        ObjectData.MonsterDefinitionAsset.Get ();
    if (!Definition)
    {
        Errors.Add (ObjectData.MonsterDefinitionId.IsNone ()
            ? TEXT ("MonsterDefinitionAsset and MonsterDefinitionId are missing.")
            : FString::Printf (
                TEXT ("MonsterDefinitionId '%s' cannot be resolved without MonsterDefinitionAsset in MON13.2."),
                *ObjectData.MonsterDefinitionId.ToString ()));
    }
    else
    {
        if (ObjectData.MonsterDefinitionId.IsNone ())
        {
            Errors.Add (TEXT ("MonsterDefinitionId is missing."));
        }
        else if (Definition->MonsterId !=
            ObjectData.MonsterDefinitionId)
        {
            Errors.Add (FString::Printf (
                TEXT ("MonsterDefinitionId '%s' differs from asset MonsterId '%s'."),
                *ObjectData.MonsterDefinitionId.ToString (),
                *Definition->MonsterId.ToString ()));
        }

        FString DefinitionError;
        if (!Definition->ValidateDefinition (DefinitionError))
        {
            Errors.Add (FString::Printf (
                TEXT ("MonsterDefinition '%s' is invalid: %s"),
                *GetPathNameSafe (Definition),
                *DefinitionError));
        }
        else
        {
            UClass* ActorClass =
                Definition->MonsterActorClass.Get ();
            if (!ActorClass)
            {
                Errors.Add (TEXT ("MonsterActorClass is missing."));
            }
            else if (!ActorClass->IsChildOf (
                    AGridMonsterActor::StaticClass ()))
            {
                Errors.Add (FString::Printf (
                    TEXT ("MonsterActorClass '%s' is not an AGridMonsterActor."),
                    *ActorClass->GetPathName ()));
            }
            else if (ActorClass->HasAnyClassFlags (CLASS_Abstract))
            {
                Errors.Add (FString::Printf (
                    TEXT ("MonsterActorClass '%s' is abstract."),
                    *ActorClass->GetPathName ()));
            }
            else
            {
                OutActorClass =
                    Definition->MonsterActorClass;
            }
        }
    }

    if (!Errors.IsEmpty ())
    {
        OutError = FString::Join (Errors, TEXT (" "));
        return false;
    }

    OutDefinition = Definition;
    return true;
}

bool AGridLevelRuntimeActor::GetMonsterSpawnTransform (
    const FGridLevelObjectData& ObjectData,
    FTransform& OutTransform) const
{
    OutTransform = FTransform::Identity;
    if (!LevelAsset ||
        ObjectData.Type != EGridLevelObjectType::MonsterSpawn ||
        !LevelAsset->IsValidCoord (
            ObjectData.CellX,
            ObjectData.CellY) ||
        !IsCardinalMonsterSpawnFacing (
            ObjectData.InitialFacing))
    {
        return false;
    }

    OutTransform = FTransform (
        FRotator (
            0.0f,
            GridDirectionUtils::ToYaw (
                ObjectData.InitialFacing),
            0.0f),
        GetCellCenterWorld (
            ObjectData.CellX,
            ObjectData.CellY),
        FVector::OneVector);
    return IsSafeRuntimeRenderTransform (OutTransform);
}

AGridMonsterActor*
    AGridLevelRuntimeActor::FindSpawnedMonsterActor (
        const FGuid& SpawnId) const
{
    if (const TObjectPtr<AGridMonsterActor>* Monster =
        SpawnedMonsterActors.Find (SpawnId))
    {
        return IsValid (Monster->Get ())
            ? Monster->Get ()
            : nullptr;
    }
    return nullptr;
}

int32 AGridLevelRuntimeActor::GetSpawnedMonsterActorCount () const
{
    int32 Count = 0;
    for (const TPair<FGuid, TObjectPtr<AGridMonsterActor>>& Pair :
        SpawnedMonsterActors)
    {
        Count += IsValid (Pair.Value.Get ()) ? 1 : 0;
    }
    return Count;
}

bool AGridLevelRuntimeActor::StartMonsterEncounter (
    FGuid AnchorSpawnId)
{
    return MonsterEncounterComponent &&
        MonsterEncounterComponent->StartEncounter (AnchorSpawnId);
}

bool AGridLevelRuntimeActor::IsMonsterEncounterCompleted (
    FName EncounterGroupId) const
{
    return MonsterEncounterComponent &&
        MonsterEncounterComponent->IsEncounterCompleted (
            EncounterGroupId);
}

int32 AGridLevelRuntimeActor::GetMonsterEncounterActiveWave (
    FName EncounterGroupId) const
{
    return MonsterEncounterComponent
        ? MonsterEncounterComponent->GetActiveWaveIndex (
            EncounterGroupId)
        : INDEX_NONE;
}

void AGridLevelRuntimeActor::NotifyMonsterEncounterDeath (
    FGuid SpawnId)
{
    if (MonsterEncounterComponent)
    {
        MonsterEncounterComponent->NotifyMonsterDied (SpawnId);
    }
}

bool AGridLevelRuntimeActor::StoreMonsterPlacementState (
    const FGridLevelObjectData& ObjectData,
    AGridMonsterActor* Monster,
    bool bIsSpawned)
{
    if (!ObjectData.ObjectId.IsValid () ||
        ObjectData.Type != EGridLevelObjectType::MonsterSpawn)
    {
        return false;
    }

    FGridLevelRuntimeState* State =
        GetOrCreateRuntimeStateForCurrentLevel ();
    if (!State)
    {
        return false;
    }

    FGridRuntimeMonsterState CapturedState;
    bool bCapturedMonsterState = false;
    if (IsValid (Monster))
    {
        if (!Monster->CaptureRuntimeMonsterState (
                CapturedState,
                State->LevelId))
        {
            return false;
        }
        bCapturedMonsterState = true;
    }

    FGridRuntimeMonsterPlacementState& PlacementState =
        State->MonsterPlacements.FindOrAdd (
            ObjectData.ObjectId);
    PlacementState.SpawnId = ObjectData.ObjectId;
    PlacementState.bIsSpawned = bIsSpawned;

    if (bCapturedMonsterState)
    {
        PlacementState.bHasMonsterState = true;
        PlacementState.MonsterState = CapturedState;
        if (bIsSpawned)
        {
            State->Monsters.Add (
                CapturedState.PersistenceId,
                CapturedState);
        }
        else
        {
            State->Monsters.Remove (
                CapturedState.PersistenceId);
        }
    }
    else if (!bIsSpawned)
    {
        State->Monsters.Remove (ObjectData.ObjectId);
    }

    State->bHasBeenVisited = true;
    return true;
}

bool AGridLevelRuntimeActor::DespawnMonsterSpawnActor (
    const FGridLevelObjectData& ObjectData,
    bool bRememberState,
    bool bEmitEvent)
{
    AGridMonsterActor* Monster =
        FindSpawnedMonsterActor (ObjectData.ObjectId);
    if (!Monster)
    {
        return !bRememberState ||
            StoreMonsterPlacementState (
                ObjectData,
                nullptr,
                false);
    }

    if (bRememberState &&
        !StoreMonsterPlacementState (
            ObjectData,
            Monster,
            false))
    {
        UE_LOG (LogGridMonsterState, Warning,
            TEXT ("[GridMonsterLifecycle] DespawnRejected SpawnId=%s Reason=StateCaptureFailed"),
            *ObjectData.ObjectId.ToString (
                EGuidFormats::DigitsWithHyphens));
        return false;
    }

    AbortActiveCombatAndMonsterActions ();
    SetMonsterRuntimeLevelActive (Monster, false);
    SpawnedMonsterActors.Remove (ObjectData.ObjectId);
    Monster->Destroy ();

    UE_LOG (LogGridMonsterState, Log,
        TEXT ("[GridMonsterLifecycle] Despawned SpawnId=%s Encounter=%s"),
        *ObjectData.ObjectId.ToString (
            EGuidFormats::DigitsWithHyphens),
        *ObjectData.EncounterGroupId.ToString ());

    if (bEmitEvent)
    {
        ExecuteLinksFromRuntimeObject (
            ObjectData.ObjectId,
            EGridObjectEvent::MonsterDespawned);
    }
    return true;
}

bool AGridLevelRuntimeActor::ExecuteMonsterSpawnCommand (
    FGuid SpawnId,
    EGridObjectCommand Command)
{
    if (!LevelAsset || !SpawnId.IsValid ())
    {
        return false;
    }

    const FGridLevelObjectData* ObjectData =
        LevelAsset->FindMonsterSpawnById (SpawnId);
    if (!ObjectData)
    {
        UE_LOG (LogGridMonsterState, Warning,
            TEXT ("[GridMonsterLifecycle] CommandRejected SpawnId=%s Command=%s Reason=PlacementNotFound"),
            *SpawnId.ToString (EGuidFormats::DigitsWithHyphens),
            *UEnum::GetValueAsString (Command));
        return false;
    }

    const FGridLevelRuntimeState* ExistingState =
        FindRuntimeStateForCurrentLevel ();
    if (!ExistingState || !ExistingState->bHasBeenVisited)
    {
        CaptureCurrentLevelRuntimeState ();
    }

    const bool bIsCurrentlySpawned =
        FindSpawnedMonsterActor (SpawnId) != nullptr;
    switch (Command)
    {
        case EGridObjectCommand::Toggle:
            Command = bIsCurrentlySpawned
                ? EGridObjectCommand::Despawn
                : EGridObjectCommand::Spawn;
            break;

        case EGridObjectCommand::Activate:
        case EGridObjectCommand::Enable:
            Command = EGridObjectCommand::Spawn;
            break;

        case EGridObjectCommand::Deactivate:
        case EGridObjectCommand::Disable:
            Command = EGridObjectCommand::Despawn;
            break;

        default:
            break;
    }

    if (Command == EGridObjectCommand::Despawn)
    {
        return DespawnMonsterSpawnActor (
            *ObjectData,
            true,
            bIsCurrentlySpawned);
    }

    if (Command == EGridObjectCommand::Teleport)
    {
        return TeleportSpawnedMonster (
            SpawnId,
            ObjectData->CellX,
            ObjectData->CellY,
            ObjectData->InitialFacing);
    }

    if (Command != EGridObjectCommand::Spawn)
    {
        return false;
    }

    if (bIsCurrentlySpawned)
    {
        return true;
    }

    const FGridRuntimeMonsterState* RestoreState = nullptr;
    if (const FGridLevelRuntimeState* State =
        FindRuntimeStateForCurrentLevel ())
    {
        if (const FGridRuntimeMonsterPlacementState* PlacementState =
            State->MonsterPlacements.Find (SpawnId))
        {
            RestoreState = PlacementState->bHasMonsterState
                ? &PlacementState->MonsterState
                : nullptr;
        }
    }

    AGridMonsterActor* Monster =
        AddMonsterSpawnActor (*ObjectData, RestoreState);
    if (!Monster)
    {
        UE_LOG (LogGridMonsterState, Log,
            TEXT ("[GridMonsterLifecycle] SpawnRejected SpawnId=%s Reason=AtomicSpawnFailed"),
            *SpawnId.ToString (EGuidFormats::DigitsWithHyphens));
        return false;
    }

    if (!StoreMonsterPlacementState (
            *ObjectData,
            Monster,
            true))
    {
        DespawnMonsterSpawnActor (
            *ObjectData,
            false,
            false);
        return false;
    }

    AbortActiveCombatAndMonsterActions ();
    UE_LOG (LogGridMonsterState, Log,
        TEXT ("[GridMonsterLifecycle] SpawnCommandCompleted SpawnId=%s Encounter=%s"),
        *SpawnId.ToString (EGuidFormats::DigitsWithHyphens),
        *Monster->EncounterGroupId.ToString ());
    ExecuteLinksFromRuntimeObject (
        SpawnId,
        EGridObjectEvent::MonsterSpawned);
    return true;
}

bool AGridLevelRuntimeActor::TeleportSpawnedMonster (
    FGuid SpawnId,
    int32 TargetCellX,
    int32 TargetCellY,
    EGridEdge TargetFacing)
{
    if (!LevelAsset || !SpawnId.IsValid ())
    {
        return false;
    }

    const FGridLevelObjectData* ObjectData =
        LevelAsset->FindMonsterSpawnById (SpawnId);
    AGridMonsterActor* Monster =
        FindSpawnedMonsterActor (SpawnId);
    UGridMonsterMovementComponent* Movement = Monster
        ? Monster->FindComponentByClass<
            UGridMonsterMovementComponent> ()
        : nullptr;
    UGridMonsterOccupancySubsystem* Occupancy = GetWorld ()
        ? GetWorld ()->GetSubsystem<
            UGridMonsterOccupancySubsystem> ()
        : nullptr;
    const FIntPoint TargetCell (TargetCellX, TargetCellY);

    bool bGeneratedMonsterOccupiesTarget = false;
    for (const TPair<FGuid, TObjectPtr<AGridMonsterActor>>& Pair :
        SpawnedMonsterActors)
    {
        const AGridMonsterActor* Other = Pair.Value.Get ();
        if (IsValid (Other) &&
            Other != Monster &&
            Other->CurrentCell == TargetCell)
        {
            bGeneratedMonsterOccupiesTarget = true;
            break;
        }
    }

    if (!ObjectData || !Monster ||
        (Movement && !Movement->IsInitialized ()) ||
        Monster->IsDead () ||
        !IsCardinalMonsterSpawnFacing (TargetFacing) ||
        !IsValidCell (TargetCellX, TargetCellY) ||
        !IsWalkableCell (TargetCellX, TargetCellY) ||
        IsPartyOnCell (TargetCellX, TargetCellY) ||
        bGeneratedMonsterOccupiesTarget ||
        !Occupancy ||
        Occupancy->IsCellBlocked (TargetCell, Monster))
    {
        UE_LOG (LogGridMonsterState, Log,
            TEXT ("[GridMonsterLifecycle] TeleportRejected SpawnId=%s Target=(%d,%d) Facing=%s Reason=InvalidOrOccupiedTarget"),
            *SpawnId.ToString (EGuidFormats::DigitsWithHyphens),
            TargetCellX,
            TargetCellY,
            *GetRuntimeEdgeText (TargetFacing));
        return false;
    }

    if (Monster->CurrentCell == TargetCell &&
        Monster->Facing == TargetFacing)
    {
        return true;
    }

    const FGridLevelRuntimeState* ExistingState =
        FindRuntimeStateForCurrentLevel ();
    if (!ExistingState || !ExistingState->bHasBeenVisited)
    {
        CaptureCurrentLevelRuntimeState ();
    }

    const FIntPoint PreviousCell = Monster->CurrentCell;
    const EGridEdge PreviousFacing = Monster->Facing;
    AbortActiveCombatAndMonsterActions ();
    bool bTeleported = false;
    if (Movement)
    {
        bTeleported = Movement->TeleportToGridPose (
            TargetCell,
            TargetFacing);
    }
    else
    {
        Occupancy->UnregisterMonster (Monster);
        if (Occupancy->RegisterMonster (Monster, TargetCell))
        {
            Monster->CurrentCell = TargetCell;
            Monster->Facing = TargetFacing;
            Monster->SetActorLocation (GetCellCenterWorld (
                TargetCell.X,
                TargetCell.Y));
            Monster->ApplyFacingRotation ();
            bTeleported = true;
        }
        else
        {
            Occupancy->RegisterMonster (Monster, PreviousCell);
        }
    }
    if (!bTeleported)
    {
        return false;
    }

    if (!StoreMonsterPlacementState (
            *ObjectData,
            Monster,
            true))
    {
        if (Movement)
        {
            Movement->TeleportToGridPose (
                PreviousCell,
                PreviousFacing);
        }
        else
        {
            Occupancy->UnregisterMonster (Monster);
            Occupancy->RegisterMonster (Monster, PreviousCell);
            Monster->CurrentCell = PreviousCell;
            Monster->Facing = PreviousFacing;
            Monster->SetActorLocation (GetCellCenterWorld (
                PreviousCell.X,
                PreviousCell.Y));
            Monster->ApplyFacingRotation ();
        }
        return false;
    }

    UE_LOG (LogGridMonsterState, Log,
        TEXT ("[GridMonsterLifecycle] Teleported SpawnId=%s From=(%d,%d) To=(%d,%d) Facing=%s Encounter=%s"),
        *SpawnId.ToString (EGuidFormats::DigitsWithHyphens),
        PreviousCell.X,
        PreviousCell.Y,
        TargetCellX,
        TargetCellY,
        *GetRuntimeEdgeText (TargetFacing),
        *Monster->EncounterGroupId.ToString ());
    ExecuteLinksFromRuntimeObject (
        SpawnId,
        EGridObjectEvent::MonsterTeleported);
    return true;
}

TSubclassOf<AGridRuntimeObjectActor> AGridLevelRuntimeActor::GetObjectRuntimeActorClass (const FGridLevelObjectData& ObjectData) const
{
    const UGridObjectArchetypeAsset* Archetype = FindObjectArchetype (ObjectData.ArchetypeId);
    return Archetype ? Archetype->RuntimeActorClass : nullptr;
}

bool AGridLevelRuntimeActor::IsRuntimeSpawnableObject (const FGridLevelObjectData& ObjectData) const
{
    if (!LevelAsset)
    {
        return false;
    }

    if (!ObjectData.bInitiallyEnabled)
    {
        return false;
    }

    if (!LevelAsset->IsValidCoord (ObjectData.CellX, ObjectData.CellY))
    {
        return false;
    }

    if (ObjectData.Type == EGridLevelObjectType::Item)
    {
        const UGridObjectArchetypeAsset* Archetype = FindObjectArchetype (ObjectData.ArchetypeId);
        return ObjectData.ItemDefinitionAsset ||
            !ObjectData.ItemDefinitionId.IsNone () ||
            (Archetype && (Archetype->DefaultBehavior.Item.ItemDefinitionAsset ||
                !Archetype->DefaultBehavior.Item.ItemDefinitionId.IsNone ())) ||
            !ObjectData.ArchetypeId.IsNone ();
    }

    if (ObjectData.Type == EGridLevelObjectType::MonsterSpawn)
    {
        UGridMonsterDefinitionAsset* Definition = nullptr;
        TSubclassOf<AGridMonsterActor> ActorClass;
        FString Error;
        return ResolveMonsterSpawn (
            ObjectData,
            Definition,
            ActorClass,
            Error);
    }

    const UGridObjectArchetypeAsset* Archetype = FindObjectArchetype (ObjectData.ArchetypeId);
    if (Archetype && Archetype->RuntimeActorClass)
    {
        return !Archetype->IsEdgePlaced () || ObjectData.Edge != EGridEdge::None;
    }

    switch (ObjectData.Type)
    {
        case EGridLevelObjectType::Door:
        case EGridLevelObjectType::Button:
        case EGridLevelObjectType::Lever:
        case EGridLevelObjectType::Receptacle:
        return ObjectData.Edge != EGridEdge::None;

        case EGridLevelObjectType::PressurePlate:
        return true;

        case EGridLevelObjectType::Trigger:
        return true;

        default:
        return false;
    }
}

void AGridLevelRuntimeActor::AddPlacedItemActor (const FGridLevelObjectData& ObjectData)
{
    FTransform Transform;
    if (!GetObjectPlacementTransform (ObjectData, Transform))
    {
        UE_LOG (LogTemp, Warning, TEXT ("Placed item skipped: could not compute placement transform for object %s."), *ObjectData.ObjectId.ToString ());
        return;
    }
    if (!IsSafeRuntimeRenderTransform (Transform))
    {
        LogUnsafeObjectTransform (TEXT ("AddPlacedItemActor"), ObjectData, GetObjectMesh (ObjectData), Transform);
        return;
    }

    const UGridObjectArchetypeAsset* Archetype = FindObjectArchetype (ObjectData.ArchetypeId);
    UGridItemDefinitionAsset* ItemDefinition = ResolveObjectItemDefinitionAsset (ObjectData, Archetype);
    const FName ItemDefinitionId = ResolveObjectItemDefinitionId (ObjectData, Archetype);
    AGridItemActor* ItemActor = SpawnItemActorForDefinition (ItemDefinition, ItemDefinitionId, this, nullptr);
    if (!ItemActor)
    {
        UE_LOG (LogTemp, Warning, TEXT ("Placed item skipped: failed to spawn item definition %s."), *ItemDefinitionId.ToString ());
        return;
    }

    ItemActor->SetActorTransform (Transform);
    ItemActor->SetRuntimeObjectId (ObjectData.ObjectId);
    if (ItemDefinition)
    {
        ItemActor->InitializeFromItemDefinition (ItemDefinition, ObjectData.ObjectId);
    }
    else if (!ItemDefinitionId.IsNone ())
    {
        ItemActor->InitializeFromItemDefinitionId (ItemDefinitionId, ObjectData.ObjectId);
    }
    UGridReadableContentAsset* ReadableContentAsset = ObjectData.ReadableContentAsset;
    FName ReadableContentId = ObjectData.ReadableContentId;
    FText ReadTitleOverride = ObjectData.ReadTitleOverride;
    FText ReadTextOverride = ObjectData.ReadTextOverride;
    if (Archetype)
    {
        const FGridItemBehaviorParams& ItemDefaults = Archetype->DefaultBehavior.Item;
        if (!ReadableContentAsset)
        {
            ReadableContentAsset = ItemDefaults.DefaultReadableContentAsset;
        }
        if (ReadableContentId.IsNone ())
        {
            ReadableContentId = ItemDefaults.DefaultReadableContentId;
        }
        if (ReadTitleOverride.IsEmpty ())
        {
            ReadTitleOverride = ItemDefaults.DefaultReadTitleOverride;
        }
        if (ReadTextOverride.IsEmpty ())
        {
            ReadTextOverride = ItemDefaults.DefaultReadTextOverride;
        }
    }
    ItemActor->InitializeReadableContent (
        ReadableContentAsset,
        ReadableContentId,
        ReadTitleOverride,
        ReadTextOverride);
    ItemActor->SetRuntimeCell (ObjectData.CellX, ObjectData.CellY);
    ItemActor->ConfigureAsWorldPickup ();
    ItemActor->OnRemovedFromWorld ();
    SpawnedItemActors.Add (ItemActor);

    FGridSpawnedItemRuntimeEntry Entry;
    Entry.Cell = FIntPoint (ObjectData.CellX, ObjectData.CellY);
    Entry.Edge = ObjectData.Edge;
    Entry.ItemActor = ItemActor;
    Entry.ObjectId = ObjectData.ObjectId;
    Entry.ItemArchetypeId = ObjectData.ArchetypeId;
    Entry.ItemDefinitionAsset = ItemDefinition;
    Entry.ItemDefinitionId = ItemDefinitionId;
    Entry.Quantity = 1;
    SpawnedItemEntries.Add (Entry);
    UE_LOG (
        LogTemp,
        Log,
        TEXT ("Placed item spawned: %s at object %s. Runtime=%s RebuildGeneration=%d ActiveItemCount=%d"),
        *ItemDefinitionId.ToString (),
        *ObjectData.ObjectId.ToString (),
        *GetName (),
        RuntimeObjectRebuildGeneration,
        SpawnedItemEntries.Num ());
}

void AGridLevelRuntimeActor::AddRuntimeObjectActor (const FGridLevelObjectData& ObjectData)
{
    UStaticMesh* Mesh = nullptr;
    UMaterialInterface* Material = nullptr;
    FTransform Transform;
    const TSubclassOf<AGridRuntimeObjectActor> RuntimeActorClass = GetObjectRuntimeActorClass (ObjectData);
    AGridRuntimeObjectActor* Actor = SpawnRuntimeObjectActor<AGridRuntimeObjectActor> (ObjectData, Mesh, Material, Transform);
    UE_LOG (LogTemp, VeryVerbose,
        TEXT ("GridRuntime Diagnostic AddRuntimeObjectActor ObjectId=%s ArchetypeId=%s ObjectData.Type=%s "
            "RuntimeActorClass=%s ActorClass=%s Mesh=%s Transform=%s"),
        *ObjectData.ObjectId.ToString (),
        *ObjectData.ArchetypeId.ToString (),
        *UEnum::GetValueAsString (ObjectData.Type),
        RuntimeActorClass ? *RuntimeActorClass->GetPathName () : TEXT ("None"),
        Actor ? *Actor->GetClass ()->GetPathName () : TEXT ("None"),
        Mesh ? *Mesh->GetPathName () : TEXT ("None"),
        *Transform.ToHumanReadableString ());
    if (!Actor) return;
    FGridLevelObjectData RuntimeObjectData = ObjectData;
    const UGridObjectArchetypeAsset* Archetype = FindObjectArchetype (ObjectData.ArchetypeId);
    if (AGridReceptacleActor* ReceptacleActor = Cast<AGridReceptacleActor> (Actor))
    {
        ReceptacleActor->ContainedItemActorClass = Archetype ? Archetype->ItemActorClass : nullptr;
    }
    if (AGridMechanismActor* MechanismActor = Cast<AGridMechanismActor> (Actor))
    {
        MechanismActor->InitializeMechanismVisuals (RuntimeObjectData, Archetype, Transform);
        Actor->InitializeGridObject (RuntimeObjectData, Mesh, Material, Transform);
    } else if (AGridGenericObjectActor* GenericActor = Cast<AGridGenericObjectActor> (Actor))
    {
        GenericActor->InitializeGenericObject (RuntimeObjectData, Archetype, Mesh, Material, Transform);
    } else
    {
        Actor->InitializeGridObject (RuntimeObjectData, Mesh, Material, Transform);
    }
    if (ActivationComponent)
    {
        ActivationComponent->RegisterInitialObjectState (RuntimeObjectData);
    }

    if (ObjectData.Type == EGridLevelObjectType::Door && DoorSystemComponent)
    {
        DoorSystemComponent->RegisterDoorObject (ObjectData, Actor);
    }
}

AGridMonsterActor* AGridLevelRuntimeActor::AddMonsterSpawnActor (
    const FGridLevelObjectData& ObjectData,
    const FGridRuntimeMonsterState* RestoreState)
{
    FGridLevelObjectData SpawnData = ObjectData;
    if (RestoreState)
    {
        SpawnData.CellX = RestoreState->CellX;
        SpawnData.CellY = RestoreState->CellY;
        SpawnData.InitialFacing =
            IsCardinalMonsterSpawnFacing (
                RestoreState->Facing)
                ? RestoreState->Facing
                : ObjectData.InitialFacing;
        SpawnData.EncounterGroupId =
            RestoreState->EncounterGroupId;
    }

    const FString SpawnIdText =
        SpawnData.ObjectId.ToString (EGuidFormats::DigitsWithHyphens);
    if (SpawnedMonsterActors.Contains (SpawnData.ObjectId))
    {
        UE_LOG (LogGridMonsterState, Error,
            TEXT ("[GridMonsterSpawn] Skipped SpawnId=%s Cell=(%d,%d) Reason=DuplicateSpawnId"),
            *SpawnIdText,
            SpawnData.CellX,
            SpawnData.CellY);
        return nullptr;
    }

    UGridMonsterDefinitionAsset* Definition = nullptr;
    TSubclassOf<AGridMonsterActor> MonsterActorClass;
    FString ResolutionError;
    if (!ResolveMonsterSpawn (
            SpawnData,
            Definition,
            MonsterActorClass,
            ResolutionError))
    {
        UE_LOG (LogGridMonsterState, Error,
            TEXT ("[GridMonsterSpawn] Skipped SpawnId=%s Cell=(%d,%d) DefinitionId=%s Reason=%s"),
            *SpawnIdText,
            SpawnData.CellX,
            SpawnData.CellY,
            *SpawnData.MonsterDefinitionId.ToString (),
            *ResolutionError);
        return nullptr;
    }

    FTransform SpawnTransform;
    if (!GetMonsterSpawnTransform (
            SpawnData,
            SpawnTransform))
    {
        UE_LOG (LogGridMonsterState, Error,
            TEXT ("[GridMonsterSpawn] Skipped SpawnId=%s Cell=(%d,%d) DefinitionId=%s Reason=InvalidTransform"),
            *SpawnIdText,
            SpawnData.CellX,
            SpawnData.CellY,
            *SpawnData.MonsterDefinitionId.ToString ());
        return nullptr;
    }

    UWorld* World = GetWorld ();
    if (!World)
    {
        UE_LOG (LogGridMonsterState, Error,
            TEXT ("[GridMonsterSpawn] Skipped SpawnId=%s Reason=MissingWorld"),
            *SpawnIdText);
        return nullptr;
    }

    for (TActorIterator<AGridMonsterActor> It (World); It; ++It)
    {
        AGridMonsterActor* ExistingMonster = *It;
        if (IsValid (ExistingMonster) &&
            ExistingMonster->ResolvePersistenceId () ==
                SpawnData.ObjectId)
        {
            UE_LOG (LogGridMonsterState, Error,
                TEXT ("[GridMonsterSpawn] Skipped SpawnId=%s Cell=(%d,%d) Reason=PersistenceIdAlreadyExists ExistingActor=%s"),
                *SpawnIdText,
                SpawnData.CellX,
                SpawnData.CellY,
                *GetNameSafe (ExistingMonster));
            return nullptr;
        }
    }

    const FIntPoint SpawnCell (
        SpawnData.CellX,
        SpawnData.CellY);
    for (const TPair<FGuid, TObjectPtr<AGridMonsterActor>>& Pair :
        SpawnedMonsterActors)
    {
        const AGridMonsterActor* SpawnedMonster =
            Pair.Value.Get ();
        if (IsValid (SpawnedMonster) &&
            !SpawnedMonster->IsDead () &&
            SpawnedMonster->CurrentCell == SpawnCell)
        {
            UE_LOG (LogGridMonsterState, Error,
                TEXT ("[GridMonsterSpawn] Skipped SpawnId=%s Cell=(%d,%d) Reason=GeneratedMonsterCellConflict OccupantSpawnId=%s"),
                *SpawnIdText,
                SpawnCell.X,
                SpawnCell.Y,
                *Pair.Key.ToString (
                    EGuidFormats::DigitsWithHyphens));
            return nullptr;
        }
    }
    if (IsPartyOnCell (SpawnCell.X, SpawnCell.Y))
    {
        UE_LOG (LogGridMonsterState, Error,
            TEXT ("[GridMonsterSpawn] Skipped SpawnId=%s Cell=(%d,%d) Reason=PartyOccupiesCell"),
            *SpawnIdText,
            SpawnCell.X,
            SpawnCell.Y);
        return nullptr;
    }
    if (const UGridMonsterOccupancySubsystem* Occupancy =
        World->GetSubsystem<UGridMonsterOccupancySubsystem> ())
    {
        if (Occupancy->IsCellBlocked (SpawnCell))
        {
            UE_LOG (LogGridMonsterState, Error,
                TEXT ("[GridMonsterSpawn] Skipped SpawnId=%s Cell=(%d,%d) Reason=MonsterOccupancyConflict"),
                *SpawnIdText,
                SpawnCell.X,
                SpawnCell.Y);
            return nullptr;
        }
    }

    AGridMonsterActor* Monster =
        World->SpawnActorDeferred<AGridMonsterActor> (
            MonsterActorClass,
            SpawnTransform,
            this,
            nullptr,
            ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    if (!Monster)
    {
        UE_LOG (LogGridMonsterState, Error,
            TEXT ("[GridMonsterSpawn] Skipped SpawnId=%s Cell=(%d,%d) DefinitionId=%s Class=%s Reason=DeferredSpawnFailed"),
            *SpawnIdText,
            SpawnCell.X,
            SpawnCell.Y,
            *Definition->MonsterId.ToString (),
            *GetPathNameSafe (MonsterActorClass.Get ()));
        return nullptr;
    }

    Monster->HomeDungeonLevelId =
        ResolveRuntimeStateLevelId (
            DungeonAsset,
            CurrentDungeonLevelId);
    Monster->bMonsterEnabled = true;
    Monster->bRuntimeLevelActive = true;
    if (!Monster->InitializeMonster (
            Definition,
            SpawnData.ObjectId,
            SpawnCell,
            SpawnData.InitialFacing,
            SpawnData.EncounterGroupId))
    {
        UE_LOG (LogGridMonsterState, Error,
            TEXT ("[GridMonsterSpawn] Skipped SpawnId=%s Cell=(%d,%d) DefinitionId=%s Class=%s Reason=InitializationFailed"),
            *SpawnIdText,
            SpawnCell.X,
            SpawnCell.Y,
            *Definition->MonsterId.ToString (),
            *GetPathNameSafe (MonsterActorClass.Get ()));
        Monster->Destroy ();
        return nullptr;
    }

    UGameplayStatics::FinishSpawningActor (
        Monster,
        SpawnTransform);
    if (!IsValid (Monster))
    {
        UE_LOG (LogGridMonsterState, Error,
            TEXT ("[GridMonsterSpawn] Skipped SpawnId=%s Cell=(%d,%d) DefinitionId=%s Reason=FinishSpawningFailed"),
            *SpawnIdText,
            SpawnCell.X,
            SpawnCell.Y,
            *Definition->MonsterId.ToString ());
        return nullptr;
    }

    Monster->SetActorLocation (SpawnTransform.GetLocation ());
    Monster->ApplyFacingRotation ();
    ApplyMonsterPlacementMetadata (Monster);
    if (Monster->DeathComponent)
    {
        Monster->DeathComponent->InitializeDeathComponent (this);
    }

    bool bOccupancyInitialized = false;
    if (UGridMonsterMovementComponent* Movement =
        Monster->FindComponentByClass<
            UGridMonsterMovementComponent> ())
    {
        bOccupancyInitialized = Movement->IsInitialized () ||
            Movement->InitializeMovement (this);
    }
    else if (UGridMonsterOccupancySubsystem* Occupancy =
        World->GetSubsystem<UGridMonsterOccupancySubsystem> ())
    {
        bOccupancyInitialized =
            Occupancy->RegisterMonster (Monster, SpawnCell);
    }
    if (!bOccupancyInitialized)
    {
        UE_LOG (LogGridMonsterState, Error,
            TEXT ("[GridMonsterSpawn] Skipped SpawnId=%s Cell=(%d,%d) DefinitionId=%s Reason=OccupancyInitializationFailed"),
            *SpawnIdText,
            SpawnCell.X,
            SpawnCell.Y,
            *Definition->MonsterId.ToString ());
        Monster->Destroy ();
        return nullptr;
    }

    FString PresentationError;
    if (!Monster->ValidatePresentationSetup (PresentationError))
    {
        PresentationError.ReplaceInline (
            TEXT ("\n"),
            TEXT (" | "));
        UE_LOG (LogGridMonsterState, Warning,
            TEXT ("[GridMonsterSpawn] PresentationWarning SpawnId=%s DefinitionId=%s Actor=%s Reason=%s"),
            *SpawnIdText,
            *Definition->MonsterId.ToString (),
            *GetNameSafe (Monster),
            *PresentationError);
    }
    SpawnedMonsterActors.Add (
        SpawnData.ObjectId,
        Monster);

    if (RestoreState &&
        !Monster->RestoreRuntimeMonsterState (
            *RestoreState,
            this))
    {
        SpawnedMonsterActors.Remove (SpawnData.ObjectId);
        SetMonsterRuntimeLevelActive (Monster, false);
        Monster->Destroy ();
        UE_LOG (LogGridMonsterState, Error,
            TEXT ("[GridMonsterSpawn] Skipped SpawnId=%s Cell=(%d,%d) DefinitionId=%s Reason=RestoreStateFailed"),
            *SpawnIdText,
            SpawnCell.X,
            SpawnCell.Y,
            *Definition->MonsterId.ToString ());
        return nullptr;
    }

    UE_LOG (LogGridMonsterState, Log,
        TEXT ("[GridMonsterSpawn] Spawned SpawnId=%s DefinitionId=%s Class=%s Cell=(%d,%d) Facing=%s Encounter=%s RuntimeLevel=%s"),
        *SpawnIdText,
        *Definition->MonsterId.ToString (),
        *Monster->GetClass ()->GetPathName (),
        SpawnCell.X,
        SpawnCell.Y,
        *GetRuntimeEdgeText (SpawnData.InitialFacing),
        *SpawnData.EncounterGroupId.ToString (),
        *Monster->HomeDungeonLevelId.ToString ());
    return Monster;
}

void AGridLevelRuntimeActor::RebuildRuntimeObjects ()
{
    if (!LevelAsset)
    {
        return;
    }
    LevelAsset->EnsureCellCount ();
    RuntimeMonsterSpawnFailureCount = 0;
    const FGridLevelRuntimeState* SavedLevelState =
        FindRuntimeStateForCurrentLevel ();
    for (const FGridLevelObjectData& ObjectData : LevelAsset->Objects)
    {
        if (ObjectData.Type ==
            EGridLevelObjectType::MonsterSpawn)
        {
            const FGridRuntimeMonsterPlacementState* PlacementState =
                SavedLevelState
                    ? SavedLevelState->MonsterPlacements.Find (
                        ObjectData.ObjectId)
                    : nullptr;
            const bool bShouldSpawn = PlacementState
                ? PlacementState->bIsSpawned
                : ObjectData.bInitiallyEnabled;
            const FGridRuntimeMonsterState* RestoreState =
                PlacementState &&
                PlacementState->bHasMonsterState
                    ? &PlacementState->MonsterState
                    : nullptr;
            if (bShouldSpawn &&
                !AddMonsterSpawnActor (
                    ObjectData,
                    RestoreState))
            {
                ++RuntimeMonsterSpawnFailureCount;
            }
            continue;
        }
        if (ObjectData.Type == EGridLevelObjectType::Item)
        {
            if (ObjectData.bInitiallyEnabled)
            {
                AddPlacedItemActor (ObjectData);
            }
            continue;
        }
        if (!IsRuntimeSpawnableObject (ObjectData))
        {
            if (ObjectData.bInitiallyEnabled)
            {
                const UGridObjectArchetypeAsset* Archetype = FindObjectArchetype (ObjectData.ArchetypeId);
                if (Archetype && !Archetype->RuntimeActorClass)
                {
                    UE_LOG (
                        LogTemp,
                        Warning,
                        TEXT ("Runtime object skipped: archetype %s has no RuntimeActorClass."),
                        *ObjectData.ArchetypeId.ToString ());
                }
            }
            continue;
        }
        AddRuntimeObjectActor (ObjectData);
    }
}

FString AGridLevelRuntimeActor::GetRuntimeDebugSummary () const
{
    FString Result;

    Result += FString::Printf (TEXT ("Grid Runtime | Level=%s\n"), LevelAsset ? *LevelAsset->GetName () : TEXT ("None"));
    Result += FString::Printf (TEXT ("Runtime Actors=%d\n"), SpawnedRuntimeObjectActors.Num ());
    Result += FString::Printf (TEXT ("Spawned Items=%d\n"), SpawnedItemActors.Num ());
    Result += FString::Printf (
        TEXT ("Spawned Monsters=%d Failures=%d\n"),
        GetSpawnedMonsterActorCount (),
        RuntimeMonsterSpawnFailureCount);
    Result += ActivationComponent ? ActivationComponent->GetDebugSummary () : TEXT ("Activation | Missing");
    Result += TEXT ("\n");
    Result += DoorSystemComponent ? DoorSystemComponent->GetDebugSummary () : TEXT ("Doors | Missing");
    return Result;
}

void AGridLevelRuntimeActor::LogRuntimeDebugSummary () const
{
    const FString Summary = GetRuntimeDebugSummary ();
    UE_LOG (LogTemp, Log, TEXT ("%s"), *Summary);
    if (ActivationComponent)
    {
        ActivationComponent->LogDebugSummary ();
    }
    if (DoorSystemComponent)
    {
        DoorSystemComponent->LogDebugSummary ();
    }
}

FString AGridLevelRuntimeActor::GetLevelAssetDiagnostics () const
{
    const UWorld* World = GetWorld ();
    FString WorldType = TEXT ("None");
    FString MapName = TEXT ("None");

    if (World)
    {
        MapName = World->GetMapName ();
        switch (World->WorldType)
        {
            case EWorldType::Game:
                WorldType = TEXT ("Game");
                break;

            case EWorldType::PIE:
                WorldType = TEXT ("PIE");
                break;

            case EWorldType::Editor:
                WorldType = TEXT ("Editor");
                break;

            case EWorldType::EditorPreview:
                WorldType = TEXT ("EditorPreview");
                break;

            case EWorldType::GamePreview:
                WorldType = TEXT ("GamePreview");
                break;

            case EWorldType::Inactive:
                WorldType = TEXT ("Inactive");
                break;

            default:
                WorldType = FString::Printf (TEXT ("Unknown(%d)"), static_cast<int32> (World->WorldType));
                break;
        }
    }

    FString Result;
    Result += TEXT ("Grid LevelAsset Diagnostics\n");
    Result += FString::Printf (TEXT ("RuntimeActor=%s\n"), *GetPathName ());
#if WITH_EDITOR
    Result += FString::Printf (TEXT ("ActorLabel=%s\n"), *GetActorLabel ());
#else
    Result += FString::Printf (TEXT ("ActorName=%s\n"), *GetName ());
#endif
    Result += FString::Printf (TEXT ("World=%s\n"), *GetNameSafe (World));
    Result += FString::Printf (TEXT ("WorldType=%s\n"), *WorldType);
    Result += FString::Printf (TEXT ("Map=%s\n"), *MapName);
    Result += FString::Printf (TEXT ("DungeonAsset=%s\n"), DungeonAsset ? *DungeonAsset->GetPathName () : TEXT ("None"));
    Result += FString::Printf (TEXT ("CurrentDungeonLevelId=%s\n"), *CurrentDungeonLevelId.ToString ());
    Result += FString::Printf (TEXT ("LevelAsset=%s\n"), LevelAsset ? *LevelAsset->GetPathName () : TEXT ("None"));

    const FGridLevelRuntimeState* RuntimeState = FindRuntimeStateForCurrentLevel ();
    Result += FString::Printf (TEXT ("HasRuntimeState=%s\n"), RuntimeState && RuntimeState->bHasBeenVisited ? TEXT ("true") : TEXT ("false"));
    Result += FString::Printf (TEXT ("RuntimeRemovedObjects=%d\n"), CountRemovedRuntimeObjects (RuntimeState));
    Result += FString::Printf (TEXT ("RuntimeDoors=%d\n"), RuntimeState ? RuntimeState->Doors.Num () : 0);
    Result += FString::Printf (TEXT ("RuntimeItems=%d\n"), RuntimeState ? RuntimeState->Items.Num () : 0);
    Result += FString::Printf (TEXT ("RuntimeReceptacles=%d\n"), RuntimeState ? RuntimeState->Receptacles.Num () : 0);

    const FName RuntimeLevelId =
        ResolveRuntimeStateLevelId (DungeonAsset, CurrentDungeonLevelId);
    TArray<AGridMonsterActor*> DiagnosticMonsters;
    GetWorldMonsters (World, DiagnosticMonsters);
    int32 AssociatedMonsterCount = 0;
    int32 InvalidMonsterIdCount = 0;
    TMap<FGuid, int32> MonsterIdCounts;
    for (AGridMonsterActor* Monster : DiagnosticMonsters)
    {
        if (Monster->ResolveRuntimeDungeonLevelId (RuntimeLevelId) !=
            RuntimeLevelId)
        {
            continue;
        }

        ++AssociatedMonsterCount;
        const FGuid PersistenceId =
            Monster->ResolvePersistenceId ();
        if (!PersistenceId.IsValid ())
        {
            ++InvalidMonsterIdCount;
            continue;
        }
        ++MonsterIdCounts.FindOrAdd (PersistenceId);
    }

    int32 DuplicateMonsterIdCount = 0;
    for (const TPair<FGuid, int32>& Pair : MonsterIdCounts)
    {
        DuplicateMonsterIdCount += Pair.Value > 1
            ? Pair.Value
            : 0;
    }

    int32 SavedDeadMonsterCount = 0;
    if (RuntimeState)
    {
        for (const TPair<FGuid, FGridRuntimeMonsterState>& Pair :
            RuntimeState->Monsters)
        {
            SavedDeadMonsterCount += Pair.Value.bIsDead ? 1 : 0;
        }
    }

    Result += FString::Printf (
        TEXT ("MonstersAssociated=%d RuntimeMonsters=%d RuntimeDeadMonsters=%d InvalidMonsterIds=%d DuplicateMonsterIds=%d\n"),
        AssociatedMonsterCount,
        RuntimeState ? RuntimeState->Monsters.Num () : 0,
        SavedDeadMonsterCount,
        InvalidMonsterIdCount,
        DuplicateMonsterIdCount);

    if (!LevelAsset)
    {
        Result += TEXT ("Status=ERROR: missing LevelAsset reference.\n");
        return Result;
    }

    const int32 ExpectedCellCount = LevelAsset->Width * LevelAsset->Height;
    int32 NonEmptyCellCount = 0;
    int32 BlockingCellCount = 0;
    int32 CeilingCellCount = 0;
    const int32 TransitionObjectCount = CountRuntimeTransitionObjects (LevelAsset);
    const int32 HiddenFloorCellCount = CountHiddenFloorCells (LevelAsset, this);

    for (const FGridLevelCellData& Cell : LevelAsset->Cells)
    {
        if (Cell.CellType != EGridCellType::Empty)
        {
            ++NonEmptyCellCount;
        }
        if (Cell.bBlocksOccupancy)
        {
            ++BlockingCellCount;
        }
        if (Cell.bHasCeiling)
        {
            ++CeilingCellCount;
        }
    }

    Result += FString::Printf (TEXT ("AssetPackage=%s\n"), *GetNameSafe (LevelAsset->GetOutermost ()));
    Result += FString::Printf (TEXT ("GridSize=%dx%d\n"), LevelAsset->Width, LevelAsset->Height);
    Result += FString::Printf (TEXT ("CellSize=%.2f\n"), LevelAsset->CellSize);
    Result += FString::Printf (TEXT ("StartCell=(%d,%d) StartFacing=%s StartCellValid=%s\n"),
        LevelAsset->StartCellX,
        LevelAsset->StartCellY,
        StaticEnum<EGridEdge> () ? *StaticEnum<EGridEdge> ()->GetNameStringByValue (static_cast<int64> (LevelAsset->StartFacing)) : TEXT ("Unknown"),
        LevelAsset->IsStartCellValid () ? TEXT ("true") : TEXT ("false"));
    Result += FString::Printf (TEXT ("Cells=%d ExpectedCells=%d\n"), LevelAsset->Cells.Num (), ExpectedCellCount);
    Result += FString::Printf (TEXT ("NonEmptyCells=%d BlockingCells=%d CeilingCells=%d\n"),
        NonEmptyCellCount,
        BlockingCellCount,
        CeilingCellCount);
    Result += FString::Printf (
        TEXT ("Objects=%d Links=%d TransitionObjects=%d HiddenFloorCells=%d\n"),
        LevelAsset->Objects.Num (),
        LevelAsset->Links.Num (),
        TransitionObjectCount,
        HiddenFloorCellCount);
    Result += FString::Printf (TEXT ("ObjectArchetypesOnRuntimeActor=%d\n"), ObjectArchetypes.Num ());
    Result += FString::Printf (TEXT ("FloorMesh=%s WallMesh=%s CeilingMesh=%s\n"),
        *GetNameSafe (FloorMesh),
        *GetNameSafe (WallMesh),
        *GetNameSafe (CeilingMesh));

    if (LevelAsset->Cells.Num () != ExpectedCellCount)
    {
        Result += TEXT ("Status=WARNING: Cells.Num does not match Width*Height. Run EnsureLevelReady from the editor actor.\n");
    }
    else
    {
        Result += TEXT ("Status=OK\n");
    }

    return Result;
}

void AGridLevelRuntimeActor::LogLevelAssetDiagnostics () const
{
    const FString Diagnostics = GetLevelAssetDiagnostics ();
    UE_LOG (LogTemp, Log, TEXT ("%s"), *Diagnostics);
}

FString AGridLevelRuntimeActor::GetPIEReadinessDiagnostics () const
{
    const UWorld* World = GetWorld ();
    const bool bHasGameWorld = World && World->IsGameWorld ();
    const bool bHasRequiredMeshes = FloorMesh && WallMesh && CeilingMesh;
    const bool bHasValidStart = LevelAsset && LevelAsset->IsStartCellValid ();

    int32 NullArchetypeCount = 0;
    for (const TObjectPtr<UGridObjectArchetypeAsset>& Archetype : ObjectArchetypes)
    {
        if (!Archetype)
        {
            ++NullArchetypeCount;
        }
    }

    AGrimrockPartyPawn* FoundPartyPawn = nullptr;
    AGameModeBase* ActiveGameMode = nullptr;
    if (World)
    {
        ActiveGameMode = World->GetAuthGameMode ();
        for (TActorIterator<AGrimrockPartyPawn> It (World); It; ++It)
        {
            FoundPartyPawn = *It;
            break;
        }
    }

    FString Result;
    Result += TEXT ("GridLevelRuntimeActor PIE Readiness\n");
    Result += FString::Printf (TEXT ("RuntimeActor: %s\n"), *GetName ());
    Result += FString::Printf (TEXT ("World: %s\n"), World ? *World->GetMapName () : TEXT ("None"));
    Result += FString::Printf (TEXT ("WorldType: %s\n"), *GetRuntimeWorldTypeText (World));
    Result += FString::Printf (TEXT ("IsGameWorld: %s\n"), *GetRuntimeBoolText (bHasGameWorld));
    Result += FString::Printf (TEXT ("DungeonAsset: %s\n"), DungeonAsset ? *DungeonAsset->GetPathName () : TEXT ("None"));
    Result += FString::Printf (TEXT ("CurrentDungeonLevelId: %s\n"), *CurrentDungeonLevelId.ToString ());
    Result += FString::Printf (TEXT ("LevelAsset: %s\n"), LevelAsset ? *LevelAsset->GetPathName () : TEXT ("None"));
    Result += FString::Printf (TEXT ("ApplyLevelStartOnBeginPlay: %s\n"), *GetRuntimeBoolText (bApplyLevelStartOnBeginPlay));

    if (LevelAsset)
    {
        Result += FString::Printf (
            TEXT ("Start: Cell=(%d,%d) Facing=%s Valid=%s\n"),
            LevelAsset->StartCellX,
            LevelAsset->StartCellY,
            *GetRuntimeEdgeText (LevelAsset->StartFacing),
            *GetRuntimeBoolText (bHasValidStart));
        Result += FString::Printf (
            TEXT ("Asset Stats: Cells=%d Objects=%d Links=%d TransitionObjects=%d\n"),
            LevelAsset->Cells.Num (),
            LevelAsset->Objects.Num (),
            LevelAsset->Links.Num (),
            CountRuntimeTransitionObjects (LevelAsset));
    }
    else
    {
        Result += TEXT ("Start: Cell=None Facing=None Valid=false\n");
        Result += TEXT ("Asset Stats: Cells=0 Objects=0 Links=0 TransitionObjects=0\n");
    }

    Result += FString::Printf (
        TEXT ("Meshes: Floor=%s Wall=%s Ceiling=%s\n"),
        *GetNameSafe (FloorMesh),
        *GetNameSafe (WallMesh),
        *GetNameSafe (CeilingMesh));
    Result += FString::Printf (
        TEXT ("ObjectArchetypes: Count=%d NullEntries=%d\n"),
        ObjectArchetypes.Num (),
        NullArchetypeCount);
    Result += FString::Printf (
        TEXT ("Components: Activation=%s Doors=%s EditorPreview=%s\n"),
        *GetRuntimeBoolText (ActivationComponent != nullptr),
        *GetRuntimeBoolText (DoorSystemComponent != nullptr),
        *GetRuntimeBoolText (EditorPreviewComponent != nullptr));
    Result += FString::Printf (
        TEXT ("EditorPreviewInGameWorld: %s\n"),
        bHasGameWorld
            ? TEXT ("Runtime objects are used; editor preview rebuild is skipped.")
            : TEXT ("Editor preview may be used outside PIE/game world."));
    Result += FString::Printf (
        TEXT ("PartyPawnInCurrentWorld: %s\n"),
        FoundPartyPawn ? *FoundPartyPawn->GetName () : TEXT ("None"));
    Result += FString::Printf (
        TEXT ("ActiveGameMode: %s\n"),
        ActiveGameMode ? *ActiveGameMode->GetClass ()->GetName () : TEXT ("None"));
    Result += FString::Printf (
        TEXT ("GameModeDefaultPawnClass: %s\n"),
        ActiveGameMode && ActiveGameMode->DefaultPawnClass ? *ActiveGameMode->DefaultPawnClass->GetName () : TEXT ("None"));
    Result += FString::Printf (
        TEXT ("GameModePlayerControllerClass: %s\n"),
        ActiveGameMode && ActiveGameMode->PlayerControllerClass ? *ActiveGameMode->PlayerControllerClass->GetName () : TEXT ("None"));

    if (ActiveGameMode && ActiveGameMode->GetClass () == AGrimrockGameMode::StaticClass ())
    {
        Result += TEXT ("GameModeNote: Native AGrimrockGameMode spawns the native C++ pawn unless a Blueprint override is used.\n");
    }
    else if (ActiveGameMode && ActiveGameMode->GetClass ()->IsChildOf (AGrimrockGameMode::StaticClass ()))
    {
        Result += TEXT ("GameModeNote: Blueprint GameMode is active; its Default Pawn Class and Player Controller Class overrides are used.\n");
    }
    else if (ActiveGameMode)
    {
        Result += TEXT ("GameModeNote: Active GameMode is not derived from AGrimrockGameMode.\n");
    }
    else
    {
        Result += TEXT ("GameModeNote: No active GameMode in this world yet. Run PIE to inspect the spawned GameMode.\n");
    }

    if (!LevelAsset)
    {
        Result += TEXT ("Status: ERROR - LevelAsset is null.");
    }
    else if (!bHasValidStart)
    {
        Result += TEXT ("Status: ERROR - LevelAsset start cell is invalid.");
    }
    else if (!bHasRequiredMeshes)
    {
        Result += TEXT ("Status: ERROR - FloorMesh, WallMesh or CeilingMesh is missing.");
    }
    else if (!ActivationComponent || !DoorSystemComponent)
    {
        Result += TEXT ("Status: ERROR - Runtime activation or door component is missing.");
    }
    else if (!FoundPartyPawn)
    {
        Result += TEXT ("Status: WARNING - No AGrimrockPartyPawn exists in the current world. PIE can still work if GameMode spawns one.");
    }
    else
    {
        Result += TEXT ("Status: OK - Runtime actor is ready for PIE with this LevelAsset.");
    }

    return Result;
}

void AGridLevelRuntimeActor::LogPIEReadinessDiagnostics () const
{
    UE_LOG (LogTemp, Log, TEXT ("%s"), *GetPIEReadinessDiagnostics ());
}

void AGridLevelRuntimeActor::ShowRuntimeDebugSummary (float Duration) const
{
    if (!GEngine)
    {
        return;
    }
    GEngine->AddOnScreenDebugMessage (-1, Duration, FColor::Green, GetRuntimeDebugSummary ()
    );
}
