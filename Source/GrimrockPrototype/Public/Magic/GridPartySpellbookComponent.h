#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Magic/GridSpellbookTypes.h"
#include "GridPartySpellbookComponent.generated.h"

class UGridPartyInventoryComponent;
struct FGridCharacterInventoryState;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGridPartySpellbookChangedSignature);

/**
 * Spellbook mutation/read facade.
 *
 * TD07.3.3.7 removes the component-owned party Spellbook state. Durable spell
 * knowledge lives directly in FGridCharacterInventoryState::KnownSpellIds.
 * This component only resolves characters, applies validated mutations and
 * broadcasts presentation notifications.
 */
UCLASS(ClassGroup = (Grimrock), meta = (BlueprintSpawnableComponent))
class GRIMROCKPROTOTYPE_API UGridPartySpellbookComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGridPartySpellbookComponent();

	void InitializeSpellbookComponent(UGridPartyInventoryComponent* InInventoryComponent);

	UPROPERTY(BlueprintAssignable, Category = "Magic|Spellbook|Events")
	FGridPartySpellbookChangedSignature OnSpellbookChanged;

	UFUNCTION(BlueprintCallable, Category = "Magic|Spellbook")
	bool EnsureCharacterSpellbook(FGuid CharacterId);

	UFUNCTION(BlueprintCallable, Category = "Magic|Spellbook")
	bool RemoveCharacterSpellbook(FGuid CharacterId);

	UFUNCTION(BlueprintCallable, Category = "Magic|Spellbook")
	EGridSpellbookMutationResult LearnSpell(FGuid CharacterId, FName SpellId);

	UFUNCTION(BlueprintCallable, Category = "Magic|Spellbook")
	EGridSpellbookMutationResult ForgetSpell(FGuid CharacterId, FName SpellId);

	UFUNCTION(BlueprintPure, Category = "Magic|Spellbook")
	bool KnowsSpell(FGuid CharacterId, FName SpellId) const;

	UFUNCTION(BlueprintPure, Category = "Magic|Spellbook")
	TArray<FName> GetKnownSpellIds(FGuid CharacterId) const;

	UFUNCTION(BlueprintPure, Category = "Magic|Spellbook")
	bool GetCharacterSpellbookState(FGuid CharacterId, FGridCharacterSpellbookState& OutState) const;

	UFUNCTION(BlueprintCallable, Category = "Magic|Spellbook")
	void ResetAllSpellbooks();

	bool ValidateSpellbookState(FString& OutError) const;

private:
	UGridPartyInventoryComponent* ResolveInventoryComponent() const;
	FGridCharacterInventoryState* FindMutableCharacter(FGuid CharacterId) const;
	const FGridCharacterInventoryState* FindCharacter(FGuid CharacterId) const;

	TWeakObjectPtr<UGridPartyInventoryComponent> InventoryComponentOverride;
};
