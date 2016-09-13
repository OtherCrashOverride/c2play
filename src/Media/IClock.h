#pragma once

#include <memory>
#include <vector>

#include "Exception.h"



class IClockSink
{
protected:
	IClockSink() {}

public:
	virtual ~IClockSink() {}

	virtual void SetTimeStamp(double value) = 0;
};

typedef std::shared_ptr<IClockSink> IClockSinkSPTR;


class ClockList
{
	std::vector<IClockSinkSPTR> sinks;

public:

	auto begin() -> decltype(sinks.begin())
	{
		return sinks.begin();
	}
	auto end() -> decltype(sinks.end())
	{
		return sinks.end();
	}

	void Add(IClockSinkSPTR item)
	{
		if (!item)
			throw ArgumentNullException();

		sinks.push_back(item);
	}

	void Remove(IClockSinkSPTR item)
	{
		if (!item)
			throw ArgumentNullException();


		bool found = false;
		for (auto iter = sinks.begin(); iter != sinks.end(); ++iter)
		{
			if (*iter == item)
			{
				sinks.erase(iter);
				found = true;
				break;
			}
		}

		if (!found)
			throw InvalidOperationException("The item was not found in the list.");
	}

	void Clear()
	{
		sinks.clear();
	}
};
