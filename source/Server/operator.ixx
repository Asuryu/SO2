module;

// workaround to intellisense that might be not as smart as we thought
//
#if __INTELLISENSE__
#include <Windows.h>
#include <functional>
#endif

#include "map.hpp"

export module op;

#ifndef __INTELLISENSE__
import <Windows.h>;
import <functional>;
#endif

import console;

#define MAX_ENTITIES    160  // 10 - 2 = 8, 8 * 20 = 160

export enum COMMAND_TYPE
{
	INFO,
	RESULT
};

export enum COMMAND_ACTION
{
	EXIT,
	FREEZE,
	INVERSE,
	ROCK,
	COMMAND_ACTION_MAX
};

export typedef struct
{
	COMMAND_ACTION action;

	union
	{
		int freeze_time;
		int road_index;
		struct
		{
			int x, y;
		} pos;
	};
} COMMAND_INFO;

export typedef struct
{
	bool status;
} COMMAND_RESULT;

export typedef struct
{
	int sender_pid, target_pid;     // -1 -> Target All Instances | 0 -> Target Server | <PID> -> Target Instance with the specified PID
	COMMAND_TYPE type;
	union
	{
		COMMAND_INFO info;
		COMMAND_RESULT result;
	};
} COMMAND;

export typedef struct
{
	ENTITY_TYPE type;
	FACING direction;
	int pos_x, pos_y;
} ENTITY;

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

typedef struct
{
	GAME_STATE state;
	int time, level;
	int width, height;
	int num_entities;
	ENTITY entities[ MAX_ENTITIES ];
} DATA;

enum EVENT_TYPE
{
	EVENT_GAME_UPDATE,
	EVENT_COMMAND_RESOLVE,
	EVENT_COMMAND_RESOLVED,
	EVENT_TYPE_MAX
};

using EventCallback = std::function<void( void* ptr_data )>;

using WriteDataFn = bool( * )( DATA data );
using GetDataFn = bool( * )( DATA* ptr_data );
using UnregisterFn = void( * )( EVENT_TYPE type );
using SendCommandFn = bool( * )( COMMAND_INFO command_info, int target_pid );
using RegisterFn = bool( * )( EVENT_TYPE type, EventCallback callback );
using SendCommandFeedbackFn = bool( * )( COMMAND_RESULT command_result, int target_pid );

export using OnCommandResolveFn = std::function<void( COMMAND* ptr_command )>;

export class Operator
{
private:
	inline static constexpr auto operator_dll = TEXT( "Dll.dll" );

	HMODULE h_dll = nullptr;

	GetDataFn getdata_fn = nullptr;
	RegisterFn register_fn = nullptr;
	UnregisterFn unregister_fn = nullptr;
	WriteDataFn writedata_fn = nullptr;
	SendCommandFn sendcommand_fn = nullptr;
	SendCommandFeedbackFn sendcommandfeedback_fn = nullptr;

public:
	Operator( )
	{
		console::log( TEXT( "Operator Constructor" ) );

		h_dll = LoadLibrary( operator_dll );
		if ( !h_dll )
		{
			console::error( TEXT("LoadLibrary failed: 0x"), GetLastError( ) );
			std::exit( 1 );
		}

		getdata_fn = reinterpret_cast<GetDataFn>( GetProcAddress( h_dll, "getData" ) );
		if ( !getdata_fn )
		{
			console::error( TEXT( "GetProcAddress \"getData\" failed: 0x" ), GetLastError( ) );

			CloseHandle( h_dll );
			std::exit( 1 );
		}

		register_fn = reinterpret_cast<RegisterFn>( GetProcAddress( h_dll, "registerEvent" ) );
		if ( !register_fn )
		{
			console::error( TEXT( "GetProcAddress \"registerEvent\" failed: 0x" ), GetLastError( ) );

			CloseHandle( h_dll );
			std::exit( 1 );
		}

		unregister_fn = reinterpret_cast<UnregisterFn>( GetProcAddress( h_dll, "unregisterEvent" ) );
		if ( !unregister_fn )
		{
			console::error( TEXT( "GetProcAddress \"unregisterEvent\" failed: 0x" ), GetLastError( ) );

			CloseHandle( h_dll );
			std::exit( 1 );
		}

		writedata_fn = reinterpret_cast<WriteDataFn>( GetProcAddress( h_dll, "writeData" ) );
		if ( !writedata_fn )
		{
			console::error( TEXT( "GetProcAddress \"writeData\" failed: 0x" ), GetLastError( ) );

			CloseHandle( h_dll );
			std::exit( 1 );
		}

		sendcommand_fn = reinterpret_cast<SendCommandFn>( GetProcAddress( h_dll, "sendCommand" ) );
		if ( !sendcommand_fn )
		{
			console::error( TEXT( "GetProcAddress \"sendCommand\" failed: 0x" ), GetLastError( ) );

			CloseHandle( h_dll );
			std::exit( 1 );
		}

		sendcommandfeedback_fn = reinterpret_cast<SendCommandFeedbackFn>( GetProcAddress( h_dll, "sendCommandFeedback" ) );
		if ( !sendcommandfeedback_fn )
		{
			console::error( TEXT( "GetProcAddress \"sendCommandFeedback\" failed: 0x" ), GetLastError( ));

			CloseHandle( h_dll );
			std::exit( 1 );
		}
	}

	~Operator( )
	{
		console::log( TEXT( "Operator Destructor" ) );

		if ( h_dll && !FreeLibrary( h_dll ) )
		{
			console::error( TEXT( "FreeLibrary failed: 0x" ), GetLastError( ) );

			std::exit( 1 );
		}
	}

	bool updateData( const std::vector<Entity*>& entities, GAME_STATE state, int time, int level, int width, int height )
	{
		DATA data;
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
				console::error( "updateData failed: Invalid entity" );
				return false;
			}
			
		}
		return writedata_fn && writedata_fn( data ) ? true : false;
	}

	bool setOnCommandResolve( OnCommandResolveFn callback )
	{
		return callback && register_fn ?
			register_fn( EVENT_TYPE::EVENT_COMMAND_RESOLVE, [ callback ] ( void* ptr_data )
				{
					callback( static_cast<COMMAND*>( ptr_data ) );
				} ) : false;
	}

	bool sendFeedback( COMMAND_RESULT result, int target_pid )
	{
		return sendcommandfeedback_fn && target_pid > 0 ? sendcommandfeedback_fn( result, target_pid ) : false;
	}

	bool sendCommand( COMMAND_INFO command_info, int target_pid )
	{
		return sendcommand_fn && target_pid > 0 ? sendcommand_fn( command_info, target_pid ) : false;
	}
};