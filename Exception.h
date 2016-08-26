/*
*
* Copyright (C) 2016 OtherCrashOverride@users.noreply.github.com.
* All rights reserved.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2, as
* published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
* FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
* more details.
*
*/

#pragma once

#include <exception>
#include <cstdio>


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

class InvalidOperationException : Exception
{
public:
	InvalidOperationException()
	{}

	InvalidOperationException(const char* message)
		: Exception(message)
	{}
};