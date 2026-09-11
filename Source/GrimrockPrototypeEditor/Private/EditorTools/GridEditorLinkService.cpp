#include "EditorTools/GridEditorLinkService.h"

#include "EditorTools/GridEditorLinkPolicy.h"
#include "EditorTools/GridLevelEditorActor.h"
#include "Core/GridLevelAsset.h"
#include "Core/GridLevelPlacementTypes.h"

namespace
{
	EGridLogicNodeType GetLogicNodeTypeForPlacement(const UGridLevelAsset& LevelAsset, const FGuid& PlacementId)
	{
		if (const FGridLogicObjectInstance* Logic = LevelAsset.FindLogicObjectInstanceById(PlacementId))
		{
			return Logic->Logic.NodeType;
		}
		return EGridLogicNodeType::Relay;
	}
}

namespace GridEditorLinkService
{
	FGridObjectLink NormalizeLink(const FGridObjectLink& Link)
	{
		FGridObjectLink Normalized = Link;

		const FName DefinitionId = Normalized.ConditionItemDefinitionId;
		const FName ItemTag = Normalized.ConditionItemTag;
		const EGridItemType ItemType = Normalized.ConditionItemType;
		const int32 Count = Normalized.ConditionCount;
		const float Weight = Normalized.ConditionWeight;

		Normalized.ConditionItemDefinitionId = NAME_None;
		Normalized.ConditionItemTag = NAME_None;
		Normalized.ConditionItemType = EGridItemType::None;
		Normalized.ConditionCount = 1;
		Normalized.ConditionWeight = 0.0f;

		switch (Normalized.Condition)
		{
			case EGridObjectCondition::ReceptacleContainsItemDefinition:
				Normalized.ConditionItemDefinitionId = DefinitionId;
				break;

			case EGridObjectCondition::ReceptacleContainsItemTag:
				Normalized.ConditionItemTag = ItemTag;
				break;

			case EGridObjectCondition::ReceptacleContainsItemType:
				Normalized.ConditionItemType = ItemType;
				break;

			case EGridObjectCondition::ReceptacleItemCountAtLeast:
				Normalized.ConditionCount = Count;
				break;

			case EGridObjectCondition::ReceptacleWeightAtLeast:
				Normalized.ConditionWeight = Weight;
				break;

			case EGridObjectCondition::None:
				Normalized.bInvertCondition = false;
				break;

			case EGridObjectCondition::ReceptacleIsEmpty:
			case EGridObjectCondition::ReceptacleHasAnyItem:
			default:
				break;
		}

		return Normalized;
	}

	bool IsConditionConfigurationValid(const FGridObjectLink& Link)
	{
		switch (Link.Condition)
		{
			case EGridObjectCondition::None:
			case EGridObjectCondition::ReceptacleIsEmpty:
			case EGridObjectCondition::ReceptacleHasAnyItem:
				return true;

			case EGridObjectCondition::ReceptacleContainsItemDefinition:
				return !Link.ConditionItemDefinitionId.IsNone();

			case EGridObjectCondition::ReceptacleContainsItemTag:
				return !Link.ConditionItemTag.IsNone();

			case EGridObjectCondition::ReceptacleContainsItemType:
				return Link.ConditionItemType != EGridItemType::None;

			case EGridObjectCondition::ReceptacleItemCountAtLeast:
				return Link.ConditionCount > 0;

			case EGridObjectCondition::ReceptacleWeightAtLeast:
				return Link.ConditionWeight > 0.0f;

			default:
				return false;
		}
	}

	bool IsLinkSupported(const UGridLevelAsset& LevelAsset, const FGridObjectLink& Link)
	{
		if (!Link.SourceObjectId.IsValid() || !Link.TargetObjectId.IsValid())
		{
			return false;
		}

		const EGridLevelObjectType SourceType = LevelAsset.GetTypedPlacementType(Link.SourceObjectId);
		const EGridLevelObjectType TargetType = LevelAsset.GetTypedPlacementType(Link.TargetObjectId);
		if (SourceType == EGridLevelObjectType::None || TargetType == EGridLevelObjectType::None)
		{
			return false;
		}

		const EGridLogicNodeType SourceLogicNodeType = GetLogicNodeTypeForPlacement(LevelAsset, Link.SourceObjectId);
		const EGridLogicNodeType TargetLogicNodeType = GetLogicNodeTypeForPlacement(LevelAsset, Link.TargetObjectId);

		if (!GridEditorLinkPolicy::GetSupportedEventsForSource(SourceType, SourceLogicNodeType).Contains(Link.SourceEvent) ||
			!GridEditorLinkPolicy::GetSupportedCommandsForTarget(TargetType, TargetLogicNodeType).Contains(Link.Command) ||
			!GridEditorLinkPolicy::GetSupportedConditionsForTarget(TargetType).Contains(Link.Condition) || !IsConditionConfigurationValid(Link))
		{
			return false;
		}

		return true;
	}

	bool ContainsExactLink(const TArray<FGridObjectLink>& Links, const FGridObjectLink& Link)
	{
		return Links.ContainsByPredicate(
			[&Link](const FGridObjectLink& Existing)
			{
				return GridEditorLinkPolicy::AreLinksExactlyEquivalent(Existing, Link);
			});
	}

	bool AddExactLink(TArray<FGridObjectLink>& Links, const FGridObjectLink& Link)
	{
		const FGridObjectLink Normalized = NormalizeLink(Link);
		if (ContainsExactLink(Links, Normalized))
		{
			return false;
		}

		Links.Add(Normalized);
		return true;
	}

	int32 RemoveExactLink(TArray<FGridObjectLink>& Links, const FGridObjectLink& Link)
	{
		const int32 Index = Links.IndexOfByPredicate(
			[&Link](const FGridObjectLink& Existing)
			{
				return GridEditorLinkPolicy::AreLinksExactlyEquivalent(Existing, Link);
			});

		if (Index == INDEX_NONE)
		{
			return 0;
		}

		Links.RemoveAt(Index);
		return 1;
	}

	bool CreateLink(AGridLevelEditorActor& EditorActor, const FGridObjectLink& Link)
	{
		UGridLevelAsset* LevelAsset = EditorActor.LevelAsset;
		if (!LevelAsset)
		{
			return false;
		}

		const FGridObjectLink Normalized = NormalizeLink(Link);
		if (!IsLinkSupported(*LevelAsset, Normalized))
		{
			return false;
		}

#if WITH_EDITOR
		LevelAsset->Modify();
#endif

		if (!AddExactLink(LevelAsset->Links, Normalized))
		{
			return false;
		}

#if WITH_EDITOR
		LevelAsset->MarkPackageDirty();
#endif

		EditorActor.LastSelectedObjectId = Normalized.SourceObjectId;
		EditorActor.RebuildPreview();
		return true;
	}

	bool RemoveExactLink(AGridLevelEditorActor& EditorActor, const FGridObjectLink& Link)
	{
		UGridLevelAsset* LevelAsset = EditorActor.LevelAsset;
		if (!LevelAsset)
		{
			return false;
		}

#if WITH_EDITOR
		LevelAsset->Modify();
#endif

		if (RemoveExactLink(LevelAsset->Links, Link) <= 0)
		{
			return false;
		}

#if WITH_EDITOR
		LevelAsset->MarkPackageDirty();
#endif

		EditorActor.RebuildPreview();
		return true;
	}
}
