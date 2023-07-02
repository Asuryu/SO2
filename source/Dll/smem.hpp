#pragma once

#include <Windows.h>
#include <functional>

#include "events.h"

import console;

#define MAX_COMMANDS    10
#define MAX_ENTITIES    160  // 10 - 2 = 8, 8 * 20 = 160

enum COMMAND_TYPE
{
    INFO,
    RESULT
};

enum COMMAND_ACTION
{
    FREEZE,
    INVERSE,
    ROCK,
    COMMAND_ACTION_MAX
};

typedef struct
{
    COMMAND_ACTION action;

    union
    {
        int road_index;
        struct
        {
            int x, y;
        } pos;
    };
} COMMAND_INFO;

typedef struct
{
    bool status;
} COMMAND_RESULT;

typedef struct
{
    int sender_pid, target_pid;     // -1 -> Target All Instances | 0 -> Target Server | <PID> -> Target Instance with the specified PID
    COMMAND_TYPE type;
    union
    {
        COMMAND_INFO info;
        COMMAND_RESULT result;
    };
} COMMAND;

using CommandHandler = std::function<void( COMMAND command )>;

enum ENTITY_TYPE
{
    ENTITY_TYPE_OBSTACLE,
    ENTITY_TYPE_CAR,
    ENTITY_TYPE_FROG,
    ENTITY_TYPE_MAX
};

enum FACING
{
    UP,
    DOWN,
    LEFT,
    RIGHT
};

typedef struct
{
    ENTITY_TYPE type;
    FACING direction;
    int pos_x, pos_y;
} ENTITY;

enum GAME_STATE
{
    GAME_STATE_READY,
    GAME_STATE_RUNNING,
    GAME_STATE_PLAYER1_WINS,
    GAME_STATE_PLAYER2_WINS,
    GAME_STATE_DRAW,
    GAME_STATE_LOSS,
    GAME_STATE_MAX
};

typedef struct
{
    GAME_STATE state;
    int time, level;
    int width, height;
    int num_entities;
    ENTITY entities[ MAX_ENTITIES ];
} DATA;

using DataHandler = std::function<void( DATA data )>;

typedef struct
{
    int num_instances, processed_instances;
    DATA data;
    struct
    {
        unsigned long read_index, write_index;
        COMMAND commands[ MAX_COMMANDS ];
    } commands;
} SMEM;

class SMem
{
private:
    inline static constexpr auto update_interval        = 100;

    inline static constexpr auto max_readers            = 10;
    inline static constexpr auto max_commands           = MAX_COMMANDS;

    inline static constexpr auto identifier             = TEXT( "Local\\CRR" );

    inline static constexpr auto updated_event          = TEXT( "Local\\CRR_SMEM_UPDATED_EVENT" );

    inline static constexpr auto mutex_data_usage       = TEXT( "Local\\CRR_SMEM_MUTEX_DATA_USAGE" );

    inline static constexpr auto sem_read_commands      = TEXT( "Local\\CRR_SMEM_SEM_READ_COMMANDS" );
    inline static constexpr auto sem_write_commands     = TEXT( "Local\\CRR_SMEM_SEM_WRITE_COMMANDS" );

    inline static constexpr auto mutex_read_commands    = TEXT( "Local\\CRR_SMEM_MUTEX_READ_COMMANDS" );
    inline static constexpr auto mutex_write_commands   = TEXT( "Local\\CRR_SMEM_MUTEX_WRITE_COMMANDS" );

    inline static constexpr auto mutex_instances_usage  = TEXT( "Local\\CRR_SMEM_MUTEX_INSTANCES_USAGE" );

    // local
    //
    bool is_main_instance              = false;

    HANDLE h_thread             = nullptr;

    CRITICAL_SECTION cs_usage { };

    DataHandler data_handler    = nullptr;
    CommandHandler cmd_handler  = nullptr;

    // shared
    //
    SMEM* ptr_smem                  = nullptr;

    HANDLE h_smem                   = nullptr;

    HANDLE h_event                  = nullptr, h_updated_event          = nullptr;

    HANDLE h_sem_read_commands      = nullptr, h_sem_write_commands     = nullptr;
    HANDLE h_mutex_read_commands    = nullptr, h_mutex_write_commands   = nullptr;

    HANDLE h_mutex_data_usage       = nullptr, h_mutex_instances_usage  = nullptr;

public:
    SMem( )
    {
        h_smem = CreateFileMapping( INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, sizeof( SMEM ), identifier );
        if ( !h_smem )
            exit( 1 );

        is_main_instance = GetLastError( ) != ERROR_ALREADY_EXISTS;

        ptr_smem = reinterpret_cast<SMEM*>( MapViewOfFile( h_smem, FILE_MAP_ALL_ACCESS, 0, 0, sizeof( SMEM ) ) );
        if ( !ptr_smem )
        {
            CloseHandle( h_smem );
            exit( 1 );
        }

        if ( !InitializeCriticalSectionEx( &cs_usage, 200, NULL ) )
        {
            UnmapViewOfFile( ptr_smem );
            CloseHandle( h_smem );
            exit( 1 );
        }

        h_mutex_data_usage = CreateMutexEx( nullptr, mutex_data_usage, 0, MUTEX_ALL_ACCESS );
        if ( !h_mutex_data_usage )
        {
            DeleteCriticalSection( &cs_usage );
            UnmapViewOfFile( ptr_smem );
            CloseHandle( h_smem );
            exit( 1 );
        }

        h_mutex_instances_usage = CreateMutexEx( nullptr, mutex_instances_usage, 0, MUTEX_ALL_ACCESS );
        if ( !h_mutex_instances_usage )
        {
            CloseHandle( h_mutex_data_usage );
            DeleteCriticalSection( &cs_usage );
            UnmapViewOfFile( ptr_smem );
            CloseHandle( h_smem );
            exit( 1 );
        }

        h_mutex_read_commands = CreateMutexEx( nullptr, mutex_read_commands, 0, MUTEX_ALL_ACCESS );
        if ( !h_mutex_read_commands )
        {
            CloseHandle( h_mutex_instances_usage );
            CloseHandle( h_mutex_data_usage );
            DeleteCriticalSection( &cs_usage );
            UnmapViewOfFile( ptr_smem );
            CloseHandle( h_smem );
            exit( 1 );
        }

        h_mutex_write_commands = CreateMutexEx( nullptr, mutex_write_commands, 0, MUTEX_ALL_ACCESS );
        if ( !h_mutex_write_commands )
        {
            CloseHandle( h_mutex_read_commands );
            CloseHandle( h_mutex_instances_usage );
            CloseHandle( h_mutex_data_usage );
            DeleteCriticalSection( &cs_usage );
            UnmapViewOfFile( ptr_smem );
            CloseHandle( h_smem );
            exit( 1 );
        }

        h_sem_read_commands = CreateSemaphoreEx( nullptr, 0, max_commands, sem_read_commands, 0, SEMAPHORE_ALL_ACCESS );
        if ( !h_sem_read_commands )
        {
            CloseHandle( h_mutex_write_commands );
            CloseHandle( h_mutex_read_commands );
            CloseHandle( h_mutex_instances_usage );
            CloseHandle( h_mutex_data_usage );
            DeleteCriticalSection( &cs_usage );
            UnmapViewOfFile( ptr_smem );
            CloseHandle( h_smem );
            exit( 1 );
        }

        h_sem_write_commands = CreateSemaphoreEx( nullptr, max_commands, max_commands, sem_write_commands, 0, SEMAPHORE_ALL_ACCESS );
        if ( !h_sem_write_commands )
        {
            CloseHandle( h_sem_read_commands );
            CloseHandle( h_mutex_write_commands );
            CloseHandle( h_mutex_read_commands );
            CloseHandle( h_mutex_instances_usage );
            CloseHandle( h_mutex_data_usage );
            DeleteCriticalSection( &cs_usage );
            UnmapViewOfFile( ptr_smem );
            CloseHandle( h_smem );
            exit( 1 );
        }

        h_event = CreateEvent( nullptr, true, false, nullptr );
        if ( !h_event )
        {
            CloseHandle( h_sem_write_commands );
            CloseHandle( h_sem_read_commands );
            CloseHandle( h_mutex_write_commands );
            CloseHandle( h_mutex_read_commands );
            CloseHandle( h_mutex_instances_usage );
            CloseHandle( h_mutex_data_usage );
            DeleteCriticalSection( &cs_usage );
            UnmapViewOfFile( ptr_smem );
            CloseHandle( h_smem );
            exit( 1 );
        }

        h_updated_event = CreateEvent( nullptr, true, false, updated_event );
        if ( !h_updated_event )
        {
            CloseHandle( h_event );
            CloseHandle( h_sem_write_commands );
            CloseHandle( h_sem_read_commands );
            CloseHandle( h_mutex_write_commands );
            CloseHandle( h_mutex_read_commands );
            CloseHandle( h_mutex_instances_usage );
            CloseHandle( h_mutex_data_usage );
            DeleteCriticalSection( &cs_usage );
            UnmapViewOfFile( ptr_smem );
            CloseHandle( h_smem );
            exit( 1 );
        }

        h_thread = CreateThread( nullptr, NULL, reinterpret_cast<LPTHREAD_START_ROUTINE>( listenerRoutine ), this, NULL, nullptr );
        if ( !h_thread )
        {
            CloseHandle( h_updated_event );
            CloseHandle( h_event );
            CloseHandle( h_sem_write_commands );
            CloseHandle( h_sem_read_commands );
            CloseHandle( h_mutex_write_commands );
            CloseHandle( h_mutex_read_commands );
            CloseHandle( h_mutex_instances_usage );
            CloseHandle( h_mutex_data_usage );
            DeleteCriticalSection( &cs_usage );
            UnmapViewOfFile( ptr_smem );
            CloseHandle( h_smem );
            exit( 1 );
        }

        if ( is_main_instance )
            memset( ptr_smem, 0, sizeof( SMEM ) );

        WaitForSingleObjectEx( h_mutex_instances_usage, INFINITE, false );

        ptr_smem->num_instances++;

        ReleaseMutex( h_mutex_instances_usage );
    }

    ~SMem( )
    {
        if ( h_event )
            SetEvent( h_event );

        if ( h_thread )
        {
            if ( getStatus( ) == -1 )
                WaitForSingleObjectEx( h_thread, INFINITE, false );

            CloseHandle( h_thread );
        }

        if ( h_updated_event )
            CloseHandle( h_updated_event );

        if ( h_event )
            CloseHandle( h_event );

        if ( h_sem_write_commands )
            CloseHandle( h_sem_write_commands );

        if ( h_sem_read_commands )
            CloseHandle( h_sem_read_commands );

        if ( h_mutex_write_commands )
            CloseHandle( h_mutex_write_commands );

        if ( h_mutex_read_commands )
            CloseHandle( h_mutex_read_commands );

        if ( h_mutex_instances_usage )
            CloseHandle( h_mutex_instances_usage );

        if ( h_mutex_data_usage )
            CloseHandle( h_mutex_data_usage );

        DeleteCriticalSection( &cs_usage );

        if ( ptr_smem )
            UnmapViewOfFile( ptr_smem );

        if ( h_smem )
            CloseHandle( h_smem );
    }

    DWORD getStatus( )
    {
        DWORD status = NULL;

        switch ( GetExitCodeThread( h_thread, &status ) )
        {
        case STILL_ACTIVE:
            return -1;
        case NULL:
            return 1;
        default:
            return status;
        }
    }

    bool getData( DATA* ptr_data )
    {
        if ( WaitForSingleObjectEx( h_mutex_data_usage, INFINITE, false ) != WAIT_OBJECT_0 )
            return false;

        memcpy_s( ptr_data, sizeof( DATA ), &ptr_smem->data, sizeof( DATA ) );

        ReleaseMutex( h_mutex_data_usage );
        return true;
    }

    bool writeData( DATA* ptr_data )
    {
        if ( WaitForSingleObjectEx( h_mutex_data_usage, INFINITE, false ) != WAIT_OBJECT_0 )
            return false;

        memcpy_s( &ptr_smem->data, sizeof( DATA ), ptr_data, sizeof( DATA ) );

        ReleaseMutex( h_mutex_data_usage );

        if ( !SetEvent( h_updated_event ) )
            console::error( TEXT( "SetEvent failed: " ), GetLastError( ) );

        return true;
    }

    bool writeCommand( COMMAND* ptr_command )
    {
        if ( WaitForSingleObjectEx( h_sem_write_commands, INFINITE, false ) != WAIT_OBJECT_0 )
            return false;

        if ( WaitForSingleObjectEx( h_mutex_write_commands, INFINITE, false ) != WAIT_OBJECT_0 )
        {
            ReleaseSemaphore( h_sem_write_commands, 1, nullptr );
            return false;
        }

        memcpy_s( &ptr_smem->commands.commands[ ptr_smem->commands.write_index++ ], sizeof( COMMAND ), ptr_command, sizeof( COMMAND ) );

        if ( ptr_smem->commands.write_index == MAX_COMMANDS )
            ptr_smem->commands.write_index = 0;

        ReleaseMutex( h_mutex_write_commands );
        ReleaseSemaphore( h_sem_read_commands, 1, nullptr );

        return true;
    }

    bool registerCmdHandler( CommandHandler cmd_handler )
    {
        EnterCriticalSection( &cs_usage );

        this->cmd_handler = cmd_handler;

        LeaveCriticalSection( &cs_usage );

        return true;
    }

    bool registerHandler( DataHandler data_handler )
    {
        EnterCriticalSection( &cs_usage );

        this->data_handler = data_handler;

        LeaveCriticalSection( &cs_usage );

        return true;
    }

private:
    bool onProcessedInstance( )
    {
        if ( WaitForSingleObjectEx( h_mutex_instances_usage, INFINITE, false ) != WAIT_OBJECT_0 )
            return false;

        ptr_smem->processed_instances++;

        ReleaseMutex( h_mutex_instances_usage );

        return true;
    }

    bool resetProcessedInstance( )
    {
        if ( WaitForSingleObjectEx( h_mutex_instances_usage, INFINITE, false ) != WAIT_OBJECT_0 )
            return false;

        ptr_smem->processed_instances = 0;

        ReleaseMutex( h_mutex_instances_usage );

        return true;
    }

    static DWORD WINAPI listenerRoutine( SMem* _this )
    {
        DWORD ret_cause = NULL;
        while ( ( ret_cause = WaitForSingleObjectEx( _this->h_event, update_interval, false ) ) == WAIT_TIMEOUT )
        {
            auto ptr_smem = ( _this->ptr_smem );

            EnterCriticalSection( &_this->cs_usage );

            if ( WaitForSingleObjectEx( _this->h_updated_event, NULL, false ) == WAIT_OBJECT_0 )
            {
                if ( _this->data_handler )
                    _this->data_handler( ptr_smem->data );

                _this->onProcessedInstance( );

                if ( ptr_smem->processed_instances >= ptr_smem->num_instances )
                {
                    _this->resetProcessedInstance( );

                    ResetEvent( _this->h_updated_event );
                }
            }

            if ( WaitForSingleObjectEx( _this->h_sem_read_commands, NULL, false ) == WAIT_OBJECT_0 )
            {
                if ( WaitForSingleObjectEx( _this->h_mutex_read_commands, NULL, false ) != WAIT_OBJECT_0 )
                {
                    ReleaseSemaphore( _this->h_sem_read_commands, 1, nullptr );
                    LeaveCriticalSection( &_this->cs_usage );
                    continue;
                }

                const auto target_pid = ptr_smem->commands.commands[ ptr_smem->commands.read_index ].target_pid;
                if ( ( target_pid == -1 && _this->is_main_instance ) && target_pid != GetCurrentProcessId( ) && ( target_pid == 0 && !_this->is_main_instance ) )
                {
                    ReleaseMutex( _this->h_mutex_read_commands );
                    ReleaseSemaphore( _this->h_sem_read_commands, 1, nullptr );
                    LeaveCriticalSection( &_this->cs_usage );
                    continue;
                }

                if ( _this->cmd_handler )
                    _this->cmd_handler( ptr_smem->commands.commands[ ptr_smem->commands.read_index ] );

                ptr_smem->commands.read_index++;

                if ( _this->ptr_smem->commands.read_index == MAX_COMMANDS )
                    _this->ptr_smem->commands.read_index = 0;

                ReleaseMutex( _this->h_mutex_read_commands );
                ReleaseSemaphore( _this->h_sem_write_commands, 1, nullptr );
            }

            LeaveCriticalSection( &_this->cs_usage );
        }

        return ret_cause != WAIT_OBJECT_0;
    }
};