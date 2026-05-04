#pragma once

#include "SceneViewExtension.h"

class FFullScreenPassSceneViewExtension : public FSceneViewExtensionBase
{
public:
    FFullScreenPassSceneViewExtension(const FAutoRegister& AutoRegister);

    virtual void PrePostProcessPass_RenderThread(
        FRDGBuilder& GraphBuilder,
        const FSceneView& View,
        const FPostProcessingInputs& Inputs
    ) override;
};
