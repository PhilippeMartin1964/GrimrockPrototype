#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateColor.h"
#include "Widgets/Input/SButton.h"

#if WITH_EDITOR

class SWidget;
enum class EGridLevelObjectType : uint8;

namespace GridEditorWidgetHelpers
{
	FText GetGridEnumDisplayText(const UEnum* Enum, int64 Value);
	FText GetGridObjectGlyph(EGridLevelObjectType Type);

	TSharedRef<SWidget> BuildGridPropertyRow(const FText& Label, TSharedRef<SWidget> ValueWidget);
	TSharedRef<SWidget> BuildGridReadOnlyPropertyRow(const FText& Label, const FText& Value);
	TSharedRef<SWidget> BuildGridActionButton(const FText& Label, const FOnClicked& OnClicked);
	TSharedRef<SWidget> BuildGridPanelSection(const FText& Title, TSharedRef<SWidget> Content);
	TSharedRef<SWidget> BuildGridCompactStatusBadge(const FText& Label, const FText& Value, const FSlateColor& AccentColor);
}

#endif
