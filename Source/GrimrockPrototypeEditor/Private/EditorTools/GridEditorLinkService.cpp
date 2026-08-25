#include "EditorTools/GridEditorLinkService.h"

#include "EditorTools/GridEditorLinkPolicy.h"
#include "EditorTools/GridLevelEditorActor.h"
#include "Core/GridLevelAsset.h"
#include "Core/GridLevelVariableTypes.h"

namespace
{
	const FGridLevelObjectData* FindLinkServiceObjectById(const UGridLevelAsset& LevelAsset, const FGuid& ObjectId)
	{
		return LevelAsset.Objects.FindByPredicate(
			[&ObjectId](const FGridLevelObjectData& Object)
			{
				return Object.ObjectId == ObjectId;
			});
	}

	const FGridLevelVariableDefinition* FindLinkServiceVariableDefinition(const UGridLevelAsset& LevelAsset, FName VariableId)
	{
		return LevelAsset.LevelVariables.FindByPredicate(
			[VariableId](const FGridLevelVariableDefinition& Definition)
			{
				return Definition.VariableId == VariableId;
			});
	}

	bool IsSupportedIntComparison(EGridLogicIntComparison Comparison)
	{
		switch (Comparison)
		{
			case EGridLogicIntComparison::Equal:
			case EGridLogicIntComparison::NotEqual:
			case EGridLogicIntComparison::Less:
			case EGridLogicIntComparison::LessOrEqual:
			case EGridLogicIntComparison::Greater:
			case EGridLogicIntComparison::GreaterOrEqual:
				return true;

			default:
				return false;
		}
	}
}

namespace GridEditorLinkService
{
	FGridObjectLink NormalizeLink(const FGridObjectLink& Link)
	{
		FGridObjectLink Normalized = Link;

		const FName VariableId = Normalized.ConditionVariableId;
		const bool bBoolValue = Normalized.ConditionBoolValue;
		const EGridLogicIntComparison IntComparison = Normalized.ConditionIntComparison;
		const int32 IntValue = Normalized.ConditionIntValue;
		const FName DefinitionId = Normalized.ConditionItemDefinitionId;
		const FName ItemTag = Normalized.ConditionItemTag;
		const EGridItemType ItemType = Normalized.ConditionItemType;
		const int32 Count = Normalized.ConditionCount;
		const float Weight = Normalized.ConditionWeight;

		Normalized.ConditionVariableId = NAME_None;
		Normalized.ConditionBoolValue = false;
		Normalized.ConditionIntComparison = EGridLogicIntComparison::Equal;
		Normalized.ConditionIntValue = 0;
		Normalized.ConditionItemDefinitionId = NAME_None;
		Normalized.ConditionItemTag = NAME_None;
		Normalized.ConditionItemType = EGridItemType::None;
		Normalized.ConditionCount = 1;
		Normalized.ConditionWeight = 0.0f;

		switch (Normalized.Condition)
		{
			case EGridObjectCondition::LevelVariableBoolEquals:
				Normalized.ConditionVariableId = VariableId;
				Normalized.ConditionBoolValue = bBoolValue;
				break;

			case EGridObjectCondition::LevelVariableIntCompare:
				Normalized.ConditionVariableId = VariableId;
				Normalized.ConditionIntComparison = IntComparison;
				Normalized.ConditionIntValue = IntValue;
				break;

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

			case EGridObjectCondition::LevelVariableBoolEquals:
				return !Link.ConditionVariableId.IsNone();

			case EGridObjectCondition::LevelVariableIntCompare:
				return !Link.ConditionVariableId.IsNone() && IsSupportedIntComparison(Link.ConditionIntComparison);

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

		const FGridLevelObjectData* Source = FindLinkServiceObjectById(LevelAsset, Link.SourceObjectId);
		const FGridLevelObjectData* Target = FindLinkServiceObjectById(LevelAsset, Link.TargetObjectId);
		if (!Source || !Target)
		{
			return false;
		}

		if (!GridEditorLinkPolicy::GetSupportedEventsForSource(*Source).Contains(Link.SourceEvent) ||
			!GridEditorLinkPolicy::GetSupportedCommandsForTarget(*Target).Contains(Link.Command) ||
			!GridEditorLinkPolicy::GetSupportedConditionsForTarget(*Target).Contains(Link.Condition) || !IsConditionConfigurationValid(Link))
		{
			return false;
		}

		if (Link.Condition == EGridObjectCondition::LevelVariableBoolEquals || Link.Condition == EGridObjectCondition::LevelVariableIntCompare)
		{
			const FGridLevelVariableDefinition* Definition = FindLinkServiceVariableDefinition(LevelAsset, Link.ConditionVariableId);
			if (!Definition)
			{
				return false;
			}

			const EGridLevelVariableType RequiredType =
				Link.Condition == EGridObjectCondition::LevelVariableBoolEquals ? EGridLevelVariableType::Bool : EGridLevelVariableType::Int32;
			if (Definition->Type != RequiredType)
			{
				return false;
			}
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
