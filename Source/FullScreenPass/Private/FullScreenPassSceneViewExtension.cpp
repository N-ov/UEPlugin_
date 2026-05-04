#include "FullScreenPassSceneViewExtension.h"
#include "PixelShaderUtils.h"
#include "FullScreenPassLog.h"
#include "FullScreenPassShaders.h"

#include "FXRenderingUtils.h"
#include "PostProcess/PostProcessInputs.h"
#include "RenderGraphUtils.h"
#include "ScreenPass.h"
#include "RHIStaticStates.h"

static TAutoConsoleVariable<int32> CVarFSPEnabled(
    TEXT("r.FSP"),
    1,
    TEXT("Controls FullScreenPass DepthHaze effect.\n")
    TEXT("0: disabled\n")
    TEXT("1: enabled"),
    ECVF_RenderThreadSafe
);

static TAutoConsoleVariable<float> CVarFSPEffectStrength(
    TEXT("r.FSP.EffectStrength"),
    0.9f,
    TEXT("Depth haze blur strength. Original ReShade parameter: EffectStrength."),
    ECVF_RenderThreadSafe
);

static TAutoConsoleVariable<float> CVarFSPFogStart(
    TEXT("r.FSP.FogStart"),
    0.2f,
    TEXT("Depth where fog starts. Original ReShade parameter: FogStart."),
    ECVF_RenderThreadSafe
);

static TAutoConsoleVariable<float> CVarFSPFogFactor(
    TEXT("r.FSP.FogFactor"),
    0.2f,
    TEXT("Fog intensity. Original ReShade parameter: FogFactor."),
    ECVF_RenderThreadSafe
);

static TAutoConsoleVariable<float> CVarFSPFogColorR(
    TEXT("r.FSP.FogColorR"),
    0.8f,
    TEXT("Fog color red channel."),
    ECVF_RenderThreadSafe
);

static TAutoConsoleVariable<float> CVarFSPFogColorG(
    TEXT("r.FSP.FogColorG"),
    0.8f,
    TEXT("Fog color green channel."),
    ECVF_RenderThreadSafe
);

static TAutoConsoleVariable<float> CVarFSPFogColorB(
    TEXT("r.FSP.FogColorB"),
    0.8f,
    TEXT("Fog color blue channel."),
    ECVF_RenderThreadSafe
);

static TAutoConsoleVariable<int32> CVarFSPBlurRadius(
    TEXT("r.FSP.BlurRadius"),
    4,
    TEXT("Blur radius. Original ReShade effect uses 4 samples per side."),
    ECVF_RenderThreadSafe
);

static TAutoConsoleVariable<float> CVarFSPBlurStart(
    TEXT("r.FSP.BlurStart"),
    0.35f,
    TEXT("Depth where blur starts. Higher value means blur starts farther from camera."),
    ECVF_RenderThreadSafe
);

FFullScreenPassSceneViewExtension::FFullScreenPassSceneViewExtension(const FAutoRegister& AutoRegister)
    : FSceneViewExtensionBase(AutoRegister)
{
    UE_LOG(FullScreenPass, Warning, TEXT("FullScreenPass SceneViewExtension constructed"));
}

void FFullScreenPassSceneViewExtension::PrePostProcessPass_RenderThread(
    FRDGBuilder& GraphBuilder,
    const FSceneView& View,
    const FPostProcessingInputs& Inputs)
{
    static bool bLoggedRenderPass = false;
    if (!bLoggedRenderPass)
    {
        UE_LOG(FullScreenPass, Warning, TEXT("FullScreenPass DepthHaze render pass is running"));
        bLoggedRenderPass = true;
    }

    if (CVarFSPEnabled.GetValueOnRenderThread() == 0)
    {
        return;
    }

    Inputs.Validate();

    if (!Inputs.SceneTextures)
    {
        return;
    }

    const FIntRect PrimaryViewRect = UE::FXRenderingUtils::GetRawViewRectUnsafe(View);

    FRDGTextureRef SceneColorTexture = (*Inputs.SceneTextures)->SceneColorTexture;
    FRDGTextureRef SceneDepthTexture = (*Inputs.SceneTextures)->SceneDepthTexture;

    if (!SceneColorTexture || !SceneDepthTexture)
    {
        return;
    }

    FScreenPassTexture SceneColor(SceneColorTexture, PrimaryViewRect);

    if (!SceneColor.IsValid())
    {
        return;
    }

    const FScreenPassTextureViewport Viewport(SceneColor);

    FRDGTextureDesc PassTextureDesc = SceneColor.Texture->Desc;
    PassTextureDesc.Flags |= TexCreate_RenderTargetable;
    PassTextureDesc.Flags |= TexCreate_ShaderResource;

    FRDGTextureRef BlurXTexture = GraphBuilder.CreateTexture(
        PassTextureDesc,
        TEXT("FullScreenPass_DepthHaze_BlurX")
    );

    FRDGTextureRef BlurYTexture = GraphBuilder.CreateTexture(
        PassTextureDesc,
        TEXT("FullScreenPass_DepthHaze_BlurY")
    );

    FRDGTextureRef ResultTexture = GraphBuilder.CreateTexture(
        PassTextureDesc,
        TEXT("FullScreenPass_DepthHaze_Result")
    );

    FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);

    TShaderMapRef<FFullScreenPassPS> ScreenPassPS(GlobalShaderMap);

    const FIntPoint BufferSize = SceneColor.Texture->Desc.Extent;

    const FVector2f InvBufferSize(
        BufferSize.X > 0 ? 1.0f / static_cast<float>(BufferSize.X) : 0.0f,
        BufferSize.Y > 0 ? 1.0f / static_cast<float>(BufferSize.Y) : 0.0f
    );

    const float EffectStrength = FMath::Clamp(CVarFSPEffectStrength.GetValueOnRenderThread(), 0.0f, 1.0f);
    const float BlurStart = FMath::Clamp(CVarFSPBlurStart.GetValueOnRenderThread(), 0.0f, 1.0f);
    const float FogStart = FMath::Clamp(CVarFSPFogStart.GetValueOnRenderThread(), 0.0f, 1.0f);
    const float FogFactor = FMath::Clamp(CVarFSPFogFactor.GetValueOnRenderThread(), 0.0f, 1.0f);

    const FVector3f FogColor(
        FMath::Clamp(CVarFSPFogColorR.GetValueOnRenderThread(), 0.0f, 1.0f),
        FMath::Clamp(CVarFSPFogColorG.GetValueOnRenderThread(), 0.0f, 1.0f),
        FMath::Clamp(CVarFSPFogColorB.GetValueOnRenderThread(), 0.0f, 1.0f)
    );

    const int32 BlurRadius = FMath::Clamp(CVarFSPBlurRadius.GetValueOnRenderThread(), 1, 16);

    auto AddDepthHazePass = [&](
        const TCHAR* PassName,
        FRDGTextureRef InputTexture,
        FRDGTextureRef BlurredTexture,
        FRDGTextureRef OutputTexture,
        int32 PassIndex)
    {
        FFullScreenPassPS::FParameters* Parameters =
            GraphBuilder.AllocParameters<FFullScreenPassPS::FParameters>();

        Parameters->View = View.ViewUniformBuffer;

        Parameters->InputTexture = InputTexture;
        Parameters->InputSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();

        Parameters->BlurredTexture = BlurredTexture;
        Parameters->BlurredSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();

        Parameters->SceneDepthTexture = SceneDepthTexture;
        Parameters->DepthSampler = TStaticSamplerState<SF_Point, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();

        Parameters->InvBufferSize = InvBufferSize;

        Parameters->EffectStrength = EffectStrength;
        Parameters->BlurStart = BlurStart;
        Parameters->FogStart = FogStart;
        Parameters->FogFactor = FogFactor;
        Parameters->FogColor = FogColor;
        Parameters->BlurRadius = BlurRadius;
        Parameters->PassIndex = PassIndex;

        FScreenPassRenderTarget OutputRenderTarget(
            OutputTexture,
            SceneColor.ViewRect,
            ERenderTargetLoadAction::EClear
        );

        Parameters->RenderTargets[0] = OutputRenderTarget.GetRenderTargetBinding();

        FPixelShaderUtils::AddFullscreenPass(
            GraphBuilder,
            GlobalShaderMap,
            RDG_EVENT_NAME("FullScreenPass_DepthHaze"),
            ScreenPassPS,
            Parameters,
            SceneColor.ViewRect
        );
    };

    // Original ReShade structure:
    // Pass 0: horizontal blur.
    // Pass 1: vertical blur.
    // Pass 2: blend blurred image with normal scene color using depth and fog.
    AddDepthHazePass(
        TEXT("HorizontalBlur"),
        SceneColor.Texture,
        SceneColor.Texture,
        BlurXTexture,
        0
    );

    AddDepthHazePass(
        TEXT("VerticalBlur"),
        BlurXTexture,
        SceneColor.Texture,
        BlurYTexture,
        1
    );

    AddDepthHazePass(
        TEXT("BlendDepthHaze"),
        SceneColor.Texture,
        BlurYTexture,
        ResultTexture,
        2
    );

    AddCopyTexturePass(GraphBuilder, ResultTexture, SceneColor.Texture);
}
