#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>

#define SERVANT_COUNT 4
#define PRESENT_COUNT 500000

struct node
{
	int data;
	struct node *next;
};

// coarse-grained singly linked list
struct list
{
	std::mutex *mtx;
	struct node *head;
	size_t size;
};

struct node* node_create( int data )
{
	struct node *n = new struct node;

	n->data = data;
	n->next = NULL;

	return n;
}

void node_free( struct node *elem )
{
	if ( elem == NULL )
		return;

	delete elem;
}

void list_init( struct list *lst )
{
	// create sentinel node
	lst->head = NULL;
	lst->mtx = new std::mutex();
	lst->size = 0;
}

void list_free( struct list *lst )
{
	if ( lst == NULL )
		return;

	lst->mtx->lock();

	// start free memory of all nodes
	struct node *elem = lst->head;
	while ( elem != NULL )
	{
		struct node *tmp = elem;
		elem = elem->next;
		node_free( tmp );
	}

	lst->mtx->unlock();

	delete lst->mtx;
}

size_t list_size( struct list *lst )
{
	if ( lst == NULL )
		return 0;
	
	size_t res;
	lst->mtx->lock();
	res = lst->size;
	lst->mtx->unlock();
	return res;
}

int list_contains( struct list *lst, int data )
{
	if ( lst == NULL )
		return 0;

	lst->mtx->lock();

	struct node *elem = lst->head;
	while ( elem != NULL )
	{
		if ( elem->data == data )
		{
			lst->mtx->unlock();
			return 1;
		}

		elem = elem->next;
	}

	lst->mtx->unlock();

	return 0;
}

int list_pop( struct list *lst )
{
	if ( lst == NULL )
		return -1;

	lst->mtx->lock();

	struct node *elem = lst->head;

	// list is empty
	if ( elem == NULL )
	{
		lst->mtx->unlock();
		return -1;
	}

	// get result then update head
	int res = elem->data;
	lst->head = elem->next;
	node_free( elem );

	lst->size -= 1;
	lst->mtx->unlock();

	return res;
}

// inserts in ascending order
void list_insert( struct list *lst, int data )
{
	if ( lst == NULL )
		return;

	lst->mtx->lock();

	struct node **elem = &lst->head;
	struct node *new_node = node_create( data );

	// find where to insert
	while ( *elem != NULL && ( *elem )->data < data )
		elem = &( *elem )->next;

	new_node->next = *elem;
	*elem = new_node;

	lst->size += 1;
	lst->mtx->unlock();
}

void shuffle_array( int *arr, size_t n )
{
	for ( int i = 0; i < n - 1; i++ )
	{
		size_t j = i + rand() / ( RAND_MAX / ( n - i ) + 1 );
		int t = arr[ j ];
		arr[ j ] = arr[ i ];
		arr[ i ] = t;
	}
}

void bag_init( int *arr, size_t n )
{
	for ( size_t i = 0; i < n; i++ )
		arr[ i ] = i;

	shuffle_array( arr, n );
}

void servant_logic( struct list *lst, int *bag, std::atomic< int > *bag_pos, std::atomic< int > *card_count, int id )
{
	while ( card_count->load() < PRESENT_COUNT )
	{
		int action = rand() % 3;

		// add a present into the chain
		if ( action == 0 )
		{
			int present = bag_pos->load();

			// no presents in the bag
			if ( present >= PRESENT_COUNT )
				continue;

			// add to chain
			bag_pos->fetch_add( 1 );
			list_insert( lst, bag[ present ] );
		}

		// remove a present from the chain and write a card
		else if ( action == 1 )
		{
			int present = list_pop( lst );

			// no presents in the chain
			if ( present <= -1 )
				continue;

			card_count->fetch_add( 1 );
		}

		// check if a present is in the chain
		else if ( action == 2 )
		{
			int present = rand() % PRESENT_COUNT;
			list_contains( lst, present );
		}
	}
}

void write_cards()
{
	int bag[ PRESENT_COUNT ];
	std::atomic< int > bag_pos = 0;
	std::atomic< int > card_count = 0;
	std::thread servants[ SERVANT_COUNT ];
	struct list lst;

	// init stuff
	list_init( &lst );
	bag_init( bag, PRESENT_COUNT );
	srand( time( NULL ) );

	auto start = std::chrono::high_resolution_clock::now();

	for ( int i = 0; i < SERVANT_COUNT; i++ )
	{
		servants[ i ] = std::thread( servant_logic, &lst, bag, &bag_pos, &card_count, i );
	}

	for ( int i = 0; i < SERVANT_COUNT; i++ )
	{
		servants[ i ].join();
	}

	auto end = std::chrono::high_resolution_clock::now();
	std::chrono::duration< double > dur = end - start;

	// print evaluation
	std::cout << "Number of cards written         : " << card_count.load()				<< std::endl;
	std::cout << "Number of presents in the chain : " << ( int ) list_size( &lst )		<< std::endl;
	std::cout << "Number of presents in the bag   : " << bag_pos.load() - PRESENT_COUNT	<< std::endl;
	std::cout << "Time it took to write all cards : " << dur.count() << " seconds"		<< std::endl;

	list_free( &lst );
}

int main( void )
{
	write_cards();
	return 0;
}
