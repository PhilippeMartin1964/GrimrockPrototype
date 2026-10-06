#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Editor.h"
#include "EngineUtils.h"
#include "RPG/RPGCharacterRulesLibrary.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGClassProgressionTransactionService.h"
#include "RPG/RPGTalentRuntimeService.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Tests/AutomationCommon.h"
#include "Tests/AutomationEditorCommon.h"

namespace RPG0310PIE
{
	const FString MapPath = TEXT("/Game/GrimrockPrototype/Maps/L_Dungeon");

	struct FPIEClassSpec
	{
		FName ClassId;
		const TCHAR* ObjectPath = nullptr;
		const TCHAR* BranchToken = nullptr;
	};

	const FPIEClassSpec Specs[] = {
		{ TEXT("Warrior"), TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Warrior.DA_Class_Warrior"), TEXT("_Guardian_") },
		{ TEXT("Rogue"), TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Rogue.DA_Class_Rogue"), TEXT("_Assassin_") },
		{ TEXT("Ranger"), TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Ranger.DA_Class_Ranger"), TEXT("_Marksman_") },
		{ TEXT("Mage"), TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Mage.DA_Class_Mage"), TEXT("_Arcanist_") },
		{ TEXT("Priest"), TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Priest.DA_Class_Priest"), TEXT("_Restoration_") },
		{ TEXT("Alchemist"), TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Alchemist.DA_Class_Alchemist"), TEXT("_Grenadier_") }
	};

	UWorld* GetPIEWorld()
	{
		return GEditor ? GEditor->PlayWorld : nullptr;
	}

	bool BuildFiveTalentBranch(
		FAutomationTestBase& Test, const URPGClassAsset& ClassAsset, const TCHAR* BranchToken, TArray<FName>& OutChoices)
	{
		OutChoices.Reset();
		const int32 Levels[] = { 2, 6, 10, 14, 18 };
		FName Previous = NAME_None;
		for (const int32 Level : Levels)
		{
			TArray<const FRPGClassProgressionChoiceDefinition*> Candidates;
			for (const FRPGClassProgressionChoiceDefinition& Choice : ClassAsset.ProgressionChoices)
			{
				if (Choice.MinimumLevel == Level && Choice.ChoiceId.ToString().Contains(BranchToken))
				{
					Candidates.Add(&Choice);
				}
			}
			Test.TestEqual(
				*FString::Printf(TEXT("%s branch token %s has one choice at level %d"),
					*ClassAsset.ClassId.ToString(), BranchToken, Level),
				Candidates.Num(), 1);
			if (Candidates.Num() != 1)
			{
				return false;
			}
			const FRPGClassProgressionChoiceDefinition* Choice = Candidates[0];
			if (!Previous.IsNone())
			{
				Test.TestTrue(
					*FString::Printf(TEXT("%s branch level %d depends on the preceding choice"), *ClassAsset.ClassId.ToString(), Level),
					Choice->PrerequisiteChoiceIds.Contains(Previous));
			}
			OutChoices.Add(Choice->ChoiceId);
			Previous = Choice->ChoiceId;
		}
		return OutChoices.Num() == 5;
	}

	class FWaitForPIEWorld : public IAutomationLatentCommand
	{
	public:
		explicit FWaitForPIEWorld(FAutomationTestBase* InTest)
			: Test(InTest)
		{
		}

		virtual bool Update() override
		{
			if (StartSeconds <= 0.0)
			{
				StartSeconds = FPlatformTime::Seconds();
			}
			UWorld* World = GetPIEWorld();
			if (World && World->HasBegunPlay())
			{
				return true;
			}
			if (FPlatformTime::Seconds() - StartSeconds > 30.0)
			{
				Test->AddError(TEXT("RPG03.10 timed out waiting for the real PIE world."));
				return true;
			}
			return false;
		}

	private:
		FAutomationTestBase* Test = nullptr;
		double StartSeconds = 0.0;
	};

	class FCheckPIERuntime : public IAutomationLatentCommand
	{
	public:
		explicit FCheckPIERuntime(FAutomationTestBase* InTest)
			: Test(InTest)
		{
		}

		virtual bool Update() override
		{
			UWorld* World = GetPIEWorld();
			Test->TestNotNull(TEXT("RPG03.10 has a genuine PIE world"), World);
			if (!World)
			{
				return true;
			}
			Test->TestEqual(TEXT("RPG03.10 world type is PIE"), World->WorldType, EWorldType::PIE);

			TArray<AGridLevelRuntimeActor*> Runtimes;
			for (TActorIterator<AGridLevelRuntimeActor> It(World); It; ++It)
			{
				Runtimes.Add(*It);
			}
			Test->TestEqual(TEXT("L_Dungeon PIE has exactly one grid runtime actor"), Runtimes.Num(), 1);

			AGrimrockPartyPawn* Party = nullptr;
			int32 PartyCount = 0;
			for (TActorIterator<AGrimrockPartyPawn> It(World); It; ++It)
			{
				Party = *It;
				++PartyCount;
			}
			Test->TestEqual(TEXT("L_Dungeon PIE has exactly one party pawn"), PartyCount, 1);
			if (!Party || !Test->TestNotNull(TEXT("PIE party has its authoritative inventory component"), Party->PartyInventoryComponent.Get()))
			{
				return true;
			}

			FRPGClassProgressionTransactionService::ResetRuntimeState();
			UGridPartyInventoryComponent* Inventory = Party->PartyInventoryComponent;
			Inventory->PartyInventoryState = FGridPartyInventoryState();
			Inventory->PartyInventoryState.MaxActiveCharacters = 6;
			Inventory->PartyInventoryState.SelectedCharacterIndex = 0;

			TArray<TArray<FName>> Selections;
			for (const FPIEClassSpec& Spec : Specs)
			{
				URPGClassAsset* ClassAsset = LoadObject<URPGClassAsset>(nullptr, Spec.ObjectPath);
				if (!Test->TestNotNull(*FString::Printf(TEXT("PIE loads %s class asset"), *Spec.ClassId.ToString()), ClassAsset))
				{
					continue;
				}
				TArray<FName> Selection;
				if (!BuildFiveTalentBranch(*Test, *ClassAsset, Spec.BranchToken, Selection))
				{
					continue;
				}

				FGridCharacterInventoryState Character;
				Character.CharacterId = FGuid::NewGuid();
				Character.DisplayName = FText::FromName(Spec.ClassId);
				Character.ClassId = Spec.ClassId;
				Character.ClassDisplayName = ClassAsset->DisplayName;
				Character.ClassDefinition = ClassAsset;
				Character.Level = 20;
				Character.Experience = URPGCharacterRulesLibrary::GetCumulativeExperienceRequiredForLevel(20);
				Character.LastAcknowledgedLevel = 20;
				Character.Attributes = ClassAsset->BaseAttributes;
				Character.DerivedStats = URPGCharacterRulesLibrary::CalculateDerivedStats(Character.Attributes, ClassAsset, Character.Level);
				Character.Resources = URPGCharacterRulesLibrary::InitializeCharacterResources(Character.DerivedStats, ClassAsset);
				Character.SelectedClassProgressionChoiceIds = Selection;
				Inventory->PartyInventoryState.ActiveCharacters.Add(Character);
				Inventory->PartyInventoryState.ActiveEquipment.AddDefaulted();
				Selections.Add(MoveTemp(Selection));
			}

			Test->TestEqual(TEXT("PIE transient party contains all six RPG03 classes"),
				Inventory->PartyInventoryState.ActiveCharacters.Num(), 6);
			if (Inventory->PartyInventoryState.ActiveCharacters.Num() != 6 || Selections.Num() != 6)
			{
				return true;
			}

			for (int32 Index = 0; Index < 6; ++Index)
			{
				const FGridCharacterInventoryState& Character = Inventory->PartyInventoryState.ActiveCharacters[Index];
				Test->TestTrue(*FString::Printf(TEXT("%s runtime requirement projection rebuilds"), *Character.ClassId.ToString()),
					FRPGClassProgressionTransactionService::RefreshCharacterProjection(Inventory, Index));

				TArray<FRPGTalentRuntimeView> SelectedTalents;
				Test->TestTrue(*FString::Printf(TEXT("%s selected talents are readable in PIE"), *Character.ClassId.ToString()),
					FRPGTalentRuntimeService::TryGetSelectedTalents(Inventory, Index, SelectedTalents));
				Test->TestEqual(*FString::Printf(TEXT("%s exposes five selected talents in PIE"), *Character.ClassId.ToString()),
					SelectedTalents.Num(), 5);

				FRPGTalentPointBalance Balance;
				Test->TestTrue(*FString::Printf(TEXT("%s talent balance is readable in PIE"), *Character.ClassId.ToString()),
					FRPGTalentRuntimeService::TryGetTalentPointBalance(Inventory, Index, Balance));
				Test->TestEqual(TEXT("Level 20 grants ten Talent Points"), Balance.GrantedPoints, 10);
				Test->TestEqual(TEXT("A complete five-node branch spends five Talent Points"), Balance.SpentPoints, 5);
				Test->TestEqual(TEXT("Unspent Talent Points are preserved"), Balance.RemainingPoints, 5);

				TSet<FName> Requirements;
				FRPGClassProgressionTransactionService::AppendRuntimeSatisfiedRequirements(Character.CharacterId, Requirements);
				Test->TestTrue(TEXT("Runtime requirements contain the class identity"), Requirements.Contains(Character.ClassId));
				Test->TestTrue(TEXT("Runtime requirements contain the selected terminal talent"),
					Requirements.Contains(Selections[Index].Last()));
			}

			FRPGClassProgressionTransactionService::ResetRuntimeState(Inventory);
			Inventory->PartyInventoryState = FGridPartyInventoryState();
			return true;
		}

	private:
		FAutomationTestBase* Test = nullptr;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0310RealPIESmokeTest,
	"Grimrock.RPG.RPG03.10.PIE.SixClassTalentRuntime",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0310RealPIESmokeTest::RunTest(const FString&)
{
	ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(RPG0310PIE::MapPath));
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(RPG0310PIE::FWaitForPIEWorld(this));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.0f));
	ADD_LATENT_AUTOMATION_COMMAND(RPG0310PIE::FCheckPIERuntime(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.0f));
	return true;
}

#endif
