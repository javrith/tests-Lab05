#pragma once
#include <string>
#include <set>
#include <vector>
#include "../catch.hpp"

struct Mix_Chunk
{
	std::string mName;
};

struct Mock
{
	void OpenAudio(int devid, int* spec)
	{
		mDevID = devid;
		mSpec = spec;

		AllocateChannels(4);
	}

	void AllocateChannels(int channels) { mChannels.resize(channels); }

	void Reset()
	{
		mDevID = -1;
		mSpec = nullptr;
		mChannels.clear();
		mChunks.clear();
	}

	void FreeChunk(Mix_Chunk* chunk)
	{
		auto iter = mChunks.find(chunk);
		if (iter != mChunks.end())
		{
			mChunks.erase(iter);
			delete chunk;
		}
		else
		{
			FAIL("Mix_FreeChunk called with an invalid chunk");
		}
	}

	void CloseAudio()
	{
		if (!mChunks.empty())
		{
			FAIL("Mix_CloseAudio called but not all chunks were freed");
		}
		Reset();
	}

	int PlayChannel(int channel, Mix_Chunk* chunk, int loops)
	{
		if (channel == -1)
		{
			FAIL("Mix_PlayChannel should not be called with a channel of -1");
		}

		if (channel < 0 || channel >= mChannels.size())
		{
			FAIL("Mix_PlayChannel called with an out-of-bounds channel");
		}

		mChannels[channel].mChunk = chunk;
		mChannels[channel].mPlaying = true;
		mChannels[channel].mPaused = false;
		mChannels[channel].mLoops = loops;

		return 0;
	}

	void HaltChannel(int channel)
	{
		if (channel == -1)
		{
			FAIL("Mix_HaltChannel should not be called with a channel of -1");
		}

		if (channel < 0 || channel >= mChannels.size())
		{
			FAIL("Mix_HaltChannel called with an out-of-bounds channel");
		}

		mChannels[channel].mChunk = nullptr;
		mChannels[channel].mPlaying = false;
	}

	void Pause(int channel)
	{
		if (channel == -1)
		{
			FAIL("Mix_Pause should not be called with a channel of -1");
		}

		if (channel < 0 || channel >= mChannels.size())
		{
			FAIL("Mix_Pause called with an out-of-bounds channel");
		}

		mChannels[channel].mPaused = true;
	}

	void Resume(int channel)
	{
		if (channel == -1)
		{
			FAIL("Mix_Resume should not be called with a channel of -1");
		}

		if (channel < 0 || channel >= mChannels.size())
		{
			FAIL("Mix_Resume called with an out-of-bounds channel");
		}

		mChannels[channel].mPaused = false;
	}

	int Playing(int channel) { return mChannels[channel].mPlaying ? 1 : 0; }

	Mix_Chunk* LoadWAV(const char* file)
	{
		Mix_Chunk* chunk = new Mix_Chunk{file};
		mChunks.emplace(chunk);
		return chunk;
	}

	struct ChannelInfo
	{
		Mix_Chunk* mChunk = nullptr;
		bool mPlaying = false;
		bool mPaused = false;
		int mLoops = 0;
	};

	int mDevID = -1;
	int* mSpec = nullptr;

	std::set<Mix_Chunk*> mChunks;
	std::vector<ChannelInfo> mChannels;

	static Mock Mixer;
};

inline bool Mix_OpenAudio(int devid, int *spec)
{
	Mock::Mixer.Reset();
	Mock::Mixer.OpenAudio(devid, spec);
	return 0;
}

inline int Mix_AllocateChannels(int numchans)
{
	Mock::Mixer.AllocateChannels(numchans);
	return 0;
}

inline void Mix_FreeChunk(Mix_Chunk* chunk)
{
	Mock::Mixer.FreeChunk(chunk);
}

inline void Mix_CloseAudio()
{
	Mock::Mixer.CloseAudio();
}

inline int Mix_PlayChannel(int channel, Mix_Chunk* chunk, int loops)
{
	return Mock::Mixer.PlayChannel(channel, chunk, loops);
}

inline int Mix_HaltChannel(int channel)
{
	Mock::Mixer.HaltChannel(channel);
	return 0;
}

inline void Mix_Pause(int channel)
{
	Mock::Mixer.Pause(channel);
}

inline void Mix_Resume(int channel)
{
	Mock::Mixer.Resume(channel);
}

inline int Mix_Playing(int channel)
{
	return Mock::Mixer.Playing(channel);
}

inline Mix_Chunk* Mix_LoadWAV(const char* file)
{
	return Mock::Mixer.LoadWAV(file);
}

inline const char* Mix_GetError()
{
	return "stubbed error";
}
