#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "Core/GridObjectPaletteAsset.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridCanonicalItemCatalogues.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "UObject/StrongObjectPtr.h"

namespace ARMCLEAN064X13
{
enum class EField : uint8 { Label, Description, Icon };

struct FFixture
{
    TStrongObjectPtr<UGridItemSystemAsset> Root;
    TStrongObjectPtr<UGridObjectPaletteAsset> Palette;
    TStrongObjectPtr<UGameInstance> GI;
    TStrongObjectPtr<UGridItemSubsystem> Registry;
    bool bReady = false;
    FFixture()
        : Root(LoadObject<UGridItemSystemAsset>(nullptr,
            TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ItemSystem.DA_ItemSystem")))
        , Palette(LoadObject<UGridObjectPaletteAsset>(nullptr,
            TEXT("/Game/GrimrockPrototype/Core/DataAssets/DA_ObjectPalette_Default.DA_ObjectPalette_Default")))
        , GI(NewObject<UGameInstance>())
        , Registry(NewObject<UGridItemSubsystem>(GI.Get()))
    {
        TArray<FString> Errors;
        bReady = Root.IsValid() && Palette.IsValid() && Registry.IsValid() &&
            Registry->RegisterItemSystem(Root.Get(), Errors);
    }
};

bool Verify(FAutomationTestBase& Test, EField Field)
{
    FFixture F;
    if (!Test.TestTrue(TEXT("Real palette and canonical registry available"), F.bReady))
        {return false;}
    const bool bRootDirty = F.Root->GetOutermost()->IsDirty();
    const bool bPaletteDirty = F.Palette->GetOutermost()->IsDirty();
    int32 Count = 0;
    int32 Differences = 0;
    for (const FGridObjectPaletteEntry& Entry : F.Palette->Entries)
    {
        if (!Entry.DefaultItemDefinition) {continue;}
        ++Count;
        FGridResolvedItem Resolved;
        if (!Test.TestTrue(TEXT("Persisted palette identity resolves physically"),
            F.Registry->ResolveItem(Entry.CanonicalItemId,
                Entry.CanonicalDefaultItemLevel, Resolved)))
            {return false;}
        bool bSame = false;
        switch (Field)
        {
            case EField::Label:
            {
                const FString Old = Entry.GetEffectiveDisplayName().ToString();
                const FString New = Resolved.DisplayName.IsEmpty()
                    ? Entry.CanonicalItemId.ToString() : Resolved.DisplayName.ToString();
                bSame = Old == New;
                break;
            }
            case EField::Description:
                bSame = Entry.DefaultItemDefinition->Description.ToString() ==
                    Resolved.Description.ToString();
                break;
            case EField::Icon:
                bSame = Entry.DefaultItemDefinition->Icon.ToSoftObjectPath() ==
                    Resolved.ItemIcon.ToSoftObjectPath();
                break;
        }
        if (!bSame) {++Differences;}
    }
    if (!Test.TestEqual(TEXT("All 33 real direct entries checked"), Count, 33) ||
        !Test.TestEqual(TEXT("No UI field can change silently at cutover"),
            Differences, 0) ||
        !Test.TestEqual(TEXT("Read-only palette"), F.Palette->GetOutermost()->IsDirty(), bPaletteDirty) ||
        !Test.TestEqual(TEXT("Read-only catalogue"), F.Root->GetOutermost()->IsDirty(), bRootDirty))
        {return false;}
    return true;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridEditorClean064X13Labels,
    "Grimrock.RPG.ARM.CLEAN06_4X13.ProbeA_LabelParity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridEditorClean064X13Labels::RunTest(const FString&)
{
    return ARMCLEAN064X13::Verify(*this, ARMCLEAN064X13::EField::Label);
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridEditorClean064X13Descriptions,
    "Grimrock.RPG.ARM.CLEAN06_4X13.ProbeB_DescriptionParity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridEditorClean064X13Descriptions::RunTest(const FString&)
{
    return ARMCLEAN064X13::Verify(*this, ARMCLEAN064X13::EField::Description);
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridEditorClean064X13Icons,
    "Grimrock.RPG.ARM.CLEAN06_4X13.ProbeC_IconParity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridEditorClean064X13Icons::RunTest(const FString&)
{
    return ARMCLEAN064X13::Verify(*this, ARMCLEAN064X13::EField::Icon);
}
#endif
