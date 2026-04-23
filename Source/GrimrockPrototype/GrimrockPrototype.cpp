#include "Modules/ModuleManager.h"

#if WITH_EDITOR
#include "EditorModeRegistry.h"
#include "EditorTools/GridLevelEdMode.h"
#endif

class FGrimrockPrototypeModule : public FDefaultGameModuleImpl
{
public:
    virtual void StartupModule () override
    {
        FDefaultGameModuleImpl::StartupModule ();

#if WITH_EDITOR
        FEditorModeRegistry::Get ().RegisterMode<FGridLevelEdMode> (
            FGridLevelEdMode::EM_GridLevelEdModeId,
            FText::FromString (TEXT ("Grimrock Grid Editor")),
            FSlateIcon (),
            true);
#endif
    }

    virtual void ShutdownModule () override
    {
#if WITH_EDITOR
        if (FModuleManager::Get ().IsModuleLoaded ("UnrealEd"))
        {
            FEditorModeRegistry::Get ().UnregisterMode (FGridLevelEdMode::EM_GridLevelEdModeId);
        }
#endif

        FDefaultGameModuleImpl::ShutdownModule ();
    }
};

IMPLEMENT_PRIMARY_GAME_MODULE (FGrimrockPrototypeModule, GrimrockPrototype, "GrimrockPrototype");