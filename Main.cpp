#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "catch_reporter_github.hpp"
// Some Windows BS I guess included by Catch?
#ifdef far
#undef far
#endif
#ifdef near
#undef near
#endif
#include "Actor.h"

// This is janky but doing it this way to account for weird include
// dependencies people may have introduced into CollisionComponent.cpp
#include <algorithm>
#include <vector>
// Create dummy implementations for a few SDL functions/macros
#ifdef SDL_assert
#undef SDL_assert
#endif
#define SDL_assert(condition) REQUIRE(condition)

#ifdef SDL_assert_release
#undef SDL_assert_release
#endif
#define SDL_assert_release(condition) REQUIRE(condition)

// Skip over SDL logs completely
static void SDL_Log(...)
{
}

#include "AudioSystem.cpp"
const float DELTA_TIME = 0.016f;

TEST_CASE("AudioSystem tests")
{
	SECTION("Constructor")
	{
		AudioSystem as;
		REQUIRE(Mock::Mixer.mFrequency == 44100);
		REQUIRE(Mock::Mixer.mFormat == MIX_DEFAULT_FORMAT);
		REQUIRE(Mock::Mixer.mOutputChannels == 2);
		REQUIRE(Mock::Mixer.mChunkSize == 2048);
		REQUIRE(Mock::Mixer.mChannels.size() == 8);
	}

	SECTION("Passing in 16 to constructor has 16 channels")
	{
		AudioSystem as(16);
		REQUIRE(Mock::Mixer.mFrequency == 44100);
		REQUIRE(Mock::Mixer.mFormat == MIX_DEFAULT_FORMAT);
		REQUIRE(Mock::Mixer.mOutputChannels == 2);
		REQUIRE(Mock::Mixer.mChunkSize == 2048);
		REQUIRE(Mock::Mixer.mChannels.size() == 16);
	}

	SECTION("Destructor frees allocated chunks before calling Mix_CloseAudio")
	{
		AudioSystem as;
		as.CacheSound("Sound1.wav");
		as.CacheSound("Sound2.wav");
		as.CacheSound("Sound3.wav");
	}
}
