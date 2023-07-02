module;

// workaround to intellisense that might be not as smart as we thought
//
#if __INTELLISENSE__
#include <Windows.h>
#endif

export module settings;

#ifndef __INTELLISENSE__
import <Windows.h>;
#endif

import console;

export namespace settings
{
	inline int tick_ms = 15;
}

namespace settings
{
	inline constexpr auto identifier = TEXT("CRR");
}