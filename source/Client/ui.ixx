module;

// workaround to intellisense that might be not as smart as we thought.
//
#if __INTELLISENSE__
#include <Windows.h>
#include <windowsx.h>
#include <functional>
#endif

#include "resource.h"

#include "window.hpp"

#include "DraculaTheme.hpp"

export module ui;

#ifndef __INTELLISENSE__
import <Windows.h>;
import <windowsx.h>;
import <functional>;
#endif

import image;
import server;
import console;
import settings;

export typedef enum
{
    ON_START = IDC_START_BTN,
    ON_ABOUT = IDC_ABOUT_BTN,
    ON_EXIT = IDC_EXIT_BTN,
    ON_BACK = IDC_BACK_BTN,
    ON_CHANGE_BITMAP = IDC_CHANGE_BITMAP_BTN,
    ON_SINGLEPLAYER = IDC_SP_BTN,
    ON_MULTIPLAYER = IDC_MP_BTN,
    ON_RBTN_DOWN = WM_RBUTTONDOWN
} UI_CALLBACK_TYPE;

export typedef enum
{
    KEY_UP = VK_UP,
    KEY_DOWN = VK_DOWN,
    KEY_LEFT = VK_LEFT,
    KEY_RIGHT = VK_RIGHT
} UI_DIRECTION_TYPE;

export typedef enum
{
    PLAYER1 = IDC_PLAYER1_STATIC,
    PLAYER2 = IDC_PLAYER2_STATIC
} UI_CLICK_TYPE;

export class UI
{
private:
    DATA game_data { };
    HDC mem_hdc = nullptr;

    Window* ptr_wnd = nullptr;
    Control* ptr_game_board = nullptr;
    HFONT title_font = nullptr, description_font = nullptr;
    HBRUSH btn_background = nullptr;

    CRITICAL_SECTION critical_section { };

    int current_bitmap = 1;

    std::pair<int, int> measures = std::make_pair( 1000, 1000 );

    std::map<UI_CALLBACK_TYPE, std::function<void( )>> callbacks_map;
    std::map<UI_DIRECTION_TYPE, std::function<void( )>> direction_callbacks_map;
    std::map<UI_CLICK_TYPE, std::function<void( bool right_click )>> click_callbacks_map;

    static constexpr auto square_size = std::make_pair( 40, 40 );

public:
    UI( )
    {
        console::log( TEXT( "UI Constructor" ) );

        image::initializeResources( );

        InitializeCriticalSectionEx( &critical_section, settings::tick_ms, NULL );

        WND_INFO info;
        info.style = CS_HREDRAW | CS_VREDRAW;
        info.pos = std::make_pair( CW_USEDEFAULT, NULL );
        info.measures = std::make_pair( measures.first, measures.second );
        info.icon = LoadIcon( GetModuleHandle( nullptr ), static_cast<TCHAR*>( MAKEINTRESOURCE( IDI_LOGO ) ) );
        info.icon_sm = LoadIcon( GetModuleHandle( nullptr ), static_cast<TCHAR*>( MAKEINTRESOURCE( IDI_SMALL_LOGO ) ) );
        info.background_brush = CreateSolidBrush( DraculaTheme::BACKGROUND_DARKER );

        ptr_wnd = new Window( info, TEXT( "CRR Client - Game" ), TEXT( "CRR_CLIENT_WND" ) );

        title_font = CreateFont( measures.second * 1 / 12, ( ( measures.first * 6 / 8 ) / 20 ) - 4, 0, 0,
            FW_BOLD, false, false, false, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
            DEFAULT_PITCH | FF_SWISS, TEXT( "Arial" ) );

        description_font = CreateFont( 24, 0, 0, 0, FW_NORMAL, false, false, false, ANSI_CHARSET, OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, TEXT( "Arial" ) );

        btn_background = CreateSolidBrush( DraculaTheme::BACKGROUND );

        ptr_wnd->registerCallback( WM_CTLCOLORSTATIC, [ & ] ( WPARAM wParam, LPARAM lParam )
            {
                return this->ctlColorStatic( wParam, lParam );
            } );
        ptr_wnd->registerCallback( WM_CTLCOLOREDIT, [ & ] ( WPARAM wParam, LPARAM lParam )
            {
                return this->ctlColorEdit( wParam, lParam );
            } );
        ptr_wnd->registerCallback( WM_CTLCOLORBTN, [ & ] ( WPARAM wParam, LPARAM lParam )
            {
                return this->ctlColorBtn( wParam, lParam );
            } );
        ptr_wnd->registerCallback( WM_ERASEBKGND, [ & ] ( WPARAM wParam, LPARAM lParam )
            {
                return this->eraseBkgnd( wParam, lParam );
            } );
        ptr_wnd->registerCallback( WM_DRAWITEM, [ & ] ( WPARAM wParam, LPARAM lParam )
            {
                return this->drawItem( wParam, lParam );
            } );
        ptr_wnd->registerCallback( WM_DESTROY, [ & ] ( WPARAM wParam, LPARAM lParam )
            {
                return this->destroy( wParam, lParam );
            } );
        ptr_wnd->registerCallback( WM_COMMAND, [ & ] ( WPARAM wParam, LPARAM lParam )
            {
                return command( wParam, lParam );
            } );
        ptr_wnd->registerCallback( WM_KEYDOWN, [ & ] ( WPARAM wParam, LPARAM lParam )
            {
                return this->handleKeyDown( wParam, lParam );
            } );
        ptr_wnd->registerCallback( WM_LBUTTONDOWN, [ & ] ( WPARAM wParam, LPARAM lParam )
            {
                return this->handleClick( wParam, lParam, false );
            } );
        ptr_wnd->registerCallback( WM_RBUTTONDOWN, [ & ] ( WPARAM wParam, LPARAM lParam )
            {
                return this->handleClick( wParam, lParam, true );
            } );

        drawMenu( );
    }

    ~UI( )
    {
        delete ptr_wnd;
        ptr_wnd = nullptr;

        DeleteCriticalSection( &critical_section );

        image::deleteResources( );

        console::log( TEXT( "UI Destructor" ) );
    }

    void changeCurrentBitmap() {
        current_bitmap = (current_bitmap + 1) % 2;
    }

    bool registerDirectionCallback( UI_DIRECTION_TYPE type, std::function<void( )> direction_callback )
    {
        if ( direction_callbacks_map.find( type ) != direction_callbacks_map.end( ) )
            return false;

        direction_callbacks_map[ type ] = direction_callback;
        return true;
    }

    bool removeDirectionCallback( UI_DIRECTION_TYPE type )
    {
        if ( direction_callbacks_map.find( type ) == direction_callbacks_map.end( ) )
            return false;

        direction_callbacks_map.erase( type );
        return true;
    }

    bool registerClickCallback( UI_CLICK_TYPE type, std::function<void( bool right_click )> callback )
    {
        if ( click_callbacks_map.find( type ) != click_callbacks_map.end( ) )
            return false;

        click_callbacks_map[ type ] = callback;
        return true;
    }

    bool removeClickCallback( UI_CLICK_TYPE type )
    {
        if ( click_callbacks_map.find( type ) == click_callbacks_map.end( ) )
            return false;

        click_callbacks_map.erase( type );
        return true;
    }

    bool registerCallback( UI_CALLBACK_TYPE type, std::function<void( )> callback )
    {
        if ( callbacks_map.find( type ) != callbacks_map.end( ) )
            return false;

        callbacks_map[ type ] = callback;
        return true;
    }

    bool removeCallback( UI_CALLBACK_TYPE type )
    {
        if ( callbacks_map.find( type ) == callbacks_map.end( ) )
            return false;

        callbacks_map.erase( type );
        return true;
    }

    void drawMenu( )
    {
        ptr_wnd->destroyControls( );

        CONTROL_INFO info;
        info.style = WS_VISIBLE | WS_CHILD;

        info.class_name = TEXT( "static" );
        info.text = TEXT( "Crossy Road Reloaded" );
        info.menu = reinterpret_cast<HMENU>( IDC_TITLE_STATIC );
        info.measures = std::make_pair( measures.first * 6 / 8, measures.second * 1 / 12 );
        info.pos = std::make_pair( ( measures.first - info.measures.first ) / 2,
            measures.second * 1 / 4 - info.measures.second / 2 );

        Control* ptr_ctrl = nullptr;
        ptr_wnd->createControl( info, ptr_ctrl );
        ptr_ctrl->setFont( title_font );

        info.style |= BS_OWNERDRAW;
        info.class_name = TEXT( "button" );
        info.measures = std::make_pair( 100, 50 );

        int spacing = 10;
        int total_width = 3 * info.measures.first + 2 * spacing;
        int initial_x = ( measures.first - total_width ) / 2;

        info.text = TEXT( "Start" );
        info.menu = reinterpret_cast<HMENU>( IDC_START_BTN );
        info.pos = std::make_pair( initial_x,
            measures.second * 3 / 4 - info.measures.second / 2 );
        ptr_wnd->createControl( info );

        info.text = TEXT( "Exit" );
        info.menu = reinterpret_cast<HMENU>( IDC_EXIT_BTN );
        info.pos = std::make_pair( initial_x + info.measures.first + spacing,
            measures.second * 3 / 4 - info.measures.second / 2 );
        ptr_wnd->createControl( info );

        info.text = TEXT( "About" );
        info.menu = reinterpret_cast<HMENU>( IDC_ABOUT_BTN );
        info.pos = std::make_pair( initial_x + 2 * ( info.measures.first + spacing ),
            measures.second * 3 / 4 - info.measures.second / 2 );
        ptr_wnd->createControl( info );
    }

    void drawGame( )
    {
        ptr_wnd->destroyControls( );

        CONTROL_INFO info;
        info.class_name = TEXT( "static" );
        info.style = SS_CENTER | WS_VISIBLE | WS_CHILD;
        info.pos = std::make_pair( 0, 0 );
        info.measures = measures;
        ptr_wnd->createControl( info, ptr_game_board );

        info.style |= BS_OWNERDRAW;
        info.class_name = TEXT("button");
        info.measures = std::make_pair(160, 50);
        info.text = TEXT("Change Bitmap Set");
        info.menu = reinterpret_cast<HMENU>(IDC_CHANGE_BITMAP_BTN);
        info.pos = std::make_pair(measures.first / 2 - info.measures.first / 2,
            			measures.second * 3 / 4 - info.measures.second / 2);

        ptr_wnd->createControl(info);

        ptr_game_board->registerCallback( WM_PAINT, [ & ] ( WPARAM wParam, LPARAM )
            {
                return paintGame( );
            } );
        ptr_game_board->registerCallback( WM_DESTROY, [ & ] ( WPARAM wParam, LPARAM lParam )
            {
                if ( mem_hdc )
                {
                    DeleteDC( mem_hdc );
                    mem_hdc = nullptr;
                }

                PostQuitMessage( 0 );
                return std::make_pair( true, 0 );
            } );

        SetWindowPos( ptr_game_board->getHandle( ), HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE );
    }

    void drawPreGame( )
    {
        ptr_wnd->destroyControls( );

        CONTROL_INFO info;
        info.style = WS_VISIBLE | WS_CHILD;

        info.class_name = TEXT( "static" );
        info.text = TEXT( "Crossy Road Reloaded" );
        info.menu = reinterpret_cast<HMENU>( IDC_TITLE_STATIC );
        info.measures = std::make_pair( measures.first * 6 / 8, measures.second * 1 / 12 );
        info.pos = std::make_pair( ( measures.first - info.measures.first ) / 2,
            measures.second * 1 / 4 - info.measures.second / 2 );

        Control* ptr_ctrl = nullptr;
        ptr_wnd->createControl( info, ptr_ctrl );
        ptr_ctrl->setFont( title_font );

        info.text = TEXT( "Enter your Nickname:" );
        info.menu = reinterpret_cast<HMENU>( IDC_DESCRIPTION_STATIC );
        info.measures = std::make_pair( 225, 30 );
        info.pos = std::make_pair( ( measures.first - info.measures.first - 150 ) / 2,
            measures.second * 3 / 5 - info.measures.second / 2 );

        ptr_wnd->createControl( info, ptr_ctrl );
        ptr_ctrl->setFont( description_font );

        info.style |= ES_CENTER | WS_BORDER;
        info.text = TEXT( "Player" );
        info.class_name = TEXT( "edit" );
        info.menu = reinterpret_cast<HMENU>( IDC_NAME_EDIT );
        info.pos = std::make_pair( info.pos.first + info.measures.first, info.pos.second - 2 );
        info.measures = std::make_pair( 150, 30 );

        ptr_wnd->createControl( info, ptr_ctrl );
        ptr_ctrl->setFont( description_font );
        info.style &= ~ES_CENTER;
        info.style &= ~WS_BORDER;

        info.style |= BS_OWNERDRAW;
        info.class_name = TEXT( "button" );
        info.measures = std::make_pair( 150, 50 );

        int spacing = 10;
        int total_width = 3 * info.measures.first + 2 * spacing;
        int initial_x = ( measures.first - total_width ) / 2;

        info.text = TEXT( "Singleplayer" );
        info.menu = reinterpret_cast<HMENU>( IDC_SP_BTN );
        info.pos = std::make_pair( initial_x,
            measures.second * 3 / 4 - info.measures.second / 2 );
        ptr_wnd->createControl( info );

        info.text = TEXT( "Multiplayer" );
        info.menu = reinterpret_cast<HMENU>( IDC_MP_BTN );
        info.pos = std::make_pair( initial_x + info.measures.first + spacing,
            measures.second * 3 / 4 - info.measures.second / 2 );
        ptr_wnd->createControl( info );

        info.text = TEXT( "Back" );
        info.menu = reinterpret_cast<HMENU>( IDC_BACK_BTN );
        info.pos = std::make_pair( initial_x + 2 * ( info.measures.first + spacing ),
            measures.second * 3 / 4 - info.measures.second / 2 );
        ptr_wnd->createControl( info, ptr_ctrl );
        registerCallback( ON_BACK, [ & ] ( )
            {
                this->drawMenu( );
            } );
    }

    void updateGameData( const DATA& data )
    {
        EnterCriticalSection( &critical_section );

        memcpy( &game_data, &data, sizeof( data ) );

        LeaveCriticalSection( &critical_section );

        ptr_game_board->redraw( );
    }

private:
    std::pair<bool, LRESULT> ctlColorStatic( WPARAM wParam, LPARAM lParam )
    {
        const auto hdc = reinterpret_cast<HDC>( wParam );

        switch ( reinterpret_cast<WPARAM>( GetMenu( WindowFromDC( hdc ) ) ) )
        {
        case IDC_TITLE_STATIC:
            SetTextColor( hdc, DraculaTheme::GREEN );
            SetBkMode( hdc, TRANSPARENT );
            break;
        default:
            SetTextColor( hdc, DraculaTheme::FOREGROUND );
            SetBkMode( hdc, TRANSPARENT );
            break;
        }

        return std::make_pair( true, static_cast<LRESULT>( GetClassLongPtr( ptr_wnd->getHandle( ), GCLP_HBRBACKGROUND ) ) );
    }

    std::pair<bool, LRESULT> ctlColorEdit( WPARAM wParam, LPARAM lParam )
    {
        const auto hdc = reinterpret_cast<HDC>( wParam );

        SetTextColor( hdc, DraculaTheme::FOREGROUND );
        SetBkMode( hdc, TRANSPARENT );

        return std::make_pair( true, reinterpret_cast<LRESULT>( btn_background ) );
    }

    std::pair<bool, LRESULT> ctlColorBtn( WPARAM wParam, LPARAM lParam )
    {
        return std::make_pair( true, reinterpret_cast<LRESULT>( btn_background ) );
    }

    std::pair<bool, LRESULT> eraseBkgnd( WPARAM wParam, LPARAM lParam )
    {
        return std::make_pair( true, 1 );
    }

    std::pair<bool, LRESULT> drawItem( WPARAM wParam, LPARAM lParam )
    {
        const auto lp_draw = reinterpret_cast<LPDRAWITEMSTRUCT>( lParam );

        SetTextColor( lp_draw->hDC, DraculaTheme::FOREGROUND );
        SetBkMode( lp_draw->hDC, TRANSPARENT );

        const auto len = GetWindowTextLength( lp_draw->hwndItem );
        const auto ptr_text = static_cast<TCHAR*>( VirtualAlloc( nullptr, len + 1, MEM_COMMIT, PAGE_READWRITE ) );
        if ( !ptr_text )
            return std::make_pair( false, static_cast<LRESULT>( 0 ) );

        GetWindowText( lp_draw->hwndItem, ptr_text, len + 1 );
        DrawText( lp_draw->hDC, ptr_text, -1, &lp_draw->rcItem, DT_SINGLELINE | DT_VCENTER | DT_CENTER );

        VirtualFree( ptr_text, NULL, MEM_RELEASE );

        return std::make_pair( true, static_cast<LRESULT>( 0 ) );
    }

    std::pair<bool, LRESULT> destroy( WPARAM wParam, LPARAM lParam )
    {
        if ( btn_background )
        {
            DeleteObject( btn_background );
            btn_background = nullptr;
        }

        if ( title_font )
        {
            DeleteObject( title_font );
            title_font = nullptr;
        }

        if ( description_font )
        {
            DeleteObject( description_font );
            description_font = nullptr;
        }

        DeleteObject( reinterpret_cast<HBRUSH>( GetClassLongPtr( ptr_wnd->getHandle( ), GCLP_HBRBACKGROUND ) ) );
        PostQuitMessage( 0 );
        return std::make_pair( true, 0 );
    }

    std::pair<bool, LRESULT> command( WPARAM wParam, LPARAM lParam )
    {
        const auto id = static_cast<UI_CALLBACK_TYPE>( wParam );

        if ( callbacks_map.contains( id ) )
            callbacks_map[ id ]( );

        return std::make_pair( true, 0 );
    }

    std::pair<bool, LRESULT> handleKeyDown( WPARAM wParam, LPARAM lParam )
    {

        if ( direction_callbacks_map.contains( static_cast<UI_DIRECTION_TYPE>( wParam ) ) )
            direction_callbacks_map[ static_cast<UI_DIRECTION_TYPE>( wParam ) ]( );

        return std::make_pair( true, static_cast<LRESULT>( 0 ) );
    }

    std::pair<bool, LRESULT> handleClick( WPARAM wParam, LPARAM lParam, bool right_click )
    {
        POINT pt;
        pt.x = GET_X_LPARAM( lParam );
        pt.y = GET_Y_LPARAM( lParam );

        const auto hwnd = ChildWindowFromPointEx( ptr_wnd->getHandle( ), pt, CWP_ALL );
        const auto menu = GetMenu( hwnd );
        const auto menu_id = static_cast<UI_CLICK_TYPE>( GetMenuItemID( menu, NULL ) );

        if ( click_callbacks_map.contains( menu_id ) )
            click_callbacks_map[ menu_id ]( right_click );

        return std::make_pair( true, static_cast<LRESULT>( 0 ) );
    }

    std::pair<bool, LRESULT> paintGame( )
    {
        EnterCriticalSection( &critical_section );

        PAINTSTRUCT ps { };
        const auto hdc = BeginPaint( ptr_game_board->getHandle( ), &ps );

        RECT rect;
        GetClientRect( ptr_game_board->getHandle( ), &rect );

        if ( !mem_hdc )
        {
            mem_hdc = CreateCompatibleDC( hdc );

            const auto h_bm = CreateCompatibleBitmap( hdc, rect.right, rect.bottom );
            SelectObject( mem_hdc, h_bm );

            DeleteObject( h_bm );
        }

        FillRect( mem_hdc, &rect, reinterpret_cast<HBRUSH>( GetClassLongPtr( ptr_wnd->getHandle( ), GCLP_HBRBACKGROUND ) ) );

        if ( game_data.height && game_data.width )
        {
            const auto measures = std::make_pair( square_size.first * game_data.width, square_size.second * game_data.height );
            const auto pos = std::make_pair( ( this->measures.first - measures.first ) / 2, ( this->measures.second - measures.second ) / 2 );

            for ( int i = 0; i < game_data.num_entities; i++ )
            {
                const auto& entity = game_data.entities[ i ];

                if ( entity.pos_x >= game_data.width || entity.pos_x < 0 ||
                    entity.pos_y >= game_data.height || entity.pos_y < 0 )
                {
                    console::log( TEXT( "Invalid Entity found at (" ), entity.pos_x, TEXT( ", " ), entity.pos_y, TEXT( "): 0x" ), &entity );
                    continue;
                }

                HBITMAP h_image = image::get( entity.type, current_bitmap, entity.direction );
                if ( !h_image )
                    continue;

                BITMAP bm;
                GetObject( h_image, sizeof( bm ), &bm );

                auto image_hdc = CreateCompatibleDC( hdc );
                auto old_bm = SelectObject( image_hdc, h_image );

                const auto x = pos.first + entity.pos_x * square_size.first;
                const auto y = ( pos.second + measures.second ) - ( entity.pos_y + 1 ) * square_size.second;

                StretchBlt( mem_hdc, x, y, square_size.first, square_size.second, image_hdc, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY );

                SelectObject( image_hdc, old_bm );
                DeleteDC( image_hdc );
            }
        }

        LeaveCriticalSection( &critical_section );

        BitBlt( hdc, 0, 0, rect.right, rect.bottom, mem_hdc, 0, 0, SRCCOPY );

        EndPaint( ptr_game_board->getHandle( ), &ps );
        ValidateRect( ptr_game_board->getHandle( ), nullptr );

        return std::make_pair( true, 0 );
    }
};