#pragma once

#include "Core_IImage.h"

////////////////////////////////////////////////////////////////////////////////
// TGA Loader
// A very junky TGA loader that barely supports the standard.
////////////////////////////////////////////////////////////////////////////////
ImageOwned Load_TGA(const char *filename);