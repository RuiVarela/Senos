// sokol implementation library on non-Apple platforms
#define SOKOL_IMPL

#if defined(_WIN32)
#define SOKOL_D3D11
#elif defined(__EMSCRIPTEN__)
#define SOKOL_GLES2
#elif defined(__APPLE__)
// NOTE: on macOS, sokol.c is compiled explicitely as ObjC
#define SOKOL_METAL
#else
#define SOKOL_GLCORE33
#endif

#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_glue.h"
#include "sokol_audio.h"

#define SOKOL_IMGUI_IMPL
#include "../imgui/imgui.h"
#include "util/sokol_imgui.h"


