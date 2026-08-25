#include "EditorTools/Widgets/SGridEditorToolPalettePanel.h"

#if WITH_EDITOR

#include "EditorTools/GridLevelEditorActor.h"
#include "EditorTools/Widgets/GridEditorWidgetHelpers.h"
#include "Core/GridObjectArchetypeAsset.h"
#include "Core/GridObjectPaletteAsset.h"
#include "Core/GridTypes.h"

#include "Brushes/SlateImageBrush.h"
#include "Engine/Texture2D.h"

#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"

#include "Widgets/Images/SImage.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"

namespace
{
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
	return SNew(SHorizontalBox) +
		SHorizontalBox::Slot().AutoWidth().Padding(
			2.f)[BuildToolTile(FText::FromString(TEXT("Select")), GetToolGlyph(EGridEditorTool::Select), EGridEditorTool::Select)] +
		SHorizontalBox::Slot().AutoWidth().Padding(
			2.f)[BuildToolTile(FText::FromString(TEXT("Paint Cell")), GetToolGlyph(EGridEditorTool::PaintCell), EGridEditorTool::PaintCell)] +
		SHorizontalBox::Slot().AutoWidth().Padding(
			2.f)[BuildToolTile(FText::FromString(TEXT("Paint Wall")), GetToolGlyph(EGridEditorTool::PaintWall), EGridEditorTool::PaintWall)] +
		SHorizontalBox::Slot().AutoWidth().Padding(
			2.f)[BuildToolTile(FText::FromString(TEXT("Paint Object")), GetToolGlyph(EGridEditorTool::PaintObject), EGridEditorTool::PaintObject)] +
		SHorizontalBox::Slot().AutoWidth().Padding(
			2.f)[BuildToolTile(FText::FromString(TEXT("Erase")), GetToolGlyph(EGridEditorTool::Erase), EGridEditorTool::Erase)];
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

	TMap<FName, TArray<const FGridObjectPaletteEntry*>> EntriesByCategory;

	for (const FGridObjectPaletteEntry& Entry : CurrentEditorActor->ObjectPalette->Entries)
	{
		if (Entry.GetEffectiveObjectType() == EGridLevelObjectType::ItemSpawn)
		{
			continue;
		}

		EntriesByCategory.FindOrAdd(GetPaletteCategoryForEntry(Entry)).Add(&Entry);
	}

	TArray<FName> SortedCategories;
	EntriesByCategory.GetKeys(SortedCategories);
	SortedCategories.Sort(
		[](const FName& CategoryA, const FName& CategoryB)
		{
			const int32 SortOrderA = GetPaletteCategorySortOrder(CategoryA);
			const int32 SortOrderB = GetPaletteCategorySortOrder(CategoryB);

			if (SortOrderA != SortOrderB)
			{
				return SortOrderA < SortOrderB;
			}

			return CategoryA.ToString() < CategoryB.ToString();
		});

	for (const FName& Category : SortedCategories)
	{
		const TArray<const FGridObjectPaletteEntry*>* CategoryEntries = EntriesByCategory.Find(Category);
		if (!CategoryEntries || CategoryEntries->Num() == 0)
		{
			continue;
		}

		Root->AddSlot().AutoHeight().Padding(
			0.f, 6.f, 0.f, 2.f)[SNew(STextBlock).Text(GetPaletteCategoryDisplayText(Category)).Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))];

		TSharedRef<SUniformGridPanel> Grid = SNew(SUniformGridPanel).SlotPadding(FMargin(4.f));

		for (int32 Index = 0; Index < CategoryEntries->Num(); ++Index)
		{
			const int32 Row = Index / 5;
			const int32 Column = Index % 5;
			const FGridObjectPaletteEntry* Entry = (*CategoryEntries)[Index];

			if (Entry)
			{
				Grid->AddSlot(Column, Row)[BuildPaletteTile(*Entry)];
			}
		}

		Root->AddSlot().AutoHeight()[Grid];
	}
	Root->AddSlot().AutoHeight().Padding(0.f, 6.f, 0.f, 0.f)[SNew(STextBlock)
			.Text_Lambda(
				[this]()
				{
					return FText::Format(FText::FromString(TEXT("Selected Palette Entry: {0}")), GetSelectedPaletteEntryText());
				})];
	return Root;
}

TSharedRef<SWidget> SGridEditorToolPalettePanel::BuildPaletteTile(const FGridObjectPaletteEntry& Entry)
{
	const AGridLevelEditorActor* CurrentEditorActor = GetEditorActor();

	const bool bSelected = CurrentEditorActor && CurrentEditorActor->SelectedPaletteEntryId == Entry.EntryId;

	const FText Label = Entry.GetEffectiveDisplayName();

	const FName EntryId = Entry.EntryId;
	UTexture2D* IconTexture = Entry.Icon.Get();

	return SNew(SButton)
		.ContentPadding(6.f)
		.ButtonColorAndOpacity(bSelected ? FLinearColor(0.10f, 0.45f, 0.55f, 1.f) : FLinearColor(0.07f, 0.07f, 0.07f, 1.f))
		.OnClicked_Lambda(
			[this, EntryId]() -> FReply
			{
				return OnPaletteEntryClicked(EntryId);
			})[SNew(SBox).WidthOverride(112.f).HeightOverride(96.f)[SNew(SVerticalBox)

			+ SVerticalBox::Slot()
				  .FillHeight(1.f)
				  .HAlign(HAlign_Center)
				  .VAlign(VAlign_Center)[BuildIconOrFallback(IconTexture, Entry.GetEffectiveObjectType(), 52.f)]

			+ SVerticalBox::Slot()
				  .AutoHeight()
				  .HAlign(HAlign_Center)
				  .Padding(0.f, 4.f, 0.f, 0.f)[SNew(STextBlock).Text(Label).Justification(ETextJustify::Center)]]];
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
