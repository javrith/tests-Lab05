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
		bool mLooping = false;
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

int Mix_Playing(int channel)
{
	return 0;
}

int Mix_PlayChannel(int channel, Mix_Chunk* chunk, int loops)
{
	return 0;
}

int Mix_HaltChannel(int channel)
{
	return 0;
}

void Mix_Pause(int channel)
{
}

void Mix_Resume(int channel)
{
}

Mix_Chunk* Mix_LoadWAV(const char* file)
{
	return Mock::Mixer.LoadWAV(file);
}

Mock Mock::Mixer;
