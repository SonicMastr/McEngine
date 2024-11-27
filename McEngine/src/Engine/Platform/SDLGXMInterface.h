#ifndef SDLGXMINTERFACE_H
#define SDLGXMINTERFACE_H

#include "GXMInterface.h"

#if defined(MCENGINE_FEATURE_GXM)

#include "SDL.h"

class SDLGXMInterface : public GXMInterface
{
public:
	SDLGXMInterface(SDL_Window *window);
	virtual ~SDLGXMInterface();

private:
	SDL_Window *m_window;
};

#endif

#endif
