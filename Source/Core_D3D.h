#pragma once

#define TRYD3D(FN)                                                             \
  if (S_OK != (FN))                                                            \
    throw #FN;