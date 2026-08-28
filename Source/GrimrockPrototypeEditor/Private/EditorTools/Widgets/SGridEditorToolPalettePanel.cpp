#include "EditorTools/Widgets/SGridEditorToolPalettePanel.h"

#if WITH_EDITOR

#include "EditorTools/GridLevelEditorActor.h"
#include "EditorTools/Widgets/GridEditorWidgetHelpers.h"
#include "Core/GridObjectArchetypeAsset.h"
#include "Core/GridObjectPaletteAsset.h"
#include "Core/GridTypes.h"

#include "Brushes/SlateImageBrush.h"
#include "Engine/Texture2D.h"
#include "Misc/ConfigCacheIni.h"

#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"

#include "Widgets/Images/SImage.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SUniformGridPanel.h"

namespace
{
	const TCHAR* PaletteUserConfigSection = TEXT("Grimrock.GridEditor.Palette");
	const TCHAR* PaletteFavoritesKey = TEXT("Favorites");
	const TCHAR* PaletteRecentKey = TEXT("Recent");
	constexpr int32 MaxRecentPaletteEntries = 16;

	FText GetToolGlyph(EGridEditorTool Tool)
	{
		switch (Tool)
		{
			case EGridEditorTool::Select:
				return FText::FromString(TEXT("S"));
			case EGridEditorTool::PaintCell:
				return FText::FromString(TEXT("C"));
			case EGridEditorTool::PaintWall:
				return FText::FromString(TEXT("W"));
			case EGridEditorTool::PaintObject:
				return FText::FromString(TEXT("O"));
			case EGridEditorTool::Erase:
				return FText::FromString(TEXT("X"));
			case EGridEditorTool::Link:
				return FText::FromString(TEXT("L"));
			default:
				return FText::FromString(TEXT("?"));
		}
	}

	FName GetPaletteCategoryForEntry(const FGridObjectPaletteEntry& Entry)
	{
		const FName EffectiveCategory = Entry.GetEffectiveCategory();
		return EffectiveCategory.IsNone() ? FName(TEXT("Uncategorized")) : EffectiveCategory;
	}

	FText GetPaletteCategoryDisplayText(FName Category)
	{
		return Category.IsNone() ? FText::FromString(TEXT("Uncategorized")) : FText::FromName(Category);
	}

	int32 GetPaletteCategorySortOrder(FName Category)
	{
		if (Category == FName(TEXT("Doors")))
			return 0;
		if (Category == FName(TEXT("Mechanisms")))
			return 1;
		if (Category == FName(TEXT("Receptacles")))
			return 2;
		if (Category == FName(TEXT("Transitions")))
			return 3;
		if (Category == FName(TEXT("Items")))
			return 4;
		if (Category == FName(TEXT("Logic")))
			return 5;
		if (Category == FName(TEXT("Readable")))
			return 6;
		if (Category == FName(TEXT("Wall Decorations")))
			return 7;
		if (Category == FName(TEXT("Floor Decorations")))
			return 8;
		if (Category == FName(TEXT("Lights")))
			return 9;
		if (Category == FName(TEXT("Spawns")))
			return 10;
		if (Category == FName(TEXT("Uncategorized")))
			return 11;
		return 100;
	}
}

void SGridEditorToolPalettePanel::Construct(const FArguments& InArgs)
{
	EditorActor = InArgs._EditorActor;
	ToolPaletteState = InArgs._ToolPaletteState;
	OnGetEditorActor = InArgs._OnGetEditorActor;
	OnRequestRefresh = InArgs._OnRequestRefresh;

	LoadUserPaletteState();
	ChildSlot[BuildToolPalettePanel()];
}

AGridLevelEditorActor* SGridEditorToolPalettePanel::GetEditorActor() const
{
	if (EditorActor.IsValid())
	{
		return EditorActor.Get();
	}

	return OnGetEditorActor.IsBound() ? OnGetEditorActor.Execute() : nullptr;
}

FGridEditorToolPalettePanelState& SGridEditorToolPalettePanel::GetToolPaletteState() const
{
	static FGridEditorToolPalettePanelState FallbackToolPaletteState;

	return ToolPaletteState.IsValid() ? *ToolPaletteState : FallbackToolPaletteState;
}

void SGridEditorToolPalettePanel::RequestRefresh() const
{
	if (OnRequestRefresh.IsBound())
	{
		OnRequestRefresh.Execute();
	}
}

void SGridEditorToolPalettePanel::LoadUserPaletteState()
{
	FGridEditorToolPalettePanelState& State = GetToolPaletteState();
	if (State.bUserStateLoaded)
	{
		return;
	}

	State.bUserStateLoaded = true;

	if (!GConfig)
	{
		return;
	}

	TArray<FString> FavoriteStrings;
	GConfig->GetArray(PaletteUserConfigSection, PaletteFavoritesKey, FavoriteStrings, GEditorPerProjectIni);
	for (const FString& FavoriteString : FavoriteStrings)
	{
		if (!FavoriteString.IsEmpty())
		{
			State.FavoriteEntryIds.Add(FName(*FavoriteString));
		}
	}

	TArray<FString> RecentStrings;
	GConfig->GetArray(PaletteUserConfigSection, PaletteRecentKey, RecentStrings, GEditorPerProjectIni);
	for (const FString& RecentString : RecentStrings)
	{
		if (!RecentString.IsEmpty())
		{
			State.RecentEntryIds.AddUnique(FName(*RecentString));
		}
	}

	if (State.RecentEntryIds.Num() > MaxRecentPaletteEntries)
	{
		State.RecentEntryIds.SetNum(MaxRecentPaletteEntries);
	}
}

void SGridEditorToolPalettePanel::SaveUserPaletteState() const
{
	if (!GConfig)
	{
		return;
	}

	const FGridEditorToolPalettePanelState& State = GetToolPaletteState();

	TArray<FString> FavoriteStrings;
	FavoriteStrings.Reserve(State.FavoriteEntryIds.Num());
	for (const FName& EntryId : State.FavoriteEntryIds)
	{
		FavoriteStrings.Add(EntryId.ToString());
	}
	FavoriteStrings.Sort();

	TArray<FString> RecentStrings;
	RecentStrings.Reserve(State.RecentEntryIds.Num());
	for (const FName& EntryId : State.RecentEntryIds)
	{
		RecentStrings.Add(EntryId.ToString());
	}

	GConfig->SetArray(PaletteUserConfigSection, PaletteFavoritesKey, FavoriteStrings, GEditorPerProjectIni);
	GConfig->SetArray(PaletteUserConfigSection, PaletteRecentKey, RecentStrings, GEditorPerProjectIni);
	GConfig->Flush(false, GEditorPerProjectIni);
}

void SGridEditorToolPalettePanel::AddRecentEntry(FName EntryId)
{
	if (EntryId.IsNone())
	{
		return;
	}

	FGridEditorToolPalettePanelState& State = GetToolPaletteState();
	State.RecentEntryIds.Remove(EntryId);
	State.RecentEntryIds.Insert(EntryId, 0);

	if (State.RecentEntryIds.Num() > MaxRecentPaletteEntries)
	{
		State.RecentEntryIds.SetNum(MaxRecentPaletteEntries);
	}

	SaveUserPaletteState();
}

bool SGridEditorToolPalettePanel::IsFavoriteEntry(FName EntryId) const
{
	return GetToolPaletteState().FavoriteEntryIds.Contains(EntryId);
}

TSharedRef<SWidget> SGridEditorToolPalettePanel::BuildToolPalettePanel()
{
	const AGridLevelEditorActor* CurrentEditorActor = GetEditorActor();
	const bool bShowPalette = CurrentEditorActor && CurrentEditorActor->ActiveTool == EGridEditorTool::PaintObject;

	TSharedRef<SVerticalBox> Root = SNew(SVerticalBox);

	Root->AddSlot().AutoHeight().Padding(
		0.f, 0.f, 0.f, bShowPalette ? 6.f : 0.f)[GridEditorWidgetHelpers::BuildGridPanelSection(FText::FromString(TEXT("TOOLS")), BuildToolSection())];

	if (bShowPalette)
	{
		Root->AddSlot().AutoHeight().Padding(0.f)[GridEditorWidgetHelpers::BuildGridPanelSection(FText::FromString(TEXT("PALETTE")), BuildPaletteSection())];
	}

	return Root;
}

TSharedRef<SWidget> SGridEditorToolPalettePanel::BuildToolSection()
{
	TSharedRef<SHorizontalBox> ToolRow = SNew(SHorizontalBox);

	const auto AddTool = [this, &ToolRow](const FText& Label, EGridEditorTool Tool)
	{
		ToolRow->AddSlot()
			.AutoWidth()
			.Padding(FMargin(2.f, 0.f, 2.f, 0.f))
			[
				BuildToolTile(Label, GetToolGlyph(Tool), Tool)
			];
	};

	AddTool(FText::FromString(TEXT("Select")), EGridEditorTool::Select);
	AddTool(FText::FromString(TEXT("Paint Cell")), EGridEditorTool::PaintCell);
	AddTool(FText::FromString(TEXT("Paint Wall")), EGridEditorTool::PaintWall);
	AddTool(FText::FromString(TEXT("Paint Object")), EGridEditorTool::PaintObject);
	AddTool(FText::FromString(TEXT("Erase")), EGridEditorTool::Erase);

	return ToolRow;
}

UTexture2D* SGridEditorToolPalettePanel::GetToolIcon(EGridEditorTool Tool) const
{
	switch (Tool)
	{
		case EGridEditorTool::Select:
			return LoadObject<UTexture2D>(nullptr, TEXT("/Game/GrimrockPrototype/Icons/T_Tool_Select.T_Tool_Select"));

		case EGridEditorTool::PaintCell:
			return LoadObject<UTexture2D>(nullptr, TEXT("/Game/GrimrockPrototype/Icons/T_Tool_PaintCell.T_Tool_PaintCell"));

		case EGridEditorTool::PaintWall:
			return LoadObject<UTexture2D>(nullptr, TEXT("/Game/GrimrockPrototype/Icons/T_Tool_PaintWall.T_Tool_PaintWall"));

		case EGridEditorTool::PaintObject:
			return LoadObject<UTexture2D>(nullptr, TEXT("/Game/GrimrockPrototype/Icons/T_Tool_PaintObject.T_Tool_PaintObject"));

		case EGridEditorTool::Erase:
			return LoadObject<UTexture2D>(nullptr, TEXT("/Game/GrimrockPrototype/Icons/T_Tool_Erase.T_Tool_Erase"));

		case EGridEditorTool::Link:
			return LoadObject<UTexture2D>(nullptr, TEXT("/Game/GrimrockPrototype/Icons/T_Tool_Link.T_Tool_Link"));

		default:
			return nullptr;
	}
}

TSharedRef<SWidget> SGridEditorToolPalettePanel::BuildToolTile(const FText& Label, const FText& Glyph, EGridEditorTool ToolValue)
{
	const AGridLevelEditorActor* CurrentEditorActor = GetEditorActor();
	const bool bSelected = CurrentEditorActor && CurrentEditorActor->ActiveTool == ToolValue;

	UTexture2D* IconTexture = GetToolIcon(ToolValue);

	const float TileWidth = 80.f;
	const float TileHeight = 80.f;
	const float IconSize = 64.f;

	const FLinearColor NormalColor = FLinearColor(0.25f, 0.22f, 0.18f, 1.f);
	const FLinearColor HoverColor = FLinearColor(0.52f, 0.46f, 0.38f, 1.f);
	const FLinearColor SelectedColor = FLinearColor(0.f, 0.85f, 1.f, 0.85f);

	TSharedPtr<SBorder> TileBorder;

	TSharedRef<SWidget> ButtonContent =
		SAssignNew(TileBorder, SBorder)
			.Padding(2.f)
			.BorderImage(bSelected ? FAppStyle::GetBrush("FocusRectangle") : FAppStyle::GetBrush("ToolPanel.GroupBorder"))
			.BorderBackgroundColor(bSelected ? SelectedColor : NormalColor)[SNew(SBox).WidthOverride(TileWidth).HeightOverride(TileHeight)[SNew(SVerticalBox) +
				SVerticalBox::Slot()
					.FillHeight(1.f)
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)[IconTexture
							? StaticCastSharedRef<SWidget>(SNew(SImage).Image(GetOrCreateBrush(IconTexture, IconSize)))
							: StaticCastSharedRef<SWidget>(SNew(STextBlock).Text(Glyph).Font(FCoreStyle::GetDefaultFontStyle("Regular", 34)))] +
				SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Center)
					.Padding(0.f, 2.f, 0.f, 2.f)[SNew(STextBlock).Text(Label).Justification(ETextJustify::Center)]]];
	return SNew(SButton)
		.ButtonStyle(FAppStyle::Get(), "NoBorder")
		.ContentPadding(0.f)
		.Cursor(EMouseCursor::Hand)
		.OnHovered_Lambda(
			[TileBorder, bSelected, HoverColor]()
			{
				if (TileBorder.IsValid() && !bSelected)
				{
					TileBorder->SetBorderBackgroundColor(HoverColor);
				}
			})
		.OnUnhovered_Lambda(
			[TileBorder, bSelected, NormalColor]()
			{
				if (TileBorder.IsValid() && !bSelected)
				{
					TileBorder->SetBorderBackgroundColor(NormalColor);
				}
			})
		.OnClicked_Lambda(
			[this, ToolValue]() -> FReply
			{
				return OnToolClicked(static_cast<int32>(ToolValue));
			})[ButtonContent];
}

TSharedRef<SWidget> SGridEditorToolPalettePanel::BuildIconOrFallback(UTexture2D* Icon, EGridLevelObjectType FallbackType, float Size)
{
	if (Icon)
	{
		return SNew(SBox).WidthOverride(Size).HeightOverride(Size)[SNew(SImage).Image(GetOrCreateBrush(Icon, Size))];
	}

	return SNew(SBox)
		.WidthOverride(Size)
		.HeightOverride(Size)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
			[SNew(STextBlock).Text(GridEditorWidgetHelpers::GetGridObjectGlyph(FallbackType)).Font(FCoreStyle::GetDefaultFontStyle("Regular", 30))];
}

TSharedRef<SWidget> SGridEditorToolPalettePanel::BuildPaletteSection()
{
	AGridLevelEditorActor* CurrentEditorActor = GetEditorActor();
	TSharedRef<SVerticalBox> Root = SNew(SVerticalBox);

	if (!CurrentEditorActor)
	{
		Root->AddSlot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(TEXT("No GridLevelEditorActor found.")))];
		return Root;
	}
	if (!CurrentEditorActor->ObjectPalette)
	{
		Root->AddSlot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(TEXT("No ObjectPalette assigned.")))];
		return Root;
	}

	const FGridObjectPaletteEntry* StairsUpEntry = CurrentEditorActor->ObjectPalette->FindEntryById(FName(TEXT("Stairs_Up")));
	const FGridObjectPaletteEntry* StairsDownEntry = CurrentEditorActor->ObjectPalette->FindEntryById(FName(TEXT("Stairs_Down")));
	if (!StairsUpEntry || !StairsUpEntry->DefaultArchetype || !StairsDownEntry || !StairsDownEntry->DefaultArchetype ||
		!StairsUpEntry->DefaultArchetype->DefaultBehavior.Transition.bIsTransition ||
		!StairsDownEntry->DefaultArchetype->DefaultBehavior.Transition.bIsTransition)
	{
		FString Error;
		if (!CurrentEditorActor->EnsureStairsTransitionArchetypes(Error))
		{
			UE_LOG(LogTemp, Warning, TEXT("Stairs transition palette provisioning failed: %s"), *Error);
		}
	}

	TArray<FName> Categories;
	for (const FGridObjectPaletteEntry& Entry : CurrentEditorActor->ObjectPalette->Entries)
	{
		if (Entry.GetEffectiveObjectType() == EGridLevelObjectType::ItemSpawn)
		{
			continue;
		}

		Categories.AddUnique(GetPaletteCategoryForEntry(Entry));
	}

	Categories.Sort(
		[](const FName& CategoryA, const FName& CategoryB)
		{
			const int32 SortOrderA = GetPaletteCategorySortOrder(CategoryA);
			const int32 SortOrderB = GetPaletteCategorySortOrder(CategoryB);
			return SortOrderA == SortOrderB ? CategoryA.ToString() < CategoryB.ToString() : SortOrderA < SortOrderB;
		});

	FGridEditorToolPalettePanelState& State = GetToolPaletteState();
	if (State.SelectedView == EGridEditorPaletteView::Category &&
		(State.SelectedCategory.IsNone() || !Categories.Contains(State.SelectedCategory)))
	{
		State.SelectedView = EGridEditorPaletteView::All;
		State.SelectedCategory = NAME_None;
	}

	Root->AddSlot()
		.AutoHeight()
		.Padding(0.f, 0.f, 0.f, 6.f)
		[
			SNew(SSearchBox)
				.HintText(FText::FromString(TEXT("Search palette by name, id, category or archetype...")))
				.InitialText(FText::FromString(State.SearchText))
				.OnTextChanged(this, &SGridEditorToolPalettePanel::OnPaletteSearchTextChanged)
		];

	TSharedRef<SHorizontalBox> PaletteTabs = SNew(SHorizontalBox);

	const auto AddPaletteTab =
		[this, &PaletteTabs](EGridEditorPaletteView View, FName Category, const FText& Label, bool bSelected)
		{
			const FSlateColor LabelColor = bSelected
				? FSlateColor(FLinearColor(0.90f, 0.96f, 1.f, 1.f))
				: FSlateColor(FLinearColor(0.72f, 0.72f, 0.72f, 1.f));

			PaletteTabs->AddSlot()
				.AutoWidth()
				.VAlign(VAlign_Bottom)
				[
					SNew(SBorder)
						.Padding(0.f)
						.BorderImage(FAppStyle::GetBrush(bSelected ? "ToolPanel.GroupBorder" : "ToolPanel.DarkGroupBorder"))
						.BorderBackgroundColor(
							bSelected
								? FSlateColor(FLinearColor(0.10f, 0.30f, 0.38f, 1.f))
								: FSlateColor(FLinearColor(0.035f, 0.035f, 0.035f, 1.f)))
						[
							SNew(SButton)
								.ButtonStyle(FAppStyle::Get(), "NoBorder")
								.ContentPadding(FMargin(12.f, 6.f, 12.f, 7.f))
								.OnClicked(FOnClicked::CreateSP(this, &SGridEditorToolPalettePanel::OnPaletteViewClicked, View, Category))
								[
									SNew(SVerticalBox)

									+ SVerticalBox::Slot()
									.AutoHeight()
									.HAlign(HAlign_Center)
									[
										SNew(STextBlock)
											.Text(Label)
											.Font(FCoreStyle::GetDefaultFontStyle(bSelected ? "Bold" : "Regular", 9))
											.ColorAndOpacity(LabelColor)
									]

									+ SVerticalBox::Slot()
									.AutoHeight()
									.Padding(0.f, 4.f, 0.f, 0.f)
									[
										SNew(SBox)
											.HeightOverride(2.f)
											.Visibility(bSelected ? EVisibility::Visible : EVisibility::Hidden)
											[
												SNew(SBorder)
													.Padding(0.f)
													.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
													.BorderBackgroundColor(FSlateColor(FLinearColor(0.20f, 0.80f, 1.f, 1.f)))
											]
									]
								]
						]
				];
		};

	AddPaletteTab(
		EGridEditorPaletteView::All,
		NAME_None,
		FText::FromString(TEXT("All")),
		State.SelectedView == EGridEditorPaletteView::All);

	AddPaletteTab(
		EGridEditorPaletteView::Favorites,
		NAME_None,
		FText::FromString(TEXT("Favorites")),
		State.SelectedView == EGridEditorPaletteView::Favorites);

	AddPaletteTab(
		EGridEditorPaletteView::Recent,
		NAME_None,
		FText::FromString(TEXT("Recent")),
		State.SelectedView == EGridEditorPaletteView::Recent);

	for (const FName& Category : Categories)
	{
		AddPaletteTab(
			EGridEditorPaletteView::Category,
			Category,
			GetPaletteCategoryDisplayText(Category),
			State.SelectedView == EGridEditorPaletteView::Category && State.SelectedCategory == Category);
	}

	Root->AddSlot()
		.AutoHeight()
		.Padding(0.f, 0.f, 0.f, 6.f)
		[
			SNew(SScrollBox)
				.Orientation(Orient_Horizontal)
				.ScrollBarVisibility(EVisibility::Visible)
				+ SScrollBox::Slot()
				[
					PaletteTabs
				]
		];

	Root->AddSlot()
		.AutoHeight()
		[
			SAssignNew(PaletteResultsRoot, SVerticalBox)
		];

	RebuildPaletteResults();

	Root->AddSlot()
		.AutoHeight()
		.Padding(0.f, 6.f, 0.f, 0.f)
		[
			SNew(STextBlock)
				.Text_Lambda(
					[this]()
					{
						return FText::Format(
							FText::FromString(TEXT("Selected Palette Entry: {0}")),
							GetSelectedPaletteEntryText());
					})
		];

	return Root;
}

void SGridEditorToolPalettePanel::RebuildPaletteResults()
{
	if (!PaletteResultsRoot.IsValid())
	{
		return;
	}

	PaletteResultsRoot->ClearChildren();
	PaletteResultsRoot->AddSlot()
		.AutoHeight()
		[
			BuildPaletteResults()
		];
}

bool SGridEditorToolPalettePanel::DoesPaletteEntryPassFilters(const FGridObjectPaletteEntry& Entry) const
{
	const FGridEditorToolPalettePanelState& State = GetToolPaletteState();
	const FName Category = GetPaletteCategoryForEntry(Entry);

	switch (State.SelectedView)
	{
		case EGridEditorPaletteView::Favorites:
			if (!State.FavoriteEntryIds.Contains(Entry.EntryId))
			{
				return false;
			}
			break;

		case EGridEditorPaletteView::Recent:
			if (!State.RecentEntryIds.Contains(Entry.EntryId))
			{
				return false;
			}
			break;

		case EGridEditorPaletteView::Category:
			if (State.SelectedCategory != Category)
			{
				return false;
			}
			break;

		case EGridEditorPaletteView::All:
		default:
			break;
	}

	FString Search = State.SearchText;
	Search.TrimStartAndEndInline();
	if (Search.IsEmpty())
	{
		return true;
	}

	const auto Matches = [&Search](const FString& Candidate)
	{
		return Candidate.Contains(Search, ESearchCase::IgnoreCase);
	};

	if (Matches(Entry.GetEffectiveDisplayName().ToString()) || Matches(Entry.EntryId.ToString()) || Matches(Category.ToString()))
	{
		return true;
	}

	if (Entry.DefaultArchetype)
	{
		return Matches(Entry.DefaultArchetype->ArchetypeId.ToString()) ||
			Matches(Entry.DefaultArchetype->DisplayName.ToString()) ||
			Matches(Entry.DefaultArchetype->Description.ToString());
	}

	return false;
}

TSharedRef<SWidget> SGridEditorToolPalettePanel::BuildPaletteEntryGrid(const TArray<const FGridObjectPaletteEntry*>& Entries)
{
	constexpr int32 PaletteColumnsPerRow = 8;

	TSharedRef<SUniformGridPanel> Grid = SNew(SUniformGridPanel)
		.SlotPadding(FMargin(2.f));

	for (int32 Index = 0; Index < Entries.Num(); ++Index)
	{
		const FGridObjectPaletteEntry* Entry = Entries[Index];
		if (!Entry)
		{
			continue;
		}

		const int32 Row = Index / PaletteColumnsPerRow;
		const int32 Column = Index % PaletteColumnsPerRow;

		Grid->AddSlot(Column, Row)
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Top)
			[
				BuildPaletteTile(*Entry)
			];
	}

	return SNew(SBox)
		.HAlign(HAlign_Left)
		[
			Grid
		];
}

TSharedRef<SWidget> SGridEditorToolPalettePanel::BuildPaletteResults()
{
	AGridLevelEditorActor* CurrentEditorActor = GetEditorActor();
	TSharedRef<SVerticalBox> Root = SNew(SVerticalBox);

	if (!CurrentEditorActor || !CurrentEditorActor->ObjectPalette)
	{
		return Root;
	}

	const FGridEditorToolPalettePanelState& State = GetToolPaletteState();
	TArray<const FGridObjectPaletteEntry*> MatchedEntries;
	int32 EligibleCount = 0;

	for (const FGridObjectPaletteEntry& Entry : CurrentEditorActor->ObjectPalette->Entries)
	{
		if (Entry.GetEffectiveObjectType() != EGridLevelObjectType::ItemSpawn)
		{
			++EligibleCount;
		}
	}

	if (State.SelectedView == EGridEditorPaletteView::Recent)
	{
		for (const FName& EntryId : State.RecentEntryIds)
		{
			const FGridObjectPaletteEntry* Entry = CurrentEditorActor->ObjectPalette->FindEntryById(EntryId);
			if (Entry && Entry->GetEffectiveObjectType() != EGridLevelObjectType::ItemSpawn && DoesPaletteEntryPassFilters(*Entry))
			{
				MatchedEntries.Add(Entry);
			}
		}
	}
	else
	{
		for (const FGridObjectPaletteEntry& Entry : CurrentEditorActor->ObjectPalette->Entries)
		{
			if (Entry.GetEffectiveObjectType() == EGridLevelObjectType::ItemSpawn)
			{
				continue;
			}

			if (DoesPaletteEntryPassFilters(Entry))
			{
				MatchedEntries.Add(&Entry);
			}
		}
	}

	Root->AddSlot()
		.AutoHeight()
		.Padding(0.f, 2.f, 0.f, 3.f)
		[
			SNew(STextBlock)
				.Text(FText::Format(
					FText::FromString(TEXT("Showing {0} of {1} palette entries")),
					FText::AsNumber(MatchedEntries.Num()),
					FText::AsNumber(EligibleCount)))
				.ColorAndOpacity(FSlateColor(FLinearColor(0.68f, 0.68f, 0.68f, 1.f)))
		];

	if (MatchedEntries.Num() == 0)
	{
		FText EmptyText = FText::FromString(TEXT("No palette entries match the active filters."));
		if (State.SearchText.TrimStartAndEnd().IsEmpty())
		{
			if (State.SelectedView == EGridEditorPaletteView::Favorites)
			{
				EmptyText = FText::FromString(TEXT("No favorites yet. Use the star on a palette tile to add one."));
			}
			else if (State.SelectedView == EGridEditorPaletteView::Recent)
			{
				EmptyText = FText::FromString(TEXT("No recently used entries yet."));
			}
		}

		Root->AddSlot()
			.AutoHeight()
			.Padding(0.f, 4.f, 0.f, 0.f)
			[
				SNew(STextBlock)
					.Text(EmptyText)
					.AutoWrapText(true)
			];
		return Root;
	}

	if (State.SelectedView != EGridEditorPaletteView::All)
	{
		Root->AddSlot()
			.AutoHeight()
			.HAlign(HAlign_Left)
			[
				BuildPaletteEntryGrid(MatchedEntries)
			];
		return Root;
	}

	TMap<FName, TArray<const FGridObjectPaletteEntry*>> EntriesByCategory;
	for (const FGridObjectPaletteEntry* Entry : MatchedEntries)
	{
		if (Entry)
		{
			EntriesByCategory.FindOrAdd(GetPaletteCategoryForEntry(*Entry)).Add(Entry);
		}
	}

	TArray<FName> SortedCategories;
	EntriesByCategory.GetKeys(SortedCategories);
	SortedCategories.Sort(
		[](const FName& CategoryA, const FName& CategoryB)
		{
			const int32 SortOrderA = GetPaletteCategorySortOrder(CategoryA);
			const int32 SortOrderB = GetPaletteCategorySortOrder(CategoryB);
			return SortOrderA == SortOrderB ? CategoryA.ToString() < CategoryB.ToString() : SortOrderA < SortOrderB;
		});

	for (const FName& Category : SortedCategories)
	{
		const TArray<const FGridObjectPaletteEntry*>* CategoryEntries = EntriesByCategory.Find(Category);
		if (!CategoryEntries || CategoryEntries->Num() == 0)
		{
			continue;
		}

		Root->AddSlot()
			.AutoHeight()
			.Padding(0.f, 5.f, 0.f, 2.f)
			[
				SNew(STextBlock)
					.Text(GetPaletteCategoryDisplayText(Category))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
			];

		Root->AddSlot()
			.AutoHeight()
			.HAlign(HAlign_Left)
			[
				BuildPaletteEntryGrid(*CategoryEntries)
			];
	}

	return Root;
}

void SGridEditorToolPalettePanel::OnPaletteSearchTextChanged(const FText& NewText)
{
	GetToolPaletteState().SearchText = NewText.ToString();
	RebuildPaletteResults();
}

FReply SGridEditorToolPalettePanel::OnPaletteViewClicked(EGridEditorPaletteView View, FName Category)
{
	FGridEditorToolPalettePanelState& State = GetToolPaletteState();
	State.SelectedView = View;
	State.SelectedCategory = View == EGridEditorPaletteView::Category ? Category : NAME_None;
	RequestRefresh();
	return FReply::Handled();
}

FReply SGridEditorToolPalettePanel::OnPaletteFavoriteClicked(FName EntryId)
{
	FGridEditorToolPalettePanelState& State = GetToolPaletteState();
	if (State.FavoriteEntryIds.Contains(EntryId))
	{
		State.FavoriteEntryIds.Remove(EntryId);
	}
	else
	{
		State.FavoriteEntryIds.Add(EntryId);
	}

	SaveUserPaletteState();
	RebuildPaletteResults();
	return FReply::Handled();
}

TSharedRef<SWidget> SGridEditorToolPalettePanel::BuildPaletteTile(const FGridObjectPaletteEntry& Entry)
{
	const AGridLevelEditorActor* CurrentEditorActor = GetEditorActor();
	const bool bSelected = CurrentEditorActor && CurrentEditorActor->SelectedPaletteEntryId == Entry.EntryId;
	const bool bFavorite = IsFavoriteEntry(Entry.EntryId);

	const FText Label = Entry.GetEffectiveDisplayName();
	const FName EntryId = Entry.EntryId;
	UTexture2D* IconTexture = Entry.Icon.Get();

	constexpr float TileSize = 96.f;
	constexpr float IconSize = 60.f;

	return SNew(SBox)
		.WidthOverride(TileSize)
		.HeightOverride(TileSize)
		[
			SNew(SOverlay)

			+ SOverlay::Slot()
			[
				SNew(SButton)
					.ContentPadding(FMargin(3.f))
					.ButtonColorAndOpacity(
						bSelected
							? FLinearColor(0.10f, 0.45f, 0.55f, 1.f)
							: FLinearColor(0.035f, 0.035f, 0.035f, 1.f))
					.OnClicked_Lambda(
						[this, EntryId]() -> FReply
						{
							return OnPaletteEntryClicked(EntryId);
						})
					[
						SNew(SVerticalBox)

						+ SVerticalBox::Slot()
						.FillHeight(1.f)
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						[
							BuildIconOrFallback(IconTexture, Entry.GetEffectiveObjectType(), IconSize)
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						.HAlign(HAlign_Center)
						.Padding(2.f, 2.f, 2.f, 1.f)
						[
							SNew(STextBlock)
								.Text(Label)
								.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
								.Justification(ETextJustify::Center)
								.AutoWrapText(true)
						]
					]
			]

			+ SOverlay::Slot()
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Top)
			.Padding(FMargin(2.f))
			[
				SNew(SButton)
					.ButtonStyle(FAppStyle::Get(), "NoBorder")
					.ContentPadding(FMargin(2.f, 0.f))
					.ToolTipText(
						bFavorite
							? FText::FromString(TEXT("Remove from Favorites"))
							: FText::FromString(TEXT("Add to Favorites")))
					.OnClicked(FOnClicked::CreateSP(this, &SGridEditorToolPalettePanel::OnPaletteFavoriteClicked, EntryId))
					[
						SNew(STextBlock)
							.Text(FText::FromString(bFavorite ? TEXT("★") : TEXT("☆")))
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 12))
							.ColorAndOpacity(
								bFavorite
									? FSlateColor(FLinearColor(1.f, 0.78f, 0.20f, 1.f))
									: FSlateColor(FLinearColor(0.70f, 0.70f, 0.70f, 1.f)))
					]
			]
		];
}

FReply SGridEditorToolPalettePanel::OnToolClicked(int32 ToolValue)
{
	if (AGridLevelEditorActor* CurrentEditorActor = GetEditorActor())
	{
		CurrentEditorActor->Modify();
		CurrentEditorActor->ActiveTool = static_cast<EGridEditorTool>(ToolValue);
		RequestRefresh();
	}

	return FReply::Handled();
}

FReply SGridEditorToolPalettePanel::OnPaletteEntryClicked(FName EntryId)
{
	if (AGridLevelEditorActor* CurrentEditorActor = GetEditorActor())
	{
		CurrentEditorActor->Modify();
		CurrentEditorActor->ApplyPaletteEntry(EntryId);
		CurrentEditorActor->ActiveTool = EGridEditorTool::PaintObject;
		AddRecentEntry(EntryId);
		RequestRefresh();
	}
	return FReply::Handled();
}

FText SGridEditorToolPalettePanel::GetSelectedPaletteEntryText() const
{
	if (const AGridLevelEditorActor* CurrentEditorActor = GetEditorActor())
	{
		return CurrentEditorActor->SelectedPaletteEntryId.IsNone() ? FText::FromString(TEXT("None"))
																   : FText::FromName(CurrentEditorActor->SelectedPaletteEntryId);
	}
	return FText::FromString(TEXT("None"));
}

const FSlateBrush* SGridEditorToolPalettePanel::GetOrCreateBrush(UTexture2D* Texture, float Size)
{
	if (!Texture)
	{
		return nullptr;
	}

	FGridEditorToolPalettePanelState& CurrentToolPaletteState = GetToolPaletteState();
	const FString CacheKey = FString::Printf(TEXT("%s@%.2f"), *Texture->GetPathName(), Size);

	if (TSharedPtr<FSlateBrush>* Existing = CurrentToolPaletteState.CachedIconBrushes.Find(CacheKey))
	{
		return Existing->Get();
	}

	TSharedPtr<FSlateBrush> NewBrush = MakeShared<FSlateImageBrush>(Texture, FVector2D(Size, Size));
	CurrentToolPaletteState.CachedIconBrushes.Add(CacheKey, NewBrush);
	return NewBrush.Get();
}

#endif
