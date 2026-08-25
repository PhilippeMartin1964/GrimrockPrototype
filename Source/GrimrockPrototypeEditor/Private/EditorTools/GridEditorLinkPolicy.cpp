#include "EditorTools/GridEditorLinkPolicy.h"

#include "Core/GridTypes.h"

namespace
{
	bool IsGenericStateCommand(EGridObjectCommand Command)
	{
		return Command == EGridObjectCommand::Toggle || Command == EGridObjectCommand::Open || Command == EGridObjectCommand::Close ||
			Command == EGridObjectCommand::Activate || Command == EGridObjectCommand::Deactivate;
	}

	bool IsGridEditorReceptacleCommand(EGridObjectCommand Command)
	{
		return Command == EGridObjectCommand::ReceptacleConsumeItem || Command == EGridObjectCommand::ReceptacleConsumeAllItems ||
			Command == EGridObjectCommand::ReceptacleEnableRemoval || Command == EGridObjectCommand::ReceptacleDisableRemoval;
	}
}

namespace GridEditorLinkPolicy
{
	TArray<EGridObjectEvent> GetSupportedEventsForSource(const FGridLevelObjectData& ObjectData)
	{
		switch (ObjectData.Type)
		{
			case EGridLevelObjectType::Button:
				return { EGridObjectEvent::Activated };

			case EGridLevelObjectType::Lever:
			case EGridLevelObjectType::PressurePlate:
			case EGridLevelObjectType::Trigger:
				return { EGridObjectEvent::Activated, EGridObjectEvent::Deactivated };

			case EGridLevelObjectType::Receptacle:
				return { EGridObjectEvent::ItemInserted, EGridObjectEvent::ItemRemoved, EGridObjectEvent::ItemChanged };

			case EGridLevelObjectType::MonsterSpawn:
				return { EGridObjectEvent::MonsterDied, EGridObjectEvent::MonsterSpawned, EGridObjectEvent::MonsterDespawned,
					EGridObjectEvent::MonsterTeleported, EGridObjectEvent::EncounterWaveStarted, EGridObjectEvent::EncounterCompleted };

			case EGridLevelObjectType::Logic:
				switch (ObjectData.Logic.NodeType)
				{
					case EGridLogicNodeType::CompareBool:
					case EGridLogicNodeType::CompareInt:
					case EGridLogicNodeType::Latch:
						return { EGridObjectEvent::Activated, EGridObjectEvent::Deactivated };

					default:
						return { EGridObjectEvent::Activated };
				}

			default:
				return {};
		}
	}

	TArray<EGridObjectCommand> GetSupportedCommandsForTarget(const FGridLevelObjectData& ObjectData)
	{
		switch (ObjectData.Type)
		{
			case EGridLevelObjectType::Door:
				return { EGridObjectCommand::Open, EGridObjectCommand::Close, EGridObjectCommand::Toggle, EGridObjectCommand::Activate,
					EGridObjectCommand::Deactivate };

			case EGridLevelObjectType::Receptacle:
				return { EGridObjectCommand::ReceptacleConsumeItem, EGridObjectCommand::ReceptacleConsumeAllItems, EGridObjectCommand::ReceptacleEnableRemoval,
					EGridObjectCommand::ReceptacleDisableRemoval };

			case EGridLevelObjectType::MonsterSpawn:
				return { EGridObjectCommand::Spawn, EGridObjectCommand::Despawn, EGridObjectCommand::Teleport, EGridObjectCommand::Activate,
					EGridObjectCommand::Deactivate, EGridObjectCommand::Enable, EGridObjectCommand::Disable, EGridObjectCommand::Toggle,
					EGridObjectCommand::StartEncounter };

			case EGridLevelObjectType::Logic:
				if (ObjectData.Logic.NodeType == EGridLogicNodeType::Latch)
				{
					return { EGridObjectCommand::LogicExecute, EGridObjectCommand::LogicReset };
				}
				return { EGridObjectCommand::LogicExecute };

			case EGridLevelObjectType::StoryCompanion:
				return { EGridObjectCommand::OfferRecruitment };

			case EGridLevelObjectType::CustomRecruiter:
				return { EGridObjectCommand::OpenCustomRecruit };

			default:
				return {};
		}
	}

	TArray<EGridObjectCondition> GetSupportedConditionsForTarget(const FGridLevelObjectData& ObjectData)
	{
		TArray<EGridObjectCondition> Conditions = { EGridObjectCondition::None, EGridObjectCondition::LevelVariableBoolEquals,
			EGridObjectCondition::LevelVariableIntCompare };

		if (ObjectData.Type == EGridLevelObjectType::Receptacle)
		{
			Conditions.Add(EGridObjectCondition::ReceptacleIsEmpty);
			Conditions.Add(EGridObjectCondition::ReceptacleHasAnyItem);
			Conditions.Add(EGridObjectCondition::ReceptacleContainsItemDefinition);
			Conditions.Add(EGridObjectCondition::ReceptacleContainsItemTag);
			Conditions.Add(EGridObjectCondition::ReceptacleContainsItemType);
			Conditions.Add(EGridObjectCondition::ReceptacleItemCountAtLeast);
			Conditions.Add(EGridObjectCondition::ReceptacleWeightAtLeast);
		}

		return Conditions;
	}

	EGridEditorCommandRuntimeSupport GetCommandRuntimeSupport(const FGridLevelObjectData& ObjectData, EGridObjectCommand Command)
	{
		if (ObjectData.Type == EGridLevelObjectType::MonsterSpawn || ObjectData.Type == EGridLevelObjectType::Logic ||
			ObjectData.Type == EGridLevelObjectType::StoryCompanion || ObjectData.Type == EGridLevelObjectType::CustomRecruiter)
		{
			return GetSupportedCommandsForTarget(ObjectData).Contains(Command) ? EGridEditorCommandRuntimeSupport::Gameplay
																			   : EGridEditorCommandRuntimeSupport::Unsupported;
		}

		if (IsGridEditorReceptacleCommand(Command))
		{
			return ObjectData.Type == EGridLevelObjectType::Receptacle ? EGridEditorCommandRuntimeSupport::Gameplay
																	   : EGridEditorCommandRuntimeSupport::Unsupported;
		}

		if (!IsGenericStateCommand(Command))
		{
			return EGridEditorCommandRuntimeSupport::Unsupported;
		}

		switch (ObjectData.Type)
		{
			case EGridLevelObjectType::Door:
			case EGridLevelObjectType::Lever:
			case EGridLevelObjectType::PressurePlate:
				return EGridEditorCommandRuntimeSupport::Gameplay;

			case EGridLevelObjectType::Button:
			case EGridLevelObjectType::Decoration:
			case EGridLevelObjectType::ItemSpawn:
			case EGridLevelObjectType::Item:
			case EGridLevelObjectType::Light:
			case EGridLevelObjectType::Teleporter:
			case EGridLevelObjectType::Trigger:
			case EGridLevelObjectType::Receptacle:
				return EGridEditorCommandRuntimeSupport::StateOnly;

			case EGridLevelObjectType::CustomRecruiter:
			case EGridLevelObjectType::StoryCompanion:
			case EGridLevelObjectType::Logic:
			case EGridLevelObjectType::None:
			default:
				return EGridEditorCommandRuntimeSupport::Unsupported;
		}
	}

	bool AreLinksExactlyEquivalent(const FGridObjectLink& A, const FGridObjectLink& B)
	{
		return A.SourceObjectId == B.SourceObjectId && A.TargetObjectId == B.TargetObjectId && A.SourceEvent == B.SourceEvent && A.Command == B.Command &&
			A.LuaScriptId == B.LuaScriptId && A.LuaCallbackName == B.LuaCallbackName && A.Condition == B.Condition &&
			A.ConditionVariableId == B.ConditionVariableId && A.ConditionBoolValue == B.ConditionBoolValue &&
			A.ConditionIntComparison == B.ConditionIntComparison && A.ConditionIntValue == B.ConditionIntValue &&
			A.ConditionItemDefinitionId == B.ConditionItemDefinitionId && A.ConditionItemTag == B.ConditionItemTag &&
			A.ConditionItemType == B.ConditionItemType && A.ConditionCount == B.ConditionCount && A.ConditionWeight == B.ConditionWeight &&
			A.bInvertCondition == B.bInvertCondition;
	}

	bool CanObjectEmitEvents(const FGridLevelObjectData& ObjectData)
	{
		return !GetSupportedEventsForSource(ObjectData).IsEmpty();
	}

	bool CanObjectReceiveCommands(const FGridLevelObjectData& ObjectData)
	{
		return !GetSupportedCommandsForTarget(ObjectData).IsEmpty();
	}

	TArray<EGridObjectEvent> GetEventDisplayOrder()
	{
		return { EGridObjectEvent::Activated, EGridObjectEvent::Deactivated, EGridObjectEvent::ItemInserted, EGridObjectEvent::ItemRemoved,
			EGridObjectEvent::ItemChanged, EGridObjectEvent::Used, EGridObjectEvent::Entered, EGridObjectEvent::Exited, EGridObjectEvent::Opened,
			EGridObjectEvent::Closed, EGridObjectEvent::Enabled, EGridObjectEvent::Disabled, EGridObjectEvent::MonsterDied, EGridObjectEvent::MonsterSpawned,
			EGridObjectEvent::MonsterDespawned, EGridObjectEvent::MonsterTeleported, EGridObjectEvent::EncounterWaveStarted,
			EGridObjectEvent::EncounterCompleted };
	}
}
