#pragma once

#include "CoreMinimal.h"
#include "GlobalShader.h"
#include "RenderGraphResources.h"
#include "ShaderParameterStruct.h"

class FFullScreenPassPS : public FGlobalShader
{
    DECLARE_GLOBAL_SHADER(FFullScreenPassPS);
    SHADER_USE_PARAMETER_STRUCT(FFullScreenPassPS, FGlobalShader);

public:
    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)

        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, InputTexture)
        SHADER_PARAMETER_SAMPLER(SamplerState, InputSampler)

        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, BlurredTexture)
        SHADER_PARAMETER_SAMPLER(SamplerState, BlurredSampler)

        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SceneDepthTexture)
        SHADER_PARAMETER_SAMPLER(SamplerState, DepthSampler)

        SHADER_PARAMETER(FVector2f, InvBufferSize)

        SHADER_PARAMETER(float, EffectStrength)
        SHADER_PARAMETER(float, BlurStart)
        SHADER_PARAMETER(float, FogStart)
        SHADER_PARAMETER(float, FogFactor)
        SHADER_PARAMETER(FVector3f, FogColor)

        SHADER_PARAMETER(int32, BlurRadius)
        SHADER_PARAMETER(int32, PassIndex)

        RENDER_TARGET_BINDING_SLOTS()
    END_SHADER_PARAMETER_STRUCT()

    static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
    {
        return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
    }
};
