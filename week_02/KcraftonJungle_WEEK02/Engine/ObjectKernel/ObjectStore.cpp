#include "ObjectStore.h"

ObjectStore::ObjectStore()
{
}

void ObjectStore::Add(std::unique_ptr<UObject> obj)
{
	if (!obj) return;
	Objects.push_back(std::move(obj));
}

void ObjectStore::Remove(uint32 uuid)
{
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
	Objects.clear();
}

UObject* ObjectStore::Find(uint32 uuid) const
{
	for (const auto& obj : Objects)
		if (obj->GetUUID() == uuid) return obj.get();
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

