#pragma once

#include <Windows.h>

import console;

enum FACING
{
	UP,
	DOWN,
	LEFT,
	RIGHT
};

class Player
{
private:
	int points = 0;

public:
	Player() {

	}

	~Player()
	{

	}

	bool move(FACING direction ) 
	{
		
		return false;
	}

	bool resetPos() {
		console::log( "Player resetPos" );
		// Fazer request para o servidor
		return false;
	}
};