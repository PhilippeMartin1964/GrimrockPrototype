#include "RPG/RPGSkillCheckService.h"

#include "RPG/RPGCharacterRulesLibrary.h"
#include "RPG/RPGSkillAsset.h"
#include "RPG/RPGSkillService.h"
#include "Runtime/GridInventoryTypes.h"

int32 FRPGSkillCheckService::GetGoverningAttributeValue (
    const FRPGAttributes& Attributes,
    ERPGSkillGoverningAttribute GoverningAttribute)
{
    switch (GoverningAttribute)
    {
    case ERPGSkillGoverningAttribute::Strength:
        return Attributes.Strength;
    case ERPGSkillGoverningAttribute::Dexterity:
        return Attributes.Dexterity;
    case ERPGSkillGoverningAttribute::Constitution:
        return Attributes.Constitution;
    case ERPGSkillGoverningAttribute::Intelligence:
        return Attributes.Intelligence;
    case ERPGSkillGoverningAttribute::Wisdom:
        return Attributes.Wisdom;
    case ERPGSkillGoverningAttribute::Charisma:
        return Attributes.Charisma;
    case ERPGSkillGoverningAttribute::None:
    default:
        return 0;
    }
}

bool FRPGSkillCheckService::TryResolveSkillCheck (
    const FGridCharacterInventoryState& CharacterState,
    const URPGSkillAsset* SkillDefinition,
    int32 Difficulty,
    FRandomStream& RandomStream,
    FRPGSkillCheckResult& OutResult)
{
    OutResult = FRPGSkillCheckResult ();

    if (!IsValid (SkillDefinition) || !SkillDefinition->IsValidDefinition ())
    {
        OutResult.RejectReason = ERPGSkillCheckRejectReason::InvalidDefinition;
        return false;
    }

    OutResult.SkillId = SkillDefinition->SkillId;
    OutResult.GoverningAttribute = SkillDefinition->GoverningAttribute;
    OutResult.Difficulty = Difficulty;

    if (Difficulty <= 0)
    {
        OutResult.RejectReason = ERPGSkillCheckRejectReason::InvalidDifficulty;
        return false;
    }

    if (!FRPGSkillService::ValidateSkillState (CharacterState))
    {
        OutResult.RejectReason =
            ERPGSkillCheckRejectReason::InvalidCharacterState;
        return false;
    }

    const int32 Rank = FRPGSkillService::GetSkillRank (
        CharacterState,
        SkillDefinition->SkillId);
    OutResult.Rank = Rank;

    if (Rank > SkillDefinition->MaxRank)
    {
        OutResult.RejectReason =
            ERPGSkillCheckRejectReason::InvalidCharacterState;
        return false;
    }

    if (Rank == 0 && !SkillDefinition->bAllowUntrainedChecks)
    {
        OutResult.RejectReason =
            ERPGSkillCheckRejectReason::UntrainedNotAllowed;
        return false;
    }

    OutResult.AttributeValue = GetGoverningAttributeValue (
        CharacterState.Attributes,
        SkillDefinition->GoverningAttribute);
    OutResult.AttributeModifier =
        SkillDefinition->GoverningAttribute ==
                ERPGSkillGoverningAttribute::None
            ? 0
            : URPGCharacterRulesLibrary::GetAttributeModifier (
                OutResult.AttributeValue);

    OutResult.Roll = RandomStream.RandRange (1, 20);
    OutResult.Total =
        OutResult.Roll + OutResult.Rank + OutResult.AttributeModifier;
    OutResult.bResolved = true;
    OutResult.bSuccess = OutResult.Total >= OutResult.Difficulty;
    return true;
}
