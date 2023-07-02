module;

// workaround to intellisense that might be not as smart as we thought
//
#if __INTELLISENSE__
#include <vector>
#include <Windows.h>
#endif

export module ui;

#ifndef __INTELLISENSE__
import <vector>;
import <Windows.h>;
#endif

import console;

export class UI
{
private:
	HANDLE h_console = nullptr;

	CRITICAL_SECTION critical_section { };
		
	COORD game_start_corner { };

	int width_game = 0, height_game = 0;
	int width_commands = 0, height_commands = 0;

	int commands_offset = 1;

public:
	UI( int width_commands, int height_commands, int width_game, int height_game )
	{
		h_console = GetStdHandle( STD_OUTPUT_HANDLE );

		this->width_game = width_game;
		this->height_game = height_game;
		this->width_commands = width_commands;
		this->height_commands = height_commands;

		if ( !onResize( ) )
		{
			console::error( "UI Constructor failed" );
			exit( 1 );
		}

		InitializeCriticalSectionEx( &critical_section, 100, NULL );
	}

	~UI( )
	{
		DeleteCriticalSection( &critical_section );
	}
	
	bool onResize( )
	{
		CONSOLE_SCREEN_BUFFER_INFOEX info;
		info.cbSize = sizeof( CONSOLE_SCREEN_BUFFER_INFOEX );
		if ( !GetConsoleScreenBufferInfoEx( h_console, &info ) )
		{
			console::error( "GetConsoleScreenBufferInfoEx failed: 0x", GetLastError( ) );
			return false;
		}

		if ( info.dwSize.X < width_game || info.dwSize.X < width_commands ||
			info.dwSize.Y < height_game || info.dwSize.Y < height_commands )
		{
			console::error( "Invalid screen size" );
			return false;
		}

		game_start_corner.X = ( info.dwSize.X - width_game ) / 2;
		game_start_corner.Y = height_commands + 1;
		return true;
	}

	void printGame( const std::vector<console::tstring>& lines )
	{
		EnterCriticalSection( &critical_section );

		const auto old_cursor_pos = getCursorPos( );

		auto print_corner = game_start_corner;
		for ( auto line = lines.rbegin( ); line != lines.rend(); line++ )
		{
			setCursorPos( print_corner );

			console::print( line->c_str( ) );

			print_corner.Y++;
		}

		setCursorPos( old_cursor_pos );

		LeaveCriticalSection( &critical_section );
	}

	console::tstring get( const console::tstring& text = TEXT( "Command: " ) )
	{
		EnterCriticalSection( &critical_section );

		const auto old_cursor_pos = getCursorPos( );

		COORD print_corner;
		print_corner.X = 0;
		print_corner.Y = 3;

		setCursorPos( print_corner );

		clearLine( );
		console::print( text );

		LeaveCriticalSection( &critical_section );

		console::tstring command;
		console::get( command );

		EnterCriticalSection( &critical_section );

		setCursorPos( old_cursor_pos );

		LeaveCriticalSection( &critical_section );

		return command;
	}

	void printToPrompt( console::tstring string )
	{
		EnterCriticalSection( &critical_section );

		const auto old_cursor_pos = getCursorPos( );

		COORD print_corner;
		print_corner.X = 0;
		print_corner.Y = 3 + ( commands_offset++ );

		if ( commands_offset == 6 )
			commands_offset = 1;

		setCursorPos( print_corner );

		clearLine( );
		console::print( string, "\n" );

		setCursorPos( old_cursor_pos );

		LeaveCriticalSection( &critical_section );
	}

private:
	COORD getCursorPos( )
	{
		CONSOLE_SCREEN_BUFFER_INFOEX info;
		info.cbSize = sizeof( CONSOLE_SCREEN_BUFFER_INFOEX );
		if ( !GetConsoleScreenBufferInfoEx( h_console, &info ) )
		{
			console::error( "GetConsoleScreenBufferInfoEx failed: 0x", GetLastError( ) );
			exit( 1 );
		}

		return info.dwCursorPosition;
	}

	bool setCursorPos( COORD pos )
	{
		return SetConsoleCursorPosition( h_console, pos );
	}

	bool clearLine( )
	{
		COORD temp;
		if ( !getTerminalSize( temp ) )
			return false;

		const auto max = temp.X;

		const auto old_pos = getCursorPos( );
		temp.Y = old_pos.Y;
		temp.X = 0;

		console::tstring str;
		for ( int i = 0; i < max; i++ )
			str.push_back( TEXT( ' ' ) );

		setCursorPos( temp );

		console::print( str );

		setCursorPos( old_pos );

		return true;
	}

	bool getTerminalSize( COORD& coord )
	{
		CONSOLE_SCREEN_BUFFER_INFOEX info;
		info.cbSize = sizeof( CONSOLE_SCREEN_BUFFER_INFOEX );
		if ( !GetConsoleScreenBufferInfoEx( h_console, &info ) )
		{
			console::error( "GetConsoleScreenBufferInfoEx failed: 0x", GetLastError( ) );
			return false;
		}

		coord = info.dwSize;

		return true;
	}
};