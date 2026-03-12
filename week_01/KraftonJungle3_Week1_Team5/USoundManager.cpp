#include "USoundManager.h"

#include <algorithm>
#include <corecrt_io.h>

#define WIN32_LEAN_AND_MEAN
#include <ranges>
#include <Windows.h>

USoundManager::USoundManager() : ChannelArray(), System(nullptr)
{
}

USoundManager::~USoundManager()
{
	Release();
}

bool USoundManager::Initialize()
{
	Release();

	FMOD_RESULT result = FMOD_System_Create(&System, FMOD_VERSION);
	if (result != FMOD_OK)
	{
		return false;
	}

	// 1. 시스템 포인터, 2. 사용할 가상채널 수 , 초기화 방식) 
	result = FMOD_System_Init(System, 512, FMOD_INIT_NORMAL, nullptr);
	if (result != FMOD_OK)
	{
		FMOD_System_Release(System);
		System = nullptr;
		return false;
	}

	if (!LoadSoundFile())
	{
		Release();
		return false;
	}

	return true;
}

void USoundManager::Release()
{
	for (const auto& val : MapSound | std::views::values)
	{
		FMOD_Sound_Release(val);
	}
	MapSound.clear();

	if (System)
	{
		FMOD_System_Close(System);
		FMOD_System_Release(System);
		System = nullptr;
	}

	ChannelList.clear();
	ChannelArray.fill(nullptr);
}

#undef PlaySound

FMOD_SOUND* USoundManager::FindSound(const std::wstring& pSoundKey) const
{
	const auto iter = MapSound.find(pSoundKey);
	if (iter == MapSound.end())
	{
		return nullptr;
	}

	return iter->second;
}

FMOD_CHANNEL* USoundManager::StartSound(FMOD_SOUND* sound, FMOD_MODE mode, float fVolume)
{
	if (!System || !sound)
	{
		return nullptr;
	}

	FMOD_CHANNEL* channel = nullptr;
	const FMOD_RESULT playResult = FMOD_System_PlaySound(System, sound, nullptr, TRUE, &channel);
	if (playResult != FMOD_OK || !channel)
	{
		return nullptr;
	}

	const FMOD_RESULT modeResult = FMOD_Channel_SetMode(channel, mode);
	const FMOD_RESULT volumeResult = FMOD_Channel_SetVolume(channel, fVolume);
	const FMOD_RESULT pauseResult = FMOD_Channel_SetPaused(channel, FALSE);

	if (modeResult != FMOD_OK || volumeResult != FMOD_OK || pauseResult != FMOD_OK)
	{
		FMOD_Channel_Stop(channel);
		return nullptr;
	}

	return channel;
}

bool USoundManager::StartSound(CHANNELID eID, FMOD_SOUND* sound, FMOD_MODE mode, float fVolume)
{
	if (ChannelArray[eID])
	{
		FMOD_Channel_Stop(ChannelArray[eID]);
		ChannelArray[eID] = nullptr;
	}

	FMOD_CHANNEL* channel = StartSound(sound, mode, fVolume);
	if (!channel)
	{
		return false;
	}

	ChannelArray[eID] = channel;
	return true;
}

void USoundManager::PlaySound(const std::wstring& pSoundKey, CHANNELID eID, float fVolume)
{
	FMOD_SOUND* sound = FindSound(pSoundKey);
	if (!sound)
	{
		return;
	}

	if (!StartSound(eID, sound, FMOD_DEFAULT, fVolume))
	{
		return;
	}

	FMOD_System_Update(System);
	UpdateChannelList();
}

FMOD_CHANNEL* USoundManager::PlaySound(const std::wstring& pSoundKey, float fVolume)
{
	FMOD_SOUND* sound = FindSound(pSoundKey);
	if (!sound)
	{
		return nullptr;
	}

	FMOD_CHANNEL* channel = StartSound(sound, FMOD_DEFAULT, fVolume);
	if (!channel)
	{
		return nullptr;
	}

	ChannelList.push_back(channel);

	FMOD_System_Update(System);
	UpdateChannelList();
	return channel;
}

void USoundManager::PlayLoopSound(const std::wstring& pSoundKey, CHANNELID eID, float fVolume)
{
	FMOD_SOUND* sound = FindSound(pSoundKey);
	if (!sound)
	{
		return;
	}

	if (!StartSound(eID, sound, FMOD_LOOP_NORMAL, fVolume))
	{
		return;
	}
	FMOD_System_Update(System);
	UpdateChannelList();
}

FMOD_CHANNEL* USoundManager::PlayLoopSound(const std::wstring& pSoundKey, float fVolume)
{
	FMOD_SOUND* sound = FindSound(pSoundKey);
	if (!sound)
	{
		return nullptr;
	}

	FMOD_CHANNEL* channel = StartSound(sound, FMOD_LOOP_NORMAL, fVolume);
	if (!channel)
	{
		return nullptr;
	}

	ChannelList.push_back(channel);
	FMOD_System_Update(System);
	UpdateChannelList();

	return channel;
}

void USoundManager::PlayBGM(const std::wstring& pSoundKey, float fVolume)
{
	FMOD_SOUND* sound = FindSound(pSoundKey);
	if (!sound)
	{
		return;
	}

	if (!StartSound(SOUND_BGM, sound, FMOD_LOOP_NORMAL, fVolume))
	{
		return;
	}

	FMOD_System_Update(System);
	UpdateChannelList();
}

void USoundManager::StopSound(CHANNELID eID)
{
	if (!ChannelArray[eID])
	{
		return;
	}

	FMOD_Channel_Stop(ChannelArray[eID]);
	ChannelArray[eID] = nullptr;
}

void USoundManager::StopSound(FMOD_CHANNEL* channel)
{
	if (!channel)
	{
		return;
	}

	FMOD_Channel_Stop(channel);

	for (auto& fixedChannel : ChannelArray)
	{
		if (fixedChannel == channel)
		{
			fixedChannel = nullptr;
		}
	}

	for (auto it = ChannelList.begin(); it != ChannelList.end();)
	{
		if (*it == channel)
		{
			it = ChannelList.erase(it);
		}

		else
		{
			++it;
		}
	}
}

void USoundManager::StopAll()
{
	for (int i = 0; i < MAXCHANNEL; ++i)
	{
		if (ChannelArray[i])
		{
			FMOD_Channel_Stop(ChannelArray[i]);
		}

		ChannelArray[i] = nullptr;
	}

	for (auto channel : ChannelList)
	{
		if (channel)
		{
			FMOD_Channel_Stop(channel);
		}
	}

	ChannelList.clear();

	if (System)
	{
		FMOD_System_Update(System);
	}
}

void USoundManager::SetChannelVolume(CHANNELID eID, float fVolume)
{
	if (!System || !ChannelArray[eID])
	{
		return;
	}

	FMOD_Channel_SetVolume(ChannelArray[eID], fVolume);

	FMOD_System_Update(System);
}

void USoundManager::SetSystemVolume(float Volume)
{
	if (!System)
	{
		return;
	}

	SystemVolume = Volume;

	for (auto& channel : ChannelArray)
	{
		FMOD_Channel_SetVolume(channel, Volume);
	}

	FMOD_System_Update(System);
}

bool USoundManager::IsPlaying(CHANNELID eID)
{
	if (!ChannelArray[eID])
	{
		return false;
	}

	FMOD_BOOL bPlay = FALSE;
	FMOD_RESULT result = FMOD_Channel_IsPlaying(ChannelArray[eID], &bPlay);

	return (result == FMOD_OK) && (bPlay == TRUE);
}

bool USoundManager::IsPlaying(FMOD_CHANNEL* channel)
{
	if (!channel)
	{
		return false;
	}

	FMOD_BOOL bPlay = FALSE;
	FMOD_RESULT result = FMOD_Channel_IsPlaying(channel, &bPlay);

	return (result == FMOD_OK) && (bPlay == TRUE);
}

void USoundManager::UpdateChannelList()
{
	for (auto it = ChannelList.begin(); it != ChannelList.end();)
	{
		if (!*it)
		{
			it = ChannelList.erase(it);
			continue;
		}

		FMOD_BOOL isPlaying = FALSE;
		FMOD_RESULT result = FMOD_Channel_IsPlaying((*it), &isPlaying);

		if (result != FMOD_OK || !isPlaying)
		{
			it = ChannelList.erase(it);
		}
		else
		{
			++it;
		}
	}
}

bool USoundManager::LoadSoundFile()
{
	if (!System)
	{
		return false;
	}
	_finddata_t fd;

	long long handle = _findfirst("Resource/Sound/*.*", &fd);

	if (handle == -1)
	{
		return false;
	}
		
	int iResult = 0;

	char szCurPath[128] = "Resource/Sound/";	 // 상대 경로
	char szFullPath[128] = "";

	while (iResult != -1)
	{
		strcpy_s(szFullPath, szCurPath);

		strcat_s(szFullPath, fd.name);

		FMOD_SOUND* pSound = nullptr;

		FMOD_RESULT eRes = FMOD_System_CreateSound(System, szFullPath, FMOD_DEFAULT, nullptr, &pSound);

		if (eRes == FMOD_OK)
		{
			int len = MultiByteToWideChar(CP_ACP, 0, fd.name, -1, nullptr, 0);
			std::wstring soundKey(len, L'\0');
			MultiByteToWideChar(CP_ACP, 0, fd.name, -1, soundKey.data(), len);
			soundKey.resize(len - 1);

			MapSound.emplace(soundKey, pSound);
		}
		iResult = _findnext(handle, &fd);
	}

	FMOD_System_Update(System);

	_findclose(handle);

	return true;
}
