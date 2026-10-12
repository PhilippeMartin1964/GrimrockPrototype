#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "RPG/RPGAlchemistAuthoring.h"
#include "Runtime/GridItemDefinitionAsset.h"

namespace UIRPGCONTENTTEXT021
{
	void CollectProductionItemIds(TArray<FName>& OutItemIds)
	{
		TArray<FName> B1ItemIds;
		TArray<FName> B2ItemIds;
		FRPGAlchemistAuthoring::GetB1ItemIds(B1ItemIds);
		FRPGAlchemistAuthoring::GetB2ItemIds(B2ItemIds);

		OutItemIds = MoveTemp(B1ItemIds);
		OutItemIds.Append(B2ItemIds);
	}

	void LogSerializedText(const UGridItemDefinitionAsset& Item)
	{
		UE_LOG(
			LogTemp,
			Display,
			TEXT("[UI-RPG-CONTENT-TEXT02.1][ITEM] ItemId=%s DisplayName=\"%s\" Description=\"%s\" ReadText=\"%s\" QuickActionId=%s QuickDisplayName=\"%s\" QuickDescription=\"%s\""),
			*Item.ItemDefinitionId.ToString(),
			*Item.DisplayName.ToString(),
			*Item.Description.ToString(),
			*Item.ReadText.ToString(),
			*Item.QuickItemCombatAction.ActionId.ToString(),
			*Item.QuickItemCombatAction.DisplayName.ToString(),
			*Item.QuickItemCombatAction.Description.ToString());
	}

	void LogOverwrite(
		FName ItemId,
		const TCHAR* Field,
		const FText& Before,
		const FText& After)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[UI-RPG-CONTENT-TEXT02.1][TEXT-OVERWRITE] ItemId=%s Field=%s Before=\"%s\" After=\"%s\""),
			*ItemId.ToString(),
			Field,
			*Before.ToString(),
			*After.ToString());
	}
}

/**
 * UI-RPG-CONTENT-TEXT02.1 is a characterization audit only.
 *
 * It never saves a package. The eleven production Alchemist DA_Item_* assets
 * are loaded as serialized text-authority candidates, while ConfigureItem()
 * runs only against transient duplicates.
 *
 * TEXT-OVERWRITE entries are informational: they identify fields currently
 * reclaimed or cleared by the Alchemist mechanical rematerializer.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIRPGCONTENTTEXT021ItemTextOwnershipAuditTest,
	"Grimrock.UI.RPG.CONTENTTEXT02.ItemOwnershipAudit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPGCONTENTTEXT021ItemTextOwnershipAuditTest::RunTest(const FString&)
{
	using namespace UIRPGCONTENTTEXT021;

	TArray<FName> ItemIds;
	CollectProductionItemIds(ItemIds);

	TestEqual(TEXT("Alchemist production authoring targets exactly eleven QuickItems"), ItemIds.Num(), 11);

	TSet<FName> UniqueItemIds;
	for (const FName ItemId : ItemIds)
	{
		TestFalse(TEXT("Alchemist production ItemId is non-empty"), ItemId.IsNone());
		TestFalse(
			*FString::Printf(TEXT("%s ItemId appears only once"), *ItemId.ToString()),
			UniqueItemIds.Contains(ItemId));
		UniqueItemIds.Add(ItemId);
	}

	int32 LoadedItemCount = 0;
	int32 SerializedDisplayNameCount = 0;
	int32 SerializedDescriptionCount = 0;
	int32 SerializedReadTextCount = 0;
	int32 SerializedQuickDisplayNameCount = 0;
	int32 SerializedQuickDescriptionCount = 0;

	int32 ItemDisplayNameOverwriteCount = 0;
	int32 ItemDescriptionOverwriteCount = 0;
	int32 ItemReadTextOverwriteCount = 0;
	int32 QuickDisplayNameOverwriteCount = 0;
	int32 QuickDescriptionOverwriteCount = 0;

	for (const FName ItemId : ItemIds)
	{
		const FString ObjectPath = FRPGAlchemistAuthoring::GetItemObjectPath(ItemId);
		UGridItemDefinitionAsset* Serialized =
			LoadObject<UGridItemDefinitionAsset>(nullptr, *ObjectPath);
		if (!TestNotNull(
				*FString::Printf(TEXT("%s serialized DA_Item loads"), *ItemId.ToString()),
				Serialized))
		{
			continue;
		}

		++LoadedItemCount;
		TestEqual(
			*FString::Printf(TEXT("%s keeps canonical ItemDefinitionId"), *ItemId.ToString()),
			Serialized->ItemDefinitionId,
			ItemId);
		TestTrue(
			*FString::Printf(TEXT("%s remains structurally valid"), *ItemId.ToString()),
			Serialized->IsValidDefinition());
		TestTrue(
			*FString::Printf(TEXT("%s exposes a QuickItem action"), *ItemId.ToString()),
			Serialized->bProvidesQuickItemCombatAction);

		if (!Serialized->DisplayName.IsEmpty())
		{
			++SerializedDisplayNameCount;
		}
		else
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[UI-RPG-CONTENT-TEXT02.1][ITEM-DISPLAYNAME-EMPTY] ItemId=%s"),
				*ItemId.ToString());
		}

		if (!Serialized->Description.IsEmpty())
		{
			++SerializedDescriptionCount;
		}
		else
		{
			UE_LOG(
				LogTemp,
				Display,
				TEXT("[UI-RPG-CONTENT-TEXT02.1][ITEM-DESCRIPTION-EMPTY] ItemId=%s"),
				*ItemId.ToString());
		}

		if (!Serialized->ReadText.IsEmpty())
		{
			++SerializedReadTextCount;
		}
		if (!Serialized->QuickItemCombatAction.DisplayName.IsEmpty())
		{
			++SerializedQuickDisplayNameCount;
		}
		if (!Serialized->QuickItemCombatAction.Description.IsEmpty())
		{
			++SerializedQuickDescriptionCount;
		}

		LogSerializedText(*Serialized);

		UGridItemDefinitionAsset* Working =
			DuplicateObject<UGridItemDefinitionAsset>(Serialized, GetTransientPackage());
		if (!TestNotNull(
				*FString::Printf(TEXT("%s transient audit duplicate is created"), *ItemId.ToString()),
				Working))
		{
			continue;
		}

		const FText SentinelItemDisplayName = FText::FromString(
			FString::Printf(TEXT("TEXT02.1 Item Name %s"), *ItemId.ToString()));
		const FText SentinelItemDescription = FText::FromString(
			FString::Printf(TEXT("TEXT02.1 Item Description %s"), *ItemId.ToString()));
		const FText SentinelReadText = FText::FromString(
			FString::Printf(TEXT("TEXT02.1 Read Text %s"), *ItemId.ToString()));
		const FText SentinelQuickDisplayName = FText::FromString(
			FString::Printf(TEXT("TEXT02.1 Quick Name %s"), *ItemId.ToString()));
		const FText SentinelQuickDescription = FText::FromString(
			FString::Printf(TEXT("TEXT02.1 Quick Description %s"), *ItemId.ToString()));

		Working->DisplayName = SentinelItemDisplayName;
		Working->Description = SentinelItemDescription;
		Working->ReadText = SentinelReadText;
		Working->QuickItemCombatAction.DisplayName = SentinelQuickDisplayName;
		Working->QuickItemCombatAction.Description = SentinelQuickDescription;
		Working->QuickItemCombatAction.ActionPointCost = 777;

		if (!TestTrue(
				*FString::Printf(TEXT("%s current ConfigureItem rematerializes a transient duplicate"), *ItemId.ToString()),
				FRPGAlchemistAuthoring::ConfigureItem(*Working, ItemId)))
		{
			continue;
		}

		TestTrue(
			*FString::Printf(TEXT("%s QuickItem mechanics were actually rebuilt"), *ItemId.ToString()),
			Working->QuickItemCombatAction.ActionPointCost != 777);

		if (!SentinelItemDisplayName.EqualTo(Working->DisplayName))
		{
			++ItemDisplayNameOverwriteCount;
			LogOverwrite(ItemId, TEXT("Item.DisplayName"), SentinelItemDisplayName, Working->DisplayName);
		}
		if (!SentinelItemDescription.EqualTo(Working->Description))
		{
			++ItemDescriptionOverwriteCount;
			LogOverwrite(ItemId, TEXT("Item.Description"), SentinelItemDescription, Working->Description);
		}
		if (!SentinelReadText.EqualTo(Working->ReadText))
		{
			++ItemReadTextOverwriteCount;
			LogOverwrite(ItemId, TEXT("Item.ReadText"), SentinelReadText, Working->ReadText);
		}
		if (!SentinelQuickDisplayName.EqualTo(Working->QuickItemCombatAction.DisplayName))
		{
			++QuickDisplayNameOverwriteCount;
			LogOverwrite(
				ItemId,
				TEXT("QuickItemCombatAction.DisplayName"),
				SentinelQuickDisplayName,
				Working->QuickItemCombatAction.DisplayName);
		}
		if (!SentinelQuickDescription.EqualTo(Working->QuickItemCombatAction.Description))
		{
			++QuickDescriptionOverwriteCount;
			LogOverwrite(
				ItemId,
				TEXT("QuickItemCombatAction.Description"),
				SentinelQuickDescription,
				Working->QuickItemCombatAction.Description);
		}
	}

	TestEqual(TEXT("Audit loads all eleven production Alchemist DA_Item assets"), LoadedItemCount, 11);
	TestEqual(TEXT("Every production Alchemist item has a serialized DisplayName"), SerializedDisplayNameCount, 11);

	UE_LOG(
		LogTemp,
		Display,
		TEXT("[UI-RPG-CONTENT-TEXT02.1][SUMMARY] LoadedItems=%d SerializedDisplayNames=%d SerializedDescriptions=%d SerializedReadTexts=%d SerializedQuickDisplayNames=%d SerializedQuickDescriptions=%d ItemDisplayNameOverwrites=%d ItemDescriptionOverwrites=%d ItemReadTextOverwrites=%d QuickDisplayNameOverwrites=%d QuickDescriptionOverwrites=%d"),
		LoadedItemCount,
		SerializedDisplayNameCount,
		SerializedDescriptionCount,
		SerializedReadTextCount,
		SerializedQuickDisplayNameCount,
		SerializedQuickDescriptionCount,
		ItemDisplayNameOverwriteCount,
		ItemDescriptionOverwriteCount,
		ItemReadTextOverwriteCount,
		QuickDisplayNameOverwriteCount,
		QuickDescriptionOverwriteCount);

	return true;
}

#endif
