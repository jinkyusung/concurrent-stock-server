#include "csapp.h"
#include "stock.h"
#define NTHREADS  4		
#define SBUFSIZE  1024

typedef struct {
    int *buf;          /* Buffer array */         
    int n;             /* Maximum number of slots */
    int front;         /* buf[(front+1)%n] is first item */
    int rear;          /* buf[rear%n] is last item */
    sem_t mutex;       /* Protects accesses to buf */
    sem_t slots;       /* Counts available slots */
    sem_t items;       /* Counts available items */
} sbuf_t;

volatile int readcnt = 0;
static sem_t mutex, w;
sbuf_t sbuf;

void sbuf_init(sbuf_t *sp, int n);
void sbuf_deinit(sbuf_t *sp);
void sbuf_insert(sbuf_t *sp, int item);
int sbuf_remove(sbuf_t *sp);

void echo(int connfd);
static void init_echo(void);
void *thread(void *vargp);


int main(int argc, char **argv) 
{
    int i, listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    pthread_t tid;

	char client_hostname[MAXLINE];
	char client_port[MAXLINE];
	FILE* fp_read;

	fp_read = fopen("stock.txt", "r");
    load_data(fp_read);
	fclose(fp_read);

    if (argc != 2) {
		fprintf(stderr, "usage: %s <port>\n", argv[0]);
		exit(0);
    }
    listenfd = Open_listenfd(argv[1]);
	sbuf_init(&sbuf, SBUFSIZE);
	for (i = 0; i < NTHREADS; i++)
		Pthread_create(&tid, NULL, thread, NULL);

    while (1) {
        clientlen = sizeof(struct sockaddr_storage);
		connfd = Accept(listenfd, (SA *) &clientaddr, &clientlen); //line:conc:echoservert:endmalloc
		Getnameinfo((SA *) &clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0);
		printf("Connected to (%s, %s)\n", client_hostname, client_port);
		sbuf_insert(&sbuf, connfd);
    }

	// fclose(perform);
	sbuf_deinit(&sbuf);
    freenode(root);
}

/* Thread routine */
void *thread(void *vargp) 
{  
    Pthread_detach(pthread_self()); //line:conc:echoservert:detach
	while (1) {
		int connfd = sbuf_remove(&sbuf);
		echo(connfd);
		Close(connfd);
		FILE* fp_write = fopen("stock.txt", "w");
		update_data(root, fp_write);
		fclose(fp_write);
	}
}
/* $end echoservertmain */

/* $begin echo */
void echo(int connfd) 
{
    size_t n; 
    char buf[MAXLINE]; 
    rio_t rio;
	char* argv[3];

	static pthread_once_t once = PTHREAD_ONCE_INIT;
	Pthread_once(&once, init_echo);

	Rio_readinitb(&rio, connfd);
	while((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) { //line:netp:echo:eof
		printf("server received %d bytes\n", (int)n);
		split(argv, buf);
		memset(message, 0, MAXLINE);  // init message
		if (!strcmp(argv[0], "buy") && argv[1] && argv[2]){
			P(&w);
			make_buy_msg(root, atoi(argv[1]), atoi(argv[2]));
			V(&w);
		}
		else if (!strcmp(argv[0], "sell") && argv[1] && argv[2]) {
			P(&w);
			make_sell_msg(root, atoi(argv[1]), atoi(argv[2]));
			V(&w);
		}
		else if (!strcmp(argv[0], "show")) {
			P(&mutex);
			readcnt++;
			if (readcnt == 1)
				P(&w);
			V(&mutex);
			make_show_msg(root);
			P(&mutex);
			readcnt--;
			if (readcnt == 0)
				V(&w);
			V(&mutex);
		}
		else if (!strcmp(argv[0], "exit")) {
			Rio_writen(connfd, "", MAXLINE);
			V(&mutex);
			return;
		}
		init_argv(argv, 3);
		Rio_writen(connfd, message, MAXLINE);
    }
}
/* $end echo */

static void init_echo(void) 
{
	Sem_init(&mutex, 0, 1);
	Sem_init(&w, 0, 1);
}

void init_argv(char** argv, int n) 
{
	for (int i = 0; i < n; i++)
		argv[i] = NULL;
}

void sbuf_init(sbuf_t *sp, int n)
{
    sp->buf = Calloc(n, sizeof(int)); 
    sp->n = n;                       /* Buffer holds max of n items */
    sp->front = sp->rear = 0;        /* Empty buffer iff front == rear */
    Sem_init(&sp->mutex, 0, 1);      /* Binary semaphore for locking */
    Sem_init(&sp->slots, 0, n);      /* Initially, buf has n empty slots */
    Sem_init(&sp->items, 0, 0);      /* Initially, buf has zero data items */
}
/* $end sbuf_init */

/* Clean up buffer sp */
/* $begin sbuf_deinit */
void sbuf_deinit(sbuf_t *sp)
{
    Free(sp->buf);
}
/* $end sbuf_deinit */

/* Insert item onto the rear of shared buffer sp */
/* $begin sbuf_insert */
void sbuf_insert(sbuf_t *sp, int item)
{
    P(&sp->slots);                          /* Wait for available slot */
    P(&sp->mutex);                          /* Lock the buffer */
    sp->buf[(++sp->rear)%(sp->n)] = item;   /* Insert the item */
    V(&sp->mutex);                          /* Unlock the buffer */
    V(&sp->items);                          /* Announce available item */
}
/* $end sbuf_insert */

/* Remove and return the first item from buffer sp */
/* $begin sbuf_remove */
int sbuf_remove(sbuf_t *sp)
{
    int item;
    P(&sp->items);                          /* Wait for available item */
    P(&sp->mutex);                          /* Lock the buffer */
    item = sp->buf[(++sp->front)%(sp->n)];  /* Remove the item */
    V(&sp->mutex);                          /* Unlock the buffer */
    V(&sp->slots);                          /* Announce available slot */
    return item;
}