#pragma once

#include "entity.hpp"

import console;
import settings;

enum FACING
{
	UP,
	DOWN,
	LEFT,
	RIGHT
};

class MovingEntity : public Entity
{
protected:
	FACING facing_direction;
	bool moving = true;
	double speed;
	int time_accumulator = 0;

public:
	MovingEntity( int x, int y, FACING facing_direction, double speed ) : Entity( x, y )
	{
		this->facing_direction = facing_direction;
		this->speed = speed;
	}

	FACING getFacingDirection( )
	{
		return facing_direction;
	}

	void setFacingDirection( FACING facing_direction )
	{
		if ( facing_direction >= FACING::UP && facing_direction <= FACING::RIGHT )
			this->facing_direction = facing_direction;
	}

	void invertFacingDirection()
	{
		facing_direction = (facing_direction % 2) ? static_cast<FACING>(facing_direction - 1) : static_cast<FACING>(facing_direction + 1);
	}

	double getSpeed( )
	{
		return speed;
	}

	void setSpeed( double speed )
	{
		this->speed = speed;
	}

	bool move() {
		switch (facing_direction)
		{
		case LEFT:
			position.first--;
			break;
		case RIGHT:
			position.first++;
			break;
		case DOWN:
			position.second--;
			break;
		case UP:
			position.second++;
			break;
		default:
			return false;
		}
		time_accumulator = 0;
		return true;
	}

	std::pair<int, int> getNextPosition()
	{
		auto ret = position;
		switch (facing_direction)
		{
		case LEFT:
			ret.first--;
			break;
		case RIGHT:
			ret.first++;
			break;
		case DOWN:
			ret.second--;
			break;
		case UP:
			ret.second++;
			break;
		default:
			return position;
		}
		return ret;
	}

	void setMoving(bool moving)
	{
		this->moving = moving;
	}

	bool isMoving()
	{
		return moving;
	}

	bool canMove()
	{
		return time_accumulator >= 500 / speed;
	}

	bool processTick( ) override
	{
		time_accumulator += settings::tick_ms;

		if (!moving || !canMove())
			return false;
			
		move( );
		
		return true;
	}
};