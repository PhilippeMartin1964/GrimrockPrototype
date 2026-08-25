#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Runtime/GridInteractableInterface.h"
#include "GridItemActor.generated.h"

class UStaticMeshComponent;
class UGridItemDefinitionAsset;
class UGridReadableContentAsset;

UCLASS(Blueprintable)
class GRIMROCKPROTOTYPE_API AGridItemActor : public AActor, public IGridInteractableInterface
{
	GENERATED_BODY()

public:
	AGridItemActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FName ArchetypeId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item")
	TObjectPtr<UGridItemDefinitionAsset> ItemDefinitionAsset = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item")
	FName ItemDefinitionId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	TArray<FName> ItemTags;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item")
	FGuid RuntimeObjectId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Reading")
	TObjectPtr<UGridReadableContentAsset> ReadableContentAsset = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Reading")
	FName ReadableContentId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Reading")
	FText ReadTitleOverride;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Reading")
	FText ReadTextOverride;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid")
	int32 RuntimeCellX = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid")
	int32 RuntimeCellY = INDEX_NONE;

	UFUNCTION(BlueprintCallable, Category = "Item")
	virtual void InitializeItem(FName InArchetypeId, const TArray<FName>& InItemTags, UStaticMesh* Mesh, UMaterialInterface* Material);

	UFUNCTION(BlueprintCallable, Category = "Item")
	void InitializeFromItemDefinition(UGridItemDefinitionAsset* InDefinition, const FGuid& InRuntimeObjectId);

	UFUNCTION(BlueprintCallable, Category = "Item")
	void InitializeFromItemDefinitionId(FName InItemDefinitionId, const FGuid& InRuntimeObjectId);

	UFUNCTION(BlueprintCallable, Category = "Reading")
	void InitializeReadableContent(
		UGridReadableContentAsset* InReadableContentAsset, FName InReadableContentId, const FText& InReadTitleOverride, const FText& InReadTextOverride);

	UFUNCTION(BlueprintCallable, Category = "Item")
	virtual void OnPlacedInWorld();

	UFUNCTION(BlueprintCallable, Category = "Item")
	virtual void OnRemovedFromWorld();

	UFUNCTION(BlueprintCallable, Category = "Item")
	void SetItemLightsEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "Item")
	bool AreItemLightsEnabled() const;

	UFUNCTION(BlueprintCallable, Category = "Item")
	void SetRuntimeObjectId(FGuid InRuntimeObjectId);

	UFUNCTION(BlueprintCallable, Category = "Item")
	FGuid GetRuntimeObjectId() const;

	UFUNCTION(BlueprintCallable, Category = "Item")
	void ConfigureAsWorldPickup();

	UFUNCTION(BlueprintCallable, Category = "Item")
	void ConfigureAsAttachedItem();

	UFUNCTION(BlueprintCallable, Category = "Item")
	FName GetItemArchetypeId() const;

	UFUNCTION(BlueprintCallable, Category = "Item")
	UGridItemDefinitionAsset* GetItemDefinitionAsset() const;

	UFUNCTION(BlueprintCallable, Category = "Item")
	FName GetItemDefinitionId() const;

	UFUNCTION(BlueprintCallable, Category = "Item")
	bool HasItemTag(FName Tag) const;

	UFUNCTION(BlueprintCallable, Category = "Item")
	void SetRuntimeCell(int32 InCellX, int32 InCellY);

	virtual bool CanInteract_Implementation(APawn* InstigatorPawn, UPrimitiveComponent* HitComponent) const override;
	virtual void Interact_Implementation(APawn* InstigatorPawn, UPrimitiveComponent* HitComponent) override;
	virtual void InteractWithHit_Implementation(APawn* InstigatorPawn, UPrimitiveComponent* HitComponent, const FHitResult& HitResult) override;
	virtual EGridInteractionCursor GetInteractionCursor_Implementation(UPrimitiveComponent* HitComponent) const override;
	virtual FText GetInteractionText_Implementation(UPrimitiveComponent* HitComponent) const override;
};
