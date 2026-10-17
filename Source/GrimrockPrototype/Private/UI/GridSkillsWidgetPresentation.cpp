#include "UI/GridSkillsWidget.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Engine/Texture2D.h"
#include "RPG/RPGAuthoringIdentityResolver.h"
#include "RPG/RPGClassVisualAsset.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "RPG/RPGClassProgressionTransactionService.h"
#include "RPG/RPGSkillAsset.h"
#include "RPG/RPGSkillPointService.h"
#include "UI/GridSkillDetailWidget.h"
#include "UI/GridSkillEntryWidget.h"
#include "UI/GridSkillsPageService.h"
#include "UI/GridTalentBranchWidget.h"
#include "UI/GridTalentDetailWidget.h"
#include "UI/GridTalentNodeWidget.h"
#include "UI/GridTalentVariantFlyoutWidget.h"
#include "UI/GridRPGNotificationWidget.h"
#include "UI/RPGProgressionFeedbackService.h"
#include "UI/RPGSkillPresentationAsset.h"
#include "UI/UILocalizationText.h"

DEFINE_LOG_CATEGORY_STATIC(LogGridSkillsUI, Log, All);

namespace GridSkillsWidgetPrivate
{
	UGridTalentBranchWidget* BranchWidgetByIndex(
		int32 Index,
		UGridTalentBranchWidget* Left,
		UGridTalentBranchWidget* Center,
		UGridTalentBranchWidget* Right)
	{
		switch (Index)
		{
			case 0: return Left;
			case 1: return Center;
			case 2: return Right;
			default: return nullptr;
		}
	}

	FText FormatTalentPoints(const FText& Pattern, const FText& TalentPoints)
	{
		if (Pattern.IsEmpty()) return FText::GetEmpty();
		FFormatNamedArguments Arguments;
		Arguments.Add(TEXT("TalentPoints"), TalentPoints);
		return FText::Format(Pattern, Arguments);
	}

	FText FormatClassLevel(const FText& Pattern, const FText& ClassName, int32 Level)
	{
		if (Pattern.IsEmpty() || ClassName.IsEmpty()) return FText::GetEmpty();
		FFormatNamedArguments Arguments;
		Arguments.Add(TEXT("Class"), ClassName);
		Arguments.Add(TEXT("Level"), FText::AsNumber(Level));
		return FText::Format(Pattern, Arguments);
	}

}

void UGridSkillsWidget::ApplyDesignerPresentation()
{
	RebuildSkillEntryWidgets();
	RefreshSkillSortIndicators();
	RefreshSkillSummaryPresentation();
	ApplyClassVisualIdentity();
	RefreshTabSelection();

	if (!View.IsValid())
	{
		Text_CharacterName->SetText(
			FUILocalizationText::Get(TEXT("Talent.Screen.NoCharacterSelected")));
		Text_ClassLevel->SetText(FText::GetEmpty());

		const FText TalentPointsFormat =
			FUILocalizationText::Get(TEXT("Talent.Screen.TalentPointsFormat"));
		Text_TalentPoints->SetText(
			GridSkillsWidgetPrivate::FormatTalentPoints(
				TalentPointsFormat,
				FText::FromString(TEXT("—"))));


		Branch_Left->ClearTalentBranch();
		Branch_Center->ClearTalentBranch();
		Branch_Right->ClearTalentBranch();
		return;
	}

	Text_CharacterName->SetText(
		View.CharacterName.IsEmpty()
			? FUILocalizationText::Get(TEXT("Talent.Screen.SelectedCharacterFallback"))
			: View.CharacterName);

	const FText ClassLevelFormat =
		FUILocalizationText::Get(TEXT("Talent.Screen.ClassLevelFormat"));
	Text_ClassLevel->SetText(
		GridSkillsWidgetPrivate::FormatClassLevel(
			ClassLevelFormat,
			View.ClassDisplayName,
			View.CharacterLevel));

	const FText TalentPointsFormat =
		FUILocalizationText::Get(TEXT("Talent.Screen.TalentPointsFormat"));
	Text_TalentPoints->SetText(
		GridSkillsWidgetPrivate::FormatTalentPoints(
			TalentPointsFormat,
			FText::AsNumber(View.RemainingTalentPoints)));


	FRPGClassPresentationDefinition ClassPresentation;
	if (GetCurrentClassPresentation(ClassPresentation))
	{
		Text_ClassLevel->SetColorAndOpacity(FSlateColor(ClassPresentation.AccentColor));
	}

	const URPGClassVisualAsset* ClassVisual = ResolveCurrentClassVisual();
	for (int32 Index = 0; Index < 3; ++Index)
	{
		UGridTalentBranchWidget* BranchWidget = GridSkillsWidgetPrivate::BranchWidgetByIndex(
			Index, Branch_Left, Branch_Center, Branch_Right);
		if (!BranchWidget)
		{
			continue;
		}

		FGridTalentBranchView BranchView;
		FRPGTalentBranchPresentationDefinition BranchPresentation;
		if (GetPresentedTalentBranch(Index, BranchView, BranchPresentation))
		{
			if (BranchWidget->InitializeTalentBranch(BranchView, BranchPresentation))
			{
				UTexture2D* BackgroundTexture = nullptr;
				if (ClassVisual)
				{
					if (const FRPGTalentBranchVisualDefinition* BranchVisual =
						ClassVisual->FindTalentBranchVisual(BranchView.TalentBranchId))
					{
						BackgroundTexture = BranchVisual->Background.LoadSynchronous();
					}
				}
				BranchWidget->SetBranchBackground(BackgroundTexture);

				for (const FGridTalentNodeView& NodeView : BranchView.Nodes)
				{
					UTexture2D* IconTexture = nullptr;
					if (ClassVisual)
					{
						if (const FRPGTalentNodeVisualDefinition* NodeVisual =
							ClassVisual->FindTalentNodeVisual(NodeView.TalentNodeId))
						{
							IconTexture = NodeVisual->Icon.LoadSynchronous();
						}
						if (!NodeView.SelectedChoiceId.IsNone())
						{
							if (const FRPGTalentVariantVisualDefinition* VariantVisual =
								ClassVisual->FindTalentVariantVisual(NodeView.SelectedChoiceId))
							{
								if (UTexture2D* AcquiredVariantIcon = VariantVisual->Icon.LoadSynchronous())
								{
									IconTexture = AcquiredVariantIcon;
								}
							}
						}
					}

					if (UGridTalentNodeWidget* NodeWidget = BranchWidget->GetTalentNodeWidgetForTier(NodeView.Tier))
					{
						NodeWidget->SetTalentIcon(IconTexture);
						NodeWidget->ClearVariantIcons();
						if (ClassVisual && NodeView.bHasExclusiveVariants && NodeView.SelectedChoiceId.IsNone())
						{
							for (const FGridTalentVariantView& Variant : NodeView.Variants)
							{
								if (const FRPGTalentVariantVisualDefinition* VariantVisual =
									ClassVisual->FindTalentVariantVisual(Variant.ChoiceId))
								{
									NodeWidget->SetVariantIcon(
										Variant.ChoiceId,
										VariantVisual->Icon.LoadSynchronous());
								}
							}
						}
					}
				}
			}
		}
		else
		{
			BranchWidget->ClearTalentBranch();
		}
	}
}

const URPGClassVisualAsset* UGridSkillsWidget::ResolveCurrentClassVisual() const
{
	return View.IsValid() && !View.ClassId.IsNone()
		? FRPGAuthoringIdentityResolver::ResolveClassVisualByClassId(View.ClassId)
		: nullptr;
}

void UGridSkillsWidget::ApplyClassVisualIdentity()
{
	UTexture2D* BannerTexture = nullptr;
	UTexture2D* FlagTexture = nullptr;

	if (View.IsValid() && !View.ClassId.IsNone())
	{
		if (const URPGClassVisualAsset* ClassVisual = ResolveCurrentClassVisual())
		{
			BannerTexture = ClassVisual->Banner.LoadSynchronous();
			FlagTexture = ClassVisual->Flag.LoadSynchronous();
		}

		if (!BannerTexture || !FlagTexture)
		{
			UE_LOG(
				LogGridSkillsUI,
				Warning,
				TEXT("GridSkills ClassVisual Missing ClassId=%s Banner=%s Flag=%s"),
				*View.ClassId.ToString(),
				BannerTexture ? TEXT("OK") : TEXT("Missing"),
				FlagTexture ? TEXT("OK") : TEXT("Missing"));
		}
	}

	Border_Header->SetBrushFromTexture(BannerTexture);
	Image_ClassFlag->SetBrushFromTexture(FlagTexture, false);
}

void UGridSkillsWidget::RefreshTabSelection()
{
	const int32 ActiveIndex = Switcher_SkillsTalents->GetActiveWidgetIndex();

	Image_SkillsTabSelectionFrame->SetVisibility(
		ActiveIndex == 0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	Image_TalentsTabSelectionFrame->SetVisibility(
		ActiveIndex == 1 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

void UGridSkillsWidget::ShowSkillsTab()
{
	CloseTalentVariantFlyout();
	Switcher_SkillsTalents->SetActiveWidgetIndex(0);
	RefreshTabSelection();
}

void UGridSkillsWidget::ShowTalentsTab()
{
	Switcher_SkillsTalents->SetActiveWidgetIndex(1);
	RefreshTabSelection();
}

void UGridSkillsWidget::HandleSkillsTabClicked()
{
	ShowSkillsTab();
}

void UGridSkillsWidget::HandleTalentsTabClicked()
{
	ShowTalentsTab();
}

