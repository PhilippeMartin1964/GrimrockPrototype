#pragma once

#include "CoreMinimal.h"
#include "Core/GridTypes.h"

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
	/** WORLDOBJ-MIG09 native policy surface: connector capabilities depend only on typed identity and, for Logic, its node type. */
	GRIMROCKPROTOTYPEEDITOR_API bool CanObjectEmitEvents(EGridLevelObjectType ObjectType, EGridLogicNodeType LogicNodeType = EGridLogicNodeType::Relay);
	GRIMROCKPROTOTYPEEDITOR_API bool CanObjectReceiveCommands(EGridLevelObjectType ObjectType, EGridLogicNodeType LogicNodeType = EGridLogicNodeType::Relay);
	GRIMROCKPROTOTYPEEDITOR_API TArray<EGridObjectEvent> GetSupportedEventsForSource(
		EGridLevelObjectType ObjectType, EGridLogicNodeType LogicNodeType = EGridLogicNodeType::Relay);
	GRIMROCKPROTOTYPEEDITOR_API TArray<EGridObjectCommand> GetSupportedCommandsForTarget(
		EGridLevelObjectType ObjectType, EGridLogicNodeType LogicNodeType = EGridLogicNodeType::Relay);
	GRIMROCKPROTOTYPEEDITOR_API TArray<EGridObjectCondition> GetSupportedConditionsForTarget(EGridLevelObjectType ObjectType);
	GRIMROCKPROTOTYPEEDITOR_API EGridEditorCommandRuntimeSupport GetCommandRuntimeSupport(
		EGridLevelObjectType ObjectType, EGridLogicNodeType LogicNodeType, EGridObjectCommand Command);


	/**
	 * Exact persistent identity used by MON19.2 when comparing connectors.
	 * Every condition field participates so two links with the same historical
	 * source/event/target/command quadruplet may legitimately coexist.
	 */
	GRIMROCKPROTOTYPEEDITOR_API bool AreLinksExactlyEquivalent(const FGridObjectLink& A, const FGridObjectLink& B);

	GRIMROCKPROTOTYPEEDITOR_API TArray<EGridObjectEvent> GetEventDisplayOrder();
}
