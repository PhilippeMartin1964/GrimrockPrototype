#include "Runtime/GridDungeonTrapPerceptionService.h"

#include "RPG/RPGCharacterRulesLibrary.h"
#include "RPG/RPGSkillAsset.h"
#include "RPG/RPGSkillService.h"
#include "Runtime/GridInventoryTypes.h"

int32 FGridDungeonTrapPerceptionService::SelectBestObserver(
    const TArray<FGridCharacterInventoryState>& Characters,
    const URPGSkillAsset* Skill, int32& OutEffectiveScore)
{
    OutEffectiveScore = 0;
    if (!IsValid(Skill) || !Skill->IsValidDefinition() ||
        Skill->SkillId != FName(TEXT("Skill_Perception")) ||
        Skill->GoverningAttribute != ERPGSkillGoverningAttribute::Wisdom)
    {
        return INDEX_NONE;
    }

    int32 Winner = INDEX_NONE;
    int32 WinnerScore = MIN_int32;
    int32 WinnerRank = MIN_int32;
    for (int32 i = 0; i < Characters.Num(); ++i)
    {
        const FGridCharacterInventoryState& Hero = Characters[i];
        if (Hero.Resources.CurrentHealth <= 0 || !FRPGSkillService::ValidateSkillState(Hero))
        {
            continue;
        }
        const int32 Rank = FRPGSkillService::GetSkillRank(Hero, Skill->SkillId);
        if (Rank == 0 && !Skill->bAllowUntrainedChecks)
        {
            continue;
        }
        const int32 Score = Rank + URPGCharacterRulesLibrary::GetAttributeModifier(Hero.Attributes.Wisdom);
        if (Winner == INDEX_NONE || Score > WinnerScore ||
            (Score == WinnerScore && Rank > WinnerRank))
        {
            Winner = i;
            WinnerScore = Score;
            WinnerRank = Rank;
        }
    }
    if (Winner != INDEX_NONE)
    {
        OutEffectiveScore = WinnerScore;
    }
    return Winner;
}

int32 FGridDungeonTrapPerceptionService::GetMaxRangeCells(
    int32 Score, bool bHasLight, EGridDungeonTrapDetectionSense Sense, int32 DefinitionRangeCap)
{
    const int32 Base = Score <= 1 ? 1 : Score <= 3 ? 2 : Score <= 5 ? 3 : 4;
    const bool bRequiresLight = Sense == EGridDungeonTrapDetectionSense::Visual;
    return FMath::Min(FMath::Clamp(DefinitionRangeCap, 1, 4),
        FMath::Max(1, Base - ((bRequiresLight && !bHasLight) ? 1 : 0)));
}

bool FGridDungeonTrapPerceptionService::GetCardinalDirectionAndDistance(
    FIntPoint Origin, FIntPoint Target, EGridEdge& OutDirection, int32& OutDistance)
{
    OutDirection = EGridEdge::None;
    OutDistance = 0;
    const int32 Dx = Target.X - Origin.X;
    const int32 Dy = Target.Y - Origin.Y;
    if ((Dx == 0 && Dy == 0) || (Dx != 0 && Dy != 0))
    {
        return false;
    }
    OutDistance = FMath::Abs(Dx) + FMath::Abs(Dy);
    OutDirection = Dx > 0 ? EGridEdge::East : Dx < 0 ? EGridEdge::West :
        Dy > 0 ? EGridEdge::North : EGridEdge::South;
    return true;
}

FText FGridDungeonTrapPerceptionService::FormatRelativeLocation(
    EGridEdge Facing, EGridEdge AbsoluteDirection, int32 Distance,
    EGridObjectPlacementKind Surface)
{
    const int32 From = Facing == EGridEdge::North ? 0 : Facing == EGridEdge::East ? 1 :
        Facing == EGridEdge::South ? 2 : Facing == EGridEdge::West ? 3 : INDEX_NONE;
    const int32 To = AbsoluteDirection == EGridEdge::North ? 0 :
        AbsoluteDirection == EGridEdge::East ? 1 : AbsoluteDirection == EGridEdge::South ? 2 :
        AbsoluteDirection == EGridEdge::West ? 3 : INDEX_NONE;
    FText Orientation = NSLOCTEXT("TRAP01", "TrapNearby", "près du groupe");
    if (From != INDEX_NONE && To != INDEX_NONE)
    {
        switch ((To - From + 4) % 4)
        {
            case 0: Orientation = NSLOCTEXT("TRAP01", "TrapAhead", "devant vous"); break;
            case 1: Orientation = NSLOCTEXT("TRAP01", "TrapRight", "sur votre droite"); break;
            case 2: Orientation = NSLOCTEXT("TRAP01", "TrapBehind", "derrière vous"); break;
            case 3: Orientation = NSLOCTEXT("TRAP01", "TrapLeft", "sur votre gauche"); break;
        }
    }
    const FText SurfaceText = Surface == EGridObjectPlacementKind::Wall
        ? NSLOCTEXT("TRAP01", "TrapOnWall", "sur un mur")
        : Surface == EGridObjectPlacementKind::Ceiling
        ? NSLOCTEXT("TRAP01", "TrapOnCeiling", "au plafond")
        : NSLOCTEXT("TRAP01", "TrapOnFloor", "au sol");
    return FText::Format(
        NSLOCTEXT("TRAP01", "TrapRelativeLocation", "{0}, à {1} {2}, {3}"),
        SurfaceText, FText::AsNumber(FMath::Max(0, Distance)),
        FMath::Max(0, Distance) == 1
            ? NSLOCTEXT("TRAP01", "OneCell", "case")
            : NSLOCTEXT("TRAP01", "SeveralCells", "cases"),
        Orientation);
}
