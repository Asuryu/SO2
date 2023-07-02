#pragma once

#include <map>
#include <compare>
#include <Windows.h>
#include <functional>

#include "control.hpp"

import console;

typedef struct
{
    HICON icon, icon_sm;

    HBRUSH background_brush;

    UINT style;

    std::pair<int, int> pos;
    std::pair<int, int> measures;
} WND_INFO;

class Window
{
private:
    HWND hwnd = nullptr;

    console::tstring class_name = nullptr, title = nullptr;

    std::vector<Control*> childs;

    std::map<UINT, std::function<std::pair<bool, LRESULT>(WPARAM wParam, LPARAM lParam)>> callbacks_map;

public:
	Window( const WND_INFO& wnd, const console::tstring& title, const console::tstring& class_name ) :
        title( title ), class_name( class_name )
	{
        WNDCLASSEX wnd_class;

        wnd_class.cbSize = sizeof( WNDCLASSEX );

        wnd_class.lpszMenuName = nullptr;
        wnd_class.lpszClassName = class_name.c_str( );

        wnd_class.lpfnWndProc = WndProc;
        wnd_class.hInstance = GetModuleHandle(nullptr);

        wnd_class.hIcon = wnd.icon;
        wnd_class.hIconSm = wnd.icon_sm;

        wnd_class.style = wnd.style;
        wnd_class.hbrBackground = wnd.background_brush;

        wnd_class.cbClsExtra = 0;
        wnd_class.cbWndExtra = 0;
        wnd_class.hCursor = LoadCursor( nullptr, IDC_ARROW );

		RegisterClassEx( &wnd_class );

        hwnd = CreateWindow( class_name.c_str( ), title.c_str( ), WS_OVERLAPPEDWINDOW, wnd.pos.first, wnd.pos.second, 
            wnd.measures.first, wnd.measures.second, nullptr, nullptr, wnd_class.hInstance, this );
        if ( !hwnd )
        {
            console::error( TEXT( "" ) );
            return;
        }

        ShowWindow( hwnd, SW_SHOWNORMAL );
        UpdateWindow( hwnd );
	}

    ~Window( )
    {
        destroyControls( );

        DestroyWindow( hwnd );
    }

    bool registerCallback( UINT message, std::function<std::pair<bool, LRESULT>( WPARAM wParam, LPARAM lParam )> callback )
    {
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

    bool createControl( const CONTROL_INFO& info )
    {
        const auto ptr_ctrl = new Control( hwnd, info );
        if ( ptr_ctrl->getHandle( ) )
        {
            childs.push_back( ptr_ctrl );
            return true;
        }

        delete ptr_ctrl;
        return false;
    }

    bool createControl( const CONTROL_INFO& info, Control*& ptr_ctrl )
    {
        ptr_ctrl = new Control( hwnd, info );
        if ( ptr_ctrl->getHandle( ) )
        {
            childs.push_back( ptr_ctrl );
            return true;
        }

        delete ptr_ctrl;
        ptr_ctrl = nullptr;
        return false;
    }

    void destroyControls( )
    {
        for ( const auto child : childs )
            delete child;

        childs.clear( );
    }

    const std::vector<Control*>& getChilds( )
    {
        return childs;
    }
    
    void redraw( )
    {
        InvalidateRect( hwnd, nullptr, true );
    }

private:
    static LRESULT CALLBACK WndProc( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam )
    {
        // obtain _this pointer from hwnd object
        //
        Window* _this = nullptr;
        if ( message == WM_NCCREATE )
        {
            _this = reinterpret_cast<Window*>( reinterpret_cast<CREATESTRUCT*>( lParam )->lpCreateParams );

            SetWindowLongPtr( hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>( _this ) );
        }
        else
            _this = reinterpret_cast<Window*>( GetWindowLongPtr( hwnd, GWLP_USERDATA ) );

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
        return DefWindowProc( hwnd, message, wParam, lParam );
    }

    std::pair<bool, LRESULT> processMessage( UINT message, WPARAM wParam, LPARAM lParam )
    {
        return callbacks_map[ message ] ? callbacks_map[ message ]( wParam, lParam ) :
            std::make_pair( false, static_cast<LRESULT>( -1 ) );
    }
};