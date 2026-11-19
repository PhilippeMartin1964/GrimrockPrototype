#pragma once
#include "ARMEMENT/GridWeaponPipelineManifest.h"
#include "ARMEMENT/GridArmClean06ProjectilePolicyProjection.h"
#include "ARMEMENT/GridArmClean06ProjectileVisualProjection.h"
#include "Runtime/Items/GridCanonicalItemCatalogues.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "UObject/UnrealType.h"

/** Exactly the 19 non-specialized active ARM flight policies deferred by 06.2W/X.
 * Never change bEnabled, projectile mesh, action bindings, combat permissions,
 * physical equipment level progression or any inactive archetype. */
namespace GridEditorArmHandThrowPlan
{
    constexpr int32 ArmCount = 22;
    constexpr int32 OrdinaryCount = 19;
    constexpr int32 SpecializedCount = 3;

    template <typename T>
    bool Equal(const T& A, const T& B)
    {
        return T::StaticStruct()->CompareScriptStruct(&A, &B, 0);
    }

    inline bool Project(const UGridItemDefinitionAsset& Source,
        FGridCanonicalProjectilePolicy& Physics, FGridCanonicalProjectileVisual& Appearance)
    {
        if (Source.bCombatThrowWeapon || Physics.bEnabled || Appearance.bEnabled ||
            !FMath::IsFinite(Source.ThrowSpeed) || Source.ThrowSpeed < 0.f ||
            !FMath::IsFinite(Source.ThrowArc) || !FMath::IsFinite(Source.ThrowLifeSeconds) || Source.ThrowLifeSeconds < 0.f ||
            !FMath::IsFinite(Source.ThrowImpactDropOffset) || Source.ThrowImpactDropOffset < 0.f ||
            Source.ThrowVisualRelativeRotation.ContainsNaN() || Source.ThrowVisualRelativeScale.ContainsNaN() ||
            Source.ThrowVisualTumbleAxis.ContainsNaN() || !FMath::IsFinite(Source.ThrowVisualTumbleDegreesPerSecond) ||
            !FMath::IsFinite(Source.ThrowVisualSpinDegreesPerSecond))
            return false;
        FGridCanonicalProjectilePolicy P = Physics;
        FGridCanonicalProjectileVisual V = Appearance;
        switch (Source.ThrowVisualMode)
        {
        case EGridThrowVisualMode::Stable: V.Mode = EGridCanonicalProjectileVisualMode::Stable; break;
        case EGridThrowVisualMode::Tumble: V.Mode = EGridCanonicalProjectileVisualMode::Tumble; break;
        case EGridThrowVisualMode::Spin: V.Mode = EGridCanonicalProjectileVisualMode::Spin; break;
        default: return false;
        }
        P.bGenericThrowAuthored = true;
        P.bGenericThrowOneHanded = Source.GetEffectiveHandUsage() == EGridItemHandUsage::OneHanded;
        P.InitialSpeed = Source.ThrowSpeed;
        P.Arc = Source.ThrowArc;
        P.FlightLifetime = Source.ThrowLifeSeconds;
        P.ImpactDropOffset = Source.ThrowImpactDropOffset;
        V.RelativeRotation = Source.ThrowVisualRelativeRotation;
        V.RelativeScale = Source.ThrowVisualRelativeScale;
        V.MeshForwardAxis = Source.ProjectileMeshForwardAxis;
        V.TumbleAxis = Source.ThrowVisualTumbleAxis;
        V.TumbleDegreesPerSecond = Source.ThrowVisualTumbleDegreesPerSecond;
        V.SpinDegreesPerSecond = Source.ThrowVisualSpinDegreesPerSecond;
        Physics = P;
        Appearance = V;
        return true;
    }

    struct FPlan
    {
        TArray<FGridCanonicalWeaponArchetype> Archetypes;
        TArray<FGridCanonicalWeaponVisual> Visuals;
        int32 Already = 0;
        int32 Ordinary = 0;
        int32 Specialized = 0;
        int32 PhysicallyThrowable = 0;
    };

    inline bool Build(const UGridItemSystemAsset& Root, FPlan& Out, FString& Error)
    {
        Out = FPlan();
        TArray<FString> Errors;
        TArray<FGridWeaponPipelineRow> Rows;
        if (Root.ItemDefinitions.Num() != 48 || !Root.Validate(Errors) ||
            !FGridWeaponPipelineManifest::Load(FGridWeaponPipelineManifest::DefaultFile(), Rows, Error) ||
            Rows.Num() != ArmCount || !Root.WeaponCatalogue || !Root.WeaponVisualCatalogue)
        {
            Error = TEXT("Native registry or exact production manifest rejected.");
            return false;
        }
        Out.Archetypes = Root.WeaponCatalogue->Archetypes;
        Out.Visuals = Root.WeaponVisualCatalogue->VisualProfiles;
        TSet<FName> SeenArms, SeenVisuals;
        for (const FGridWeaponPipelineRow& Row : Rows)
        {
            const FName Id(*Row.ItemDefinitionId), Arch(*Row.ArchetypeId), VisualId(*Row.VisualId);
            const auto* Old = LoadObject<UGridItemDefinitionAsset>(nullptr, *Row.ItemPath);
            auto* A = Out.Archetypes.FindByPredicate([Arch](const auto& X){ return X.ArchetypeId == Arch; });
            auto* V = Out.Visuals.FindByPredicate([VisualId](const auto& X){ return X.VisualId == VisualId; });
            const auto* BoundVisual = Root.WeaponVisualCatalogue->FindForArchetype(Arch);
            if (!IsValid(Old) || Old->GetPathName() != Row.ItemPath || Old->GetOutermost()->IsDirty() ||
                Row.ProductionState != TEXT("Active") || Old->ItemDefinitionId != Id || Old->WeaponArchetypeId != Arch ||
                Old->bCombatThrowWeapon != Row.IsThrowable() || !A || !V || !BoundVisual ||
                BoundVisual->VisualId != VisualId || SeenArms.Contains(Arch) || SeenVisuals.Contains(VisualId) ||
                !Root.ItemDefinitions.ContainsByPredicate([&](const auto& D){
                    const auto* Arm = Cast<UGridArmamentDefinitionAsset>(D.Get());
                    return Arm && Arm->ItemId == Id && Arm->ArchetypeId == Arch; }))
            {
                Error = TEXT("Identity/visual/native source mismatch: ") + Row.ArchetypeId;
                return false;
            }
            SeenArms.Add(Arch);
            SeenVisuals.Add(VisualId);
            if (Row.IsThrowable())
            {
                if (A->Projectile.bGenericThrowAuthored ||
                    A->Projectile.bGenericThrowOneHanded ||
                    !GridArmClean06ProjectilePolicyProjection::Matches(*Old, A->Projectile) ||
                    !GridArmClean06ProjectileVisualProjection::Matches(*Old, *V))
                {
                    Error = TEXT("Specialized projectile mismatch: ") + Row.ArchetypeId;
                    return false;
                }
                ++Out.Specialized;
                continue;
            }
            ++Out.Ordinary;
            FGridCanonicalProjectilePolicy P = A->Projectile;
            FGridCanonicalProjectileVisual Visual = V->ProjectileAppearance;
            if (!V->ProjectileMesh.IsNull() || !Project(*Old, P, Visual))
            {
                Error = TEXT("Ordinary physical/visual source rejected: ") + Row.ArchetypeId;
                return false;
            }
            // Prove old permission rather than manufacture it from the
            // existence of a physical policy. The existing strength gate
            // additionally enforces its main-hand rule, unchanged.
            const bool bProjectedPhysical = P.bGenericThrowOneHanded &&
                FMath::IsFinite(Old->Weight) && Old->Weight >= 0.f && P.InitialSpeed > KINDA_SMALL_NUMBER;
            if (bProjectedPhysical != Old->IsPhysicallyThrowable())
            {
                Error = TEXT("Hand-toss eligibility differs: ") + Row.ArchetypeId;
                return false;
            }
            if (bProjectedPhysical) ++Out.PhysicallyThrowable;
            if (A->Projectile.bGenericThrowAuthored)
            {
                ++Out.Already;
                if (!Equal(A->Projectile, P) || !Equal(V->ProjectileAppearance, Visual))
                {
                    Error = TEXT("Previously authored flight data contradicts source: ") + Row.ArchetypeId;
                    return false;
                }
            }
            else
            {
                // No hand-toss policy was authored in 06.2W/06.2X.
                // Reject unexpected independent changes rather than overwrite.
                if (!Equal(A->Projectile, FGridCanonicalProjectilePolicy()) ||
                    !Equal(V->ProjectileAppearance, FGridCanonicalProjectileVisual()))
                {
                    Error = TEXT("Unconverted policy contains unexpected authored values: ") + Row.ArchetypeId;
                    return false;
                }
            }
            A->Projectile = P;
            V->ProjectileAppearance = Visual;
            for (int32 Level : {1,20})
            {
                FGridCanonicalResolvedWeapon Resolved;
                if (!A->TryResolve(Level, Resolved) || !Equal(Resolved.Projectile, P))
                {
                    Error = TEXT("Resolved level lost ordinary flight data: ") + Row.ArchetypeId;
                    return false;
                }
            }
        }
        int32 Count = 0;
        for (const auto& D : Root.ItemDefinitions)
            if (Cast<UGridArmamentDefinitionAsset>(D.Get())) ++Count;
        if (Count != ArmCount || Out.Ordinary != OrdinaryCount || Out.Specialized != SpecializedCount ||
            (Out.Already != 0 && Out.Already != OrdinaryCount) || !Out.PhysicallyThrowable)
        {
            Error = TEXT("Partial migration or unexpected active item count.");
            return false;
        }
        return true;
    }

    inline bool AssignPreview(const UGridItemSystemAsset& Root, const FPlan& Plan, UGridItemSystemAsset& Preview)
    {
        Preview.ItemDefinitions = Root.ItemDefinitions;
        Preview.ActionCatalogue = Root.ActionCatalogue;
        Preview.ActionPresentationCatalogue = Root.ActionPresentationCatalogue;
        Preview.WeaponCatalogue = DuplicateObject<UGridCanonicalWeaponCatalogueAsset>(Root.WeaponCatalogue.Get(), &Preview);
        Preview.WeaponVisualCatalogue = DuplicateObject<UGridCanonicalWeaponVisualCatalogueAsset>(Root.WeaponVisualCatalogue.Get(), &Preview);
        if (!Preview.WeaponCatalogue || !Preview.WeaponVisualCatalogue) return false;
        Preview.WeaponCatalogue->Archetypes = Plan.Archetypes;
        Preview.WeaponVisualCatalogue->VisualProfiles = Plan.Visuals;
        TArray<FString> Errors;
        return Preview.Validate(Errors);
    }

    inline FString Export(const UObject& Obj)
    {
        FString Result;
        for (TFieldIterator<FProperty> Field(Obj.GetClass(), EFieldIteratorFlags::ExcludeSuper); Field; ++Field)
        {
            if (Field->HasAnyPropertyFlags(CPF_Transient)) continue;
            FString Value;
            Field->ExportTextItem_Direct(Value, Field->ContainerPtrToValuePtr<void>(&Obj), nullptr, const_cast<UObject*>(&Obj), PPF_None);
            Result += Field->GetName() + TEXT("=") + Value + TEXT("\n");
        }
        return Result;
    }

    inline FString Manifest(const UGridItemSystemAsset& Root, const UGridItemSystemAsset& Preview)
    {
        FString Result = Export(Root) + Export(*Root.ActionCatalogue) + Export(*Root.ActionPresentationCatalogue) +
            Export(*Preview.WeaponCatalogue) + Export(*Preview.WeaponVisualCatalogue);
        for (const auto& D : Root.ItemDefinitions)
            Result += D->GetPathName() + TEXT("\n");
        return Result;
    }
}
