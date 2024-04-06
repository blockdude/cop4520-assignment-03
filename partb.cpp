#include <iostream>
#include <random>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>

#define SENSOR_COUNT 8
#define MINUTES_TO_RUN 12
#define MINUTES_IN_HOUR 60

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
	static thread_local std::mt19937 generator;
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

		printf( "time: %d, %d | %d | %d\n", time->load(), id, sen->done, atm->count.fetch_add( 0 ) );
		sen->readings[ sen->idx ] = gen_rand( -100, 70 );
		sen->idx = ( sen->idx + 1 ) % MINUTES_IN_HOUR;
		sen->done = 1;

		lk.unlock();

		atm->count.fetch_add( 1 );
		atm->cv.notify_all();
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
	}

	time.fetch_add( 5 );
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
