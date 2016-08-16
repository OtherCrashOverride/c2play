#pragma once

#include <exception>


class Exception : std::exception
{
public:
	Exception()
	{}

	Exception(const char* message)
	{
		printf("%s\n", message);
	}
};



class NotSupportedException : Exception
{
public:
	NotSupportedException()
	{}

	NotSupportedException(const char* message)
		: Exception(message)
	{}
};



class NotImplementedException : Exception
{
public:
	NotImplementedException()
	{}

	NotImplementedException(const char* message)
		: Exception(message)
	{}
};



class ArgumentException : Exception
{
public:
	ArgumentException()
	{}

	ArgumentException(const char* message)
		: Exception(message)
	{}
};



class ArgumentOutOfRangeException : Exception
{
public:
	ArgumentOutOfRangeException()
	{}

	ArgumentOutOfRangeException(const char* message)
		: Exception(message)
	{}
};

class ArgumentNullException : Exception
{
public:
	ArgumentNullException()
	{}

	ArgumentNullException(const char* message)
		: Exception(message)
	{}
};