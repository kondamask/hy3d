#pragma once
#include <string>
#include <sstream>
#include <utility>

class Mouse
{
	friend class Window;

public:
	class Event
	{
	public:
		enum Type
		{
			LPress, LRelease,
			RPress, RRelease,
			WheelUp, WheelDown,
			Enter, Leave, Move,
			Invalid
		};

	private:
		Type type;
		bool leftIsPressed;
		bool rightIsPressed;
		int x;
		int y;

	public:
		Event();
		Event(Type type, const Mouse& parent);
		
		Type GetType() const;
		bool IsValid() const;
		bool LeftIsPressed() const;
		bool RightIsPressed() const;

		std::pair<int, int> GetPos() const;
		int GetPosX() const;
		int GetPosY() const;
	};

public:
	Mouse() = default;
	Mouse(const Mouse&) = delete;
	Mouse& operator = (const Mouse&) = delete;
	
	bool LeftIsPressed() const;
	bool RightIsPressed() const;
	bool IsEmpty() const;
	std::pair<int, int> GetPos() const;
	int GetPosX() const;
	int GetPosY() const;
	Mouse::Event Read();
	void Clear();
	bool IsInWindow();

private:
	void OnLeftPress(int x, int y);
	void OnLeftRelease(int x, int y);
	void OnRightPress(int x, int y);
	void OnRightRelease(int x, int y);
	void OnMove(int x, int y);
	void OnEnter();
	void OnLeave();
	void OnWheelUp(int x, int y);
	void OnWheelDown(int x, int y);
	void OnWheelDelta(int x, int y, int wheelDelta_in);

	bool leftIsPressed;
	bool rightIsPressed;
	int x;
	int y;
	int wheelDelta;
	bool isInWindow;
	Mouse::Event e;
};

