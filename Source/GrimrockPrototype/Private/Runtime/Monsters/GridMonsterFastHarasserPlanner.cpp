#include "Runtime/Monsters/GridMonsterFastHarasserPlanner.h"

#include "Runtime/Monsters/GridMonsterPathfinder.h"

int32 FGridFastHarasserPlanner::ScoreCandidate(const FGridRetreatCandidate& Candidate, int32 CurrentDistanceToParty)
{
	const int32 DistanceDelta = Candidate.DistanceToParty - CurrentDistanceToParty;
	return DistanceDelta * 100 + Candidate.ExitCount * 10 + (Candidate.bCanContinuePursuit ? 5 : 0) - (Candidate.bIsCulDeSac ? 60 : 0);
}

bool FGridFastHarasserPlanner::SelectBestRetreatCell(
	const TArray<FGridRetreatCandidate>& Candidates, int32 CurrentDistanceToParty, FGridRetreatDecision& OutDecision)
{
	OutDecision = FGridRetreatDecision();

	int32 BestStableOrder = MAX_int32;
	for (const FGridRetreatCandidate& Candidate : Candidates)
	{
		const int32 CandidateScore = ScoreCandidate(Candidate, CurrentDistanceToParty);
		if (!OutDecision.bHasRetreatCell || CandidateScore > OutDecision.Score ||
			(CandidateScore == OutDecision.Score && Candidate.StableDirectionOrder < BestStableOrder))
		{
			OutDecision.bHasRetreatCell = true;
			OutDecision.RetreatCell = Candidate.Cell;
			OutDecision.RetreatDirection = Candidate.Direction;
			OutDecision.Score = CandidateScore;
			BestStableOrder = Candidate.StableDirectionOrder;
		}
	}

	return OutDecision.bHasRetreatCell;
}

bool FGridFastHarasserPlanner::ShouldRetreat(float RetreatChance, FRandomStream& RandomStream, float* OutRoll)
{
	const float Roll = RandomStream.FRand();
	if (OutRoll)
	{
		*OutRoll = Roll;
	}
	return Roll < FMath::Clamp(RetreatChance, 0.0f, 1.0f);
}

void FGridFastHarasserPlanner::SelectAggroTargets(const FGuid& SourceObjectId, FName SourceMonsterId, FName EncounterGroupId, const FIntPoint& SourceCell,
	int32 AggroPropagationRange, const TArray<FGridMonsterAggroCandidate>& Candidates, TArray<FGuid>& OutTargetIds)
{
	OutTargetIds.Reset();
	if (!SourceObjectId.IsValid() || SourceMonsterId.IsNone() || EncounterGroupId.IsNone() || AggroPropagationRange < 0)
	{
		return;
	}

	TArray<const FGridMonsterAggroCandidate*> OrderedCandidates;
	OrderedCandidates.Reserve(Candidates.Num());
	for (const FGridMonsterAggroCandidate& Candidate : Candidates)
	{
		if (!Candidate.SpawnObjectId.IsValid() || Candidate.SpawnObjectId == SourceObjectId || Candidate.MonsterId != SourceMonsterId ||
			Candidate.EncounterGroupId != EncounterGroupId || !Candidate.bIsAlive || !Candidate.bIsEnabled ||
			FGridMonsterPathfinder::ManhattanDistance(SourceCell, Candidate.Cell) > AggroPropagationRange)
		{
			continue;
		}
		OrderedCandidates.Add(&Candidate);
	}

	OrderedCandidates.Sort(
		[](const FGridMonsterAggroCandidate& Left, const FGridMonsterAggroCandidate& Right)
		{
			return Left.SpawnObjectId.ToString(EGuidFormats::Digits) < Right.SpawnObjectId.ToString(EGuidFormats::Digits);
		});

	TSet<FGuid> AddedIds;
	for (const FGridMonsterAggroCandidate* Candidate : OrderedCandidates)
	{
		if (Candidate && !AddedIds.Contains(Candidate->SpawnObjectId))
		{
			AddedIds.Add(Candidate->SpawnObjectId);
			OutTargetIds.Add(Candidate->SpawnObjectId);
		}
	}
}
