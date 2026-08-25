#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Animation/AnimClassInterface.h"
#include "Animation/AnimInstance.h"
#include "Animation/Skeleton.h"
#include "Engine/SkeletalMesh.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterTypes.h"
#include "UObject/UnrealType.h"

namespace GridMonsterMON178
{
	struct FPresentationPair
	{
		const TCHAR* Label = TEXT("");
		const TCHAR* MeshPath = TEXT("");
		const TCHAR* AnimClassPath = TEXT("");
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON178BestiaryPresentationBridgeTest, "Grimrock.Monsters.MON17.8.BestiaryPresentationBridge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON178BestiaryPresentationBridgeTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const GridMonsterMON178::FPresentationPair Pairs[] = { { TEXT("RatGiant"), TEXT("/Game/GrimrockPrototype/Monsters/RatGiant/Meshes/SK_RatGiant.SK_RatGiant"),
															   TEXT(
																   "/Game/GrimrockPrototype/Monsters/RatGiant/Animation/ABP_MON_RatGiant.ABP_MON_RatGiant_C") },
		{ TEXT("GoblinThrower"), TEXT("/Game/GrimrockPrototype/Monsters/GoblinThrower/Meshes/SK_GoblinThrower.SK_GoblinThrower"),
			TEXT("/Game/GrimrockPrototype/Monsters/GoblinThrower/Animation/ABP_MON_GoblinThrower.ABP_MON_GoblinThrower_C") } };

	int32 ValidatedPairs = 0;
	for (const GridMonsterMON178::FPresentationPair& Pair : Pairs)
	{
		USkeletalMesh* SkeletalMesh = LoadObject<USkeletalMesh>(nullptr, Pair.MeshPath);
		UClass* AnimationClass = LoadClass<UAnimInstance>(nullptr, Pair.AnimClassPath);

		TestNotNull(FString::Printf(TEXT("%s skeletal mesh loads"), Pair.Label), SkeletalMesh);
		TestNotNull(FString::Printf(TEXT("%s Animation Blueprint class loads"), Pair.Label), AnimationClass);
		if (!SkeletalMesh || !AnimationClass)
		{
			continue;
		}

		USkeleton* MeshSkeleton = SkeletalMesh->GetSkeleton();
		TestNotNull(FString::Printf(TEXT("%s mesh owns a Skeleton"), Pair.Label), MeshSkeleton);

		TestTrue(FString::Printf(TEXT("%s AnimBP derives from UGridMonsterAnimInstance"), Pair.Label),
			AnimationClass->IsChildOf(UGridMonsterAnimInstance::StaticClass()));

		const IAnimClassInterface* AnimClassInterface = IAnimClassInterface::GetFromClass(AnimationClass);
		TestTrue(FString::Printf(TEXT("%s AnimBP exposes IAnimClassInterface"), Pair.Label), AnimClassInterface != nullptr);

		USkeleton* AnimationSkeleton = AnimClassInterface ? AnimClassInterface->GetTargetSkeleton() : nullptr;
		TestNotNull(FString::Printf(TEXT("%s AnimBP targets a Skeleton"), Pair.Label), AnimationSkeleton);

		if (!MeshSkeleton || !AnimationSkeleton)
		{
			continue;
		}

		const bool bCompatibleSkeletons =
			MeshSkeleton == AnimationSkeleton || MeshSkeleton->IsCompatible(AnimationSkeleton) || AnimationSkeleton->IsCompatible(MeshSkeleton);
		TestTrue(FString::Printf(TEXT("%s mesh and AnimBP Skeletons are compatible"), Pair.Label), bCompatibleSkeletons);

		if (bCompatibleSkeletons && AnimationClass->IsChildOf(UGridMonsterAnimInstance::StaticClass()))
		{
			++ValidatedPairs;
		}
	}

	TestEqual(TEXT("RatGiant and GoblinThrower share the generic monster animation bridge"), ValidatedPairs, 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON178AnimationStateBridgeContractTest, "Grimrock.Monsters.MON17.8.AnimationStateBridgeContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON178AnimationStateBridgeContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UClass* AnimInstanceClass = UGridMonsterAnimInstance::StaticClass();
	TestNotNull(TEXT("Generic monster AnimInstance class exists"), AnimInstanceClass);
	if (!AnimInstanceClass)
	{
		return false;
	}

	const FName RequiredProperties[] = { GET_MEMBER_NAME_CHECKED(UGridMonsterAnimInstance, MonsterState),
		GET_MEMBER_NAME_CHECKED(UGridMonsterAnimInstance, bIsMoving), GET_MEMBER_NAME_CHECKED(UGridMonsterAnimInstance, bIsTurning),
		GET_MEMBER_NAME_CHECKED(UGridMonsterAnimInstance, bIsDead), GET_MEMBER_NAME_CHECKED(UGridMonsterAnimInstance, MoveAlpha),
		GET_MEMBER_NAME_CHECKED(UGridMonsterAnimInstance, TurnDirection), GET_MEMBER_NAME_CHECKED(UGridMonsterAnimInstance, CurrentHealth),
		GET_MEMBER_NAME_CHECKED(UGridMonsterAnimInstance, MaxHealth), GET_MEMBER_NAME_CHECKED(UGridMonsterAnimInstance, CurrentCell),
		GET_MEMBER_NAME_CHECKED(UGridMonsterAnimInstance, Facing) };

	for (const FName PropertyName : RequiredProperties)
	{
		const FProperty* Property = FindFProperty<FProperty>(AnimInstanceClass, PropertyName);
		TestNotNull(FString::Printf(TEXT("AnimInstance exposes %s"), *PropertyName.ToString()), Property);
	}

	const UEnum* MonsterStateEnum = StaticEnum<EGridMonsterState>();
	TestNotNull(TEXT("EGridMonsterState reflection enum exists"), MonsterStateEnum);
	if (!MonsterStateEnum)
	{
		return false;
	}

	const EGridMonsterState RequiredStates[] = { EGridMonsterState::Dormant, EGridMonsterState::Idle, EGridMonsterState::Alert, EGridMonsterState::Pursuing,
		EGridMonsterState::Attacking, EGridMonsterState::Repositioning, EGridMonsterState::Hurt, EGridMonsterState::Dead };

	for (const EGridMonsterState State : RequiredStates)
	{
		TestTrue(FString::Printf(TEXT("Monster presentation state %s remains reflected"), *UEnum::GetValueAsString(State)),
			MonsterStateEnum->IsValidEnumValue(static_cast<int64>(State)));
	}

	return true;
}

#endif
