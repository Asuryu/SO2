// main.cpp : Defines the entry point for the application.
//
#include <Windows.h>

import client;
import console;

Client* client = nullptr;

void exitHandler( )
{
    if ( !client )
        return;

    delete client;
    client = nullptr;
}

int main( )
{
    client = new Client( );

    atexit( exitHandler );

    if ( !client->isConnected( ) )
    {
        MessageBox( nullptr, TEXT( "Server is not running!" ), TEXT( "Error" ), MB_OK | MB_ICONERROR );
        exit( 1 );
    }

    MSG msg;
    while ( GetMessage( &msg, nullptr, 0, 0 ) )
    {
        TranslateMessage( &msg );
        DispatchMessage( &msg );
    }

    return (int) msg.wParam;
}