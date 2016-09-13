#pragma once


struct Int32Rectangle
{
public:
	int X;
	int Y;
	int Width;
	int Height;



	int Left() const
	{
		return X;
	}

	int Right() const
	{
		return X + Width;
	}

	int Top() const
	{
		return Y;
	}

	int Bottom() const
	{
		return Y + Height;
	}



	Int32Rectangle()
		: X(0), Y(0), Width(0), Height(0)
	{
	}

	Int32Rectangle(int x, int y, int width, int height)
		: X(x), Y(y), Width(width), Height(height)
	{
	}
};



struct Rectangle
{
public:
	float X;
	float Y;
	float Width;
	float Height;



	float Left() const
	{
		return X;
	}

	float Right() const
	{
		return X + Width;
	}

	float Top() const
	{
		return Y;
	}

	float Bottom() const
	{
		return Y + Height;
	}



	Rectangle()
		: X(0), Y(0), Width(0), Height(0)
	{
	}

	Rectangle(float x, float y, float width, float height)
		: X(x), Y(y), Width(width), Height(height)
	{
	}
};
