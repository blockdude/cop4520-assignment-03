#include <iostream>
#include <random>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>

#define SENSOR_COUNT 8
#define MINUTES_TO_RUN 120
#define MINUTES_IN_HOUR 60
#define MAX_HIGH_COUNT 5
#define MAX_LOW_COUNT 5

struct sensor
{
	int readings[ MINUTES_IN_HOUR ];
	int done;
	size_t idx;

	std::mutex *mtx;
};

// atmospheric temperature module
struct atm
{
	struct sensor sensors[ SENSOR_COUNT ];
	std::condition_variable cv;
	std::mutex *mtx;

	int ready;

	// counts how many sensors are done
	std::atomic< int > count;
};

struct temp_data
{
	int high[ MAX_HIGH_COUNT ];
	size_t high_count;

	int low[ 5 ];
	size_t low_count;
};

void temp_data_init( struct temp_data *td )
{
	td->high_count = 0;
	td->low_count = 0;
}

void sensor_init( struct sensor *s )
{
	if ( s == NULL )
		return;

	s->idx = 0;
	s->done = 0;
	s->mtx = new std::mutex();
}

void sensor_free( struct sensor *s )
{
	if ( s == NULL )
		return;

	delete s->mtx;
}

void atm_init( struct atm *atm )
{
	if ( atm == NULL )
		return;

	for ( int i = 0; i < SENSOR_COUNT; i++ )
	{
		sensor_init( &atm->sensors[ i ] );
	}

	atm->mtx = new std::mutex();
	atm->ready = 0;
	atm->count = 0;
}

void atm_free( struct atm *atm )
{
	if ( atm == NULL )
		return;

	for ( int i = 0; i < SENSOR_COUNT; i++ )
	{
		sensor_free( &atm->sensors[ i ] );
	}

	delete atm->mtx;
}

// thread safe random number generator
int gen_rand( int min, int max )
{
	std::random_device rd;
	static thread_local std::mt19937 generator(rd());
	std::uniform_int_distribution< int > distribution( min, max );
	return distribution( generator );
}

void sensor_logic( struct atm *atm, std::atomic< int > *time, int id )
{
	while ( 1 )
	{
		struct sensor *sen = &atm->sensors[ id ];
		std::unique_lock< std::mutex > lk( *sen->mtx );
		atm->cv.wait( lk, [ &sen, &atm ]{ return sen->done == 0 && atm->ready; } );

		if ( time->fetch_add( 0 ) > MINUTES_TO_RUN )
			break;

		sen->readings[ sen->idx ] = gen_rand( -100, 70 );
		sen->idx = ( sen->idx + 1 ) % MINUTES_IN_HOUR;
		sen->done = 1;

		lk.unlock();

		atm->count.fetch_add( 1 );
	}
}

void reset_sensor_status( struct atm *atm )
{
	for ( int i = 0; i < SENSOR_COUNT; i++ )
	{
		atm->sensors[ i ].mtx->lock();
		atm->sensors[ i ].done = 0;
		atm->sensors[ i ].mtx->unlock();
	}
}

void insert_low( struct temp_data *data, int val )
{
	// smaller than all values
	if ( data->low_count >= MAX_HIGH_COUNT && data->low[ MAX_HIGH_COUNT - 1 ] < val )
		return;

	// fill array if it is not filled
	if ( data->low_count < MAX_HIGH_COUNT )
	{
		data->low[ data->low_count ] = val;
		data->low_count++;
	}

	// index of current val
	size_t i = data->low_count - 1;

	if ( data->low[ i ] < val )
		return;

	// replace current smallest val
	data->low[ i ] = val;

	// insertion sort
	while ( i > 0 && data->low[ i ] < data->low[ i - 1 ] )
	{
		int tmp = data->low[ i ];
		data->low[ i ] = data->low[ i - 1 ];
		data->low[ i - 1 ] = tmp;
		i--;
	}
}

void insert_high( struct temp_data *data, int val )
{
	// smaller than all values
	if ( data->high_count >= MAX_HIGH_COUNT && data->high[ MAX_HIGH_COUNT - 1 ] > val )
		return;

	// fill array if it is not filled
	if ( data->high_count < MAX_HIGH_COUNT )
	{
		data->high[ data->high_count ] = val;
		data->high_count++;
	}

	// index of current val
	size_t i = data->high_count - 1;

	if ( data->high[ i ] > val )
		return;

	// replace current smallest val
	data->high[ i ] = val;

	// insertion sort
	while ( i > 0 && data->high[ i ] > data->high[ i - 1 ] )
	{
		int tmp = data->high[ i ];
		data->high[ i ] = data->high[ i - 1 ];
		data->high[ i - 1 ] = tmp;
		i--;
	}
}

void report( struct atm *atm )
{
	struct temp_data td;
	temp_data_init( &td );
	
	// find highs and lows
	for ( int i = 0; i < SENSOR_COUNT; i++ )
	{
		for ( int t = 0; t < MINUTES_IN_HOUR; t++ )
		{
			insert_high( &td, atm->sensors[ i ].readings[ t ] );
			insert_low( &td, atm->sensors[ i ].readings[ t ] );
		}
	}

	// find greatest difference
	int diff = 0;
	for ( int t = 0; t < MINUTES_IN_HOUR - 10; t++ )
	{
		int max = atm->sensors[ 0 ].readings[ t ];
		int min = atm->sensors[ 0 ].readings[ t ];

		for ( int i = 0; i < SENSOR_COUNT; i++ )
		{
			for ( int j = 0; j < 10; j++ )
			{
				int a = atm->sensors[ i ].readings[ j ];

				if ( a > max ) max = a;
				if ( a < min ) min = a;
			}
		}

		if ( max - min > diff ) diff = max - min;
	}

	printf( "Top 5 highest temperatures: " );
	for ( int i = 0; i < MAX_HIGH_COUNT; i++ )
		printf( "%d, ", td.high[ i ] );
	printf( "\n" );

	printf( "Top 5 lowest temperatures: " );
	for ( int i = MAX_LOW_COUNT - 1; i >= 0; i-- )
		printf( "%d, ", td.low[ i ] );
	printf( "\n" );

	printf( "Greatest difference: %d\n", diff );
}

void read_temps( void )
{
	std::atomic< int > time = 0;
	std::thread sensors[ SENSOR_COUNT ];
	struct atm atm;
	atm_init( &atm );

	for ( int i = 0; i < SENSOR_COUNT; i++ )
	{
		sensors[ i ] = std::thread( sensor_logic, &atm, &time, i );
	}

	atm.ready = 1;
	while ( time.fetch_add( 0 ) < MINUTES_TO_RUN + 1 )
	{
		atm.cv.notify_all();

		// wait for sensors to be done
		while ( atm.count.fetch_add( 0 ) < SENSOR_COUNT );

		atm.count = 0;
		reset_sensor_status( &atm );

		time.fetch_add( 1 );

		if ( time.fetch_add( 0 ) % MINUTES_IN_HOUR == 0 )
		{
			printf( "Hour: %d\n", time.fetch_add( 0 ) / MINUTES_IN_HOUR );
			printf( "----------------\n" );
			report( &atm );
			printf( "----------------\n" );
		}
	}

	atm.cv.notify_all();
	for ( int i = 0; i < SENSOR_COUNT; i++ )
	{
		sensors[ i ].join();
	}

	atm_free( &atm );
}

int main( void )
{
	read_temps();
	return 0;
}
