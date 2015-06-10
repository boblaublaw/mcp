typedef struct elem_t
{
    char *name;
    struct elem_t *next;
} elem_t;

typedef struct queue
{
    pthread_mutex_t mtx;
    pthread_cond_t  cond;
    int             size;   // length of the list 
    elem_t          *head;  // head of the linked list of values
    int             drain;  // set this when you have no more elements to add
} queue_t;

void queue_drain(queue_t *q);
void queue_init(queue_t *q);
void queue_add(queue_t *q, const char *value);
int queue_get(queue_t *q, char **val_r);
