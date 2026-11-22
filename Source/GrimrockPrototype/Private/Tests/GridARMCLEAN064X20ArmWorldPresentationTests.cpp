#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "NiagaraSystem.h"
#include "Runtime/Items/GridItemCatalogues.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UObjectGlobals.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean064X20ArmWorldPresentation,
    "Grimrock.RPG.ARM.CLEAN06_4X20.ArmWorldPresentationProjection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean064X20ArmWorldPresentation::RunTest(const FString&)
{
    TStrongObjectPtr<UGridItemSystemAsset> Actual(LoadObject<UGridItemSystemAsset>(
        nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_ItemSystem.DA_ItemSystem")));
    if (!TestNotNull(TEXT("Actual item system"), Actual.Get()) ||
        !TestNotNull(TEXT("Actual ARM visual catalogue"), Actual->WeaponVisualCatalogue.Get()))
    {
        return false;
    }

    // No native asset can be edited by this test: duplicate both owners.
    TStrongObjectPtr<UGridItemSystemAsset> Root(
        DuplicateObject<UGridItemSystemAsset>(Actual.Get(), GetTransientPackage()));
    if (!TestNotNull(TEXT("Transient root"), Root.Get())) { return false; }
    TStrongObjectPtr<UGridCanonicalWeaponVisualCatalogueAsset> Visuals(
        DuplicateObject<UGridCanonicalWeaponVisualCatalogueAsset>(
            Actual->WeaponVisualCatalogue.Get(), Root.Get()));
    if (!TestNotNull(TEXT("Transient visual catalogue"), Visuals.Get()) ||
        !TestTrue(TEXT("A real ARM visual exists"), !Visuals->VisualProfiles.IsEmpty()))
    {
        return false;
    }
    Root->WeaponVisualCatalogue = Visuals.Get();
    TStrongObjectPtr<UGridCanonicalWeaponCatalogueAsset> Rules(
        DuplicateObject<UGridCanonicalWeaponCatalogueAsset>(
            Actual->WeaponCatalogue.Get(), Root.Get()));
    if (!TestNotNull(TEXT("Transient physics catalogue"), Rules.Get()))
    {
        return false;
    }
    Root->WeaponCatalogue = Rules.Get();

    FGridWeaponVisual& Edited = Visuals->VisualProfiles[0];
    Edited.LightEmitter.bDefaultEnabled = true;
    Edited.LightEmitter.bUsePointLight = true;
    Edited.LightEmitter.LightIntensity = 427.f;
    Edited.WorldSparkleIntensity = 3.5f;
    Edited.WorldSparkleSpeed = 1.25f;
    Edited.WorldSparkleVariation = 0.75f;

    FGridItemNiagaraEffectConfig Effect;
    Effect.EffectId = FName(TEXT("X20_PersistentEffect"));
    Effect.NiagaraSystem = TSoftObjectPtr<UNiagaraSystem>(FSoftObjectPath(
        TEXT("/Game/GrimrockPrototype/Effects/Fire/Torch/NS_Flame_8_Torch.NS_Flame_8_Torch")));
    Edited.VisualEffects.Effects.Add(Effect);

    TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
    TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
    TArray<FString> Errors;
    if (!TestTrue(TEXT("Transient item system registered"),
        Registry->RegisterItemSystem(Root.Get(), Errors)))
    {
        for (const FString& Error : Errors) { AddError(Error); }
        return false;
    }

    const FGridWeaponVisualBinding* Binding =
        Visuals->ArchetypeBindings.FindByPredicate(
            [&Edited](const FGridWeaponVisualBinding& Entry)
            {
                return Entry.VisualId == Edited.VisualId;
            });
    if (!TestNotNull(TEXT("Edited visual has a bound ARM"), Binding)) { return false; }

    const UGridArmamentDefinitionAsset* Item = nullptr;
    for (const UGridCanonicalItemDefinitionAsset* Def : Root->ItemDefinitions)
    {
        const UGridArmamentDefinitionAsset* Arm = Cast<UGridArmamentDefinitionAsset>(Def);
        if (Arm && Arm->ArchetypeId == Binding->ArchetypeId)
        {
            Item = Arm;
            break;
        }
    }
    if (!TestNotNull(TEXT("ARM item for edited visual"), Item)) { return false; }

    FGridWeaponArchetype* Rule = Rules->Archetypes.FindByPredicate(
        [&Item](const FGridWeaponArchetype& Entry)
        {
            return Entry.ArchetypeId == Item->ArchetypeId;
        });
    if (!TestNotNull(TEXT("Transient ARM physics policy"), Rule)) { return false; }
    Rule->bUseItemWeightAsWorldPhysicsMass = true;
    Rule->WorldPhysicsInitialTiltDegrees = 17.f;

    for (const int32 N : {1, 20})
    {
        FGridResolvedItem Resolved;
        if (!TestTrue(TEXT("N1/N20 resolves"), Registry->ResolveItem(Item->ItemId, N, Resolved)) ||
            !TestTrue(TEXT("Light enabled projected"), Resolved.LightEmitter.bDefaultEnabled) ||
            !TestTrue(TEXT("Point light projected"), Resolved.LightEmitter.bUsePointLight) ||
            !TestEqual(TEXT("Light intensity projected"), Resolved.LightEmitter.LightIntensity, 427.f) ||
            !TestEqual(TEXT("Niagara count projected"), Resolved.VisualEffects.Effects.Num(), 1) ||
            !TestEqual(TEXT("Niagara identity projected"),
                Resolved.VisualEffects.Effects[0].EffectId, Effect.EffectId) ||
            !TestEqual(TEXT("Sparkle intensity projected"), Resolved.WorldSparkleIntensity, 3.5f) ||
            !TestEqual(TEXT("Sparkle speed projected"), Resolved.WorldSparkleSpeed, 1.25f) ||
            !TestEqual(TEXT("Sparkle variation projected"), Resolved.WorldSparkleVariation, 0.75f) ||
            !TestTrue(TEXT("Physics mass policy projected"), Resolved.bUseItemWeightAsWorldPhysicsMass) ||
            !TestEqual(TEXT("Physics tilt projected"), Resolved.WorldPhysicsInitialTiltDegrees, 17.f))
        {
            return false;
        }
    }

    const FGridWeaponArchetype* Native =
        Actual->WeaponCatalogue->Find(Item->ArchetypeId);
    return TestNotNull(TEXT("Native physics policy unchanged"), Native) &&
        TestFalse(TEXT("Native tilt was not written"), Native->WorldPhysicsInitialTiltDegrees == 17.f);
}
#endif
