#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateColor.h"
#include "Templates/Function.h"
#include "Widgets/Input/SButton.h"

#if WITH_EDITOR

class SWidget;
class UGridObjectPaletteAsset;
enum class EGridLevelObjectType : uint8;

namespace GridEditorWidgetHelpers
{
    struct FGridArchetypeOption
    {
        FName ArchetypeId = NAME_None;
        FText Label;
    };

    FText GetGridEnumDisplayText (const UEnum* Enum, int64 Value);
    FText GetGridObjectGlyph (EGridLevelObjectType Type);

    TSharedRef<SWidget> BuildGridPropertyRow (const FText& Label, TSharedRef<SWidget> ValueWidget);
    TSharedRef<SWidget> BuildGridReadOnlyPropertyRow (const FText& Label, const FText& Value);
    TSharedRef<SWidget> BuildGridActionButton (const FText& Label, const FOnClicked& OnClicked);
    TSharedRef<SWidget> BuildGridPanelSection (const FText& Title, TSharedRef<SWidget> Content);
    TSharedRef<SWidget> BuildGridCollapsiblePanelSection (
        const FText& Title,
        const TFunctionRef<TSharedRef<SWidget> ()>& BuildContent,
        bool bExpanded,
        const FOnClicked& OnToggleClicked);
    TSharedRef<SWidget> BuildGridStatusBadge (const FText& Label, const FText& Value, const FSlateColor& AccentColor);
    TSharedRef<SWidget> BuildGridCompactStatusBadge (const FText& Label, const FText& Value, const FSlateColor& AccentColor);

    FString NameArrayToCommaSeparatedText (const TArray<FName>& Names);
    TArray<FName> ParseCommaSeparatedNames (const FString& Text);
    TArray<FGridArchetypeOption> GetItemArchetypeOptions (const UGridObjectPaletteAsset* ObjectPalette);
}

#endif
