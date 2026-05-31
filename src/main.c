//
// Created by maiconpintoabreu on 25/02/2026.
//

#ifdef IS_ANDROID
#include "raymob.h"
#include <stdlib.h>
#else
#include "raylib.h"
#endif

#include "game_manager.h"

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif


void UpdateDrawFrameWeb(void)
{
    if (!UpdateDrawFrame())
    {
        Destroy();
        CloseWindow();
    }
}

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main()
{
    if (!Init())
    {
        TraceLog(LOG_WARNING, "Initialization was canceled");
        Destroy();
        CloseWindow();
        return 0;
    }

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(UpdateDrawFrameWeb, 0, 1);
#else

    SetTargetFPS(60);
    // Main game loop
    while (!WindowShouldClose()) {
        if(!UpdateDrawFrame())
        {
            break;
        }
    }
#endif

    // De-Initialization
    //--------------------------------------------------------------------------------------
    Destroy();
    CloseWindow();
    //--------------------------------------------------------------------------------------

    return 0;
}