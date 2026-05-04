#include "FullScreenPassModule.h"
#include "FullScreenPassLog.h"
#include "FullScreenPassSceneViewExtension.h"

#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "SceneViewExtension.h"
#include "ShaderCore.h"
#include "Misc/CoreDelegates.h"

#define LOCTEXT_NAMESPACE "FFullScreenPassModule"

void FFullScreenPassModule::StartupModule()
{
    UE_LOG(FullScreenPass, Log, TEXT("FFullScreenPassModule startup"));

    TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("FullScreenPass"));
    if (!Plugin.IsValid())
    {
        UE_LOG(FullScreenPass, Error, TEXT("FullScreenPass plugin was not found by PluginManager"));
        return;
    }

    const FString PluginShaderDir = FPaths::Combine(Plugin->GetBaseDir(), TEXT("Shaders"));
    AddShaderSourceDirectoryMapping(TEXT("/FullScreenPass"), PluginShaderDir);

    UE_LOG(FullScreenPass, Log, TEXT("FullScreenPass shader directory mapped to: %s"), *PluginShaderDir);

    PostEngineInitHandle = FCoreDelegates::OnPostEngineInit.AddRaw(this, &FFullScreenPassModule::CreateSceneViewExtension);
}

void FFullScreenPassModule::CreateSceneViewExtension()
{
    if (ViewExtension.IsValid())
    {
        return;
    }

    UE_LOG(FullScreenPass, Warning, TEXT("Creating FullScreenPass SceneViewExtension"));

    ViewExtension = FSceneViewExtensions::NewExtension<FFullScreenPassSceneViewExtension>();

    if (ViewExtension.IsValid())
    {
        UE_LOG(FullScreenPass, Warning, TEXT("FullScreenPass SceneViewExtension created"));
    }
    else
    {
        UE_LOG(FullScreenPass, Error, TEXT("Failed to create FullScreenPass SceneViewExtension"));
    }
}

void FFullScreenPassModule::ShutdownModule()
{
    UE_LOG(FullScreenPass, Log, TEXT("FFullScreenPassModule shutdown"));

    if (PostEngineInitHandle.IsValid())
    {
        FCoreDelegates::OnPostEngineInit.Remove(PostEngineInitHandle);
        PostEngineInitHandle.Reset();
    }

    ViewExtension.Reset();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FFullScreenPassModule, FullScreenPass);
