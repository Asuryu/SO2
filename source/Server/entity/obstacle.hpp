#pragma once

#include "entity.hpp"

import settings;

class Obstacle : public Entity
{
public:
	Obstacle( int x, int y ) : Entity( x, y )
	{

	}

	bool processTick() override 
	{
		return false;
	}

	void invertFacingDirection() override
	{

	}

	ENTITY_TYPE getType( ) override
	{
		return ENTITY_TYPE_OBSTACLE;
	}
};