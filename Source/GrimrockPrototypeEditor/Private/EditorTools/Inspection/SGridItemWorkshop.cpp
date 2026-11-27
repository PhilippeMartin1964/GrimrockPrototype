#include "SGridItemWorkshop.h"

#include "SGridItemWorkshopViewport.h"
#include "Components/StaticMeshComponent.h"
#include "Editor.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstance.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Misc/TransactionObjectEvent.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/Package.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/STableRow.h"

#define LOCTEXT_NAMESPACE "GrimrockItemWorkshop"

namespace
{
	bool IsProjectAsset(const UObject* Object)
	{
		return IsValid(Object) && Object->GetOutermost()->GetName().StartsWith(TEXT("/Game/"));
	}
}

void SGridItemWorkshop::Construct(const FArguments&)
{
	SAssignNew(PreviewViewport, SGridItemWorkshopViewport);
	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(6)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1)
			[
				SNew(STextBlock).Text(LOCTEXT("Title", "Grimrock — Atelier d'objets | Lecture seule"))
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SButton).Text(LOCTEXT("Refresh", "Actualiser les assets"))
				.OnClicked_Lambda([this]() { RefreshRegistry(); return FReply::Handled(); })
			]
		]
		+ SVerticalBox::Slot().FillHeight(1)
		[
			SNew(SSplitter)
			+ SSplitter::Slot().Value(0.25f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(4)
				[
					SNew(SSearchBox).HintText(LOCTEXT("Search", "Nom, ItemId ou ARMXXX"))
					.OnTextChanged_Lambda([this](const FText& Text) { Search = Text.ToString(); RefreshList(); })
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(4)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1)
					[
						SNew(SButton).Text(LOCTEXT("All", "Tous"))
						.OnClicked_Lambda([this]() { Filter = EGridWorkshopFilter::All; RefreshList(); return FReply::Handled(); })
					]
					+ SHorizontalBox::Slot().FillWidth(1)
					[
						SNew(SButton).Text(LOCTEXT("Arms", "ARM"))
						.OnClicked_Lambda([this]() { Filter = EGridWorkshopFilter::Armaments; RefreshList(); return FReply::Handled(); })
					]
					+ SHorizontalBox::Slot().FillWidth(1)
					[
						SNew(SButton).Text(LOCTEXT("Equipment", "Équip."))
						.OnClicked_Lambda([this]() { Filter = EGridWorkshopFilter::Equipment; RefreshList(); return FReply::Handled(); })
					]
					+ SHorizontalBox::Slot().FillWidth(1)
					[
						SNew(SButton).Text(LOCTEXT("Other", "Autres"))
						.OnClicked_Lambda([this]() { Filter = EGridWorkshopFilter::Other; RefreshList(); return FReply::Handled(); })
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(4)
				[
					SNew(STextBlock).Text_Lambda([this]() {
						return FText::Format(LOCTEXT("Count", "{0} / {1} objets"), FText::AsNumber(Filtered.Num()), FText::AsNumber(Session.GetEntries().Num()));
					})
				]
				+ SVerticalBox::Slot().FillHeight(1)
				[
					SAssignNew(ItemList, SListView<TSharedPtr<FGridWorkshopEntry>>)
					.ListItemsSource(&Filtered).SelectionMode(ESelectionMode::Single)
					.OnGenerateRow(this, &SGridItemWorkshop::MakeItemRow)
					.OnSelectionChanged(this, &SGridItemWorkshop::OnSelection)
				]
			]
			+ SSplitter::Slot().Value(0.50f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(4)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1)
					[
						SNew(SButton).Text(LOCTEXT("Frame", "Recentrer"))
						.OnClicked_Lambda([this]() { PreviewViewport->Frame(); return FReply::Handled(); })
					]
					+ SHorizontalBox::Slot().FillWidth(1)
					[
						SNew(SButton).Text(LOCTEXT("Front", "Face (+X)"))
						.OnClicked_Lambda([this]() { PreviewViewport->Frame(FRotator(0, 180, 0)); return FReply::Handled(); })
					]
					+ SHorizontalBox::Slot().FillWidth(1)
					[
						SNew(SButton).Text(LOCTEXT("Side", "Profil (+Y)"))
						.OnClicked_Lambda([this]() { PreviewViewport->Frame(FRotator(0, -90, 0)); return FReply::Handled(); })
					]
					+ SHorizontalBox::Slot().FillWidth(1)
					[
						SNew(SButton).Text(LOCTEXT("Back", "Dos (-X)"))
						.OnClicked_Lambda([this]() { PreviewViewport->Frame(FRotator::ZeroRotator); return FReply::Handled(); })
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(4)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(SButton)
						.IsEnabled_Lambda([this]() { return Session.GetView().HasLevel(); })
						.Text_Lambda([this]() { return bShowComparison ? LOCTEXT("SingleView", "Vue simple") : LOCTEXT("CompareAB", "Comparer A/B"); })
						.OnClicked_Lambda([this]() { ToggleComparison(); return FReply::Handled(); })
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(8, 0)
					[
						SNew(SCheckBox)
						.Visibility_Lambda([this]() { return bShowComparison ? EVisibility::Visible : EVisibility::Collapsed; })
						.IsChecked_Lambda([this]() { return bSynchronizeViews ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
						.OnCheckStateChanged_Lambda([this](ECheckBoxState State) { bSynchronizeViews = State == ECheckBoxState::Checked; LinkViews(); })
						[
							SNew(STextBlock).Text(LOCTEXT("LinkViews", "Synchroniser les vues"))
						]
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(4)
				[
					MakeContextControls()
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(4)
				[
					SNew(SVerticalBox)
					.Visibility_Lambda([this]() { return Session.IsRoughnessPilot() ? EVisibility::Visible : EVisibility::Collapsed; })
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock).AutoWrapText(true)
						.Text(LOCTEXT("RoughnessTrialLabel", "ARM014 — essai temporaire N1 / N2, rugosité uniquement (tous les slots compatibles)"))
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1)
						[
							SNew(SButton).Text(LOCTEXT("Roughness85", "Essai × 0,85"))
							.OnClicked_Lambda([this]() { StartRoughnessTrial(0.85f); return FReply::Handled(); })
						]
						+ SHorizontalBox::Slot().FillWidth(1)
						[
							SNew(SButton).Text(LOCTEXT("Roughness70", "Essai × 0,70"))
							.OnClicked_Lambda([this]() { StartRoughnessTrial(0.70f); return FReply::Handled(); })
						]
						+ SHorizontalBox::Slot().FillWidth(1)
						[
							SNew(SButton).Text(LOCTEXT("RoughnessReset", "Rendu canonique"))
							.IsEnabled_Lambda([this]() { return RoughnessTrialScale != 1.0f; })
							.OnClicked_Lambda([this]() { ResetRoughnessTrial(); return FReply::Handled(); })
						]
					]
				]
				+ SVerticalBox::Slot().FillHeight(1)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1)
					[
						MakePreviewPane(PreviewViewport.ToSharedRef(), false)
					]
					+ SHorizontalBox::Slot().FillWidth(1)
					[
						SAssignNew(ComparisonHost, SBox)
						.Visibility_Lambda([this]() { return bShowComparison ? EVisibility::Visible : EVisibility::Collapsed; })
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(6)
				[
					SNew(STextBlock).AutoWrapText(true)
					.Text(LOCTEXT("Navigation", "Clic gauche : rotation 360° · Clic droit ou milieu : déplacement · Molette : zoom de détail · Recentrer : vue initiale"))
				]
			]
			+ SSplitter::Slot().Value(0.25f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().FillHeight(0.65f).Padding(6)
				[
					SNew(SScrollBox)
					+ SScrollBox::Slot()
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(STextBlock).AutoWrapText(true).Text_Lambda([this]() { return DetailsText; })
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 8)
						[
							SNew(STextBlock).AutoWrapText(true)
							.Visibility_Lambda([this]() { return bShowComparison ? EVisibility::Visible : EVisibility::Collapsed; })
							.Text_Lambda([this]() { return ComparisonDetails; })
						]
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(6)
				[
					SNew(STextBlock).AutoWrapText(true).Text(LOCTEXT("Sources", "Sources — ouvrir l'asset natif (édition réelle)"))
				]
				+ SVerticalBox::Slot().FillHeight(0.35f)
				[
					SAssignNew(AssetList, SListView<TSharedPtr<FGridWorkshopAssetLink>>)
					.ListItemsSource(&Assets).SelectionMode(ESelectionMode::None)
					.OnGenerateRow(this, &SGridItemWorkshop::MakeAssetRow)
				]
			]
		]
	];

	ChangedHandle = FCoreUObjectDelegates::OnObjectPropertyChanged.AddSP(this, &SGridItemWorkshop::OnAssetChanged);
	TransactedHandle = FCoreUObjectDelegates::OnObjectTransacted.AddSP(this, &SGridItemWorkshop::OnAssetTransacted);
	RefreshRegistry(true);
}

SGridItemWorkshop::~SGridItemWorkshop()
{
	FCoreUObjectDelegates::OnObjectPropertyChanged.Remove(ChangedHandle);
	FCoreUObjectDelegates::OnObjectTransacted.Remove(TransactedHandle);
}

void SGridItemWorkshop::Tick(const FGeometry& Geometry, double Time, float Delta)
{
	SCompoundWidget::Tick(Geometry, Time, Delta);
	if (bRefreshPending)
	{
		RefreshRegistry();
	}
	if (PendingLevel != INDEX_NONE)
	{
		const int32 Level = PendingLevel;
		PendingLevel = INDEX_NONE;
		if (Session.GetSelectedLevel() != Level)
		{
			const bool bWasTrial = RoughnessTrialScale != 1.0f;
			RoughnessTrialScale = 1.0f;
			Session.SelectLevel(Level);
			RefreshSelection(false);
			if (bWasTrial)
			{
				RefreshComparison(false);
			}
		}
	}
	if (PendingComparisonLevel != INDEX_NONE)
	{
		const int32 Level = PendingComparisonLevel;
		PendingComparisonLevel = INDEX_NONE;
		if (bShowComparison && ComparisonLevel != Level)
		{
			RoughnessTrialScale = 1.0f;
			ComparisonLevel = Level;
			RefreshComparison(false);
		}
	}
}

TSharedRef<SWidget> SGridItemWorkshop::MakePreviewPane(const TSharedRef<SGridItemWorkshopViewport>& Viewport, bool bComparison)
{
	const auto GetLevel = [this, bComparison]()
	{
		const int32 Pending = bComparison ? PendingComparisonLevel : PendingLevel;
		return Pending == INDEX_NONE ? (bComparison ? ComparisonLevel : Session.GetSelectedLevel()) : Pending;
	};
	const auto Queue = [this, bComparison](int32 Level)
	{
		if (bComparison)
		{
			QueueComparisonLevel(Level);
		}
		else
		{
			QueueLevel(Level);
		}
	};
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(STextBlock)
			.Visibility_Lambda([this]() { return bShowComparison ? EVisibility::Visible : EVisibility::Collapsed; })
			.Text(bComparison ? LOCTEXT("ViewB", "B") : LOCTEXT("ViewA", "A"))
		]
		+ SVerticalBox::Slot().FillHeight(1)
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				Viewport
			]
			+ SOverlay::Slot()
			[
				SNew(SGridItemWorkshopOrbitInput).PreviewViewport(Viewport).Visibility(EVisibility::Visible)
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(4)
		[
			SNew(SHorizontalBox)
			.Visibility_Lambda([this]() { return Session.GetView().HasLevel() ? EVisibility::Visible : EVisibility::Collapsed; })
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SButton).Text(LOCTEXT("Previous", "−"))
				.OnClicked_Lambda([GetLevel, Queue]() { Queue(GetLevel() - 1); return FReply::Handled(); })
			]
			+ SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center).Padding(6, 0)
			[
				SNew(SSlider).StepSize(1.0f / 19.0f)
				.Value_Lambda([GetLevel]() { return (FMath::Clamp(GetLevel(), 1, 20) - 1) / 19.0f; })
				.OnValueChanged_Lambda([Queue](float Value) { Queue(1 + FMath::RoundToInt(Value * 19.0f)); })
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(6, 0)
			[
				SNew(STextBlock).Text_Lambda([GetLevel]() { return FText::Format(LOCTEXT("Level", "N{0}"), FText::AsNumber(GetLevel())); })
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SButton).Text(LOCTEXT("Next", "+"))
				.OnClicked_Lambda([GetLevel, Queue]() { Queue(GetLevel() + 1); return FReply::Handled(); })
			]
		];
}

TSharedRef<SWidget> SGridItemWorkshop::MakeContextControls()
{
	const TSharedRef<SHorizontalBox> ContextRow = SNew(SHorizontalBox);
	const TSharedRef<SHorizontalBox> LightingRow = SNew(SHorizontalBox);
	const TPair<EGridWorkshopContext, FText> Contexts[] = {
		{EGridWorkshopContext::Examination, LOCTEXT("ContextExam", "Examen")},
		{EGridWorkshopContext::Floor, LOCTEXT("ContextFloor", "Sol")},
		{EGridWorkshopContext::Decor, LOCTEXT("ContextDecor", "Décor")},
		{EGridWorkshopContext::Held, LOCTEXT("ContextHeld", "Main (mesh)")},
		{EGridWorkshopContext::Flight, LOCTEXT("ContextFlight", "Vol (pose)")},
	};
	for (const auto& Entry : Contexts)
	{
		const EGridWorkshopContext Value = Entry.Key;
		const FText Label = Entry.Value;
		ContextRow->AddSlot().FillWidth(1)
		[
			SNew(SButton).Text(Label)
			.IsEnabled_Lambda([this, Value]() { return Value != Context && (Value == EGridWorkshopContext::Examination || FGridWorkshopContext::CanShow(Session.GetView(), Value)); })
			.OnClicked_Lambda([this, Value]() { SetContext(Value); return FReply::Handled(); })
		];
	}
	const TPair<EGridWorkshopLighting, FText> Lights[] = {
		{EGridWorkshopLighting::Studio, LOCTEXT("LightStudio", "Studio")},
		{EGridWorkshopLighting::Torch, LOCTEXT("LightTorch", "Torche")},
		{EGridWorkshopLighting::Dungeon, LOCTEXT("LightDungeon", "Donjon")},
	};
	for (const auto& Entry : Lights)
	{
		const EGridWorkshopLighting Value = Entry.Key;
		LightingRow->AddSlot().FillWidth(1)
		[
			SNew(SButton).Text(Entry.Value)
			.IsEnabled_Lambda([this, Value]() { return Value != Lighting; })
			.OnClicked_Lambda([this, Value]() { SetLighting(Value); return FReply::Handled(); })
		];
	}
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[ContextRow]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 3)[LightingRow];
}

void SGridItemWorkshop::SetContext(EGridWorkshopContext NewContext)
{
	Context = NewContext;
	if (Context == EGridWorkshopContext::Held || Context == EGridWorkshopContext::Flight)
	{
		RoughnessTrialScale = 1.0f;
	}
	RefreshSelection(false);
	RefreshComparison(false);
}

void SGridItemWorkshop::SetLighting(EGridWorkshopLighting NewLighting)
{
	Lighting = NewLighting;
	PreviewViewport->SetLighting(Lighting);
	if (ComparisonViewport.IsValid())
	{
		ComparisonViewport->SetLighting(Lighting);
	}
}

void SGridItemWorkshop::ToggleComparison()
{
	bShowComparison = !bShowComparison && Session.GetView().HasLevel();
	if (bShowComparison)
	{
		SAssignNew(ComparisonViewport, SGridItemWorkshopViewport);
		ComparisonViewport->SetLighting(Lighting);
		ComparisonHost->SetContent(MakePreviewPane(ComparisonViewport.ToSharedRef(), true));
		RefreshComparison(true);
	}
	else
	{
		PendingComparisonLevel = INDEX_NONE;
		RoughnessTrialScale = 1.0f;
		ComparisonHost->SetContent(SNullWidget::NullWidget);
		ComparisonViewport.Reset();
		ComparisonDetails = FText::GetEmpty();
	}
	LinkViews();
}

void SGridItemWorkshop::LinkViews()
{
	const bool bLink = bShowComparison && bSynchronizeViews && ComparisonViewport.IsValid();
	PreviewViewport->SetLinkedViewport(bLink ? ComparisonViewport : nullptr);
	if (ComparisonViewport.IsValid())
	{
		ComparisonViewport->SetLinkedViewport(bLink ? PreviewViewport : nullptr);
		if (bLink)
		{
			ComparisonViewport->MatchFraming(*PreviewViewport);
		}
	}
}

void SGridItemWorkshop::RefreshComparison(bool bRecenter)
{
	if (!bShowComparison || !ComparisonViewport.IsValid())
	{
		return;
	}
	if (!Session.GetView().HasLevel())
	{
		ToggleComparison();
		return;
	}
	FGridItemInspectionView ComparisonView;
	const bool bTrialRequested = RoughnessTrialScale != 1.0f;
	const bool bTrialApplied = bTrialRequested && ComparisonLevel == 2 &&
		Session.ResolveRoughnessTrial(RoughnessTrialScale, ComparisonView);
	if (!bTrialApplied)
	{
		RoughnessTrialScale = 1.0f;
		Session.ResolveComparison(ComparisonLevel, ComparisonView);
	}
	const EGridItemPreviewResult Result = ComparisonViewport->Display(ComparisonView, bRecenter, Context);
	if (bSynchronizeViews)
	{
		ComparisonViewport->MatchFraming(*PreviewViewport);
	}
	ComparisonDetails = FText::Format(LOCTEXT("DetailsB", "B — {0}"), BuildDetails(ComparisonView, *ComparisonViewport, Result));
	if (bTrialRequested)
	{
		int32 DynamicSlots = 0;
		if (bTrialApplied && Result == EGridItemPreviewResult::Ready)
		{
			if (const UStaticMeshComponent* Mesh = ComparisonViewport->GetMesh())
			{
				for (int32 Slot = 0; Slot < Mesh->GetNumMaterials(); ++Slot)
				{
					DynamicSlots += Cast<UMaterialInstanceDynamic>(Mesh->GetMaterial(Slot)) != nullptr ? 1 : 0;
				}
			}
		}
		const FText Trial = bTrialApplied && DynamicSlots > 0
			? FText::Format(LOCTEXT("TrialApplied", "ESSAI NON ENREGISTRÉ — rugosité relative × {0}, {1} slot(s) dynamique(s). Vérifier lame ET poignée.\n"),
				FText::AsNumber(RoughnessTrialScale), FText::AsNumber(DynamicSlots))
			: LOCTEXT("TrialUnavailable", "ESSAI NON APPLIQUÉ — projection ou matériau incompatible.\n");
		ComparisonDetails = FText::Format(LOCTEXT("TrialDetails", "{0}{1}"), Trial, ComparisonDetails);
	}
}

void SGridItemWorkshop::StartRoughnessTrial(float RelativeScale)
{
	if (!Session.IsRoughnessPilot())
	{
		return;
	}
	PendingLevel = INDEX_NONE;
	PendingComparisonLevel = INDEX_NONE;
	if (!Session.SelectLevel(1))
	{
		ResetRoughnessTrial();
		RefreshSelection(false);
		return;
	}
	if (Context == EGridWorkshopContext::Held || Context == EGridWorkshopContext::Flight)
	{
		Context = EGridWorkshopContext::Examination;
	}
	ComparisonLevel = 2;
	RoughnessTrialScale = RelativeScale;
	RefreshSelection(false);
	if (!bShowComparison)
	{
		ToggleComparison();
	}
	else
	{
		RefreshComparison(false);
	}
}

void SGridItemWorkshop::ResetRoughnessTrial()
{
	RoughnessTrialScale = 1.0f;
	RefreshComparison(false);
}

void SGridItemWorkshop::QueueComparisonLevel(int32 Level)
{
	if (bShowComparison && Session.GetView().HasLevel())
	{
		PendingComparisonLevel = FMath::Clamp(Level, 1, 20);
	}
}

void SGridItemWorkshop::RefreshRegistry(bool bInitial)
{
	bRefreshPending = false;
	PendingLevel = INDEX_NONE;
	PendingComparisonLevel = INDEX_NONE;
	RoughnessTrialScale = 1.0f;
	Session.Rebuild();
	if (bInitial && Session.GetSelectedId().IsNone() && !Session.GetEntries().IsEmpty())
	{
		const TSharedPtr<FGridWorkshopEntry>* Pilot = Session.GetEntries().FindByPredicate(
			[](const TSharedPtr<FGridWorkshopEntry>& Entry) { return Entry->ArchetypeId == FName(TEXT("ARM014")); });
		Session.Select(Pilot ? (*Pilot)->Id : Session.GetEntries()[0]->Id);
	}
	RefreshList();
	RefreshSelection(true);
}

void SGridItemWorkshop::RefreshList()
{
	Session.Filter(Search, Filter, Filtered);
	if (!ItemList.IsValid())
	{
		return;
	}
	ItemList->RequestListRefresh();
	const TSharedPtr<FGridWorkshopEntry>* Found = Filtered.FindByPredicate(
		[this](const TSharedPtr<FGridWorkshopEntry>& Entry) { return Entry->Id == Session.GetSelectedId(); });
	if (Found)
	{
		ItemList->SetSelection(*Found, ESelectInfo::Direct);
	}
	else
	{
		ItemList->ClearSelection();
	}
}

void SGridItemWorkshop::RefreshSelection(bool bRecenter)
{
	const bool bContextUnavailable = Context != EGridWorkshopContext::Examination && !FGridWorkshopContext::CanShow(Session.GetView(), Context);
	if (bContextUnavailable)
	{
		Context = EGridWorkshopContext::Examination;
	}
	PreviewResult = PreviewViewport->Display(Session.GetView(), bRecenter, Context);
	UpdateAssets();
	DetailsText = BuildDetails(Session.GetView(), *PreviewViewport, PreviewResult);
	if (bRecenter || bContextUnavailable || !Session.GetView().HasLevel())
	{
		RefreshComparison(bRecenter);
	}
	else if (bShowComparison && bSynchronizeViews)
	{
		ComparisonViewport->MatchFraming(*PreviewViewport);
	}
}

void SGridItemWorkshop::OnAssetChanged(UObject* Object, FPropertyChangedEvent&)
{
	bRefreshPending |= IsProjectAsset(Object);
}

void SGridItemWorkshop::OnAssetTransacted(UObject* Object, const FTransactionObjectEvent&)
{
	bRefreshPending |= IsProjectAsset(Object);
}

void SGridItemWorkshop::OnSelection(TSharedPtr<FGridWorkshopEntry> Entry, ESelectInfo::Type SelectInfo)
{
	if (!Entry.IsValid() || SelectInfo == ESelectInfo::Direct)
	{
		return;
	}
	PendingLevel = INDEX_NONE;
	PendingComparisonLevel = INDEX_NONE;
	RoughnessTrialScale = 1.0f;
	Session.Select(Entry->Id);
	RefreshSelection(true);
}

TSharedRef<ITableRow> SGridItemWorkshop::MakeItemRow(TSharedPtr<FGridWorkshopEntry> Entry, const TSharedRef<STableViewBase>& Owner)
{
	const FString Prefix = Entry->ArchetypeId.IsNone() ? Entry->Id.ToString() : Entry->ArchetypeId.ToString();
	const FString Text = FString::Printf(TEXT("%s — %s%s"), *Prefix, *Entry->Name.ToString(), Entry->bResolved ? TEXT("") : TEXT(" [!]"));
	return SNew(STableRow<TSharedPtr<FGridWorkshopEntry>>, Owner)
	[
		SNew(STextBlock).Text(FText::FromString(Text)).ToolTipText(FText::FromName(Entry->Id))
	];
}

void SGridItemWorkshop::QueueLevel(int32 Level)
{
	if (Session.GetView().HasLevel())
	{
		PendingLevel = FMath::Clamp(Level, 1, 20);
	}
}

FText SGridItemWorkshop::BuildDetails(const FGridItemInspectionView& View, const SGridItemWorkshopViewport& Viewport, EGridItemPreviewResult Result) const
{
	if (!View.IsValid())
	{
		const FString Error = Session.GetError();
		return Error.IsEmpty() ? FText::Format(LOCTEXT("Invalid", "Aucune projection valide. Résolution : {0}"),
			FText::FromString(StaticEnum<EGridItemResolveError>()->GetNameStringByValue(static_cast<int64>(View.Item.Error)))) : FText::FromString(Error);
	}
	const FGridResolvedItem& Item = View.Item;
	FString Details = FString::Printf(TEXT("%s\n%s\n\nItemId : %s\nPoids : %.3f kg\n%s\n"),
		*Item.DisplayName.ToString(), *Item.Description.ToString(), *Item.ItemId.ToString(), Item.WeightKg,
		View.bCanRead ? TEXT("Objet lisible") : TEXT(""));
	if (View.HasLevel())
	{
		Details += FString::Printf(TEXT("\n%s — N%d\nFinition : %s\nRugosité × %.3f\nNormales × %.3f\nTeinte : %s\nProgression fine non authorée à ce stade.\n"),
			*Item.ArchetypeId.ToString(), Item.ItemLevel, *Item.WeaponFinish.FinishId.ToString(), Item.WeaponFinish.RoughnessScale,
			Item.WeaponFinish.NormalScale, *Item.WeaponFinish.Tint.ToString());
	}
	if (View.HasAttackStats())
	{
		const auto& W = Item.Weapon;
		Details += FString::Printf(TEXT("\nDégâts : %dd%d %+d\nPrécision : %+d\nPA : %d\nPortée : %d cases\n"),
			W.DamageDice.DiceCount, W.DamageDice.DiceSides, W.DamageDice.FlatBonus, W.AccuracyBonus, W.ActionPointCost, W.RangeCells);
	}
	if (View.HasProtection())
	{
		Details += FString::Printf(TEXT("\nProtection nominale physique : %d\nProtection nominale magique : %d\n"),
			View.GetNominalPhysicalArmor(), View.GetNominalMagicalArmor());
	}
	if (View.HasEquipment() && !Item.bIsArmament)
	{
		Details += TEXT("\nÉquipement non ARM :\n");
		for (EGridEquipmentSlot Slot : Item.NonArmEquipmentSlots)
		{
			Details += StaticEnum<EGridEquipmentSlot>()->GetNameStringByValue(static_cast<int64>(Slot)) + TEXT("\n");
		}
	}
	if (View.HasActionBindings())
	{
		Details += TEXT("\nActions configurées :\n");
		for (const FGridItemActionBinding& Binding : Item.AvailableActions)
		{
			Details += Binding.ActionId.ToString() + TEXT(" / ") + Binding.PresentationId.ToString() + TEXT("\n");
		}
	}
	if (Context == EGridWorkshopContext::Held)
	{
		Details += TEXT("\nMain : mesh équipé émetteur, sans pose du personnage ni lumière active simulée.\n");
	}
	else if (Context == EGridWorkshopContext::Flight)
	{
		Details += TEXT("\nVol : pose statique de lancement d'un objet physique, animation réservée au simulateur.\n");
	}
	else if (FGridWorkshopContext::HasFloor(Context))
	{
		Details += TEXT("\nSupport technique à l'échelle native, pose statique sans simulation de chute.\n");
	}
	if (Viewport.HasStageError())
	{
		Details += TEXT("\nSupport de décor indisponible.\n");
	}
	if (Result != EGridItemPreviewResult::Ready)
	{
		Details += TEXT("\nMesh non affiché : ressource manquante ou hôte invalide.\n");
	}
	Details += FString::Printf(TEXT("\nMesh monde : %s\nTransform : %s\nEffets persistants configurés : %d\n"),
		*Item.WorldMesh.ToSoftObjectPath().ToString(), *Item.WorldVisualTransform.ToString(), Item.VisualEffects.Effects.Num());
	UStaticMeshComponent* Mesh = Viewport.GetMesh();
	if (Mesh && Mesh->GetStaticMesh())
	{
		Details += FString::Printf(TEXT("\nMesh affiché : %s\nDimensions (AABB, cm) : %s\n"), *Mesh->GetStaticMesh()->GetName(), *(Mesh->Bounds.BoxExtent * 2.0).ToString());
		const TArray<FName> SlotNames = Mesh->GetMaterialSlotNames();
		for (int32 Slot = 0; Slot < Mesh->GetNumMaterials(); ++Slot)
		{
			const UMaterialInterface* Material = Mesh->GetMaterial(Slot);
			Details += FString::Printf(TEXT("\nSlot %d : %s\n%s\n"), Slot, SlotNames.IsValidIndex(Slot) ? *SlotNames[Slot].ToString() : TEXT(""),
				Material ? *Material->GetName() : TEXT("—"));
			if (!Material)
			{
				continue;
			}
			if (const UMaterialInterface* Base = Mesh->GetStaticMesh()->GetMaterial(Slot))
			{
				Details += TEXT("  Source : ") + Base->GetPathName() + TEXT("\n");
				if (const UMaterialInstance* Instance = Cast<UMaterialInstance>(Base))
				{
					if (Instance->Parent)
					{
						Details += TEXT("  Parent : ") + Instance->Parent->GetPathName() + TEXT("\n");
					}
				}
			}
			for (const FName Parameter : { FName(TEXT("RoughnessMultiplier")), FName(TEXT("RoughnessOverride")), FName(TEXT("NormalStrength")) })
			{
				float Value = 0.0f;
				if (Material->GetScalarParameterValue(FMaterialParameterInfo(Parameter), Value))
				{
					Details += FString::Printf(TEXT("  %s : %.3f\n"), *Parameter.ToString(), Value);
				}
			}
			FLinearColor Tint;
			if (Material->GetVectorParameterValue(FMaterialParameterInfo(TEXT("BaseColorTint")), Tint))
			{
				Details += TEXT("  BaseColorTint : ") + Tint.ToString() + TEXT("\n");
			}
		}
	}
	return FText::FromString(Details);
}

void SGridItemWorkshop::UpdateAssets()
{
	Assets.Reset();
	const FGridItemInspectionView& View = Session.GetView();
	const auto Add = [this](const FText& Label, const FSoftObjectPath& Path)
	{
		if (!Path.IsNull() && !Assets.ContainsByPredicate([&Path](const TSharedPtr<FGridWorkshopAssetLink>& Existing) { return Existing->Path == Path; }))
		{
			TSharedPtr<FGridWorkshopAssetLink> Entry = MakeShared<FGridWorkshopAssetLink>();
			Entry->Label = Label;
			Entry->Path = Path;
			Assets.Add(Entry);
		}
	};
	UGridItemSystemAsset* Root = GetDefault<UGridItemSubsystem>()->LoadConfiguredItemSystemForPreview();
	if (IsValid(Root))
	{
		Add(LOCTEXT("Root", "Registre DA_ItemSystem"), FSoftObjectPath(Root));
		if (View.IsValid())
		{
			for (const auto& Definition : Root->ItemDefinitions)
			{
				if (Definition && Definition->ItemId == View.Item.ItemId)
				{
					Add(LOCTEXT("Definition", "Fiche de l'objet"), FSoftObjectPath(Definition.Get()));
				}
			}
			if (View.Item.bIsArmament)
			{
				Add(LOCTEXT("WeaponCatalog", "Catalogue mécanique"), FSoftObjectPath(Root->WeaponCatalogue.Get()));
				Add(LOCTEXT("VisualCatalog", "Catalogue visuel"), FSoftObjectPath(Root->WeaponVisualCatalogue.Get()));
			}
			if (View.HasActionBindings())
			{
				Add(LOCTEXT("ActionCatalog", "Catalogue d'actions"), FSoftObjectPath(Root->ActionCatalogue.Get()));
				Add(LOCTEXT("PresentationCatalog", "Catalogue de présentation"), FSoftObjectPath(Root->ActionPresentationCatalogue.Get()));
			}
		}
	}
	if (View.IsValid())
	{
		Add(LOCTEXT("WorldMesh", "Mesh monde"), View.Item.WorldMesh.ToSoftObjectPath());
		Add(LOCTEXT("EquippedMesh", "Mesh équipé"), View.Item.EquippedMesh.ToSoftObjectPath());
		Add(LOCTEXT("ProjectileMesh", "Mesh projectile"), View.Item.ProjectileMesh.ToSoftObjectPath());
		Add(LOCTEXT("Icon", "Icône"), View.Item.ItemIcon.ToSoftObjectPath());
		if (UStaticMesh* Mesh = View.Item.WorldMesh.Get())
		{
			for (const FStaticMaterial& Slot : Mesh->GetStaticMaterials())
			{
				if (Slot.MaterialInterface)
				{
					Add(FText::FromString(Slot.MaterialInterface->GetName()), FSoftObjectPath(Slot.MaterialInterface.Get()));
				}
			}
		}
		for (const FGridItemNiagaraEffectConfig& Effect : View.Item.VisualEffects.Effects)
		{
			Add(FText::FromName(Effect.EffectId), Effect.NiagaraSystem.ToSoftObjectPath());
		}
	}
	if (AssetList.IsValid())
	{
		AssetList->RequestListRefresh();
	}
}

TSharedRef<ITableRow> SGridItemWorkshop::MakeAssetRow(TSharedPtr<FGridWorkshopAssetLink> Entry, const TSharedRef<STableViewBase>& Owner)
{
	return SNew(STableRow<TSharedPtr<FGridWorkshopAssetLink>>, Owner)
	[
		SNew(SButton).Text(Entry->Label).ToolTipText(FText::FromString(Entry->Path.ToString()))
		.OnClicked_Lambda([Entry]()
		{
			if (GEditor)
			{
				if (UObject* Object = Entry->Path.TryLoad())
				{
					if (UAssetEditorSubsystem* Editors = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
					{
						Editors->OpenEditorForAsset(Object);
					}
				}
			}
			return FReply::Handled();
		})
	];
}

#undef LOCTEXT_NAMESPACE
