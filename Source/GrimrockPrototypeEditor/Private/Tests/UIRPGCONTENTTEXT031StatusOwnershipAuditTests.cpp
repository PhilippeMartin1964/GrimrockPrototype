#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "RPG/RPGAlchemistAuthoring.h"
#include "RPG/RPGMageAuthoring.h"
#include "RPG/RPGPriestAuthoring.h"
#include "RPG/RPGRangerAuthoring.h"
#include "RPG/RPGRogueAuthoring.h"
#include "RPG/RPGWarriorAuthoring.h"
#include "RPG/StatusEffects/GridStatusEffectDefinitionAsset.h"

namespace UIRPGCONTENTTEXT031
{
	enum class EStatusOwner : uint8
	{
		Warrior,
		Rogue,
		Ranger,
		Mage,
		Priest,
		Alchemist
	};

	struct FStatusAuditSpec
	{
		EStatusOwner Owner = EStatusOwner::Warrior;
		FName EffectId = NAME_None;
		FString ObjectPath;
	};

	const TCHAR* OwnerName(EStatusOwner Owner)
	{
		switch (Owner)
		{
			case EStatusOwner::Warrior: return TEXT("Warrior");
			case EStatusOwner::Rogue: return TEXT("Rogue");
			case EStatusOwner::Ranger: return TEXT("Ranger");
			case EStatusOwner::Mage: return TEXT("Mage");
			case EStatusOwner::Priest: return TEXT("Priest");
			case EStatusOwner::Alchemist: return TEXT("Alchemist");
			default: return TEXT("Unknown");
		}
	}

	void AppendSpecs(
		TArray<FStatusAuditSpec>& OutSpecs,
		EStatusOwner Owner,
		const TArray<FName>& EffectIds,
		TFunctionRef<FString(FName)> ResolvePath)
	{
		for (const FName EffectId : EffectIds)
		{
			FStatusAuditSpec Spec;
			Spec.Owner = Owner;
			Spec.EffectId = EffectId;
			Spec.ObjectPath = ResolvePath(EffectId);
			OutSpecs.Add(MoveTemp(Spec));
		}
	}

	void CollectStatusSpecs(TArray<FStatusAuditSpec>& OutSpecs)
	{
		OutSpecs.Reset();

		TArray<FName> EffectIds;

		FRPGWarriorAuthoring::GetRequiredStatusIds(EffectIds);
		AppendSpecs(OutSpecs, EStatusOwner::Warrior, EffectIds,
			[](FName EffectId) { return FRPGWarriorAuthoring::GetStatusObjectPath(EffectId); });

		EffectIds.Reset();
		FRPGRogueAuthoring::GetRequiredStatusIds(EffectIds);
		AppendSpecs(OutSpecs, EStatusOwner::Rogue, EffectIds,
			[](FName EffectId) { return FRPGRogueAuthoring::GetStatusObjectPath(EffectId); });

		FStatusAuditSpec Ranger;
		Ranger.Owner = EStatusOwner::Ranger;
		Ranger.EffectId = TEXT("Status_MarkedByRanger");
		Ranger.ObjectPath = FRPGRangerAuthoring::MarkedStatusPath();
		OutSpecs.Add(MoveTemp(Ranger));

		EffectIds.Reset();
		FRPGMageAuthoring::GetRequiredMageStatusIds(EffectIds);
		AppendSpecs(OutSpecs, EStatusOwner::Mage, EffectIds,
			[](FName EffectId) { return FRPGMageAuthoring::GetStatusObjectPath(EffectId); });

		EffectIds.Reset();
		FRPGPriestAuthoring::GetRequiredPriestStatusIds(EffectIds);
		AppendSpecs(OutSpecs, EStatusOwner::Priest, EffectIds,
			[](FName EffectId) { return FRPGPriestAuthoring::GetStatusObjectPath(EffectId); });

		TArray<FName> B1StatusIds;
		TArray<FName> B2StatusIds;
		FRPGAlchemistAuthoring::GetB1StatusIds(B1StatusIds);
		FRPGAlchemistAuthoring::GetB2StatusIds(B2StatusIds);
		B1StatusIds.Append(B2StatusIds);
		AppendSpecs(OutSpecs, EStatusOwner::Alchemist, B1StatusIds,
			[](FName EffectId) { return FRPGAlchemistAuthoring::GetStatusObjectPath(EffectId); });
	}

	bool ConfigureStatus(const FStatusAuditSpec& Spec, UGridStatusEffectDefinitionAsset& Status)
	{
		switch (Spec.Owner)
		{
			case EStatusOwner::Warrior:
				return FRPGWarriorAuthoring::ConfigureStatus(Status, Spec.EffectId);
			case EStatusOwner::Rogue:
				return FRPGRogueAuthoring::ConfigureStatus(Status, Spec.EffectId);
			case EStatusOwner::Ranger:
				return FRPGRangerAuthoring::ConfigureMarkedStatus(Status);
			case EStatusOwner::Mage:
				return FRPGMageAuthoring::ConfigureStatus(Status, Spec.EffectId);
			case EStatusOwner::Priest:
				return FRPGPriestAuthoring::ConfigureStatus(Status, Spec.EffectId);
			case EStatusOwner::Alchemist:
				return FRPGAlchemistAuthoring::ConfigureStatus(Status, Spec.EffectId);
			default:
				return false;
		}
	}

	void LogSerializedText(const FStatusAuditSpec& Spec, const UGridStatusEffectDefinitionAsset& Status)
	{
		UE_LOG(
			LogTemp,
			Display,
			TEXT("[UI-RPG-CONTENT-TEXT03.1][STATUS] Owner=%s EffectId=%s DisplayName=\"%s\" Description=\"%s\""),
			OwnerName(Spec.Owner),
			*Spec.EffectId.ToString(),
			*Status.DisplayName.ToString(),
			*Status.Description.ToString());
	}

	void LogOverwrite(
		const FStatusAuditSpec& Spec,
		const TCHAR* Field,
		const FText& Before,
		const FText& After)
	{
		UE_LOG(
			LogTemp,
			Display,
			TEXT("[UI-RPG-CONTENT-TEXT03.1][TEXT-OVERWRITE] Owner=%s EffectId=%s Field=%s Before=\"%s\" After=\"%s\""),
			OwnerName(Spec.Owner),
			*Spec.EffectId.ToString(),
			Field,
			*Before.ToString(),
			*After.ToString());
	}
}

/**
 * UI-RPG-CONTENT-TEXT03.1 is a characterization audit only.
 *
 * No package is saved and no production Data Asset is modified. Each
 * production DA_Status_* is loaded as a serialized text-authority candidate;
 * the matching class authoring function runs only against a transient
 * duplicate carrying sentinel text and deliberately poisoned mechanics.
 *
 * TEXT-OVERWRITE entries are informational. They identify any text field
 * reclaimed by C++ authoring; after preservation work they naturally disappear.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIRPGCONTENTTEXT031StatusTextOwnershipAuditTest,
	"Grimrock.UI.RPG.CONTENTTEXT03.StatusOwnershipAudit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPGCONTENTTEXT031StatusTextOwnershipAuditTest::RunTest(const FString&)
{
	using namespace UIRPGCONTENTTEXT031;

	TArray<FStatusAuditSpec> Specs;
	CollectStatusSpecs(Specs);

	TestEqual(TEXT("Status text audit covers the six RPG class authoring families"), Specs.Num(), 32);

	TSet<FName> UniqueEffectIds;
	int32 WarriorCount = 0;
	int32 RogueCount = 0;
	int32 RangerCount = 0;
	int32 MageCount = 0;
	int32 PriestCount = 0;
	int32 AlchemistCount = 0;

	for (const FStatusAuditSpec& Spec : Specs)
	{
		TestFalse(
			*FString::Printf(TEXT("%s/%s EffectId is non-empty"), OwnerName(Spec.Owner), *Spec.EffectId.ToString()),
			Spec.EffectId.IsNone());
		TestFalse(
			*FString::Printf(TEXT("%s appears only once in the status authoring ownership map"), *Spec.EffectId.ToString()),
			UniqueEffectIds.Contains(Spec.EffectId));
		UniqueEffectIds.Add(Spec.EffectId);

		switch (Spec.Owner)
		{
			case EStatusOwner::Warrior: ++WarriorCount; break;
			case EStatusOwner::Rogue: ++RogueCount; break;
			case EStatusOwner::Ranger: ++RangerCount; break;
			case EStatusOwner::Mage: ++MageCount; break;
			case EStatusOwner::Priest: ++PriestCount; break;
			case EStatusOwner::Alchemist: ++AlchemistCount; break;
		}
	}

	TestEqual(TEXT("Warrior owns five production statuses"), WarriorCount, 5);
	TestEqual(TEXT("Rogue owns nine production statuses"), RogueCount, 9);
	TestEqual(TEXT("Ranger owns one production status"), RangerCount, 1);
	TestEqual(TEXT("Mage owns four production statuses"), MageCount, 4);
	TestEqual(TEXT("Priest owns seven production statuses"), PriestCount, 7);
	TestEqual(TEXT("Alchemist owns six production statuses"), AlchemistCount, 6);

	int32 LoadedStatusCount = 0;
	int32 SerializedDisplayNameCount = 0;
	int32 SerializedDescriptionCount = 0;
	int32 DisplayNameOverwriteCount = 0;
	int32 DescriptionOverwriteCount = 0;
	int32 MechanicsRebuiltCount = 0;

	for (const FStatusAuditSpec& Spec : Specs)
	{
		UGridStatusEffectDefinitionAsset* Serialized =
			LoadObject<UGridStatusEffectDefinitionAsset>(nullptr, *Spec.ObjectPath);
		if (!TestNotNull(
				*FString::Printf(TEXT("%s/%s serialized DA_Status loads"), OwnerName(Spec.Owner), *Spec.EffectId.ToString()),
				Serialized))
		{
			continue;
		}

		++LoadedStatusCount;
		TestEqual(
			*FString::Printf(TEXT("%s keeps canonical EffectId"), *Spec.EffectId.ToString()),
			Serialized->EffectId,
			Spec.EffectId);
		TestTrue(
			*FString::Printf(TEXT("%s production status is structurally valid"), *Spec.EffectId.ToString()),
			Serialized->IsValidDefinition());

		if (!Serialized->DisplayName.IsEmpty())
		{
			++SerializedDisplayNameCount;
		}
		else
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[UI-RPG-CONTENT-TEXT03.1][DISPLAYNAME-EMPTY] Owner=%s EffectId=%s"),
				OwnerName(Spec.Owner),
				*Spec.EffectId.ToString());
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
				TEXT("[UI-RPG-CONTENT-TEXT03.1][DESCRIPTION-EMPTY] Owner=%s EffectId=%s"),
				OwnerName(Spec.Owner),
				*Spec.EffectId.ToString());
		}

		LogSerializedText(Spec, *Serialized);

		UGridStatusEffectDefinitionAsset* Working =
			DuplicateObject<UGridStatusEffectDefinitionAsset>(Serialized, GetTransientPackage());
		if (!TestNotNull(
				*FString::Printf(TEXT("%s transient audit duplicate is created"), *Spec.EffectId.ToString()),
				Working))
		{
			continue;
		}

		const FText SentinelDisplayName = FText::FromString(
			FString::Printf(TEXT("TEXT03.1 Status Name %s"), *Spec.EffectId.ToString()));
		const FText SentinelDescription = FText::FromString(
			FString::Printf(TEXT("TEXT03.1 Status Description %s"), *Spec.EffectId.ToString()));

		Working->DisplayName = SentinelDisplayName;
		Working->Description = SentinelDescription;
		Working->StatusTags = { TEXT("TEXT03_1_POISON_TAG") };
		Working->DefaultDuration = 777;
		Working->DefaultPotency = 777;
		Working->InitiativeModifier = 777;
		Working->CombatModifiers.Reset();
		Working->CombatReactions.Reset();

		if (!TestTrue(
				*FString::Printf(TEXT("%s current class authoring rematerializes a transient status duplicate"), *Spec.EffectId.ToString()),
				ConfigureStatus(Spec, *Working)))
		{
			continue;
		}

		const bool bMechanicsRebuilt =
			Working->DefaultDuration != 777 &&
			Working->DefaultPotency != 777 &&
			Working->InitiativeModifier != 777 &&
			!Working->StatusTags.Contains(TEXT("TEXT03_1_POISON_TAG"));
		TestTrue(
			*FString::Printf(TEXT("%s status mechanics were actually rebuilt"), *Spec.EffectId.ToString()),
			bMechanicsRebuilt);
		if (bMechanicsRebuilt)
		{
			++MechanicsRebuiltCount;
		}

		if (!SentinelDisplayName.EqualTo(Working->DisplayName))
		{
			++DisplayNameOverwriteCount;
			LogOverwrite(Spec, TEXT("Status.DisplayName"), SentinelDisplayName, Working->DisplayName);
		}
		if (!SentinelDescription.EqualTo(Working->Description))
		{
			++DescriptionOverwriteCount;
			LogOverwrite(Spec, TEXT("Status.Description"), SentinelDescription, Working->Description);
		}
	}

	TestEqual(TEXT("Audit loads all thirty-two production DA_Status assets"), LoadedStatusCount, 32);
	TestEqual(TEXT("Every production status has a serialized DisplayName"), SerializedDisplayNameCount, 32);
	TestEqual(TEXT("Every audited status has its mechanics rematerialized"), MechanicsRebuiltCount, 32);

	UE_LOG(
		LogTemp,
		Display,
		TEXT("[UI-RPG-CONTENT-TEXT03.1][SUMMARY] Statuses=%d SerializedDisplayNames=%d SerializedDescriptions=%d DisplayNameOverwrites=%d DescriptionOverwrites=%d Warrior=%d Rogue=%d Ranger=%d Mage=%d Priest=%d Alchemist=%d"),
		LoadedStatusCount,
		SerializedDisplayNameCount,
		SerializedDescriptionCount,
		DisplayNameOverwriteCount,
		DescriptionOverwriteCount,
		WarriorCount,
		RogueCount,
		RangerCount,
		MageCount,
		PriestCount,
		AlchemistCount);

	return true;
}


/**
 * UI-RPG-CONTENT-TEXT03.2 proves that all six class authoring families
 * rematerialize status mechanics without reclaiming serialized status text.
 *
 * DisplayName must remain non-empty because UGridStatusEffectDefinitionAsset
 * requires it for structural validity. Description may legitimately be empty,
 * so a second probe verifies that an empty serialized description remains
 * authoritative.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIRPGCONTENTTEXT032StatusTextPreservationTest,
	"Grimrock.UI.RPG.CONTENTTEXT03.Preservation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPGCONTENTTEXT032StatusTextPreservationTest::RunTest(const FString&)
{
	using namespace UIRPGCONTENTTEXT031;

	TArray<FStatusAuditSpec> Specs;
	CollectStatusSpecs(Specs);

	int32 PreservedStatusCount = 0;
	int32 EmptyDescriptionCount = 0;

	for (const FStatusAuditSpec& Spec : Specs)
	{
		UGridStatusEffectDefinitionAsset* Serialized =
			LoadObject<UGridStatusEffectDefinitionAsset>(nullptr, *Spec.ObjectPath);
		if (!TestNotNull(
				*FString::Printf(TEXT("%s/%s serialized DA_Status loads for preservation test"), OwnerName(Spec.Owner), *Spec.EffectId.ToString()),
				Serialized))
		{
			continue;
		}

		UGridStatusEffectDefinitionAsset* Working =
			DuplicateObject<UGridStatusEffectDefinitionAsset>(Serialized, GetTransientPackage());
		if (!TestNotNull(
				*FString::Printf(TEXT("%s preservation duplicate is created"), *Spec.EffectId.ToString()),
				Working))
		{
			continue;
		}

		const FString DisplayName =
			FString::Printf(TEXT("TEXT03.2 Status Name %s"), *Spec.EffectId.ToString());
		const FString Description =
			FString::Printf(TEXT("TEXT03.2 Status Description %s"), *Spec.EffectId.ToString());

		Working->DisplayName = FText::FromString(DisplayName);
		Working->Description = FText::FromString(Description);
		Working->StatusTags = { TEXT("TEXT03_2_POISON_TAG") };
		Working->DefaultDuration = 777;
		Working->DefaultPotency = 777;
		Working->InitiativeModifier = 777;
		Working->CombatModifiers.Reset();
		Working->CombatReactions.Reset();

		if (!TestTrue(
				*FString::Printf(TEXT("%s rematerializes mechanics while preserving serialized text"), *Spec.EffectId.ToString()),
				ConfigureStatus(Spec, *Working)))
		{
			continue;
		}

		TestEqual(
			*FString::Printf(TEXT("%s preserves serialized DisplayName"), *Spec.EffectId.ToString()),
			Working->DisplayName.ToString(),
			DisplayName);
		TestEqual(
			*FString::Printf(TEXT("%s preserves serialized Description"), *Spec.EffectId.ToString()),
			Working->Description.ToString(),
			Description);
		TestTrue(
			*FString::Printf(TEXT("%s rematerializes DefaultDuration"), *Spec.EffectId.ToString()),
			Working->DefaultDuration != 777);
		TestTrue(
			*FString::Printf(TEXT("%s rematerializes DefaultPotency"), *Spec.EffectId.ToString()),
			Working->DefaultPotency != 777);
		TestTrue(
			*FString::Printf(TEXT("%s rematerializes InitiativeModifier"), *Spec.EffectId.ToString()),
			Working->InitiativeModifier != 777);
		TestFalse(
			*FString::Printf(TEXT("%s rematerializes StatusTags"), *Spec.EffectId.ToString()),
			Working->StatusTags.Contains(TEXT("TEXT03_2_POISON_TAG")));
		TestTrue(
			*FString::Printf(TEXT("%s remains structurally valid after text restoration"), *Spec.EffectId.ToString()),
			Working->IsValidDefinition());

		UGridStatusEffectDefinitionAsset* EmptyDescriptionProbe =
			DuplicateObject<UGridStatusEffectDefinitionAsset>(Serialized, GetTransientPackage());
		if (!TestNotNull(
				*FString::Printf(TEXT("%s empty-description duplicate is created"), *Spec.EffectId.ToString()),
				EmptyDescriptionProbe))
		{
			continue;
		}

		const FString EmptyProbeName =
			FString::Printf(TEXT("TEXT03.2 Empty Description Name %s"), *Spec.EffectId.ToString());
		EmptyDescriptionProbe->DisplayName = FText::FromString(EmptyProbeName);
		EmptyDescriptionProbe->Description = FText::GetEmpty();
		EmptyDescriptionProbe->DefaultDuration = 777;

		if (!TestTrue(
				*FString::Printf(TEXT("%s rematerializes with an empty serialized Description"), *Spec.EffectId.ToString()),
				ConfigureStatus(Spec, *EmptyDescriptionProbe)))
		{
			continue;
		}

		TestEqual(
			*FString::Printf(TEXT("%s preserves DisplayName while Description is empty"), *Spec.EffectId.ToString()),
			EmptyDescriptionProbe->DisplayName.ToString(),
			EmptyProbeName);
		TestTrue(
			*FString::Printf(TEXT("%s preserves an empty serialized Description"), *Spec.EffectId.ToString()),
			EmptyDescriptionProbe->Description.IsEmpty());
		TestTrue(
			*FString::Printf(TEXT("%s still rebuilds mechanics with empty Description"), *Spec.EffectId.ToString()),
			EmptyDescriptionProbe->DefaultDuration != 777);
		TestTrue(
			*FString::Printf(TEXT("%s remains valid with empty Description"), *Spec.EffectId.ToString()),
			EmptyDescriptionProbe->IsValidDefinition());

		++PreservedStatusCount;
		++EmptyDescriptionCount;
	}

	TestEqual(TEXT("Preservation covers all thirty-two production statuses"), PreservedStatusCount, 32);
	TestEqual(TEXT("Empty Description preservation covers all thirty-two production statuses"), EmptyDescriptionCount, 32);

	UE_LOG(
		LogTemp,
		Display,
		TEXT("[UI-RPG-CONTENT-TEXT03.2][SUMMARY] PreservedStatuses=%d PreservedTextFields=%d EmptyDescriptionsPreserved=%d"),
		PreservedStatusCount,
		PreservedStatusCount * 2,
		EmptyDescriptionCount);

	return true;
}

#endif
