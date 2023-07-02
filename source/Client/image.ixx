module;

// workaround to intellisense that might be not as smart as we thought.
//
#if __INTELLISENSE__
#include <Windows.h>
#endif

#include <map>
#include "resource.h"

export module image;

#ifndef __INTELLISENSE__
import <Windows.h>;
#endif

import server;
import console;

namespace image
{
	namespace
	{
		std::map<UINT, HBITMAP> bitmap_set1;
		std::map<UINT, HBITMAP> bitmap_set2;
	}
}

export namespace image
{
	void initializeResources() {
		for (int i = 0; i < 12; ++i)
		{
			bitmap_set1[IDB_BITMAP1 + i] = LoadBitmap( GetModuleHandle( nullptr ), MAKEINTRESOURCE( IDB_BITMAP1 + i ) );
			bitmap_set2[IDB_BITMAP2 + i] = LoadBitmap( GetModuleHandle( nullptr ), MAKEINTRESOURCE( IDB_BITMAP2 + i ) );
		}
	}

	HBITMAP get(ENTITY_TYPE type, int bitmap_set, FACING direction = UP, int version = 0)
	{
		switch (type)
		{
		case ENTITY_TYPE_OBSTACLE:
			return bitmap_set == 1 ? bitmap_set1[IDB_BITMAP1_ROCK] : bitmap_set2[IDB_BITMAP2_ROCK];
		case ENTITY_TYPE_CAR:
			switch (direction)
			{
			case LEFT:
				return bitmap_set == 1 ? bitmap_set1[IDB_BITMAP1_CAR_LEFT] : bitmap_set2[IDB_BITMAP2_CAR_LEFT];
			case RIGHT:
				return bitmap_set == 1 ? bitmap_set1[IDB_BITMAP1_CAR_RIGHT] : bitmap_set2[IDB_BITMAP2_CAR_RIGHT];
			default:
				break;
			}
			break;
		case ENTITY_TYPE_FROG:
			switch (direction)
			{
			case LEFT:
				switch (version)
				{
				case 0:
					return bitmap_set == 1 ? bitmap_set1[IDB_BITMAP1_FROG1_LEFT] : bitmap_set2[IDB_BITMAP2_FROG1_LEFT];
				case 1:
					return bitmap_set == 1 ? bitmap_set1[IDB_BITMAP1_FROG2_LEFT] : bitmap_set2[IDB_BITMAP2_FROG2_LEFT];
				default:
					break;
				}
				break;
			case RIGHT:
				switch (version)
				{
				case 0:
					return bitmap_set == 1 ? bitmap_set1[IDB_BITMAP1_FROG1_RIGHT] : bitmap_set2[IDB_BITMAP2_FROG1_RIGHT];
				case 1:
					return bitmap_set == 1 ? bitmap_set1[IDB_BITMAP1_FROG2_RIGHT] : bitmap_set2[IDB_BITMAP2_FROG2_RIGHT];
				default:
					break;
				}
				break;
			case UP:
				switch (version)
				{
				case 0:
					return bitmap_set == 1 ? bitmap_set1[IDB_BITMAP1_FROG1_UP] : bitmap_set2[IDB_BITMAP2_FROG1_UP];
				case 1:
					return bitmap_set == 1 ? bitmap_set1[IDB_BITMAP1_FROG2_UP] : bitmap_set2[IDB_BITMAP2_FROG2_UP];
				default:
					break;
				}
				break;
			case DOWN:
				switch (version)
				{
				case 0:
					return bitmap_set == 1 ? bitmap_set1[IDB_BITMAP1_FROG1_DOWN] : bitmap_set2[IDB_BITMAP2_FROG1_DOWN];
				case 1:
					return bitmap_set == 1 ? bitmap_set1[IDB_BITMAP1_FROG2_DOWN] : bitmap_set2[IDB_BITMAP2_FROG2_DOWN];
				default:
					break;
				}
				break;
			default:
				break;
			}

		}
		return nullptr;
	}

	void deleteResources() {
		for (auto& [id, bitmap] : bitmap_set1)
		{
			DeleteObject( bitmap );
		}
		for (auto& [id, bitmap] : bitmap_set2)
		{
			DeleteObject( bitmap );
		}
	}
}