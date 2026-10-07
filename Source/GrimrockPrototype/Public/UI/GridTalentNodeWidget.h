#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/GridSkillsUiTypes.h"
#include "UI/RPGTalentPresentationAsset.h"
#include "GridTalentNodeWidget.generated.h"

class UBorder;
class UButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGridTalentNodeClickedSignature, FName, TalentNodeId);

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
	FLinearColor BranchAccentColor = FLinearColor::White;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|Node")
	bool bInitialized = false;

	UPROPERTY(BlueprintAssignable, Category = "RPG|Talents|Node|Events")
	FGridTalentNodeClickedSignature OnTalentNodeClicked;

	UFUNCTION(BlueprintCallable, Category = "RPG|Talents|Node")
	bool InitializeTalentNode(
		const FGridTalentNodeView& InNodeView,
		const FRPGTalentBranchPresentationDefinition& InBranchPresentation);

	UFUNCTION(BlueprintCallable, Category = "RPG|Talents|Node")
	void ClearTalentNode();

	UFUNCTION(BlueprintPure, Category = "RPG|Talents|Node")
	int32 GetVariantCount() const { return NodeView.Variants.Num(); }

	UFUNCTION(BlueprintPure, Category = "RPG|Talents|Node")
	bool HasVariants() const { return NodeView.Variants.Num() > 1; }

	UFUNCTION(BlueprintPure, Category = "RPG|Talents|Node")
	EGridTalentNodeState GetTalentNodeState() const { return NodeView.State; }

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UFUNCTION()
	void HandleNodeClicked();

	bool ResolveConceptualText(const FRPGTalentBranchPresentationDefinition& BranchPresentation);
	void ApplyNodePresentation();

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_TalentNode;

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
