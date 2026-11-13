#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "UObject/UnrealType.h"
#include "UObject/StrongObjectPtr.h"

namespace Clean061
{
	struct FFixture
	{
		// Automation tests may trigger GC between fixtures or validation passes.
		// Keep the entire transient catalogue object graph strongly referenced.
		TStrongObjectPtr<UGridItemSystemAsset> Owner;
		TStrongObjectPtr<UGameInstance> GameInstance;
		UGridItemSystemAsset* System = nullptr;
		UGridArmamentDefinitionAsset* Sword = nullptr;
		UGridArmamentDefinitionAsset* Shield = nullptr;
		UGridConsumableDefinitionAsset* Potion = nullptr;
		UGridAmmoDefinitionAsset* Ammo = nullptr;
	};

	FFixture Make()
	{
		FFixture F;
		F.System = NewObject<UGridItemSystemAsset>();
		F.Owner.Reset(F.System);
		// UGameInstanceSubsystem is Within=GameInstance in UE 5.5.4.
		// Keep a real owner rooted for the lifetime of each isolated fixture.
		F.GameInstance.Reset(NewObject<UGameInstance>());
		F.System->WeaponCatalogue = NewObject<UGridCanonicalWeaponCatalogueAsset>(F.System);
		F.System->WeaponVisualCatalogue = NewObject<UGridCanonicalWeaponVisualCatalogueAsset>(F.System);
		F.System->ActionCatalogue = NewObject<UGridActionCatalogueAsset>(F.System);
		F.System->ActionPresentationCatalogue = NewObject<UGridActionPresentationCatalogueAsset>(F.System);

		FGridCanonicalAction AttackAction;
		AttackAction.ActionId = TEXT("BasicStrike");
		AttackAction.Kind = EGridCanonicalActionKind::Attack;
		F.System->ActionCatalogue->Actions.Add(AttackAction);

		FGridCanonicalActionPresentation Presentation;
		Presentation.PresentationId = TEXT("BasicMotion");
		F.System->ActionPresentationCatalogue->Presentations.Add(Presentation);

		FGridCanonicalWeaponVisual Visual;
		Visual.VisualId = TEXT("CommonSilhouette");
		F.System->WeaponVisualCatalogue->VisualProfiles.Add(Visual);
		for (int32 Index = 1; Index <= 82; ++Index)
		{
			FGridCanonicalWeaponArchetype Weapon;
			Weapon.ArchetypeId = FName(*FString::Printf(TEXT("ARM%03d"), Index));
			// Every synthetic row has its hand and kind explicitly authored in
			// this fixture; real unmaterialized source rows remain unproven.
			Weapon.bHandAndKindProven = true;
			const bool bPassive = Index >= 70; // 13 x 20 = 260
			Weapon.Kind = bPassive ? EGridCanonicalArmamentKind::Shield :
				EGridCanonicalArmamentKind::Weapon;
			Weapon.ActionPointCost = bPassive ? 0 : 2;
			Weapon.RangeCells = bPassive ? 0 : 1;
			Weapon.DamageDiceN1.bEnabled = !bPassive;
			Weapon.DamageDiceN1.DiceCount = bPassive ? 0 : 2;
			Weapon.DamageDiceN1.DiceSides = bPassive ? 0 : 8;
			Weapon.DamageDiceN1.FlatBonus = bPassive ? 0 : 1;
			Weapon.PhysicalArmorN1 = bPassive ? 3 : 0;
			Weapon.FlatBonusGrowth.StepEveryLevels = 1;
			Weapon.FlatBonusGrowth.StepAmount = bPassive ? 0 : 1;
			if (Index == 14)
			{
				FGridItemActionBinding Binding;
				Binding.ActionId = TEXT("BasicStrike");
				Binding.PresentationId = TEXT("BasicMotion");
				Weapon.Actions.Add(Binding);
				Weapon.AttackSourceSlots.Add(EGridEquipmentSlot::MainHand);
			}
			if (Index == 33)
			{
				Weapon.RequiredAmmoKindId = TEXT("Arrow");
			}
			F.System->WeaponCatalogue->Archetypes.Add(Weapon);

			FGridCanonicalWeaponVisualBinding Binding;
			Binding.ArchetypeId = Weapon.ArchetypeId;
			Binding.VisualId = Visual.VisualId;
			F.System->WeaponVisualCatalogue->ArchetypeBindings.Add(Binding);
		}

		F.Sword = NewObject<UGridArmamentDefinitionAsset>(F.System);
		F.Sword->ItemId = TEXT("TestSword");
		F.Sword->ArchetypeId = TEXT("ARM014");
		F.Sword->DisplayName = FText::FromString(TEXT("Sword"));
		F.Sword->WeightKg = 2.75f;
		F.System->ItemDefinitions.Add(F.Sword);

		F.Shield = NewObject<UGridArmamentDefinitionAsset>(F.System);
		F.Shield->ItemId = TEXT("TestShield");
		F.Shield->ArchetypeId = TEXT("ARM075");
		F.Shield->WeightKg = 3.5f;
		F.System->ItemDefinitions.Add(F.Shield);

		F.Potion = NewObject<UGridConsumableDefinitionAsset>(F.System);
		F.Potion->ItemId = TEXT("TestPotion");
		F.Potion->WeightKg = 0.3f;
		F.Potion->MaxStack = 12;
		FGridItemActionBinding UseBinding;
		UseBinding.ActionId = TEXT("BasicStrike");
		UseBinding.PresentationId = TEXT("BasicMotion");
		F.Potion->UseActions.Add(UseBinding);
		F.System->ItemDefinitions.Add(F.Potion);

		F.Ammo = NewObject<UGridAmmoDefinitionAsset>(F.System);
		F.Ammo->ItemId = TEXT("TestArrow");
		F.Ammo->AmmoKindId = TEXT("Arrow");
		F.Ammo->MaxStack = 50;
		F.System->ItemDefinitions.Add(F.Ammo);
		return F;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean061CanonicalSchemaTest,
	"Grimrock.RPG.ARM.CLEAN06_1.CanonicalSchema",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean061CanonicalSchemaTest::RunTest(const FString&)
{
	int32 BaseFields = 0;
	for (TFieldIterator<FProperty> Property(UGridCanonicalItemDefinitionAsset::StaticClass(),
		EFieldIteratorFlags::ExcludeSuper); Property; ++Property)
	{
		++BaseFields;
	}
	int32 ArmFields = 0;
	for (TFieldIterator<FProperty> Property(UGridArmamentDefinitionAsset::StaticClass(),
		EFieldIteratorFlags::ExcludeSuper); Property; ++Property)
	{
		++ArmFields;
	}
	TestEqual(TEXT("Canonical item exactly five editable fields"), BaseFields, 5);
	TestEqual(TEXT("Armament adds only ArchetypeId"), ArmFields, 1);
	const Clean061::FFixture F = Clean061::Make();
	TArray<FString> Errors;
	TestTrue(TEXT("82/20 and 260 passives valid"), F.System->Validate(Errors));
	TestEqual(TEXT("No referential errors"), Errors.Num(), 0);
	TestEqual(TEXT("No catalogue pointers on weapon"), ArmFields, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean061CanonicalResolutionTest,
	"Grimrock.RPG.ARM.CLEAN06_1.SingleResolver.Registry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean061CanonicalResolutionTest::RunTest(const FString&)
{
	const Clean061::FFixture F = Clean061::Make();
	TStrongObjectPtr<UGridItemSubsystem> Subsystem(NewObject<UGridItemSubsystem>(F.GameInstance.Get()));
	FGridResolvedItem Result;
	TestTrue(TEXT("Subsystem belongs to its GameInstance"),
		Subsystem->GetGameInstance() == F.GameInstance.Get());
	TestFalse(TEXT("Resolver fails closed without registry"),
		Subsystem->ResolveItem(TEXT("TestSword"), 1, Result));
	TestEqual(TEXT("Explicit configuration failure reason"), Result.Error,
		EGridCanonicalItemResolveError::RegistryNotConfigured);
	TArray<FString> Errors;
	if (!TestTrue(TEXT("Register exactly one canonical asset"), Subsystem->RegisterItemSystem(F.System, Errors)))
	{
		return false;
	}
	TestFalse(TEXT("Second registry registration rejected"),
		Subsystem->RegisterItemSystem(F.System, Errors));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean061SwordResolutionTest,
	"Grimrock.RPG.ARM.CLEAN06_1.SingleResolver.SwordN20",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean061SwordResolutionTest::RunTest(const FString&)
{
	const Clean061::FFixture F = Clean061::Make();
	const FString WorldPath(TEXT("/Game/Tests/ARM062/TestSwordWorld.TestSwordWorld"));
	F.System->WeaponVisualCatalogue->VisualProfiles[0].WorldMesh =
		TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(WorldPath));
	F.System->WeaponVisualCatalogue->VisualProfiles[0].EquippedMesh.Reset();
	TStrongObjectPtr<UGridItemSubsystem> Subsystem(NewObject<UGridItemSubsystem>(F.GameInstance.Get()));
	TArray<FString> Errors;
	if (!TestTrue(TEXT("Register fixture"), Subsystem->RegisterItemSystem(F.System, Errors))) return false;
	FGridResolvedItem Result;
	if (!TestTrue(TEXT("Sword level 20 resolved"),
		Subsystem->ResolveItem(TEXT("TestSword"), 20, Result))) return false;
	TestEqual(TEXT("Item owns weight, not N1"), Result.WeightKg, 2.75f);
	TestEqual(TEXT("Archetype one ID"), Result.ArchetypeId, FName(TEXT("ARM014")));
	TestTrue(TEXT("Sword has dice"), Result.Weapon.DamageDice.bEnabled);
	TestEqual(TEXT("N20 growth is resolved"), Result.Weapon.DamageDice.FlatBonus, 20);
	TestEqual(TEXT("Attack cost stays in archetype"), Result.Weapon.ActionPointCost, 2);
	TestEqual(TEXT("Equipped mesh resolves from WorldMesh without duplicating an asset field"),
		Result.EquippedMesh.ToSoftObjectPath().ToString(), WorldPath);
	TestEqual(TEXT("Authored action exists"), Result.AvailableActions.Num(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean061PassiveResolutionTest,
	"Grimrock.RPG.ARM.CLEAN06_1.SingleResolver.Shield",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean061PassiveResolutionTest::RunTest(const FString&)
{
	const Clean061::FFixture F = Clean061::Make();
	TStrongObjectPtr<UGridItemSubsystem> Subsystem(NewObject<UGridItemSubsystem>(F.GameInstance.Get()));
	TArray<FString> Errors;
	if (!TestTrue(TEXT("Register fixture"), Subsystem->RegisterItemSystem(F.System, Errors))) return false;
	FGridResolvedItem Result;
	if (!TestTrue(TEXT("Shield level 20 resolves"),
		Subsystem->ResolveItem(TEXT("TestShield"), 20, Result))) return false;
	TestFalse(TEXT("Shield has no fake attack dice"), Result.Weapon.DamageDice.bEnabled);
	TestEqual(TEXT("No shield damage dice"), Result.Weapon.DamageDice.DiceCount, 0);
	TestEqual(TEXT("Shield passive armor"), Result.Weapon.PhysicalArmor, 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean061ConsumableResolutionTest,
	"Grimrock.RPG.ARM.CLEAN06_1.SingleResolver.Consumable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean061ConsumableResolutionTest::RunTest(const FString&)
{
	const Clean061::FFixture F = Clean061::Make();
	TStrongObjectPtr<UGridItemSubsystem> Subsystem(NewObject<UGridItemSubsystem>(F.GameInstance.Get()));
	TArray<FString> Errors;
	if (!TestTrue(TEXT("Register fixture"), Subsystem->RegisterItemSystem(F.System, Errors))) return false;
	FGridResolvedItem Result;
	if (!TestTrue(TEXT("Consumable resolves"), Subsystem->ResolveItem(TEXT("TestPotion"), 1, Result)))
	{
		return false;
	}
	TestEqual(TEXT("Consumable stack"), Result.MaxStack, 12);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean061AmmoResolutionTest,
	"Grimrock.RPG.ARM.CLEAN06_1.SingleResolver.AmmoResolve",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean061AmmoResolutionTest::RunTest(const FString&)
{
	const Clean061::FFixture F = Clean061::Make();
	TStrongObjectPtr<UGridItemSubsystem> Subsystem(NewObject<UGridItemSubsystem>(F.GameInstance.Get()));
	TArray<FString> Errors;
	if (!TestTrue(TEXT("Register fixture"), Subsystem->RegisterItemSystem(F.System, Errors))) return false;
	FGridResolvedItem Result;
	return TestTrue(TEXT("Ammo resolves"), Subsystem->ResolveItem(TEXT("TestArrow"), 1, Result));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean061AmmoIdentityTest,
	"Grimrock.RPG.ARM.CLEAN06_1.SingleResolver.AmmoKind",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean061AmmoIdentityTest::RunTest(const FString&)
{
	const Clean061::FFixture F = Clean061::Make();
	TStrongObjectPtr<UGridItemSubsystem> Subsystem(NewObject<UGridItemSubsystem>(F.GameInstance.Get()));
	TArray<FString> Errors;
	if (!TestTrue(TEXT("Register fixture"), Subsystem->RegisterItemSystem(F.System, Errors))) return false;
	FGridResolvedItem Result;
	if (!TestTrue(TEXT("Ammo resolves"), Subsystem->ResolveItem(TEXT("TestArrow"), 1, Result)))
	{
		return false;
	}
	TestEqual(TEXT("Resolved ammo kind equals its single source"), Result.AmmoKindId, F.Ammo->AmmoKindId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean061AmmoSourceTest,
	"Grimrock.RPG.ARM.CLEAN06_1.SingleResolver.AmmoSource",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean061AmmoSourceTest::RunTest(const FString&)
{
	const Clean061::FFixture F = Clean061::Make();
	TestEqual(TEXT("Authored source ammo kind"), F.Ammo->AmmoKindId, FName(TEXT("Arrow")));
	TestEqual(TEXT("Authored source item id"), F.Ammo->ItemId, FName(TEXT("TestArrow")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean061AmmoTypeTest,
	"Grimrock.RPG.ARM.CLEAN06_1.SingleResolver.AmmoType",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean061AmmoTypeTest::RunTest(const FString&)
{
	const Clean061::FFixture F = Clean061::Make();
	const UGridCanonicalItemDefinitionAsset* Last =
		F.System->ItemDefinitions.IsEmpty() ? nullptr : F.System->ItemDefinitions.Last();
	TestNotNull(TEXT("Last registry item exists"), Last);
	TestNotNull(TEXT("Registered ammo retains subclass"), Cast<UGridAmmoDefinitionAsset>(Last));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean061AmmoOutTypeTest,
	"Grimrock.RPG.ARM.CLEAN06_1.SingleResolver.AmmoOutType",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean061AmmoOutTypeTest::RunTest(const FString&)
{
	const Clean061::FFixture F = Clean061::Make();
	TStrongObjectPtr<UGridItemSubsystem> Subsystem(NewObject<UGridItemSubsystem>(F.GameInstance.Get()));
	TArray<FString> Errors;
	if (!TestTrue(TEXT("Register fixture"), Subsystem->RegisterItemSystem(F.System, Errors))) return false;
	FGridResolvedItem Result;
	if (!TestTrue(TEXT("Ammo resolves"), Subsystem->ResolveItem(TEXT("TestArrow"), 1, Result))) return false;
	TestEqual(TEXT("Resolved item id"), Result.ItemId, FName(TEXT("TestArrow")));
	TestFalse(TEXT("Ammo must not resolve as armament"), Result.bIsArmament);
	TestEqual(TEXT("MaxStack from ammo definition"), Result.MaxStack, 50);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean061AmmoAfterRegisterTest,
	"Grimrock.RPG.ARM.CLEAN06_1.SingleResolver.AmmoAfterRegister",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean061AmmoAfterRegisterTest::RunTest(const FString&)
{
	const Clean061::FFixture F = Clean061::Make();
	TStrongObjectPtr<UGridItemSubsystem> Subsystem(NewObject<UGridItemSubsystem>(F.GameInstance.Get()));
	TArray<FString> Errors;
	if (!TestTrue(TEXT("Register fixture"), Subsystem->RegisterItemSystem(F.System, Errors))) return false;
	
	// A subsystem created without a GameInstance outer is transient in this
	// isolation test: keep it alive explicitly during registry validation.
	TestTrue(TEXT("Canonical subsystem remains rooted while registered"), Subsystem.IsValid());
	TestEqual(TEXT("Authored ammo kind after registration"), F.Ammo->AmmoKindId, FName(TEXT("Arrow")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean061AmmoNonemptyTest,
	"Grimrock.RPG.ARM.CLEAN06_1.SingleResolver.AmmoNonempty",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean061AmmoNonemptyTest::RunTest(const FString&)
{
	const Clean061::FFixture F = Clean061::Make();
	TStrongObjectPtr<UGridItemSubsystem> Subsystem(NewObject<UGridItemSubsystem>(F.GameInstance.Get()));
	TArray<FString> Errors;
	if (!TestTrue(TEXT("Register fixture"), Subsystem->RegisterItemSystem(F.System, Errors))) return false;
	FGridResolvedItem Result;
	if (!TestTrue(TEXT("Resolve test ammo"), Subsystem->ResolveItem(TEXT("TestArrow"), 1, Result))) return false;
	TestFalse(TEXT("Resolved ammo kind nonempty"), Result.AmmoKindId.IsNone());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean061AmmoTextTest,
	"Grimrock.RPG.ARM.CLEAN06_1.SingleResolver.AmmoText",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean061AmmoTextTest::RunTest(const FString&)
{
	const Clean061::FFixture F = Clean061::Make();
	TStrongObjectPtr<UGridItemSubsystem> Subsystem(NewObject<UGridItemSubsystem>(F.GameInstance.Get()));
	TArray<FString> Errors;
	if (!TestTrue(TEXT("Register fixture"), Subsystem->RegisterItemSystem(F.System, Errors))) return false;
	FGridResolvedItem Result;
	if (!TestTrue(TEXT("Resolve test ammo"), Subsystem->ResolveItem(TEXT("TestArrow"), 1, Result))) return false;
	TestEqual(TEXT("Resolved AmmoKindId spelling"), Result.AmmoKindId.ToString(), FString(TEXT("Arrow")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean061RejectionTest,
	"Grimrock.RPG.ARM.CLEAN06_1.SingleResolver.Rejections",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean061RejectionTest::RunTest(const FString&)
{
	const Clean061::FFixture F = Clean061::Make();
	TStrongObjectPtr<UGridItemSubsystem> Subsystem(NewObject<UGridItemSubsystem>(F.GameInstance.Get()));
	TArray<FString> Errors;
	if (!TestTrue(TEXT("Register fixture"), Subsystem->RegisterItemSystem(F.System, Errors))) return false;
	FGridResolvedItem Result;
	TestFalse(TEXT("Missing ItemId rejected"), Subsystem->ResolveItem(TEXT("NeverExists"), 1, Result));
	TestEqual(TEXT("No fallback on unknown ID"), Result.Error,
		EGridCanonicalItemResolveError::UnknownItemId);
	TestFalse(TEXT("Level 0 rejected, no legacy alias"),
		Subsystem->ResolveItem(TEXT("TestSword"), 0, Result));
	TestEqual(TEXT("Invalid level diagnostic"), Result.Error,
		EGridCanonicalItemResolveError::InvalidItemLevel);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean061ReferentialIntegrityTest,
	"Grimrock.RPG.ARM.CLEAN06_1.ReferentialIntegrity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean061ReferentialIntegrityTest::RunTest(const FString&)
{
	Clean061::FFixture F = Clean061::Make();
	TArray<FString> Errors;
	F.System->ItemDefinitions.Add(F.Sword);
	TestFalse(TEXT("Duplicate ItemId rejected"), F.System->Validate(Errors));
	F.System->ItemDefinitions.Pop();

	Errors.Reset();
	F.Shield->ArchetypeId = TEXT("ARM999");
	TestFalse(TEXT("Unknown ARM rejected"), F.System->Validate(Errors));
	F.Shield->ArchetypeId = TEXT("ARM075");

	Errors.Reset();
	F.System->WeaponCatalogue->Archetypes[0].ArchetypeId = TEXT("ARM014");
	TestFalse(TEXT("Duplicate ARM rejected"), F.System->Validate(Errors));
	F.System->WeaponCatalogue->Archetypes[0].ArchetypeId = TEXT("ARM001");

	Errors.Reset();
	F.System->WeaponVisualCatalogue->ArchetypeBindings[0].VisualId = TEXT("MissingVisual");
	TestFalse(TEXT("Unknown visual reference rejected"), F.System->Validate(Errors));
	F.System->WeaponVisualCatalogue->ArchetypeBindings[0].VisualId = TEXT("CommonSilhouette");

	Errors.Reset();
	F.System->WeaponCatalogue->Archetypes[13].Actions[0].ActionId = TEXT("NotInCatalogue");
	TestFalse(TEXT("Action foreign key rejected"), F.System->Validate(Errors));
	F.System->WeaponCatalogue->Archetypes[13].Actions[0].ActionId = TEXT("BasicStrike");

	Errors.Reset();
	F.System->WeaponCatalogue->Archetypes[74].DamageDiceN1.bEnabled = true;
	TestFalse(TEXT("Fake shield offensive profile rejected"), F.System->Validate(Errors));
	F.System->WeaponCatalogue->Archetypes[74].DamageDiceN1.bEnabled = false;
	Errors.Reset();
	TestTrue(TEXT("Restored fixture remains consistent"), F.System->Validate(Errors));
	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean062IBoundActionResolutionTest,
	"Grimrock.RPG.ARM.CLEAN06_2.Canonical.SingleBoundActionResolver",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean062IBoundActionResolutionTest::RunTest(const FString&)
{
	const Clean061::FFixture F = Clean061::Make();
	TStrongObjectPtr<UGridItemSubsystem> Subsystem(
		NewObject<UGridItemSubsystem>(F.GameInstance.Get()));
	FGridResolvedBoundAction Out;
	TestFalse(TEXT("Unconfigured subsystem refuses actions"),
		Subsystem->ResolveBoundAction(TEXT("TestSword"), 20, TEXT("BasicStrike"), Out));
	TestEqual(TEXT("Unconfigured error is explicit"), Out.Error,
		EGridCanonicalBoundActionError::RegistryNotConfigured);

	TArray<FString> Errors;
	if (!TestTrue(TEXT("Canonical registry is installed exactly once"),
		Subsystem->RegisterItemSystem(F.System, Errors)))
	{
		return false;
	}
	if (!TestTrue(TEXT("Sword N20 bound action resolved"),
		Subsystem->ResolveBoundAction(TEXT("TestSword"), 20, TEXT("BasicStrike"), Out)))
	{
		return false;
	}
	TestEqual(TEXT("ItemId stays with source"), Out.ItemId, FName(TEXT("TestSword")));
	TestEqual(TEXT("Level stays with source"), Out.ItemLevel, 20);
	TestEqual(TEXT("ActionId is exact"), Out.Action.ActionId, FName(TEXT("BasicStrike")));
	TestEqual(TEXT("Presentation ID is authored"), Out.Presentation.PresentationId,
		FName(TEXT("BasicMotion")));
	TestEqual(TEXT("No action-side duplicate of Sword action points"),
		Out.Action.ManaCost, 0);
	TestTrue(TEXT("Bound action is valid"), Out.IsValid());

	TestFalse(TEXT("Shield does not inherit Sword attack"),
		Subsystem->ResolveBoundAction(TEXT("TestShield"), 20, TEXT("BasicStrike"), Out));
	TestEqual(TEXT("Shield has no action binding"), Out.Error,
		EGridCanonicalBoundActionError::ActionNotBound);
	TestTrue(TEXT("Failure clears previous output action identity"), Out.ActionId.IsNone());
	TestTrue(TEXT("Failure clears previous output presentation identity"), Out.PresentationId.IsNone());

	TestFalse(TEXT("Ammo cannot borrow any action"),
		Subsystem->ResolveBoundAction(TEXT("TestArrow"), 1, TEXT("BasicStrike"), Out));
	TestEqual(TEXT("Ammo has no bound actions"), Out.Error,
		EGridCanonicalBoundActionError::ActionNotBound);

	TestFalse(TEXT("Unknown action ID does not fall back"),
		Subsystem->ResolveBoundAction(TEXT("TestSword"), 1, TEXT("UnknownAction"), Out));
	TestEqual(TEXT("No unknown ActionId lookup"), Out.Error,
		EGridCanonicalBoundActionError::ActionNotBound);

	TestFalse(TEXT("Invalid ItemLevel is rejected"),
		Subsystem->ResolveBoundAction(TEXT("TestSword"), 21, TEXT("BasicStrike"), Out));
	TestEqual(TEXT("Bad level fails at item resolver"), Out.Error,
		EGridCanonicalBoundActionError::ItemResolutionFailed);
	TestEqual(TEXT("Specific item error survives"), Out.ItemError,
		EGridCanonicalItemResolveError::InvalidItemLevel);

	TestFalse(TEXT("Unknown ItemId is rejected"),
		Subsystem->ResolveBoundAction(TEXT("MissingSword"), 1, TEXT("BasicStrike"), Out));
	TestEqual(TEXT("Unknown ItemId is explicit"), Out.ItemError,
		EGridCanonicalItemResolveError::UnknownItemId);

	TestTrue(TEXT("Potion usage has its own explicitly bound action"),
		Subsystem->ResolveBoundAction(TEXT("TestPotion"), 1, TEXT("BasicStrike"), Out));
	TestEqual(TEXT("Potion is the source"), Out.ItemId, FName(TEXT("TestPotion")));
	TestEqual(TEXT("Potion presentation is shared by ID only"), Out.PresentationId,
		FName(TEXT("BasicMotion")));
	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean062NLiveVisualReferencesTest,
	"Grimrock.RPG.ARM.CLEAN06_2.Canonical.LiveArmamentVisualReferences",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean062NLiveVisualReferencesTest::RunTest(const FString&)
{
	const Clean061::FFixture F = Clean061::Make();
	const int32 UnusedRemoved = F.System->WeaponVisualCatalogue->ArchetypeBindings.RemoveAll(
		[](const FGridCanonicalWeaponVisualBinding& Binding)
		{
			return Binding.ArchetypeId != FName(TEXT("ARM014")) &&
				Binding.ArchetypeId != FName(TEXT("ARM075"));
		});
	TestEqual(TEXT("Fixture removes exactly 80 unreferenced ARM visual bindings"),
		UnusedRemoved, 80);
	TArray<FString> Errors;
	TestTrue(TEXT("82 valid numeric ARM with only two concrete visuals are allowed"),
		F.System->Validate(Errors));

	const int32 LiveRemoved = F.System->WeaponVisualCatalogue->ArchetypeBindings.RemoveAll(
		[](const FGridCanonicalWeaponVisualBinding& Binding)
		{
			return Binding.ArchetypeId == FName(TEXT("ARM075"));
		});
	TestEqual(TEXT("One active shield binding removed"), LiveRemoved, 1);
	Errors.Reset();
	TestFalse(TEXT("A concrete shield with no visual is still invalid"),
		F.System->Validate(Errors));
	TestTrue(TEXT("Missing live visual reports an error"), Errors.Num() > 0);
	return UnusedRemoved == 80 && LiveRemoved == 1;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean062RArmSemanticGateTest,
	"Grimrock.RPG.ARM.CLEAN06_2.Canonical.ActiveArmamentSemanticGate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean062RArmSemanticGateTest::RunTest(const FString&)
{
	Clean061::FFixture F = Clean061::Make();
	const FGridCanonicalWeaponArchetype* Source =
		F.System->WeaponCatalogue->Find(TEXT("ARM014"));
	if (!TestNotNull(TEXT("ARM014 real linked slot of fixture"), Source))
	{
		return false;
	}
	FGridCanonicalWeaponArchetype* Active = F.System->WeaponCatalogue->Archetypes.FindByPredicate(
		[](const FGridCanonicalWeaponArchetype& Row) { return Row.ArchetypeId == TEXT("ARM014"); });
	FGridCanonicalWeaponArchetype* Unused = F.System->WeaponCatalogue->Archetypes.FindByPredicate(
		[](const FGridCanonicalWeaponArchetype& Row) { return Row.ArchetypeId == TEXT("ARM060"); });
	if (!TestNotNull(TEXT("ARM014 mutable fixture"), Active) ||
		!TestNotNull(TEXT("ARM060 numeric-only archetype"), Unused))
	{
		return false;
	}
	TArray<FString> Errors;
	TestTrue(TEXT("Fully authoried fixture has a valid registry"), F.System->Validate(Errors));

	Unused->bHandAndKindProven = false;
	Errors.Reset();
	TestTrue(TEXT("Unreferenced 60th numeric ARM needs no guessed hands"),
		F.System->Validate(Errors));

	Active->bHandAndKindProven = false;
	Errors.Reset();
	TestFalse(TEXT("Concrete ARM014 cannot enter registry with unproven hand policy"),
		F.System->Validate(Errors));
	TestTrue(TEXT("Unproven active item returns a rejection reason"), Errors.Num() > 0);

	Active->bHandAndKindProven = true;
	Errors.Reset();
	TestTrue(TEXT("Restored active semantics are valid"), F.System->Validate(Errors));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
