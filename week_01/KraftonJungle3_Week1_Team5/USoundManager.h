#pragma once
#include <array>
#include <list>
#include <map>
#include <string>

#include "Enum.h"
#include "UPrimitive.h"

#include "fmod.h"
#include "fmod.hpp"
#pragma comment(lib, "fmod_vc.lib")
#undef PlaySound
class USoundManager : public UPrimitive
{
public:
	USoundManager();
	~USoundManager() override;

	bool Initialize();
	void Release();

	void 			PlaySound(const std::wstring& pSoundKey, CHANNELID eID, float fVolume);
	FMOD_CHANNEL*	PlaySound(const std::wstring& pSoundKey, float fVolume);
	void 			PlayLoopSound(const std::wstring& pSoundKey, CHANNELID eID, float fVolume);
	FMOD_CHANNEL*	PlayLoopSound(const std::wstring& pSoundKey, float fVolume);
	void 			PlayBGM(const std::wstring& pSoundKey, float fVolume);
	void 			StopSound(CHANNELID eID);
	void			StopSound(FMOD_CHANNEL* channel);
	void 			StopAll();
	void 			SetChannelVolume(CHANNELID eID, float fVolume);
	void			SetSystemVolume(float Volume);
	bool 			IsPlaying(CHANNELID eID);
	bool			IsPlaying(FMOD_CHANNEL* channel);
	void 			UpdateChannelList();

private:
	bool LoadSoundFile();
	FMOD_SOUND* FindSound(const std::wstring& pSoundKey) const;
	FMOD_CHANNEL* StartSound(FMOD_SOUND* sound, FMOD_MODE mode, float fVolume);
	bool StartSound(CHANNELID eID, FMOD_SOUND* sound, FMOD_MODE mode, float fVolume);

	// 사운드 리소스 정보를 가지고 있는 객체
	std::map<std::wstring, FMOD_SOUND*> MapSound;

	// FMOD_CHANNEL : 재생하고 있는 사운드를 관리할 객체
	std::array<FMOD_CHANNEL*, MAXCHANNEL> ChannelArray;
	std::list<FMOD_CHANNEL*> ChannelList;

	float SystemVolume = 0.8f;

	FMOD_SYSTEM* System;
};
