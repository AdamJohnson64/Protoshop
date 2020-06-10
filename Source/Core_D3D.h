#pragma once

#define RENDERTARGET_WIDTH 640
#define RENDERTARGET_HEIGHT 480

#define TRYD3D(FN) if (S_OK != (FN)) throw #FN;