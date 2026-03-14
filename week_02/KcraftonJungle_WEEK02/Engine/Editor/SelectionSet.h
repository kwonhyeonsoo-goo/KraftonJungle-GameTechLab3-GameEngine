#pragma once
#include "../Foundation/Core/CoreTypes.h"
#include "../World/USceneComponent.h"

class SelectionSet
{
public:
	SelectionSet();

	void Select(USceneComponent* comp, bool additive = false);
	void Deselect(USceneComponent* comp);
	void Clear();

	bool IsSelected(uint32 uuid) const;
	bool IsEmpty() const;
	uint32 Count() const;

	// Gizmo following component
	USceneComponent* GetPrimary() const;
	const TArray<USceneComponent*>& GetAll() const;

	void OnObjectDestroyed(const ObjectDestroyedEvent& e);
	TDelegate<SelectionChangedEvent> OnSelectionChanged;
private:
	TArray<USceneComponent*> Items;
};

