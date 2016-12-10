#pragma once
#include <lsk/lsk_types.h>
#include <lsk/lsk_array.h>
#include <lsk/lsk_allocator.h>
#include <soloud.h>
#include <soloud_wav.h>

struct AudioManager
{
	SINGLETON_IMP(AudioManager)

	SoLoud::Soloud _soloud;
	// TODO: limit sound data memory space
	lsk_DStrHashMap<SoLoud::Wav> _soundData;
	lsk_DArray<u32> _playlist;

	bool init();
	void destroy();

	void play(u32 soundNameHash);
	void update();

	bool loadFromDisk(const char* path, u32 soundNameHash);
	bool loadFromMem(u8* path, u32 dataSize, u32 soundNameHash);
	//void loadSoundsToPlay(u32* soundNameHashes, u32 count);
};

// TODO: find better names for all those singletons
// also do they have to be singletons?
#define AudioGet AudioManager::get()
