#include "AudioSystem.h"
#include "SDL3/SDL.h"
#include <filesystem>

SoundHandle SoundHandle::Invalid;

// Create the AudioSystem with specified number of channels
// (Defaults to 8 channels)
AudioSystem::AudioSystem(int numChannels)
{

	Mix_OpenAudio(0, nullptr);
	Mix_AllocateChannels(numChannels);
	mChannels.resize(numChannels);
}

// Destroy the AudioSystem
AudioSystem::~AudioSystem()
{
	for (auto const& [s, m] : mSounds)
	{
		Mix_FreeChunk(m);
	}
	mSounds.clear();
	Mix_CloseAudio();
}

// Updates the status of all the active sounds every frame
void AudioSystem::Update(float deltaTime)
{
	// Every frame, you need to do a regular for loop over mChannels.
	// For any indices which have an IsValid() SoundHandle, use Mix_Playing
	// to see if that sound is still playing on its corresponding SDL channel number.

	std::vector<SoundHandle> toRemove = {};
	for (int i = 0; i < mChannels.size(); i++)
	{
		if (mChannels[i].IsValid())
		{
			int playing = Mix_Playing(i);
			//If the sound is NOT playing anymore, this means you need to remove that SoundHandle
			//from the mHandleMap and you should reset that index in mChannels with
			//.Reset() as the channel should be flagged as available again.
			if (playing == 0) // 0 = not playing
			{
				auto iter = mHandleMap.find(mChannels[i]); // Make a new one so don't dereference
				mHandleMap.erase(iter);
				mChannels[i].Reset();
			}
		}
	}
}

// Plays the sound with the specified name and loops if looping is true
// Returns the SoundHandle which is used to perform any other actions on the
// sound when active
// NOTE: The soundName is without the "Assets/Sounds/" part of the file
//       For example, pass in "ChompLoop.wav" rather than
//       "Assets/Sounds/ChompLoop.wav".
SoundHandle AudioSystem::PlaySound(const std::string& soundName, bool looping)
{
	Mix_Chunk* sound = GetSound(soundName);
	if (sound == nullptr)
	{
		SDL_Log("[AudioSystem] PlaySound couldn't find sound for %s", soundName.c_str());
		return SoundHandle::Invalid;
	}
	//Find the first available channel
	int firstAvailChannel = -1; //-1 for not yet set
	for (int i = 0; i < mChannels.size(); i++)
	{
		if (firstAvailChannel == -1)
		{
			if (!mChannels[i].IsValid())
			{
				firstAvailChannel = i;
			}
		}
	}

	//Increment
	++mLastHandle;
	SoundHandle soundHandle = SoundHandle(mLastHandle);

	//Handle info
	HandleInfo handleInfo = HandleInfo(soundName, firstAvailChannel, looping, false);

	//Put in map
	mHandleMap.emplace(soundHandle, handleInfo);
	mChannels[firstAvailChannel] = soundHandle;

	//Play the sound
	int loopInt = (looping) ? -1 : 0;
	Mix_PlayChannel(firstAvailChannel, sound, loopInt);

	return soundHandle;
}

// Stops the sound if it is currently playing
void AudioSystem::StopSound(SoundHandle sound)
{
	auto iter = mHandleMap.find(sound);
	if (iter == mHandleMap.end())
	{
		SDL_Log("[AudioSystem] StopSound couldn't find handle %s", sound.GetDebugStr());
	}
	else
	{
		Mix_HaltChannel(iter->second.mChannel);
		mChannels[iter->second.mChannel].Reset();
		mHandleMap.erase(iter);
	}
}

// Pauses the sound if it is currently playing
void AudioSystem::PauseSound(SoundHandle sound)
{
	auto iter = mHandleMap.find(sound);
	if (iter == mHandleMap.end())
	{
		SDL_Log("[AudioSystem] PauseSound couldn't find handle %s", sound.GetDebugStr());
	}
	else
	{
		HandleInfo info = iter->second;
		if (!info.mIsPaused) // If not yet paused
		{
			Mix_Pause(info.mChannel);
			//Would use info.mIsPaused here, but I think it's not actually setting the value
			iter->second.mIsPaused = true;
		}
	}
}

// Resumes the sound if it is currently paused
void AudioSystem::ResumeSound(SoundHandle sound)
{
	auto iter = mHandleMap.find(sound);
	if (iter == mHandleMap.end())
	{
		SDL_Log("[AudioSystem] PauseSound couldn't find handle %s", sound.GetDebugStr());
	}
	else
	{
		HandleInfo info = iter->second;
		if (info.mIsPaused) // If not yet paused
		{
			Mix_Resume(info.mChannel);
			//Would use info.mIsPaused here, but I think it's not actually setting the value
			iter->second.mIsPaused = false;
		}
	}
}

// Returns the current state of the sound
SoundState AudioSystem::GetSoundState(SoundHandle sound) const
{
	auto iter = mHandleMap.find(sound);
	if (iter == mHandleMap.end())
	{
		return SoundState::Stopped;
	}
	//Referenced Process input
	if (iter->second.mIsPaused)
	{
		return SoundState::Paused;
	}
	return SoundState::Playing;
}

// Stops all sounds on all channels
void AudioSystem::StopAllSounds()
{
	Mix_HaltChannel(-1);
	for (SoundHandle handle : mChannels)
	{
		handle.Reset();
	}
	mHandleMap.clear();
}

// Cache all sounds under Assets/Sounds
void AudioSystem::CacheAllSounds()
{
#ifndef __clang_analyzer__
	std::error_code ec{};
	for (const auto& rootDirEntry : std::filesystem::directory_iterator{"Assets/Sounds", ec})
	{
		std::string extension = rootDirEntry.path().extension().string();
		if (extension == ".ogg" || extension == ".wav")
		{
			std::string fileName = rootDirEntry.path().stem().string();
			fileName += extension;
			CacheSound(fileName);
		}
	}
#endif
}

// Used to preload the sound data of a sound
// NOTE: The soundName is without the "Assets/Sounds/" part of the file
//       For example, pass in "ChompLoop.wav" rather than
//       "Assets/Sounds/ChompLoop.wav".
void AudioSystem::CacheSound(const std::string& soundName)
{
	GetSound(soundName);
}

// If the sound is already loaded, returns Mix_Chunk from the map.
// Otherwise, will attempt to load the file and save it in the map.
// Returns nullptr if sound is not found.
// NOTE: The soundName is without the "Assets/Sounds/" part of the file
//       For example, pass in "ChompLoop.wav" rather than
//       "Assets/Sounds/ChompLoop.wav".
Mix_Chunk* AudioSystem::GetSound(const std::string& soundName)
{
	std::string fileName = "Assets/Sounds/";
	fileName += soundName;

	Mix_Chunk* chunk = nullptr;
	auto iter = mSounds.find(fileName);
	if (iter != mSounds.end())
	{
		chunk = iter->second;
	}
	else
	{
		chunk = Mix_LoadWAV(fileName.c_str());
		if (!chunk)
		{
			SDL_Log("[AudioSystem] Failed to load sound file %s", fileName.c_str());
			return nullptr;
		}

		mSounds.emplace(fileName, chunk);
	}
	return chunk;
}

// Input for debugging purposes
void AudioSystem::ProcessInput(const bool keys[])
{
	// Debugging code that outputs all active sounds on leading edge of period key
	if (keys[SDL_SCANCODE_PERIOD] && !mLastDebugKey)
	{
		SDL_Log("[AudioSystem] Active Sounds:");
		for (size_t i = 0; i < mChannels.size(); i++)
		{
			if (mChannels[i].IsValid())
			{
				auto iter = mHandleMap.find(mChannels[i]);
				if (iter != mHandleMap.end())
				{
					HandleInfo& hi = iter->second;
					SDL_Log("Channel %d: %s, %s, looping = %d, paused = %d",
							static_cast<unsigned>(i), mChannels[i].GetDebugStr(),
							hi.mSoundName.c_str(), hi.mIsLooping, hi.mIsPaused);
				}
				else
				{
					SDL_Log("Channel %d: %s INVALID", static_cast<unsigned>(i),
							mChannels[i].GetDebugStr());
				}
			}
		}
	}

	mLastDebugKey = keys[SDL_SCANCODE_PERIOD];
}
