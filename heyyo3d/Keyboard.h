#pragma once
#include <bitset>

using VK_CODE = unsigned char;

class Keyboard
{
	friend class Window;
public:
	
	class Event
	{
	public:
		enum Type
		{
			Press, Release, Invalid
		};

	private:
		Type type;
		VK_CODE code;

	public:
		Event();
		Event(Type type, VK_CODE code);
		bool IsPress() const;
		bool IsRelease() const;
		bool IsValid() const;
		VK_CODE GetCode() const;
	};

public:
	Keyboard() = default;
	Keyboard(const Keyboard&) = delete;
	Keyboard& operator = (const Keyboard&) = delete;

	bool KeyIsPressed(VK_CODE code) const;
	bool KeyIsEmpty() const;
	Keyboard::Event ReadKey();

	bool CharIsEmpty() const;
	char ReadChar();

	void ClearEvent();
	void ClearChar();
	void Clear();

	void EnableAutoRepeat();
	void DisableAutoRepeat();
	bool AutoRepeatEnabled() const;

private:
	void OnPress(VK_CODE code);
	void OnRelease(VK_CODE code);
	void OnChar(char c);
	void ClearState();

	std::bitset<256> keyStates;
	Keyboard::Event e;
	char c;
	bool autoRepeatEnabled = false;
};
