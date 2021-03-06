#ifndef Consts_h
#define Consts_h

#define WINDOW_WIDTH 1041
#define WINDOW_HEIGHT 768

#define ANGLE_SHIFT 7
#define POSITION_SCALE 64
#define TEXEL_SCALE 64

#define ARITHMETIC_SHIFT(nb, shift) ((nb) >> (shift))

#include "FP32.h"
// using CType = float;
// using CType = double;
using CType = FP32<FP_SHIFT>;

#endif