#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Runtime/Combat/GridWeaponCatalogueAsset.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/Package.h"

namespace ARMEMENT0197
{
    UGridWeaponCatalogueAsset* LoadCatalogue()
    {
        return LoadObject<UGridWeaponCatalogueAsset>(nullptr,
            TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponCatalogue.DA_WeaponCatalogue"));
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0197QualifiedPriest,
    "Grimrock.RPG.ARMEMENT01.9.KitApproval.QualifiedPriestAffinities",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0197QualifiedPriest::RunTest(const FString&)
{
    UGridWeaponCatalogueAsset* Catalogue = ARMEMENT0197::LoadCatalogue();
    if (!TestNotNull(TEXT("Materialized workbook catalogue"), Catalogue)) return false;
    if (!TestTrue(TEXT("82 real class affinities exist"), Catalogue->IsValidClassAffinityData())) return false;
    using A = EGridWeaponKitAffinity;
    TestEqual(TEXT("Great sword cannot be automatically recommended to Priest"),
        Catalogue->GetKitRecommendationAffinity(TEXT("ARM027"), TEXT("Priest")), A::NeedsReview);
    TestEqual(TEXT("Great axe cannot be automatically recommended to Priest"),
        Catalogue->GetKitRecommendationAffinity(TEXT("ARM031"), TEXT("Priest")), A::NeedsReview);
    TestEqual(TEXT("Two-handed hammer still needs human review of the qualifier"),
        Catalogue->GetKitRecommendationAffinity(TEXT("ARM034"), TEXT("Priest")), A::NeedsReview);
    TestEqual(TEXT("War scythe still needs human review of the qualifier"),
        Catalogue->GetKitRecommendationAffinity(TEXT("ARM037"), TEXT("Priest")), A::NeedsReview);
    TestEqual(TEXT("Warrior remains primary for great axe"),
        Catalogue->GetKitRecommendationAffinity(TEXT("ARM031"), TEXT("Warrior")), A::Primary);
    TestEqual(TEXT("Ordinary Priest round shield is an unqualified secondary"),
        Catalogue->GetKitRecommendationAffinity(TEXT("ARM075"), TEXT("Priest")), A::Secondary);
    TestEqual(TEXT("Ordinary Ranger spear is an unqualified secondary"),
        Catalogue->GetKitRecommendationAffinity(TEXT("ARM038"), TEXT("Ranger")), A::Secondary);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0197IndependentFragments,
    "Grimrock.RPG.ARMEMENT01.9.KitApproval.IndependentSourceQualifiers",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0197IndependentFragments::RunTest(const FString&)
{
    UGridWeaponCatalogueAsset* Synthetic = NewObject<UGridWeaponCatalogueAsset>(GetTransientPackage());
    if (!TestNotNull(TEXT("Transient test catalogue"), Synthetic)) return false;
    FGridWeaponArchetypeClassAffinity& Row = Synthetic->ClassAffinities.AddDefaulted_GetRef();
    Row.ArchetypeId = TEXT("ARM999");
    Row.PrimaryClassId = TEXT("Warrior");
    Row.SecondaryClassIds = {FName(TEXT("Priest")), FName(TEXT("Ranger"))};
    Row.SecondarySourceLabel = TEXT("Prêtre (marteaux) / Rôdeur");
    using A = EGridWeaponKitAffinity;
    TestEqual(TEXT("Priest is qualified and needs review"),
        Synthetic->GetKitRecommendationAffinity(Row.ArchetypeId, TEXT("Priest")), A::NeedsReview);
    TestEqual(TEXT("Ranger is unqualified on the SAME row"),
        Synthetic->GetKitRecommendationAffinity(Row.ArchetypeId, TEXT("Ranger")), A::Secondary);
    TestEqual(TEXT("Primary stays primary without qualifier"),
        Synthetic->GetKitRecommendationAffinity(Row.ArchetypeId, TEXT("Warrior")), A::Primary);
    TestEqual(TEXT("No invented affinity for Rogue"),
        Synthetic->GetKitRecommendationAffinity(Row.ArchetypeId, TEXT("Rogue")), A::Unspecified);
    Row.SecondarySourceLabel = TEXT("Prêtre / Rôdeur");
    TestEqual(TEXT("Removing a source qualifier makes Priest an ordinary secondary"),
        Synthetic->GetKitRecommendationAffinity(Row.ArchetypeId, TEXT("Priest")), A::Secondary);
    Row.SecondarySourceLabel = TEXT("Rôdeur");
    TestEqual(TEXT("Missing source fragment does not silently approve Priest"),
        Synthetic->GetKitRecommendationAffinity(Row.ArchetypeId, TEXT("Priest")), A::NeedsReview);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0197NoGameplayMutation,
    "Grimrock.RPG.ARMEMENT01.9.KitApproval.NoGameplayOrCatalogueMutation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0197NoGameplayMutation::RunTest(const FString&)
{
    UGridWeaponCatalogueAsset* Catalogue = ARMEMENT0197::LoadCatalogue();
    if (!TestNotNull(TEXT("Production catalogue"), Catalogue)) return false;
    if (!TestTrue(TEXT("Original 1640 profiles remain valid"), Catalogue->IsValidCatalogue())) return false;
    const bool bDirtyBefore = Catalogue->GetOutermost()->IsDirty();
    const FGridWeaponLevelProfile* OriginalN1 = Catalogue->FindProfile(TEXT("ARM031"), 1);
    if (!TestNotNull(TEXT("Great axe N1 exists"), OriginalN1)) return false;
    const int32 OriginalPA = OriginalN1->ActionPointCost;
    const int32 OriginalDiceCount = OriginalN1->DamageDice.DiceCount;
    // Backward compatibility: original three-value read model is unchanged.
    TestEqual(TEXT("Legacy advisory API unchanged"),
        Catalogue->GetClassAffinity(TEXT("ARM031"), TEXT("Priest")),
        EGridWeaponClassAffinity::Secondary);
    TestEqual(TEXT("Qualified affinity does not change profiles or equip legality"),
        Catalogue->GetKitRecommendationAffinity(TEXT("ARM031"), TEXT("Priest")),
        EGridWeaponKitAffinity::NeedsReview);
    const FGridWeaponLevelProfile* After = Catalogue->FindProfile(TEXT("ARM031"), 1);
    if (!TestNotNull(TEXT("Profile stays addressable"), After)) return false;
    TestEqual(TEXT("Action-point cost intact"), After->ActionPointCost, OriginalPA);
    TestEqual(TEXT("Dice still unchanged"), After->DamageDice.DiceCount, OriginalDiceCount);
    TestEqual(TEXT("No production package dirtied"), Catalogue->GetOutermost()->IsDirty(), bDirtyBefore);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
