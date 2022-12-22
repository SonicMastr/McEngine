#ifdef __vita__

int _newlib_heap_size_user = 100 * 1024 * 1024;
unsigned int sceLibcHeapSize = 30 * 1024 * 1024;

#include "cbase.h"

#ifndef MCENGINE_FEATURE_SDL
#error SDL2 is currently required for vita builds
#endif

#include "VitaSDLEnvironment.h"
#include "ConVar.h"

#include <SDL2/SDL.h>
#include <vitasdk.h>

extern "C" {
	#include <gpu_es4/psp2_pvr_hint.h>
}

#define MAX_PATH 256

extern int mainSDL(int argc, char *argv[], SDLEnvironment *customSDLEnvironment);

int main(int argc, char* argv[])
{
	int ret = 0;

	PVRSRV_PSP2_APPHINT hint;
    char target_path[MAX_PATH];

    scePowerSetArmClockFrequency(444);
	scePowerSetGpuClockFrequency(222);

    /* Disable Back Touchpad to prevent "misclicks" */
    SDL_setenv("VITA_DISABLE_TOUCH_BACK", "1", 1);

    scePowerSetGpuClockFrequency(222);

    /* We need to use some custom hints */
    SDL_setenv("VITA_PVR_SKIP_INIT", "yeet", 1);

    /* Load Modules */
    sceKernelLoadStartModule("vs0:sys/external/libfios2.suprx", 0, NULL, 0, NULL, NULL);
    sceKernelLoadStartModule("vs0:sys/external/libc.suprx", 0, NULL, 0, NULL, NULL);
    snprintf(target_path, MAX_PATH, "app0:module/%s", "libgpu_es4_ext.suprx");
    sceKernelLoadStartModule(target_path, 0, NULL, 0, NULL, NULL);
    snprintf(target_path, MAX_PATH, "app0:module/%s", "libIMGEGL.suprx");
    sceKernelLoadStartModule(target_path, 0, NULL, 0, NULL, NULL);

    /* Set PVR Hints */
    PVRSRVInitializeAppHint(&hint);
    snprintf(hint.szGLES1, MAX_PATH, "app0:module/%s", "libGLESv1_CM.suprx");
    snprintf(hint.szGLES2, MAX_PATH, "app0:module/%s", "libGLESv2.suprx");
    snprintf(hint.szWindowSystem, MAX_PATH, "app0:module/%s", "libpvrPSP2_WSEGL.suprx");
	//hint.bDumpProfileData = 1;
	//hint.bDisableMetricsOutput = 0;
	//hint.ui32ProfileStartFrame = 40;
	//hint.ui32ProfileEndFrame = 70;
    hint.ui32SwTexOpCleanupDelay = 16000; // Set to 16 milliseconds to prevent a pool of unfreed memory
    PVRSRVCreateVirtualAppHint(&hint);
	

	ret = mainSDL(argc, argv, new VitaSDLEnvironment());
	

	return ret;
}

#endif
