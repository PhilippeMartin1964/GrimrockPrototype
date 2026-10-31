#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridDungeonTrapDisarmFeedback.h"
#include "Core/GridDungeonTrapInstanceUtils.h"

#include "RPG/RPGSkillAsset.h"
#include "RPG/RPGExperienceRewardService.h"
#include "Runtime/GridDungeonTrapRewardRules.h"
#include "RPG/RPGSkillCheckService.h"
#include "RPG/RPGSkillService.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/GridRuntimeObjectActor.h"
#include "Runtime/GridInteractableInterface.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"
#include "Runtime/GridEventFeedPublisher.h"
#include "Runtime/GridDungeonTrapPerceptionService.h"
#include "Components/StaticMeshComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogGridCombatTrapRuntime, Log, All);

bool AGridLevelRuntimeActor::ApplyCombatTrapAtCell(int32 CellX, int32 CellY, const FGridCombatTrapEffectProfile& Profile,
	const FGuid& SourceCombatantId, FName SourceActionId, int32 ResolvedRawDamage)
{
	if (!Profile.bPlaceTrap || !Profile.IsValid() || !IsValidCell(CellX, CellY) || !IsWalkableCell(CellX, CellY) ||
		!SourceCombatantId.IsValid() || SourceActionId.IsNone() || ResolvedRawDamage < 0)
	{
		return false;
	}
	FGridLevelRuntimeState* RuntimeState = GetOrCreateRuntimeStateForCurrentLevel();
	if (!RuntimeState)
	{
		return false;
	}

	FGridCombatTrapState State;
	State.TrapId = Profile.TrapId;
	State.RemainingRounds = Profile.DurationRounds;
	State.SourceCombatantId = SourceCombatantId;
	State.SourceActionId = SourceActionId;
	State.RawDamage = ResolvedRawDamage;
	State.DamageType = Profile.DamageType;
	State.PhysicalSubtype = Profile.PhysicalSubtype;
	State.StatusApplications = Profile.StatusApplications;
	State.bConsumeOnTrigger = Profile.bConsumeOnTrigger;
	if (!State.IsValid())
	{
		return false;
	}

	RuntimeState->CombatTraps.Add(FIntPoint(CellX, CellY), State);
	UE_LOG(LogGridCombatTrapRuntime, Log, TEXT("[RPG03.9.2] TrapApplied Cell=(%d,%d) Trap=%s Rounds=%d Damage=%d"),
		CellX, CellY, *State.TrapId.ToString(), State.RemainingRounds, State.RawDamage);
	return true;
}

const FGridCombatTrapState* AGridLevelRuntimeActor::FindCombatTrapAtCell(int32 CellX, int32 CellY) const
{
	const FGridLevelRuntimeState* RuntimeState = FindRuntimeStateForCurrentLevel();
	return RuntimeState ? RuntimeState->CombatTraps.Find(FIntPoint(CellX, CellY)) : nullptr;
}

bool AGridLevelRuntimeActor::ConsumeCombatTrapAtCell(int32 CellX, int32 CellY, FGridCombatTrapState& OutState)
{
	OutState = FGridCombatTrapState();
	FGridLevelRuntimeState* RuntimeState = GetOrCreateRuntimeStateForCurrentLevel();
	if (!RuntimeState)
	{
		return false;
	}
	const FIntPoint Cell(CellX, CellY);
	FGridCombatTrapState* State = RuntimeState->CombatTraps.Find(Cell);
	if (!State || !State->IsValid())
	{
		return false;
	}
	OutState = *State;
	RuntimeState->CombatTraps.Remove(Cell);
	return true;
}

int32 AGridLevelRuntimeActor::AdvanceCombatTrapRound(TArray<FIntPoint>* OutExpiredCells)
{
	if (OutExpiredCells) OutExpiredCells->Reset();
	FGridLevelRuntimeState* RuntimeState = GetOrCreateRuntimeStateForCurrentLevel();
	if (!RuntimeState)
	{
		return 0;
	}

	TArray<FIntPoint> Cells;
	RuntimeState->CombatTraps.GetKeys(Cells);
	Cells.Sort([](const FIntPoint& A, const FIntPoint& B) { return A.Y == B.Y ? A.X < B.X : A.Y < B.Y; });

	int32 Expired = 0;
	for (const FIntPoint& Cell : Cells)
	{
		FGridCombatTrapState* State = RuntimeState->CombatTraps.Find(Cell);
		if (!State) continue;
		State->RemainingRounds = FMath::Max(0, State->RemainingRounds - 1);
		if (State->RemainingRounds > 0) continue;
		if (OutExpiredCells) OutExpiredCells->Add(Cell);
		RuntimeState->CombatTraps.Remove(Cell);
		++Expired;
	}
	return Expired;
}


// ARMEMENT01.16.2A — Persistent authored dungeon traps are Trigger objects,
// not FGridCombatTrapState (which is temporary and keyed by cell).
bool AGridLevelRuntimeActor::IsTriggerTrapDisarmed(FGuid TrapObjectId) const
{
    const FGridLevelRuntimeState* State = FindRuntimeStateForCurrentLevel();
    return TrapObjectId.IsValid() && State &&
        State->DisarmedTriggerTrapIds.Contains(TrapObjectId);
}

bool AGridLevelRuntimeActor::IsTriggerTrapDetected(FGuid TrapObjectId) const
{
    const FGridLevelRuntimeState* State = FindRuntimeStateForCurrentLevel();
    return TrapObjectId.IsValid() && State &&
        State->DetectedTriggerTrapIds.Contains(TrapObjectId);
}

EGridDungeonTrapState AGridLevelRuntimeActor::GetDungeonTrapState(FGuid TrapObjectId) const
{
    if (!TrapObjectId.IsValid() || !IsValid(LevelAsset))
    {
        return EGridDungeonTrapState::Unknown;
    }
    const FGridWorldObjectInstance* Trap =
        LevelAsset->FindWorldObjectInstanceById(TrapObjectId);
    const UGridWorldObjectDefinitionAsset* Definition = Trap
        ? FindWorldObjectDefinition(Trap->WorldObjectDefinitionId) : nullptr;
    if (!Trap || Trap->Type != EGridLevelObjectType::Trigger ||
        !IsValid(Definition) || Definition->SupportedType != EGridLevelObjectType::Trigger ||
        !Definition->DungeonTrap.bIsDungeonTrap ||
        !GridDungeonTrapInstance::Resolve(*Trap, *Definition).IsValidContract())
    {
        return EGridDungeonTrapState::Unknown;
    }

    const FGridLevelRuntimeState* State = FindRuntimeStateForCurrentLevel();
    if (!State)
    {
        return EGridDungeonTrapState::Hidden;
    }
    if (State->DisabledTriggerTrapIds.Contains(TrapObjectId))
    {
        return EGridDungeonTrapState::Disabled;
    }
    if (State->DisarmedTriggerTrapIds.Contains(TrapObjectId))
    {
        return EGridDungeonTrapState::Disarmed;
    }
    if (State->TriggeredTriggerTrapIds.Contains(TrapObjectId))
    {
        return EGridDungeonTrapState::Triggered;
    }
    return State->DetectedTriggerTrapIds.Contains(TrapObjectId)
        ? EGridDungeonTrapState::Detected : EGridDungeonTrapState::Hidden;
}

bool AGridLevelRuntimeActor::TryRecordDungeonTrapDetection(FGuid TrapObjectId)
{
    const EGridDungeonTrapState CurrentState = GetDungeonTrapState(TrapObjectId);
    if (CurrentState != EGridDungeonTrapState::Hidden)
    {
        return false;
    }
    FGridLevelRuntimeState* State = GetOrCreateRuntimeStateForCurrentLevel();
    if (!State)
    {
        return false;
    }
    State->DetectedTriggerTrapIds.Add(TrapObjectId);
    RefreshDungeonTrapRevealVisual(TrapObjectId);
    return true;
}

void AGridLevelRuntimeActor::RefreshDungeonTrapRevealVisual(FGuid TrapObjectId)
{
    const EGridDungeonTrapState TrapState = GetDungeonTrapState(TrapObjectId);
    if (TrapState == EGridDungeonTrapState::Unknown || !IsValid(LevelAsset))
    {
        return;
    }
    const FGridWorldObjectInstance* Instance =
        LevelAsset->FindWorldObjectInstanceById(TrapObjectId);
    const UGridWorldObjectDefinitionAsset* Definition = Instance
        ? FindWorldObjectDefinition(Instance->WorldObjectDefinitionId) : nullptr;
    if (!IsValid(Definition))
    {
        return;
    }

    AGridRuntimeObjectActor* Visual = FindRuntimeObjectActor<AGridRuntimeObjectActor>(TrapObjectId);
    const bool bRevealed = TrapState != EGridDungeonTrapState::Hidden;
    if (!IsValid(Visual) && bRevealed && Definition->RuntimeActorClass)
    {
        // This is the same normal world-object visual pipeline; its GUID
        // remains the original level instance. No secondary gameplay actor.
        AddRuntimeObjectActor(*Instance);
        Visual = FindRuntimeObjectActor<AGridRuntimeObjectActor>(TrapObjectId);
    }
    if (!IsValid(Visual))
    {
        return; // No visual authored yet: gameplay discovery still persists.
    }
    Visual->SetActorHiddenInGame(!bRevealed);
    if (UStaticMeshComponent* Mesh = Visual->MeshComponent)
    {
        // Revealed visuals have mouse query collision only; the actor still
        // never blocks party movement. No collision exists for hidden traps.
        if (Visual->GetClass()->ImplementsInterface(UGridInteractableInterface::StaticClass()))
        {
            Mesh->SetCollisionEnabled(bRevealed ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
            Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
            Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
        }
        const FGridDungeonTrapDefinitionParams& Presentation = Definition->DungeonTrap;
        const bool bDangerous = TrapState == EGridDungeonTrapState::Detected ||
            (TrapState == EGridDungeonTrapState::Triggered && !Presentation.bOneShot);
        const bool bHighlight = bDangerous && Presentation.bEnableDangerHighlight;
        const bool bCustomDepth = bHighlight && Presentation.bUseDangerCustomDepth;
        // Assign both active and inactive values: refresh must also remove old effects.
        Mesh->SetOverlayMaterial(bHighlight ? Presentation.RevealedOverlayMaterial.Get() : nullptr);
        Mesh->SetRenderCustomDepth(bCustomDepth);
        Mesh->SetCustomDepthStencilValue(bCustomDepth ? FMath::Clamp(Presentation.DangerStencilValue, 3, 255) : 0);
    }
}

void AGridLevelRuntimeActor::RestoreDungeonTrapRevealVisuals()
{
    if (!IsValid(LevelAsset))
    {
        return;
    }
    for (const FGridWorldObjectInstance& Object : LevelAsset->WorldObjectInstances)
    {
        if (GetDungeonTrapState(Object.InstanceId) != EGridDungeonTrapState::Unknown)
        {
            RefreshDungeonTrapRevealVisual(Object.InstanceId);
        }
    }
}

bool AGridLevelRuntimeActor::TryRecordDungeonTrapTriggered(
    FGuid TrapObjectId, AGrimrockPartyPawn* PartyPawn)
{
    const EGridDungeonTrapState CurrentState = GetDungeonTrapState(TrapObjectId);
    if (CurrentState != EGridDungeonTrapState::Hidden &&
        CurrentState != EGridDungeonTrapState::Detected &&
        CurrentState != EGridDungeonTrapState::Triggered)
    {
        return false;
    }
    const FGridWorldObjectInstance* Instance =
        LevelAsset->FindWorldObjectInstanceById(TrapObjectId);
    const UGridWorldObjectDefinitionAsset* Definition = Instance
        ? FindWorldObjectDefinition(Instance->WorldObjectDefinitionId) : nullptr;
    if (!IsValid(Definition) ||
        (CurrentState == EGridDungeonTrapState::Triggered && Definition->DungeonTrap.bOneShot))
    {
        return false;
    }
    FGridLevelRuntimeState* State = GetOrCreateRuntimeStateForCurrentLevel();
    if (!State)
    {
        return false;
    }
    const bool bFirstTrigger = !State->TriggeredTriggerTrapIds.Contains(TrapObjectId);
    State->TriggeredTriggerTrapIds.Add(TrapObjectId);
    State->DetectedTriggerTrapIds.Add(TrapObjectId);
    RefreshDungeonTrapRevealVisual(TrapObjectId);
    if (bFirstTrigger && IsValid(PartyPawn) && PartyPawn->LevelRuntimeActor == this)
    {
        const int32 Dx = Instance->CellX - PartyPawn->CurrentCellX;
        const int32 Dy = Instance->CellY - PartyPawn->CurrentCellY;
        const int32 Distance = FMath::Abs(Dx) + FMath::Abs(Dy);
        EGridEdge Direction = EGridEdge::None;
        int32 CardinalDistance = 0;
        FGridDungeonTrapPerceptionService::GetCardinalDirectionAndDistance(
            FIntPoint(PartyPawn->CurrentCellX, PartyPawn->CurrentCellY),
            FIntPoint(Instance->CellX, Instance->CellY), Direction, CardinalDistance);
        const FText Where = Distance == 0
            ? NSLOCTEXT("TRAP01", "TrapUnderParty", "sous le groupe")
            : FGridDungeonTrapPerceptionService::FormatRelativeLocation(
                PartyPawn->Facing, Direction, Distance, Definition->PlacementSurface);
        GridEventFeedPublisher::PublishDungeonEvent(
            this, FName(TEXT("Dungeon.Trap.Triggered")),
            FText::Format(NSLOCTEXT("TRAP01", "TrapTriggeredEvent", "Un piège se déclenche {0} !"), Where),
            EGridHudEventSemantic::Mechanism, EGridEventVisibility::Always,
            true, GridEventFeedPublisher::SourceIdFromGuid(TrapObjectId));
    }
    return true;
}

bool AGridLevelRuntimeActor::TryInteractWithRevealedDungeonTrap(
    AGrimrockPartyPawn* PartyPawn, FGuid TrapObjectId)
{
    if (!IsValid(PartyPawn) || PartyPawn->LevelRuntimeActor != this)
    {
        return false;
    }
    const EGridDungeonTrapState TrapState = GetDungeonTrapState(TrapObjectId);
    if (TrapState == EGridDungeonTrapState::Hidden ||
        TrapState == EGridDungeonTrapState::Unknown)
    {
        return false; // No spoiler, no menu, no interaction.
    }

    auto Report = [this, TrapObjectId](FName EventCode, const FText& Message)
    {
        GridEventFeedPublisher::PublishDungeonEvent(
            this, EventCode, Message, EGridHudEventSemantic::Mechanism,
            EGridEventVisibility::Always, true,
            GridEventFeedPublisher::SourceIdFromGuid(TrapObjectId));
    };

    if (TrapState == EGridDungeonTrapState::Disarmed)
    {
        Report(FName(TEXT("Dungeon.Trap.AlreadyDisarmed")),
            NSLOCTEXT("TRAP01", "AlreadyDisarmed", "Ce piège est déjà désamorcé."));
        return true;
    }
    if (TrapState == EGridDungeonTrapState::Disabled)
    {
        Report(FName(TEXT("Dungeon.Trap.AlreadyDisabled")),
            NSLOCTEXT("TRAP01", "AlreadyDisabled", "Ce mécanisme est déjà désactivé."));
        return true;
    }

    const FGridWorldObjectInstance* Instance =
        LevelAsset->FindWorldObjectInstanceById(TrapObjectId);
    const UGridWorldObjectDefinitionAsset* Def = Instance
        ? FindWorldObjectDefinition(Instance->WorldObjectDefinitionId) : nullptr;
    if (!IsValid(Def))
    {
        return false;
    }
    if (TrapState == EGridDungeonTrapState::Triggered && Def->DungeonTrap.bOneShot)
    {
        Report(FName(TEXT("Dungeon.Trap.Spent")),
            NSLOCTEXT("TRAP01", "AlreadyTriggeredOneShot",
                "Ce piège s'est déjà déclenché : il est inactif et ne nécessite aucun désamorçage."));
        return true;
    }
    if (!Def->bCanBeDisarmed)
    {
        Report(FName(TEXT("Dungeon.Trap.NotDisarmable")),
            NSLOCTEXT("TRAP01", "TrapNotDisarmable",
                "Ce mécanisme ne peut pas être désamorcé manuellement."));
        return true;
    }

    FGuid ReachableId;
    if (!FindDisarmableTriggerTrapForParty(PartyPawn, ReachableId) ||
        ReachableId != TrapObjectId)
    {
        Report(FName(TEXT("Dungeon.Trap.OutOfReach")),
            NSLOCTEXT("TRAP01", "DisarmOutOfReach",
                "Approchez-vous du piège et faites-lui face pour le désamorcer."));
        return true;
    }

    const UGridTurnManagerComponent* TurnManager =
        FindComponentByClass<UGridTurnManagerComponent>();
    if (IsValid(TurnManager) && TurnManager->bCombatActive)
    {
        Report(FName(TEXT("Dungeon.Trap.DisarmInCombat")),
            NSLOCTEXT("TRAP01", "DisarmRequiresExploration",
                "Le désamorçage n'est pas disponible pendant un combat."));
        return true;
    }

    UGridPartyInventoryComponent* Inventory = PartyPawn->PartyInventoryComponent;
    if (!IsValid(Inventory))
    {
        return true;
    }
    const URPGSkillAsset* Skill = LoadObject<URPGSkillAsset>(nullptr,
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/Skills/DA_Skill_Traps.DA_Skill_Traps"));
    if (!IsValid(Skill) || !Skill->IsValidDefinition())
    {
        Report(FName(TEXT("Dungeon.Trap.SkillMissing")),
            NSLOCTEXT("TRAP01", "DisarmSkillMissing",
                "La définition de la compétence Pièges est indisponible."));
        return true;
    }

    const FName ToolId(TEXT("Item_DisarmTools_01"));
    const UGridItemDefinitionAsset* Tool = Inventory->FindItemDefinition(ToolId);
    int32 ToolSourceIndex = INDEX_NONE;
    bool bTrainedWithTools = false;
    for (int32 i = 0; i < Inventory->PartyInventoryState.ActiveCharacters.Num(); ++i)
    {
        const FGridCharacterInventoryState& Character =
            Inventory->PartyInventoryState.ActiveCharacters[i];
        if (Character.Resources.CurrentHealth <= 0 ||
            Inventory->CountItemDefinitionInCharacterInventory(i, ToolId) < 1)
        {
            continue;
        }
        ToolSourceIndex = i;
        if (FRPGSkillService::GetSkillRank(Character, Skill->SkillId) > 0)
        {
            bTrainedWithTools = true;
        }
    }
    if (ToolSourceIndex == INDEX_NONE || !IsValid(Tool) ||
        !Tool->ItemTags.Contains(Def->DungeonTrap.RequiredToolTag))
    {
        Report(FName(TEXT("Dungeon.Trap.ToolsMissing")),
            NSLOCTEXT("TRAP01", "DisarmNeedsTools",
                "Aucun membre du groupe ne possède une trousse de désamorçage utilisable."));
        return true;
    }
    if (!bTrainedWithTools && !Skill->bAllowUntrainedChecks)
    {
        Report(FName(TEXT("Dungeon.Trap.TrainingMissing")),
            NSLOCTEXT("TRAP01", "DisarmNeedsTraining",
                "Un personnage vivant doit avoir appris la compétence Pièges et posséder ses outils."));
        return true;
    }

    FRandomStream RandomStream(FMath::Rand());
    FRPGSkillCheckResult Result;
    int32 DisarmerIndex = INDEX_NONE;
    bool bTriggeredOnFailure = false;
    const bool bResolved = TryDisarmTriggerTrap(
        PartyPawn, TrapObjectId, ToolSourceIndex, Skill, RandomStream,
        Result, &DisarmerIndex, &bTriggeredOnFailure);
    if (!bResolved)
    {
        Report(FName(TEXT("Dungeon.Trap.DisarmRejected")),
            NSLOCTEXT("TRAP01", "DisarmRejectedWorld",
                "Le désamorçage a été refusé. Vérifiez la position, la compétence et les outils."));
        return true;
    }

    const FText Name = Inventory->PartyInventoryState.ActiveCharacters.IsValidIndex(DisarmerIndex)
        ? Inventory->PartyInventoryState.ActiveCharacters[DisarmerIndex].DisplayName
        : NSLOCTEXT("TRAP01", "DisarmerFallback", "Un membre du groupe");
    const FText Message = GridDungeonTrapDisarmFeedback::Format(Name, Result, bTriggeredOnFailure);
    Report(Result.bSuccess ? FName(TEXT("Dungeon.Trap.Disarmed"))
                           : FName(TEXT("Dungeon.Trap.DisarmFailed")), Message);
    return true;
}

bool AGridLevelRuntimeActor::FindDisarmableTriggerTrapForParty(
    const AGrimrockPartyPawn* PartyPawn, FGuid& OutTrapObjectId) const
{
    OutTrapObjectId.Invalidate();
    if (!IsValid(PartyPawn) || PartyPawn->LevelRuntimeActor != this || !IsValid(LevelAsset))
    {
        return false;
    }

    const FIntPoint PartyCell(PartyPawn->CurrentCellX, PartyPawn->CurrentCellY);
    FIntPoint FrontCell(INDEX_NONE, INDEX_NONE);
    int32 FrontX = INDEX_NONE;
    int32 FrontY = INDEX_NONE;
    if (TryGetNeighborCell(PartyCell.X, PartyCell.Y, PartyPawn->Facing, FrontX, FrontY) &&
        CanMove(PartyCell.X, PartyCell.Y, PartyPawn->Facing))
    {
        FrontCell = FIntPoint(FrontX, FrontY);
    }

    // Prefer the trigger in the party cell, then the next traversable cell.
    // Two targets in the same cell are ambiguous: never choose randomly.
    for (const FIntPoint CandidateCell : { PartyCell, FrontCell })
    {
        if (CandidateCell.X == INDEX_NONE)
        {
            continue;
        }
        FGuid Candidate;
        for (const FGridWorldObjectInstance& Object : LevelAsset->WorldObjectInstances)
        {
            if (Object.Type != EGridLevelObjectType::Trigger ||
                Object.CellX != CandidateCell.X || Object.CellY != CandidateCell.Y)
            {
                continue;
            }
            int32 Difficulty = 0;
            if (!GetTriggerTrapDisarmDifficulty(Object.InstanceId, Difficulty))
            {
                continue;
            }
            if (Candidate.IsValid())
            {
                OutTrapObjectId.Invalidate();
                return false;
            }
            Candidate = Object.InstanceId;
        }
        if (Candidate.IsValid())
        {
            OutTrapObjectId = Candidate;
            return true;
        }
    }
    return false;
}

bool AGridLevelRuntimeActor::GetTriggerTrapDisarmDifficulty(
    FGuid TrapObjectId, int32& OutDifficulty) const
{
    OutDifficulty = 0;
    if (!TrapObjectId.IsValid() || !IsValid(LevelAsset) ||
        !IsTriggerTrapDetected(TrapObjectId) || IsTriggerTrapDisarmed(TrapObjectId))
    {
        return false;
    }

    const FGridWorldObjectInstance* Trap =
        LevelAsset->FindWorldObjectInstanceById(TrapObjectId);
    const UGridWorldObjectDefinitionAsset* Definition = Trap
        ? FindWorldObjectDefinition(Trap->WorldObjectDefinitionId) : nullptr;

    const int32 EffectiveDifficulty = Trap && IsValid(Definition)
        ? GridDungeonTrapInstance::ResolveDisarmDifficulty(*Trap, *Definition) : 0;
    if (!Trap || Trap->Type != EGridLevelObjectType::Trigger ||
        !IsValid(Definition) || !Definition->bCanBeDisarmed ||
        Definition->SupportedType != EGridLevelObjectType::Trigger ||
        !Definition->DungeonTrap.bIsDungeonTrap ||
        !GridDungeonTrapInstance::Resolve(*Trap, *Definition).IsValidContract() ||
        EffectiveDifficulty < 1 || EffectiveDifficulty > 40 ||
        GetDungeonTrapState(TrapObjectId) == EGridDungeonTrapState::Disabled ||
        (Definition->DungeonTrap.bOneShot &&
         GetDungeonTrapState(TrapObjectId) == EGridDungeonTrapState::Triggered))
    {
        return false;
    }
    OutDifficulty = EffectiveDifficulty;
    return true;
}

bool AGridLevelRuntimeActor::TryDisarmTriggerTrap(
    AGrimrockPartyPawn* PartyPawn, FGuid TrapObjectId,
    int32 CharacterIndex, const URPGSkillAsset* SkillDefinition,
    FRandomStream& RandomStream, FRPGSkillCheckResult& OutResult,
    int32* OutDisarmerCharacterIndex, bool* OutTriggeredOnFailure)
{
    if (OutTriggeredOnFailure)
    {
        *OutTriggeredOnFailure = false;
    }
    if (OutDisarmerCharacterIndex)
    {
        *OutDisarmerCharacterIndex = INDEX_NONE;
    }
    OutResult = FRPGSkillCheckResult();

    int32 Difficulty = 0;
    if (!GetTriggerTrapDisarmDifficulty(TrapObjectId, Difficulty) ||
        !IsValid(PartyPawn) || PartyPawn->LevelRuntimeActor != this ||
        !IsValid(PartyPawn->PartyInventoryComponent) ||
        !IsValid(SkillDefinition) ||
        SkillDefinition->SkillId != FName(TEXT("Skill_Traps")))
    {
        return false;
    }

    const FGridWorldObjectInstance* Target =
        LevelAsset->FindWorldObjectInstanceById(TrapObjectId);
    const FIntPoint PartyCell(PartyPawn->CurrentCellX, PartyPawn->CurrentCellY);
    const FIntPoint TargetCell(Target->CellX, Target->CellY);
    // The player may disarm a trap directly underfoot or immediately ahead.
    // A wall, closed door or non-walkable destination prevents reaching
    // a trap in the adjacent cell (same grid collision authority as movement).
    if (PartyCell != TargetCell)
    {
        int32 FrontX = INDEX_NONE;
        int32 FrontY = INDEX_NONE;
        if (!TryGetNeighborCell(PartyCell.X, PartyCell.Y, PartyPawn->Facing, FrontX, FrontY) ||
            TargetCell != FIntPoint(FrontX, FrontY) ||
            !CanMove(PartyCell.X, PartyCell.Y, PartyPawn->Facing))
        {
            return false;
        }
    }

    UGridPartyInventoryComponent* Inventory = PartyPawn->PartyInventoryComponent;
    const FGridPartyInventoryState& PartyState = Inventory->PartyInventoryState;
    if (!PartyState.ActiveCharacters.IsValidIndex(CharacterIndex) ||
        PartyState.ActiveCharacters[CharacterIndex].Resources.CurrentHealth <= 0 ||
        !IsValid(SkillDefinition) || !SkillDefinition->IsValidDefinition())
    {
        return false;
    }

    const FGridWorldObjectInstance* TrapInstance =
        LevelAsset->FindWorldObjectInstanceById(TrapObjectId);
    const UGridWorldObjectDefinitionAsset* TrapDefinition = TrapInstance
        ? FindWorldObjectDefinition(TrapInstance->WorldObjectDefinitionId) : nullptr;
    if (!IsValid(TrapDefinition) || TrapDefinition->DungeonTrap.RequiredToolTag.IsNone())
    {
        return false;
    }

    // The context source slot must still own a genuine tool. A different
    // trained party member may perform the check only if THEY also carry
    // their own required tool; clicking a tool does not teleport it.
    const FName ToolId(TEXT("Item_DisarmTools_01"));
    const UGridItemDefinitionAsset* Tool = Inventory->FindItemDefinition(ToolId);
    if (!IsValid(Tool) || Tool->ItemDefinitionId != ToolId ||
        !Tool->ItemTags.Contains(TrapDefinition->DungeonTrap.RequiredToolTag) ||
        Inventory->CountItemDefinitionInCharacterInventory(CharacterIndex, ToolId) < 1)
    {
        return false;
    }

    // TRAP01.4 contract: the approved governing attribute is Dexterity.
    // Until the production uasset is regenerated by the canonical Editor
    // authoring task, project the already loaded definition to a transient
    // copy. Never mutate the shared production asset at runtime.
    URPGSkillAsset* EffectiveSkill =
        DuplicateObject<URPGSkillAsset>(SkillDefinition, GetTransientPackage());
    if (!IsValid(EffectiveSkill))
    {
        return false;
    }
    EffectiveSkill->GoverningAttribute = ERPGSkillGoverningAttribute::Dexterity;
    if (!EffectiveSkill->IsValidDefinition())
    {
        return false;
    }

    // Reuse the canonical party-skill resolver, including progression bonuses,
    // by masking characters who lack their own tool from a snapshot.
    // This never changes actual HP, character ranks or the party inventory.
    FGridPartyInventoryState EligibleParty = PartyState;
    for (int32 Index = 0; Index < EligibleParty.ActiveCharacters.Num(); ++Index)
    {
        if (Inventory->CountItemDefinitionInCharacterInventory(Index, ToolId) < 1)
        {
            EligibleParty.ActiveCharacters[Index].Resources.CurrentHealth = 0;
        }
    }
    int32 BestIndex = INDEX_NONE;
    if (!FRPGSkillCheckService::TryResolveBestPartySkillCheck(
        EligibleParty, EffectiveSkill, Difficulty, RandomStream, OutResult, BestIndex))
    {
        return false; // An untrained or tool-less group does not consume RNG.
    }
    if (OutDisarmerCharacterIndex)
    {
        *OutDisarmerCharacterIndex = BestIndex;
    }
    if (!OutResult.bSuccess)
    {
        // Expert Disarm protects only the FIRST near miss on this exact trap.
        // The durable guard is keyed by the level-owned GUID, never by a shared
        // DataAsset or the performing party member.
        if (OutResult.bSafeFailure)
        {
            FGridLevelRuntimeState* State = GetOrCreateRuntimeStateForCurrentLevel();
            if (State && !State->SafeDisarmFailureUsedTrapIds.Contains(TrapObjectId))
            {
                State->SafeDisarmFailureUsedTrapIds.Add(TrapObjectId);
                UE_LOG(LogGridCombatTrapRuntime, Log,
                    TEXT("[TRAP01.8] Safe disarm failure (single use) Trap=%s Margin=%d"),
                    *TrapObjectId.ToString(), OutResult.FailureMargin);
                return true; // Resolved safe failure: stays armed, no activation.
            }
            // The once-per-trap protection has already been spent.
            OutResult.bSafeFailure = false;
        }

        // Unsafe failure springs the authored hazard via exactly the same
        // damage/PIT03/Activated path as stepping on the cell. An adjacent
        // party can trigger the floor ahead without being teleported into it.
        const bool bTriggered = TryActivateDungeonTrap(PartyPawn, TrapObjectId, RandomStream);
        if (OutTriggeredOnFailure)
        {
            *OutTriggeredOnFailure = bTriggered;
        }
        UE_LOG(LogGridCombatTrapRuntime, Log,
            TEXT("[TRAP01.8] Unsafe disarm failure Trap=%s Total=%d DC=%d Triggered=%s"),
            *TrapObjectId.ToString(), OutResult.Total, Difficulty,
            bTriggered ? TEXT("true") : TEXT("false"));
        return true;
    }

    FGridLevelRuntimeState* State = GetOrCreateRuntimeStateForCurrentLevel();
    if (!State || State->DisarmedTriggerTrapIds.Contains(TrapObjectId))
    {
        OutResult = FRPGSkillCheckResult(); // No confirmed commit.
        return false;
    }
    State->DisarmedTriggerTrapIds.Add(TrapObjectId);
    // TRAP01.4.1: the durable GUID guard is committed BEFORE external XP or
    // puzzle callbacks. A replay (including Save/Continue) cannot pay again.
    const int32 RequestedXP = FGridDungeonTrapRewardRules::GetDisarmExperience(Difficulty);
    const int32 AppliedXP = FRPGExperienceRewardService::AwardToActiveParty(Inventory, RequestedXP);
    UE_LOG(LogGridCombatTrapRuntime, Log,
        TEXT("[TRAP01.4.1] DisarmXP Target=%s DC=%d Requested=%d Applied=%d"),
        *TrapObjectId.ToString(), Difficulty, RequestedXP, AppliedXP);
    RefreshDungeonTrapRevealVisual(TrapObjectId);

    // Trigger's normal Activated/Deactivated links are suppressed by the
    // activation component; Disarmed is emitted once for puzzle links.
    ExecuteLinksFromRuntimeObject(TrapObjectId, EGridObjectEvent::Disarmed);
    UE_LOG(LogGridCombatTrapRuntime, Log,
        TEXT("[ARMEMENT01.16.2A] TrapDisarm SUCCEEDED Target=%s SkillTotal=%d DC=%d"),
        *TrapObjectId.ToString(), OutResult.Total, Difficulty);
    return true;
}
