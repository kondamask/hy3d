#include "app.h"

int CALLBACK WinMain(
	HINSTANCE instance, 
	HINSTANCE prevInstance, 
	LPSTR lpCmdLine, 
	int nShowCmd)
{
	return App{}.Go();
}