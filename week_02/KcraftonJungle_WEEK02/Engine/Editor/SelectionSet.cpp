#include "SelectionSet.h"

SelectionSet::SelectionSet()
{
}

void SelectionSet::Select(USceneComponent* comp, bool additive)
{
    if (comp == nullptr) return;
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
        OnSelectionChanged.Broadcast({ GetPrimary() });
    }
}

void SelectionSet::Deselect(USceneComponent* comp)
{
	for (auto it = Items.begin(); it < Items.end(); ++it)
	{
		if (*it == comp)
		{
			Items.erase(it);
			break;
		}
	}
}

void SelectionSet::Clear()
{
	Items.clear();
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
        OnSelectionChanged.Broadcast({ GetPrimary() });
    }
}