#include "ObjectStore.h"

ObjectStore::ObjectStore()
{
}

void ObjectStore::Add(UObject* obj)
{
	if (obj == nullptr) return;
	Objects.push_back(obj);
}

void ObjectStore::Remove(uint32 uuid)
{
	for (auto it = Objects.begin(); it != Objects.end(); ++it)
	{
		UObject* obj = *it;
		if (obj != nullptr && obj->GetUUID() == uuid)
		{
			delete obj;
			Objects.erase(it);
			return;
		}
	}
}

void ObjectStore::Clear()
{
	for (auto obj : Objects)
	{
		delete obj;
	}
	Objects.clear();
}

UObject* ObjectStore::Find(uint32 uuid) const
{
	for (auto obj : Objects)
		if (obj->GetUUID() == uuid) return obj;
	return nullptr;
}

const TArray<UObject*>& ObjectStore::GUObjectArray() const
{
	return Objects;
}

uint32 ObjectStore::Count() const
{
	return Objects.size();
}

