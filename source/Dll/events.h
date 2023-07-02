#pragma once

#include <functional>

enum EVENT_TYPE
{
    EVENT_GAME_UPDATE,
    EVENT_COMMAND_RESOLVE,
    EVENT_COMMAND_RESOLVED,
    MAX
};

using EventCallback = std::function<void( void* ptr_data )>;

extern "C"
{
    __declspec( dllexport ) bool registerEvent( EVENT_TYPE type, EventCallback callback );

    __declspec( dllexport ) void unregisterEvent( EVENT_TYPE type );
}

bool triggerEvent( EVENT_TYPE type, void* ptr_data );