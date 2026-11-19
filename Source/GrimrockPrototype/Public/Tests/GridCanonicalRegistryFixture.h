#pragma once

#if WITH_DEV_AUTOMATION_TESTS
#include "Engine/GameInstance.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "UObject/StrongObjectPtr.h"

/** Read-only production ItemSystem in a standalone automation fixture.
 * No GameInstance initialization, asset writes, or historical lookup.
 */
struct FGridCanonicalRegistryFixture
{
    TStrongObjectPtr<UGameInstance> GI{NewObject<UGameInstance>(GetTransientPackage())};
    TStrongObjectPtr<UGridItemSubsystem> Registry{NewObject<UGridItemSubsystem>(GI.Get())};
    bool bReady = false;

    FGridCanonicalRegistryFixture()
    {
        UGridItemSystemAsset* Root = GetDefault<UGridItemSubsystem>()->LoadConfiguredItemSystemForPreview();
        TArray<FString> Errors;
        bReady = IsValid(Root) && Registry.IsValid() && Registry->RegisterItemSystem(Root, Errors);
    }

    bool Resolve(FName Id, int32 PhysicalLevel, FGridResolvedItem& Out) const
    {
        return bReady && Registry->ResolveItem(Id, PhysicalLevel, Out) &&
            Out.IsValid() && Out.ItemId == Id && Out.ItemLevel == PhysicalLevel;
    }
};
#endif
