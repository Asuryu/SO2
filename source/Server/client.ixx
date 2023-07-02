module;

#include "entity/mentity.hpp"

// workaround to intellisense that might be not as smart as we thought
//
#if __INTELLISENSE__
#include <map>
#include <vector>
#include <Windows.h>
#include <functional>
#endif

export module client;

#ifndef __INTELLISENSE__
import <map>;
import <vector>;
import <Windows.h>;
import <functional>;
#endif

import op;
import console;
import settings;

#define MAX_ENTITIES    160  // 10 - 2 = 8, 8 * 20 = 160

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
	GAME_INFO_TYPE type;

	union
	{
		DATA data;
		bool status;
	};
} GAME_PIPE_OUT;

class Client;

typedef struct
{
	Client* _this;
	HANDLE h_pipe;
	DWORD pid;
} CLIENT_INFO;

export typedef enum
{
	ON_PLAYER_JOIN,
	ON_PLAYER_MOVE,
	ON_PLAYER_LEAVE
} CLIENT_CALLBACK_TYPE;

export typedef struct
{
	DWORD pid;
	HANDLE h_thread;
	HANDLE h_pipe;
} CLIENT;

export class Client
{
private:
	inline static constexpr auto num_instances = 2;

	inline static constexpr auto game_pipe = TEXT( "\\\\.\\pipe\\CRR_PIPE_GAME" );

	inline static constexpr auto close_event = TEXT( "Local\\CRR_SERVER_CLOSE_EVENT" );

	HANDLE h_thread = nullptr;
	HANDLE h_event = nullptr;

	DATA data { };

	std::vector<CLIENT> client_threads;

	std::map<CLIENT_CALLBACK_TYPE, std::function<void(DWORD pid, FACING direction)>> callbacks_map;

public:
	Client( )
	{
		console::log( TEXT( "Client Constructor" ) );

		h_event = CreateEvent( nullptr, true, false, close_event );
		if ( !h_event )
			return;

		h_thread = CreateThread( nullptr, NULL, reinterpret_cast<LPTHREAD_START_ROUTINE>( listenerRoutine ), this, NULL, nullptr );
		if ( !h_thread )
		{
			CloseHandle( h_event );
			h_event = nullptr;
			return;
		}
	}

	~Client( )
	{
		if ( h_event )
			SetEvent( h_event );

		if ( h_thread )
		{
			if ( getStatus( h_thread ) == -1 && WaitForSingleObjectEx( h_thread, 200, false ) == WAIT_TIMEOUT )
					TerminateThread( h_thread, 0 );

			CloseHandle( h_thread );
			h_thread = nullptr;
		}

		for ( const auto client : client_threads )
		{
			if ( getStatus( client.h_thread ) == -1 )
				WaitForSingleObjectEx( client.h_thread, INFINITE, false );

			CloseHandle( client.h_thread );
		}

		if ( h_event )
		{
			CloseHandle( h_event );
			h_event = nullptr;
		}

		client_threads.clear( );

		console::log( TEXT( "Client Destructor" ) );
	}

	bool isConnected( )
	{
		return h_event && getStatus( h_thread ) == -1;
	}

	bool registerCallback( CLIENT_CALLBACK_TYPE type, std::function<void( DWORD pid, FACING direction )> callback )
	{
		if ( callbacks_map.find( type ) != callbacks_map.end( ) )
			return false;

		callbacks_map[ type ] = callback;
		return true;
	}

	bool removeCallback( CLIENT_CALLBACK_TYPE type )
	{
		if ( callbacks_map.find( type ) == callbacks_map.end( ) )
			return false;

		callbacks_map.erase( type );
		return true;
	}

	bool update( const std::vector<Entity*>& entities, GAME_STATE state, int time, int level, int width, int height )
	{
		data.time = time;
		data.state = state;
		data.level = level;
		data.width = width;
		data.height = height;
		data.num_entities = 0;
		for ( auto& entity : entities )
		{
			auto entity_data = &data.entities[ data.num_entities++ ];

			auto pos = entity->getPosition( );
			entity_data->pos_x = pos.first;
			entity_data->pos_y = pos.second;

			auto mentity = dynamic_cast<MovingEntity*>( entity );
			entity_data->direction = mentity ? mentity->getFacingDirection( ) : UP;

			const auto type = entity->getType( );
			if ( type >= ENTITY_TYPE::ENTITY_TYPE_OBSTACLE && type < ENTITY_TYPE::ENTITY_TYPE_MAX )
				entity_data->type = type;
			else
			{
				console::error( "update failed: Invalid entity" );
				return false;
			}
		}

		return true;
	}

private:
	static DWORD WINAPI listenerRoutine( Client* _this )
	{
		if ( !_this )
			return 1;

		DWORD bytes = NULL;
		DWORD ret_cause = NULL;
		while ( ( ret_cause = WaitForSingleObjectEx( _this->h_event, settings::tick_ms, false ) ) == WAIT_TIMEOUT )
		{
			if ( _this->client_threads.size( ) == num_instances )
				continue;

			const auto h_pipe = CreateNamedPipe( game_pipe, PIPE_ACCESS_DUPLEX, PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
				PIPE_UNLIMITED_INSTANCES, sizeof( GAME_PIPE_OUT ), sizeof( GAME_PIPE_IN ), NULL, nullptr );
			if ( h_pipe == INVALID_HANDLE_VALUE )
			{
				if ( GetLastError( ) != ERROR_PIPE_BUSY )
					console::log( TEXT( "CreateNamedPipe failed: " ), GetLastError( ) );

				continue;
			}

			if ( !ConnectNamedPipe( h_pipe, nullptr ) )
			{
				console::log( TEXT( "ConnectNamedPipe failed: " ), GetLastError( ) );
				continue;
			}

			GAME_PIPE_IN in;
			if ( !ReadFile( h_pipe, &in, sizeof( in ), nullptr, nullptr ) )
			{
				console::log( TEXT( "ReadFile failed: " ), GetLastError( ) );

				DisconnectNamedPipe( h_pipe );
				continue;
			}

			if ( in.type != JOIN )
			{
				console::log( TEXT( "Invalid Client" ) );

				DisconnectNamedPipe( h_pipe );
				continue;
			}

			bool registered = false;
			for ( const auto client : _this->client_threads )
			{
				if ( client.pid != in.pid )
					continue;

				registered = true;
				break;
			}

			if ( registered )
			{
				console::log( TEXT( "Client already registered" ) );

				DisconnectNamedPipe( h_pipe );
				continue;
			}

			const auto num_clients = _this->client_threads.size( );

			GAME_PIPE_OUT out { };
			out.type = JOIN;
			out.status = ( in.join.type == SINGLEPLAYER && num_clients == 0 ) || ( in.join.type == MULTIPLAYER && num_clients < 2 );
			
			if ( !WriteFile( h_pipe, &out, sizeof( out ), nullptr, nullptr ) )
			{
				console::log( TEXT( "WriteFile failed: " ), GetLastError( ) );

				DisconnectNamedPipe( h_pipe );
				continue;
			}

			if ( !out.status )
			{
				console::log( TEXT( "Request from: " ), in.pid, TEXT( " declined." ) );

				DisconnectNamedPipe( h_pipe );
				continue;
			}

			const auto pinfo = reinterpret_cast<CLIENT_INFO*>( VirtualAlloc( nullptr, sizeof( CLIENT_INFO ), 
				MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE ) );
			if ( !pinfo )
			{
				console::log( TEXT( "VirtualAlloc failed: " ), GetLastError( ) );

				DisconnectNamedPipe( h_pipe );
				continue;
			}

			pinfo->pid = in.pid;
			pinfo->_this = _this;
			pinfo->h_pipe = h_pipe;

			const auto h_thread = CreateThread( nullptr, NULL, reinterpret_cast<LPTHREAD_START_ROUTINE>( gameRoutine ), pinfo, NULL, nullptr );
			if ( !h_thread )
			{
				console::log( TEXT( "CreateThread failed: " ), GetLastError( ) );

				VirtualFree( pinfo, NULL, MEM_RELEASE );
				DisconnectNamedPipe( h_pipe );
				continue;
			}

			CLIENT client;
			client.pid = in.pid;
			client.h_pipe = h_pipe;
			client.h_thread = h_thread;
			_this->client_threads.push_back( client );
			
			if ( _this->callbacks_map.find( CLIENT_CALLBACK_TYPE::ON_PLAYER_JOIN ) != _this->callbacks_map.end( ) )
				_this->callbacks_map[ CLIENT_CALLBACK_TYPE::ON_PLAYER_JOIN ]( pinfo->pid, UP );
		}

		return ret_cause != WAIT_OBJECT_0;
	}

	static DWORD WINAPI gameRoutine( CLIENT_INFO* pinfo )
	{
		if ( !pinfo )
		{
			console::log( TEXT( "Invalid Game Thread" ) );

			auto& client_threads = pinfo->_this->client_threads;
			client_threads.erase( std::remove_if( client_threads.begin( ), client_threads.end( ), [ ] ( CLIENT client )
				{
					return GetCurrentThreadId( ) == GetThreadId( client.h_thread );
				} ), client_threads.end( ) );

			DisconnectNamedPipe( pinfo->h_pipe );
			return 1;
		}

		DWORD bytes = NULL;
		DWORD ret_cause = NULL;
		while ( ( ret_cause = WaitForSingleObjectEx( pinfo->_this->h_event, settings::tick_ms, false ) ) == WAIT_TIMEOUT )
		{
			GAME_PIPE_OUT out;
			out.type = UPDATE;
			memcpy( &out.data, &pinfo->_this->data, sizeof( out.data ) );
			if ( !WriteFile( pinfo->h_pipe, &out, sizeof( out ), nullptr, nullptr ) )
			{
				if ( GetLastError( ) == ERROR_NO_DATA )
					break;

				console::log( TEXT( "WriteFile failed: " ), GetLastError( ) );
				continue;
			}

			GAME_PIPE_IN in;
			DWORD available = NULL;
			if ( !PeekNamedPipe( pinfo->h_pipe, nullptr, NULL, nullptr, &available, nullptr ) )
			{
				if ( GetLastError( ) == ERROR_BROKEN_PIPE )
					break;

				console::log( TEXT( "PeekNamedPipe error: " ), GetLastError( ) );
				continue;
			}

			if ( available < sizeof( in ) )
				continue;

			if ( !ReadFile( pinfo->h_pipe, &in, sizeof( in ), nullptr, nullptr ) )
			{
				if ( GetLastError( ) == ERROR_BROKEN_PIPE )
					break;

				console::log( TEXT( "ReadFile error: " ), GetLastError( ) );
				continue;
			}

			if ( in.type == JOIN )
			{
				console::log( TEXT( "Invalid JOIN request from: " ), in.pid );
				continue;
			}

			if ( in.type == LEAVE )
			{
				ret_cause = WAIT_OBJECT_0;
				break;
			}

			if ( in.type == MOVE )
			{
				if ( pinfo->_this->callbacks_map.find( CLIENT_CALLBACK_TYPE::ON_PLAYER_MOVE ) != pinfo->_this->callbacks_map.end( ) )
					pinfo->_this->callbacks_map[ CLIENT_CALLBACK_TYPE::ON_PLAYER_MOVE ]( pinfo->pid, in.move.direction );
			}
			else
				console::log( TEXT( "Invalid request from: " ), in.pid );
		}

		if ( pinfo->_this->callbacks_map.find( CLIENT_CALLBACK_TYPE::ON_PLAYER_LEAVE ) != pinfo->_this->callbacks_map.end( ) )
			pinfo->_this->callbacks_map[ CLIENT_CALLBACK_TYPE::ON_PLAYER_LEAVE ]( pinfo->pid, UP );

		auto& client_threads = pinfo->_this->client_threads;
		client_threads.erase( std::remove_if( client_threads.begin( ), client_threads.end( ), [ ] ( CLIENT client )
			{
				return GetCurrentThreadId( ) == GetThreadId( client.h_thread );
			} ), client_threads.end( ) );

		DisconnectNamedPipe( pinfo->h_pipe );
		VirtualFree( pinfo, NULL, MEM_RELEASE );

		return ret_cause != WAIT_OBJECT_0;
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