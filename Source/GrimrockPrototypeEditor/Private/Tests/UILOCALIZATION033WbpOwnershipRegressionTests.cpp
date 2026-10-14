#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace UILOCALIZATION033
{
	bool LoadMaterializedManifest(FAutomationTestBase& Test, FString& OutManifest)
	{
		const FString Filename = FPaths::Combine(
			FPaths::ProjectDir(),
			TEXT("Content/Localization/Game/Game.manifest"));

		if (!FFileHelper::LoadFileToString(OutManifest, *Filename))
		{
			Test.AddError(FString::Printf(TEXT("Unable to read materialized localization manifest '%s'."), *Filename));
			return false;
		}

		return true;
	}

	template <SIZE_T N>
	void RequirePathsAbsent(
		FAutomationTestBase& Test,
		const FString& Manifest,
		const TCHAR* GroupLabel,
		const TCHAR* const (&Paths)[N])
	{
		for (const TCHAR* Path : Paths)
		{
			Test.TestFalse(
				*FString::Printf(TEXT("%s legacy manifest identity stays removed: %s"), GroupLabel, Path),
				Manifest.Contains(Path));
		}
	}

	template <SIZE_T N>
	void RequirePathsPresent(
		FAutomationTestBase& Test,
		const FString& Manifest,
		const TCHAR* GroupLabel,
		const TCHAR* const (&Paths)[N])
	{
		for (const TCHAR* Path : Paths)
		{
			Test.TestTrue(
				*FString::Printf(TEXT("%s manifest identity remains intentionally present: %s"), GroupLabel, Path),
				Manifest.Contains(Path));
		}
	}

	const TCHAR* const RuntimePlaceholderPaths[] = {
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/Combat/WBP_GridCombatHud.WBP_GridCombatHud_C:WidgetTree.Text_EndTurnDisabledReason.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/Combat/WBP_GridCombatHud.WBP_GridCombatHud_C:WidgetTree.Text_MobilityActionPoints.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/Inventory/WBP_CharacterSheet.WBP_CharacterSheet_C:WidgetTree.TextBlock.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/Inventory/WBP_CharacterSheet.WBP_CharacterSheet_C:WidgetTree.Text_AttributePoints.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/Inventory/WBP_CharacterSheet.WBP_CharacterSheet_C:WidgetTree.Text_CharacterCharisma.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/Inventory/WBP_CharacterSheet.WBP_CharacterSheet_C:WidgetTree.Text_CharacterClass.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/Inventory/WBP_CharacterSheet.WBP_CharacterSheet_C:WidgetTree.Text_CharacterConstitution.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/Inventory/WBP_CharacterSheet.WBP_CharacterSheet_C:WidgetTree.Text_CharacterDexterity.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/Inventory/WBP_CharacterSheet.WBP_CharacterSheet_C:WidgetTree.Text_CharacterExperience.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/Inventory/WBP_CharacterSheet.WBP_CharacterSheet_C:WidgetTree.Text_CharacterIntelligence.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/Inventory/WBP_CharacterSheet.WBP_CharacterSheet_C:WidgetTree.Text_CharacterLevel.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/Inventory/WBP_CharacterSheet.WBP_CharacterSheet_C:WidgetTree.Text_CharacterName.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/Inventory/WBP_CharacterSheet.WBP_CharacterSheet_C:WidgetTree.Text_CharacterRace.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/Inventory/WBP_CharacterSheet.WBP_CharacterSheet_C:WidgetTree.Text_CharacterStrength.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/Inventory/WBP_CharacterSheet.WBP_CharacterSheet_C:WidgetTree.Text_CharacterWisdom.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/Inventory/WBP_ItemTooltipStatLine.WBP_ItemTooltipStatLine_C:WidgetTree.Text_StatDelta.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/MainMenu/WBP_LoadGameSlotRow.WBP_LoadGameSlotRow_C:WidgetTree.Text_DisplayName.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/MainMenu/WBP_LoadGameSlotRow.WBP_LoadGameSlotRow_C:WidgetTree.Text_SlotName.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/MainMenu/WBP_LoadGameSlotRow.WBP_LoadGameSlotRow_C:WidgetTree.Text_Status.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/RPG/WBP_CCStep_Attributes.WBP_CCStep_Attributes_C:WidgetTree.Text_AttributePointsRemaining.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/RPG/WBP_CCStep_Attributes.WBP_CCStep_Attributes_C:WidgetTree.Text_CharismaClassValue.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/RPG/WBP_CCStep_Attributes.WBP_CCStep_Attributes_C:WidgetTree.Text_CharismaModifier.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/RPG/WBP_CCStep_Attributes.WBP_CCStep_Attributes_C:WidgetTree.Text_CharismaRaceBonus.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/RPG/WBP_CCStep_Attributes.WBP_CCStep_Attributes_C:WidgetTree.Text_CharismaTotalValue.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/RPG/WBP_CCStep_Attributes.WBP_CCStep_Attributes_C:WidgetTree.Text_ConstitutionClassValue.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/RPG/WBP_CCStep_Attributes.WBP_CCStep_Attributes_C:WidgetTree.Text_ConstitutionModifier.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/RPG/WBP_CCStep_Attributes.WBP_CCStep_Attributes_C:WidgetTree.Text_ConstitutionRaceBonus.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/RPG/WBP_CCStep_Attributes.WBP_CCStep_Attributes_C:WidgetTree.Text_ConstitutionTotalValue.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/RPG/WBP_CCStep_Attributes.WBP_CCStep_Attributes_C:WidgetTree.Text_DexterityClassValue.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/RPG/WBP_CCStep_Attributes.WBP_CCStep_Attributes_C:WidgetTree.Text_DexterityModifier.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/RPG/WBP_CCStep_Attributes.WBP_CCStep_Attributes_C:WidgetTree.Text_DexterityRaceBonus.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/RPG/WBP_CCStep_Attributes.WBP_CCStep_Attributes_C:WidgetTree.Text_DexterityTotalValue.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/RPG/WBP_CCStep_Attributes.WBP_CCStep_Attributes_C:WidgetTree.Text_IntelligenceClassValue.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/RPG/WBP_CCStep_Attributes.WBP_CCStep_Attributes_C:WidgetTree.Text_IntelligenceModifier.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/RPG/WBP_CCStep_Attributes.WBP_CCStep_Attributes_C:WidgetTree.Text_IntelligenceRaceBonus.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/RPG/WBP_CCStep_Attributes.WBP_CCStep_Attributes_C:WidgetTree.Text_IntelligenceTotalValue.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/RPG/WBP_CCStep_Attributes.WBP_CCStep_Attributes_C:WidgetTree.Text_StrengthClassValue.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/RPG/WBP_CCStep_Attributes.WBP_CCStep_Attributes_C:WidgetTree.Text_StrengthModifier.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/RPG/WBP_CCStep_Attributes.WBP_CCStep_Attributes_C:WidgetTree.Text_StrengthRaceBonus.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/RPG/WBP_CCStep_Attributes.WBP_CCStep_Attributes_C:WidgetTree.Text_StrengthTotalValue.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/RPG/WBP_CCStep_Attributes.WBP_CCStep_Attributes_C:WidgetTree.Text_WisdomClassValue.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/RPG/WBP_CCStep_Attributes.WBP_CCStep_Attributes_C:WidgetTree.Text_WisdomModifier.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/RPG/WBP_CCStep_Attributes.WBP_CCStep_Attributes_C:WidgetTree.Text_WisdomRaceBonus.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/RPG/WBP_CCStep_Attributes.WBP_CCStep_Attributes_C:WidgetTree.Text_WisdomTotalValue.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/RPG/WBP_CharacterCreationWizard.WBP_CharacterCreationWizard_C:WidgetTree.Text_IdentityClass.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/RPG/WBP_CharacterCreationWizard.WBP_CharacterCreationWizard_C:WidgetTree.Text_IdentityGender.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/RPG/WBP_CharacterCreationWizard.WBP_CharacterCreationWizard_C:WidgetTree.Text_IdentityRace.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/Skills/RPG/WBP_RPGSkillEntry.WBP_RPGSkillEntry_C:WidgetTree.Text_SkillAttribute.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/Skills/RPG/WBP_RPGSkillEntry.WBP_RPGSkillEntry_C:WidgetTree.Text_SkillDescription.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/Skills/RPG/WBP_RPGSkillEntry.WBP_RPGSkillEntry_C:WidgetTree.Text_SkillName.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/Skills/RPG/WBP_RPGSkillEntry.WBP_RPGSkillEntry_C:WidgetTree.Text_SkillRank.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/Skills/RPG/WBP_RPGSkillEntry.WBP_RPGSkillEntry_C:WidgetTree.Text_SkillTrainingPolicy.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/Skills/RPG/WBP_RPGTalentBranch.WBP_RPGTalentBranch_C:WidgetTree.Text_BranchName.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/Skills/RPG/WBP_RPGTalentBranch.WBP_RPGTalentBranch_C:WidgetTree.Text_BranchProgress.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/Skills/RPG/WBP_RPGTalentDetail.WBP_RPGTalentDetail_C:WidgetTree.Text_DetailName.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/Skills/RPG/WBP_RPGTalentNode.WBP_RPGTalentNode_C:WidgetTree.Text_TalentLevel.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/Skills/RPG/WBP_RPGTalentNode.WBP_RPGTalentNode_C:WidgetTree.Text_TalentState.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/Skills/RPG/WBP_RPGTalentNode.WBP_RPGTalentNode_C:WidgetTree.Text_TalentTier.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/Skills/WBP_GridSkills.WBP_GridSkills_C:WidgetTree.Text_CharacterName.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/Skills/WBP_GridSkills.WBP_GridSkills_C:WidgetTree.Text_ClassLevel.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/Skills/WBP_GridSkills.WBP_GridSkills_C:WidgetTree.Text_SkillPoints.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/Skills/WBP_GridSkills.WBP_GridSkills_C:WidgetTree.Text_TalentPoints.Text")
	};

	const TCHAR* const ScriptBytecodePaths[] = {
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/Inventory/WBP_ItemTooltip.WBP_ItemTooltip_C:InitFromInventorySlot [Script Bytecode]"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/Inventory/WBP_ItemTooltipComparisonRow.WBP_ItemTooltipComparisonRow_C:ExecuteUbergraph_WBP_ItemTooltipComparisonRow [Script Bytecode]"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/Inventory/WBP_ItemTooltipStatLine.WBP_ItemTooltipStatLine_C:ExecuteUbergraph_WBP_ItemTooltipStatLine [Script Bytecode]"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/MainMenu/WBP_LoadGameMenu.WBP_LoadGameMenu_C:ExecuteUbergraph_WBP_LoadGameMenu [Script Bytecode]")
	};

	const TCHAR* const ExistingAuthorityPaths[] = {
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/Inventory/WBP_CharacterSheet.WBP_CharacterSheet_C:WidgetTree.Label_Arcane.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/Inventory/WBP_CharacterSheet.WBP_CharacterSheet_C:WidgetTree.Label_Fire.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/Inventory/WBP_CharacterSheet.WBP_CharacterSheet_C:WidgetTree.Label_Ice.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/RPG/WBP_CCStep_Attributes.WBP_CCStep_Attributes_C:WidgetTree.Text_AttributeHelp.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/RPG/WBP_CCStep_Attributes.WBP_CCStep_Attributes_C:WidgetTree.Text_HeaderClass.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/RPG/WBP_CCStep_Attributes.WBP_CCStep_Attributes_C:WidgetTree.Text_LabelCharisma.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/RPG/WBP_CCStep_Attributes.WBP_CCStep_Attributes_C:WidgetTree.Text_LabelConstitution.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/RPG/WBP_CCStep_Attributes.WBP_CCStep_Attributes_C:WidgetTree.Text_LabelDexterity.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/RPG/WBP_CCStep_Attributes.WBP_CCStep_Attributes_C:WidgetTree.Text_LabelIntelligence.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/RPG/WBP_CCStep_Attributes.WBP_CCStep_Attributes_C:WidgetTree.Text_LabelStrength.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/RPG/WBP_CCStep_Attributes.WBP_CCStep_Attributes_C:WidgetTree.Text_LabelWisdom.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/RPG/WBP_CharacterCreationWizard.WBP_CharacterCreationWizard_C:WidgetTree.Text_IdentitySummaryTitle.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/RPG/WBP_CharacterCreationWizard.WBP_CharacterCreationWizard_C:WidgetTree.Text_PortraitCaption.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/RPG/WBP_CharacterCreationWizard.WBP_CharacterCreationWizard_C:WidgetTree.Text_SummaryTitle.Text")
	};

	const TCHAR* const TalentPaths[] = {
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/Skills/RPG/WBP_RPGTalentDetail.WBP_RPGTalentDetail_C:WidgetTree.Text_AcquireTalent.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/Skills/RPG/WBP_RPGTalentDetail.WBP_RPGTalentDetail_C:WidgetTree.Text_ConfirmAcquire.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/Skills/RPG/WBP_RPGTalentDetail.WBP_RPGTalentDetail_C:WidgetTree.Text_LabelAcquisition.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/Skills/RPG/WBP_RPGTalentDetail.WBP_RPGTalentDetail_C:WidgetTree.Text_LabelEffects.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/Skills/RPG/WBP_RPGTalentDetail.WBP_RPGTalentDetail_C:WidgetTree.Text_LabelPrinciple.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/Skills/RPG/WBP_RPGTalentDetail.WBP_RPGTalentDetail_C:WidgetTree.Text_LabelStatus.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/Skills/RPG/WBP_RPGTalentDetail.WBP_RPGTalentDetail_C:WidgetTree.Text_LabelType.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/Skills/RPG/WBP_RPGTalentDetail.WBP_RPGTalentDetail_C:WidgetTree.Text_LabelUsage.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/Skills/RPG/WBP_RPGTalentDetail.WBP_RPGTalentDetail_C:WidgetTree.Text_LabelVariants.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/Skills/RPG/WBP_RPGTalentVariantBlock.WBP_RPGTalentVariantBlock_C:WidgetTree.Text_ChooseVariant.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/Skills/RPG/WBP_RPGTalentVariantBlock.WBP_RPGTalentVariantBlock_C:WidgetTree.Text_LabelVariantEffects.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/Skills/RPG/WBP_RPGTalentVariantBlock.WBP_RPGTalentVariantBlock_C:WidgetTree.Text_LabelVariantUsage.Text")
	};

	const TCHAR* const SharedVocabularyPaths[] = {
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/Inventory/WBP_CharacterSheet.WBP_CharacterSheet_C:WidgetTree.TextLabel_Equipment.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/Inventory/WBP_CharacterSheet.WBP_CharacterSheet_C:WidgetTree.TextLabel_Mana.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/Inventory/WBP_InventoryBag.WBP_InventoryBag_C:WidgetTree.Button_FilterEquipment.ToolTipText"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/MainMenu/WBP_CreditsMenu.WBP_CreditsMenu_C:WidgetTree.Text_Back.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/MainMenu/WBP_LicenseMenu.WBP_LicenseMenu_C:WidgetTree.Text_Back.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/MainMenu/WBP_LicenseMenu.WBP_LicenseMenu_C:WidgetTree.Text_Title.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/MainMenu/WBP_LoadGameMenu.WBP_LoadGameMenu_C:WidgetTree.Text_Back.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/MainMenu/WBP_LoadGameMenu.WBP_LoadGameMenu_C:WidgetTree.Text_Title.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/MainMenu/WBP_MainMenu.WBP_MainMenu_C:WidgetTree.TextBlock_1181.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/MainMenu/WBP_MainMenu.WBP_MainMenu_C:WidgetTree.TextBlock_729.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/MainMenu/WBP_MainMenu.WBP_MainMenu_C:WidgetTree.TextBlock_875.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/MainMenu/WBP_OptionsMenu.WBP_OptionsMenu_C:WidgetTree.Text_Back.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/MainMenu/WBP_OptionsMenu.WBP_OptionsMenu_C:WidgetTree.Text_Title.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/RPG/WBP_CCStep_Attributes.WBP_CCStep_Attributes_C:WidgetTree.TextLabel_Mana.Text")
	};

	const TCHAR* const RaceIllustrationPaths[] = {
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/RPG/WBP_CharacterCreationWizard.Default__WBP_CharacterCreationWizard_C.AvailableRaceIllustrations(0).AvailableRaceIllustrations.DisplayName"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/RPG/WBP_CharacterCreationWizard.Default__WBP_CharacterCreationWizard_C.AvailableRaceIllustrations(1).AvailableRaceIllustrations.DisplayName"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/RPG/WBP_CharacterCreationWizard.Default__WBP_CharacterCreationWizard_C.AvailableRaceIllustrations(10).AvailableRaceIllustrations.DisplayName"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/RPG/WBP_CharacterCreationWizard.Default__WBP_CharacterCreationWizard_C.AvailableRaceIllustrations(11).AvailableRaceIllustrations.DisplayName"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/RPG/WBP_CharacterCreationWizard.Default__WBP_CharacterCreationWizard_C.AvailableRaceIllustrations(2).AvailableRaceIllustrations.DisplayName"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/RPG/WBP_CharacterCreationWizard.Default__WBP_CharacterCreationWizard_C.AvailableRaceIllustrations(3).AvailableRaceIllustrations.DisplayName"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/RPG/WBP_CharacterCreationWizard.Default__WBP_CharacterCreationWizard_C.AvailableRaceIllustrations(4).AvailableRaceIllustrations.DisplayName"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/RPG/WBP_CharacterCreationWizard.Default__WBP_CharacterCreationWizard_C.AvailableRaceIllustrations(5).AvailableRaceIllustrations.DisplayName"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/RPG/WBP_CharacterCreationWizard.Default__WBP_CharacterCreationWizard_C.AvailableRaceIllustrations(6).AvailableRaceIllustrations.DisplayName"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/RPG/WBP_CharacterCreationWizard.Default__WBP_CharacterCreationWizard_C.AvailableRaceIllustrations(7).AvailableRaceIllustrations.DisplayName"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/RPG/WBP_CharacterCreationWizard.Default__WBP_CharacterCreationWizard_C.AvailableRaceIllustrations(8).AvailableRaceIllustrations.DisplayName"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/RPG/WBP_CharacterCreationWizard.Default__WBP_CharacterCreationWizard_C.AvailableRaceIllustrations(9).AvailableRaceIllustrations.DisplayName")
	};

	const TCHAR* const MechanicalGlyphPaths[] = {
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/InGameMenu/WBP_GridMap.WBP_GridMap_C:WidgetTree.Text_LevelDownGlyph.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/InGameMenu/WBP_GridMap.WBP_GridMap_C:WidgetTree.Text_LevelUpGlyph.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/Inventory/WBP_GridPersistentHud.WBP_GridPersistentHud_C:WidgetTree.Text_NavEscape.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/Inventory/WBP_GridPersistentHud.WBP_GridPersistentHud_C:WidgetTree.TextBlock_255.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/Inventory/WBP_GridPersistentHud.WBP_GridPersistentHud_C:WidgetTree.TextBlock_394.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/Inventory/WBP_GridPersistentHud.WBP_GridPersistentHud_C:WidgetTree.TextBlock_550.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/Inventory/WBP_GridPersistentHud.WBP_GridPersistentHud_C:WidgetTree.TextBlock_688.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/Inventory/WBP_GridPersistentHud.WBP_GridPersistentHud_C:WidgetTree.TextBlock_884.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/Inventory/WBP_GridPersistentHud.WBP_GridPersistentHud_C:WidgetTree.TextBlock_994.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/Skills/RPG/WBP_RPGSkillEntry.WBP_RPGSkillEntry_C:WidgetTree.Text_DecreaseSkill.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/Skills/RPG/WBP_RPGSkillEntry.WBP_RPGSkillEntry_C:WidgetTree.Text_IncreaseSkill.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/Skills/WBP_GridSkills.WBP_GridSkills_C:WidgetTree.Text_HeaderIncrease.Text")
	};

	const TCHAR* const IntentionalLocalCopyPaths[] = {
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/MainMenu/WBP_CreditsMenu.WBP_CreditsMenu_C:WidgetTree.Text_Content.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/MainMenu/WBP_OptionsMenu.WBP_OptionsMenu_C:WidgetTree.Text_OptionPlaceholder.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/InGameMenu/WBP_GridSpellbook.WBP_GridSpellbook_C:WidgetTree.Text_Titre.Text"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/InGameMenu/WBP_GridJournal.WBP_GridJournal_C:WidgetTree.Text_Placeholder.Text")
	};

	static_assert(UE_ARRAY_COUNT(RuntimePlaceholderPaths) == 62);
	// The six historical Script Bytecode occurrences shared four manifest paths.
	static_assert(UE_ARRAY_COUNT(ScriptBytecodePaths) == 4);
	static_assert(UE_ARRAY_COUNT(ExistingAuthorityPaths) == 14);
	static_assert(UE_ARRAY_COUNT(TalentPaths) == 12);
	static_assert(UE_ARRAY_COUNT(SharedVocabularyPaths) == 14);
	static_assert(UE_ARRAY_COUNT(RaceIllustrationPaths) == 12);
	static_assert(UE_ARRAY_COUNT(MechanicalGlyphPaths) == 12);
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUILOCALIZATION033WbpOwnershipRegressionTest,
	"Game.UI.Localization.LOCALIZATION03.WbpOwnershipRegression",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUILOCALIZATION033WbpOwnershipRegressionTest::RunTest(const FString&)
{
	using namespace UILOCALIZATION033;

	FString Manifest;
	if (!LoadMaterializedManifest(*this, Manifest))
	{
		return false;
	}

	RequirePathsAbsent(*this, Manifest, TEXT("Runtime placeholder"), RuntimePlaceholderPaths);
	RequirePathsAbsent(*this, Manifest, TEXT("Blueprint Script Bytecode"), ScriptBytecodePaths);
	RequirePathsAbsent(*this, Manifest, TEXT("Existing text authority"), ExistingAuthorityPaths);
	RequirePathsAbsent(*this, Manifest, TEXT("Talent text authority"), TalentPaths);
	RequirePathsAbsent(*this, Manifest, TEXT("Shared WBP vocabulary"), SharedVocabularyPaths);
	RequirePathsAbsent(*this, Manifest, TEXT("Race illustration authored text"), RaceIllustrationPaths);

	// Localization ownership does not mean "zero FText in WBP":
	// mechanical glyphs and screen-local copy are intentional.
	RequirePathsPresent(*this, Manifest, TEXT("Mechanical glyph"), MechanicalGlyphPaths);
	RequirePathsPresent(*this, Manifest, TEXT("Intentional local WBP copy"), IntentionalLocalCopyPaths);

	const TCHAR* const RequiredSharedKeys[] = {
		TEXT("Navigation.Back"),
		TEXT("Menu.LoadGame"),
		TEXT("Menu.Options"),
		TEXT("Menu.License"),
		TEXT("Inventory.Category.Equipment"),
		TEXT("Resource.Mana")
	};
	for (const TCHAR* Key : RequiredSharedKeys)
	{
		TestTrue(
			*FString::Printf(TEXT("Materialized manifest keeps shared ST_UI key '%s'"), Key),
			Manifest.Contains(Key));
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
