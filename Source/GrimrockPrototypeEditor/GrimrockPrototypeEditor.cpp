#include "Modules/ModuleManager.h"
#include "EditorModeRegistry.h"
#include "EditorTools/GridLevelEdMode.h"

class FGrimrockPrototypeEditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        FEditorModeRegistry::Get().RegisterMode<FGridLevelEdMode>(
            FGridLevelEdMode::EM_GridLevelEdModeId,
            FText::FromString(TEXT("Grimrock Grid Editor")),
            FSlateIcon(),
            true);
    }

    virtual void ShutdownModule() override
    {
        if (FModuleManager::Get().IsModuleLoaded("UnrealEd"))
        {
            FEditorModeRegistry::Get().UnregisterMode(FGridLevelEdMode::EM_GridLevelEdModeId);
        }
    }
};

IMPLEMENT_MODULE(FGrimrockPrototypeEditorModule, GrimrockPrototypeEditor)
