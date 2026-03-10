#pragma once
#include <memory>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include "UScene.h"

class SceneRegistry
{
public:
    using CreateFunc = std::unique_ptr<UScene>(*)();

    struct Entry
    {
        std::string Name;
        std::type_index Type;
        CreateFunc Creator;
    };

public:
    static SceneRegistry& Get()
    {
        static SceneRegistry Instance;
        return Instance;
    }

    template<typename T>
    bool Register(const std::string& Name)
    {
        static_assert(std::is_base_of_v<UScene, T>, "T must derive from UScene");

        const std::type_index Type = std::type_index(typeid(T));

        if (NameToIndex.contains(Name))
        {
            return false; // 이름 중복
        }

        if (TypeToIndex.contains(Type))
        {
            return false; // 타입 중복 등록
        }

        Entry NewEntry{
            Name,
            Type,
            []() -> std::unique_ptr<UScene>
            {
                return std::make_unique<T>();
            }
        };

        const size_t Index = Entries.size();
        Entries.push_back(std::move(NewEntry));
        NameToIndex.emplace(Name, Index);
        TypeToIndex.emplace(Type, Index);

        return true;
    }

    const std::vector<Entry>& GetEntries() const
    {
        return Entries;
    }

    std::unique_ptr<UScene> CreateSceneByName(const std::string& Name) const
    {
        auto It = NameToIndex.find(Name);
        if (It == NameToIndex.end())
        {
            return nullptr;
        }

        return Entries[It->second].Creator();
    }

    const Entry* FindByName(const std::string& Name) const
    {
        auto It = NameToIndex.find(Name);
        if (It == NameToIndex.end())
        {
            return nullptr;
        }

        return &Entries[It->second];
    }

    template<typename T>
    const Entry* FindByType() const
    {
        static_assert(std::is_base_of_v<UScene, T>, "T must derive from UScene");

        auto It = TypeToIndex.find(std::type_index(typeid(T)));
        if (It == TypeToIndex.end())
        {
            return nullptr;
        }

        return &Entries[It->second];
    }

private:
    SceneRegistry() = default;

private:
    std::vector<Entry> Entries;
    std::unordered_map<std::string, size_t> NameToIndex;
    std::unordered_map<std::type_index, size_t> TypeToIndex;
};