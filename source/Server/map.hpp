#pragma once 

#include <map>
#include <vector>
#include <stdexcept>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <iterator>

#include "road.hpp"
#include "entity/entity.hpp"
#include "entity/car.hpp"
#include "entity/frog.hpp"
#include "entity/obstacle.hpp"

import console;
import settings;
import utils;

class Map
{
private:
	int level = 1;
	int lines = 10, columns = 20;
	std::vector<Road*> roads;
	std::vector<Frog*> frogs;

	void initialize()
	{
		std::pair<int, int> coords;
		for (int i = 0; i < settings::num_roads; i++) {
			Road* road = new Road(columns);
			for (int j = 0; j < settings::init_car_number; j++){
				do
				{
					coords = { utils::generateRandomInt(0, columns - 1), i + 1 };
				} while ( getEntitiesAtCoords( coords ).size( ) );
				road->addEntity(new Car(coords.first, coords.second, i % 2 == 0 ? RIGHT : LEFT));
			}
			roads.push_back(road);
		}
	}

	Frog* checkWin()
	{
		for (int i = 0; i < frogs.size(); i++) 
			if (frogs.at(i)->getPosition().second >= lines - 1)
				return frogs.at(i);

		return nullptr;
	}

	void repositionEntities()
	{
		for (int i = 0; i < roads.size(); i++)
			for ( int j = 0; j < roads.at( i )->getEntities( ).size( ); j++ )
			{
				MovingEntity* mov_entity = dynamic_cast<MovingEntity*>( roads.at( i )->getEntities( ).at( j ) );
				if ( mov_entity == nullptr )
					continue;

				const auto entities_future = getEntitiesAtCoords( mov_entity->getNextPosition( ) );
				if ( entities_future.size( ) )
				{
					bool can_move = true;
					for ( const auto entity : entities_future )
					{
						if ( dynamic_cast<Obstacle*>( entity ) == nullptr && dynamic_cast<Car*>( entity ) == nullptr )
							continue;

						can_move = false;
						break;
					}

					if ( !can_move )
						mov_entity->invertFacingDirection( );
				}

				std::pair<int, int> entity_pos = mov_entity->getPosition( );
				if ( entity_pos.first < 0 )
					mov_entity->setPosition( columns - 1, entity_pos.second );
				if ( entity_pos.first > columns - 1 )
					mov_entity->setPosition( 0, entity_pos.second );
				if ( entity_pos.second > lines - 1 )
					mov_entity->setPosition( entity_pos.first, lines - 1 );
				if ( entity_pos.second < 0 )
					mov_entity->setPosition( entity_pos.first, 0 );
			}
	}

	void checkColision()
	{
		for (int i = 0; i < roads.size(); i++)
			for (int j = 0; j < roads.at(i)->getEntities().size(); j++) {
				Entity* entity = roads.at(i)->getEntities().at(j);
				if( entity == nullptr )
					continue;

				for (int k = 0; k < frogs.size(); k++) {
					Frog* frog = frogs.at(k);
					if (frog == nullptr)
						continue;

					if (frog->getPosition() == entity->getPosition())
						frog->setPosition(0, 0);

				}
			}
	}

public:
	Map( int num_roads )
	{
		console::log( "Map Constructor" );

		if ( num_roads < 1 )
			throw std::runtime_error( "The number of roads must be greater than 0" );

		if ( num_roads > 8 )
			throw std::runtime_error( "The number of roads must be lesser than 9" );

		this->lines = num_roads + 2;

		initialize();

		setLevel(1);
	}

	~Map( )
	{
		console::log( "Map Destructor" );

		for (const auto& frog : frogs)
			delete frog;
		
		for (const auto& road : roads)
			delete road;

		frogs.clear();
		roads.clear();
	}

	bool processTick()
	{
		bool ret = false;
		for (int i = 0; i < frogs.size(); i++)
			ret |= frogs.at(i)->processTick();
		for (int i = 0; i < roads.size(); i++)
			ret |= roads.at(i)->processTick();

		Frog* winner = checkWin();
		if (winner != nullptr) {
			roads.clear();
			initialize();
			setLevel(getLevel() + 1);

			for (int i = 0; i < frogs.size(); i++)
				frogs.at(i)->setPosition(utils::generateRandomInt( 0, columns - 1 ), 0);

		}

		repositionEntities();

		checkColision();

		return ret;
	}

	std::vector<Entity*> getEntitiesAtCoords( std::pair<int, int> cords )
	{
		std::vector <Entity*> entities_at_pos;

		for ( int i = 0; i < frogs.size( ); i++ )
			if ( frogs.at( i )->getPosition( ) == cords )
				entities_at_pos.push_back( frogs.at( i ) );

		for ( int i = 0; i < roads.size( ); i++ )
			for ( int j = 0; j < roads.at( i )->getEntities( ).size( ); j++ )
				if ( roads.at( i )->getEntities( ).at( j )->getPosition( ) == cords )
					entities_at_pos.push_back( roads.at( i )->getEntities( ).at( j ) );

		return entities_at_pos;
	}

	const std::vector<Frog*>& getFrogs( )
	{
		return frogs;
	}

	bool placeRock(int x, int y) {
		if (x < 0 || x > columns - 1 || y < 0 || y > lines - 1)
			return false;
		std::vector<Entity*> entities_at_pos = getEntitiesAtCoords({ x, y });
		if (entities_at_pos.size())
			return false;
		roads.at(y - 1)->addEntity(new Obstacle(x, y));
		return true;
	}

	const std::vector<Road*>& getRoads()
	{
		return roads;
	}

	const std::vector<Entity*> getEntities()
	{
		std::vector<Entity*> entities;

		for (int i = 0; i < frogs.size(); i++)
			entities.push_back(frogs.at(i));

		for (int i = 0; i < roads.size(); i++)
			for (int j = 0; j < roads.at(i)->getEntities().size(); j++)
				entities.push_back(roads.at(i)->getEntities().at(j));

		return entities;
	}

	std::pair<int, int> getSize( )
	{
		return std::make_pair( columns, lines );
	}

	int getLevel()
	{
		return level;
	}

	void setLevel(int level)
	{
		this->level = level;
		for (int i = 0; i < roads.size(); i++) {
			for (int j = 0; j < roads.at(i)->getEntities().size(); j++) {
				Car* car = dynamic_cast<Car*>(roads.at(i)->getEntities().at(j));
				if (car == nullptr)
					continue;	
				car->setSpeed(settings::init_car_speed + (level * 0.25));
			}
		}
	}

	void addFrog( Frog* ptr_frog )
	{
		frogs.push_back( ptr_frog );
	}

	void removeFrog( Frog* ptr_frog )
	{
		frogs.erase( std::remove( frogs.begin( ), frogs.end( ), ptr_frog ), frogs.end( ) );
	}
};