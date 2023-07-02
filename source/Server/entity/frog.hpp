#pragma once

#include "mentity.hpp"

import utils;
import settings;

class Frog : public MovingEntity
{
private:
	int afk_timer = 0;
public:
	Frog( int x, int y ) : MovingEntity( x, y, FACING::UP, settings::init_car_speed )
	{
		moving = false;
		setSpeed( 2.0f );
	}

	bool move(FACING facing_direction)
	{
		setFacingDirection(facing_direction);
		if(__super::move())
		{
			afk_timer = 0;
			return true;
		}

		return false;
	}

	bool processTick( ) override
	{
		__super::processTick( );

		if (position.second == 0)
		{
			afk_timer = 0;
			return false;
		}

		afk_timer += settings::tick_ms;
		return false;
	}

	ENTITY_TYPE getType( ) override
	{
		return ENTITY_TYPE_FROG;
	}

	int getAfkTimer() {
		return afk_timer;
	}

	void setAfkTimer(int afk_timer) {
		this->afk_timer = afk_timer;
	}

	void resetAfkTimer() {
		afk_timer = 0;
	}


};