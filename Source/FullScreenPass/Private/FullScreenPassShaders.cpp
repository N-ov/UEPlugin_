#include "FullScreenPassShaders.h"

IMPLEMENT_GLOBAL_SHADER(
    FFullScreenPassPS,
    "/FullScreenPass/FullScreenShader.usf",
    "MainPS",
    SF_Pixel
);
