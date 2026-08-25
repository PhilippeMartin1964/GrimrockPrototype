#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridTypes.h"
#include "EditorTools/GridEditorLinkPolicy.h"
#include "EditorTools/GridEditorLinkService.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridEditorMON192ExactLinkMutationsTest, "Grimrock.MON19.2.Editor.ExactLinkMutations",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorMON192ExactLinkMutationsTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridObjectLink DefinitionLink;
	DefinitionLink.SourceObjectId = FGuid::NewGuid();
	DefinitionLink.TargetObjectId = FGuid::NewGuid();
	DefinitionLink.SourceEvent = EGridObjectEvent::Activated;
	DefinitionLink.Command = EGridObjectCommand::ReceptacleConsumeItem;
	DefinitionLink.Condition = EGridObjectCondition::ReceptacleContainsItemDefinition;
	DefinitionLink.ConditionItemDefinitionId = TEXT("Item_RedGem");

	FGridObjectLink TagLink = DefinitionLink;
	TagLink.Condition = EGridObjectCondition::ReceptacleContainsItemTag;
	TagLink.ConditionItemDefinitionId = NAME_None;
	TagLink.ConditionItemTag = TEXT("Gem_Red");

	TArray<FGridObjectLink> Links;

	TestTrue(TEXT("First conditional variant is added"), GridEditorLinkService::AddExactLink(Links, DefinitionLink));
	TestTrue(TEXT("Second condition may share the same historical quadruplet"), GridEditorLinkService::AddExactLink(Links, TagLink));
	TestEqual(TEXT("Both conditional variants coexist"), Links.Num(), 2);

	TestFalse(TEXT("An exactly identical conditional link is rejected"), GridEditorLinkService::AddExactLink(Links, DefinitionLink));
	TestEqual(TEXT("Duplicate rejection preserves the two variants"), Links.Num(), 2);

	TestEqual(TEXT("Exact removal removes one definition variant"), GridEditorLinkService::RemoveExactLink(Links, Links[0]), 1);
	TestEqual(TEXT("Only one variant remains after exact removal"), Links.Num(), 1);
	TestTrue(TEXT("The tag variant remains intact"), GridEditorLinkPolicy::AreLinksExactlyEquivalent(Links[0], GridEditorLinkService::NormalizeLink(TagLink)));

	TestEqual(TEXT("Removing the already removed canonical variant is a no-op"),
		GridEditorLinkService::RemoveExactLink(Links, GridEditorLinkService::NormalizeLink(DefinitionLink)), 0);
	TestEqual(TEXT("Removing the final exact variant succeeds"), GridEditorLinkService::RemoveExactLink(Links, Links[0]), 1);
	TestEqual(TEXT("No connector remains"), Links.Num(), 0);

	FGridObjectLink StaleNone;
	StaleNone.Condition = EGridObjectCondition::None;
	StaleNone.ConditionItemDefinitionId = TEXT("StaleDefinition");
	StaleNone.ConditionItemTag = TEXT("StaleTag");
	StaleNone.ConditionItemType = EGridItemType::Gem;
	StaleNone.ConditionCount = 99;
	StaleNone.ConditionWeight = 42.0f;
	StaleNone.bInvertCondition = true;

	const FGridObjectLink NormalizedNone = GridEditorLinkService::NormalizeLink(StaleNone);
	TestTrue(TEXT("None clears the definition parameter"), NormalizedNone.ConditionItemDefinitionId.IsNone());
	TestTrue(TEXT("None clears the tag parameter"), NormalizedNone.ConditionItemTag.IsNone());
	TestTrue(TEXT("None clears the item type parameter"), NormalizedNone.ConditionItemType == EGridItemType::None);
	TestEqual(TEXT("None restores the canonical count"), NormalizedNone.ConditionCount, 1);
	TestTrue(TEXT("None restores the canonical weight"), FMath::IsNearlyZero(NormalizedNone.ConditionWeight));
	TestFalse(TEXT("None cannot remain inverted"), NormalizedNone.bInvertCondition);
	TestFalse(TEXT("Normalization does not redefine the exact identity of persisted data"),
		GridEditorLinkPolicy::AreLinksExactlyEquivalent(StaleNone, NormalizedNone));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridEditorMON192ConditionConfigurationTest, "Grimrock.MON19.2.Editor.ConditionConfiguration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorMON192ConditionConfigurationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridObjectLink Link;

	Link.Condition = EGridObjectCondition::None;
	TestTrue(TEXT("None requires no parameter"), GridEditorLinkService::IsConditionConfigurationValid(Link));

	Link.Condition = EGridObjectCondition::ReceptacleContainsItemDefinition;
	Link.ConditionItemDefinitionId = NAME_None;
	TestFalse(TEXT("Item-definition condition rejects an empty id"), GridEditorLinkService::IsConditionConfigurationValid(Link));
	Link.ConditionItemDefinitionId = TEXT("Item_RedGem");
	TestTrue(TEXT("Item-definition condition accepts an id"), GridEditorLinkService::IsConditionConfigurationValid(Link));

	Link = FGridObjectLink();
	Link.Condition = EGridObjectCondition::ReceptacleContainsItemTag;
	TestFalse(TEXT("Item-tag condition rejects an empty tag"), GridEditorLinkService::IsConditionConfigurationValid(Link));
	Link.ConditionItemTag = TEXT("Gem_Red");
	TestTrue(TEXT("Item-tag condition accepts a tag"), GridEditorLinkService::IsConditionConfigurationValid(Link));

	Link = FGridObjectLink();
	Link.Condition = EGridObjectCondition::ReceptacleContainsItemType;
	TestFalse(TEXT("Item-type condition rejects None"), GridEditorLinkService::IsConditionConfigurationValid(Link));
	Link.ConditionItemType = EGridItemType::Gem;
	TestTrue(TEXT("Item-type condition accepts a concrete type"), GridEditorLinkService::IsConditionConfigurationValid(Link));

	Link = FGridObjectLink();
	Link.Condition = EGridObjectCondition::ReceptacleItemCountAtLeast;
	Link.ConditionCount = 0;
	TestFalse(TEXT("Count condition rejects zero"), GridEditorLinkService::IsConditionConfigurationValid(Link));
	Link.ConditionCount = 2;
	TestTrue(TEXT("Count condition accepts a positive count"), GridEditorLinkService::IsConditionConfigurationValid(Link));

	Link = FGridObjectLink();
	Link.Condition = EGridObjectCondition::ReceptacleWeightAtLeast;
	Link.ConditionWeight = 0.0f;
	TestFalse(TEXT("Weight condition rejects zero"), GridEditorLinkService::IsConditionConfigurationValid(Link));
	Link.ConditionWeight = 2.5f;
	TestTrue(TEXT("Weight condition accepts a positive value"), GridEditorLinkService::IsConditionConfigurationValid(Link));

	return true;
}

#endif
