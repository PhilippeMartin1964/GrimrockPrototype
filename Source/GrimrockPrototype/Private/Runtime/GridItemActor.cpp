#include "Runtime/GridItemActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Runtime/GridInteractionUtils.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "GridItemMeshResolver.h"
#include "GridWorldPhysicsMassReadModel.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Engine/StaticMesh.h"
#endif
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridLightEmitterComponent.h"
#include "Runtime/GridItemVisualEffectsComponent.h"
#include "Runtime/GridReadableContentAsset.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/GridReceptacleActor.h"
#include "UI/UILocalizationText.h"

AGridItemActor::AGridItemActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(SceneRoot);

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	MeshComponent->SetupAttachment(SceneRoot);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	SparkleMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WorldSparkleMesh"));
	SparkleMeshComponent->SetupAttachment(MeshComponent);
	SparkleMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SparkleMeshComponent->SetSimulatePhysics(false);
	SparkleMeshComponent->SetEnableGravity(false);
	SparkleMeshComponent->SetCastShadow(false);
	SparkleMeshComponent->SetVisibility(false, true);

	LightEmitterComponent = CreateDefaultSubobject<UGridLightEmitterComponent>(TEXT("LightEmitter"));
	// ITEM-LIGHT03: world physics is simulated by MeshComponent, so the emitter
	// must share that transform instead of remaining behind on the actor root.
	LightEmitterComponent->SetupAttachment(MeshComponent);

	VisualEffectsComponent = CreateDefaultSubobject<UGridItemVisualEffectsComponent>(TEXT("VisualEffects"));
	VisualEffectsComponent->SetupAttachment(MeshComponent);
}

void AGridItemActor::OnPlacedInWorld()
{
	// Presentation visibility is transient and never overwrites the instance light state.
	if (VisualEffectsComponent)
	{
		VisualEffectsComponent->SetPresentationEnabled(true);
	}
}

void AGridItemActor::OnRemovedFromWorld()
{
	SetItemLightsEnabled(false);
	if (VisualEffectsComponent)
	{
		VisualEffectsComponent->SetPresentationEnabled(false);
	}
	SetWorldSparkleEnabled(false);
}

void AGridItemActor::SetItemLightsEnabled(bool bEnabled)
{
	if (LightEmitterComponent)
	{
		LightEmitterComponent->SetLightEnabled(bEnabled);
	}
	if (VisualEffectsComponent)
	{
		VisualEffectsComponent->SetItemLightState(bEnabled);
	}
}

bool AGridItemActor::InitializeFromItemInstance(
	const FGridItemInstance& Instance, const UGridItemSubsystem* Registry)
{
	// The entire resolve must complete before touching the actor.
	if (!Registry || !Instance.IsValid() ||
		!Registry->ContainsItem(Instance.ItemDefinitionId) ||
		!MeshComponent || !LightEmitterComponent || !VisualEffectsComponent)
	{
		return false;
	}
	FGridResolvedItem Projected;
	if (!Registry->ResolveItem(Instance.ItemDefinitionId, Instance.ItemLevel, Projected) ||
		!Projected.IsValid() || Projected.ItemId != Instance.ItemDefinitionId ||
		Projected.ItemLevel != Instance.ItemLevel)
	{
		return false; // Known invalid ARM N0 never silently becomes N1.
	}
	// All registered non-ARM physical instances are N0.
	if (!Projected.bIsArmament && Instance.ItemLevel != 0)
	{
		return false; // Leave any preexisting actor state completely unchanged.
	}

	ItemDefinitionId = Projected.ItemId;
	WorldItemLevel = Projected.ItemLevel;
	ItemTags = Projected.Tags;
	SetRuntimeObjectId(Instance.RuntimeObjectId);
	// This is not a second authority: a transient copy from the sole registry.
	ResolvedWorldPresentation = MoveTemp(Projected);

	SetPresentationMesh(ResolvedWorldPresentation.WorldMesh.LoadSynchronous());
	MeshComponent->SetRelativeTransform(ResolvedWorldPresentation.WorldVisualTransform);
	SetItemLightsEnabled(false);
	LightEmitterComponent->ApplyConfig(ResolvedWorldPresentation.LightEmitter);
	VisualEffectsComponent->ApplyConfig(ResolvedWorldPresentation.VisualEffects);
	// Reinitialization must not keep another pickup's active sparkle.
	SetWorldSparkleEnabled(false);
	if (MeshComponent->IsSimulatingPhysics())
	{
		RefreshWorldPickupMass();
	}
	return true;
}

bool AGridItemActor::ApplyResolvedWorldVisual(const FGridItemInstance& Instance)
{
	if (!MeshComponent || !Instance.IsValid() || Instance.ItemDefinitionId != GetItemDefinitionId())
	{
		return false;
	}
	const UWorld* World = GetWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	const UGridItemSubsystem* Registry = GameInstance
		? GameInstance->GetSubsystem<UGridItemSubsystem>() : nullptr;
	const FGridItemMeshView Visual =
		FGridItemMeshResolver::Resolve(Registry, Instance);
	if (!Visual.bRegisteredIdentity)
	{
		return false; // An unregistered identity cannot replace the current presentation.
	}
	WorldItemLevel = Instance.ItemLevel;
	// Explicitly clear rather than reusing an old DA_Weapon_* mesh if N0 is invalid.
	UStaticMesh* NewMesh = Visual.bResolved ? Visual.WorldMesh.LoadSynchronous() : nullptr;
	const FTransform NewTransform = Visual.bResolved ? Visual.WorldTransform : FTransform::Identity;
	if (MeshComponent->GetStaticMesh() != NewMesh || ResolvedWorldPresentation.WeaponFinish.FinishId != Visual.WeaponFinish.FinishId)
	{
		ResolvedWorldPresentation.WeaponFinish = Visual.WeaponFinish;
		SetPresentationMesh(NewMesh);
	}
	if (!MeshComponent->GetRelativeTransform().Equals(NewTransform))
	{
		MeshComponent->SetRelativeTransform(NewTransform);
	}
	RefreshWorldSparklePresentation();
	if (MeshComponent->IsSimulatingPhysics())
	{
		// Saved actors can already have a live Chaos body before visual restoration.
		RefreshWorldPickupMass();
	}
	return Visual.bResolved;
}

void AGridItemActor::InitializeFromItemDefinitionId(FName InItemDefinitionId, const FGuid& InRuntimeObjectId)
{
	if (InItemDefinitionId.IsNone())
	{
		return;
	}

	ClearWeaponFinish();
	ResolvedWorldPresentation = FGridResolvedItem();
	ItemDefinitionId = InItemDefinitionId;
	WorldItemLevel = 0;
	ItemTags.Reset();
	SetRuntimeObjectId(InRuntimeObjectId);
	if (!RuntimeObjectId.IsValid())
	{
		RuntimeObjectId = FGuid::NewGuid();
	}
	if (LightEmitterComponent)
	{
		LightEmitterComponent->ApplyConfig(FGridLightEmitterConfig());
		LightEmitterComponent->SetLightEnabled(false);
	}
	if (VisualEffectsComponent)
	{
		VisualEffectsComponent->SetItemLightState(false);
		VisualEffectsComponent->ApplyConfig(FGridItemVisualEffectsConfig());
	}
}

void AGridItemActor::InitializeReadableContent(
	UGridReadableContentAsset* InReadableContentAsset, FName InReadableContentId, const FText& InReadTitleOverride, const FText& InReadTextOverride)
{
	ReadableContentAsset = InReadableContentAsset;
	ReadableContentId = InReadableContentId;
	ReadTitleOverride = InReadTitleOverride;
	ReadTextOverride = InReadTextOverride;

	if (ReadableContentAsset && ReadableContentId.IsNone())
	{
		ReadableContentId = ReadableContentAsset->ReadableContentId;
	}
}

void AGridItemActor::SetPresentationMesh(UStaticMesh* Mesh)
{
	if (!MeshComponent)
	{
		return;
	}
	ClearWeaponFinish();
	MeshComponent->SetStaticMesh(Mesh);
	ApplyWeaponFinish();
}

void AGridItemActor::ClearWeaponFinish()
{
	for (int32 Slot = 0; MeshComponent && Slot < FinishMaterials.Num(); ++Slot)
	{
		if (FinishMaterials[Slot] && MeshComponent->GetMaterial(Slot) == FinishMaterials[Slot])
		{
			MeshComponent->SetMaterial(Slot, nullptr);
		}
	}
	FinishMaterials.Reset();
}

void AGridItemActor::ApplyWeaponFinish()
{
	const FGridWeaponFinish& Finish = ResolvedWorldPresentation.WeaponFinish;
	if (Finish.FinishId.IsNone() || (Finish.RoughnessScale == 1.0f && Finish.NormalScale == 1.0f && Finish.Tint == FLinearColor::White))
	{
		return;
	}

	static const FMaterialParameterInfo RoughnessMultiplier(TEXT("RoughnessMultiplier"));
	static const FMaterialParameterInfo RoughnessOverride(TEXT("RoughnessOverride"));
	static const FMaterialParameterInfo NormalStrength(TEXT("NormalStrength"));
	static const FMaterialParameterInfo BaseColorTint(TEXT("BaseColorTint"));
	FinishMaterials.SetNum(MeshComponent->GetNumMaterials());
	for (int32 Slot = 0; Slot < FinishMaterials.Num(); ++Slot)
	{
		// An explicit override owned by a Blueprint or another feature keeps its slot.
		if (MeshComponent->OverrideMaterials.IsValidIndex(Slot) && MeshComponent->OverrideMaterials[Slot])
		{
			continue;
		}
		const UMaterialInterface* Base = MeshComponent->GetMaterial(Slot);
		float RoughnessScale;
		float RoughnessAbsolute;
		float Normal;
		FLinearColor Tint;
		if (!Base || !Base->GetScalarParameterValue(RoughnessMultiplier, RoughnessScale) ||
			!Base->GetScalarParameterValue(RoughnessOverride, RoughnessAbsolute) || !Base->GetScalarParameterValue(NormalStrength, Normal) ||
			!Base->GetVectorParameterValue(BaseColorTint, Tint))
		{
			continue;
		}
		if (UMaterialInstanceDynamic* Material = MeshComponent->CreateDynamicMaterialInstance(Slot))
		{
			Material->SetScalarParameterValue(RoughnessMultiplier.Name, RoughnessScale * Finish.RoughnessScale);
			Material->SetScalarParameterValue(RoughnessOverride.Name, RoughnessAbsolute * Finish.RoughnessScale);
			Material->SetScalarParameterValue(NormalStrength.Name, Normal * Finish.NormalScale);
			Material->SetVectorParameterValue(BaseColorTint.Name, Tint * Finish.Tint);
			FinishMaterials[Slot] = Material;
		}
	}
}

bool AGridItemActor::AreItemLightsEnabled() const
{
	return LightEmitterComponent && LightEmitterComponent->IsLightEnabled();
}

void AGridItemActor::SetRuntimeObjectId(FGuid InRuntimeObjectId)
{
	if (InRuntimeObjectId.IsValid())
	{
		RuntimeObjectId = InRuntimeObjectId;
	}
}

FGuid AGridItemActor::GetRuntimeObjectId() const
{
	return RuntimeObjectId;
}

void AGridItemActor::RefreshWorldPickupMass()
{
	if (!MeshComponent) return;
	const UWorld* World = GetWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	const UGridItemSubsystem* Registry = GameInstance
		? GameInstance->GetSubsystem<UGridItemSubsystem>() : nullptr;
	FGridItemInstance MassInstance;
	MassInstance.RuntimeObjectId = RuntimeObjectId;
	MassInstance.ItemDefinitionId = GetItemDefinitionId();
	MassInstance.ItemLevel = WorldItemLevel;
	MassInstance.Quantity = 1;
	float MassKg = 0.0f;
	bool bOverrideMass = false;
	if (Registry)
	{
		bOverrideMass = FGridWorldPhysicsMassReadModel::TryResolve(
			Registry, MassInstance, MassKg);
	}
	else if (!GameInstance && ResolvedWorldPresentation.IsValid() &&
		ResolvedWorldPresentation.ItemId == MassInstance.ItemDefinitionId &&
		ResolvedWorldPresentation.ItemLevel == MassInstance.ItemLevel && MassInstance.IsValid())
	{
		// Explicit registry projection supplied at canonical initialization.
		bOverrideMass = FGridWorldPhysicsMassReadModel::TryResolveProjected(
			ResolvedWorldPresentation, MassKg);
	}
	MeshComponent->SetMassOverrideInKg(NAME_None, bOverrideMass ? MassKg : 0.0f, bOverrideMass);
}

void AGridItemActor::ConfigureAsWorldPickup()
{
	if (!MeshComponent)
	{
		return;
	}

	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	MeshComponent->SetCollisionProfileName(TEXT("PhysicsActor"));
	MeshComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	RefreshWorldPickupMass();

	MeshComponent->SetEnableGravity(true);
	MeshComponent->SetSimulatePhysics(true);
	MeshComponent->WakeRigidBody();
	SetWorldSparkleEnabled(true);
}

void AGridItemActor::ApplyWorldPhysicsInitialNudge()
{
	const UWorld* World = GetWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	const UGridItemSubsystem* Registry = GameInstance
		? GameInstance->GetSubsystem<UGridItemSubsystem>() : nullptr;
	FGridItemInstance Instance;
	Instance.RuntimeObjectId = RuntimeObjectId;
	Instance.ItemDefinitionId = GetItemDefinitionId();
	Instance.ItemLevel = WorldItemLevel;
	Instance.Quantity = 1;
	float AuthoredTilt = 0.0f;
	if (Registry)
	{
		FGridResolvedItem Item;
		if (!FGridWorldPhysicsMassReadModel::TryResolve(Registry, Instance, Item)) return;
		AuthoredTilt = Item.WorldPhysicsInitialTiltDegrees;
	}
	else if (!GameInstance && ResolvedWorldPresentation.IsValid() && Instance.IsValid() &&
		ResolvedWorldPresentation.ItemId == Instance.ItemDefinitionId &&
		ResolvedWorldPresentation.ItemLevel == Instance.ItemLevel)
	{
		AuthoredTilt = ResolvedWorldPresentation.WorldPhysicsInitialTiltDegrees;
	}
	if (!FMath::IsFinite(AuthoredTilt)) return;
	const float TiltDegrees = FMath::Clamp(AuthoredTilt, 0.0f, 45.0f);
	if (TiltDegrees <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const uint32 Hash = RuntimeObjectId.IsValid() ? GetTypeHash(RuntimeObjectId) : GetTypeHash(GetItemDefinitionId());
	const float AzimuthRadians = (static_cast<float>(Hash % 3600u) / 10.0f) * (PI / 180.0f);
	const FVector TiltAxis(FMath::Cos(AzimuthRadians), FMath::Sin(AzimuthRadians), 0.0f);
	const FQuat TiltRotation(TiltAxis, FMath::DegreesToRadians(TiltDegrees));
	AddActorWorldRotation(TiltRotation, false, nullptr, ETeleportType::TeleportPhysics);
}

void AGridItemActor::ConfigureAsAttachedItem()
{
	if (!MeshComponent)
	{
		return;
	}

	MeshComponent->SetSimulatePhysics(false);
	MeshComponent->SetEnableGravity(false);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetWorldSparkleEnabled(false);
}

void AGridItemActor::SetWorldSparkleEnabled(bool bEnabled)
{
	// Refresh validates the complete current profile before enabling an overlay.
	bWorldSparkleActive = bEnabled;
	RefreshWorldSparklePresentation();
}

float AGridItemActor::BuildWorldSparklePhase() const
{
	const uint32 Hash = RuntimeObjectId.IsValid() ? GetTypeHash(RuntimeObjectId) : GetTypeHash(GetItemDefinitionId());
	return static_cast<float>(Hash % 10000u) / 10000.0f;
}

void AGridItemActor::RefreshWorldSparklePresentation()
{
	if (!SparkleMeshComponent)
	{
		bWorldSparkleActive = false;
		WorldSparkleMaterialInstance = nullptr;
		return;
	}

	SparkleMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SparkleMeshComponent->SetSimulatePhysics(false);
	SparkleMeshComponent->SetEnableGravity(false);
	SparkleMeshComponent->SetCastShadow(false);
	SparkleMeshComponent->SetRelativeTransform(FTransform::Identity);

	UStaticMesh* SourceMesh = MeshComponent ? MeshComponent->GetStaticMesh() : nullptr;
	SparkleMeshComponent->SetStaticMesh(SourceMesh);

	const auto Disable = [this]()
	{
		bWorldSparkleActive = false;
		SparkleMeshComponent->SetVisibility(false, true);
		WorldSparkleMaterialInstance = nullptr;
	};
	if (!bWorldSparkleActive || !SourceMesh)
	{
		Disable();
		return;
	}

	const UWorld* World = GetWorld();
	const UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
	const UGridItemSubsystem* Registry = GI ? GI->GetSubsystem<UGridItemSubsystem>() : nullptr;
	FGridItemInstance Instance;
	Instance.RuntimeObjectId = RuntimeObjectId;
	Instance.ItemDefinitionId = ItemDefinitionId;
	Instance.ItemLevel = WorldItemLevel;
	Instance.Quantity = 1;
	FGridResolvedItem Profile;
	bool bResolved = false;
	if (Registry)
	{
		bResolved = FGridWorldPhysicsMassReadModel::TryResolve(Registry, Instance, Profile);
	}
	else if (!GI && Instance.IsValid() && ResolvedWorldPresentation.IsValid() &&
		ResolvedWorldPresentation.ItemId == Instance.ItemDefinitionId &&
		ResolvedWorldPresentation.ItemLevel == Instance.ItemLevel)
	{
		Profile = ResolvedWorldPresentation;
		bResolved = true;
	}
	if (!bResolved || !Profile.bEnableWorldSparkle || Profile.WorldSparkleMaterial.IsNull())
	{
		Disable();
		return;
	}
	UMaterialInterface* SparkleMaterial = Profile.WorldSparkleMaterial.LoadSynchronous();
	if (!SparkleMaterial)
	{
		bWorldSparkleActive = false;
		SparkleMeshComponent->SetVisibility(false, true);
		WorldSparkleMaterialInstance = nullptr;
		return;
	}

	WorldSparkleMaterialInstance = UMaterialInstanceDynamic::Create(SparkleMaterial, this);
	if (!WorldSparkleMaterialInstance)
	{
		bWorldSparkleActive = false;
		SparkleMeshComponent->SetVisibility(false, true);
		return;
	}

	const FLinearColor SparkleColor = Profile.WorldSparkleColor;
	const float SparkleIntensity = Profile.WorldSparkleIntensity;
	const float SparkleSpeed = Profile.WorldSparkleSpeed;
	const float SparkleVariation = Profile.WorldSparkleVariation;
	WorldSparkleMaterialInstance->SetVectorParameterValue(TEXT("SparkleColor"), SparkleColor);
	WorldSparkleMaterialInstance->SetScalarParameterValue(TEXT("SparkleIntensity"), FMath::Max(0.0f, SparkleIntensity));
	WorldSparkleMaterialInstance->SetScalarParameterValue(TEXT("SparkleSpeed"), FMath::Max(0.0f, SparkleSpeed));
	WorldSparkleMaterialInstance->SetScalarParameterValue(TEXT("SparkleVariation"), FMath::Clamp(SparkleVariation, 0.0f, 1.0f));
	WorldSparkleMaterialInstance->SetScalarParameterValue(TEXT("SparklePhase"), BuildWorldSparklePhase());

	const int32 MaterialCount = FMath::Max(1, SparkleMeshComponent->GetNumMaterials());
	for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
	{
		SparkleMeshComponent->SetMaterial(MaterialIndex, WorldSparkleMaterialInstance);
	}

	SparkleMeshComponent->SetVisibility(true, true);
}

FName AGridItemActor::GetItemDefinitionId() const
{
	// The materialized physical identity is never renamed by a definition.
	// Consumers validate membership and physical level separately.
	return ItemDefinitionId;
}

bool AGridItemActor::HasItemTag(FName Tag) const
{
	if (Tag.IsNone()) return false;
	const UWorld* World = GetWorld();
	const UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
	const UGridItemSubsystem* Registry = GI ? GI->GetSubsystem<UGridItemSubsystem>() : nullptr;
	FGridItemInstance Instance;
	Instance.RuntimeObjectId = RuntimeObjectId;
	Instance.ItemDefinitionId = ItemDefinitionId;
	Instance.ItemLevel = WorldItemLevel;
	Instance.Quantity = 1;
	if (Registry)
	{
		FGridResolvedItem Item;
		return FGridWorldPhysicsMassReadModel::TryResolve(Registry, Instance, Item) &&
			Item.Tags.Contains(Tag);
	}
	if (!GI && Instance.IsValid() && ResolvedWorldPresentation.IsValid() &&
		ResolvedWorldPresentation.ItemId == Instance.ItemDefinitionId &&
		ResolvedWorldPresentation.ItemLevel == Instance.ItemLevel)
	{
		return ResolvedWorldPresentation.Tags.Contains(Tag);
	}
#if WITH_DEV_AUTOMATION_TESTS
	if (!GI && GIsAutomationTesting) return ItemTags.Contains(Tag);
#endif
	return false;
}

void AGridItemActor::SetRuntimeCell(int32 InCellX, int32 InCellY)
{
	RuntimeCellX = InCellX;
	RuntimeCellY = InCellY;
}

FVector AGridItemActor::GetWorldPickupLocation() const
{
	return IsValid(MeshComponent) && MeshComponent->GetStaticMesh() ? MeshComponent->Bounds.Origin : GetActorLocation();
}

FVector AGridItemActor::GetClosestWorldPickupPoint(const FVector& WorldLocation) const
{
	if (!IsValid(MeshComponent) || !MeshComponent->GetStaticMesh())
		return GetActorLocation();
	const FVector Query(WorldLocation.X, WorldLocation.Y, MeshComponent->Bounds.Origin.Z);
	FVector Closest;
	const float Distance = MeshComponent->GetClosestPointOnCollision(Query, Closest);
	if (Distance == 0.f)
		return Query;
	// Editor previews / worlds without a query body still have valid mesh bounds.
	return Distance > 0.f ? Closest : MeshComponent->Bounds.GetBox().GetClosestPointTo(Query);
}

bool AGridItemActor::CanInteract_Implementation(APawn* InstigatorPawn, UPrimitiveComponent* HitComponent) const
{
	if (!InstigatorPawn || !HitComponent || HitComponent != MeshComponent || GetItemDefinitionId().IsNone())
	{
		return false;
	}
	if (const AGridReceptacleActor* OwnerReceptacle = Cast<AGridReceptacleActor>(GetOwner()))
	{
		return OwnerReceptacle->CanInteract_Implementation(const_cast<APawn*>(InstigatorPawn), MeshComponent);
	}

	const AGrimrockPartyPawn* PartyPawn = GridInteractionUtils::ResolvePartyPawn(InstigatorPawn);
	const AGridLevelRuntimeActor* RuntimeActor = GridInteractionUtils::ResolveRuntimeActor(InstigatorPawn, this);
	return PartyPawn && RuntimeActor && RuntimeActor->CanPartyPickupItemActor(this, PartyPawn);
}

void AGridItemActor::Interact_Implementation(APawn* InstigatorPawn, UPrimitiveComponent* HitComponent)
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

	if (AGridReceptacleActor* OwnerReceptacle = Cast<AGridReceptacleActor>(GetOwner()))
	{
		OwnerReceptacle->Interact_Implementation(PartyPawn, MeshComponent);
		return;
	}

	AGridLevelRuntimeActor* RuntimeActor = GridInteractionUtils::ResolveRuntimeActor(InstigatorPawn, this);
	if (RuntimeActor)
	{
		RuntimeActor->TryPickupItemActor(this, PartyPawn);
	}
}

void AGridItemActor::InteractWithHit_Implementation(APawn* InstigatorPawn, UPrimitiveComponent* HitComponent, const FHitResult& HitResult)
{
	(void)HitResult;
	Interact_Implementation(InstigatorPawn, HitComponent);
}

EGridInteractionCursor AGridItemActor::GetInteractionCursor_Implementation(UPrimitiveComponent* HitComponent) const
{
	if (HitComponent != MeshComponent || GetItemDefinitionId().IsNone())
	{
		return EGridInteractionCursor::Default;
	}
	if (const AGridReceptacleActor* OwnerReceptacle = Cast<AGridReceptacleActor>(GetOwner()))
	{
		return OwnerReceptacle->GetInteractionCursor_Implementation(MeshComponent);
	}
	return EGridInteractionCursor::Take;
}

FText AGridItemActor::GetInteractionText_Implementation(UPrimitiveComponent* HitComponent) const
{
	if (HitComponent != MeshComponent || GetItemDefinitionId().IsNone())
	{
		return FText::GetEmpty();
	}
	if (const AGridReceptacleActor* OwnerReceptacle = Cast<AGridReceptacleActor>(GetOwner()))
	{
		return OwnerReceptacle->GetInteractionText_Implementation(MeshComponent);
	}
	return FUILocalizationText::Get(TEXT("Interaction.Item.Take"));
}
