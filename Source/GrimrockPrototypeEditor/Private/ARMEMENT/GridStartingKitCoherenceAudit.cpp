#include "ARMEMENT/GridStartingKitCoherenceAudit.h"

#include "RPG/RPGClassAsset.h"
#include "RPG/RPGStartingEquipmentPresetCatalog.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "UI/RPGSkillPresentationAsset.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/UnrealType.h"
#include <initializer_list>

namespace StartingKitAudit
{
    constexpr int32 ExpectedClasses = 6;
    constexpr int32 ExpectedKits = 12;
    constexpr int32 ExpectedBranches = 18;
    constexpr int32 ExpectedSkills = 25;

    const TCHAR* ClassNames[] =
    {
        TEXT("Warrior"), TEXT("Rogue"), TEXT("Ranger"),
        TEXT("Mage"), TEXT("Priest"), TEXT("Alchemist")
    };

    const TCHAR* SkillCatalogPath =
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/UI/RPG/DA_RPGSkillPresentation.DA_RPGSkillPresentation");

    /** REVIEW CANDIDATES only: these are not unlocks, prerequisites or final design decisions.
     * Empty branch means current assets cannot justify linking the kit to a talent branch.
     */
    struct FReviewProposal
    {
        const TCHAR* KitId;
        const TCHAR* BranchId;
        const TCHAR* RelatedSkillId;
    };

    const FReviewProposal Proposals[] =
    {
        {TEXT("Warrior_Defender"), TEXT("Guardian"), TEXT("Skill_Shield")},
        {TEXT("Warrior_Skirmisher"), TEXT(""), TEXT("Skill_Throwing")},
        {TEXT("Rogue_Shadow"), TEXT("Shadow"), TEXT("Skill_Stealth")},
        {TEXT("Rogue_Explorer"), TEXT(""), TEXT("Skill_Perception")},
        {TEXT("Ranger_Archer"), TEXT("Marksman"), TEXT("Skill_RangedWeapons")},
        {TEXT("Ranger_Pathfinder"), TEXT("Scout"), TEXT("Skill_Survival")},
        {TEXT("Mage_Scholar"), TEXT("Arcanist"), TEXT("Skill_Arcana")},
        {TEXT("Mage_Wanderer"), TEXT(""), TEXT("Skill_Arcana")},
        {TEXT("Priest_Pilgrim"), TEXT(""), TEXT("Skill_Religion")},
        {TEXT("Priest_Protector"), TEXT("Protection"), TEXT("Skill_Shield")},
        {TEXT("Alchemist_Apothecary"), TEXT("Apothecary"), TEXT("Skill_Alchemy")},
        {TEXT("Alchemist_Firestarter"), TEXT("Grenadier"), TEXT("Skill_Alchemy")}
    };

    FString CsvEscape(FString Value)
    {
        // One physical line per record for easy reviews and diffing.
        Value.ReplaceInline(TEXT("\r"), TEXT(" "));
        Value.ReplaceInline(TEXT("\n"), TEXT(" "));
        Value.ReplaceInline(TEXT("\""), TEXT("\"\""));
        return FString::Printf(TEXT("\"%s\""), *Value);
    }

    FString CsvRow(std::initializer_list<FString> Columns)
    {
        TArray<FString> Escaped;
        for (const FString& Column : Columns)
        {
            Escaped.Add(CsvEscape(Column));
        }
        return FString::Join(Escaped, TEXT(",")) + TEXT("\n");
    }

    FString AffinityName(EGridSkillClassAffinity Affinity)
    {
        switch (Affinity)
        {
        case EGridSkillClassAffinity::Recommended: return TEXT("Recommended");
        case EGridSkillClassAffinity::Useful: return TEXT("Useful");
        case EGridSkillClassAffinity::Discouraged: return TEXT("Discouraged");
        case EGridSkillClassAffinity::Neutral:
        default: return TEXT("Neutral");
        }
    }

    const FReviewProposal* FindProposal(FName KitId)
    {
        for (const FReviewProposal& Proposal : Proposals)
        {
            if (KitId == FName(Proposal.KitId))
            {
                return &Proposal;
            }
        }
        return nullptr;
    }

    URPGClassAsset* LoadClass(const TCHAR* ClassName)
    {
        const FString Path = FString::Printf(
            TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_%s.DA_Class_%s"),
            ClassName, ClassName);
        return LoadObject<URPGClassAsset>(nullptr, *Path);
    }

    bool MaterializeReviewCopy(
        URPGClassAsset* Production, URPGClassAsset*& OutCopy, FString& OutError)
    {
        OutCopy = nullptr;
        if (!IsValid(Production) || !Production->IsValidDefinition() ||
            !FRPGStartingEquipmentPresetCatalog::IsProductionClassAsset(Production))
        {
            OutError = TEXT("Missing or invalid production class.");
            return false;
        }
        OutCopy = DuplicateObject<URPGClassAsset>(Production, GetTransientPackage());
        if (!OutCopy)
        {
            OutError = TEXT("Cannot duplicate production class for read-only audit.");
            return false;
        }
        if (OutCopy->StartingEquipmentKits.IsEmpty() &&
            OutCopy->DefaultStartingEquipmentKitId.IsNone() &&
            !FRPGStartingEquipmentPresetCatalog::TryPopulateTransientClass(*OutCopy, OutError))
        {
            return false;
        }
        if (!OutCopy->IsValidDefinition() || OutCopy->StartingEquipmentKits.Num() != 2)
        {
            OutError = TEXT("Production class has invalid or unexpected starting-kit content.");
            return false;
        }
        return true;
    }

    FString DescribeItems(const FRPGStartingEquipmentKit& Kit, FString& OutError)
    {
        TArray<FString> Descriptions;
        for (const FRPGStartingEquipmentEntry& Entry : Kit.Items)
        {
            const UGridItemDefinitionAsset* Item = Entry.ItemDefinition.Get();
            if (!IsValid(Item) || Item->ItemDefinitionId.IsNone() || Entry.Quantity < 1 ||
                (Entry.TargetSlot != EGridEquipmentSlot::None && !Item->CanEquipToSlot(Entry.TargetSlot)))
            {
                OutError = FString::Printf(TEXT("Invalid starter item in %s."), *Kit.KitId.ToString());
                return {};
            }
            Descriptions.Add(FString::Printf(TEXT("%s x%d @%s [ItemLevel=%d]"),
                *Item->ItemDefinitionId.ToString(), Entry.Quantity,
                *StaticEnum<EGridEquipmentSlot>()->GetNameStringByValue(static_cast<int64>(Entry.TargetSlot)),
                Entry.ItemLevel));
        }
        return FString::Join(Descriptions, TEXT("; "));
    }
}

bool FGridStartingKitCoherenceAudit::BuildProductionReport(
    FGridStartingKitCoherenceAuditReport& OutReport, FString& OutError)
{
    using namespace StartingKitAudit;
    OutReport = FGridStartingKitCoherenceAuditReport();
    OutError.Reset();

    URPGSkillPresentationAsset* SkillCatalog =
        LoadObject<URPGSkillPresentationAsset>(nullptr, SkillCatalogPath);
    if (!IsValid(SkillCatalog) || !SkillCatalog->IsValidDefinition() ||
        SkillCatalog->Skills.Num() != ExpectedSkills)
    {
        OutError = TEXT("Production Skill affinity catalog is absent, invalid, or not 25 Skills.");
        return false;
    }

    // Verify 12 distinct review proposals. They are explicitly separate
    // from data authorities and are never consulted at runtime by the wizard.
    TSet<FName> ProposedKitIds;
    for (const FReviewProposal& Proposal : Proposals)
    {
        const FName Id(Proposal.KitId);
        if (Id.IsNone() || ProposedKitIds.Contains(Id))
        {
            OutError = TEXT("Duplicate or missing review proposal KitId.");
            return false;
        }
        ProposedKitIds.Add(Id);
    }
    if (ProposedKitIds.Num() != ExpectedKits)
    {
        OutError = TEXT("Audit review proposal list must cover exactly 12 kits.");
        return false;
    }

    OutReport.KitMatrixCsv =
        TEXT("ClassId,KitId,IsDefault,ItemCount,ActualItemsAndSlots,ProposedBranchId,BranchExists,FirstTalentAtLevel2,RelatedSkillId,ActualClassSkillAffinity,ReviewStatus\n");
    OutReport.TalentBranchesCsv =
        TEXT("ClassId,BranchId,TalentChoiceCount,MinimumTalentLevel,Level2TalentChoiceIds\n");
    OutReport.SkillAffinitiesCsv =
        TEXT("ClassId,SkillId,ActualClassAffinity\n");

    TSet<FName> SeenKitIds;
    for (const TCHAR* ClassName : ClassNames)
    {
        URPGClassAsset* Production = LoadClass(ClassName);
        URPGClassAsset* Class = nullptr;
        if (!MaterializeReviewCopy(Production, Class, OutError))
        {
            OutError = FString::Printf(TEXT("%s: %s"), ClassName, *OutError);
            return false;
        }
        if (Class->ClassId != FName(ClassName))
        {
            OutError = TEXT("Production class ID does not match path.");
            return false;
        }
        ++OutReport.ClassCount;

        TMap<FName, TArray<const FRPGClassProgressionChoiceDefinition*>> BranchChoices;
        for (const FRPGClassProgressionChoiceDefinition& Choice : Class->ProgressionChoices)
        {
            if (Choice.TalentBranchId.IsNone() || Choice.ChoiceId.IsNone())
            {
                OutError = TEXT("Invalid production talent branch or choice ID.");
                return false;
            }
            BranchChoices.FindOrAdd(Choice.TalentBranchId).Add(&Choice);
        }
        TArray<FName> BranchIds;
        BranchChoices.GetKeys(BranchIds);
        BranchIds.Sort([](const FName& Left, const FName& Right)
        {
            return Left.LexicalLess(Right);
        });
        if (BranchIds.Num() != 3)
        {
            OutError = FString::Printf(TEXT("Expected 3 talent branches for %s."), ClassName);
            return false;
        }
        OutReport.BranchCount += BranchIds.Num();

        for (FName BranchId : BranchIds)
        {
            const TArray<const FRPGClassProgressionChoiceDefinition*>& Choices = BranchChoices[BranchId];
            int32 MinimumLevel = MAX_int32;
            TArray<FString> Level2Choices;
            for (const FRPGClassProgressionChoiceDefinition* Choice : Choices)
            {
                MinimumLevel = FMath::Min(MinimumLevel, Choice->MinimumLevel);
                if (Choice->MinimumLevel == 2)
                {
                    Level2Choices.Add(Choice->ChoiceId.ToString());
                }
            }
            Level2Choices.Sort();
            OutReport.TalentBranchesCsv += CsvRow({
                Class->ClassId.ToString(), BranchId.ToString(),
                FString::FromInt(Choices.Num()), FString::FromInt(MinimumLevel),
                FString::Join(Level2Choices, TEXT(";"))
            });
        }

        TArray<const FRPGSkillPresentationDefinition*> Skills;
        for (const FRPGSkillPresentationDefinition& Skill : SkillCatalog->Skills)
        {
            Skills.Add(&Skill);
        }
        Skills.Sort([](const FRPGSkillPresentationDefinition& A, const FRPGSkillPresentationDefinition& B)
        {
            return A.SkillId.LexicalLess(B.SkillId);
        });
        for (const FRPGSkillPresentationDefinition* Skill : Skills)
        {
            OutReport.SkillAffinitiesCsv += CsvRow({
                Class->ClassId.ToString(), Skill->SkillId.ToString(),
                AffinityName(SkillCatalog->ResolveClassAffinity(Skill->SkillId, Class->ClassId))
            });
            ++OutReport.AffinityCount;
        }

        for (const FRPGStartingEquipmentKit& Kit : Class->StartingEquipmentKits)
        {
            if (Kit.KitId.IsNone() || SeenKitIds.Contains(Kit.KitId) || Kit.Items.Num() != 2)
            {
                OutError = TEXT("Invalid, duplicated, or incomplete starting kit.");
                return false;
            }
            SeenKitIds.Add(Kit.KitId);
            const FReviewProposal* Proposal = FindProposal(Kit.KitId);
            if (!Proposal)
            {
                OutError = FString::Printf(TEXT("Kit %s has no review entry."), *Kit.KitId.ToString());
                return false;
            }
            FString ItemError;
            const FString ActualItems = DescribeItems(Kit, ItemError);
            if (!ItemError.IsEmpty())
            {
                OutError = ItemError;
                return false;
            }

            const FName SuggestedBranch(Proposal->BranchId);
            const FName RelatedSkill(Proposal->RelatedSkillId);
            if (!SkillCatalog->FindSkill(RelatedSkill))
            {
                OutError = FString::Printf(TEXT("Proposed skill %s not in production catalog."), *RelatedSkill.ToString());
                return false;
            }
            FString BranchValid = TEXT("NotProposed");
            FString FirstLevel2;
            FString Status = TEXT("NeedsDesignReview");
            if (!SuggestedBranch.IsNone())
            {
                const TArray<const FRPGClassProgressionChoiceDefinition*>* Choices = BranchChoices.Find(SuggestedBranch);
                if (!Choices)
                {
                    OutError = FString::Printf(TEXT("Kit %s references absent class branch %s."),
                        *Kit.KitId.ToString(), *SuggestedBranch.ToString());
                    return false;
                }
                BranchValid = TEXT("Yes");
                TArray<FString> Found;
                for (const FRPGClassProgressionChoiceDefinition* Choice : *Choices)
                {
                    if (Choice->MinimumLevel == 2) Found.Add(Choice->ChoiceId.ToString());
                }
                Found.Sort();
                FirstLevel2 = FString::Join(Found, TEXT(";"));
                Status = TEXT("ProposedMatchNeedsReview");
            }
            else
            {
                ++OutReport.KitsWithoutProposedBranch;
            }

            OutReport.KitMatrixCsv += CsvRow({
                Class->ClassId.ToString(),
                Kit.KitId.ToString(),
                Kit.KitId == Class->DefaultStartingEquipmentKitId ? TEXT("Yes") : TEXT("No"),
                FString::FromInt(Kit.Items.Num()), ActualItems, SuggestedBranch.ToString(),
                BranchValid, FirstLevel2, RelatedSkill.ToString(),
                AffinityName(SkillCatalog->ResolveClassAffinity(RelatedSkill, Class->ClassId)), Status
            });
            ++OutReport.KitCount;
        }
    }
    OutReport.SkillCount = SkillCatalog->Skills.Num();
    if (OutReport.ClassCount != ExpectedClasses || OutReport.KitCount != ExpectedKits ||
        OutReport.BranchCount != ExpectedBranches ||
        OutReport.AffinityCount != ExpectedClasses * ExpectedSkills ||
        OutReport.KitsWithoutProposedBranch != 4)
    {
        OutError = TEXT("Unexpected kit, branch or affinity coverage in ARMEMENT01.9.4 audit.");
        return false;
    }
    return true;
}
