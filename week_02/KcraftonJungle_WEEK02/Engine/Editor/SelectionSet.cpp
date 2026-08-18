#include "SelectionSet.h"

SelectionSet::SelectionSet()
{
}

void SelectionSet::Select(USceneComponent* comp, bool additive)
{
    if (comp == nullptr)
    {
        Items.clear();
        OnSelectionChanged.Broadcast({ Items });
        return;
    }

    bool bChanged = false;
    if (!additive)
    {
        if (Items.size() != 1 || Items[0] != comp)
        {
            Items.clear();
            Items.push_back(comp);
            bChanged = true;
        }
    }
    else
    {
        bool bFound = false;
        for (auto it = Items.begin(); it != Items.end(); ++it)
        {
            if (*it == comp)
            {
                bFound = true;
                if (it + 1 != Items.end())
                {
                    Items.erase(it);
                    Items.push_back(comp);
                    bChanged = true;
                }
                break;
            }
        }
        if (!bFound)
        {
            Items.push_back(comp);
            bChanged = true;
        }
    }

    if (bChanged)
    {
        OnSelectionChanged.Broadcast({ Items });
    }
}

void SelectionSet::Deselect(USceneComponent* comp)
{
    bool bChanged = false;
    for (auto it = Items.begin(); it != Items.end(); ++it)
    {
        if (*it == comp)
        {
            Items.erase(it);
            bChanged = true;
            break;
        }
    }

    if (bChanged)
    {
        OnSelectionChanged.Broadcast({ Items });
    }
}

void SelectionSet::Clear()
{
    if (Items.empty())
        return;

    Items.clear();
    OnSelectionChanged.Broadcast({ Items });
}

bool SelectionSet::IsSelected(uint32 uuid) const
{
	for (auto it = Items.begin(); it < Items.end(); ++it)
	{
		if ((*it)->GetUUID() == uuid)
		{
			return true;
		}
	}
	return false;
}

bool SelectionSet::IsEmpty() const
{
	return Items.empty();
}

uint32 SelectionSet::Count() const
{
	return static_cast<uint32>(Items.size());
}

USceneComponent* SelectionSet::GetPrimary() const
{
	return Items.empty() ? nullptr : Items.back();
}

const TArray<USceneComponent*>& SelectionSet::GetAll() const
{
	return Items;
}

void SelectionSet::OnObjectDestroyed(const ObjectDestroyedEvent& e)
{
    bool bChanged = false;

    for (auto it = Items.begin(); it != Items.end(); )
    {
        USceneComponent* comp = *it;
        if (comp != nullptr && comp->GetUUID() == e.UUID)
        {
            it = Items.erase(it);
            bChanged = true;
        }
        else
        {
            ++it;
        }
    }

    if (bChanged)
    {
        OnSelectionChanged.Broadcast({ Items });
    }
}