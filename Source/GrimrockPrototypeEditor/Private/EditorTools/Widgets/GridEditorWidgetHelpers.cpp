#include "EditorTools/Widgets/GridEditorWidgetHelpers.h"

#if WITH_EDITOR

#include "Core/GridObjectArchetypeAsset.h"
#include "Core/GridObjectPaletteAsset.h"
#include "Core/GridTypes.h"

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
            case EGridLevelObjectType::ItemSpawn:     return FText::FromString (TEXT ("S"));
            case EGridLevelObjectType::Item:          return FText::FromString (TEXT ("I"));
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

    TSharedRef<SWidget> BuildGridPanelSection (const FText& Title, TSharedRef<SWidget> Content)
    {
        return SNew (SBorder)
            .Padding (6.f)
            .BorderImage (FAppStyle::GetBrush ("ToolPanel.GroupBorder"))
            [
                SNew (SVerticalBox)

                    + SVerticalBox::Slot ()
                    .AutoHeight ()
                    .Padding (0.f, 0.f, 0.f, 6.f)
                    [
                        SNew (STextBlock)
                            .Text (Title)
                            .Font (FAppStyle::GetFontStyle ("DetailsView.CategoryFontStyle"))
                    ]

                    + SVerticalBox::Slot ()
                    .AutoHeight ()
                    [
                        Content
                    ]
            ];
    }

    TSharedRef<SWidget> BuildGridCollapsiblePanelSection (
        const FText& Title,
        const TFunctionRef<TSharedRef<SWidget> ()>& BuildContent,
        bool bExpanded,
        const FOnClicked& OnToggleClicked)
    {
        TSharedRef<SVerticalBox> SectionBox = SNew (SVerticalBox)

            + SVerticalBox::Slot ()
            .AutoHeight ()
            .Padding (0.f, 0.f, 0.f, bExpanded ? 6.f : 0.f)
            [
                SNew (SButton)
                    .ButtonStyle (&FCoreStyle::Get ().GetWidgetStyle<FButtonStyle> ("NoBorder"))
                    .ContentPadding (FMargin (2.f, 1.f))
                    .HAlign (HAlign_Fill)
                    .OnClicked (OnToggleClicked)
                    [
                        SNew (SHorizontalBox)

                        + SHorizontalBox::Slot ()
                        .AutoWidth ()
                        .VAlign (VAlign_Center)
                        .Padding (0.f, 0.f, 5.f, 0.f)
                        [
                            SNew (STextBlock)
                                .Text (bExpanded
                                    ? FText::FromString (FString::Chr (0x25BC))
                                    : FText::FromString (FString::Chr (0x25B6)))
                                .Font (FCoreStyle::GetDefaultFontStyle ("Regular", 8))
                        ]

                        + SHorizontalBox::Slot ()
                        .FillWidth (1.f)
                        .VAlign (VAlign_Center)
                        [
                            SNew (STextBlock)
                                .Text (Title)
                                .Font (FAppStyle::GetFontStyle ("DetailsView.CategoryFontStyle"))
                        ]
                    ]
            ];

        if (bExpanded)
        {
            SectionBox->AddSlot ()
                .AutoHeight ()
                [
                    BuildContent ()
                ];
        }

        return SNew (SBorder)
            .Padding (6.f)
            .BorderImage (FAppStyle::GetBrush ("ToolPanel.GroupBorder"))
            [
                SectionBox
            ];
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

    TArray<FGridArchetypeOption> GetItemArchetypeOptions (const UGridObjectPaletteAsset* ObjectPalette)
    {
        TArray<FGridArchetypeOption> Options;
        if (!ObjectPalette)
        {
            return Options;
        }

        TSet<FName> SeenIds;
        for (const FGridObjectPaletteEntry& Entry : ObjectPalette->Entries)
        {
            const UGridObjectArchetypeAsset* Archetype = Entry.DefaultArchetype;
            if (!Archetype ||
                Archetype->ArchetypeId.IsNone () ||
                Archetype->SupportedType != EGridLevelObjectType::Item ||
                SeenIds.Contains (Archetype->ArchetypeId))
            {
                continue;
            }

            FGridArchetypeOption Option;
            Option.ArchetypeId = Archetype->ArchetypeId;
            Option.Label = !Archetype->DisplayName.IsEmpty ()
                ? Archetype->DisplayName
                : FText::FromName (Archetype->ArchetypeId);
            Options.Add (Option);
            SeenIds.Add (Option.ArchetypeId);
        }

        Options.Sort ([] (const FGridArchetypeOption& A, const FGridArchetypeOption& B)
        {
            return A.Label.ToString () < B.Label.ToString ();
        });

        return Options;
    }
}

#endif
