#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/GridSkillsUiTypes.h"
#include "UI/RPGTalentPresentationAsset.h"
#include "GridTalentNodeWidget.generated.h"

class UBorder;
class UButton;
class UMenuAnchor;
class UTextBlock;
class UTexture2D;
class UGridTalentVariantFlyoutWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGridTalentNodeClickedSignature, FName, TalentNodeId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FGridTalentVariantClickedSignature,
	FName, TalentNodeId,
	FName, ChoiceId);

/** Reusable read-only presentation widget for one conceptual Talent node. */
UCLASS()
class GRIMROCKPROTOTYPE_API UGridTalentNodeWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|Node")
	FGridTalentNodeView NodeView;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|Node")
	FText ResolvedDisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|Node")
	FText ResolvedDescription;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|Node")
	bool bInitialized = false;

	UPROPERTY(BlueprintAssignable, Category = "RPG|Talents|Node|Events")
	FGridTalentNodeClickedSignature OnTalentNodeClicked;

	UPROPERTY(BlueprintAssignable, Category = "RPG|Talents|Node|Events")
	FGridTalentVariantClickedSignature OnTalentVariantClicked;

	UFUNCTION(BlueprintCallable, Category = "RPG|Talents|Node")
	bool InitializeTalentNode(
		const FGridTalentNodeView& InNodeView,
		const FRPGTalentBranchPresentationDefinition& InBranchPresentation);

	UFUNCTION(BlueprintCallable, Category = "RPG|Talents|Node")
	void ClearTalentNode();

	/** Presentation-only icon supplied by URPGClassVisualAsset for this canonical TalentNodeId. */
	void SetTalentIcon(UTexture2D* IconTexture);

	void SetVariantIcon(FName ChoiceId, UTexture2D* IconTexture);
	void ClearVariantIcons();

	UFUNCTION(BlueprintPure, Category = "RPG|Talents|Node")
	int32 GetVariantCount() const { return NodeView.bHasExclusiveVariants ? NodeView.Variants.Num() : 0; }

	UFUNCTION(BlueprintPure, Category = "RPG|Talents|Node")
	bool HasVariants() const { return NodeView.bHasExclusiveVariants; }

	UFUNCTION(BlueprintPure, Category = "RPG|Talents|Node")
	EGridTalentNodeState GetTalentNodeState() const { return NodeView.State; }

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UFUNCTION()
	void HandleNodeClicked();

	UFUNCTION()
	UUserWidget* BuildVariantFlyoutContent();

	UFUNCTION()
	void HandleVariantChosen(FName ChoiceId);

	UFUNCTION()
	void HandleVariantMenuOpenChanged(bool bIsOpen);

	void ApplyNodePresentation();

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_TalentNode;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UMenuAnchor> MenuAnchor_Variants;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RPG|Talents|Variant Flyout", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UGridTalentVariantFlyoutWidget> VariantFlyoutWidgetClass;

	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UTexture2D>> VariantIcons;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> Border_TalentNode;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_TalentTier;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_TalentName;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_TalentLevel;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_TalentState;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_VariantCount;
};
