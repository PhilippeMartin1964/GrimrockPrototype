#include "Runtime/Items/GridItemCacheParityAudit.h"

#if WITH_EDITOR
#include "../GridItemWeightReadModel.h"
#include "../GridWorldPhysicsMassReadModel.h"
#include "../GridPhysicalThrowPreflightReadModel.h"
#include "../GridInventoryDiagnosticWeightReadModel.h"
#include "../../UI/GridInventorySortReadModel.h"

bool GridItemCacheParityAudit::HasSameCanonicalProjection(
    const UGridItemSubsystem& Registry, const FGridItemInstance& Instance)
{
    FGridResolvedItem Canonical;
    if (!Instance.IsValid() || !FMath::IsFinite(Instance.Weight) || Instance.Weight < 0.f ||
        !Registry.ResolveItem(Instance.ItemDefinitionId, Instance.ItemLevel, Canonical) ||
        !Canonical.IsValid() || Canonical.ItemId != Instance.ItemDefinitionId ||
        Canonical.ItemLevel != Instance.ItemLevel ||
        (Canonical.bIsArmament ? Instance.ItemLevel < 1 : Instance.ItemLevel != 0) ||
        Instance.Quantity > FMath::Max(1, Canonical.MaxStack) ||
        !FMath::IsFinite(Canonical.WeightKg) || Canonical.WeightKg < 0.f) return false;

    FGridItemInstance Cleared = Instance;
    Cleared.Weight = 0.f;
    Cleared.DisplayName = FText::GetEmpty();

    float BeforeWeight = 0.f, AfterWeight = 0.f;
    if (!FGridItemWeightReadModel::TryResolveUnitWeight(&Registry, Instance, BeforeWeight) ||
        !FGridItemWeightReadModel::TryResolveUnitWeight(&Registry, Cleared, AfterWeight) ||
        BeforeWeight != Canonical.WeightKg || AfterWeight != BeforeWeight ||
        FGridItemWeightReadModel::GetTotalWeight(&Registry, Instance) !=
        FGridItemWeightReadModel::GetTotalWeight(&Registry, Cleared) ||
        FGridInventoryDiagnosticWeightReadModel::FormatInstanceUnitWeight(&Registry, Instance) !=
        FGridInventoryDiagnosticWeightReadModel::FormatInstanceUnitWeight(&Registry, Cleared)) return false;

    const auto BeforeMeta = FGridInventoryItemMetadataReadModel::Resolve(&Registry, nullptr, Instance);
    const auto AfterMeta = FGridInventoryItemMetadataReadModel::Resolve(&Registry, nullptr, Cleared);
    if (!BeforeMeta.bResolved || !AfterMeta.bResolved || !BeforeMeta.bCanonicalIdentity || !AfterMeta.bCanonicalIdentity ||
        BeforeMeta.UnitWeightKg != AfterMeta.UnitWeightKg || !BeforeMeta.DisplayName.EqualTo(AfterMeta.DisplayName) ||
        !BeforeMeta.Description.EqualTo(AfterMeta.Description)) return false;
    const auto BeforeSort = FGridInventorySortReadModel::Resolve(&Registry, nullptr, Instance);
    const auto AfterSort = FGridInventorySortReadModel::Resolve(&Registry, nullptr, Cleared);
    if (BeforeSort.TotalWeight != AfterSort.TotalWeight || BeforeSort.DisplayName != AfterSort.DisplayName) return false;
    float BeforeMass = 0.f, AfterMass = 0.f;
    const bool bBeforePhysics = FGridWorldPhysicsMassReadModel::TryResolve(&Registry, nullptr, Instance, BeforeMass);
    const bool bAfterPhysics = FGridWorldPhysicsMassReadModel::TryResolve(&Registry, nullptr, Cleared, AfterMass);
    if (bBeforePhysics != bAfterPhysics || BeforeMass != AfterMass) return false;

    // Compare the full preflight, including below/above ordinary Strength
    // thresholds. Unit mass equivalence is also checked directly above.
    for (const int32 Strength : {0, 1, 5, 10, 20, 40, 100})
    {
        const auto A = FGridPhysicalThrowPreflightReadModel::Resolve(&Registry, nullptr, Instance, Strength);
        const auto B = FGridPhysicalThrowPreflightReadModel::Resolve(&Registry, nullptr, Cleared, Strength);
        if (A.bCanonicalIdentity != B.bCanonicalIdentity || A.bResolved != B.bResolved || A.bCanThrow != B.bCanThrow ||
            A.Strength.bResolved != B.Strength.bResolved || A.Strength.bOneHanded != B.Strength.bOneHanded ||
            A.Strength.bCanThrow != B.Strength.bCanThrow || A.Strength.UnitWeightKg != B.Strength.UnitWeightKg ||
            A.Strength.MaxWeightKg != B.Strength.MaxWeightKg || A.Strength.SpeedScale != B.Strength.SpeedScale ||
            A.Ballistics.bResolved != B.Ballistics.bResolved || A.Ballistics.bSpecializedProjectile != B.Ballistics.bSpecializedProjectile ||
            A.Ballistics.InitialSpeed != B.Ballistics.InitialSpeed || A.Ballistics.Arc != B.Ballistics.Arc) return false;
    }
    return true;
}
#endif
