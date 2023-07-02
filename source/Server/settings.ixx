module;

// workaround to intellisense that might be not as smart as we thought
//
#if __INTELLISENSE__
#include <Windows.h>
#endif

export module settings;

#ifndef __INTELLISENSE__
import <Windows.h>;
#endif

import console;

export namespace settings
{
	inline int num_roads = 5;
	inline double init_car_speed = 1.0f;
	inline int init_car_number = 2;
	inline int tick_ms = 15;
	inline int max_afk_timer = 10000;

	void load( );

	void save( );
}

namespace settings
{
	inline constexpr auto identifier = TEXT( "CRR" );

	HKEY settings_key = nullptr;

	void load( )
	{
		if ( settings::settings_key )
			return;

		HKEY software_key = nullptr;

		if ( RegOpenKey( HKEY_CURRENT_USER, TEXT( "Software" ), &software_key ) != ERROR_SUCCESS )
		{
			console::error( "Error opening Software registry key" );
			return;
		}

		if ( RegCreateKeyEx( software_key, identifier, 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, nullptr, &settings::settings_key, nullptr ) != ERROR_SUCCESS )
		{
			console::error( "Error opening/creating ", identifier, " registry key" );
			return;
		}
		
		RegCloseKey( software_key );

		DWORD type = 0;
		DWORD size = sizeof( settings::num_roads );
		RegQueryValueEx( settings::settings_key, TEXT("num_roads"), nullptr, &type, reinterpret_cast<LPBYTE>( &settings::num_roads ), &size );
		
		size = sizeof( settings::init_car_speed );
		RegQueryValueEx( settings::settings_key, TEXT("init_car_speed"), nullptr, &type, reinterpret_cast<LPBYTE>( &settings::init_car_speed ), &size );
	}

	void save( )
	{
		if ( !settings::settings_key )
			return;

		RegSetValueEx( settings::settings_key, TEXT( "num_roads" ), 0, REG_BINARY, reinterpret_cast<LPBYTE>( &settings::num_roads ), sizeof( settings::num_roads ) );
		RegSetValueEx( settings::settings_key, TEXT( "init_car_speed" ), 0, REG_BINARY, reinterpret_cast<LPBYTE>( &settings::init_car_speed ), sizeof( settings::init_car_speed ) );
	
		RegCloseKey( settings::settings_key );
		settings::settings_key = nullptr;
	}
}