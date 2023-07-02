#pragma once

#include "entity/frog.hpp"

import utils;

class Player : public Frog
{
private:
	int pid = 0;
	int points = 0;

public:
	Player( int pid ) : Frog( 0, 0 )
	{
		this->pid = pid;
	}

	~Player( )
	{

	}

	int getPid( )
	{
		return pid;
	}

	int getPoints( )
	{
		return points;
	}

	void setPoints( int points )
	{
		this->points = points;
	}

	void addPoints( int points )
	{
		this->points += points;
	}

	void resetPoints( )
	{
		points = 0;
	}
};