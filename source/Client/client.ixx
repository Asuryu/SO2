module;

// workaround to intellisense that might be not as smart as we thought
//
#if __INTELLISENSE__
#include <Windows.h>
#endif

export module client;

#ifndef __INTELLISENSE__
import <Windows.h>;
#endif

import ui;
import image;
import server;
import console;

export class Client
{
private:
    UI* ptr_ui = nullptr;
    Server* ptr_server = nullptr;
    HANDLE instance_semaphore = nullptr;

    inline static constexpr auto client_instance_sp = TEXT( "Local\\CRR_CLIENT_INSTANCE_SPH" );

public:
    Client( )
    {
        console::log( TEXT( "Client Constructor" ) );

        // initialize UI
        //
        ptr_ui = new UI( );

        // register button callbacks
        //
        ptr_ui->registerCallback( UI_CALLBACK_TYPE::ON_START, [ & ] ( )
            {
                ptr_ui->drawPreGame( );
            } );
        ptr_ui->registerCallback( UI_CALLBACK_TYPE::ON_EXIT, [ ] ( )
            {
                exit( 0 );
            } );
        ptr_ui->registerCallback( UI_CALLBACK_TYPE::ON_ABOUT, [ ] ( )
            {
                MessageBox( nullptr, TEXT( "This practical work was carried out within the scope of the Operating Systems 2 discipline of the degree in Computer Engineering at ISEC." ), TEXT( "About" ), MB_OK);
            } );
        ptr_ui->registerCallback( UI_CALLBACK_TYPE::ON_SINGLEPLAYER, [ & ] ( )
            {
                if ( !ptr_server->joinMatch( SINGLEPLAYER ) )
                {
                    MessageBox( nullptr, TEXT( "Server declined connection" ), TEXT( "Singleplayer Match" ), MB_OK );
                    return;
                }

                ptr_ui->drawGame( );
            } );
        ptr_ui->registerCallback( UI_CALLBACK_TYPE::ON_MULTIPLAYER, [ & ] ( )
            {
                if ( !ptr_server->joinMatch( MULTIPLAYER ) )
                {
                    MessageBox( nullptr, TEXT( "Server declined connection" ), TEXT( "Multiplayer Match" ), MB_OK );
                    return;
                }

                ptr_ui->drawGame( );
            } );
        ptr_ui->registerCallback(UI_CALLBACK_TYPE::ON_CHANGE_BITMAP, [&]()
            {
                ptr_ui->changeCurrentBitmap();
            });
        ptr_ui->registerDirectionCallback( UI_DIRECTION_TYPE::KEY_UP, [ & ] ( )
            {
                ptr_server->sendMove( FACING::UP );
            } );
        ptr_ui->registerDirectionCallback( UI_DIRECTION_TYPE::KEY_DOWN, [ & ] ( )
            {
                ptr_server->sendMove( FACING::DOWN );
            } );
        ptr_ui->registerDirectionCallback( UI_DIRECTION_TYPE::KEY_LEFT, [ & ] ( )
            {
                ptr_server->sendMove( FACING::LEFT );
            } );
        ptr_ui->registerDirectionCallback( UI_DIRECTION_TYPE::KEY_RIGHT, [ & ] ( )
            {
                ptr_server->sendMove( FACING::RIGHT );
            } );

        // initialize Server
        //
        ptr_server = new Server( );
        ptr_server->setOnUpdateCallback( [ & ] ( const DATA& data )
            {
                onUpdate( data );
            } );

        if ( checkMaxInstances( ) )
        {
            MessageBox( nullptr, TEXT( "Max instances reached" ), TEXT( "Error" ), MB_OK | MB_ICONERROR);
            exit( 0 );
        }
    }

    ~Client( )
    {
        if ( ptr_server )
        {
            delete ptr_server;
            ptr_server = nullptr;
        }

        if ( ptr_ui )
        {
            delete ptr_ui;
            ptr_ui = nullptr;
        }

        if ( instance_semaphore )
        {
            WaitForSingleObjectEx( instance_semaphore, INFINITE, false );

            CloseHandle( instance_semaphore );
            instance_semaphore = nullptr;
        }

        console::log( TEXT( "Client Destructor" ) );
    }

    bool isConnected( )
    {
        return ptr_server->isConnected( );
    }

    bool checkMaxInstances( )
    {
        instance_semaphore = CreateSemaphoreEx( nullptr, 0, 2, client_instance_sp, 0, SEMAPHORE_ALL_ACCESS );
        if ( !instance_semaphore )
        {
            console::log( TEXT( "CreateSemaphoreEx failed: " ), GetLastError( ) );
            return true;
        }

        if ( ReleaseSemaphore( instance_semaphore, 1, nullptr ) )
            return false;

        CloseHandle( instance_semaphore );
        instance_semaphore = nullptr;
        return true;
    }

private:
    void onUpdate( const DATA& data )
    {
        ptr_ui->updateGameData( data );

        // todo: logica do jogo
    }
};