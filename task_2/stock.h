#include "csapp.h"
#include <string.h>
#include <stdlib.h>


typedef struct Node{
	int id;
	int left_stock;
	int price;

    struct Node* left;
    struct Node* right;
} Node_t;


static Node_t* root = NULL;

int split(char** argv, char* str);
void init_argv(char** argv, int n);
void load_data(FILE* fp);
void update_data(Node_t* root, FILE* fp);


Node_t* BST_insert(Node_t* root, int id, int left_stock, int price);
Node_t* BST_search(Node_t* root, int target_id);
void freenode(Node_t *root);

/* Message for Rio_writen for data transfering to Clients */
char message[MAXLINE];
void make_show_msg(Node_t* root);
void make_buy_msg(Node_t* root, int target_id, int order);
void make_sell_msg(Node_t* root, int target_id, int order);


/* Split buffer to argv[] */
int split(char* argv[], char* str)
{
	char *ptr = strtok(str, "\n");	
	int argc = 0;
	ptr = strtok(str, " ");
	while (ptr != NULL) {
		argv[argc++] = ptr;
		ptr = strtok(NULL, " ");
	}
	return argc;
}


void load_data(FILE* fp)
{
    char line[MAXLINE];
    char* param[3];
    while(Fgets(line, (int)MAXLINE, fp)) {
        split(param, line);
        root = BST_insert(root, atoi(param[0]), atoi(param[1]), atoi(param[2]));
    }
}


void update_data(Node_t* root, FILE* fp)
{
	if(root == NULL)
        return;
    update_data(root->left, fp);
    fprintf(fp, "%d %d %d\n", root->id, root->left_stock, root->price);
    fflush(fp);
    update_data(root->right, fp);
}


void make_show_msg(Node_t* root)
{
    char tmp[MAXLINE];
	if(root == NULL)
        return;
    make_show_msg(root->left);
    sprintf(tmp, "%d %d %d\n", root->id, root->left_stock, root->price);
	strcat(message, tmp);
    make_show_msg(root->right);
}


void make_buy_msg(Node_t* root, int target_id, int order)
{
    Node_t* target = BST_search(root, target_id);
    if (target == NULL || target->left_stock < order)
        strcpy(message, "Not enough left stocks\n");
    else {
        target->left_stock -= order;
        strcpy(message, "[buy] success\n");
    }
}


void make_sell_msg(Node_t* root, int target_id, int order)
{
    Node_t* target = BST_search(root, target_id);
    if (target == NULL)
        strcpy(message, "No exists stock\n");
    else{
        target->left_stock += order;
        strcpy(message, "[sell] success\n");
    }
}


Node_t* BST_insert(Node_t* root, int id, int left_stock, int price)
{
    if (root == NULL) {
        root = (Node_t*)malloc(sizeof(Node_t));
        root->left = root->right = NULL;
        root->id = id;
        root->left_stock = left_stock;
	    root->price = price;
        return root;
    } 
    else {
        if (root->id > id)
            root->left = BST_insert(root->left, id, left_stock, price);
        else
            root->right = BST_insert(root->right, id, left_stock, price);
    }
    return root;
}


Node_t* BST_search(Node_t* root, int target_id)
{
    if (root == NULL)
        return NULL;
    if (root->id == target_id)
        return root;
    else if (root->id > target_id)
        return BST_search(root->left, target_id);
    else
        return BST_search(root->right, target_id);
}


void freenode(Node_t *root) 
{
    if (root == NULL) return;
    freenode(root->left);
    freenode(root->right);
    free(root);
}
