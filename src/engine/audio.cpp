#include "audio.h"
#include <lsk/lsk_file.h>

bool AudioManager::init()
{
	_soloud.init();
	_soundData.init(16);
	_playlist.init(32);

	return true;
}

void AudioManager::destroy()
{
	_soundData.destroy();
	_playlist.destroy();
	_soloud.deinit();
}

void AudioManager::play(u32 soundNameHash)
{
	assert(_soundData.geth(soundNameHash));
	_playlist.push(soundNameHash);
}

void AudioManager::update()
{
	for(u32 soundNameHash: _playlist) {
		_soloud.play(*_soundData.geth(soundNameHash), 0.5f);
	}

	_playlist.clear();
}

bool AudioManager::loadFromDisk(const char* path, u32 soundNameHash)
{
	SoLoud::Wav& wav = _soundData.seth(soundNameHash, SoLoud::Wav());
	if(wav.load(path) == 0) {
		lsk_succf("AudioManager::loadFromDisk(%s): load success", path);
		return true;
	}
	lsk_errf("AudioManager::loadFromDisk(%s): could not read file", path);
	return false;
}

bool AudioManager::loadFromMem(u8* data, u32 dataSize, u32 soundNameHash)
{
	SoLoud::Wav& wav = _soundData.seth(soundNameHash, SoLoud::Wav());
	if(wav.loadMem(data, dataSize, true, true) == 0) {
		lsk_succf("AudioManager::loadFromMem(): load success");
		return true;
	}
	lsk_errf("AudioManager::loadFromMem(): could not read memory");
	return false;
}
