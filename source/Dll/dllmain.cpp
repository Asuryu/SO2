// dllmain.cpp : Defines the entry point for the DLL application.
#include <Windows.h>

#include "smem.hpp"
#include "events.h"

SMem* mem = nullptr;

BOOL APIENTRY DllMain( HMODULE h_module,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        if ( mem )
            break;

        mem = new SMem( );

        mem->registerCmdHandler( [ ] ( COMMAND command )
            {
                if ( command.type == INFO )
                    triggerEvent( EVENT_TYPE::EVENT_COMMAND_RESOLVE, &command );
                else if ( command.type == RESULT )
                    triggerEvent( EVENT_TYPE::EVENT_COMMAND_RESOLVED, &command );
            } );
        mem->registerHandler( [ ] ( DATA data )
            {
                triggerEvent( EVENT_TYPE::EVENT_GAME_UPDATE, &data );
            } );

        break;
    case DLL_PROCESS_DETACH:
        if ( !mem )
            break;

        delete mem;
        mem = nullptr;

        break;
    default:
        break;
    }
    return TRUE;
}

extern "C"
{
    __declspec( dllexport ) bool getData( DATA* ptr_data )
    {
        return mem ? mem->getData( ptr_data ) : false;
    }

    __declspec( dllexport ) bool writeData( DATA data )
    {
        return mem ? mem->writeData( &data ) : false;
    }

    __declspec( dllexport ) bool sendCommand( COMMAND_INFO command_info, int target_pid )
    {
        COMMAND command;
        command.target_pid = target_pid;
        command.type = COMMAND_TYPE::INFO;
        command.sender_pid = GetCurrentProcessId( );
        memcpy_s( &command.info, sizeof( COMMAND_INFO ), &command_info, sizeof( COMMAND_INFO ) );

        return mem ? mem->writeCommand( &command ) : false;
    }

    __declspec( dllexport ) bool sendCommandFeedback( COMMAND_RESULT command_result, int target_pid )
    {
        COMMAND command;
        command.target_pid = target_pid;
        command.type = COMMAND_TYPE::RESULT;
        command.sender_pid = GetCurrentProcessId( );
        memcpy_s( &command.info, sizeof( COMMAND_RESULT ), &command_result, sizeof( COMMAND_RESULT ) );

        return mem ? mem->writeCommand( &command ) : false;
    }
}