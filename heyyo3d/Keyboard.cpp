#include "Keyboard.h"
#include "Mouse.h"

Keyboard::Event::Event()
	: Event(Type::Invalid, 0u)
{
}

Keyboard::Event::Event(Type type, VK_CODE code)
	: type(type), code(code)
{
}

bool Keyboard::Event::IsPress() const
{
	return type == Keyboard::Event::Type::Press;
}

bool Keyboard::Event::IsRelease() const
{
	return type == Keyboard::Event::Type::Release;
}

bool Keyboard::Event::IsValid() const
{
	return !(type == Keyboard::Event::Type::Invalid);
}

VK_CODE Keyboard::Event::GetCode() const
{
	return code;
}

bool Keyboard::KeyIsPressed(VK_CODE code) const
{
	return keyStates[code];
}

bool Keyboard::KeyIsEmpty() const
{
	return !e.IsValid();
}

Keyboard::Event Keyboard::ReadKey()
{
	auto temp = e;
	ClearEvent();
	return temp;
}

bool Keyboard::CharIsEmpty() const
{
	return (c == '\0');
}

char Keyboard::ReadChar()
{
	char temp = c;
	ClearChar();
	return temp;
}

void Keyboard::ClearEvent()
{
	e = Keyboard::Event();
}

void Keyboard::ClearChar()
{
	c = '\0';
}

void Keyboard::Clear()
{
	ClearEvent();
	ClearChar();
}

void Keyboard::EnableAutoRepeat()
{
	autoRepeatEnabled = true;
}

void Keyboard::DisableAutoRepeat()
{
	autoRepeatEnabled = false;
}

bool Keyboard::AutoRepeatEnabled() const
{
	return autoRepeatEnabled;
}

void Keyboard::OnPress(VK_CODE code)
{
	keyStates[code] = true;
	e = Keyboard::Event(Keyboard::Event::Type::Press, code);
}

void Keyboard::OnRelease(VK_CODE code)
{
	keyStates[code] = false;
	e = Keyboard::Event(Keyboard::Event::Type::Release, code);
}

void Keyboard::OnChar(char c_in)
{
	c = c_in;
}

void Keyboard::ClearState()
{
	keyStates.reset();
}
