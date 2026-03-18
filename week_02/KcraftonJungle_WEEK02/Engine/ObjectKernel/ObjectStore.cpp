#include "ObjectStore.h"

ObjectStore::ObjectStore()
{
}

void ObjectStore::Add(std::unique_ptr<UObject> obj)
{
	if (!obj) return;
	Index[obj->GetUUID()] = obj.get();
	Objects.push_back(std::move(obj));
}

void ObjectStore::Remove(uint32 uuid)
{
	Index.erase(uuid);

	for (auto it = Objects.begin(); it != Objects.end(); ++it)
	{
		if ((*it) != nullptr && (*it)->GetUUID() == uuid)
		{
			Objects.erase(it);
			return;
		}
	}
}

void ObjectStore::Clear()
{
	Index.clear();
	Index.rehash(0);
	Objects.clear();
	Objects.shrink_to_fit();
}

void ObjectStore::ReleaseMemory()
{
	decltype(Index) emptyIndex;
	Index.swap(emptyIndex);

	decltype(Objects) emptyObjects;
	Objects.swap(emptyObjects);
}

void ObjectStore::ClearAndReleaseMemory()
{
	decltype(Index) emptyIndex;
	Index.swap(emptyIndex);

	decltype(Objects) emptyObjects;
	Objects.swap(emptyObjects);
}

UObject* ObjectStore::Find(uint32 uuid) const
{
	auto it = Index.find(uuid);
	if (it != Index.end()) return it->second;
	return nullptr;
}

const TArray<std::unique_ptr<UObject>>& ObjectStore::GUObjectArray() const
{
	return Objects;
}

uint32 ObjectStore::Count() const
{
	return Objects.size();
}

