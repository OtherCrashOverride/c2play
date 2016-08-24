#pragma once

//#include "Event.h"


template<typename T>
using EventFunction = std::function<void(void* sender, const T& args)>;

template<typename T>
class Event;


template<typename T>
class EventListener
{
	friend class Event<T>;

	//Event<T>* source = nullptr;
	EventFunction<T> target;


	//void InvalidateSource()
	//{
	//	source = nullptr;
	//}

public:
	EventListener()
	{
	}

	EventListener(EventFunction<T> target)
		: target(target)
	{
	}

	//EventListener(Event<T>* source, EventFunction<T> target)
	//	: source(source), target(target)
	//{
	//	if (source == nullptr)
	//		throw ArgumentNullException();
	//}

	~EventListener()
	{
		//// Remove event
		//if (source)
		//{
		//	source->RemoveListener(this);
		//}
	}


	void Invoke(void *sender, const T& args)
	{
		if (target)
		{
			target(sender, args);
		}
	}
};

template<typename T>
using EventListenerSPTR = std::shared_ptr<EventListener<T>>;

template<typename T>
using EventListenerWPTR = std::weak_ptr<EventListener<T>>;
