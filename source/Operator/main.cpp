#include <iostream>
#include <Windows.h>

import op;
import console;

Operator* op = nullptr;

void exitHandler( )
{
	if ( !op )
		return;

	delete op;
	op = nullptr;
}

int main( )
{
	op = new Operator( );

	atexit( exitHandler );

	while ( true )
		op->processCommands( );

	return 0;
}