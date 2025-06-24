#ifndef CLIENT_SERVER_H
#define CLIENT_SERVER_H
#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include<fcntl.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<sys/mman.h>
#include<sys/wait.h>
#include<sys/time.h>
#include<sys/resource.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<netdb.h>
#include<errno.h>
#include<string.h>
#include<signal.h>
#include<time.h>
#include<sys/select.h>
#include<strings.h>
#include<pthread.h>
#include<semaphore.h>
#include<sys/sem.h>
#include<stdbool.h>
#include</usr/include/asm-generic/socket.h>

#define MAX_CLIENTS 100
#define PORT 6004
#define chk(x,msg) if(x<0){perror(msg);exit(1);}
#define ADMIN 1
#define USER 2
#define UNAUTHORISED 401
#define DUPLICATE 402
#define BAD_REQUEST 400
#define OK 200
#define NOT_ALLOWED 403
#define MAX_BOOKS 10000 

struct User
{
    char username[100];
    char password[100];
    bool isAdmin;
};

struct Book
{
    int book_Id;
    int copies;
    char title[100];
    char author[100];
    bool valid;
};

struct User_Book
{
    int book_Id;
    char username[100];
};

int authentication(int nsd);
void *connection(void *args);

int admin_portal(int nsd);
void add_new_user(int nsd);
void add_book(int nsd);
void get_all_books(int fd);
void delete_book(int nsd);
void add_more_copies(int nsd);

int client_portal(int nsd);
void issue_book(int nsd);
void get_all_issue_entry();
void get_all_issue_entry_nsd(int nsd);
void return_book(int nsd);

#define MAX_BOOKS 10000 
#define max(a,b) a>b?a:b

int admin_portal_client(int sd);
int user_portal_client(int sd, struct User *user);

#endif