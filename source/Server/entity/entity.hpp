#pragma once

#include <utility>

enum ENTITY_TYPE
{
	ENTITY_TYPE_OBSTACLE,
	ENTITY_TYPE_CAR,
	ENTITY_TYPE_FROG,
	ENTITY_TYPE_MAX
};

class Entity
{
protected:
	std::pair<int, int> position;

public:
	Entity(int x, int y)
	{
		this->position.first = x;
		this->position.second = y;
	}

	std::pair<int, int> getPosition()
	{
		return position;
	}

	void setPosition(int x, int y)
	{
		position.first = x;
		position.second = y;
	}
	virtual bool processTick() = 0;
	virtual void invertFacingDirection() = 0;

	virtual ENTITY_TYPE getType() = 0;
};