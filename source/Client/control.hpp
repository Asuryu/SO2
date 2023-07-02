#pragma once

#include <map>
#include <compare>
#include <Windows.h>
#include <functional>

import console;

typedef struct
{
    console::tstring class_name, text;

    DWORD style;

    std::pair<int, int> pos;
    std::pair<int, int> measures;

    HMENU menu;
} CONTROL_INFO;

class Control
{
private:
    HWND hwnd = nullptr, parent_wnd = nullptr;

    std::map<UINT, std::function<std::pair<bool, LRESULT>( WPARAM wParam, LPARAM lParam )>> callbacks_map;

public:
    Control( const HWND parent_wnd, const CONTROL_INFO& info ) :
        parent_wnd( parent_wnd )
    {
        hwnd = CreateWindow( info.class_name.c_str( ), info.text.c_str( ),
            info.style, info.pos.first, info.pos.second, info.measures.first, info.measures.second, 
            parent_wnd, info.menu, GetModuleHandle( nullptr ), nullptr );
    }

    ~Control( )
    {
        DestroyWindow( hwnd );
    }

    bool registerCallback( UINT message, std::function<std::pair<bool, LRESULT>( WPARAM wParam, LPARAM lParam )> callback )
    {
        static auto init = [ & ] ( )
        {
            SetWindowLongPtr( hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>( this ) );
            SetWindowLongPtr( hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>( WndProc ) );

            return true;
        } ( );

        if ( callbacks_map.find( message ) != callbacks_map.end( ) )
            return false;

        callbacks_map[ message ] = callback;
        return true;
    }

    bool removeCallback( UINT message )
    {
        if ( callbacks_map.find( message ) == callbacks_map.end( ) )
            return false;

        callbacks_map.erase( message );
        return true;
    }

    HWND getHandle( )
    {
        return hwnd;
    }

    void redraw( )
    {
        InvalidateRect( hwnd, nullptr, true );
    }

    void setFont( HFONT font )
    {
        SendMessage( hwnd, WM_SETFONT, reinterpret_cast<WPARAM>( font ), true );
    }

    LONG setLong( int nIndex, LONG value )
    {
        return SetWindowLong( hwnd, nIndex, value );
    }

private:
    static LRESULT CALLBACK WndProc( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam )
    {
        // obtain _this pointer from hwnd object
        //
        Control* _this = reinterpret_cast<Control*>( GetWindowLongPtr( hwnd, GWLP_USERDATA ) );
        if ( !_this )
            return CallWindowProc( DefWindowProc, hwnd, message, wParam, lParam );

        // create lambda to erase the object pointer from the hwnd object
        //
        const auto cleanupOnDestroy = [ & ] ( ) noexcept -> void
        {
            if ( message != WM_NCDESTROY )
                return;

            SetWindowLongPtr( hwnd, GWLP_USERDATA, NULL );
        };

        // call object processMessage 
        //
        if ( _this )
        {
            const auto ret = _this->processMessage( message, wParam, lParam );
            if ( ret.first )
            {
                cleanupOnDestroy( );
                return ret.second;
            }
        }

        // call default wndproc
        //
        cleanupOnDestroy( );
        return CallWindowProc( DefWindowProc, hwnd, message, wParam, lParam );
    }

    std::pair<bool, LRESULT> processMessage( UINT message, WPARAM wParam, LPARAM lParam )
    {
        return callbacks_map[ message ] ? callbacks_map[ message ]( wParam, lParam ) :
            std::make_pair( false, static_cast<LRESULT>( -1 ) );
    }
};