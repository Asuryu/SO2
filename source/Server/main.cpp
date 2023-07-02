#include <windows.h>

import <string>;

import server;
import console;
import settings;

Server* server = nullptr;

void exitHandler( )
{
	if ( !server )
		return;

	delete server;
	server = nullptr;
}

int main( int argc, char* argv[ ] )
{	
	if (argc == 1)
		server = new Server( );
	else if (argc == 3)
		server = new Server( std::stoi( argv[ 1 ] ), std::atof( argv[ 2 ] ) );
	else
	{
		console::error( TEXT( "Invalid number of commands provided" ) );
		return 1;
	}

	atexit( exitHandler );

	console::print( TEXT( "Int Car Speed: " ), settings::init_car_speed, TEXT( "\nNum Roads: " ), settings::num_roads, "\n" );

	while (server != nullptr && server->isRunning())
		Sleep( settings::tick_ms );

	return 0;
}