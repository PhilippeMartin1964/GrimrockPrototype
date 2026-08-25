#pragma once

#include "CoreMinimal.h"

enum class EGridObjectCommand : uint8;
enum class EGridObjectCondition : uint8;
enum class EGridObjectEvent : uint8;
struct FGridLevelObjectData;
struct FGridObjectLink;

/**
 * Qualifie ce que le runtime sait réellement faire lorsqu'une commande est
 * appliquée à un type de cible. Cette distinction évite de confondre un simple
 * stockage de l'état d'activation avec un effet de gameplay effectivement
 * implémenté.
 */
enum class EGridEditorCommandRuntimeSupport : uint8
{
	Unsupported,
	StateOnly,
	Gameplay
};

/** Shared Grid Editor connector policy used by Slate and automation tests. */
namespace GridEditorLinkPolicy
{
	GRIMROCKPROTOTYPEEDITOR_API bool CanObjectEmitEvents(const FGridLevelObjectData& ObjectData);

	GRIMROCKPROTOTYPEEDITOR_API bool CanObjectReceiveCommands(const FGridLevelObjectData& ObjectData);

	GRIMROCKPROTOTYPEEDITOR_API TArray<EGridObjectEvent> GetSupportedEventsForSource(const FGridLevelObjectData& ObjectData);

	GRIMROCKPROTOTYPEEDITOR_API TArray<EGridObjectCommand> GetSupportedCommandsForTarget(const FGridLevelObjectData& ObjectData);

	/**
     * Returns the condition choices that are valid for the selected target.
     * Current non-trivial conditions inspect a receptacle target; all other
     * target types therefore expose only None.
     */
	GRIMROCKPROTOTYPEEDITOR_API TArray<EGridObjectCondition> GetSupportedConditionsForTarget(const FGridLevelObjectData& ObjectData);

	/**
     * Describes the effective C++ runtime support for one target/command pair.
     * StateOnly means the generic activation state can be stored but no complete
     * specialized gameplay effect is currently implemented.
     */
	GRIMROCKPROTOTYPEEDITOR_API EGridEditorCommandRuntimeSupport GetCommandRuntimeSupport(const FGridLevelObjectData& ObjectData, EGridObjectCommand Command);

	/**
     * Exact persistent identity used by MON19.2 when comparing connectors.
     * Every condition field participates so two links with the same historical
     * source/event/target/command quadruplet may legitimately coexist.
     */
	GRIMROCKPROTOTYPEEDITOR_API bool AreLinksExactlyEquivalent(const FGridObjectLink& A, const FGridObjectLink& B);

	GRIMROCKPROTOTYPEEDITOR_API TArray<EGridObjectEvent> GetEventDisplayOrder();
}
