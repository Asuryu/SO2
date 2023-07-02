#pragma once

#include "mentity.hpp"

import settings;

class Car : public MovingEntity
{
public:
	Car( int x, int y, FACING facing_direction ) : 
		MovingEntity( x, y, facing_direction, settings::init_car_speed )
	{

	}

	ENTITY_TYPE getType( ) override
	{ 
		return ENTITY_TYPE_CAR;
	}
};