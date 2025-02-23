#pragma once
#define MIX_DEFAULT_FORMAT 0x8010
#include <string>
#include <set>
#include <vector>

struct Mix_Chunk
{
	std::string mName;
};

struct Mock
{
	void OpenAudio(int frequency, Uint16 format, int channels, int chunksize)
	{
		mFrequency = frequency;
		mFormat = format;
		mOutputChannels = channels;
		mChunkSize = chunksize;

		AllocateChannels(4);
	}

	void AllocateChannels(int channels) { mChannels.resize(channels); }

	void Reset()
	{
		mFrequency = 0;
		mFormat = 0;
		mOutputChannels = 0;
		mChunkSize = 0;
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

	int mFrequency = 0;
	Uint16 mFormat = 0;
	int mOutputChannels = 0;
	int mChunkSize = 0;

	std::set<Mix_Chunk*> mChunks;
	std::vector<ChannelInfo> mChannels;

	static Mock Mixer;
};

int Mix_OpenAudio(int frequency, Uint16 format, int channels, int chunksize)
{
	Mock::Mixer.Reset();
	Mock::Mixer.OpenAudio(frequency, format, channels, chunksize);
	return 0;
}

int Mix_AllocateChannels(int numchans)
{
	Mock::Mixer.AllocateChannels(numchans);
	return 0;
}

void Mix_FreeChunk(Mix_Chunk* chunk)
{
	Mock::Mixer.FreeChunk(chunk);
}

void Mix_CloseAudio()
{
	Mock::Mixer.CloseAudio();
}

int Mix_PlayChannel(int channel, Mix_Chunk* chunk, int loops)
{
	return Mock::Mixer.PlayChannel(channel, chunk, loops);
}

int Mix_HaltChannel(int channel)
{
	Mock::Mixer.HaltChannel(channel);
	return 0;
}

void Mix_Pause(int channel)
{
	Mock::Mixer.Pause(channel);
}

void Mix_Resume(int channel)
{
	Mock::Mixer.Resume(channel);
}

int Mix_Playing(int channel)
{
	return Mock::Mixer.Playing(channel);
}

Mix_Chunk* Mix_LoadWAV(const char* file)
{
	return Mock::Mixer.LoadWAV(file);
}

const char* Mix_GetError()
{
	return "stubbed error";
}

Mock Mock::Mixer;
