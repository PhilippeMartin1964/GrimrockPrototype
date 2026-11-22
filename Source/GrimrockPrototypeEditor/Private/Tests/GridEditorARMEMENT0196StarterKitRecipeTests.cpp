#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/Csv/CsvParser.h"
#include "RPG/RPGClassAsset.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "Tests/GridCanonicalRegistryFixture.h"
#include "UObject/UObjectGlobals.h"

namespace ARMEMENT0196
{
    struct FRecipe
    {
        FString ClassId;
        FString BranchId;
        FString KitId;
        FName Weapon1;
        int32 Weapon1Level = 0;
        FString Weapon1Slot;
        FName Weapon2;
        int32 Weapon2Level = 0;
        FString Weapon2Slot;
        FName SupportId;
        int32 SupportQuantity = 0;
        FString SupportNeed;
        FString Readiness;
        FString Approval;
        FString Note;
    };

    const TCHAR* const Headers[] = {
        TEXT("ClassId"), TEXT("BranchId"), TEXT("KitId"),
        TEXT("Weapon1ArchetypeId"), TEXT("Weapon1Level"), TEXT("Weapon1Slot"),
        TEXT("Weapon2ArchetypeId"), TEXT("Weapon2Level"), TEXT("Weapon2Slot"),
        TEXT("KnownSupportItemId"), TEXT("KnownSupportQuantity"),
        TEXT("PlannedSupportNeed"), TEXT("ProductionReadiness"),
        TEXT("ApprovalState"), TEXT("DesignNoteCode")
    };

    bool ReadRecipes(TArray<FRecipe>& Out, FString& Error)
    {
        Out.Reset();
        Error.Reset();
        FString Csv;
        const FString Path = FPaths::Combine(FPaths::ProjectDir(),
            TEXT("docs/Design/ARMEMENT01_9_6_STARTER_KIT_RECIPES.csv"));
        if (!FFileHelper::LoadFileToString(Csv, *Path))
        {
            Error = TEXT("Cannot load review-only starter-kit manifest.");
            return false;
        }
        FCsvParser Parser(Csv);
        const auto& Rows = Parser.GetRows();
        if (Rows.IsEmpty() || Rows[0].Num() != UE_ARRAY_COUNT(Headers))
        {
            Error = TEXT("Unexpected review manifest header width.");
            return false;
        }
        for (int32 I = 0; I < UE_ARRAY_COUNT(Headers); ++I)
        {
            if (FCString::Strcmp(Rows[0][I], Headers[I]) != 0)
            {
                Error = FString::Printf(TEXT("Header mismatch at column %d."), I);
                return false;
            }
        }
        for (int32 I = 1; I < Rows.Num(); ++I)
        {
            const TArray<const TCHAR*>& Row = Rows[I];
            if (Row.IsEmpty() || !Row[0] || !*Row[0]) continue;
            if (Row.Num() != UE_ARRAY_COUNT(Headers))
            {
                Error = FString::Printf(TEXT("Invalid manifest row %d."), I+1);
                return false;
            }
            FRecipe Entry;
            Entry.ClassId = Row[0];
            Entry.BranchId = Row[1];
            Entry.KitId = Row[2];
            Entry.Weapon1 = FName(Row[3]);
            Entry.Weapon1Level = FCString::Atoi(Row[4]);
            Entry.Weapon1Slot = Row[5];
            Entry.Weapon2 = Row[6][0] ? FName(Row[6]) : NAME_None;
            Entry.Weapon2Level = FCString::Atoi(Row[7]);
            Entry.Weapon2Slot = Row[8];
            Entry.SupportId = FName(Row[9]);
            Entry.SupportQuantity = FCString::Atoi(Row[10]);
            Entry.SupportNeed = Row[11];
            Entry.Readiness = Row[12];
            Entry.Approval = Row[13];
            Entry.Note = Row[14];
            if (Entry.ClassId.IsEmpty() || Entry.BranchId.IsEmpty() ||
                Entry.KitId.IsEmpty() || Entry.Weapon1.IsNone() ||
                Entry.Weapon1Slot != TEXT("MainHand") ||
                Entry.Weapon1Level != 1 || Entry.Note.IsEmpty() ||
                Entry.Approval != TEXT("ApprovedDesign") || Entry.Readiness.IsEmpty() ||
                (Entry.Weapon2.IsNone() != (Entry.Weapon2Level == 0)) ||
                (!Entry.Weapon2.IsNone() && Entry.Weapon2Level != 1) ||
                (Entry.Weapon2.IsNone() != (Entry.Weapon2Slot == TEXT("None"))) ||
                (Entry.SupportId.IsNone() != (Entry.SupportQuantity == 0)) ||
                (Entry.Weapon2Slot != TEXT("None") &&
                 Entry.Weapon2Slot != TEXT("OffHand") &&
                 Entry.Weapon2Slot != TEXT("Bag")))
            {
                Error = FString::Printf(TEXT("Invalid design recipe %s."), *Entry.KitId);
                return false;
            }
            Out.Add(MoveTemp(Entry));
        }
        if (Out.Num() != 18)
        {
            Error = FString::Printf(TEXT("Expected exactly 18 kit proposals, got %d."), Out.Num());
            return false;
        }
        return true;
    }

    URPGClassAsset* LoadClass(const FString& ClassId)
    {
        const FString Path = FString::Printf(
            TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_%s.DA_Class_%s"),
            *ClassId, *ClassId);
        return LoadObject<URPGClassAsset>(nullptr, *Path);
    }

    const UGridCanonicalWeaponCatalogueAsset* LoadWeapons()
    {
        UGridItemSystemAsset* Root =
            GetDefault<UGridItemSubsystem>()->LoadConfiguredItemSystemForPreview();
        return Root ? Root->WeaponCatalogue.Get() : nullptr;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0196Coverage,
    "Grimrock.RPG.ARMEMENT01.9.KitDesign.EighteenRealTalentBranches",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0196Coverage::RunTest(const FString&)
{
    TArray<ARMEMENT0196::FRecipe> Recipes;
    FString Error;
    if (!TestTrue(TEXT("18 proposed recipes load"),
        ARMEMENT0196::ReadRecipes(Recipes, Error)))
    {
        AddInfo(Error);
        return false;
    }
    TSet<FString> UniqueKitIds;
    TMap<FString, TSet<FString>> PerClass;
    for (const auto& Recipe : Recipes)
    {
        TestFalse(TEXT("New IDs must not duplicate a proposed kit"), UniqueKitIds.Contains(Recipe.KitId));
        UniqueKitIds.Add(Recipe.KitId);
        TestEqual(TEXT("Stable new namespaced ID, separate from 12 legacy kit IDs"),
            Recipe.KitId, FString::Printf(TEXT("Kit_%s_%s"), *Recipe.ClassId, *Recipe.BranchId));

        URPGClassAsset* Class = ARMEMENT0196::LoadClass(Recipe.ClassId);
        if (!TestNotNull(TEXT("Production class present"), Class)) return false;
        const bool bRealBranch = Class->ProgressionChoices.ContainsByPredicate(
            [&Recipe](const FRPGClassProgressionChoiceDefinition& Choice)
            {
                return Choice.TalentBranchId == FName(*Recipe.BranchId) &&
                    Choice.MinimumLevel == 2;
            });
        TestTrue(TEXT("Branch really exists and has level-two choice"), bRealBranch);
        TSet<FString>& Branches = PerClass.FindOrAdd(Recipe.ClassId);
        TestFalse(TEXT("Each class branch occurs once"), Branches.Contains(Recipe.BranchId));
        Branches.Add(Recipe.BranchId);
    }
    TestEqual(TEXT("Six classes"), PerClass.Num(), 6);
    for (const auto& Pair : PerClass)
    {
        TestEqual(TEXT("Three kits per class"), Pair.Value.Num(), 3);
        URPGClassAsset* Class = ARMEMENT0196::LoadClass(Pair.Key);
        if (!TestNotNull(TEXT("Class for branch coverage"), Class)) return false;
        TSet<FString> RealBranches;
        for (const FRPGClassProgressionChoiceDefinition& Choice : Class->ProgressionChoices)
        {
            if (!Choice.TalentBranchId.IsNone())
            {
                RealBranches.Add(Choice.TalentBranchId.ToString());
            }
        }
        for (const FString& RealBranch : RealBranches)
        {
            TestTrue(TEXT("All actual class branches represented"), Pair.Value.Contains(RealBranch));
        }
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0196Catalogue,
    "Grimrock.RPG.ARMEMENT01.9.KitDesign.RealN1WeaponsAndAdvisoryAffinity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0196Catalogue::RunTest(const FString&)
{
    TArray<ARMEMENT0196::FRecipe> Recipes;
    FString Error;
    if (!TestTrue(TEXT("Approved starter recipe manifest exists"),
        ARMEMENT0196::ReadRecipes(Recipes, Error)))
        return false;
    FGridCanonicalRegistryFixture Fixture;
    if (!TestTrue(TEXT("Production ItemSystem is configured"), Fixture.bReady))
        return false;
    const UGridCanonicalWeaponCatalogueAsset* Catalogue = ARMEMENT0196::LoadWeapons();
    if (!TestNotNull(TEXT("Single canonical authoring weapon catalogue"), Catalogue))
        return false;
    TArray<FString> Errors;
    if (!TestTrue(TEXT("All 82 numeric/class affinity profiles valid"),
        Catalogue->Validate(Errors))) return false;
    TestEqual(TEXT("82 physical archetypes retained"), Catalogue->Archetypes.Num(), 82);
    TestEqual(TEXT("Six authored class preferences"), Catalogue->ClassPreferences.Num(), 6);

    TSet<FName> UniqueWeapons;
    for (const auto& Recipe : Recipes)
    {
        const FName Class(*Recipe.ClassId);
        const FGridCanonicalWeaponArchetype* Primary =
            Catalogue->Find(Recipe.Weapon1);
        if (!TestNotNull(TEXT("Primary numeric ARM exists at N1"), Primary))
            continue;
        FGridCanonicalResolvedWeapon N1;
        TestTrue(TEXT("Primary authoring resolves N1 and a MainHand policy"),
            Primary->TryResolve(1, N1) &&
            N1.CanEquipInHand(EGridEquipmentSlot::MainHand));
        TestTrue(TEXT("Exact approved primary/secondary class affinity"),
            Primary->PrimaryClassId == Class || Primary->SecondaryClassIds.Contains(Class));
        UniqueWeapons.Add(Recipe.Weapon1);
        if (!Recipe.Weapon2.IsNone())
        {
            const FGridCanonicalWeaponArchetype* Second =
                Catalogue->Find(Recipe.Weapon2);
            if (!TestNotNull(TEXT("Second numeric ARM exists at N1"), Second))
                continue;
            FGridCanonicalResolvedWeapon N2;
            TestTrue(TEXT("Secondary physical level resolves from native catalogue"),
                Second->TryResolve(Recipe.Weapon2Level, N2));
            TestTrue(TEXT("Second has actual primary/secondary class advice"),
                Second->PrimaryClassId == Class || Second->SecondaryClassIds.Contains(Class));
            if (Recipe.Weapon2Slot == TEXT("OffHand"))
            {
                TestFalse(TEXT("Two-hand primary cannot hold an equipped OffHand item"),
                    N1.HandPolicy == EGridCanonicalHandPolicy::TwoHanded);
                TestTrue(TEXT("OffHand item is physically authorized"),
                    N2.CanEquipInHand(EGridEquipmentSlot::OffHand));
                TestFalse(TEXT("OffHand item may not itself occupy both hands"),
                    N2.HandPolicy == EGridCanonicalHandPolicy::TwoHanded);
            }
            UniqueWeapons.Add(Recipe.Weapon2);
        }
    }
    TestEqual(TEXT("22 unique approved N1 archetypes"), UniqueWeapons.Num(), 22);
    const FGridCanonicalWeaponArchetype* Dagger = Catalogue->Find(TEXT("ARM001"));
    if (TestNotNull(TEXT("One known Rogue dagger class advice"), Dagger))
        TestTrue(TEXT("Warrior does not acquire the Rogue dagger talent by fallback"),
            Dagger->PrimaryClassId != FName(TEXT("Warrior")) &&
            !Dagger->SecondaryClassIds.Contains(TEXT("Warrior")));
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0196Compatibility,
    "Grimrock.RPG.ARMEMENT01.9.KitDesign.NoLegacyKitOrSaveMutation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0196Compatibility::RunTest(const FString&)
{
    TArray<ARMEMENT0196::FRecipe> Recipes;
    FString Error;
    if (!TestTrue(TEXT("Manifest exists"), ARMEMENT0196::ReadRecipes(Recipes, Error)))
    {
        AddInfo(Error);
        return false;
    }
    // Four production N1 support items now exist: Scout torch,
    // Grenadier bomb, Transmuter oil and Mage SurfaceWeaver catalyst.
    // Check exact kit/quantity rather than preserving the obsolete count.
    const TMap<FString, FName> ExpectedSupport = {
        {TEXT("Kit_Ranger_Scout"), FName(TEXT("Item_Torch"))},
        {TEXT("Kit_Mage_SurfaceWeaver"), FName(TEXT("Item_SurfaceCatalyst_01"))},
        {TEXT("Kit_Alchemist_Grenadier"), FName(TEXT("Item_Bomb_Fire"))},
        {TEXT("Kit_Alchemist_Transmuter"), FName(TEXT("Item_Flask_Oil"))}
    };
    int32 WithSupport = 0;
    int32 WithPendingSupport = 0;
    for (const auto& Recipe : Recipes)
    {
        URPGClassAsset* Class = ARMEMENT0196::LoadClass(Recipe.ClassId);
        if (!TestNotNull(TEXT("Class asset"), Class)) return false;
        TestFalse(TEXT("Design-only namespaced kit must not already be a runtime kit"),
            Class->FindStartingEquipmentKit(FName(*Recipe.KitId)) != nullptr);

        const FName* ExpectedItemId = ExpectedSupport.Find(Recipe.KitId);
        if (ExpectedItemId)
        {
            ++WithSupport;
            TestEqual(TEXT("Approved kit has exactly the expected known support item"),
                Recipe.SupportId, *ExpectedItemId);
            TestEqual(TEXT("Approved starter support is a single item"),
                Recipe.SupportQuantity, 1);
        }
        else
        {
            TestTrue(TEXT("Other kits do not invent a known support item"), Recipe.SupportId.IsNone());
            TestEqual(TEXT("Other kits have no known support quantity"), Recipe.SupportQuantity, 0);
        }
        if (Recipe.SupportNeed != TEXT("None")) ++WithPendingSupport;
        TestEqual(TEXT("Design recipes are approved, not activated"), Recipe.Approval, FString(TEXT("ApprovedDesign")));
    }
    TestEqual(TEXT("Known support item coverage follows the four approved kits"), WithSupport, ExpectedSupport.Num());
    TestEqual(TEXT("Four kits still declare a pending support need or component"),
        WithPendingSupport, 4);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
