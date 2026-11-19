#include "Runtime/GridReceptacleActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Runtime/GridInteractionUtils.h"
#include "Runtime/GridEventFeedPublisher.h"
#include "Runtime/GridItemActor.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "GridItemWeightReadModel.h"
#include "GridReceptacleItemMetadataReadModel.h"
#include "GridReceptacleContainedIdentityReadModel.h"
#include "GridReceptacleInitialCanonicalReadModel.h"
#include "GridReceptacleAcceptedCanonicalReadModel.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Runtime/Combat/GridWeaponCatalogueAsset.h"
#include "Runtime/GridItemTransferService.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "UI/UILocalizationText.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY(LogGridReceptacle);

namespace
{
	static constexpr float MultiItemVisualSpacing = 18.0f;
	static constexpr float InitialPhysicalContentSpacing = 25.0f;

	FVector ComputeInitialPhysicalContentOffset(int32 ItemIndex)
	{
		if (ItemIndex <= 0)
		{
			return FVector::ZeroVector;
		}

		const int32 PairIndex = (ItemIndex + 1) / 2;
		const float Direction = ItemIndex % 2 == 1 ? 1.0f : -1.0f;
		return FVector(0.0f, Direction * PairIndex * InitialPhysicalContentSpacing, 0.0f);
	}

	const UGridItemSubsystem* ResolveReceptacleItemRegistry(const AGridReceptacleActor* Actor)
	{
		const UWorld* World = IsValid(Actor) ? Actor->GetWorld() : nullptr;
		UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
		return GameInstance ? GameInstance->GetSubsystem<UGridItemSubsystem>() : nullptr;
	}

	UGridItemDefinitionAsset* ResolveItemDefinition(
		UGridPartyInventoryComponent* PartyInventoryComponent, UGridItemDefinitionAsset* DirectDefinition, FName ItemDefinitionId)
	{
		if (DirectDefinition)
		{
			return DirectDefinition;
		}

		if (PartyInventoryComponent && !ItemDefinitionId.IsNone())
		{
			return PartyInventoryComponent->FindItemDefinition(ItemDefinitionId);
		}

		return nullptr;
	}

	AGridLevelRuntimeActor* FindRuntimeActor(UWorld* World)
	{
		if (!World)
		{
			return nullptr;
		}

		for (TActorIterator<AGridLevelRuntimeActor> It(World); It; ++It)
		{
			return *It;
		}

		return nullptr;
	}

	const TCHAR* GetRejectReasonName(EGridReceptacleRejectReason Reason)
	{
		switch (Reason)
		{
			case EGridReceptacleRejectReason::None:
				return TEXT("none");
			case EGridReceptacleRejectReason::InvalidItem:
				return TEXT("invalid item");
			case EGridReceptacleRejectReason::Full:
				return TEXT("receptacle full");
			case EGridReceptacleRejectReason::NoMatchingAcceptanceRule:
				return TEXT("rejected because no rule matched");
			case EGridReceptacleRejectReason::InsertionDisabled:
				return TEXT("insertion disabled");
			default:
				return TEXT("unknown");
		}
	}
}

AGridReceptacleActor::AGridReceptacleActor()
{
	PrimaryActorTick.bCanEverTick = false;

	if (MeshComponent)
	{
		MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		MeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
		MeshComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		MeshComponent->SetGenerateOverlapEvents(false);
	}

	ItemSocketRoot = CreateDefaultSubobject<USceneComponent>(TEXT("ItemSocketRoot"));
	ItemSocketRoot->SetupAttachment(RootComponent);

	ItemAttachPoint = CreateDefaultSubobject<USceneComponent>(TEXT("ItemAttachPoint"));
	ItemAttachPoint->SetupAttachment(ItemSocketRoot);

	ContainedItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ContainedItemMesh"));
	ContainedItemMesh->SetupAttachment(ItemSocketRoot);
	ContainedItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ContainedItemMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	ContainedItemMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	ContainedItemMesh->SetGenerateOverlapEvents(false);
	ContainedItemMesh->SetVisibility(false, true);
}

void AGridReceptacleActor::BeginPlay()
{
	Super::BeginPlay();

	if (!bInitialItemsInitialized)
	{
		InitializeInitialContainedItems();
	}

	UpdateContainedItemInteractionCollision();
}

void AGridReceptacleActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ForceClearRuntimeContents(false);
	Super::EndPlay(EndPlayReason);
}

void AGridReceptacleActor::InitializeRuntimeWorldObject(
	const FGridRuntimeWorldObjectData& ObjectData, UStaticMesh* Mesh, const FTransform& WorldTransform)
{
	AGridRuntimeObjectActor::InitializeRuntimeWorldObject(ObjectData, Mesh, WorldTransform);

	const FGridObjectBehaviorParams EffectiveBehavior = ResolveEffectiveBehavior(ObjectData);
	const FGridReceptacleBehaviorParams& Params = EffectiveBehavior.Receptacle;

	bCanRemoveItem = true;
	bCanInsertItems = true;

	bAcceptAnyItem = Params.bAcceptAnyItem;
	AcceptedItemDefinitionIds.Reset();
	const UGridItemSubsystem* AcceptanceRegistry = ResolveReceptacleItemRegistry(this);
	for (const FGridReceptacleAcceptedItemConfig& AcceptedItem : Params.AcceptedItems)
	{
		FName EffectiveId = NAME_None;
		if (FGridReceptacleAcceptedCanonicalReadModel::TryResolveItemId(
			AcceptanceRegistry, AcceptedItem, EffectiveId))
		{
			AcceptedItemDefinitionIds.AddUnique(EffectiveId);
		}
		else if (!AcceptedItem.CanonicalItemId.IsNone())
		{
			// Never turn an invalid persisted canonical rule into a
			// permissive legacy rule or an accept-any fallback.
			UE_LOG(LogGridReceptacle, Warning,
				TEXT("Receptacle acceptance rule rejected: canonical ItemId=%s ObjectId=%s."),
				*AcceptedItem.CanonicalItemId.ToString(), *ObjectId.ToString());
		}
	}

	MaxContainedItems = Params.MaxContainedItems;
	bSimulatePhysicsWhenPlaced = Params.bSimulatePhysicsWhenPlaced;
	VisualPlacementMode = Params.VisualPlacementMode;
	PhysicalPlacementSurfaceOffset = Params.PhysicalPlacementSurfaceOffset;
	PhysicalPlacementInitialRotationOffset = Params.PhysicalPlacementInitialRotationOffset;

	if (MeshComponent && GetEffectiveVisualPlacementMode() == EGridReceptacleVisualPlacementMode::PhysicalAtHit)
	{
		MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		MeshComponent->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Block);
		MeshComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	}

	InitialContainedItems.Reset();
	ContainedItems.Reset();
	RemovedInitialItemDefinitionIds.Reset();
	bInitialItemsInitialized = false;

	const UGridItemSubsystem* ItemRegistry = ResolveReceptacleItemRegistry(this);
	for (const FGridReceptacleInitialItemConfig& ConfigItem : Params.InitialContent)
	{
		FGridInitialReceptacleItem InitialItem;
		if (!FGridReceptacleInitialCanonicalReadModel::TryResolve(
			ItemRegistry, ConfigItem, InitialItem))
		{
			// A known canonical identity may not be downgraded to legacy
			// data when the registry or its physical N-level is invalid.
			UE_LOG(LogGridReceptacle, Warning,
				TEXT("Receptacle initial item rejected: invalid physical identity for %s."),
				*ConfigItem.CanonicalItemId.ToString());
			continue;
		}
		InitialContainedItems.Add(InitialItem);
	}

	UE_LOG(LogGridReceptacle, Verbose, TEXT("GridReceptacle Init InitialContent Receptacle=%s ObjectId=%s ConfigCount=%d RuntimeInitialCount=%d"), *GetName(),
		*ObjectId.ToString(), Params.InitialContent.Num(), InitialContainedItems.Num());

	if (ContainedItemMesh)
	{
		ContainedItemMesh->SetStaticMesh(nullptr);
		ContainedItemMesh->SetVisibility(false, true);
		ContainedItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (!bInitialItemsInitialized)
	{
		InitializeInitialContainedItems();
	}
}

bool AGridReceptacleActor::HasItem() const
{
	return ContainedItems.Num() > 0;
}

bool AGridReceptacleActor::HasAnyItem() const
{
	return HasItem();
}

int32 AGridReceptacleActor::GetContainedItemCount() const
{
	return ContainedItems.Num();
}

bool AGridReceptacleActor::IsFull() const
{
	return MaxContainedItems > 0 && ContainedItems.Num() >= MaxContainedItems;
}

bool AGridReceptacleActor::IsEmpty() const
{
	return ContainedItems.Num() <= 0;
}

bool AGridReceptacleActor::IsValidContainedItemIndex(int32 ItemIndex) const
{
	return ContainedItems.IsValidIndex(ItemIndex);
}

FName AGridReceptacleActor::GetContainedItemDefinitionId(int32 ItemIndex) const
{
	if (!ContainedItems.IsValidIndex(ItemIndex))
	{
		return NAME_None;
	}

	return ContainedItems[ItemIndex].ItemDefinitionId;
}

AGridItemActor* AGridReceptacleActor::GetContainedItemActor(int32 ItemIndex) const
{
	if (!ContainedItems.IsValidIndex(ItemIndex))
	{
		return nullptr;
	}

	return ContainedItems[ItemIndex].ItemActor.Get();
}

bool AGridReceptacleActor::ContainsItemDefinition(FName ItemDefinitionId) const
{
	if (ItemDefinitionId.IsNone())
	{
		return false;
	}

	return ContainedItems.ContainsByPredicate(
		[ItemDefinitionId](const FGridContainedReceptacleItem& Item)
		{
			return Item.ItemDefinitionId == ItemDefinitionId;
		});
}

bool AGridReceptacleActor::ContainsItemTag(FName ItemTag) const
{
	if (ItemTag.IsNone())
	{
		return false;
	}

	const UGridItemSubsystem* Registry = ResolveReceptacleItemRegistry(this);
	return ContainedItems.ContainsByPredicate(
		[Registry, ItemTag](const FGridContainedReceptacleItem& Item)
		{
			if (Registry && Registry->HasCanonicalItem(Item.ItemDefinitionId))
			{
				FGridResolvedItem Resolved;
				return Registry->ResolveItem(Item.ItemDefinitionId, Item.ItemLevel, Resolved) &&
					Resolved.Tags.Contains(ItemTag);
			}
			// Only an identity outside ItemSystem may use its historic tags.
			return Item.ItemDefinition && Item.ItemDefinition->ItemTags.Contains(ItemTag);
		});
}

bool AGridReceptacleActor::ContainsItemType(EGridItemType ItemType) const
{
	const UGridItemSubsystem* Registry = ResolveReceptacleItemRegistry(this);
	return ContainedItems.ContainsByPredicate(
		[Registry, ItemType](const FGridContainedReceptacleItem& Item)
		{
			if (Registry && Registry->HasCanonicalItem(Item.ItemDefinitionId))
			{
				FGridResolvedItem Resolved;
				if (!Registry->ResolveItem(Item.ItemDefinitionId, Item.ItemLevel, Resolved))
				{
					return false; // A known invalid N0/N21 cannot borrow a legacy type.
				}
				if (Resolved.bIsArmament)
				{
					const EGridItemType CanonicalType =
						Resolved.Weapon.Kind == EGridCanonicalArmamentKind::Shield
							? EGridItemType::Shield : EGridItemType::Weapon;
					return ItemType == CanonicalType;
				}
				// Unlike ARM, every registered non-ARM item is physically N0.
				// Keep the physical invariant explicit before consulting item type.
				if (Item.ItemLevel != 0)
				{
					return false;
				}
				// Non-ARM still has no authored gameplay EGridItemType in ItemSystem.
				// Preserve existing puzzle semantics only with a matching old ID;
				// pointerless non-ARM needs a separate explicit category migration.
				return IsValid(Item.ItemDefinition) &&
					Item.ItemDefinition->ItemDefinitionId == Item.ItemDefinitionId &&
					Item.ItemDefinition->ItemType == ItemType;
			}
			// Genuine out-of-registry items keep their historical condition.
			return IsValid(Item.ItemDefinition) && Item.ItemDefinition->ItemType == ItemType;
		});
}

float AGridReceptacleActor::GetContainedTotalWeight() const
{
	const UWorld* World = GetWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	const UGridItemSubsystem* Registry = GameInstance
		? GameInstance->GetSubsystem<UGridItemSubsystem>() : nullptr;
	float TotalWeight = 0.0f;
	for (const FGridContainedReceptacleItem& Item : ContainedItems)
	{
		FGridItemInstance Instance;
		Instance.RuntimeObjectId = Item.RuntimeObjectId;
		Instance.ItemDefinitionId = Item.ItemDefinitionId;
		Instance.ItemLevel = Item.ItemLevel;
		Instance.Quantity = FMath::Max(1, Item.Quantity);
		Instance.Weight = Item.Weight; // Used only while the ItemId remains outside ItemSystem.
		TotalWeight += FGridItemWeightReadModel::GetTotalWeight(Registry, Instance);
	}
	return TotalWeight;
}

bool AGridReceptacleActor::CanAcceptItem(FName ItemDefinitionId) const
{
	if (ItemDefinitionId.IsNone())
	{
		return false;
	}
	if (!bCanInsertItems)
	{
		return false;
	}
	if (IsFull())
	{
		return false;
	}
	if (bAcceptAnyItem)
	{
		return true;
	}
	return AcceptedItemDefinitionIds.Contains(ItemDefinitionId);
}

bool AGridReceptacleActor::SetContainedItemLightsEnabled(int32 ItemIndex, bool bEnabled)
{
	if (!ContainedItems.IsValidIndex(ItemIndex))
	{
		return false;
	}

	FGridContainedReceptacleItem& Item = ContainedItems[ItemIndex];
	Item.bLightsEnabled = bEnabled;
	if (Item.ItemActor)
	{
		Item.ItemActor->SetItemLightsEnabled(bEnabled);
	}
	return true;
}

bool AGridReceptacleActor::CanAcceptItemInstance(const FGridItemInstance& Item) const
{
	FGridReceptacleAcceptanceResult Result;
	return EvaluateItemAcceptance(Item, Result);
}

bool AGridReceptacleActor::EvaluateItemAcceptance(const FGridItemInstance& Item, FGridReceptacleAcceptanceResult& OutResult, bool bLogDiagnostics) const
{
	OutResult = FGridReceptacleAcceptanceResult();

	auto LogDiagnostic = [this, &Item, bLogDiagnostics](const TCHAR* Outcome, const FString& Decision)
	{
		if (!bLogDiagnostics)
		{
			return;
		}

		const FString AcceptedIds = FString::JoinBy(AcceptedItemDefinitionIds, TEXT(","),
			[](FName ItemDefinitionId)
			{
				return ItemDefinitionId.ToString();
			});

		UE_LOG(LogGridReceptacle, VeryVerbose,
			TEXT("GridReceptacle Diagnostic Evaluate Outcome=%s Receptacle=%s ObjectId=%s ActorClass=%s "
				 "AcceptAny=%s AcceptedItemDefinitionIds=[%s] Count=%d Max=%d "
				 "ItemDefinitionId=%s RuntimeObjectId=%s %s"),
			Outcome, *GetName(), *ObjectId.ToString(), *GetClass()->GetPathName(), bAcceptAnyItem ? TEXT("true") : TEXT("false"), *AcceptedIds,
			ContainedItems.Num(), MaxContainedItems, *Item.ItemDefinitionId.ToString(), *Item.RuntimeObjectId.ToString(), *Decision);
	};

	auto Reject = [&OutResult, &LogDiagnostic, bLogDiagnostics](EGridReceptacleRejectReason Reason)
	{
		OutResult.bAccepted = false;
		OutResult.RejectReason = Reason;
		if (bLogDiagnostics)
		{
			LogDiagnostic(TEXT("Rejected"), FString::Printf(TEXT("RejectReason=%s"), GetRejectReasonName(Reason)));
		}
		return false;
	};

	auto Accept = [&OutResult, &LogDiagnostic, bLogDiagnostics](FName MatchedRule, const FString& MatchedValue)
	{
		OutResult.bAccepted = true;
		OutResult.RejectReason = EGridReceptacleRejectReason::None;
		OutResult.MatchedRule = MatchedRule;
		if (bLogDiagnostics)
		{
			LogDiagnostic(TEXT("Accepted"), FString::Printf(TEXT("MatchedRule=\"%s\" MatchedValue=%s"), *MatchedRule.ToString(), *MatchedValue));
		}
		return true;
	};

	if (!Item.IsValid())
	{
		return Reject(EGridReceptacleRejectReason::InvalidItem);
	}
	if (!bCanInsertItems)
	{
		return Reject(EGridReceptacleRejectReason::InsertionDisabled);
	}
	if (IsFull())
	{
		return Reject(EGridReceptacleRejectReason::Full);
	}
	// Preserve authored rule precedence: an unlisted item still fails
	// NoMatchingAcceptanceRule (even if its physical level is invalid).
	// Only a *matching* rule needs the stricter world-instance preflight.
	const bool bListedItem = AcceptedItemDefinitionIds.Contains(Item.ItemDefinitionId);
	if (!bAcceptAnyItem && !bListedItem)
	{
		return Reject(EGridReceptacleRejectReason::NoMatchingAcceptanceRule);
	}
	// Match the physical-instance check performed by AddContainedItem.
	// Otherwise an ARM at N0 or a non-ARM at N1 appears insertable in
	// cursor/UI preflight even though the real insertion fails closed.
	const UGridItemSubsystem* Registry = ResolveReceptacleItemRegistry(this);
	if (Registry && Registry->HasCanonicalItem(Item.ItemDefinitionId) &&
		!FGridReceptacleItemMetadataReadModel::Resolve(
			Registry, nullptr, Item.ItemDefinitionId, Item.ItemLevel, false).bResolved)
	{
		return Reject(EGridReceptacleRejectReason::InvalidItem);
	}
	if (bAcceptAnyItem)
	{
		return Accept(TEXT("accepted by accept any"), TEXT("true"));
	}
	return Accept(TEXT("accepted item definition"), Item.ItemDefinitionId.ToString());
}

bool AGridReceptacleActor::CanAcceptCursorItemFromParty(const AGrimrockPartyPawn* PartyPawn) const
{
	FGridItemInstance CursorItem;
	return PartyPawn && PartyPawn->GetCursorItem(CursorItem) && CanAcceptItemInstance(CursorItem);
}

bool AGridReceptacleActor::TryInsertItem(FName ItemDefinitionId, UGridItemDefinitionAsset* ItemDefinition, AGrimrockPartyPawn* PartyPawn)
{
	ItemDefinitionId = FGridReceptacleContainedIdentityReadModel::Resolve(
		ResolveReceptacleItemRegistry(this), ItemDefinition, ItemDefinitionId);

	if (IsFull())
	{
		UE_LOG(LogGridReceptacle, Warning, TEXT("Receptacle insert refused: ObjectId=%s Item=%s Reason=full Count=%d Max=%d"), *ObjectId.ToString(),
			*ItemDefinitionId.ToString(), ContainedItems.Num(), MaxContainedItems);
		return false;
	}

	if (!CanAcceptItem(ItemDefinitionId))
	{
		UE_LOG(LogGridReceptacle, Warning, TEXT("Receptacle insert refused: ObjectId=%s Item=%s Reason=%s"), *ObjectId.ToString(), *ItemDefinitionId.ToString(),
			*GetItemAcceptanceFailureReason(ItemDefinitionId));
		return false;
	}

	UGridPartyInventoryComponent* PartyInventoryComponent = PartyPawn ? PartyPawn->PartyInventoryComponent : nullptr;
	ItemDefinition = ResolveItemDefinition(PartyInventoryComponent, ItemDefinition, ItemDefinitionId);

	const int32 NewIndex = AddContainedItem(ItemDefinitionId, ItemDefinition, nullptr, false, 1);

	if (NewIndex == INDEX_NONE)
	{
		return false;
	}

	ExecuteInsertionLinks();

	UE_LOG(LogGridReceptacle, Verbose, TEXT("Receptacle accepted item %s ObjectId=%s Count=%d"), *ItemDefinitionId.ToString(), *ObjectId.ToString(),
		ContainedItems.Num());

	return true;
}

bool AGridReceptacleActor::TryInsertItemInstanceFromCursor(const FGridItemInstance& CursorItem, FGridItemInstance& OutAcceptedItem)
{
	OutAcceptedItem = FGridItemInstance();

	FGridReceptacleAcceptanceResult AcceptanceResult;
	if (!EvaluateItemAcceptance(CursorItem, AcceptanceResult))
	{
		UE_LOG(LogGridReceptacle, Warning, TEXT("Receptacle cursor insert refused: ObjectId=%s Item=%s RuntimeId=%s Reason=%s"), *ObjectId.ToString(),
			*CursorItem.ItemDefinitionId.ToString(), *CursorItem.RuntimeObjectId.ToString(), GetRejectReasonName(AcceptanceResult.RejectReason));
		return false;
	}

	const UGridItemSubsystem* Registry = ResolveReceptacleItemRegistry(this);
	const bool bCanonical = Registry && Registry->HasCanonicalItem(CursorItem.ItemDefinitionId);
	UGridItemDefinitionAsset* ItemDefinition = nullptr;
	if (!bCanonical)
	{
		if (const AGrimrockPartyPawn* PartyPawn = Cast<AGrimrockPartyPawn>(UGameplayStatics::GetPlayerPawn(this, 0)))
		{
			ItemDefinition = ResolveItemDefinition(PartyPawn->PartyInventoryComponent, nullptr, CursorItem.ItemDefinitionId);
		}
		if (!ItemDefinition)
		{
			if (const AGridLevelRuntimeActor* RuntimeActor = FindRuntimeActor(GetWorld()))
			{
				ItemDefinition = RuntimeActor->ResolveRuntimeItemDefinition(CursorItem.ItemDefinitionId);
			}
		}
	}
	if (bCanonical && !FGridReceptacleItemMetadataReadModel::Resolve(
		Registry, ItemDefinition, CursorItem.ItemDefinitionId, CursorItem.ItemLevel,
		false).bResolved)
	{
		return false; // Cursor item already exists: never re-inherit N0.
	}
	const int32 NewIndex = AddContainedItem(CursorItem.ItemDefinitionId, ItemDefinition, nullptr, false, CursorItem.Quantity, CursorItem.RuntimeObjectId, CursorItem.ItemLevel);

	if (!ContainedItems.IsValidIndex(NewIndex))
	{
		return false;
	}

	FGridContainedReceptacleItem& AcceptedReceptacleItem = ContainedItems[NewIndex];
	if (!bCanonical)
	{
		AcceptedReceptacleItem.Weight = CursorItem.Weight;
	}
	// SaveGame 27 still owns possible custom instance text. Preserve it until
	// 06.4, but present the canonical name to the player for registered IDs.
	if (!CursorItem.DisplayName.IsEmpty())
	{
		AcceptedReceptacleItem.DisplayName = CursorItem.DisplayName;
	}
	AcceptedReceptacleItem.bLightsEnabled = CursorItem.bLightsEnabled;
	AcceptedReceptacleItem.ReadableContentAsset = CursorItem.ReadableContentAsset;
	AcceptedReceptacleItem.ReadableContentId = CursorItem.ReadableContentId;
	AcceptedReceptacleItem.ReadTitleOverride = CursorItem.ReadTitleOverride;
	AcceptedReceptacleItem.ReadTextOverride = CursorItem.ReadTextOverride;
	if (AcceptedReceptacleItem.ItemActor)
	{
		AcceptedReceptacleItem.ItemActor->SetItemLightsEnabled(AcceptedReceptacleItem.bLightsEnabled);
		AcceptedReceptacleItem.ItemActor->InitializeReadableContent(AcceptedReceptacleItem.ReadableContentAsset, AcceptedReceptacleItem.ReadableContentId,
			AcceptedReceptacleItem.ReadTitleOverride, AcceptedReceptacleItem.ReadTextOverride);
	}

	OutAcceptedItem = CursorItem;
	OutAcceptedItem.ItemLevel = AcceptedReceptacleItem.ItemLevel;
	if (bCanonical)
	{
		OutAcceptedItem.Weight = AcceptedReceptacleItem.Weight;
		if (OutAcceptedItem.DisplayName.IsEmpty())
		{
			OutAcceptedItem.DisplayName = AcceptedReceptacleItem.DisplayName;
		}
	}
	OutAcceptedItem.OwnerType = EGridItemOwnerType::Receptacle;
	OutAcceptedItem.OwnerGuid = ObjectId;
	OutAcceptedItem.OwnerCharacterIndex = INDEX_NONE;
	OutAcceptedItem.EquipmentSlot = EGridEquipmentSlot::None;

	const FGridReceptacleItemMetadataReadView InsertedMetadata =
		FGridReceptacleItemMetadataReadModel::Resolve(Registry, ItemDefinition,
			CursorItem.ItemDefinitionId, AcceptedReceptacleItem.ItemLevel, false);
	const FText InsertedItemName = bCanonical && InsertedMetadata.bResolved
		? InsertedMetadata.DisplayName
		: (!CursorItem.DisplayName.IsEmpty()
			? CursorItem.DisplayName : FText::FromName(CursorItem.ItemDefinitionId));
	const FText InsertedMessage = FText::Format(
		NSLOCTEXT("GridEventFeed", "ReceptacleItemInserted", "{0} est placé dans le réceptacle."),
		InsertedItemName);
	const FName InsertedSourceId = GridEventFeedPublisher::SourceIdFromGuid(ObjectId);
	const bool bGroupStarted = GridEventFeedPublisher::BeginDungeonEventGroup(
		this,
		FName(TEXT("Dungeon.Receptacle.ItemInserted")),
		InsertedMessage,
		EGridHudEventSemantic::Interaction,
		EGridEventVisibility::Always,
		true,
		InsertedSourceId);
	if (!bGroupStarted)
	{
		GridEventFeedPublisher::PublishDungeonEvent(
			this,
			FName(TEXT("Dungeon.Receptacle.ItemInserted")),
			InsertedMessage,
			EGridHudEventSemantic::Interaction,
			EGridEventVisibility::Always,
			true,
			InsertedSourceId);
	}

	ExecuteInsertionLinks();
	if (bGroupStarted)
	{
		GridEventFeedPublisher::EndDungeonEventGroup(this);
	}

	UE_LOG(LogGridReceptacle, Verbose, TEXT("Receptacle accepted cursor item %s ObjectId=%s RuntimeId=%s Count=%d"), *CursorItem.ItemDefinitionId.ToString(),
		*ObjectId.ToString(), *CursorItem.RuntimeObjectId.ToString(), ContainedItems.Num());

	return true;
}

bool AGridReceptacleActor::TryTakeFirstItem(AGrimrockPartyPawn* PartyPawn, FName& OutRemovedItemDefinitionId)
{
	return TryTakeItemAtIndex(0, PartyPawn, OutRemovedItemDefinitionId);
}

bool AGridReceptacleActor::TryTakeItemAtIndex(int32 ItemIndex, AGrimrockPartyPawn* PartyPawn, FName& OutRemovedItemDefinitionId)
{
	OutRemovedItemDefinitionId = NAME_None;

	if (!PartyPawn || !ContainedItems.IsValidIndex(ItemIndex))
	{
		return false;
	}
	if (!IsItemRemovalAllowed())
	{
		UE_LOG(LogGridReceptacle, Warning, TEXT("GridReceptacle RemoveRejected ObjectId=%s ItemIndex=%d CanRemove=%s"), *ObjectId.ToString(), ItemIndex,
			bCanRemoveItem ? TEXT("true") : TEXT("false"));
		return false;
	}

	AGridLevelRuntimeActor* RuntimeActor = GridInteractionUtils::ResolveRuntimeActor(PartyPawn, this);
	if (!RuntimeActor || !RuntimeActor->CanPartyInteractWithEdgeObject(CellX, CellY, Edge, PartyPawn))
	{
		return false;
	}

	const FGridContainedReceptacleItem& Item = ContainedItems[ItemIndex];

	if (Item.ItemDefinitionId.IsNone())
	{
		return false;
	}

	FGridItemInstance ItemInstance;
	ItemInstance.RuntimeObjectId = Item.RuntimeObjectId.IsValid() ? Item.RuntimeObjectId : FGuid::NewGuid();
	ItemInstance.ItemDefinitionId = Item.ItemDefinitionId;
	ItemInstance.ItemLevel = Item.ItemLevel;
	ItemInstance.Quantity = FMath::Max(1, Item.Quantity);
	ItemInstance.Weight = Item.Weight;
	ItemInstance.DisplayName = Item.DisplayName;
	ItemInstance.bLightsEnabled = Item.bLightsEnabled;
	ItemInstance.ReadableContentAsset = Item.ReadableContentAsset;
	ItemInstance.ReadableContentId = Item.ReadableContentId;
	ItemInstance.ReadTitleOverride = Item.ReadTitleOverride;
	ItemInstance.ReadTextOverride = Item.ReadTextOverride;
	ItemInstance.OwnerType = EGridItemOwnerType::CharacterInventory;
	ItemInstance.OwnerCharacterIndex = PartyPawn->PartyInventoryComponent ? PartyPawn->PartyInventoryComponent->GetSelectedCharacterIndex() : INDEX_NONE;

	const UGridItemSubsystem* Registry = ResolveReceptacleItemRegistry(this);
	const bool bCanonical = Registry && Registry->HasCanonicalItem(Item.ItemDefinitionId);
	const FGridReceptacleItemMetadataReadView TakenMetadata = bCanonical
		? FGridReceptacleItemMetadataReadModel::Resolve(
			Registry, Item.ItemDefinition, Item.ItemDefinitionId, Item.ItemLevel, false)
		: FGridReceptacleItemMetadataReadView();
	if (bCanonical)
	{
		if (!TakenMetadata.bResolved)
		{
			return false; // A saved ARM N0 is invalid, not a new N1 item.
		}
		ItemInstance.Weight = TakenMetadata.UnitWeightKg;
		if (ItemInstance.DisplayName.IsEmpty())
		{
			ItemInstance.DisplayName = TakenMetadata.DisplayName;
		}
	}
	else if (Item.ItemDefinition)
	{
		ItemInstance.Weight = Item.ItemDefinition->Weight;
		ItemInstance.DisplayName = Item.ItemDefinition->DisplayName;
	}
	if (IsValid(Item.ItemActor.Get()))
	{
		ItemInstance.bLightsEnabled = Item.ItemActor->AreItemLightsEnabled();
		ItemInstance.ReadableContentAsset = Item.ItemActor->ReadableContentAsset;
		ItemInstance.ReadableContentId = Item.ItemActor->ReadableContentId;
		ItemInstance.ReadTitleOverride = Item.ItemActor->ReadTitleOverride;
		ItemInstance.ReadTextOverride = Item.ItemActor->ReadTextOverride;
	}

	if (!PartyPawn->AddItemInstanceToSelectedCharacterInventory(ItemInstance))
	{
		UE_LOG(LogGridReceptacle, Warning, TEXT("Receptacle take failed: ObjectId=%s Item=%s Reason=party inventory rejected"), *ObjectId.ToString(),
			*Item.ItemDefinitionId.ToString());
		return false;
	}

	FGridContainedReceptacleItem RemovedItem;
	if (!RemoveContainedItemAtIndex(ItemIndex, RemovedItem))
	{
		return false;
	}
	OutRemovedItemDefinitionId = RemovedItem.ItemDefinitionId;

	const FText RemovedItemName = bCanonical && TakenMetadata.bResolved
		? TakenMetadata.DisplayName
		: (!RemovedItem.DisplayName.IsEmpty()
			? RemovedItem.DisplayName : FText::FromName(RemovedItem.ItemDefinitionId));
	const FText RemovedMessage = FText::Format(
		NSLOCTEXT("GridEventFeed", "ReceptacleItemRemoved", "{0} est retiré du réceptacle."),
		RemovedItemName);
	const FName RemovedSourceId = GridEventFeedPublisher::SourceIdFromGuid(ObjectId);
	const bool bGroupStarted = GridEventFeedPublisher::BeginDungeonEventGroup(
		this,
		FName(TEXT("Dungeon.Receptacle.ItemRemoved")),
		RemovedMessage,
		EGridHudEventSemantic::Interaction,
		EGridEventVisibility::Always,
		true,
		RemovedSourceId);
	if (!bGroupStarted)
	{
		GridEventFeedPublisher::PublishDungeonEvent(
			this,
			FName(TEXT("Dungeon.Receptacle.ItemRemoved")),
			RemovedMessage,
			EGridHudEventSemantic::Interaction,
			EGridEventVisibility::Always,
			true,
			RemovedSourceId);
	}

	ExecuteRemovalLinks();
	if (bGroupStarted)
	{
		GridEventFeedPublisher::EndDungeonEventGroup(this);
	}

	UE_LOG(LogGridReceptacle, Verbose, TEXT("Receptacle returned item %s ObjectId=%s Count=%d"), *RemovedItem.ItemDefinitionId.ToString(), *ObjectId.ToString(),
		ContainedItems.Num());

	return true;
}

bool AGridReceptacleActor::ConsumeItemAtIndex(int32 ItemIndex)
{
	if (!ContainedItems.IsValidIndex(ItemIndex))
	{
		UE_LOG(LogGridReceptacle, Warning, TEXT("GridReceptacle ConsumeFailed ObjectId=%s ItemIndex=%d Reason=InvalidIndex Count=%d"), *ObjectId.ToString(),
			ItemIndex, ContainedItems.Num());
		return false;
	}

	const FName ItemDefinitionId = ContainedItems[ItemIndex].ItemDefinitionId;
	const FGuid RuntimeObjectId = ContainedItems[ItemIndex].RuntimeObjectId;
	FGridContainedReceptacleItem ConsumedItem;
	if (!RemoveContainedItemAtIndex(ItemIndex, ConsumedItem))
	{
		UE_LOG(LogGridReceptacle, Warning, TEXT("GridReceptacle ConsumeFailed ObjectId=%s Item=%s RuntimeId=%s Reason=RemoveFailed"), *ObjectId.ToString(),
			*ItemDefinitionId.ToString(), *RuntimeObjectId.ToString());
		return false;
	}

	if (AGridLevelRuntimeActor* RuntimeActor = FindRuntimeActor(GetWorld()))
	{
		RuntimeActor->ExecuteLinksFromRuntimeObject(ObjectId, EGridObjectEvent::ItemChanged);
	}
	UE_LOG(LogGridReceptacle, Verbose, TEXT("GridReceptacle ItemConsumed ObjectId=%s Item=%s RuntimeId=%s Remaining=%d"), *ObjectId.ToString(),
		*ItemDefinitionId.ToString(), *RuntimeObjectId.ToString(), ContainedItems.Num());
	return true;
}

bool AGridReceptacleActor::ConsumeAllItems()
{
	bool bConsumedAny = false;
	while (ContainedItems.Num() > 0)
	{
		if (!ConsumeItemAtIndex(ContainedItems.Num() - 1))
		{
			return false;
		}
		bConsumedAny = true;
	}
	return bConsumedAny;
}

void AGridReceptacleActor::SetCanRemoveItem(bool bNewCanRemoveItem)
{
	const bool bPreviousCanRemoveItem = bCanRemoveItem;
	bCanRemoveItem = bNewCanRemoveItem;
	UE_LOG(LogGridReceptacle, Verbose, TEXT("GridReceptacle RemovalChanged ObjectId=%s Previous=%s New=%s"), *ObjectId.ToString(),
		bPreviousCanRemoveItem ? TEXT("true") : TEXT("false"), bCanRemoveItem ? TEXT("true") : TEXT("false"));
}

void AGridReceptacleActor::SetCanInsertItems(bool bNewCanInsertItems)
{
	const bool bPreviousCanInsertItems = bCanInsertItems;
	bCanInsertItems = bNewCanInsertItems;
	UE_LOG(LogGridReceptacle, Verbose, TEXT("GridReceptacle InsertionChanged ObjectId=%s Previous=%s New=%s"), *ObjectId.ToString(),
		bPreviousCanInsertItems ? TEXT("true") : TEXT("false"), bCanInsertItems ? TEXT("true") : TEXT("false"));
}

bool AGridReceptacleActor::TryInteractWithParty(AGrimrockPartyPawn* PartyPawn)
{
	if (!PartyPawn)
	{
		return false;
	}

	AGridLevelRuntimeActor* RuntimeActor = GridInteractionUtils::ResolveRuntimeActor(PartyPawn, this);
	if (!RuntimeActor || !RuntimeActor->CanPartyInteractWithEdgeObject(CellX, CellY, Edge, PartyPawn))
	{
		return false;
	}

	if (PartyPawn->PartyInventoryComponent && PartyPawn->PartyInventoryComponent->HasCursorItem())
	{
		const FGridItemInstance& CursorItem = PartyPawn->PartyInventoryComponent->GetCursorItem();
		UE_LOG(LogGridReceptacle, Verbose, TEXT("GridReceptacle Interaction Route=\"fallback legacy cursor -> receptacle\" ObjectId=%s Item=%s RuntimeId=%s"),
			*ObjectId.ToString(), *CursorItem.ItemDefinitionId.ToString(), *CursorItem.RuntimeObjectId.ToString());
		return PartyPawn->TryPlaceCursorItemInReceptacle(this);
	}

	if (GetEffectiveVisualPlacementMode() != EGridReceptacleVisualPlacementMode::PhysicalAtHit && HasItem())
	{
		if (!IsItemRemovalAllowed())
		{
			UE_LOG(LogGridReceptacle, Warning, TEXT("GridReceptacle Interaction RemoveRejected ObjectId=%s CanRemove=%s"), *ObjectId.ToString(),
				bCanRemoveItem ? TEXT("true") : TEXT("false"));
			return false;
		}
		UGridPartyInventoryComponent* Inventory = PartyPawn->PartyInventoryComponent;
		const int32 CharacterIndex = Inventory ? Inventory->GetSelectedCharacterIndex() : INDEX_NONE;
		UE_LOG(LogGridReceptacle, Verbose,
			TEXT("GridReceptacle Interaction Route=\"service transfer receptacle -> inventory\" ObjectId=%s ItemIndex=0 Character=%d"), *ObjectId.ToString(),
			CharacterIndex);

		const FGridItemTransferResult TransferResult = UGridItemTransferService::TransferReceptacleItemToInventory(this, 0, Inventory, CharacterIndex);
		if (!TransferResult.bSuccess)
		{
			UE_LOG(LogGridReceptacle, Warning,
				TEXT("GridReceptacle Interaction ServiceFailed Route=\"receptacle -> inventory\" ObjectId=%s Result=%s Message=%s"), *ObjectId.ToString(),
				*UEnum::GetValueAsString(TransferResult.Result), *TransferResult.Message.ToString());
		}
		return TransferResult.bSuccess;
	}

	UE_LOG(LogGridReceptacle, Verbose, TEXT("Receptacle interact ignored: ObjectId=%s no CursorItem and no contained item."), *ObjectId.ToString());
	return false;
}

bool AGridReceptacleActor::TryPlaceCursorItemFromHit(AGrimrockPartyPawn* PartyPawn, const FHitResult& HitResult)
{
	if (GetEffectiveVisualPlacementMode() == EGridReceptacleVisualPlacementMode::PhysicalAtHit)
	{
		PendingPlacementHitResult = HitResult;
	}
	const bool bPlaced = PartyPawn && PartyPawn->TryPlaceCursorItemInReceptacle(this);
	PendingPlacementHitResult.Reset();
	return bPlaced;
}

void AGridReceptacleActor::CaptureRuntimeReceptacleState(FGridRuntimeReceptacleState& OutState) const
{
	OutState.ObjectId = ObjectId;
	OutState.bCanRemoveItem = bCanRemoveItem;
	OutState.bCanInsertItems = bCanInsertItems;
	OutState.ContainedItems.Reset();

	const UGridItemSubsystem* CanonicalRegistry = ResolveReceptacleItemRegistry(this);
	for (const FGridContainedReceptacleItem& Item : ContainedItems)
	{
		FName ResolvedItemId = Item.ItemDefinitionId;
		if (IsValid(Item.ItemActor.Get()))
		{
			const FName ObservedId =
				FGridReceptacleContainedIdentityReadModel::ResolveActor(
					Item.ItemActor.Get(), ResolvedItemId, Item.ItemLevel);
			if (CanonicalRegistry && CanonicalRegistry->HasCanonicalItem(ResolvedItemId) &&
				ObservedId != ResolvedItemId)
			{
				UE_LOG(LogGridReceptacle, Warning,
					TEXT("GridReceptacle Capture rejected conflicting actor Item=%s Actor=%s"),
					*ResolvedItemId.ToString(), *GetNameSafe(Item.ItemActor.Get()));
				continue;
			}
			if (ResolvedItemId.IsNone())
			{
				ResolvedItemId = ObservedId;
			}
		}
		if (ResolvedItemId.IsNone() && Item.bWasInitialItem)
		{
			ResolvedItemId = FGridReceptacleContainedIdentityReadModel::Resolve(
				CanonicalRegistry, Item.ItemDefinition, Item.ItemDefinitionId);
		}
		if (!ResolvedItemId.IsNone() && CanonicalRegistry &&
			CanonicalRegistry->HasCanonicalItem(ResolvedItemId) &&
			!FGridReceptacleItemMetadataReadModel::Resolve(
				CanonicalRegistry, nullptr, ResolvedItemId, Item.ItemLevel, false).bResolved)
		{
			UE_LOG(LogGridReceptacle, Warning,
					TEXT("GridReceptacle Capture rejected invalid canonical item %s level %d"),
					*ResolvedItemId.ToString(), Item.ItemLevel);
			continue;
		}
		if (ResolvedItemId.IsNone())
		{
			UE_LOG(LogGridReceptacle, Warning,
				TEXT("GridReceptacle Capture skipped contained item: ReceptacleId=%s RuntimeId=%s Actor=%s no ItemDefinitionId resolved."),
				*ObjectId.ToString(), *Item.RuntimeObjectId.ToString(), *GetNameSafe(Item.ItemActor.Get()));
			continue;
		}
		FGridRuntimeItemState ItemState;
		ItemState.ObjectId = Item.RuntimeObjectId.IsValid() ? Item.RuntimeObjectId : FGuid::NewGuid();
		ItemState.ItemDefinitionId = ResolvedItemId;
		ItemState.ItemLevel = Item.ItemLevel;
		ItemState.Quantity = FMath::Max(1, Item.Quantity);
		ItemState.bIsContainedInReceptacle = true;
		ItemState.ReceptacleObjectId = ObjectId;
		ItemState.bLightsEnabled = true;
		ItemState.ReadableContentAsset = Item.ReadableContentAsset;
		ItemState.ReadableContentId = Item.ReadableContentId;
		ItemState.ReadTitleOverride = Item.ReadTitleOverride;
		ItemState.ReadTextOverride = Item.ReadTextOverride;
		if (IsValid(Item.ItemActor.Get()))
		{
			ItemState.Transform = Item.ItemActor->GetActorTransform();
			ItemState.bLightsEnabled = Item.ItemActor->AreItemLightsEnabled();
			ItemState.ReadableContentAsset = Item.ItemActor->ReadableContentAsset;
			ItemState.ReadableContentId = Item.ItemActor->ReadableContentId;
			ItemState.ReadTitleOverride = Item.ItemActor->ReadTitleOverride;
			ItemState.ReadTextOverride = Item.ItemActor->ReadTextOverride;
		}
		else
		{
			ItemState.Transform = ItemAttachPoint ? ItemAttachPoint->GetComponentTransform() : GetActorTransform();
		}
		OutState.ContainedItems.Add(ItemState);
	}
}

int32 AGridReceptacleActor::ForceClearRuntimeContents(bool bMarkInitialItemsRemoved)
{
	const int32 RemovedCount = ContainedItems.Num();

	if (bMarkInitialItemsRemoved)
	{
		for (const FGridContainedReceptacleItem& Item : ContainedItems)
		{
			if (Item.bWasInitialItem && !Item.ItemDefinitionId.IsNone())
			{
				RemovedInitialItemDefinitionIds.Add(Item.ItemDefinitionId);
			}
		}
	}

	ClearAllContainedActors();
	ContainedItems.Reset();

	if (ContainedItemMesh)
	{
		ContainedItemMesh->SetStaticMesh(nullptr);
		ContainedItemMesh->SetVisibility(false, true);
		ContainedItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	UpdateContainedItemInteractionCollision();

	return RemovedCount;
}

bool AGridReceptacleActor::RestoreRuntimeContainedItem(const FGridRuntimeItemState& ItemState, AGridItemActor* ItemActor)
{
	if (ItemState.ItemDefinitionId.IsNone())
	{
		UE_LOG(LogGridReceptacle, Warning,
			TEXT("GridReceptacle Restore skipped contained item: ReceptacleId=%s RuntimeId=%s no ItemDefinitionId resolved."),
			*ObjectId.ToString(), *ItemState.ObjectId.ToString());
		return false;
	}
	const UGridItemSubsystem* Registry = ResolveReceptacleItemRegistry(this);
	if (Registry && Registry->HasCanonicalItem(ItemState.ItemDefinitionId) &&
		!FGridReceptacleItemMetadataReadModel::Resolve(
			Registry, nullptr, ItemState.ItemDefinitionId, ItemState.ItemLevel,
			false).bResolved)
	{
		return false; // Saved fixed item: never re-inherit default N1.
	}
	const int32 ItemIndex = AddContainedItem(ItemState.ItemDefinitionId, nullptr, ItemActor, false,
		ItemState.Quantity, ItemState.ObjectId, ItemState.ItemLevel);

	if (ItemIndex == INDEX_NONE)
	{
		return false;
	}
	FGridContainedReceptacleItem& Item = ContainedItems[ItemIndex];
	Item.RuntimeObjectId = ItemState.ObjectId.IsValid() ? ItemState.ObjectId : FGuid::NewGuid();
	Item.ItemLevel = ItemState.ItemLevel;
	Item.bLightsEnabled = ItemState.bLightsEnabled;
	Item.ReadableContentAsset = ItemState.ReadableContentAsset;
	Item.ReadableContentId = ItemState.ReadableContentId;
	Item.ReadTitleOverride = ItemState.ReadTitleOverride;
	Item.ReadTextOverride = ItemState.ReadTextOverride;
	if (IsValid(Item.ItemActor.Get()))
	{
		Item.ItemActor->SetRuntimeObjectId(Item.RuntimeObjectId);
		Item.ItemActor->SetActorTransform(ItemState.Transform, false, nullptr, ETeleportType::TeleportPhysics);
		Item.ItemActor->SetItemLightsEnabled(ItemState.bLightsEnabled);
		Item.ItemActor->InitializeReadableContent(
			ItemState.ReadableContentAsset, ItemState.ReadableContentId, ItemState.ReadTitleOverride, ItemState.ReadTextOverride);
	}
	return true;
}

bool AGridReceptacleActor::CanInteract_Implementation(APawn* InstigatorPawn, UPrimitiveComponent* HitComponent) const
{
	if (!InstigatorPawn || !HitComponent)
	{
		return false;
	}

	const AGrimrockPartyPawn* PartyPawn = GridInteractionUtils::ResolvePartyPawn(InstigatorPawn);
	AGridLevelRuntimeActor* RuntimeActor = GridInteractionUtils::ResolveRuntimeActor(InstigatorPawn, this);
	if (!PartyPawn || !RuntimeActor || !RuntimeActor->CanPartyInteractWithEdgeObject(CellX, CellY, Edge, PartyPawn))
	{
		return false;
	}

	if (PartyPawn->PartyInventoryComponent && PartyPawn->PartyInventoryComponent->HasCursorItem())
	{
		return CanAcceptCursorItemFromParty(PartyPawn);
	}

	if (IsContainedItemHitComponent(HitComponent))
	{
		return HasItem() && IsItemRemovalAllowed();
	}
	if (HitComponent != MeshComponent)
	{
		return false;
	}

	return GetEffectiveVisualPlacementMode() != EGridReceptacleVisualPlacementMode::PhysicalAtHit && HasItem() && IsItemRemovalAllowed();
}

void AGridReceptacleActor::Interact_Implementation(APawn* InstigatorPawn, UPrimitiveComponent* HitComponent)
{
	if (!CanInteract_Implementation(InstigatorPawn, HitComponent))
	{
		return;
	}
	AGrimrockPartyPawn* PartyPawn = GridInteractionUtils::ResolvePartyPawn(InstigatorPawn);
	if (!PartyPawn)
	{
		return;
	}
	const bool bHasCursorItem = PartyPawn->PartyInventoryComponent && PartyPawn->PartyInventoryComponent->HasCursorItem();
	if (!bHasCursorItem && IsContainedItemHitComponent(HitComponent))
	{
		const int32 ItemIndex = FindContainedItemIndexForComponent(HitComponent);
		if (ItemIndex != INDEX_NONE)
		{
			FName RemovedItemId = NAME_None;
			TryTakeItemAtIndex(ItemIndex, PartyPawn, RemovedItemId);
			return;
		}
	}
	TryInteractWithParty(PartyPawn);
}

void AGridReceptacleActor::InteractWithHit_Implementation(APawn* InstigatorPawn, UPrimitiveComponent* HitComponent, const FHitResult& HitResult)
{
	if (GetEffectiveVisualPlacementMode() == EGridReceptacleVisualPlacementMode::PhysicalAtHit &&
		(HitComponent == MeshComponent || IsContainedItemHitComponent(HitComponent)))
	{
		PendingPlacementHitResult = HitResult;
	}
	Interact_Implementation(InstigatorPawn, HitComponent);
	PendingPlacementHitResult.Reset();
}

EGridInteractionCursor AGridReceptacleActor::GetInteractionCursor_Implementation(UPrimitiveComponent* HitComponent) const
{
	if (IsContainedItemHitComponent(HitComponent) && HasItem() && IsItemRemovalAllowed())
	{
		return EGridInteractionCursor::Take;
	}
	if (HitComponent == MeshComponent && !IsFull())
	{
		return EGridInteractionCursor::Use;
	}
	return EGridInteractionCursor::Default;
}

FText AGridReceptacleActor::GetInteractionText_Implementation(UPrimitiveComponent* HitComponent) const
{
	if (IsContainedItemHitComponent(HitComponent) && HasItem() && IsItemRemovalAllowed())
	{
		return FUILocalizationText::Get(TEXT("Interaction.Item.Take"));
	}
	if (HitComponent == MeshComponent && !IsFull())
	{
		return FUILocalizationText::Get(TEXT("Interaction.Receptacle.PlaceItem"));
	}
	return FText::GetEmpty();
}

int32 AGridReceptacleActor::AddContainedItem(
	FName ItemDefinitionId, UGridItemDefinitionAsset* ItemDefinition, AGridItemActor* ItemActor, bool bWasInitialItem, int32 Quantity,
	FGuid RuntimeObjectId, int32 ItemLevel, bool bPersistedCanonicalInitialItem)
{
	AGridLevelRuntimeActor* RuntimeActor = FindRuntimeActor(GetWorld());
	const UGridItemSubsystem* CanonicalRegistry = ResolveReceptacleItemRegistry(this);
	// The persisted/cursor/restore routes use the same complete canonical
	// materialization. Only unconverted initial fixtures retain their old
	// authoring source until the corresponding test assets are retired.
	const bool bCanonicalInitialSource = bWasInitialItem && bPersistedCanonicalInitialItem;
	const bool bKnownExplicit = CanonicalRegistry && CanonicalRegistry->HasCanonicalItem(ItemDefinitionId);
	const bool bCanonicalWorld = bKnownExplicit && (!bWasInitialItem || bCanonicalInitialSource);
	if (bCanonicalInitialSource && !bKnownExplicit)
	{
		return INDEX_NONE;
	}
	if (!ItemDefinition && RuntimeActor && !bCanonicalWorld)
	{
		ItemDefinition = RuntimeActor->ResolveRuntimeItemDefinition(ItemDefinitionId);
	}
	ItemDefinitionId = FGridReceptacleContainedIdentityReadModel::Resolve(
		CanonicalRegistry, ItemDefinition, ItemDefinitionId);
	if (ItemDefinitionId.IsNone())
	{
		return INDEX_NONE;
	}
	if (IsFull())
	{
		return INDEX_NONE;
	}
	// Fresh placement resolves the authored default once. Persisted cursor
	// and restore paths separately reject invalid registered N0 before here.
	const FGridReceptacleItemMetadataReadView Metadata =
		FGridReceptacleItemMetadataReadModel::Resolve(
			ResolveReceptacleItemRegistry(this), ItemDefinition,
			ItemDefinitionId, ItemLevel, !bCanonicalInitialSource);
	if (!Metadata.bResolved)
	{
		UE_LOG(LogGridReceptacle, Warning,
			TEXT("Receptacle rejected unresolved ItemLevel=%d Item=%s"),
			ItemLevel, *ItemDefinitionId.ToString());
		return INDEX_NONE;
	}
	const int32 ResolvedLevel = Metadata.FixedItemLevel;
	FGridResolvedItem CanonicalProjection;
	if (bCanonicalWorld)
	{
		if (!CanonicalRegistry->ResolveItem(ItemDefinitionId, ResolvedLevel, CanonicalProjection) ||
			!CanonicalProjection.IsValid() || (!CanonicalProjection.bIsArmament && ResolvedLevel != 0))
		{
			return INDEX_NONE;
		}
		ItemDefinition = nullptr;
	}
	if (CanonicalRegistry && CanonicalRegistry->HasCanonicalItem(ItemDefinitionId))
	{
		// Old authored data must not create an actor with a different ItemId.
		if (IsValid(ItemDefinition) && ItemDefinition->ItemDefinitionId != ItemDefinitionId)
		{
			ItemDefinition = nullptr;
		}
		if (IsValid(ItemActor) &&
			FGridReceptacleContainedIdentityReadModel::ResolveActor(
				ItemActor, ItemDefinitionId, ResolvedLevel) != ItemDefinitionId)
		{
			return INDEX_NONE;
		}
	}
	FGridContainedReceptacleItem NewItem;
	NewItem.RuntimeObjectId = RuntimeObjectId.IsValid() ? RuntimeObjectId : FGuid::NewGuid();
	NewItem.ItemDefinitionId = ItemDefinitionId;
	NewItem.ItemDefinition = ItemDefinition;
	NewItem.ItemActor = ItemActor;
	NewItem.bWasInitialItem = bWasInitialItem;
	NewItem.Quantity = FMath::Max(1, Quantity);
	NewItem.ItemLevel = ResolvedLevel;
	if (IsValid(ItemActor))
	{
		NewItem.ReadableContentAsset = ItemActor->ReadableContentAsset;
		NewItem.ReadableContentId = ItemActor->ReadableContentId;
		NewItem.ReadTitleOverride = ItemActor->ReadTitleOverride;
		NewItem.ReadTextOverride = ItemActor->ReadTextOverride;
	}
	NewItem.Weight = Metadata.UnitWeightKg;
	NewItem.DisplayName = Metadata.DisplayName;
	if (bCanonicalWorld)
	{
		NewItem.bLightsEnabled = CanonicalProjection.LightEmitter.bDefaultEnabled;
	}
	else if (ItemDefinition)
	{
		NewItem.bLightsEnabled = ItemDefinition->IsLightEnabledByDefault();
	}

	// Adopt a previously materialized actor only when its exact canonical
	// identity and level agree; otherwise rebuild its complete presentation.
	if (bCanonicalWorld && IsValid(ItemActor) &&
		(!ItemActor->CanonicalWorldPresentation.IsValid() ||
		 ItemActor->CanonicalWorldItemLevel != ResolvedLevel ||
		 ItemActor->GetItemDefinitionAsset()))
	{
		FGridItemInstance Physical;
		Physical.RuntimeObjectId = NewItem.RuntimeObjectId;
		Physical.ItemDefinitionId = ItemDefinitionId;
		Physical.ItemLevel = ResolvedLevel;
		Physical.Quantity = NewItem.Quantity;
		if (!ItemActor->InitializeFromCanonicalItem(Physical, CanonicalRegistry))
		{
			return INDEX_NONE;
		}
	}
	const int32 NewIndex = ContainedItems.Add(NewItem);

	const EGridReceptacleVisualPlacementMode PlacementMode = GetEffectiveVisualPlacementMode();
	if (!IsValid(ItemActor) && (RuntimeActor || bCanonicalWorld))
	{
		if (RuntimeActor)
		{
			ItemActor = RuntimeActor->SpawnItemActorForDefinition(
				bCanonicalWorld ? nullptr : ItemDefinition, ItemDefinitionId, this, ItemAttachPoint);
		}
		else if (UWorld* World = GetWorld())
		{
			// Standalone receptacles (including runtime tests) have a valid
			// world and registry but no central level actor. Construct the
			// SAME canonical item, never an incomplete ID-only fallback.
			const FTransform SpawnTransform = ItemAttachPoint ? ItemAttachPoint->GetComponentTransform() : GetActorTransform();
			FActorSpawnParameters Params;
			Params.Owner = this;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			ItemActor = World->SpawnActor<AGridItemActor>(AGridItemActor::StaticClass(),
				SpawnTransform.GetLocation(), SpawnTransform.GetRotation().Rotator(), Params);
		}
		if (IsValid(ItemActor))
		{
			if (bCanonicalWorld)
			{
				FGridItemInstance CanonicalInstance;
				CanonicalInstance.RuntimeObjectId = NewItem.RuntimeObjectId;
				CanonicalInstance.ItemDefinitionId = NewItem.ItemDefinitionId;
				CanonicalInstance.ItemLevel = NewItem.ItemLevel;
				CanonicalInstance.Quantity = NewItem.Quantity;
				if (!ItemActor->InitializeFromCanonicalItem(CanonicalInstance, CanonicalRegistry))
				{
					ItemActor->Destroy();
					ContainedItems.RemoveAt(NewIndex);
					return INDEX_NONE; // Fail closed, never downgrade presentation.
				}
			}
			else if (ItemDefinition)
			{
				ItemActor->InitializeFromItemDefinition(ItemDefinition, NewItem.RuntimeObjectId);
			}
			else
			{
				ItemActor->InitializeFromItemDefinitionId(ItemDefinitionId, NewItem.RuntimeObjectId);
			}
			ContainedItems[NewIndex].ItemActor = ItemActor;
		}
	}
	if (!IsValid(ItemActor) && bCanonicalWorld)
	{
		ContainedItems.RemoveAt(NewIndex);
		return INDEX_NONE;
	}
	if (!IsValid(ItemActor))
	{
		UWorld* World = GetWorld();
		if (World)
		{
			const FTransform SpawnTransform = ItemAttachPoint ? ItemAttachPoint->GetComponentTransform() : GetActorTransform();
			AGridItemActor* SpawnedItemActor = World->SpawnActorDeferred<AGridItemActor>(AGridItemActor::StaticClass(), SpawnTransform, this);
			if (SpawnedItemActor)
			{
				if (ItemDefinition)
				{
					SpawnedItemActor->InitializeFromItemDefinition(ItemDefinition, NewItem.RuntimeObjectId);
				}
				else
				{
					SpawnedItemActor->InitializeFromItemDefinitionId(ItemDefinitionId, NewItem.RuntimeObjectId);
				}
				UGameplayStatics::FinishSpawningActor(SpawnedItemActor, SpawnTransform);
				SpawnedItemActor->ConfigureAsAttachedItem();
				ContainedItems[NewIndex].ItemActor = SpawnedItemActor;
			}
		}
	}
	if (IsValid(ContainedItems[NewIndex].ItemActor.Get()))
	{
		FGridItemInstance PlacementItem;
		PlacementItem.RuntimeObjectId = ContainedItems[NewIndex].RuntimeObjectId;
		PlacementItem.ItemDefinitionId = ContainedItems[NewIndex].ItemDefinitionId;
		PlacementItem.ItemLevel = ContainedItems[NewIndex].ItemLevel;
		PlacementItem.Quantity = ContainedItems[NewIndex].Quantity;
		// Canonical insertion/restore is already fully projected; only
		// unconverted initial fixtures require the historical visual update.
		if (!bCanonicalWorld)
		{
			ContainedItems[NewIndex].ItemActor->ApplyCanonicalWorldVisual(PlacementItem);
		}
		ApplyVisualPlacement(ContainedItems[NewIndex].ItemActor.Get(), PlacementItem,
			PendingPlacementHitResult.IsSet() ? &PendingPlacementHitResult.GetValue() : nullptr, NewIndex, bWasInitialItem);
		if (PlacementMode != EGridReceptacleVisualPlacementMode::PhysicalAtHit)
		{
			ContainedItems[NewIndex].ItemActor->OnPlacedInWorld();
		}
		ContainedItems[NewIndex].ItemActor->SetItemLightsEnabled(ContainedItems[NewIndex].bLightsEnabled);
	}
	UpdateContainedItemInteractionCollision();
	return NewIndex;
}

bool AGridReceptacleActor::RemoveContainedItemAtIndex(int32 ItemIndex, FGridContainedReceptacleItem& OutRemovedItem)
{
	if (!ContainedItems.IsValidIndex(ItemIndex))
	{
		return false;
	}
	OutRemovedItem = ContainedItems[ItemIndex];
	if (OutRemovedItem.bWasInitialItem && !OutRemovedItem.ItemDefinitionId.IsNone())
	{
		RemovedInitialItemDefinitionIds.Add(OutRemovedItem.ItemDefinitionId);
	}
	ClearContainedActor(ContainedItems[ItemIndex]);
	ContainedItems.RemoveAt(ItemIndex);
	UpdateContainedItemInteractionCollision();
	return true;
}

void AGridReceptacleActor::ClearContainedActor(FGridContainedReceptacleItem& Item)
{
	if (IsValid(Item.ItemActor.Get()))
	{
		Item.ItemActor->OnRemovedFromWorld();
		Item.ItemActor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		Item.ItemActor->Destroy();
	}
	Item.ItemActor = nullptr;
}

void AGridReceptacleActor::ClearAllContainedActors()
{
	for (FGridContainedReceptacleItem& Item : ContainedItems)
	{
		ClearContainedActor(Item);
	}
}

EGridReceptacleVisualPlacementMode AGridReceptacleActor::GetEffectiveVisualPlacementMode() const
{
	return VisualPlacementMode;
}

bool AGridReceptacleActor::IsItemRemovalAllowed() const
{
	return bCanRemoveItem;
}

void AGridReceptacleActor::ApplyVisualPlacement(
	AGridItemActor* ItemActor, const FGridItemInstance& Item, const FHitResult* OptionalHit, int32 ItemIndex, bool bInitialPlacement)
{
	if (!IsValid(ItemActor) || !ItemAttachPoint)
	{
		return;
	}

	const EGridReceptacleVisualPlacementMode PlacementMode = GetEffectiveVisualPlacementMode();
	ItemActor->SetOwner(this);
	ItemActor->SetActorHiddenInGame(false);
	ItemActor->SetActorEnableCollision(true);

	if (PlacementMode == EGridReceptacleVisualPlacementMode::PhysicalAtHit)
	{
		ItemActor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		const bool bHasHit = OptionalHit && OptionalHit->bBlockingHit;
		const bool bUseInitialContentOffset = bInitialPlacement && !bHasHit;
		const FVector LocalOffset = bUseInitialContentOffset ? ComputeInitialPhysicalContentOffset(ItemIndex) : FVector::ZeroVector;
		const FVector PlacementLocation = bHasHit
			? OptionalHit->ImpactPoint + OptionalHit->ImpactNormal * PhysicalPlacementSurfaceOffset
			: ItemAttachPoint->GetComponentLocation() + ItemAttachPoint->GetComponentTransform().TransformVectorNoScale(LocalOffset);
		const FQuat PlacementRotation = ItemAttachPoint->GetComponentQuat() * PhysicalPlacementInitialRotationOffset.Quaternion();
		ItemActor->SetActorLocationAndRotation(PlacementLocation, PlacementRotation, false, nullptr, ETeleportType::TeleportPhysics);
		ItemActor->ConfigureAsWorldPickup();
		if (ItemActor->MeshComponent)
		{
			if (bSimulatePhysicsWhenPlaced)
			{
				ItemActor->MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
				ItemActor->MeshComponent->SetCollisionProfileName(TEXT("PhysicsActor"));
				ItemActor->MeshComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
				ItemActor->MeshComponent->SetEnableGravity(true);
				ItemActor->MeshComponent->SetSimulatePhysics(true);
				ItemActor->MeshComponent->WakeRigidBody();
			}
			else
			{
				ItemActor->MeshComponent->SetSimulatePhysics(false);
				ItemActor->MeshComponent->SetEnableGravity(false);
				ItemActor->MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
				ItemActor->MeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
				ItemActor->MeshComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
			}
		}
		if (bUseInitialContentOffset)
		{
			UE_LOG(LogGridReceptacle, Verbose,
				TEXT(
					"GridReceptacle PhysicalAtHit InitialOffset Place Receptacle=%s Item=%s ItemIndex=%d LocalOffset=%s Location=%s Simulating=%s Gravity=%s Collision=%d Hit=false"),
				*GetName(), *Item.ItemDefinitionId.ToString(), ItemIndex, *LocalOffset.ToCompactString(), *ItemActor->GetActorLocation().ToCompactString(),
				ItemActor->MeshComponent && ItemActor->MeshComponent->IsSimulatingPhysics() ? TEXT("true") : TEXT("false"),
				ItemActor->MeshComponent && ItemActor->MeshComponent->IsGravityEnabled() ? TEXT("true") : TEXT("false"),
				ItemActor->MeshComponent ? static_cast<int32>(ItemActor->MeshComponent->GetCollisionEnabled()) : -1);
		}
		UE_LOG(LogGridReceptacle, Verbose,
			TEXT("GridReceptacle PhysicalAtHit Place Receptacle=%s Item=%s Actor=%s Simulating=%s Gravity=%s Collision=%d Location=%s Hit=%s"), *GetName(),
			*Item.ItemDefinitionId.ToString(), *GetNameSafe(ItemActor),
			ItemActor->MeshComponent && ItemActor->MeshComponent->IsSimulatingPhysics() ? TEXT("true") : TEXT("false"),
			ItemActor->MeshComponent && ItemActor->MeshComponent->IsGravityEnabled() ? TEXT("true") : TEXT("false"),
			ItemActor->MeshComponent ? static_cast<int32>(ItemActor->MeshComponent->GetCollisionEnabled()) : -1,
			*ItemActor->GetActorLocation().ToCompactString(), bHasHit ? TEXT("true") : TEXT("false"));
		return;
	}

	ItemActor->AttachToComponent(ItemAttachPoint, FAttachmentTransformRules::SnapToTargetNotIncludingScale);

	const int32 AttachedItemIndex = FMath::Max(0,
		ContainedItems.IndexOfByPredicate(
			[&Item](const FGridContainedReceptacleItem& ContainedItem)
			{
				return ContainedItem.RuntimeObjectId == Item.RuntimeObjectId;
			}));
	const FVector LocalOffset(0.f, MultiItemVisualSpacing * AttachedItemIndex, 0.f);
	ItemActor->SetActorRelativeLocation(LocalOffset);
	ItemActor->SetActorRelativeRotation(FRotator::ZeroRotator);
	ItemActor->SetActorRelativeScale3D(FVector::OneVector);

	ItemActor->ConfigureAsAttachedItem();
}

void AGridReceptacleActor::UpdateContainedItemInteractionCollision()
{
	for (FGridContainedReceptacleItem& Item : ContainedItems)
	{
		if (IsValid(Item.ItemActor.Get()) && Item.ItemActor->MeshComponent)
		{
			const EGridReceptacleVisualPlacementMode PlacementMode = GetEffectiveVisualPlacementMode();
			if (PlacementMode == EGridReceptacleVisualPlacementMode::PhysicalAtHit)
			{
				Item.ItemActor->MeshComponent->SetCollisionEnabled(
					bSimulatePhysicsWhenPlaced ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::QueryOnly);
				if (bSimulatePhysicsWhenPlaced)
				{
					Item.ItemActor->MeshComponent->SetCollisionProfileName(TEXT("PhysicsActor"));
				}
				else
				{
					Item.ItemActor->MeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
				}
				Item.ItemActor->MeshComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
			}
			else
			{
				Item.ItemActor->MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
				Item.ItemActor->MeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
				Item.ItemActor->MeshComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
			}
		}
	}
}

bool AGridReceptacleActor::IsContainedItemHitComponent(UPrimitiveComponent* HitComponent) const
{
	return FindContainedItemIndexForComponent(HitComponent) != INDEX_NONE;
}

int32 AGridReceptacleActor::FindContainedItemIndexForComponent(UPrimitiveComponent* HitComponent) const
{
	if (!HitComponent)
	{
		return INDEX_NONE;
	}
	for (int32 Index = 0; Index < ContainedItems.Num(); ++Index)
	{
		const AGridItemActor* ItemActor = ContainedItems[Index].ItemActor.Get();
		if (!IsValid(ItemActor))
		{
			continue;
		}
		if (HitComponent == ItemActor->MeshComponent)
		{
			return Index;
		}
		if (HitComponent->GetOwner() == ItemActor)
		{
			return Index;
		}
	}
	return INDEX_NONE;
}

FString AGridReceptacleActor::GetItemAcceptanceFailureReason(FName ItemDefinitionId) const
{
	if (ItemDefinitionId.IsNone())
	{
		return TEXT("empty item id");
	}
	if (IsFull())
	{
		return TEXT("receptacle is full");
	}
	if (!bCanInsertItems)
	{
		return TEXT("receptacle insertion is disabled");
	}
	if (!bAcceptAnyItem && !AcceptedItemDefinitionIds.Contains(ItemDefinitionId))
	{
		return TEXT("item definition is not accepted");
	}
	return TEXT("unknown");
}

void AGridReceptacleActor::ExecuteInsertionLinks()
{
	if (AGridLevelRuntimeActor* RuntimeActor = FindRuntimeActor(GetWorld()))
	{
		RuntimeActor->ExecuteLinksFromRuntimeObject(ObjectId, EGridObjectEvent::ItemInserted);
		RuntimeActor->ExecuteLinksFromRuntimeObject(ObjectId, EGridObjectEvent::ItemChanged);
	}
}

void AGridReceptacleActor::ExecuteRemovalLinks()
{
	if (AGridLevelRuntimeActor* RuntimeActor = FindRuntimeActor(GetWorld()))
	{
		RuntimeActor->ExecuteLinksFromRuntimeObject(ObjectId, EGridObjectEvent::ItemRemoved);
		RuntimeActor->ExecuteLinksFromRuntimeObject(ObjectId, EGridObjectEvent::ItemChanged);
	}
}

void AGridReceptacleActor::InitializeInitialContainedItems()
{
	bInitialItemsInitialized = true;

	for (const FGridInitialReceptacleItem& InitialItem : InitialContainedItems)
	{
		const FName ItemDefinitionId = ResolveInitialItemDefinitionId(InitialItem);
		if (ItemDefinitionId.IsNone())
		{
			UE_LOG(LogGridReceptacle, Warning, TEXT("GridReceptacle InitialItem skipped: ObjectId=%s InitialItems=%d ContainedItems=%d InitialDefinitionId=%s"),
				*ObjectId.ToString(), InitialContainedItems.Num(), ContainedItems.Num(), *InitialItem.ItemDefinitionId.ToString());
			continue;
		}
		if (WasInitialItemRemoved(ItemDefinitionId))
		{
			continue;
		}
		AddContainedItem(ItemDefinitionId, nullptr, nullptr, true,
			InitialItem.Quantity, FGuid(), InitialItem.ItemLevel, true);
	}
}

FName AGridReceptacleActor::ResolveInitialItemDefinitionId(const FGridInitialReceptacleItem& InitialItem) const
{
	const UGridItemSubsystem* Registry = ResolveReceptacleItemRegistry(this);
	FGridResolvedItem Physical;
	if (!IsValid(Registry) || !Registry->IsConfigured() ||
		InitialItem.ItemDefinitionId.IsNone() ||
		InitialItem.Quantity < 1 ||
		!Registry->ResolveItem(InitialItem.ItemDefinitionId, InitialItem.ItemLevel, Physical) ||
		!Physical.IsValid() || Physical.ItemId != InitialItem.ItemDefinitionId ||
		Physical.ItemLevel != InitialItem.ItemLevel)
		return NAME_None;
	return InitialItem.ItemDefinitionId;
}

bool AGridReceptacleActor::WasInitialItemRemoved(FName ItemDefinitionId) const
{
	return RemovedInitialItemDefinitionIds.Contains(ItemDefinitionId);
}
