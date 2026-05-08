#include "EditorTools/Widgets/GridEditorWidgetHelpers.h"

#if WITH_EDITOR

#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"

namespace GridEditorWidgetHelpers
{
    FText GetGridEnumDisplayText (const UEnum* Enum, int64 Value)
    {
        return Enum
            ? Enum->GetDisplayNameTextByValue (Value)
            : FText::FromString (TEXT ("Unknown"));
    }

    FText GetGridObjectGlyph (EGridLevelObjectType Type)
    {
        switch (Type)
        {
            case EGridLevelObjectType::Door:          return FText::FromString (TEXT ("D"));
            case EGridLevelObjectType::Button:        return FText::FromString (TEXT ("B"));
            case EGridLevelObjectType::Lever:         return FText::FromString (TEXT ("L"));
            case EGridLevelObjectType::PressurePlate: return FText::FromString (TEXT ("P"));
            case EGridLevelObjectType::Teleporter:    return FText::FromString (TEXT ("X"));
            case EGridLevelObjectType::Trigger:       return FText::FromString (TEXT ("T"));
            case EGridLevelObjectType::MonsterSpawn:  return FText::FromString (TEXT ("M"));
            case EGridLevelObjectType::ItemSpawn:     return FText::FromString (TEXT ("I"));
            case EGridLevelObjectType::Decoration:    return FText::FromString (TEXT ("O"));
            case EGridLevelObjectType::Light:         return FText::FromString (TEXT ("*"));
            case EGridLevelObjectType::Receptacle:    return FText::FromString (TEXT ("R"));
            default:                                  return FText::FromString (TEXT ("?"));
        }
    }

    TSharedRef<SWidget> BuildGridPropertyRow (const FText& Label, TSharedRef<SWidget> ValueWidget)
    {
        return SNew (SHorizontalBox)
            + SHorizontalBox::Slot ()
            .FillWidth (0.35f)
            .VAlign (VAlign_Center)
            .Padding (0.f, 2.f, 8.f, 2.f)
            [
                SNew (STextBlock)
                    .Text (Label)
                    .ColorAndOpacity (FSlateColor (FLinearColor (0.72f, 0.72f, 0.72f, 1.f)))
            ]
            + SHorizontalBox::Slot ()
            .FillWidth (0.65f)
            .VAlign (VAlign_Center)
            .Padding (0.f, 2.f)
            [
                ValueWidget
            ];
    }

    TSharedRef<SWidget> BuildGridReadOnlyPropertyRow (const FText& Label, const FText& Value)
    {
        return BuildGridPropertyRow (
            Label,
            SNew (STextBlock)
                .Text (Value)
                .AutoWrapText (true));
    }

    TSharedRef<SWidget> BuildGridActionButton (const FText& Label, const FOnClicked& OnClicked)
    {
        return SNew (SButton)
            .Text (Label)
            .HAlign (HAlign_Center)
            .ContentPadding (FMargin (8.f, 3.f))
            .OnClicked (OnClicked);
    }

    TSharedRef<SWidget> BuildGridStatusBadge (
        const FText& Label,
        const FText& Value,
        const FSlateColor& AccentColor)
    {
        return SNew (SBorder)
            .Padding (FMargin (7.f, 4.f))
            .BorderImage (FAppStyle::GetBrush ("ToolPanel.DarkGroupBorder"))
            [
                SNew (SHorizontalBox)

                + SHorizontalBox::Slot ()
                .AutoWidth ()
                .VAlign (VAlign_Center)
                .Padding (0.f, 0.f, 5.f, 0.f)
                [
                    SNew (STextBlock)
                        .Text (Label)
                        .ColorAndOpacity (AccentColor)
                        .Font (FCoreStyle::GetDefaultFontStyle ("Bold", 8))
                ]

                + SHorizontalBox::Slot ()
                .AutoWidth ()
                .VAlign (VAlign_Center)
                [
                    SNew (STextBlock)
                        .Text (Value)
                        .Font (FCoreStyle::GetDefaultFontStyle ("Regular", 8))
                ]
            ];
    }

    TSharedRef<SWidget> BuildGridCompactStatusBadge (
        const FText& Label,
        const FText& Value,
        const FSlateColor& AccentColor)
    {
        return SNew (SBorder)
            .Padding (FMargin (5.f, 2.f))
            .BorderImage (FAppStyle::GetBrush ("ToolPanel.DarkGroupBorder"))
            [
                SNew (SHorizontalBox)

                + SHorizontalBox::Slot ()
                .AutoWidth ()
                .VAlign (VAlign_Center)
                .Padding (0.f, 0.f, 4.f, 0.f)
                [
                    SNew (STextBlock)
                        .Text (Label)
                        .ColorAndOpacity (AccentColor)
                        .Font (FCoreStyle::GetDefaultFontStyle ("Bold", 8))
                ]

                + SHorizontalBox::Slot ()
                .AutoWidth ()
                .VAlign (VAlign_Center)
                [
                    SNew (STextBlock)
                        .Text (Value)
                        .Font (FCoreStyle::GetDefaultFontStyle ("Regular", 8))
                ]
            ];
    }

    FString NameArrayToCommaSeparatedText (const TArray<FName>& Names)
    {
        TArray<FString> Parts;
        Parts.Reserve (Names.Num ());

        for (const FName& Name : Names)
        {
            if (!Name.IsNone ())
            {
                Parts.Add (Name.ToString ());
            }
        }

        return FString::Join (Parts, TEXT (", "));
    }

    TArray<FName> ParseCommaSeparatedNames (const FString& Text)
    {
        TArray<FString> Parts;
        Text.ParseIntoArray (Parts, TEXT (","), true);

        TArray<FName> Names;
        Names.Reserve (Parts.Num ());

        for (FString& Part : Parts)
        {
            Part.TrimStartAndEndInline ();
            if (!Part.IsEmpty ())
            {
                Names.Add (FName (*Part));
            }
        }

        return Names;
    }
}

#endif
