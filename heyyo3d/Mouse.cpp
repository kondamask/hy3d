#include "Mouse.h"

Mouse::Event::Event()
	:
	type(Mouse::Event::Type::Invalid),
	leftIsPressed(false),
	rightIsPressed(false),
	x(0),
	y(0)
{
}

Mouse::Event::Event(Type type, const Mouse& parent)
	:
	type(type),
	leftIsPressed(parent.leftIsPressed),
	rightIsPressed(parent.rightIsPressed),
	x(parent.x),
	y(parent.y)
{
}

Mouse::Event::Type Mouse::Event::GetType() const
{
	return type;
}

bool Mouse::Event::IsValid() const
{
	return (type != Mouse::Event::Type::Invalid);
}

bool Mouse::Event::LeftIsPressed() const
{
	return leftIsPressed;
}

bool Mouse::Event::RightIsPressed() const
{
	return rightIsPressed;
}

std::pair<int, int> Mouse::Event::GetPos() const
{
	return std::make_pair(x, y);
}

int Mouse::Event::GetPosX() const
{
	return x;
}

int Mouse::Event::GetPosY() const
{
	return y;
}

bool Mouse::LeftIsPressed() const
{
	return leftIsPressed;
}

bool Mouse::RightIsPressed() const
{
	return rightIsPressed;
}

bool Mouse::IsEmpty() const
{
	return !e.IsValid();
}

std::pair<int, int> Mouse::GetPos() const
{
	return std::make_pair(x, y);
}

int Mouse::GetPosX() const
{
	return x;
}

int Mouse::GetPosY() const
{
	return y;
}

Mouse::Event Mouse::Read()
{
	auto temp = e;
	Clear();
	return temp;
}

void Mouse::Clear()
{
	e = Mouse::Event();
}

bool Mouse::IsInWindow()
{
	return isInWindow;
}

void Mouse::OnLeftPress(int x, int y)
{
	leftIsPressed = true;
	e = Mouse::Event(Mouse::Event::Type::LPress, *this);
}

void Mouse::OnLeftRelease(int x, int y)
{
	leftIsPressed = false;
	e = Mouse::Event(Mouse::Event::Type::LRelease, *this);
}

void Mouse::OnRightPress(int x, int y)
{
	rightIsPressed = true;
	e = Mouse::Event(Mouse::Event::Type::RPress, *this);
}

void Mouse::OnRightRelease(int x, int y)
{
	rightIsPressed = false;
	e = Mouse::Event(Mouse::Event::Type::RRelease, *this);
}

void Mouse::OnMove(int x_in, int y_in)
{
	x = x_in;
	y = y_in;
	e = Mouse::Event(Mouse::Event::Type::Move, *this);
}

void Mouse::OnEnter()
{
	isInWindow = true;
	e = Mouse::Event(Mouse::Event::Type::Enter, *this);
}

void Mouse::OnLeave()
{
	isInWindow = false;
	e = Mouse::Event(Mouse::Event::Type::Leave, *this);
}

void Mouse::OnWheelUp(int x, int y)
{
	e = Mouse::Event(Mouse::Event::Type::WheelUp, *this);
}

void Mouse::OnWheelDown(int x, int y)
{
	e = Mouse::Event(Mouse::Event::Type::WheelDown, *this);
}

#include "Windows.h"
void Mouse::OnWheelDelta(int x, int y, int wheelDelta_in)
{
	wheelDelta += wheelDelta_in;
	while (wheelDelta >= WHEEL_DELTA)
	{
		wheelDelta -= WHEEL_DELTA;
		OnWheelUp(x, y);
	}
	while (wheelDelta <= -WHEEL_DELTA)
	{
		wheelDelta += WHEEL_DELTA;
		OnWheelDown(x, y);
	}
}
