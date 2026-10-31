#include "Runtime/GridLevelRuntimeActor.h"

#include "Core/GridLevelAsset.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "RPG/RPGSkillAsset.h"
#include "RPG/RPGSkillCheckService.h"
#include "Runtime/GridDoorSystemComponent.h"
#include "Runtime/GridDungeonTrapPerceptionService.h"
#include "Runtime/GridEventFeedPublisher.h"
#include "Runtime/GridPartyIlluminationComponent.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/Map/GridMapRevealService.h"

DEFINE_LOG_CATEGORY_STATIC(LogGridDungeonTrapPerception, Log, All);

int32 AGridLevelRuntimeActor::EvaluatePassiveDungeonTrapPerception(
    AGrimrockPartyPawn* PartyPawn, FRandomStream& RandomStream)
{
    if (!IsValid(PartyPawn) || PartyPawn->LevelRuntimeActor != this ||
        !IsValid(LevelAsset) || !IsValid(PartyPawn->PartyInventoryComponent))
    {
        return 0;
    }
    const URPGSkillAsset* Skill = LoadObject<URPGSkillAsset>(nullptr,
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/Skills/DA_Skill_Perception.DA_Skill_Perception"));
    const TArray<FGridCharacterInventoryState>& Characters =
        PartyPawn->PartyInventoryComponent->PartyInventoryState.ActiveCharacters;
    int32 ObserverScore = 0;
    const int32 ObserverIndex =
        FGridDungeonTrapPerceptionService::SelectBestObserver(Characters, Skill, ObserverScore);
    if (!Characters.IsValidIndex(ObserverIndex))
    {
        return 0;
    }

    const UGridPartyIlluminationComponent* Light =
        PartyPawn->FindComponentByClass<UGridPartyIlluminationComponent>();
    const bool bHasLight = IsValid(Light) && Light->HasActiveIlluminationSource();
    const FIntPoint Origin(PartyPawn->CurrentCellX, PartyPawn->CurrentCellY);
    FGridLevelRuntimeState* State = GetOrCreateRuntimeStateForCurrentLevel();
    if (!State)
    {
        return 0;
    }
    int32 Discoveries = 0;

    for (const FGridWorldObjectInstance& Instance : LevelAsset->WorldObjectInstances)
    {
        const UGridWorldObjectDefinitionAsset* Definition =
            Instance.Type == EGridLevelObjectType::Trigger
                ? FindWorldObjectDefinition(Instance.WorldObjectDefinitionId) : nullptr;
        if (!IsValid(Definition) || !Definition->DungeonTrap.bIsDungeonTrap ||
            !Definition->DungeonTrap.IsValidContract() ||
            GetDungeonTrapState(Instance.InstanceId) != EGridDungeonTrapState::Hidden)
        {
            continue;
        }
        EGridEdge Direction = EGridEdge::None;
        int32 Distance = 0;
        if (!FGridDungeonTrapPerceptionService::GetCardinalDirectionAndDistance(
                Origin, FIntPoint(Instance.CellX, Instance.CellY), Direction, Distance))
        {
            continue;
        }
        const FGridDungeonTrapDefinitionParams& TrapParams = Definition->DungeonTrap;
        const int32 Range = FGridDungeonTrapPerceptionService::GetMaxRangeCells(
            ObserverScore, bHasLight, TrapParams.DetectionSense, TrapParams.MaxDetectionRangeCells);
        if (Distance > Range || Distance > 4)
        {
            continue;
        }

        // A straight cardinal sight/sound path, never through a closed door
        // or a solid wall. Extended auditory propagation is deferred.
        bool bUnobstructed = true;
        FIntPoint Cursor = Origin;
        for (int32 Step = 0; Step < Distance; ++Step)
        {
            if (!FGridMapRevealService::CanRevealAcrossEdge(
                *LevelAsset, DoorSystemComponent, Cursor, Direction))
            {
                bUnobstructed = false;
                break;
            }
            switch (Direction)
            {
                case EGridEdge::North: ++Cursor.Y; break;
                case EGridEdge::East: ++Cursor.X; break;
                case EGridEdge::South: --Cursor.Y; break;
                case EGridEdge::West: --Cursor.X; break;
                default: bUnobstructed = false; break;
            }
            if (!bUnobstructed) break;
        }
        if (!bUnobstructed)
        {
            continue;
        }

        const uint8 AttemptBit = static_cast<uint8>(1u << (Distance - 1));
        uint8& AttemptMask = State->TrapPerceptionAttemptedRangeMask.FindOrAdd(Instance.InstanceId);
        if ((AttemptMask & AttemptBit) != 0)
        {
            continue;
        }
        AttemptMask |= AttemptBit; // Persist before any d20: no save-scumming per distance.

        const int32 DarknessPenalty =
            !bHasLight && TrapParams.DetectionSense == EGridDungeonTrapDetectionSense::Visual ? 2 : 0;
        const int32 EffectiveDifficulty = TrapParams.DetectionDifficulty + 2 * (Distance - 1) + DarknessPenalty;
        FRPGSkillCheckResult Result;
        if (!FRPGSkillCheckService::TryResolveSkillCheck(
            Characters[ObserverIndex], Skill, EffectiveDifficulty, RandomStream, Result))
        {
            UE_LOG(LogGridDungeonTrapPerception, Warning,
                TEXT("[TRAP01.2] Perception check rejected: %s"),
                *UEnum::GetValueAsString(Result.RejectReason));
            continue;
        }
        if (!Result.bSuccess || !TryRecordDungeonTrapDetection(Instance.InstanceId))
        {
            continue;
        }
        ++Discoveries;

        const FText ObserverName = Characters[ObserverIndex].DisplayName.IsEmpty()
            ? NSLOCTEXT("TRAP01", "ObserverFallback", "Un membre du groupe")
            : Characters[ObserverIndex].DisplayName;
        const FText Where = FGridDungeonTrapPerceptionService::FormatRelativeLocation(
            PartyPawn->Facing, Direction, Distance, Definition->PlacementSurface);
        const FText Message = FText::Format(
            NSLOCTEXT("TRAP01", "TrapDetectedEvent", "{0} repère un piège {1}."),
            ObserverName, Where);
        GridEventFeedPublisher::PublishDungeonEvent(
            this, FName(TEXT("Dungeon.Trap.Detected")), Message,
            EGridHudEventSemantic::Mechanism, EGridEventVisibility::Always,
            true, GridEventFeedPublisher::SourceIdFromGuid(Instance.InstanceId));
        UE_LOG(LogGridDungeonTrapPerception, Log,
            TEXT("[TRAP01.2] Discovered %s by character %d at (%d,%d), distance=%d, roll=%d total=%d DC=%d"),
            *Instance.InstanceId.ToString(), ObserverIndex, Instance.CellX, Instance.CellY,
            Distance, Result.Roll, Result.Total, EffectiveDifficulty);
    }
    return Discoveries;
}
