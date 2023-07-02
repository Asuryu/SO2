module;

// workaround to intellisense that might be not as smart as we thought
//
#if __INTELLISENSE__
#include <io.h>
#include <fcntl.h>
#include <iostream>
#include <Windows.h>
#endif

export module console;

#ifndef __INTELLISENSE__
import <io.h>;
import <fcntl.h>;
import <iostream>;
import <Windows.h>;
#endif

namespace console
{
	inline constexpr auto identifier = TEXT( "CRR Dll" );

#ifdef UNICODE
	auto& tcin = std::wcin;
	auto& tcerr = std::wcerr;
	auto& tclog = std::wclog;
	auto& tcout = std::wcout;

	export using tstring = std::wstring;
#else
	auto& tcin = std::cin;
	auto& tcerr = std::cerr;
	auto& tclog = std::clog;
	auto& tcout = std::cout;

	export using tstring = std::string;
#endif

	static const auto initialized = [ ] ( )
	{
#ifdef UNICODE
		if ( _setmode( _fileno( stdin ), _O_WTEXT ) == -1 ||
			_setmode( _fileno( stdout ), _O_WTEXT ) == -1 ||
			_setmode( _fileno( stderr ), _O_WTEXT ) == -1 )
		{
			std::perror( "Error setting Unicode mode" );
			return false;
		}
#endif

		return true;
	}( );
}

export
namespace console
{
	template<typename... T>
	inline void get( T&... args )
	{
		( tcin >> ... >> args );
	}

	template<typename... T>
	inline void error( T... args )
	{
		( tcerr << ... << args ) << std::endl;
	}

	template<typename... T>
	inline void log( T... args )
	{
#ifdef _DEBUG
		tclog << TEXT( "[ " ) << identifier << TEXT( " ] " );
		( tclog << ... << args );
		tclog << std::endl;
#endif
	}

	template<typename... T>
	inline void print( T... args )
	{
		( tcout << ... << args );
	}

	inline void clear()
	{
		system("cls");
	}
}