#include "USoundManager.h"

USoundManager::USoundManager()
{
	
}

USoundManager::~USoundManager()
{
}

void USoundManager::Initialize()
{
}

void USoundManager::Release()
{
}

void USoundManager::PlaySound(const std::wstring& pSoundKey, CHANNELID eID, float fVolume)
{
}

FMOD_CHANNEL* USoundManager::PlaySound(const std::wstring& pSoundKey, float fVolume)
{
}

void USoundManager::PlayLoopSound(const std::wstring& pSoundKey, CHANNELID eID, float fVolume)
{
}

FMOD_CHANNEL* USoundManager::PlayLoopSound(const std::wstring& pSoundKey, float fVolume)
{
}

void USoundManager::PlayBGM(const std::wstring& pSoundKey, float fVolume)
{
}

void USoundManager::StopSound(CHANNELID eID)
{
}

void USoundManager::StopSound(FMOD_CHANNEL* channel)
{
}

void USoundManager::StopAll()
{
}

void USoundManager::SetChannelVolume(CHANNELID eID, float fVolume)
{
}

bool USoundManager::IsPlaying(CHANNELID eID)
{
}

bool USoundManager::IsPlaying(FMOD_CHANNEL* channel)
{
}

void USoundManager::UpdateChannelList()
{
}
