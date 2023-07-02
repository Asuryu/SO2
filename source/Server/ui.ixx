module;

// workaround to intellisense that might be not as smart as we thought
//
#if __INTELLISENSE__
#include <string>
#include <vector>
#include <Windows.h>
#endif

export module ui;

#ifndef __INTELLISENSE__
import <string>;
import <utility>;
import <Windows.h>;
#endif

import console;

export class UI
{
private:
	CRITICAL_SECTION critical_section{ };

public:
	UI()
	{
		InitializeCriticalSectionEx(&critical_section, 200, NULL);
	}

	~UI()
	{
		DeleteCriticalSection(&critical_section);
	}



	console::tstring get(const console::tstring& text = TEXT("Command: "))
	{
		EnterCriticalSection(&critical_section);

		console::print(text);
		console::tstring command;
		console::get(command);

		LeaveCriticalSection(&critical_section);

		return command;
	}

	void printToPrompt(console::tstring string)
	{
		EnterCriticalSection(&critical_section);

		console::print(string, "\n");

		LeaveCriticalSection(&critical_section);
	}
	
};