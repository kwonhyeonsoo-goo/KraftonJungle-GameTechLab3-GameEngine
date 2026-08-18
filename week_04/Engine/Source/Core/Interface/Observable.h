#pragma once
#include "CoreMinimal.h"
#include <memory>
#include <algorithm>

//옵저버
template <typename T>
class IObservable {
public:
	virtual ~IObservable() = default;

	virtual void Update(const T& t) = 0;
};

//주체
template <typename T>
class Subject {
private:
	TArray< std::weak_ptr<IObservable >> observers;

public:
	void Subscribe(std::shared_ptr<IObservable> obs) {
		observers.push_back(obs);
	}

	void Unsubscribe(std::shared_ptr<IObservable<T>> obs) {
		observers.erase(
			std::remove_if(observers.begin(), observers.end(),
				[&](const std::weak_ptr<IObservable<T>>& wptr) {
					return wptr.lock() == obs;
				}),
			observers.end()
		);
	}

	void Notify(const T& t) {
		for (auto it = observers.begin(); it != observers.end(); ) {
			if (auto obs = it->lock()) {
				obs->Update(t);
				++it;
			}
			else {
				it = observers.erase(it);
			}
		}
	}
};