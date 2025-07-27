// This is just a dummy SDL header in case someone includes it
#pragma once

#include <string>
#include "SDL_stdinc.h"

void SDL_Log(...);

struct SDL_Texture
{
	std::string mName;
};
