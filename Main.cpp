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
		REQUIRE(as.mChannels.size() == 8);
		for (int i = 0; i < 8; i++)
		{
			REQUIRE(!as.mChannels[i].IsValid());
		}
	}

	SECTION("Passing in 16 to constructor has 16 channels")
	{
		AudioSystem as(16);
		REQUIRE(Mock::Mixer.mFrequency == 44100);
		REQUIRE(Mock::Mixer.mFormat == MIX_DEFAULT_FORMAT);
		REQUIRE(Mock::Mixer.mOutputChannels == 2);
		REQUIRE(Mock::Mixer.mChunkSize == 2048);
		REQUIRE(Mock::Mixer.mChannels.size() == 16);
		REQUIRE(as.mChannels.size() == 16);
		for (int i = 0; i < 16; i++)
		{
			REQUIRE(!as.mChannels[i].IsValid());
		}
	}

	SECTION("Destructor frees allocated chunks before calling Mix_CloseAudio")
	{
		AudioSystem as;
		as.CacheSound("1.wav");
		as.CacheSound("2.wav");
		as.CacheSound("3.wav");
	}

	SECTION("PlaySound for a non-looping sound")
	{
		AudioSystem as;
		as.CacheSound("1.wav");
		as.CacheSound("2.wav");
		as.CacheSound("3.wav");

		SoundHandle snd = as.PlaySound("1.wav");
		// Handle should be valid
		REQUIRE(snd.IsValid());

		// SDL_Mixer should have played this on channel 0
		REQUIRE(Mock::Mixer.mChannels[0].mChunk != nullptr);
		REQUIRE(Mock::Mixer.mChannels[0].mChunk->mName == "Assets/Sounds/1.wav");
		REQUIRE(Mock::Mixer.mChannels[0].mLoops == 0);

		// AudioSystem should know about the sound handle
		REQUIRE(as.mHandleMap.find(snd) != as.mHandleMap.end());
		REQUIRE(as.mHandleMap[snd].mSoundName == "1.wav");
		REQUIRE(as.mHandleMap[snd].mChannel == 0);
		REQUIRE(as.mHandleMap[snd].mIsLooping == false);
		REQUIRE(as.mHandleMap[snd].mIsPaused == false);

		// AudioSystem channels vector should have correct handle
		REQUIRE(as.mChannels[0] == snd);
		for (int i = 1; i < 8; i++)
		{
			REQUIRE(!as.mChannels[i].IsValid());
		}
	}

	SECTION("PlaySound for a looping sound")
	{
		AudioSystem as;
		as.CacheSound("1.wav");
		as.CacheSound("2.wav");
		as.CacheSound("3.wav");

		SoundHandle snd = as.PlaySound("1.wav", true);
		// Handle should be valid
		REQUIRE(snd.IsValid());

		// SDL_Mixer should have played this on channel 0
		REQUIRE(Mock::Mixer.mChannels[0].mChunk != nullptr);
		REQUIRE(Mock::Mixer.mChannels[0].mChunk->mName == "Assets/Sounds/1.wav");
		REQUIRE(Mock::Mixer.mChannels[0].mLoops == -1);

		// AudioSystem should know about the sound handle
		REQUIRE(as.mHandleMap.find(snd) != as.mHandleMap.end());
		REQUIRE(as.mHandleMap[snd].mSoundName == "1.wav");
		REQUIRE(as.mHandleMap[snd].mChannel == 0);
		REQUIRE(as.mHandleMap[snd].mIsLooping == true);
		REQUIRE(as.mHandleMap[snd].mIsPaused == false);

		// AudioSystem channels vector should have correct handle
		REQUIRE(as.mChannels[0] == snd);
		for (int i = 1; i < 8; i++)
		{
			REQUIRE(!as.mChannels[i].IsValid());
		}
	}

	SECTION("PlaySound multiple times (without running out of channels)")
	{
		AudioSystem as(4);
		as.CacheSound("1.wav");
		as.CacheSound("2.wav");
		as.CacheSound("3.wav");
		as.CacheSound("4.wav");

		SoundHandle snd1 = as.PlaySound("1.wav");
		SoundHandle snd2 = as.PlaySound("2.wav");
		SoundHandle snd3 = as.PlaySound("3.wav");
		SoundHandle snd4 = as.PlaySound("4.wav");

		// First sound
		// Handle should be valid
		REQUIRE(snd1.IsValid());
		// SDL_Mixer should have played this on channel 0
		REQUIRE(Mock::Mixer.mChannels[0].mChunk != nullptr);
		REQUIRE(Mock::Mixer.mChannels[0].mChunk->mName == "Assets/Sounds/1.wav");
		REQUIRE(Mock::Mixer.mChannels[0].mLoops == 0);
		// AudioSystem should know about the sound handle
		REQUIRE(as.mHandleMap.find(snd1) != as.mHandleMap.end());
		REQUIRE(as.mHandleMap[snd1].mSoundName == "1.wav");
		REQUIRE(as.mHandleMap[snd1].mChannel == 0);
		REQUIRE(as.mHandleMap[snd1].mIsLooping == false);
		REQUIRE(as.mHandleMap[snd1].mIsPaused == false);
		// AudioSystem channels vector should have correct handle
		REQUIRE(as.mChannels[0] == snd1);

		// Second sound
		// Handle should be valid
		REQUIRE(snd2.IsValid());
		// SDL_Mixer should have played this on channel 1
		REQUIRE(Mock::Mixer.mChannels[1].mChunk != nullptr);
		REQUIRE(Mock::Mixer.mChannels[1].mChunk->mName == "Assets/Sounds/2.wav");
		REQUIRE(Mock::Mixer.mChannels[1].mLoops == 0);
		// AudioSystem should know about the sound handle
		REQUIRE(as.mHandleMap.find(snd2) != as.mHandleMap.end());
		REQUIRE(as.mHandleMap[snd2].mSoundName == "2.wav");
		REQUIRE(as.mHandleMap[snd2].mChannel == 1);
		REQUIRE(as.mHandleMap[snd2].mIsLooping == false);
		REQUIRE(as.mHandleMap[snd2].mIsPaused == false);
		// AudioSystem channels vector should have correct handle
		REQUIRE(as.mChannels[1] == snd2);

		// Third sound
		// Handle should be valid
		REQUIRE(snd3.IsValid());
		// SDL_Mixer should have played this on channel 1
		REQUIRE(Mock::Mixer.mChannels[2].mChunk != nullptr);
		REQUIRE(Mock::Mixer.mChannels[2].mChunk->mName == "Assets/Sounds/3.wav");
		REQUIRE(Mock::Mixer.mChannels[2].mLoops == 0);
		// AudioSystem should know about the sound handle
		REQUIRE(as.mHandleMap.find(snd3) != as.mHandleMap.end());
		REQUIRE(as.mHandleMap[snd3].mSoundName == "3.wav");
		REQUIRE(as.mHandleMap[snd3].mChannel == 2);
		REQUIRE(as.mHandleMap[snd3].mIsLooping == false);
		REQUIRE(as.mHandleMap[snd3].mIsPaused == false);
		// AudioSystem channels vector should have correct handle
		REQUIRE(as.mChannels[2] == snd3);

		// Fourth sound
		// Handle should be valid
		REQUIRE(snd4.IsValid());
		// SDL_Mixer should have played this on channel 1
		REQUIRE(Mock::Mixer.mChannels[3].mChunk != nullptr);
		REQUIRE(Mock::Mixer.mChannels[3].mChunk->mName == "Assets/Sounds/4.wav");
		REQUIRE(Mock::Mixer.mChannels[3].mLoops == 0);
		// AudioSystem should know about the sound handle
		REQUIRE(as.mHandleMap.find(snd4) != as.mHandleMap.end());
		REQUIRE(as.mHandleMap[snd4].mSoundName == "4.wav");
		REQUIRE(as.mHandleMap[snd4].mChannel == 3);
		REQUIRE(as.mHandleMap[snd4].mIsLooping == false);
		REQUIRE(as.mHandleMap[snd4].mIsPaused == false);
		// AudioSystem channels vector should have correct handle
		REQUIRE(as.mChannels[3] == snd4);
	}

	SECTION("PlaySound and then Update finds out the sound isn't playing anymore")
	{
		AudioSystem as;
		as.CacheSound("1.wav");
		as.CacheSound("2.wav");
		as.CacheSound("3.wav");

		SoundHandle snd = as.PlaySound("1.wav");
		SoundHandle snd2 = as.PlaySound("2.wav");
		SoundHandle snd3 = as.PlaySound("3.wav");
		// Handle should be valid
		REQUIRE(snd.IsValid());

		// SDL_Mixer should have played this on channel 0
		REQUIRE(Mock::Mixer.mChannels[0].mChunk != nullptr);
		REQUIRE(Mock::Mixer.mChannels[0].mChunk->mName == "Assets/Sounds/1.wav");
		REQUIRE(Mock::Mixer.mChannels[0].mLoops == 0);

		// AudioSystem should know about the sound handle
		REQUIRE(as.mHandleMap.find(snd) != as.mHandleMap.end());
		REQUIRE(as.mHandleMap[snd].mSoundName == "1.wav");
		REQUIRE(as.mHandleMap[snd].mChannel == 0);
		REQUIRE(as.mHandleMap[snd].mIsLooping == false);
		REQUIRE(as.mHandleMap[snd].mIsPaused == false);

		// AudioSystem channels vector should have correct handle
		REQUIRE(as.mChannels[0] == snd);

		// Fake that the sound is stopped
		Mock::Mixer.HaltChannel(0);

		// Update AudioSystem, which should detect the sound is stopped
		as.Update(DELTA_TIME);

		// Make sure the handle is not in the handle map anymore
		REQUIRE(as.mHandleMap.find(snd) == as.mHandleMap.end());
		// Make sure the second/third sounds are still there
		REQUIRE(as.mHandleMap.find(snd2) != as.mHandleMap.end());
		REQUIRE(as.mChannels[1] == snd2);
		REQUIRE(as.mHandleMap.find(snd3) != as.mHandleMap.end());
		REQUIRE(as.mChannels[2] == snd3);
	}

	SECTION("PlaySound reuses a channel if Update detects it's stopped")
	{
		AudioSystem as(4);
		as.CacheSound("1.wav");
		as.CacheSound("2.wav");
		as.CacheSound("3.wav");
		as.CacheSound("4.wav");
		as.CacheSound("5.wav");

		SoundHandle snd = as.PlaySound("1.wav");
		SoundHandle snd2 = as.PlaySound("2.wav");
		SoundHandle snd3 = as.PlaySound("3.wav");
		SoundHandle snd4 = as.PlaySound("4.wav");

		// Lets pretend sound 3 has stopped
		Mock::Mixer.HaltChannel(2);
		as.Update(DELTA_TIME);

		SoundHandle snd5 = as.PlaySound("5.wav");

		// Channels should be in this order: snd, snd2, snd5, snd4
		REQUIRE(as.mChannels[0] == snd);
		REQUIRE(as.mChannels[1] == snd2);
		REQUIRE(as.mChannels[2] == snd5);
		REQUIRE(as.mChannels[3] == snd4);

		// Make sure that snd3 is not in the map (and the others are)
		REQUIRE(as.mHandleMap.find(snd3) == as.mHandleMap.end());
		REQUIRE(as.mHandleMap.find(snd) != as.mHandleMap.end());
		REQUIRE(as.mHandleMap.find(snd2) != as.mHandleMap.end());
		REQUIRE(as.mHandleMap.find(snd4) != as.mHandleMap.end());
		REQUIRE(as.mHandleMap.find(snd5) != as.mHandleMap.end());

		// Make sure we have the correct sounds in SDL_Mixer
		REQUIRE(Mock::Mixer.mChannels[0].mChunk != nullptr);
		REQUIRE(Mock::Mixer.mChannels[0].mChunk->mName == "Assets/Sounds/1.wav");
		REQUIRE(Mock::Mixer.mChannels[1].mChunk != nullptr);
		REQUIRE(Mock::Mixer.mChannels[1].mChunk->mName == "Assets/Sounds/2.wav");
		REQUIRE(Mock::Mixer.mChannels[2].mChunk != nullptr);
		REQUIRE(Mock::Mixer.mChannels[2].mChunk->mName == "Assets/Sounds/5.wav");
		REQUIRE(Mock::Mixer.mChannels[3].mChunk != nullptr);
		REQUIRE(Mock::Mixer.mChannels[3].mChunk->mName == "Assets/Sounds/4.wav");
	}
	
	SECTION("PauseSound pauses a playing sound")
	{
		AudioSystem as(4);
		as.CacheSound("1.wav");
		as.CacheSound("2.wav");
		as.CacheSound("3.wav");
		as.CacheSound("4.wav");

		SoundHandle snd = as.PlaySound("1.wav");
		as.PauseSound(snd);

		REQUIRE(as.mHandleMap.find(snd) != as.mHandleMap.end());
		REQUIRE(as.mHandleMap.find(snd)->second.mIsPaused);

		REQUIRE(Mock::Mixer.mChannels[0].mPaused);
	}

	SECTION("PauseSound does not do anything if the SoundHandle isn't in the map")
	{
		AudioSystem as(4);
		as.CacheSound("1.wav");
		as.CacheSound("2.wav");
		as.CacheSound("3.wav");
		as.CacheSound("4.wav");

		SoundHandle snd = as.PlaySound("1.wav");
		SoundHandle snd2 = as.PlaySound("2.wav");
		SoundHandle snd3 = as.PlaySound("3.wav");
		SoundHandle snd4 = as.PlaySound("4.wav");

		as.PauseSound(SoundHandle::Invalid);

		// Invalid should not be in the map
		REQUIRE(as.mHandleMap.find(SoundHandle::Invalid) == as.mHandleMap.end());
		// The other sounds should still be in the map
		REQUIRE(as.mHandleMap.find(snd) != as.mHandleMap.end());
		REQUIRE(as.mHandleMap.find(snd2) != as.mHandleMap.end());
		REQUIRE(as.mHandleMap.find(snd3) != as.mHandleMap.end());
		REQUIRE(as.mHandleMap.find(snd4) != as.mHandleMap.end());

		// Make sure the sounds are NOT flagged as paused in AudioSystem
		REQUIRE(!as.mHandleMap.find(snd)->second.mIsPaused);
		REQUIRE(!as.mHandleMap.find(snd2)->second.mIsPaused);
		REQUIRE(!as.mHandleMap.find(snd3)->second.mIsPaused);
		REQUIRE(!as.mHandleMap.find(snd4)->second.mIsPaused);

		// Make the sounds are NOT as paused in SDL_Mixer
		REQUIRE(!Mock::Mixer.mChannels[0].mPaused);
		REQUIRE(!Mock::Mixer.mChannels[1].mPaused);
		REQUIRE(!Mock::Mixer.mChannels[2].mPaused);
		REQUIRE(!Mock::Mixer.mChannels[3].mPaused);
	}

	SECTION("PauseSound and then ResumeSound works")
	{
		AudioSystem as(4);
		as.CacheSound("1.wav");
		as.CacheSound("2.wav");
		as.CacheSound("3.wav");
		as.CacheSound("4.wav");

		SoundHandle snd = as.PlaySound("1.wav");
		as.PauseSound(snd);

		REQUIRE(as.mHandleMap.find(snd) != as.mHandleMap.end());
		REQUIRE(as.mHandleMap.find(snd)->second.mIsPaused);

		REQUIRE(Mock::Mixer.mChannels[0].mPaused);

		as.ResumeSound(snd);

		REQUIRE(as.mHandleMap.find(snd) != as.mHandleMap.end());
		REQUIRE(!as.mHandleMap.find(snd)->second.mIsPaused);

		REQUIRE(!Mock::Mixer.mChannels[0].mPaused);
	}

	SECTION("ResumeSound does not do anything if the SoundHandle isn't in the map")
	{
		AudioSystem as(4);
		as.CacheSound("1.wav");
		as.CacheSound("2.wav");
		as.CacheSound("3.wav");
		as.CacheSound("4.wav");

		SoundHandle snd = as.PlaySound("1.wav");
		SoundHandle snd2 = as.PlaySound("2.wav");
		SoundHandle snd3 = as.PlaySound("3.wav");
		SoundHandle snd4 = as.PlaySound("4.wav");

		as.ResumeSound(SoundHandle::Invalid);

		// Invalid should not be in the map
		REQUIRE(as.mHandleMap.find(SoundHandle::Invalid) == as.mHandleMap.end());
		// The other sounds should still be in the map
		REQUIRE(as.mHandleMap.find(snd) != as.mHandleMap.end());
		REQUIRE(as.mHandleMap.find(snd2) != as.mHandleMap.end());
		REQUIRE(as.mHandleMap.find(snd3) != as.mHandleMap.end());
		REQUIRE(as.mHandleMap.find(snd4) != as.mHandleMap.end());
	}

	SECTION("StopSound stops the requested sound (and not others)")
	{
		AudioSystem as(4);
		as.CacheSound("1.wav");
		as.CacheSound("2.wav");
		as.CacheSound("3.wav");
		as.CacheSound("4.wav");

		SoundHandle snd = as.PlaySound("1.wav");
		SoundHandle snd2 = as.PlaySound("2.wav");
		SoundHandle snd3 = as.PlaySound("3.wav");
		SoundHandle snd4 = as.PlaySound("4.wav");

		as.StopSound(snd3);

		// Channels should be in this order: snd, snd2, Invalid, snd4
		REQUIRE(as.mChannels[0] == snd);
		REQUIRE(as.mChannels[1] == snd2);
		REQUIRE(!as.mChannels[2].IsValid());
		REQUIRE(as.mChannels[3] == snd4);

		// Make sure that snd3 is not in the map (and the others are)
		REQUIRE(as.mHandleMap.find(snd3) == as.mHandleMap.end());
		REQUIRE(as.mHandleMap.find(snd) != as.mHandleMap.end());
		REQUIRE(as.mHandleMap.find(snd2) != as.mHandleMap.end());
		REQUIRE(as.mHandleMap.find(snd4) != as.mHandleMap.end());

		// Make sure we have the correct sounds in SDL_Mixer
		REQUIRE(Mock::Mixer.mChannels[0].mChunk != nullptr);
		REQUIRE(Mock::Mixer.mChannels[0].mChunk->mName == "Assets/Sounds/1.wav");
		REQUIRE(Mock::Mixer.mChannels[1].mChunk != nullptr);
		REQUIRE(Mock::Mixer.mChannels[1].mChunk->mName == "Assets/Sounds/2.wav");
		REQUIRE(Mock::Mixer.mChannels[2].mChunk == nullptr);
		REQUIRE(Mock::Mixer.mChannels[3].mChunk != nullptr);
		REQUIRE(Mock::Mixer.mChannels[3].mChunk->mName == "Assets/Sounds/4.wav");
	}

	SECTION("StopSound does not do anything if the SoundHandle isn't in the map")
	{
		AudioSystem as(4);
		as.CacheSound("1.wav");
		as.CacheSound("2.wav");
		as.CacheSound("3.wav");
		as.CacheSound("4.wav");

		SoundHandle snd = as.PlaySound("1.wav");
		SoundHandle snd2 = as.PlaySound("2.wav");
		SoundHandle snd3 = as.PlaySound("3.wav");
		SoundHandle snd4 = as.PlaySound("4.wav");

		as.StopSound(SoundHandle::Invalid);

		// Invalid should not be in the map
		REQUIRE(as.mHandleMap.find(SoundHandle::Invalid) == as.mHandleMap.end());
		// The other sounds should still be in the map
		REQUIRE(as.mHandleMap.find(snd) != as.mHandleMap.end());
		REQUIRE(as.mHandleMap.find(snd2) != as.mHandleMap.end());
		REQUIRE(as.mHandleMap.find(snd3) != as.mHandleMap.end());
		REQUIRE(as.mHandleMap.find(snd4) != as.mHandleMap.end());

		// Make the sounds are all playing in SDL_Mixer
		REQUIRE(Mock::Mixer.mChannels[0].mPlaying);
		REQUIRE(Mock::Mixer.mChannels[1].mPlaying);
		REQUIRE(Mock::Mixer.mChannels[2].mPlaying);
		REQUIRE(Mock::Mixer.mChannels[3].mPlaying);
	}

	SECTION("GetSoundState - Playing sound")
	{
		AudioSystem as(4);
		as.CacheSound("1.wav");
		as.CacheSound("2.wav");
		as.CacheSound("3.wav");
		as.CacheSound("4.wav");

		SoundHandle snd = as.PlaySound("1.wav");

		REQUIRE(as.GetSoundState(snd) == SoundState::Playing);
	}

	SECTION("GetSoundState - Paused sound")
	{
		AudioSystem as(4);
		as.CacheSound("1.wav");
		as.CacheSound("2.wav");
		as.CacheSound("3.wav");
		as.CacheSound("4.wav");

		SoundHandle snd = as.PlaySound("1.wav");

		REQUIRE(as.GetSoundState(snd) == SoundState::Playing);

		as.PauseSound(snd);

		REQUIRE(as.GetSoundState(snd) == SoundState::Paused);
	}

	SECTION("GetSoundState - Paused and then Resumed sound")
	{
		AudioSystem as(4);
		as.CacheSound("1.wav");
		as.CacheSound("2.wav");
		as.CacheSound("3.wav");
		as.CacheSound("4.wav");

		SoundHandle snd = as.PlaySound("1.wav");

		REQUIRE(as.GetSoundState(snd) == SoundState::Playing);

		as.PauseSound(snd);

		REQUIRE(as.GetSoundState(snd) == SoundState::Paused);

		as.ResumeSound(snd);

		REQUIRE(as.GetSoundState(snd) == SoundState::Playing);
	}

	SECTION("GetSoundState - Force stopped sound")
	{
		AudioSystem as(4);
		as.CacheSound("1.wav");
		as.CacheSound("2.wav");
		as.CacheSound("3.wav");
		as.CacheSound("4.wav");

		SoundHandle snd = as.PlaySound("1.wav");

		REQUIRE(as.GetSoundState(snd) == SoundState::Playing);

		as.StopSound(snd);

		REQUIRE(as.GetSoundState(snd) == SoundState::Stopped);
	}

	SECTION("GetSoundState - Sound stopped by Update")
	{
		AudioSystem as(4);
		as.CacheSound("1.wav");
		as.CacheSound("2.wav");
		as.CacheSound("3.wav");
		as.CacheSound("4.wav");

		SoundHandle snd = as.PlaySound("1.wav");

		REQUIRE(as.GetSoundState(snd) == SoundState::Playing);

		// Fake that the sound is stopped
		Mock::Mixer.HaltChannel(0);
		as.Update(DELTA_TIME);

		REQUIRE(as.GetSoundState(snd) == SoundState::Stopped);

		// Make sure sound was not added back to map
		REQUIRE(as.mHandleMap.find(snd) == as.mHandleMap.end());
	}

	SECTION("GetSoundState - Sound doesn't exist")
	{
		AudioSystem as(4);
		as.CacheSound("1.wav");
		as.CacheSound("2.wav");
		as.CacheSound("3.wav");
		as.CacheSound("4.wav");

		REQUIRE(as.GetSoundState(SoundHandle::Invalid) == SoundState::Stopped);

		// Make sure invalid sound was not added into map
		REQUIRE(as.mHandleMap.find(SoundHandle::Invalid) == as.mHandleMap.end());
	}
	
	SECTION("PlaySound running out of channels priority 1 (oldest instance of same sound)")
	{
		AudioSystem as(4);
		as.CacheSound("1.wav");
		as.CacheSound("2.wav");
		as.CacheSound("3.wav");

		SoundHandle snd = as.PlaySound("1.wav");
		SoundHandle snd2 = as.PlaySound("2.wav");
		SoundHandle snd3 = as.PlaySound("2.wav");
		SoundHandle snd4 = as.PlaySound("2.wav");
		SoundHandle snd5 = as.PlaySound("2.wav");

		// Channels should be in this order: snd, snd5, snd3, snd4
		REQUIRE(as.mChannels[0] == snd);
		REQUIRE(as.mChannels[1] == snd5);
		REQUIRE(as.mChannels[2] == snd3);
		REQUIRE(as.mChannels[3] == snd4);

		// Make sure that snd2 is not in the map (and the others are)
		REQUIRE(as.mHandleMap.find(snd2) == as.mHandleMap.end());
		REQUIRE(as.mHandleMap.find(snd) != as.mHandleMap.end());
		REQUIRE(as.mHandleMap.find(snd3) != as.mHandleMap.end());
		REQUIRE(as.mHandleMap.find(snd4) != as.mHandleMap.end());
		REQUIRE(as.mHandleMap.find(snd5) != as.mHandleMap.end());

		// Make sure we have the correct sounds in SDL_Mixer
		REQUIRE(Mock::Mixer.mChannels[0].mChunk != nullptr);
		REQUIRE(Mock::Mixer.mChannels[0].mChunk->mName == "Assets/Sounds/1.wav");
		REQUIRE(Mock::Mixer.mChannels[1].mChunk != nullptr);
		REQUIRE(Mock::Mixer.mChannels[1].mChunk->mName == "Assets/Sounds/2.wav");
		REQUIRE(Mock::Mixer.mChannels[2].mChunk != nullptr);
		REQUIRE(Mock::Mixer.mChannels[2].mChunk->mName == "Assets/Sounds/2.wav");
		REQUIRE(Mock::Mixer.mChannels[3].mChunk != nullptr);
		REQUIRE(Mock::Mixer.mChannels[3].mChunk->mName == "Assets/Sounds/2.wav");
	}

	SECTION("PlaySound running out of channels priority 2 (oldest non-looping sound)")
	{
		AudioSystem as(4);
		as.CacheSound("1.wav");
		as.CacheSound("2.wav");
		as.CacheSound("3.wav");
		as.CacheSound("4.wav");
		as.CacheSound("5.wav");

		SoundHandle snd = as.PlaySound("1.wav", true);
		SoundHandle snd2 = as.PlaySound("2.wav", true);
		SoundHandle snd3 = as.PlaySound("3.wav");
		SoundHandle snd4 = as.PlaySound("4.wav", true);
		SoundHandle snd5 = as.PlaySound("5.wav");

		// Channels should be in this order: snd, snd2, snd5, snd4
		REQUIRE(as.mChannels[0] == snd);
		REQUIRE(as.mChannels[1] == snd2);
		REQUIRE(as.mChannels[2] == snd5);
		REQUIRE(as.mChannels[3] == snd4);

		// Make sure that snd3 is not in the map (and the others are)
		REQUIRE(as.mHandleMap.find(snd3) == as.mHandleMap.end());
		REQUIRE(as.mHandleMap.find(snd) != as.mHandleMap.end());
		REQUIRE(as.mHandleMap.find(snd2) != as.mHandleMap.end());
		REQUIRE(as.mHandleMap.find(snd4) != as.mHandleMap.end());
		REQUIRE(as.mHandleMap.find(snd5) != as.mHandleMap.end());

		// Make sure we have the correct sounds in SDL_Mixer
		REQUIRE(Mock::Mixer.mChannels[0].mChunk != nullptr);
		REQUIRE(Mock::Mixer.mChannels[0].mChunk->mName == "Assets/Sounds/1.wav");
		REQUIRE(Mock::Mixer.mChannels[1].mChunk != nullptr);
		REQUIRE(Mock::Mixer.mChannels[1].mChunk->mName == "Assets/Sounds/2.wav");
		REQUIRE(Mock::Mixer.mChannels[2].mChunk != nullptr);
		REQUIRE(Mock::Mixer.mChannels[2].mChunk->mName == "Assets/Sounds/5.wav");
		REQUIRE(Mock::Mixer.mChannels[3].mChunk != nullptr);
		REQUIRE(Mock::Mixer.mChannels[3].mChunk->mName == "Assets/Sounds/4.wav");
	}

	SECTION("PlaySound running out of channels priority 3 (oldest sound)")
	{
		AudioSystem as(4);
		as.CacheSound("1.wav");
		as.CacheSound("2.wav");
		as.CacheSound("3.wav");
		as.CacheSound("4.wav");
		as.CacheSound("5.wav");

		SoundHandle snd = as.PlaySound("1.wav", true);
		SoundHandle snd2 = as.PlaySound("2.wav", true);
		SoundHandle snd3 = as.PlaySound("3.wav", true);
		SoundHandle snd4 = as.PlaySound("4.wav", true);
		SoundHandle snd5 = as.PlaySound("5.wav");

		// Channels should be in this order: snd5, snd2, snd3, snd4
		REQUIRE(as.mChannels[0] == snd5);
		REQUIRE(as.mChannels[1] == snd2);
		REQUIRE(as.mChannels[2] == snd3);
		REQUIRE(as.mChannels[3] == snd4);

		// Make sure that snd is not in the map (and the others are)
		REQUIRE(as.mHandleMap.find(snd) == as.mHandleMap.end());
		REQUIRE(as.mHandleMap.find(snd2) != as.mHandleMap.end());
		REQUIRE(as.mHandleMap.find(snd3) != as.mHandleMap.end());
		REQUIRE(as.mHandleMap.find(snd4) != as.mHandleMap.end());
		REQUIRE(as.mHandleMap.find(snd5) != as.mHandleMap.end());

		// Make sure we have the correct sounds in SDL_Mixer
		REQUIRE(Mock::Mixer.mChannels[0].mChunk != nullptr);
		REQUIRE(Mock::Mixer.mChannels[0].mChunk->mName == "Assets/Sounds/5.wav");
		REQUIRE(Mock::Mixer.mChannels[1].mChunk != nullptr);
		REQUIRE(Mock::Mixer.mChannels[1].mChunk->mName == "Assets/Sounds/2.wav");
		REQUIRE(Mock::Mixer.mChannels[2].mChunk != nullptr);
		REQUIRE(Mock::Mixer.mChannels[2].mChunk->mName == "Assets/Sounds/3.wav");
		REQUIRE(Mock::Mixer.mChannels[3].mChunk != nullptr);
		REQUIRE(Mock::Mixer.mChannels[3].mChunk->mName == "Assets/Sounds/4.wav");
	}
}
