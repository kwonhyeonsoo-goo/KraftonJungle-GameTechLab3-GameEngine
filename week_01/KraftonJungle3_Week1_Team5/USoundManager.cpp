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
	FMOD_RESULT result = FMOD_System_Create(&System, FMOD_VERSION);
	if (result != FMOD_OK)
	{
		return false;
	}

	// 1. 시스템 포인터, 2. 사용할 가상채널 수 , 초기화 방식) 
	result = FMOD_System_Init(System, 512, FMOD_INIT_NORMAL, nullptr);
	if (result != FMOD_OK)
	{
		return false;
	}

	return LoadSoundFile();
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

void USoundManager::PlaySound(const std::wstring& pSoundKey, CHANNELID eID, float fVolume)
{
	auto iter = MapSound.find(pSoundKey);
	if (iter == MapSound.end())
	{
		return;
	}

	FMOD_BOOL bPlay = FALSE;

	if (FMOD_Channel_IsPlaying(ChannelArray[eID], &bPlay))
	{
		FMOD_System_PlaySound(System, iter->second, nullptr, FALSE, &ChannelArray[eID]);
	}

	FMOD_Channel_SetMode(ChannelArray[eID], FMOD_DEFAULT);
	FMOD_Channel_SetVolume(ChannelArray[eID], fVolume);

	FMOD_System_Update(System);
	UpdateChannelList();
}

FMOD_CHANNEL* USoundManager::PlaySound(const std::wstring& pSoundKey, float fVolume)
{
	auto iter = MapSound.find(pSoundKey);
	if (iter == MapSound.end())
		return nullptr;

	FMOD_CHANNEL* channel;

	FMOD_System_PlaySound(System, iter->second, nullptr, FALSE, &channel);
	FMOD_Channel_SetMode(channel, FMOD_DEFAULT);
	FMOD_Channel_SetVolume(channel, fVolume);
	ChannelList.push_back(channel);

	FMOD_System_Update(System);
	UpdateChannelList();
	return channel;
}

void USoundManager::PlayLoopSound(const std::wstring& pSoundKey, CHANNELID eID, float fVolume)
{
	auto iter = MapSound.find(pSoundKey);
	if (iter == MapSound.end())
		return;

	FMOD_BOOL bPlay = FALSE;

	if (FMOD_Channel_IsPlaying(ChannelArray[eID], &bPlay))
	{
		FMOD_System_PlaySound(System, iter->second, nullptr, FALSE, &ChannelArray[eID]);
	}

	FMOD_Channel_SetMode(ChannelArray[eID], FMOD_LOOP_NORMAL);
	FMOD_Channel_SetVolume(ChannelArray[eID], fVolume);
	FMOD_System_Update(System);
	UpdateChannelList();
}

FMOD_CHANNEL* USoundManager::PlayLoopSound(const std::wstring& pSoundKey, float fVolume)
{
	auto iter = MapSound.find(pSoundKey);
	if (iter == MapSound.end())
		return nullptr;

	FMOD_CHANNEL* channel;

	FMOD_System_PlaySound(System, iter->second, nullptr, FALSE, &channel);
	FMOD_Channel_SetMode(channel, FMOD_LOOP_NORMAL);
	FMOD_Channel_SetVolume(channel, fVolume);
	FMOD_System_Update(System);
	ChannelList.push_back(channel);
	UpdateChannelList();

	return channel;
}

void USoundManager::PlayBGM(const std::wstring& pSoundKey, float fVolume)
{
	auto iter = MapSound.find(pSoundKey);
	if (iter == MapSound.end())
		return;

	FMOD_System_PlaySound(System, iter->second, nullptr, FALSE, &ChannelArray[SOUND_BGM]);
	FMOD_Channel_SetMode(ChannelArray[SOUND_BGM], FMOD_LOOP_NORMAL);
	FMOD_Channel_SetVolume(ChannelArray[SOUND_BGM], fVolume);
	FMOD_System_Update(System);
	UpdateChannelList();
}

void USoundManager::StopSound(CHANNELID eID)
{
	FMOD_Channel_Stop(ChannelArray[eID]);
}

void USoundManager::StopSound(FMOD_CHANNEL* channel)
{
	FMOD_Channel_Stop(channel);

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
		FMOD_Channel_Stop(ChannelArray[i]);

	for (auto channel : ChannelList)
	{
		FMOD_Channel_Stop(channel);
	}
	FMOD_System_Update(System);
	UpdateChannelList();
}

void USoundManager::SetChannelVolume(CHANNELID eID, float fVolume)
{
	FMOD_Channel_SetVolume(ChannelArray[eID], fVolume);

	FMOD_System_Update(System);
}

bool USoundManager::IsPlaying(CHANNELID eID)
{
	FMOD_BOOL bPlay = FALSE;

	FMOD_Channel_IsPlaying(ChannelArray[eID], &bPlay);

	if (bPlay == 1) return true;
	return false;
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
		FMOD_BOOL isPlaying;

		FMOD_Channel_IsPlaying((*it), &isPlaying);

		if (!isPlaying)
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
	// _finddata_t : <io.h>에서 제공하며 파일 정보를 저장하는 구조체
	_finddata_t fd;

	// _findfirst : <io.h>에서 제공하며 사용자가 설정한 경로 내에서 가장 첫 번째 파일을 찾는 함수
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

		// "./Sound/" + "Success.wav"
		strcat_s(szFullPath, fd.name);
		// "./Sound/Success.wav"

		FMOD_SOUND* pSound = nullptr;

		FMOD_RESULT eRes = FMOD_System_CreateSound(System, szFullPath, FMOD_DEFAULT, nullptr, &pSound);

		if (eRes == FMOD_OK)
		{
			int len = MultiByteToWideChar(CP_ACP, 0, fd.name, -1, nullptr, 0);
			std::wstring soundKey(len - 1, L'\0');
			MultiByteToWideChar(CP_ACP, 0, fd.name, -1, soundKey.data(), len);

			MapSound.emplace(soundKey, pSound);
		}
		//_findnext : <io.h>에서 제공하며 다음 위치의 파일을 찾는 함수, 더이상 없다면 -1을 리턴
		iResult = _findnext(handle, &fd);
	}

	FMOD_System_Update(System);

	_findclose(handle);

	return true;
}