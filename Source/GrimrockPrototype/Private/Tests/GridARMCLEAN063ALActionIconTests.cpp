#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "../Runtime/Combat/GridCanonicalActionIconReadModel.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean063ALActionIcons,
    "Grimrock.RPG.ARM.CLEAN06_3.CanonicalEquippedActionIcons",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean063ALActionIcons::RunTest(const FString&)
{
    TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(nullptr,
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ItemSystem.DA_ItemSystem")));
    if (!TestNotNull(TEXT("Real ItemSystem asset"), Root.Get())) return false;
    TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
    TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
    TArray<FString> Errors;
    if (!TestTrue(TEXT("Register real ItemSystem"), Registry->RegisterItemSystem(Root.Get(), Errors)))
    {
        for (const FString& Error : Errors) AddError(Error);
        return false;
    }
    using Kind = EGridEquippedWeaponActionIconKind;
    TestTrue(TEXT("None never gives even a configured texture"),
        Registry->FindEquippedActionIcon(Kind::None).IsNull());

    struct FCase
    {
        FName ItemId;
        Kind ExpectedKind;
    };
    const FCase Cases[] = {
        {TEXT("Weapon_StraightSword_01"), Kind::MeleeOneHand},
        {TEXT("Weapon_GreatAxe_01"), Kind::MeleeTwoHanded},
        {TEXT("Weapon_ShortBow_01"), Kind::RangedBow},
        {TEXT("Weapon_ThrowingKnife_01"), Kind::ThrownWeapon},
        {TEXT("Weapon_MainGauche_01"), Kind::MeleeOneHand}
    };
    for (const FCase& Case : Cases)
    {
        for (const int32 Level : {1, 20})
        {
            FGridResolvedItem Item;
            if (!TestTrue(TEXT("Production ARM resolves at level"),
                Registry->ResolveItem(Case.ItemId, Level, Item))) return false;
            TestTrue(TEXT("A real production attack has a bound action"), !Item.AvailableActions.IsEmpty());
            bool bFoundExpected = false;
            for (const FGridItemActionBinding& Binding : Item.AvailableActions)
            {
                const FGridCanonicalAction* Action = Root->ActionCatalogue->Find(Binding.ActionId);
                if (!Action || Action->Kind != EGridCanonicalActionKind::Attack) continue;
                FGridCombatActionDefinition Transport;
                Transport.ActionId = Action->ActionId;
                Transport.SourcePolicy = EGridCombatActionSourcePolicy::Equipment;
                Transport.ResolutionProfile = EGridCombatActionResolutionProfile::Attack;
                Transport.ActionType = Action->bRangedAttack
                    ? EGridCombatActionType::RangedAttack : EGridCombatActionType::MeleeAttack;
                const Kind Classified = FGridCanonicalActionIconReadModel::Classify(Item, Transport);
                TestTrue(TEXT("Real attack has a nonempty visual category"), Classified != Kind::None);
                const auto Actual = Registry->FindEquippedActionIcon(Classified);
                const TSoftObjectPtr<UTexture2D>* Expected =
                    Root->WeaponVisualCatalogue->ActionIcons.Find(Classified);
                TestEqual(TEXT("Action image comes from canonical visual catalogue"),
                    Actual.ToSoftObjectPath().ToString(),
                    Expected ? Expected->ToSoftObjectPath().ToString() : FString());
                if (Classified == Case.ExpectedKind) bFoundExpected = true;
                Transport.SourcePolicy = EGridCombatActionSourcePolicy::Spell;
                TestEqual(TEXT("Spell cannot inherit an equipped weapon action icon"),
                    FGridCanonicalActionIconReadModel::Classify(Item, Transport), Kind::None);
                Transport.SourcePolicy = EGridCombatActionSourcePolicy::QuickItem;
                TestEqual(TEXT("Consumable cannot inherit equipped weapon icon"),
                    FGridCanonicalActionIconReadModel::Classify(Item, Transport), Kind::None);
            }
            TestTrue(TEXT("Production weapon retains its expected icon family"), bFoundExpected);
            FGridCombatActionDefinition Invented;
            Invented.ActionId = TEXT("FakeUnboundAttack_AL");
            Invented.SourcePolicy = EGridCombatActionSourcePolicy::Equipment;
            Invented.ResolutionProfile = EGridCombatActionResolutionProfile::Attack;
            Invented.ActionType = EGridCombatActionType::MeleeAttack;
            TestEqual(TEXT("Unbound imaginary attack cannot receive an icon"),
                FGridCanonicalActionIconReadModel::Classify(Item, Invented), Kind::None);
        }
    }
    FGridResolvedItem Shield;
    if (!TestTrue(TEXT("Real passive round shield exists"),
        Registry->ResolveItem(TEXT("Weapon_RoundShield_01"), 20, Shield))) return false;
    FGridCombatActionDefinition ForgedShieldAttack;
    ForgedShieldAttack.ActionId = TEXT("ForgedShieldAttack_AL");
    ForgedShieldAttack.SourcePolicy = EGridCombatActionSourcePolicy::Equipment;
    ForgedShieldAttack.ResolutionProfile = EGridCombatActionResolutionProfile::Attack;
    ForgedShieldAttack.ActionType = EGridCombatActionType::MeleeAttack;
    TestEqual(TEXT("Passive shield cannot invent offensive icon"),
        FGridCanonicalActionIconReadModel::Classify(Shield, ForgedShieldAttack), Kind::None);

    FGridResolvedItem OneHand;
    if (!TestTrue(TEXT("Real one-handed sword exists"),
        Registry->ResolveItem(TEXT("Weapon_StraightSword_01"), 1, OneHand))) return false;
    FGridCombatActionDefinition Composite;
    Composite.ActionId = FGridDualWieldPolicy::ActionId();
    Composite.SourcePolicy = EGridCombatActionSourcePolicy::Equipment;
    Composite.ResolutionProfile = EGridCombatActionResolutionProfile::Attack;
    Composite.ActionType = EGridCombatActionType::MeleeAttack;
    TestEqual(TEXT("Actually selected composite has one dual icon kind"),
        FGridCanonicalActionIconReadModel::Classify(OneHand, Composite), Kind::MeleeDualWield);
    return true;
}

#endif
