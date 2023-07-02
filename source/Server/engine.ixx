module;

// workaround to intellisense that might be not as smart as we thought
//
#if __INTELLISENSE__
#include <Windows.h>
#endif

#include "map.hpp"
#include "player.hpp"

export module engine;

#ifndef __INTELLISENSE__
import <Windows.h>;
#endif 

import console;
import settings;

export class GameEngine
{
private:
	Map* pmap = nullptr;
	std::vector<Player*> players;

public:
	GameEngine( )
	{
		console::log( "GameEngine Constructor" );

		try
		{
			pmap = new Map( settings::num_roads );
		}
		catch ( std::runtime_error const& e )
		{
			console::error( e.what( ) );
			std::exit( 1 );
		}
	}

	~GameEngine( )
	{
		console::log( "GameEngine Destructor" );

		if ( pmap )
			delete pmap;
	}

	void restart( )
	{
		if ( pmap )
			delete pmap;
		try
		{
			pmap = new Map( settings::num_roads );
		}
		catch ( std::runtime_error const& e )
		{
			console::error( e.what( ) );
			std::exit( 1 );
		}
	}

	bool processTick( )
	{

		bool processed = pmap->processTick( );

		for ( int i = 0; i < pmap->getFrogs( ).size( ); i++ )
		{
			const auto frog = pmap->getFrogs( ).at( i );
			if ( frog->getAfkTimer( ) < settings::max_afk_timer )
				continue;

			frog->setAfkTimer( 0 );
			frog->setPosition( utils::generateRandomInt( 0, pmap->getSize( ).first - 1 ), 0 );

			processed = true;
		}

		return processed;
	}

	std::pair<int, int> getMapSize( )
	{
		return pmap->getSize( );
	}

	const std::vector<Entity*> getEntityList( )
	{
		return pmap->getEntities( );
	}

	bool placeRock( int x, int y )
	{
		return pmap->placeRock( x, y );
	}

	void setFrozen( bool frozen )
	{
		for ( auto& road : pmap->getRoads( ) )
			road->setFrozen( frozen );
	}

	void setFrozen( bool frozen, int index )
	{
		pmap->getRoads( ).at( index )->setFrozen( frozen );
	}

	void invert( )
	{
		for ( auto& road : pmap->getRoads( ) )
			road->invert( );
	}

	void invert( int index )
	{
		pmap->getRoads( ).at( index )->invert( );
	}

	void addPlayer( DWORD pid )
	{
		Player* ptr_player = new Player( pid );

		std::pair<int, int> coords;
		do
		{
			coords = { utils::generateRandomInt( 0, pmap->getSize( ).first - 1 ), 0 };
		} while ( pmap->getEntitiesAtCoords( coords ).size( ) );

		ptr_player->setPosition( coords.first, coords.second );

		players.push_back( ptr_player );
		pmap->addFrog( dynamic_cast<Frog*>( ptr_player ) );
	}

	void removePlayer( DWORD pid )
	{
		players.erase( std::remove_if( players.begin( ), players.end( ), [ &, pid ] ( Player* player )
			{
				if ( player->getPid( ) != pid )
					return false;

				pmap->removeFrog( dynamic_cast<Frog*>( player ) );
				delete player;
				return true;
			} ), players.end( ) );
	}

	void movePlayer( DWORD pid, FACING direction )
	{
		for ( const auto player : players )
		{
			if ( player->getPid( ) != pid || !player->canMove( ) )
				continue;

			player->move( direction );

			const auto [columns, lines] = pmap->getSize( );
			const auto entity_pos = player->getPosition( );

			if ( entity_pos.first < 0 )
				player->setPosition( 0, entity_pos.second );

			if ( entity_pos.first > columns - 1 )
				player->setPosition( columns - 1, entity_pos.second );

			if ( entity_pos.second > lines - 1 )
				player->setPosition( entity_pos.first, lines - 1 );

			if ( entity_pos.second < 0 )
				player->setPosition( entity_pos.first, 0 );
		}
	}
};