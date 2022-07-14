using namespace std;

struct element_t{
    struct element_t *next;
    struct element_t *prev;
    void *value;
};

class Queue
{
public:	
	element_t *head;
	element_t *tail;
	pthread_mutex_t* mutex;
	size_t size;
	Queue(pthread_mutex_t* mutexEnv)
	{
		this->mutex = mutexEnv;
		this->head = NULL;
		this->tail = NULL;
		this->size = 0;
	}
	
	void* pop();
	size_t push(void* value_);
	size_t get_size();
};

void* Queue::pop()
{
		element_t *element;
		void *tmp;
 
		pthread_mutex_lock(this->mutex);
		if (this->tail == NULL) 
		{
			pthread_mutex_unlock(this->mutex);
			return NULL;
		}
		element = this->tail;
		
		this->tail = this->tail->prev;
		if (this->tail != NULL) 
		{
			this->tail->next = NULL;
		}
		if (element == this->head) 
		{
			this->head = NULL;
		}
		tmp = element->value;
		free(element);

		this->size--;
		/*        // ------------address test-------------
		char tempstr[50];
		snprintf(tempstr, sizeof(tempstr), "\%p" ,tmp);
    cout << tempstr << " <--pop\n";
    */        // ------------address test-------------
		pthread_mutex_unlock(this->mutex);
		return tmp;
}
size_t Queue::push(void* value_)
{
		size_t currentN;
		element_t *tmp;
		pthread_mutex_lock(this->mutex);
		tmp = (element_t*) malloc(sizeof(element_t));
		tmp->value = value_;
		tmp->next = this->head;
		tmp->prev = NULL;
		if (this->head != NULL) 
		{
			this->head->prev = tmp;
		}
		this->head = tmp;
		/*        // ------------address test-------------
		char tempstr2[50];
		snprintf(tempstr2, sizeof(tempstr2), "\%p" , tmp->value);
    cout << tempstr2 << " <--push\n";
		*/        // ------------address test-------------
		if (this->tail == NULL) 
		{
			this->tail = tmp;
		}
		currentN = this->size++;
		pthread_mutex_unlock(this->mutex);
		return currentN;
}
size_t Queue::get_size()
{
	size_t size;
    pthread_mutex_lock(this->mutex);
    size = this->size;
    pthread_mutex_unlock(this->mutex);
    return size;
}
 




