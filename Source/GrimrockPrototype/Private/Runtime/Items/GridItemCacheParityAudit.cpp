#include "Runtime/Items/GridItemCacheParityAudit.h"

#if WITH_EDITOR
#include "../GridItemWeightReadModel.h"
#include "../GridWorldPhysicsMassReadModel.h"
#include "../GridPhysicalThrowPreflightReadModel.h"
#include "../GridInventoryDiagnosticWeightReadModel.h"
#include "../../UI/GridInventorySortReadModel.h"

bool GridItemCacheParityAudit::HasCanonicalProjection(
    const UGridItemSubsystem& Registry, const FGridItemInstance& Instance)
{
    FGridResolvedItem Canonical;
    if (!Instance.IsValid() ||
        !Registry.ResolveItem(Instance.ItemDefinitionId, Instance.ItemLevel, Canonical) ||
        !Canonical.IsValid() || Canonical.ItemId != Instance.ItemDefinitionId ||
        Canonical.ItemLevel != Instance.ItemLevel ||
        (Canonical.bIsArmament ? Instance.ItemLevel < 1 : Instance.ItemLevel != 0) ||
        Instance.Quantity > FMath::Max(1, Canonical.MaxStack) ||
        !FMath::IsFinite(Canonical.WeightKg) || Canonical.WeightKg < 0.f) return false;

    const float Total = Canonical.WeightKg * static_cast<float>(Instance.Quantity);
    float UnitWeight = 0.f;
    if (!FMath::IsFinite(Total) ||
        !FGridItemWeightReadModel::TryResolveUnitWeight(&Registry, Instance, UnitWeight) ||
        UnitWeight != Canonical.WeightKg ||
        FGridItemWeightReadModel::GetTotalWeight(&Registry, Instance) != Total ||
        FGridInventoryDiagnosticWeightReadModel::FormatInstanceUnitWeight(&Registry, Instance) !=
            FString::Printf(TEXT("%.1f"), Canonical.WeightKg)) return false;

    const FText Name = Canonical.DisplayName.IsEmpty()
        ? FText::FromName(Instance.ItemDefinitionId) : Canonical.DisplayName;
    const auto Meta = FGridInventoryItemMetadataReadModel::Resolve(&Registry, nullptr, Instance);
    if (!Meta.bResolved || !Meta.bRegisteredIdentity || Meta.UnitWeightKg != Canonical.WeightKg ||
        !Meta.DisplayName.EqualTo(Name) || !Meta.Description.EqualTo(Canonical.Description)) return false;
    const auto Sort = FGridInventorySortReadModel::Resolve(&Registry, nullptr, Instance);
    if (Sort.TotalWeight != Total || Sort.DisplayName != Name.ToString()) return false;

    float Mass = 0.f;
    const bool bExpectedMass = Canonical.bUseItemWeightAsWorldPhysicsMass &&
        Canonical.WeightKg > KINDA_SMALL_NUMBER;
    if (FGridWorldPhysicsMassReadModel::TryResolve(&Registry, nullptr, Instance, Mass) != bExpectedMass ||
        Mass != (bExpectedMass ? Canonical.WeightKg : 0.f)) return false;

    const bool bAuthoredFlight = !Canonical.bIsArmament || Canonical.Weapon.Projectile.bEnabled ||
        Canonical.Weapon.Projectile.bGenericThrowAuthored;
    const float Speed = Canonical.bIsArmament ? Canonical.Weapon.Projectile.InitialSpeed : Canonical.GenericThrow.InitialSpeed;
    const float Arc = Canonical.bIsArmament ? Canonical.Weapon.Projectile.Arc : Canonical.GenericThrow.Arc;
    const bool bFlight = bAuthoredFlight && FMath::IsFinite(Speed) && Speed > KINDA_SMALL_NUMBER &&
        FMath::IsFinite(Arc);
    const bool bOneHanded = Canonical.bIsArmament
        ? (Canonical.Weapon.Projectile.bGenericThrowAuthored
            ? Canonical.Weapon.Projectile.bGenericThrowOneHanded
            : Canonical.Weapon.HandPolicy != EGridHandPolicy::TwoHanded &&
                Canonical.Weapon.CanEquipInHand(EGridEquipmentSlot::MainHand))
        : Canonical.NonArmHandPolicy == EGridNonArmamentHandPolicy::OneHanded;

    for (const int32 Strength : {0, 1, 5, 10, 20, 40, 100})
    {
        const auto View = FGridPhysicalThrowPreflightReadModel::Resolve(&Registry, nullptr, Instance, Strength);
        const float Limit = Strength * 0.25f;
        const bool bStrengthAllows = Strength > 0 && bOneHanded && bAuthoredFlight &&
            FMath::IsFinite(Speed) && Speed > KINDA_SMALL_NUMBER &&
            Canonical.WeightKg <= Limit + KINDA_SMALL_NUMBER;
        const float ExpectedScale = bStrengthAllows
            ? FMath::Clamp(FMath::Clamp(FMath::Sqrt(FMath::Max(1.f, static_cast<float>(Strength)) / 10.f),
                0.65f, 1.5f) * FMath::Lerp(1.f, 0.65f,
                    FMath::Clamp(Canonical.WeightKg / FMath::Max(KINDA_SMALL_NUMBER, Limit), 0.f, 1.f)),
                0.5f, 1.5f) : 0.f;
        if (!View.bRegisteredIdentity || !View.Strength.bRegisteredIdentity || !View.Strength.bResolved ||
            View.Strength.UnitWeightKg != Canonical.WeightKg || View.Strength.MaxWeightKg != Limit ||
            View.Strength.bOneHanded != bOneHanded || View.Strength.bCanThrow != bStrengthAllows ||
            !FMath::IsNearlyEqual(View.Strength.SpeedScale, ExpectedScale) ||
            !View.Ballistics.bRegisteredIdentity || View.Ballistics.bResolved != bFlight ||
            View.bResolved != bFlight || View.bCanThrow != (bFlight && bStrengthAllows)) return false;
        if (bFlight && (View.Ballistics.InitialSpeed != Speed || View.Ballistics.Arc != Arc ||
            View.Ballistics.bSpecializedProjectile !=
                (Canonical.bIsArmament && Canonical.Weapon.Projectile.bEnabled))) return false;
    }
    return true;
}
#endif
