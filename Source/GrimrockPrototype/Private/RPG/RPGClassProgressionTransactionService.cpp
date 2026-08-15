#include "RPG/RPGClassProgressionTransactionService.h"
#include "RPG/RPGClassAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"
DEFINE_LOG_CATEGORY_STATIC (LogGridClassProgression, Log, All);
namespace
{
struct FRuntimeClassProgressionState
{
TWeakObjectPtr<UGridPartyInventoryComponent> InventoryComponent;
int32 CharacterIndex = INDEX_NONE;
FGuid CharacterId;
FName ClassId = NAME_None;
int32 CharacterLevel = 1;
TArray<FName> SelectedChoiceIds;
TSet<FName> SatisfiedRequirements;
};
TMap<FGuid, FRuntimeClassProgressionState> RuntimeStates;
URPGClassAsset* ResolveClassDefinition (
FGridCharacterInventoryState& Character)
{
URPGClassAsset* ClassDefinition = Character.ClassDefinition.Get ();
if (!ClassDefinition && !Character.ClassDefinition.IsNull ())
{
ClassDefinition = Character.ClassDefinition.LoadSynchronous ();
}
if (!IsValid (ClassDefinition) ||
!ClassDefinition->IsValidDefinition () ||
(!Character.ClassId.IsNone () &&
ClassDefinition->ClassId != Character.ClassId))
{
return nullptr;
}
return ClassDefinition;
}
bool BuildSelectionSet (
const TArray<FName>& ChoiceIds,
TSet<FName>& OutChoiceIds)
{
OutChoiceIds.Reset ();
for (const FName ChoiceId : ChoiceIds)
{
if (ChoiceId.IsNone () || OutChoiceIds.Contains (ChoiceId))
{
OutChoiceIds.Reset ();
return false;
}
OutChoiceIds.Add (ChoiceId);
}
return true;
}
void NormalizeSelectionOrder (
const URPGClassAsset& ClassDefinition,
const TSet<FName>& SelectedChoiceIds,
TArray<FName>& OutSelectedChoiceIds)
{
OutSelectedChoiceIds.Reset (SelectedChoiceIds.Num ());
for (const FRPGClassProgressionChoiceDefinition& Choice :
ClassDefinition.ProgressionChoices)
{
if (SelectedChoiceIds.Contains (Choice.ChoiceId))
{
OutSelectedChoiceIds.Add (Choice.ChoiceId);
}
}
}
FRuntimeClassProgressionState* FindCompatibleRuntimeState (
UGridPartyInventoryComponent* PartyInventoryComponent,
const FGridCharacterInventoryState& Character)
{
FRuntimeClassProgressionState* State =
RuntimeStates.Find (Character.CharacterId);
if (!State)
{
return nullptr;
}
if (!State->InventoryComponent.IsValid () ||
State->InventoryComponent.Get () != PartyInventoryComponent ||
State->CharacterId != Character.CharacterId ||
State->ClassId != Character.ClassId)
{
RuntimeStates.Remove (Character.CharacterId);
return nullptr;
}
return State;
}
bool ResolveCharacter (
UGridPartyInventoryComponent* PartyInventoryComponent,
int32 CharacterIndex,
FGridCharacterInventoryState*& OutCharacter,
URPGClassAsset*& OutClassDefinition)
{
OutCharacter = nullptr;
OutClassDefinition = nullptr;
if (!IsValid (PartyInventoryComponent) ||
!PartyInventoryComponent->IsValidCharacterIndex (CharacterIndex))
{
return false;
}
FGridCharacterInventoryState& Character =
PartyInventoryComponent->PartyInventoryState.ActiveCharacters[
CharacterIndex];
if (!Character.CharacterId.IsValid ())
{
return false;
}
URPGClassAsset* ClassDefinition = ResolveClassDefinition (Character);
if (!ClassDefinition)
{
return false;
}
OutCharacter = &Character;
OutClassDefinition = ClassDefinition;
return true;
}
ERPGClassProgressionCommitRejectReason DiagnoseCandidateFailure (
const URPGClassAsset& ClassDefinition,
int32 CharacterLevel,
int32 GrantedPoints,
const TSet<FName>& CandidateSelection,
const TArray<FName>& RequestedChoiceIds)
{
int32 TotalCost = 0;
for (const FName SelectedChoiceId : CandidateSelection)
{
const FRPGClassProgressionChoiceDefinition* SelectedChoice =
ClassDefinition.FindProgressionChoice (SelectedChoiceId);
if (!SelectedChoice)
{
return ERPGClassProgressionCommitRejectReason::UnknownChoice;
}
if (CharacterLevel < SelectedChoice->MinimumLevel)
{
return ERPGClassProgressionCommitRejectReason::LevelTooLow;
}
for (const FName PrerequisiteId :
SelectedChoice->PrerequisiteChoiceIds)
{
if (!CandidateSelection.Contains (PrerequisiteId))
{
return ERPGClassProgressionCommitRejectReason::
MissingPrerequisite;
}
}
TotalCost += SelectedChoice->PointCost;
}
if (TotalCost > GrantedPoints)
{
return ERPGClassProgressionCommitRejectReason::
InsufficientChoicePoints;
}
for (const FName RequestedChoiceId : RequestedChoiceIds)
{
if (!ClassDefinition.FindProgressionChoice (RequestedChoiceId))
{
return ERPGClassProgressionCommitRejectReason::UnknownChoice;
}
}
return ERPGClassProgressionCommitRejectReason::InvalidCurrentSelection;
}
}
FRPGClassProgressionCommittedNativeSignature&
FRPGClassProgressionTransactionService::OnClassProgressionCommitted ()
{
static FRPGClassProgressionCommittedNativeSignature Delegate;
return Delegate;
}
bool FRPGClassProgressionTransactionService::RefreshCharacterProjection (
UGridPartyInventoryComponent* PartyInventoryComponent,
int32 CharacterIndex)
{
FGridCharacterInventoryState* Character = nullptr;
URPGClassAsset* ClassDefinition = nullptr;
if (!ResolveCharacter (
PartyInventoryComponent,
CharacterIndex,
Character,
ClassDefinition))
{
return false;
}
TArray<FName> ExistingChoices;
if (FRuntimeClassProgressionState* ExistingState =
FindCompatibleRuntimeState (
PartyInventoryComponent,
*Character))
{
ExistingChoices = ExistingState->SelectedChoiceIds;
}
TSet<FName> SelectedChoiceIds;
if (!BuildSelectionSet (ExistingChoices, SelectedChoiceIds))
{
return false;
}
int32 GrantedPoints = 0;
int32 SpentPoints = 0;
int32 RemainingPoints = 0;
if (!FRPGClassProgressionService::TryGetChoicePointBalance (
ClassDefinition,
Character->Level,
SelectedChoiceIds,
GrantedPoints,
SpentPoints,
RemainingPoints))
{
return false;
}
TSet<FName> SatisfiedRequirements;
if (!FRPGClassProgressionService::CollectSatisfiedRequirements (
ClassDefinition,
Character->Level,
SelectedChoiceIds,
SatisfiedRequirements))
{
return false;
}
FRuntimeClassProgressionState& State =
RuntimeStates.FindOrAdd (Character->CharacterId);
State.InventoryComponent = PartyInventoryComponent;
State.CharacterIndex = CharacterIndex;
State.CharacterId = Character->CharacterId;
State.ClassId = Character->ClassId;
State.CharacterLevel = Character->Level;
NormalizeSelectionOrder (
*ClassDefinition,
SelectedChoiceIds,
State.SelectedChoiceIds);
State.SatisfiedRequirements = MoveTemp (SatisfiedRequirements);
return true;
}
bool FRPGClassProgressionTransactionService::TryGetSelectedChoiceIds (
UGridPartyInventoryComponent* PartyInventoryComponent,
int32 CharacterIndex,
TArray<FName>& OutSelectedChoiceIds)
{
OutSelectedChoiceIds.Reset ();
if (!RefreshCharacterProjection (
PartyInventoryComponent,
CharacterIndex))
{
return false;
}
const FGridCharacterInventoryState& Character =
PartyInventoryComponent->PartyInventoryState.ActiveCharacters[
CharacterIndex];
const FRuntimeClassProgressionState* State =
RuntimeStates.Find (Character.CharacterId);
if (!State)
{
return false;
}
OutSelectedChoiceIds = State->SelectedChoiceIds;
return true;
}
bool FRPGClassProgressionTransactionService::TryGetChoicePointBalance (
UGridPartyInventoryComponent* PartyInventoryComponent,
int32 CharacterIndex,
int32& OutGrantedPoints,
int32& OutSpentPoints,
int32& OutRemainingPoints)
{
OutGrantedPoints = 0;
OutSpentPoints = 0;
OutRemainingPoints = 0;
FGridCharacterInventoryState* Character = nullptr;
URPGClassAsset* ClassDefinition = nullptr;
if (!ResolveCharacter (
PartyInventoryComponent,
CharacterIndex,
Character,
ClassDefinition) ||
!RefreshCharacterProjection (
PartyInventoryComponent,
CharacterIndex))
{
return false;
}
const FRuntimeClassProgressionState* State =
RuntimeStates.Find (Character->CharacterId);
if (!State)
{
return false;
}
TSet<FName> SelectedChoiceIds;
if (!BuildSelectionSet (
State->SelectedChoiceIds,
SelectedChoiceIds))
{
return false;
}
return FRPGClassProgressionService::TryGetChoicePointBalance (
ClassDefinition,
Character->Level,
SelectedChoiceIds,
OutGrantedPoints,
OutSpentPoints,
OutRemainingPoints);
}
bool FRPGClassProgressionTransactionService::TryCommitChoices (
UGridPartyInventoryComponent* PartyInventoryComponent,
int32 CharacterIndex,
const TArray<FName>& ChoiceIdsToCommit,
FRPGClassProgressionCommitResult& OutResult)
{
OutResult = FRPGClassProgressionCommitResult ();
if (!IsValid (PartyInventoryComponent))
{
OutResult.RejectReason =
ERPGClassProgressionCommitRejectReason::InvalidInventory;
return false;
}
if (!PartyInventoryComponent->IsValidCharacterIndex (CharacterIndex))
{
OutResult.RejectReason =
ERPGClassProgressionCommitRejectReason::InvalidCharacter;
return false;
}
if (ChoiceIdsToCommit.IsEmpty ())
{
OutResult.RejectReason =
ERPGClassProgressionCommitRejectReason::EmptyRequest;
return false;
}
FGridCharacterInventoryState* Character = nullptr;
URPGClassAsset* ClassDefinition = nullptr;
if (!ResolveCharacter (
PartyInventoryComponent,
CharacterIndex,
Character,
ClassDefinition))
{
OutResult.RejectReason =
ERPGClassProgressionCommitRejectReason::InvalidClassDefinition;
return false;
}
if (!RefreshCharacterProjection (
PartyInventoryComponent,
CharacterIndex))
{
OutResult.RejectReason =
ERPGClassProgressionCommitRejectReason::InvalidCurrentSelection;
return false;
}
FRuntimeClassProgressionState* State =
RuntimeStates.Find (Character->CharacterId);
if (!State)
{
OutResult.RejectReason =
ERPGClassProgressionCommitRejectReason::InvalidCurrentSelection;
return false;
}
TSet<FName> CurrentSelection;
if (!BuildSelectionSet (
State->SelectedChoiceIds,
CurrentSelection) ||
!FRPGClassProgressionService::TryGetChoicePointBalance (
ClassDefinition,
Character->Level,
CurrentSelection,
OutResult.GrantedPoints,
OutResult.SpentPointsBefore,
OutResult.RemainingPoints))
{
OutResult.RejectReason =
ERPGClassProgressionCommitRejectReason::InvalidCurrentSelection;
return false;
}
TSet<FName> RequestedUnique;
TSet<FName> CandidateSelection = CurrentSelection;
for (const FName ChoiceId : ChoiceIdsToCommit)
{
if (ChoiceId.IsNone () || RequestedUnique.Contains (ChoiceId))
{
OutResult.RejectReason =
ERPGClassProgressionCommitRejectReason::DuplicateRequest;
return false;
}
RequestedUnique.Add (ChoiceId);
const FRPGClassProgressionChoiceDefinition* Choice =
ClassDefinition->FindProgressionChoice (ChoiceId);
if (!Choice)
{
OutResult.RejectReason =
ERPGClassProgressionCommitRejectReason::UnknownChoice;
return false;
}
if (CurrentSelection.Contains (ChoiceId))
{
OutResult.RejectReason =
ERPGClassProgressionCommitRejectReason::AlreadySelected;
return false;
}
if (Character->Level < Choice->MinimumLevel)
{
OutResult.RejectReason =
ERPGClassProgressionCommitRejectReason::LevelTooLow;
return false;
}
CandidateSelection.Add (ChoiceId);
}
int32 SpentAfter = 0;
int32 RemainingAfter = 0;
int32 GrantedAfter = 0;
if (!FRPGClassProgressionService::TryGetChoicePointBalance (
ClassDefinition,
Character->Level,
CandidateSelection,
GrantedAfter,
SpentAfter,
RemainingAfter))
{
OutResult.RejectReason = DiagnoseCandidateFailure (
*ClassDefinition,
Character->Level,
OutResult.GrantedPoints,
CandidateSelection,
ChoiceIdsToCommit);
return false;
}
TSet<FName> SatisfiedRequirements;
if (!FRPGClassProgressionService::CollectSatisfiedRequirements (
ClassDefinition,
Character->Level,
CandidateSelection,
SatisfiedRequirements))
{
OutResult.RejectReason =
ERPGClassProgressionCommitRejectReason::InvalidCurrentSelection;
return false;
}
NormalizeSelectionOrder (
*ClassDefinition,
CandidateSelection,
State->SelectedChoiceIds);
State->CharacterLevel = Character->Level;
State->SatisfiedRequirements = MoveTemp (SatisfiedRequirements);
OutResult.bCommitted = true;
OutResult.RejectReason = ERPGClassProgressionCommitRejectReason::None;
OutResult.GrantedPoints = GrantedAfter;
OutResult.SpentPointsAfter = SpentAfter;
OutResult.RemainingPoints = RemainingAfter;
OutResult.CommittedChoiceIds = ChoiceIdsToCommit;
PartyInventoryComponent->NotifyPartyInventoryChanged (CharacterIndex);
OnClassProgressionCommitted ().Broadcast (
PartyInventoryComponent,
CharacterIndex,
OutResult.CommittedChoiceIds,
OutResult.RemainingPoints);
UE_LOG (
LogGridClassProgression,
Log,
TEXT ("[GridClassProgression] Character=%d CharacterId=%s Level=%d Committed=%d Granted=%d Spent=%d Remaining=%d"),
CharacterIndex,
*Character->CharacterId.ToString (EGuidFormats::Digits),
Character->Level,
OutResult.CommittedChoiceIds.Num (),
OutResult.GrantedPoints,
OutResult.SpentPointsAfter,
OutResult.RemainingPoints);
return true;
}
void FRPGClassProgressionTransactionService::AppendRuntimeSatisfiedRequirements (
const FGuid& CharacterId,
TSet<FName>& InOutSatisfiedRequirements)
{
if (!CharacterId.IsValid ())
{
return;
}
FRuntimeClassProgressionState* State = RuntimeStates.Find (CharacterId);
if (!State)
{
return;
}
if (!State->InventoryComponent.IsValid ())
{
RuntimeStates.Remove (CharacterId);
return;
}
for (const FName RequirementId : State->SatisfiedRequirements)
{
InOutSatisfiedRequirements.Add (RequirementId);
}
}
void FRPGClassProgressionTransactionService::ResetRuntimeState (
UGridPartyInventoryComponent* PartyInventoryComponent)
{
if (!PartyInventoryComponent)
{
RuntimeStates.Reset ();
return;
}
for (auto It = RuntimeStates.CreateIterator (); It; ++It)
{
if (!It.Value ().InventoryComponent.IsValid () ||
It.Value ().InventoryComponent.Get () == PartyInventoryComponent)
{
It.RemoveCurrent ();
}
}
}