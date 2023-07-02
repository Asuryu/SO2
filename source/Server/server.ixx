module;

// workaround to intellisense that might be not as smart as we thought
//
#if __INTELLISENSE__
#include <Windows.h>
#include <functional>
#endif

export module server;

#ifndef __INTELLISENSE__
import <Windows.h>;
import <functional>;
#endif

import ui;
import op;
import client;
import engine;
import console;
import settings;

export class Server
{
private:
	bool running = true;
	bool is_frozen = false;
	bool suspended = false;

	HANDLE h_thread = nullptr;
	HANDLE h_thread_console = nullptr;
	HANDLE h_event = nullptr;
	HANDLE instance_semaphore = nullptr;

	UI* pui = nullptr;
	Client* pclient = nullptr;
	Operator* poperator = nullptr;
	GameEngine* pengine = nullptr;

public:
	Server( )
	{
		console::log( TEXT( "Server Constructor" ) );

		if ( !checkUniqueInstance( ) )
			std::exit( 1 );

		settings::load( );

		pclient = new Client( );
		
		// register client callbacks
		//
		pclient->registerCallback( CLIENT_CALLBACK_TYPE::ON_PLAYER_JOIN, [ & ] ( DWORD pid, FACING direction )
			{
				pengine->addPlayer( pid );
			} );
		pclient->registerCallback( CLIENT_CALLBACK_TYPE::ON_PLAYER_MOVE, [ & ] ( DWORD pid, FACING direction )
			{
				pengine->movePlayer( pid, direction );
			} );
		pclient->registerCallback( CLIENT_CALLBACK_TYPE::ON_PLAYER_LEAVE, [ & ] ( DWORD pid, FACING direction )
			{
				pengine->removePlayer( pid );
			} );

		pui = new UI( );

		poperator = new Operator( );
		poperator->setOnCommandResolve( [ this ] ( void* ptr_data )
			{
				this->onCommandResolve( static_cast<COMMAND*>( ptr_data ) );
			} );

		pengine = new GameEngine( );

		h_thread = CreateThread( nullptr, NULL, reinterpret_cast<LPTHREAD_START_ROUTINE>( mainRoutine ), this, NULL, nullptr );
		if ( !h_thread )
			std::exit( 1 );
		
		h_thread_console = CreateThread( nullptr, NULL, reinterpret_cast<LPTHREAD_START_ROUTINE>( adminConsole ), this, NULL, nullptr );
		if ( !h_thread_console )
			std::exit( 1 );

		h_event = CreateEvent( nullptr, true, false, nullptr );
		if ( !h_event )
			std::exit( 1 );
	}

	Server( int num_roads, double init_car_speed ) : Server( )
	{
		console::log( TEXT( "Server Constructor with Params" ) );

		settings::num_roads = num_roads;
		settings::init_car_speed = init_car_speed;
	}

	~Server( )
	{
		running = false;

		if ( h_thread )
		{
			if ( getStatus( h_thread ) == -1 )
				WaitForSingleObjectEx( h_thread, INFINITE, false );

			CloseHandle( h_thread );
		}

		if ( h_thread_console )
		{
			if ( getStatus( h_thread_console ) == -1 )
				WaitForSingleObjectEx( h_thread_console, INFINITE, false );

			CloseHandle( h_thread_console );
		}

		if ( h_event )
			CloseHandle( h_event );

		if ( pclient )
			delete pclient;

		if ( pui )
			delete pui;

		if ( pengine )
			delete pengine;

		if ( poperator )
			delete poperator;

		settings::save( );

		closeInstance( );

		console::log( TEXT( "Server Destructor" ) );
	}

	bool isRunning( )
	{
		return running && pclient->isConnected( );
	}

private:
	bool checkUniqueInstance( )
	{
		if ( instance_semaphore )
			return true;

		instance_semaphore = CreateSemaphore( nullptr, 1, 1, TEXT( "CRR_SERVER_INSTANCE_SPH" ) );
		if ( GetLastError( ) != ERROR_ALREADY_EXISTS )
			return true;

		console::error( TEXT( "You can't run more than one instance of the CRR server." ) );

		return false;
	}

	void closeInstance( )
	{
		if ( !instance_semaphore )
			return;

		CloseHandle( instance_semaphore );
		instance_semaphore = nullptr;
	}

	bool processTick( )
	{
		return pengine->processTick( );
	}

	static DWORD WINAPI mainRoutine( Server* _this )
	{
		while ( _this->running )
		{
			if ( _this->suspended )
				continue;

			const auto size = _this->pengine->getMapSize( );
			if ( _this->processTick( ) && !_this->poperator->updateData( _this->pengine->getEntityList( ), GAME_STATE_READY, 0, 0, size.first, size.second ) )
				_this->pui->printToPrompt( TEXT( "updateData failed" ) );

			_this->pclient->update( _this->pengine->getEntityList( ), GAME_STATE_READY, 0, 0, size.first, size.second );

			Sleep( settings::tick_ms );
		}

		COMMAND_INFO info;
		info.action = EXIT;
		_this->poperator->sendCommand( info, -1 );

		return 0;
	}

	void exit( )
	{
		running = false;
	}

	void suspend( )
	{
		if ( suspended )
		{
			pui->printToPrompt( TEXT( "Server is not running." ) );
			return;
		}

		pui->printToPrompt( TEXT( "Suspending..." ) );
		suspended = true;
	}

	void resume( )
	{
		if ( !suspended )
		{
			pui->printToPrompt( TEXT( "Server is not suspended." ) );
			return;
		}

		pui->printToPrompt( TEXT( "Resuming..." ) );
		suspended = false;
	}

	void restart( )
	{
		pui->printToPrompt( TEXT( "Restarting game..." ) );
		delete pengine;
		pengine = new GameEngine( );
	}

	static DWORD WINAPI adminConsole( Server* _this )
	{
		// lookup table (return void and no params)
		std::unordered_map<console::tstring, std::function<void( )>> commands = {
			{ TEXT( "exit" ), [ &_this ] ( ) { _this->exit( ); } },
			{ TEXT( "suspend" ), [ &_this ] ( ) { _this->suspend( ); } },
			{ TEXT( "resume" ), [ &_this ] ( ) { _this->resume( ); } },
			{ TEXT( "restart" ), [ &_this ] ( ) { _this->restart( ); } },
		};

		while ( _this->running )
		{
			const auto command = _this->pui->get( );
			if ( command.empty( ) )
				continue;

			// search lookup table
			for ( auto& [key, value] : commands )
				if ( !command.compare( key ) )
				{
					value( );
					break;
				}
		}

		return 0;
	}

	void onCommandResolve( COMMAND* ptr_command )
	{
		COMMAND_RESULT result;

		memset( &result, 0, sizeof( COMMAND_RESULT ) );

		switch ( ptr_command->info.action )
		{
		case COMMAND_ACTION::FREEZE:
			is_frozen = !is_frozen;
			pengine->setFrozen( is_frozen );
			if ( is_frozen )
			{
				ResetEvent( h_event );
				console::log( TEXT( "Freeze " ), ptr_command->info.freeze_time );
				DWORD result = WaitForSingleObject( h_event, ptr_command->info.freeze_time * 1000 );
				if ( result == WAIT_TIMEOUT )
				{
					is_frozen = false;
					pengine->setFrozen( is_frozen );
					SetEvent( h_event );
					console::log( TEXT( "Unfreeze" ) );
				}
			}

			break;
		case COMMAND_ACTION::ROCK:
			result.status = pengine->placeRock( ptr_command->info.pos.x, ptr_command->info.pos.y );
			break;
		case COMMAND_ACTION::INVERSE:
			pengine->invert( ptr_command->info.road_index );
			break;
		default:
			result.status = false;
			poperator->sendFeedback( result, ptr_command->sender_pid );
			return;
		}

		result.status = true;
		poperator->sendFeedback( result, ptr_command->sender_pid );
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