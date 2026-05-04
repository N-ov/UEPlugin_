#pragma once

#include "Modules/ModuleManager.h"
#include "Templates/SharedPointer.h"

class FFullScreenPassSceneViewExtension;

class FFullScreenPassModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:
    void CreateSceneViewExtension();

private:
    TSharedPtr<FFullScreenPassSceneViewExtension, ESPMode::ThreadSafe> ViewExtension;
    FDelegateHandle PostEngineInitHandle;
};
