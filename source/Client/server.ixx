module;

// workaround to intellisense that might be not as smart as we thought
//
#if __INTELLISENSE__
#include <utility>
#include <Windows.h>
#include <functional>
#endif

export module server;

#ifndef __INTELLISENSE__
import <utility>;
import <Windows.h>;
import <functional>;
#endif

import console;
import settings;

#define MAX_ENTITIES    160  // 10 - 2 = 8, 8 * 20 = 160

export typedef enum
{
	UP,
	DOWN,
	LEFT,
	RIGHT
} FACING;

export typedef enum
{
	ENTITY_TYPE_OBSTACLE,
	ENTITY_TYPE_CAR,
	ENTITY_TYPE_FROG,
	ENTITY_TYPE_MAX
} ENTITY_TYPE;

export typedef struct
{
	ENTITY_TYPE type;
	FACING direction;
	int pos_x, pos_y;
} ENTITY;

export typedef enum
{
	SINGLEPLAYER,
	MULTIPLAYER
} GAME_TYPE;

export typedef enum
{
	MOVE,
	JOIN,
	LEAVE,
	UPDATE
} GAME_INFO_TYPE;

export enum GAME_STATE
{
	GAME_STATE_READY,
	GAME_STATE_RUNNING,
	GAME_STATE_PLAYER1_WINS,
	GAME_STATE_PLAYER2_WINS,
	GAME_STATE_DRAW,
	GAME_STATE_LOSS,
	GAME_STATE_MAX
};

export typedef struct
{
	DWORD pid;
	GAME_INFO_TYPE type;

	union
	{
		struct
		{
			FACING direction;
		} move;

		struct
		{
			GAME_TYPE type;
		} join;
	};
} GAME_PIPE_IN;

export typedef struct
{
	GAME_STATE state;
	int time, level;
	int width, height;
	int num_entities;
	ENTITY entities[ MAX_ENTITIES ];
} DATA;

export typedef struct
{
	GAME_INFO_TYPE type;

	union
	{
		DATA data;
		bool status;
	};
} GAME_PIPE_OUT;

export class Server
{
private:
	inline static constexpr auto game_pipe = TEXT( "\\\\.\\pipe\\CRR_PIPE_GAME" );

	inline static constexpr auto close_event = TEXT( "Local\\CRR_SERVER_CLOSE_EVENT" );

	HANDLE h_event = nullptr;

	bool is_playing = false;

	HANDLE h_pipe = nullptr, h_thread = nullptr;

	HANDLE h_exit_thread = nullptr;

	bool send_keys = false;
	FACING send_dir = UP;

	std::function<void( const DATA& data )> on_update_callback = nullptr;

public:
	Server( )
	{
		console::log( TEXT( "Server Constructor" ) );

		h_event = OpenEvent( SYNCHRONIZE, false, close_event );
		if ( !h_event )
		{
			console::log( TEXT( "OpenEvent failed: " ), GetLastError( ) );
			return;
		}

		h_exit_thread = CreateThread( nullptr, NULL, reinterpret_cast<LPTHREAD_START_ROUTINE>( exitRoutine ), this, NULL, nullptr );
		if ( !h_exit_thread )
		{
			console::log( TEXT( "CreateThread failed: " ), GetLastError( ) );

			CloseHandle( h_event );
			h_event = nullptr;
			return;
		}
	}

	~Server( )
	{
		if ( h_exit_thread )
		{
			TerminateThread( h_exit_thread, 0 );
			h_exit_thread = nullptr;
		}

		leaveMatch( );

		if ( h_event )
		{
			CloseHandle( h_event );

			h_event = nullptr;
		}

		console::log( TEXT( "Server Destructor" ) );
	}

	bool isConnected( )
	{
		return h_event;
	}

	bool isPlaying( )
	{
		return is_playing;
	}

	bool joinMatch( GAME_TYPE type )
	{
		if ( !isConnected( ) || isPlaying( ) )
			return false;

		h_pipe = CreateFile( game_pipe, GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, NULL, nullptr );
		if ( h_pipe == INVALID_HANDLE_VALUE )
		{
			h_pipe = nullptr;
			return false;
		}

		GAME_PIPE_IN in;
		in.pid = GetCurrentProcessId( );
		in.type = JOIN;
		in.join.type = type;
		if ( !WriteFile( h_pipe, &in, sizeof( in ), nullptr, nullptr ) )
		{
			console::log( TEXT( "WriteFile failed: " ), GetLastError( ) );

			CloseHandle( h_pipe );
			h_pipe = nullptr;
			return false;
		}

		GAME_PIPE_OUT out;
		if ( !ReadFile( h_pipe, &out, sizeof( out ), nullptr, nullptr ) )
		{
			console::log( TEXT( "ReadFile failed: " ), GetLastError( ) );

			CloseHandle( h_pipe );
			h_pipe = nullptr;
			return false;
		}

		if ( out.type != JOIN || !out.status )
		{
			console::log( TEXT( "Server declined request" ), GetLastError( ) );

			CloseHandle( h_pipe );
			h_pipe = nullptr;
			return false;
		}

		is_playing = true;

		h_thread = CreateThread( nullptr, NULL, reinterpret_cast<LPTHREAD_START_ROUTINE>( gameRoutine ), this, NULL, nullptr );
		if ( !h_thread )
		{
			is_playing = false;

			in.type = LEAVE;
			if ( !WriteFile( h_pipe, &in, sizeof( in ), nullptr, nullptr ) )
				console::log( TEXT( "WriteFile failed: " ), GetLastError( ) );

			CloseHandle( h_pipe );
			h_pipe = nullptr;
			return false;
		}

		return true;
	}

	void leaveMatch( )
	{
		if ( !isPlaying( ) || !isConnected( ) )
			return;

		is_playing = false;

		if ( getStatus( h_thread ) == -1 )
			WaitForSingleObjectEx( h_thread, INFINITE, false );

		CloseHandle( h_thread );
		h_thread = nullptr;

		GAME_PIPE_IN in { };
		in.pid = GetCurrentProcessId( );
		in.type = LEAVE;
		if ( !WriteFile( h_pipe, &in, sizeof( in ), nullptr, nullptr ) )
			console::log( TEXT( "WriteFile failed: " ), GetLastError( ) );

		CloseHandle( h_pipe );
		h_pipe = nullptr;
	}

	bool sendMove( FACING direction )
	{
		if ( !isPlaying( ) || !isConnected( ) )
			return false;

		send_dir = direction;
		send_keys = true;

		return true;
	}

	void setOnUpdateCallback( std::function<void( const DATA& data )> callback )
	{
		on_update_callback = callback;
	}

private:
	static DWORD WINAPI exitRoutine( Server* _this )
	{
		DWORD ret_cause = NULL;
		while ( ( ret_cause = WaitForSingleObjectEx( _this->h_event, settings::tick_ms, false ) ) == WAIT_TIMEOUT )
		{
			if ( _this->isConnected( ) )
				continue;

			ret_cause = WAIT_OBJECT_0;
			break;
		}

		if ( ret_cause == WAIT_OBJECT_0 )
			MessageBox( nullptr, TEXT( "Server is shutting down!" ), TEXT( "Server Shutdown" ), MB_OK );

		_this->h_exit_thread = nullptr;
		std::exit( ret_cause != WAIT_OBJECT_0 );
	}

	static DWORD WINAPI gameRoutine( Server* _this )
	{
		GAME_PIPE_OUT out { };
		while ( _this->is_playing )
		{
			if ( _this->send_keys )
			{
				GAME_PIPE_IN in;
				in.pid = GetCurrentProcessId( );
				in.type = MOVE;
				in.move.direction = _this->send_dir;
				if ( !WriteFile( _this->h_pipe, &in, sizeof( in ), nullptr, nullptr ) )
					console::log( TEXT( "WriteFile failed: " ), GetLastError( ) );

				_this->send_keys = false;
			}
			
			DWORD available;
			if ( !PeekNamedPipe( _this->h_pipe, nullptr, NULL, nullptr, &available, nullptr ) )
			{
				console::log( TEXT( "PeekNamedPipe failed: " ), GetLastError( ) );
				continue;
			}

			if ( available < sizeof( out ) )
				continue;

			if ( !ReadFile( _this->h_pipe, &out, sizeof( out ), nullptr, nullptr ) )
			{
				console::log( TEXT( "ReadFile failed: " ), GetLastError( ) );
				continue;
			}

			if ( out.type != UPDATE )
				continue;

			if ( !_this->on_update_callback )
				continue;

			_this->on_update_callback( out.data );
		}

		return 0;
	}

	DWORD getStatus( HANDLE h_thread )
	{
		DWORD status = NULL;
		GetExitCodeThread( h_thread, &status );
		switch ( status )
		{
		case STILL_ACTIVE:
			return -1;
		case NULL:
			return 1;
		default:
			return status;
		}
	}
};