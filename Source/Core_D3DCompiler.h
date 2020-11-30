#pragma once

#include <d3dcompiler.h>

ID3DBlob *CompileShader(const char *profile, const char *entrypoint,
                        const char *shader);