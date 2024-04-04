#include <mutex>

struct node
{
	int data;
	struct node *next;
	std::mutex *mtx;
};

// singly linked list
struct list
{
	struct node *head;
	size_t size;
};

struct node* node_create( int data )
{
	struct node *n = new struct node;

	n->mtx	= new std::mutex();
	n->data = data;
	n->next = NULL;

	return n;
}

void node_free( struct node *elem )
{
	if ( elem == NULL )
		return;

	delete elem->mtx;
	delete elem;
}

void list_init( struct list *lst )
{
	// create sentinel node
	lst->head = node_create( 0 );
}

void list_free( struct list *lst )
{
	if ( lst == NULL )
		return;

	struct node *elem;

	// lock all nodes
	elem = lst->head;
	while ( elem != NULL )
	{
		elem->mtx->lock();
		elem = elem->next;
	}

	// start free memory of all nodes
	elem = lst->head;
	while ( elem != NULL )
	{
		struct node *tmp = elem;
		elem = elem->next;

		tmp->mtx->unlock();
		node_free( tmp );
	}

	delete lst;
}

int  list_empty( struct list *lst );
int  list_contains( struct list *lst, int data );
int  list_pop( struct list *lst );
void list_insert( struct list *lst, int data );

int main( void )
{
	return 0;
}
