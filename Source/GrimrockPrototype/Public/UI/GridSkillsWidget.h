#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/GridSkillsUiTypes.h"
#include "GridSkillsWidget.generated.h"

class AGrimrockPartyPawn;
class UGridPartyInventoryComponent;
class UScrollBox;
class UVerticalBox;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGridSkillsWidgetRefreshedSignature);

/**
 * Read-only native presentation bridge for WBP_GridSkills.
 * Character selection remains authoritative in UGridPartyInventoryComponent.
 */
UCLASS()
class GRIMROCKPROTOTYPE_API UGridSkillsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|UI")
	TObjectPtr<AGrimrockPartyPawn> OwningPartyPawn;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|UI")
	TObjectPtr<UGridPartyInventoryComponent> InventoryComponent;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|UI")
	FGridSkillsPageView View;

	UPROPERTY(BlueprintAssignable, Category = "RPG|Skills|UI|Events")
	FGridSkillsWidgetRefreshedSignature OnSkillsRefreshed;

	UFUNCTION(BlueprintCallable, Category = "RPG|Skills|UI")
	void InitializeSkillsWidget(AGrimrockPartyPawn* InPartyPawn);

	UFUNCTION(BlueprintCallable, Category = "RPG|Skills|UI")
	void RefreshSkills();

	UFUNCTION(BlueprintPure, Category = "RPG|Skills|UI")
	int32 GetSkillEntryCount() const;

	UFUNCTION(BlueprintPure, Category = "RPG|Skills|UI")
	bool GetSkillEntry(int32 EntryIndex, FGridSkillEntryView& OutEntry) const;

	UFUNCTION(BlueprintPure, Category = "RPG|Skills|UI")
	int32 GetTalentEntryCount() const;

	UFUNCTION(BlueprintPure, Category = "RPG|Skills|UI")
	bool GetTalentEntry(int32 EntryIndex, FGridTalentEntryView& OutEntry) const;

protected:
	virtual void NativeDestruct() override;

private:
	UFUNCTION()
	void HandlePartyInventoryChanged(int32 CharacterIndex);

	void ClearView();

	/** Rebuild the minimal native runtime view inside the WBP root Border. */
	void RebuildPresentation();

	UPROPERTY(Transient)
	TObjectPtr<UScrollBox> NativeScrollBox;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> NativeContentBox;

	bool bRefreshInProgress = false;
};
