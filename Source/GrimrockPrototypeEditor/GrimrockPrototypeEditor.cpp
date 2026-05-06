#include "Modules/ModuleManager.h"

class FGrimrockPrototypeEditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
    }

    virtual void ShutdownModule() override
    {
    }
};

IMPLEMENT_MODULE(FGrimrockPrototypeEditorModule, GrimrockPrototypeEditor)
