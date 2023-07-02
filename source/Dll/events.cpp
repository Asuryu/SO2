#include "events.h"

EventCallback callbacks[ EVENT_TYPE::MAX ] = { };

bool registerEvent( EVENT_TYPE type, EventCallback callback )
{
    if ( type < EVENT_TYPE::EVENT_GAME_UPDATE || type >= EVENT_TYPE::MAX )
        return false;

    callbacks[ type ] = callback;
    return true;
}

void unregisterEvent( EVENT_TYPE type )
{
    if ( type >= EVENT_TYPE::EVENT_GAME_UPDATE && type < EVENT_TYPE::MAX )
        callbacks[ type ] = nullptr;
}

bool triggerEvent( EVENT_TYPE type, void* ptr_data )
{
    if ( !ptr_data || type < EVENT_TYPE::EVENT_GAME_UPDATE || type >= EVENT_TYPE::MAX || !callbacks[ type ] )
        return false;

    callbacks[ type ]( ptr_data );
    return true;
}