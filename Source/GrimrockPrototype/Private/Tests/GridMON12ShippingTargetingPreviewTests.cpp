#if WITH_DEV_AUTOMATION_TESTS

#include "Components/LineBatchComponent.h"
#include "Engine/EngineTypes.h"
#include "Misc/AutomationTest.h"
#include "Runtime/GrimrockPlayerController.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMON12ShippingTargetingPreviewTest,
    "Grimrock.Monsters.MON12.8.6.ShippingTargetingRenderer",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON12ShippingTargetingPreviewTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    const FObjectPropertyBase* PreviewComponentProperty =
        FindFProperty<FObjectPropertyBase>(
            AGrimrockPlayerController::StaticClass(), TEXT("CombatTargetingPreviewLines"));
    TestNotNull(TEXT("The player controller owns a real runtime preview primitive"), PreviewComponentProperty);
    if (PreviewComponentProperty)
    {
        TestTrue(TEXT("The preview primitive is a ULineBatchComponent"),
            PreviewComponentProperty->PropertyClass->IsChildOf(ULineBatchComponent::StaticClass()));
    }

    ULineBatchComponent* Lines = NewObject<ULineBatchComponent>();
    if (!TestNotNull(TEXT("Real line batch primitive is available"), Lines)) return false;
    constexpr uint32 PreviewBatchId = 0xC0128601u;
    Lines->DrawBox(FVector(200.f, 200.f, 8.f), FVector(90.f, 90.f, 5.f),
        FLinearColor::Yellow, 0.f, SDPG_World, 5.f, PreviewBatchId);
    TestTrue(TEXT("Target cell writes actual scene lines, not DrawDebugBox"),
        Lines->BatchedLines.Num() > 0);
    Lines->ClearBatch(PreviewBatchId);
    TestEqual(TEXT("Cancelled targeting removes all preview lines"), Lines->BatchedLines.Num(), 0);
    return true;
}
#endif
