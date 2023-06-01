#include "csapp.h"
#include "stock.h"


typedef struct { /* Represents a pool of connected descriptors */
	int maxfd;
	fd_set read_set;
	fd_set ready_set;
	int nready;
	int maxi;
	int clientfd[FD_SETSIZE]; /* Set of active descriptors */
	rio_t clientrio[FD_SETSIZE];
} pool;


void init_pool(int listenfd, pool *p);
void add_client(int connfd, pool *p);
void check_clients(pool *p);


int main(int argc, char **argv) 
{
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    static pool pool;
	char client_hostname[MAXLINE];
	char client_port[MAXLINE];
    
	fp = fopen("stock.txt", "r+");
    load_data();
    
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }
    listenfd = Open_listenfd(argv[1]);
    init_pool(listenfd, &pool);

    while (1) {
        /* Wait for listening/connected descriptor(s) to become ready */
        pool.ready_set = pool.read_set;
        pool.nready = Select(pool.maxfd+1, &pool.ready_set, NULL, NULL, NULL);
        /* If listening descriptor ready, add new client to pool */
        if (FD_ISSET(listenfd, &pool.ready_set)) {
            clientlen = sizeof(struct sockaddr_storage);
            connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
            Getnameinfo((SA *) &clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0);
			printf("Connected to (%s, %s)\n", client_hostname, client_port);
			add_client(connfd, &pool);
        }
        /* Echo a text line from each ready connected descriptor */
        check_clients(&pool);
    }

    fclose(fp);
    freenode(root);
}


void init_pool(int listenfd, pool *p) 
{   
	/* Initially, there are no connected descriptors */
	int i;
	p->maxi = -1;
	for (i=0; i< FD_SETSIZE; i++)
		p->clientfd[i] = -1;

	/* Initially, listenfd is only member of select read set */
	p->maxfd = listenfd;
	FD_ZERO(&p->read_set);
	FD_SET(listenfd, &p->read_set);
}


void add_client(int connfd, pool *p)
{
	int i;
	p->nready--;
	for (i = 0; i < FD_SETSIZE; i++) /* Find an available slot */
		if (p->clientfd[i] < 0) {
			/* Add connected descriptor to the pool */
			p->clientfd[i] = connfd;
			Rio_readinitb(&p->clientrio[i], connfd);
			/* Add the descriptor to descriptor set */
			FD_SET(connfd, &p->read_set);
			/* Update max descriptor and pool high water mark */
			if (connfd > p->maxfd)
				p->maxfd = connfd;
			if (i > p->maxi)
				p->maxi = i;
			break;
		}
	if (i == FD_SETSIZE) /* Couldn’t find an empty slot */
		app_error("add_client error: Too many clients");
}


void check_clients(pool *p) 
{
	int i, connfd, n;
	char buf[MAXLINE];
	rio_t rio;

	char* argv[3];

	for (i = 0; (i <= p->maxi) && (p->nready > 0); i++) {
		connfd = p->clientfd[i];
		rio = p->clientrio[i];

		/* If the descriptor is ready, echo a text line from it */
		if ((connfd > 0) && (FD_ISSET(connfd, &p->ready_set))) {
			p->nready--;
			if ((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) {
				printf("Server received %d bytes on fd %d\n", n, connfd);
				split(argv, buf);

				if (!strcmp(argv[0], "buy")) {
					memset(message, 0, MAXLINE);  // init message
					make_buy_msg(root, atoi(argv[1]), atoi(argv[2]));
					Rio_writen(connfd, message, MAXLINE);
				}
				else if (!strcmp(argv[0], "sell")) {
					memset(message, 0, MAXLINE);  // init message
					make_sell_msg(root, atoi(argv[1]), atoi(argv[2]));
					Rio_writen(connfd, message, MAXLINE);
				}
				else if (!strcmp(argv[0], "show")) {
					memset(message, 0, MAXLINE);  // init message
					make_show_msg(root);
					Rio_writen(connfd, message, MAXLINE);
				}
				else if (!strcmp(argv[0], "exit")) {
					Rio_writen(connfd, "", MAXLINE);
					rewind(fp);
					update_data(root);
					Close(connfd);
					FD_CLR(connfd, &p->read_set);
					p->clientfd[i] = -1;
				}
				else {
					Rio_writen(connfd, "", MAXLINE);
				}
			}
			/* EOF detected, remove descriptor from pool */
			else {
				rewind(fp);
				update_data(root);
				Close(connfd);
				FD_CLR(connfd, &p->read_set);
				p->clientfd[i] = -1;
			}
		}
	}
}
