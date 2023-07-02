module;

// workaround to intellisense that might be not as smart as we thought
//
#if __INTELLISENSE__
#include <random>
#endif

export module utils;

#ifndef __INTELLISENSE__
import <random>;
#endif

export namespace utils
{
	int generateRandomInt( int min, int max )
	{
		static std::random_device dev;
		static std::minstd_rand num_gen( dev( ) );

		return static_cast<int>( num_gen( ) ) % ( max - min + 1 ) + min;
	}
}