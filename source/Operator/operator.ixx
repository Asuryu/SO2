module;

// workaround to intellisense that might be not as smart as we thought
//
#if __INTELLISENSE__
#include <string>
#include <Windows.h>
#endif

export module op;

#ifndef __INTELLISENSE__
import <string>;
import <Windows.h>;
#endif

import ui;
import server;
import console;

export class Operator
{
public:
	Operator( )
	{
		console::log( TEXT( "Operator Constructor" ) );

		if ( !isServerOpen( ) )
			std::exit( 1 );

		pserver = new Server( );
		pserver->setOnGameUpdate( [ this ] ( void* ptr_data )
			{
				this->onGameUpdate( static_cast<DATA*>( ptr_data ) );
			} );
		pserver->setOnCommandResolved( [ this ] ( void* ptr_data )
			{
				this->onCommandResolved( static_cast<COMMAND_RESULT*>( ptr_data ) );
			} );
		pserver->setOnCommandResolve( [ this ] ( void* ptr_command )
			{
				this->onCommandResolve( static_cast<COMMAND*>( ptr_command ) );
			} );

		DATA data;
		if ( !pserver->getData( &data ) )
		{
			console::error( TEXT( "getData failed" ) );

			std::exit( 1 );
		}

		onGameUpdate( &data );
	}

	~Operator( )
	{
		console::log( TEXT( "Operator Destructor" ) );

		if ( pserver )
			delete pserver;

		if ( pui )
			delete pui;
	}

	void processCommands( )
	{
		const auto command = pui->get( );

		// enviar cada comando e receber resposta
		COMMAND_INFO info;

		if ( !command.compare( TEXT( "freeze" ) ) )
		{
			info.action = COMMAND_ACTION::FREEZE;

			const auto str_freeze_time = pui->get( TEXT( "Freeze Time: " ) );
			info.freeze_time = std::stoi( str_freeze_time );
		}
		else if ( !command.compare( TEXT( "rock" ) ) )
		{
			info.action = COMMAND_ACTION::ROCK;

			const auto str_pos_x = pui->get( TEXT( "Pos X: " ) );
			info.pos.x = std::stoi( str_pos_x );

			const auto str_pos_y = pui->get( TEXT( "Pos Y: " ) );
			info.pos.y = std::stoi( str_pos_y );
		}
		else if ( !command.compare( TEXT( "inverse" ) ) )
		{
			info.action = COMMAND_ACTION::INVERSE;

			const auto str_road_index = pui->get( TEXT( "Lane Index: " ) );
			info.road_index = std::stoi( str_road_index );
		}
		else
			return pui->printToPrompt( TEXT( "Invalid command" ) );

		if ( !pserver->sendCommand( info ) )
			return pui->printToPrompt( TEXT( "Error sending command" ) );
	}

private:
	HANDLE instance_semaphore = nullptr;

	Server* pserver = nullptr;

	UI* pui = nullptr;

	void onCommandResolved( COMMAND_RESULT* ptr_result )
	{
		if ( !ptr_result )
			return;

		pui->printToPrompt( ptr_result->status ? TEXT( "Command success!" ) : TEXT( "Command failed!" ) );
	}

	void onCommandResolve( COMMAND* ptr_command )
	{
		if ( !ptr_command )
			return;

		pui->printToPrompt( TEXT( "Command received!" ) );

		switch ( ptr_command->info.action )
		{
		case COMMAND_ACTION::EXIT:
			exit( 0 );
		default:
			return;
		}
	}

	void onGameUpdate( DATA* ptr_data )
	{
		if ( !ptr_data )
			return;

		if ( pui == nullptr )
			pui = new UI( ptr_data->width, 20, ptr_data->width, ptr_data->height );

		TCHAR buffer[ 10 ][ 20 ];

		for ( int i = 0; i < ptr_data->height; i++ )
			for ( int j = 0; j < ptr_data->width; j++ )
				buffer[ i ][ j ] = TEXT( '_' );

		for ( int i = 0; i < ptr_data->num_entities; i++ )
		{
			const auto& entity = ptr_data->entities[ i ];

			if ( entity.pos_x >= ptr_data->width || entity.pos_x < 0 ||
				entity.pos_y >= ptr_data->height || entity.pos_y < 0 )
			{
				console::log( "Invalid Entity found at (", entity.pos_x, ", ", entity.pos_y, "): 0x", &entity );
				continue;
			}

			switch ( entity.type )
			{
			case ENTITY_TYPE::ENTITY_TYPE_CAR:
				buffer[ entity.pos_y ][ entity.pos_x ] = TEXT( 'C' );
				break;
			case ENTITY_TYPE::ENTITY_TYPE_OBSTACLE:
				buffer[ entity.pos_y ][ entity.pos_x ] = TEXT( 'O' );
				break;
			case ENTITY_TYPE::ENTITY_TYPE_FROG:
				buffer[ entity.pos_y ][ entity.pos_x ] = TEXT( 'F' );
				break;
			default:
				break;
			}
		}

		std::vector<console::tstring> lines;
		for ( int i = 0; i < ptr_data->height; i++ )
		{
			console::tstring line = TEXT( "" );
			for ( int j = 0; j < ptr_data->width; j++ )
				line.push_back( buffer[ i ][ j ] );

			lines.push_back( line );
		}

		pui->printGame( lines );
	}

	bool isServerOpen( )
	{
		instance_semaphore = CreateSemaphore( nullptr, 1, 1, TEXT( "CRR_SERVER_INSTANCE_SPH" ) );

		if ( GetLastError( ) == ERROR_ALREADY_EXISTS )
			return true;

		CloseHandle( instance_semaphore );

		instance_semaphore = nullptr;

		console::error( TEXT( "You can only run the operator when a game server is open." ) );

		return false;
	}
};