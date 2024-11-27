#include "SDLGXMInterface.h"

#if defined(MCENGINE_FEATURE_SDL) && defined(MCENGINE_FEATURE_GXM)

#include "SDLEnvironment.h"

SDLGXMInterface::SDLGXMInterface(SDL_Window *window) : GXMInterface()
{
	m_window = window;
}

SDLGXMInterface::~SDLGXMInterface()
{
}

#endif
