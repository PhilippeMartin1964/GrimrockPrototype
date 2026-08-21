#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Magic/GridSpellbookTypes.h"
#include "GridPartySpellbookComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE (FGridPartySpellbookChangedSignature);

/**
 * Runtime owner for party spell knowledge.
 * MON18.2 deliberately keeps this state transient; Save/Restore is MON18.8.
 */
UCLASS (ClassGroup = (Grimrock), meta = (BlueprintSpawnableComponent))
class GRIMROCKPROTOTYPE_API UGridPartySpellbookComponent : public UActorComponent
{
    GENERATED_BODY ()

public:
    UGridPartySpellbookComponent ();

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Magic|Spellbook")
    FGridPartySpellbookState SpellbookState;

    /** Presentation notification only. Receivers must read the current state. */
    UPROPERTY (BlueprintAssignable, Category = "Magic|Spellbook|Events")
    FGridPartySpellbookChangedSignature OnSpellbookChanged;

    UFUNCTION (BlueprintCallable, Category = "Magic|Spellbook")
    bool EnsureCharacterSpellbook (FGuid CharacterId);

    UFUNCTION (BlueprintCallable, Category = "Magic|Spellbook")
    bool RemoveCharacterSpellbook (FGuid CharacterId);

    UFUNCTION (BlueprintCallable, Category = "Magic|Spellbook")
    EGridSpellbookMutationResult LearnSpell (FGuid CharacterId, FName SpellId);

    UFUNCTION (BlueprintCallable, Category = "Magic|Spellbook")
    EGridSpellbookMutationResult ForgetSpell (FGuid CharacterId, FName SpellId);

    UFUNCTION (BlueprintPure, Category = "Magic|Spellbook")
    bool KnowsSpell (FGuid CharacterId, FName SpellId) const;

    UFUNCTION (BlueprintPure, Category = "Magic|Spellbook")
    TArray<FName> GetKnownSpellIds (FGuid CharacterId) const;

    UFUNCTION (BlueprintCallable, Category = "Magic|Spellbook")
    void ResetAllSpellbooks ();

    bool ValidateSpellbookState (FString& OutError) const;
};
