#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Animation/AnimClassInterface.h"
#include "Animation/AnimInstance.h"
#include "Animation/Skeleton.h"
#include "Engine/SkeletalMesh.h"
#include "Runtime/Monsters/GridMonsterActor.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMonsterMON172PresentationBridgeContractTest,
    "Grimrock.Monsters.MON17.2.PresentationBridgeContract",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON172PresentationBridgeContractTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;

    USkeletalMesh* SkeletalMesh = LoadObject<USkeletalMesh> (
        nullptr,
        TEXT ("/Game/GrimrockPrototype/Monsters/RatGiant/Meshes/SK_RatGiant.SK_RatGiant"));
    UClass* AnimationClass = LoadClass<UAnimInstance> (
        nullptr,
        TEXT ("/Game/GrimrockPrototype/Monsters/RatGiant/Animation/ABP_MON_RatGiant.ABP_MON_RatGiant_C"));

    TestNotNull (
        TEXT ("Known-good monster skeletal mesh loads"),
        SkeletalMesh);
    TestNotNull (
        TEXT ("Known-good monster Animation Blueprint class loads"),
        AnimationClass);
    if (!SkeletalMesh || !AnimationClass)
    {
        return false;
    }

    USkeleton* MeshSkeleton = SkeletalMesh->GetSkeleton ();
    TestNotNull (
        TEXT ("Monster skeletal mesh owns a Skeleton"),
        MeshSkeleton);

    TestTrue (
        TEXT ("Monster Animation Blueprint derives from UGridMonsterAnimInstance"),
        AnimationClass->IsChildOf (
            UGridMonsterAnimInstance::StaticClass ()));

    const IAnimClassInterface* AnimClassInterface =
        IAnimClassInterface::GetFromClass (AnimationClass);
    TestTrue (
        TEXT ("Monster Animation Blueprint exposes IAnimClassInterface"),
        AnimClassInterface != nullptr);

    USkeleton* AnimationSkeleton = AnimClassInterface
        ? AnimClassInterface->GetTargetSkeleton ()
        : nullptr;
    TestNotNull (
        TEXT ("Monster Animation Blueprint targets a Skeleton"),
        AnimationSkeleton);

    if (!MeshSkeleton || !AnimationSkeleton)
    {
        return false;
    }

    const bool bCompatibleSkeletons =
        MeshSkeleton == AnimationSkeleton ||
        MeshSkeleton->IsCompatible (AnimationSkeleton) ||
        AnimationSkeleton->IsCompatible (MeshSkeleton);
    TestTrue (
        TEXT ("Monster mesh Skeleton and Animation Blueprint Skeleton are compatible"),
        bCompatibleSkeletons);

    return true;
}

#endif
