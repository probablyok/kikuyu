// Delibrately ignore warnings on raygui compile

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wformat"
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#pragma GCC diagnostic pop
