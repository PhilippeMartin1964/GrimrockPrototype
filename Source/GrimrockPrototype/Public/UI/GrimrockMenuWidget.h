#pragma once

#include "CoreMinimal.h"
#include "UI/GrimrockDesignSurfaceWidget.h"
#include "UI/GridInventoryUiTypes.h"
#include "GrimrockMenuWidget.generated.h"

class AGrimrockPartyPawn;
class UGridSkillsWidget;
class UGridSpellbookWidget;
class UWidget;
class UWidgetSwitcher;

/**
 * Temporary shell for the remaining shared pages.
 *
 * Inventory and Map are independent viewport surfaces. This shell now keeps
 * Skills / Journal / Recipes / Codex / Spellbook only.
 */
UCLASS()
class GRIMROCKPROTOTYPE_API UGrimrockMenuWidget : public UGrimrockDesignSurfaceWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Menu")
	void InitializeMenuWidget(AGrimrockPartyPawn* InPartyPawn);

	UFUNCTION(BlueprintCallable, Category = "Menu")
	void RefreshSkills();

	UFUNCTION(BlueprintCallable, Category = "Menu")
	void RefreshSpellbook();


	UFUNCTION(BlueprintCallable, Category = "Menu|Pages")
	void SetActiveTopTab(EInventoryTopTab NewTab);

	UFUNCTION(BlueprintCallable, Category = "Menu")
	UGridSkillsWidget* GetSkillsWidget() const;

	UFUNCTION(BlueprintCallable, Category = "Menu")
	UGridSpellbookWidget* GetSpellbookWidget() const;


	UPROPERTY(BlueprintReadOnly, Category = "Menu")
	TObjectPtr<AGrimrockPartyPawn> OwningPartyPawn;

	UPROPERTY(BlueprintReadOnly, Category = "Menu|Pages")
	EInventoryTopTab CurrentTopTab = EInventoryTopTab::Skills;

protected:
	virtual void NativeConstruct() override;

private:
	UWidget* GetTopTabPage(EInventoryTopTab Tab) const;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidgetSwitcher> WidgetSwitcher_MainContent;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> Page_Skills;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> Page_Journal;


	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> Page_Recipes;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> Page_Codex;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Page_Spellbook;
};
