#pragma once

#include <d3dcompiler.h>

ID3DBlob* CompileShader(const char* szProfile, const char* szEntryPoint, const char* szShaderCode);