#include "client_server.h"


sem_t mutex_book[MAX_BOOKS];
sem_t mutex_book_whole;
sem_t mutex_user;
sem_t mutex_user_book;

int authentication(int client_socket) // Returns 1 if admin, 2 if user, 401 if unauthorised
{
    struct User temp;
    recv(client_socket,&temp,sizeof(struct User),0);
    struct flock lock;
    lock.l_type=F_RDLCK;
    lock.l_whence=SEEK_SET;
    lock.l_start=0;
    lock.l_len=0;
    lock.l_pid=getpid();
    int fd=open("users.txt",O_RDONLY|O_CREAT,0666);
    if(fd<0)
    {
        perror("open\n");
        exit(1);
    }
    if(fcntl(fd,F_SETLKW,&lock)<0)
    {
        perror("fcntl\n");
        exit(1);
    }
    printf("Username: %s\n",temp.username);
    printf("Password: %s\n",temp.password);
    struct User user;
    int flag=0;
    int authMode=0;
    while(read(fd,&user,sizeof(struct User))>0)
    {
        if(strcmp(user.username,temp.username)==0 && strcmp(user.password,temp.password)==0)
        {
            flag=1;
            break;
        }
    }
    lock.l_type=F_UNLCK;
    if(fcntl(fd,F_SETLKW,&lock)<0)
    {
        perror("fcntl\n");
        exit(1);
    }
    close(fd);
    if(flag==1)
    {
        if(user.isAdmin==true)
        {
            authMode=ADMIN;
        }
        else
        {
            authMode=USER;
        }
    }
    else
    {
        authMode=UNAUTHORISED;
    }
    send(client_socket,&authMode,sizeof(int),0);
    return authMode;
}

void get_all_books_issued_by_all_users() // Returns all the books issued by all the users
{
    int fd_user_book=open("user_books.txt",O_RDONLY|O_CREAT,0666);
    if(fd_user_book<0)
    {
        perror("open\n");
        exit(1);
    }
    struct User_Book temp;
    sem_wait(&mutex_user_book);
    while(read(fd_user_book,&temp,sizeof(struct User_Book))>0)
    {
        printf("Book id: %d\n",temp.book_Id);
        printf("Username: %s\n",temp.username);
        printf("\n");
    }
    sem_post(&mutex_user_book);
    close(fd_user_book);
    return;
}

void get_all_books_issued_by_client(int client_socket) 
{
    sem_wait(&mutex_user_book);
    int fd=open("user_books.txt",O_RDONLY|O_CREAT,0666);
    int fd1=open("books.txt",O_RDONLY|O_CREAT,0666);
    chk(fd,"open");
    int offset=lseek(fd,0,SEEK_END);
    printf("offset is %d\n",offset);
    if(offset<=0) offset=0;
    int sz=sizeof(struct User_Book);
    int maxid=(offset/sz);
    printf("Total books here: %d\n",maxid);
    write(client_socket,&maxid,sizeof(int));
    lseek(fd,0,SEEK_SET);
    struct User_Book temp;
    printf("maxid is %d\n",maxid);
    if(maxid==0) return;
    int i=0;
    while(read(fd,&temp,sizeof(struct User_Book)) && i<maxid)
    {
        write(client_socket,&temp,sizeof(struct User_Book));
        sem_wait(&mutex_book[temp.book_Id]);
        struct Book book;
        lseek(fd1,(temp.book_Id-1)*sizeof(struct Book),SEEK_SET);
        read(fd1,&book,sizeof(struct Book));
        write(client_socket,&book,sizeof(struct Book));
        sem_post(&mutex_book[temp.book_Id]);
        i++;
    }
    sem_post(&mutex_user_book);
    close(fd);
}


void issue_book(int client_socket) // Issues the book to the user
{
    struct User_Book temp;
    recv(client_socket,&temp,sizeof(struct User_Book),0);
    int fd_books=open("books.txt",O_RDWR|O_CREAT,0666);
    if(fd_books<0)
    {
        perror("open\n");
        exit(1);
    }
    int offset=lseek(fd_books,0,SEEK_END);
    printf("offset is %d\n", offset);
    if(offset<=0)   offset=0;
    int size=sizeof(struct Book);
    int maxId=offset/size;
    if(temp.book_Id>maxId)
    {
        int flag=BAD_REQUEST;
        send(client_socket,&flag,sizeof(int),0);
        close(fd_books);
        return;
    }
    int check_off=-1;
    check_off=lseek(fd_books,(temp.book_Id-1)*sizeof(struct Book),SEEK_SET);
    printf("check_off is %d\n", check_off);
    sem_wait(&mutex_book[temp.book_Id]);

    struct Book book;
    check_off=lseek(fd_books,(temp.book_Id-1)*sizeof(struct Book),SEEK_SET);
    read(fd_books,&book,sizeof(struct Book));
    if(book.copies<=0 || book.valid==0)
    {
        int flag=BAD_REQUEST;
        write(client_socket,&flag,sizeof(int));
        return;
    }
    book.copies--;
    check_off=lseek(fd_books,(temp.book_Id-1)*sizeof(struct Book),SEEK_SET);
    write(fd_books,&book,sizeof(struct Book));
    sem_post(&mutex_book[temp.book_Id]);
    if(sem_wait(&mutex_user_book)<0)
    {
        perror("sem_wait\n");
        exit(1);
    }
    int fd=open("user_books.txt",O_RDWR|O_CREAT,0666);
    if(fd<0)
    {
        perror("open\n");
        exit(1);
    }
    struct User_Book ub;
    while(read(fd,&ub,sizeof(struct User_Book)))
    {
        if(strcmp(ub.username,temp.username)==0 && ub.book_Id==temp.book_Id)
        {
            int flag=DUPLICATE;
            write(client_socket,&flag,sizeof(int));
            if(sem_post(&mutex_user_book)<0)
            {
                perror("sem_post\n");
                exit(1);
            }
            return;
        }
    }
    check_off=lseek(fd,0,SEEK_END);
    printf("check_off is %d\n", check_off);
    write(fd,&temp,sizeof(struct User_Book));
    close(fd);
    close(fd_books);
    int flag=OK;
    write(client_socket,&flag,sizeof(int));
    write(client_socket,&book,sizeof(struct Book));
    if(sem_post(&mutex_user_book)<0)
    {
        perror("sem_post\n");
        exit(1);
    }
    get_all_books_issued_by_client(client_socket);
    return;
}

void get_all_books_in_library(int client_socket) // Returns all the books in the library
{
    if (sem_wait(&mutex_book_whole) < 0) {
        perror("sem_wait");
        return;
    }

    int fd = open("books.txt", O_RDONLY | O_CREAT, 0666);
    if (fd < 0) {
        perror("open");
        sem_post(&mutex_book_whole);
        return;
    }

    int offset = lseek(fd, 0, SEEK_END);
    printf("offset is %d\n", offset);
    if (offset < 0) {
        offset=0;
    }

    int size = sizeof(struct Book);
    int maxid = offset / size;
    printf("Total books: %d\n", maxid);

    if (send(client_socket, &maxid, sizeof(int), 0) < 0) {
        printf("Error sending maxid\n"); 
        perror("send");
        close(fd);
        sem_post(&mutex_book_whole);
        return;
    }

    lseek(fd, 0, SEEK_SET);
    struct Book temp;
    if (maxid == 0) {
        printf("No books in the library\n");
        close(fd);
        sem_post(&mutex_book_whole);
        return;
    }
    int i=0;
    while(read(fd, &temp, sizeof(struct Book)) > 0 && i<maxid) 
    {
        if (send(client_socket, &temp, sizeof(struct Book), 0) < 0) 
        {
            perror("send");
            close(fd);
            sem_post(&mutex_book_whole);
            return;
        }
        i++;
    }

    close(fd);
    if (sem_post(&mutex_book_whole) < 0) {
        perror("sem_post");
    }
}


void return_book(int client_socket) // Returns the book to the library
{
    struct User_Book temp;
    recv(client_socket,&temp,sizeof(struct User_Book),0);
    sem_wait(&mutex_user_book);
    int fd_user_book=open("user_books.txt",O_RDWR|O_CREAT,0666);
    if(fd_user_book<0)
    {
        perror("open\n");
        exit(1);
    }
    struct User_Book ub1;
    struct User_Book ub2;
    bzero(&ub1,sizeof(struct User_Book));
    bzero(&ub2,sizeof(struct User_Book));
    int flag=BAD_REQUEST;
    lseek(fd_user_book,-sizeof(struct User_Book),SEEK_END);
    read(fd_user_book,&ub1,sizeof(struct User_Book));
    lseek(fd_user_book,0,SEEK_SET);
    while(read(fd_user_book,&ub2,sizeof(struct User_Book))>0)
    {
        if(strcmp(ub2.username,temp.username)==0 && ub2.book_Id==temp.book_Id)
        {
            flag=OK;
            lseek(fd_user_book,-sizeof(struct User_Book),SEEK_CUR);
            write(fd_user_book,&ub1,sizeof(struct User_Book));
            break;
        }
    }
    if(flag==BAD_REQUEST)
    {
        send(client_socket,&flag,sizeof(int),0);
        if(sem_post(&mutex_user_book)>0) 
        {
            perror("sem_post\n");
            exit(1);
        }
        return;
    }
    ftruncate(fd_user_book,lseek(fd_user_book,0,SEEK_END)-sizeof(struct User_Book));
    close(fd_user_book);
    sem_post(&mutex_user_book);
    int fd_books=open("books.txt",O_RDWR|O_CREAT,0666);
    if(fd_books<0)
    {
        perror("open\n");
        exit(1);
    }
    struct Book b;
    int id=temp.book_Id;
    sem_wait(&mutex_book[id]);
    lseek(fd_books,(id-1)*sizeof(struct Book),SEEK_SET);
    read(fd_books,&b,sizeof(struct Book));
    b.copies++;
    lseek(fd_books,(id-1)*sizeof(struct Book),SEEK_SET);
    write(fd_books,&b,sizeof(struct Book));
    sem_post(&mutex_book[id]);
    close(fd_books);
    send(client_socket,&flag,sizeof(int),0);
    get_all_books_issued_by_all_users();
    return;
}

int client_portal(int client_socket)
{
    while(1)
    {
        int choice;
        recv(client_socket,&choice,sizeof(int),0);
        printf("Choice: %d\n",choice);
        if(choice==1)
            get_all_books_in_library(client_socket);
        else if(choice==2)
            issue_book(client_socket);
        else if(choice==3)
            return_book(client_socket);
        else if(choice==4)
            get_all_books_issued_by_client(client_socket);
        else
            return 0;
    }
}

void print_book(struct Book *b)
{
    printf("ID: %d\n",b->book_Id);
    printf("Title: %s\n",b->title);
    printf("Author: %s\n",b->author);
    printf("Copies: %d\n",b->copies);
    printf("Valid: %d\n",b->valid);
    printf("\n");
    printf("\n");
}

void add_new_user(int client_socket) 
{
    sem_wait(&mutex_user);
    struct User temp;
    recv(client_socket,&temp,sizeof(struct User),0);
    int fd_users=open("users.txt",O_RDWR|O_CREAT,0666);
    struct User user;
    while(read(fd_users,&user,sizeof(struct User))>0)
    {
        if(strcmp(user.username,temp.username)==0)
        {
            int flag=DUPLICATE;
            send(client_socket,&flag,sizeof(int),0);
            sem_post(&mutex_user);
            return;
        }
    }
    lseek(fd_users,0,SEEK_END);
    temp.isAdmin=0;
    write(fd_users,&temp,sizeof(struct User));
    sem_post(&mutex_user);
    close(fd_users);
    int flag=OK;
    send(client_socket,&flag,sizeof(int),0);
    return;
}

void add_book(int client_socket) {
    struct Book temp;
    bzero(&temp, sizeof(struct Book));
    recv(client_socket, &temp, sizeof(struct Book), 0);
    
    int fd_books = open("books.txt", O_RDWR | O_CREAT, 0666);
    if (fd_books < 0) {
        perror("open books.txt");
        exit(1);
    }
    
    int size = sizeof(struct Book);
    int offset = lseek(fd_books, 0, SEEK_END);
    if (offset < 0) {
        perror("lseek");
        close(fd_books);
        exit(1);
    }
    
    int id = (offset / size) + 1;
    temp.book_Id = id;
    temp.valid = 1;
    
    if (sem_wait(&mutex_book[id]) < 0) {
        perror("sem_wait");
        close(fd_books);
        exit(1);
    }
    
    if (write(fd_books, &temp, sizeof(struct Book)) != sizeof(struct Book)) {
        perror("write");
        sem_post(&mutex_book[id]);
        close(fd_books);
        exit(1);
    }
    
    sem_post(&mutex_book[id]);
    close(fd_books);
    
    send(client_socket, &id, sizeof(int), 0);
}

void delete_book(int client_socket)
{
    int id=-1;
    recv(client_socket,&id,sizeof(int),0);
    int fd_books=open("books.txt",O_RDWR|O_CREAT,0666);
    if(fd_books<0)
    {
        perror("open\n");
        exit(1);
    }
    int offset=lseek(fd_books,0,SEEK_END);
    if(offset<=0)   offset=0;
    int size=sizeof(struct Book);
    int maxid=(offset/size);
    lseek(fd_books,0,SEEK_SET);
    if(id>maxid) 
    {
        int done=BAD_REQUEST;
        send(client_socket,&done,sizeof(int),0);
    }
    if(sem_wait(&mutex_book[id])<0)
    {
        perror("sem_wait\n");
        exit(1);
    }
    struct Book book;
    lseek(fd_books,(id-1)*sizeof(struct Book),SEEK_SET);
    read(fd_books,&book,sizeof(struct Book));
    book.valid=0;
    book.copies=0;
    lseek(fd_books,(id-1)*sizeof(struct Book),SEEK_SET);
    write(fd_books,&book,sizeof(struct Book));
    if(sem_post(&mutex_book[id])<0)
    {
        perror("sem_post\n");
        exit(1);
    }
    get_all_books_in_library(fd_books);
    close(fd_books);
    int done=OK;
    send(client_socket,&done,sizeof(int),0);
    return;
}

void set_copies(int client_socket)  
{
    int id;
    recv(client_socket,&id,sizeof(int),0);
    int fd_books=open("books.txt",O_RDWR|O_CREAT,0666);
    if(fd_books<0)
    {
        perror("open\n");
        exit(1);
    }
    struct Book book;
    int offset=lseek(fd_books,0,SEEK_END);
    if(offset<=0) offset=0;
    int sz=sizeof(struct Book);
    int maxid=(offset/sz);
    if(id>maxid) 
    {
        int flag=BAD_REQUEST;
        send(client_socket,&flag,sizeof(int),0);
        return;
    }
    sem_wait(&mutex_book[id]);
    lseek(fd_books,(id-1)*sizeof(struct Book),SEEK_SET);
    read(fd_books,&book,sizeof(struct Book));
    int copies;
    recv(client_socket,&copies,sizeof(int),0);
    book.copies=copies;
    lseek(fd_books,(id-1)*sizeof(struct Book),SEEK_SET);
    write(fd_books,&book,sizeof(struct Book));
    sem_post(&mutex_book[id]);
    close(fd_books);
    int flag=OK;
    send(client_socket,&flag,sizeof(int),0);
    return;
}

int admin_portal(int client_socket)
{
    while (1)
    {
        int choice;
        recv(client_socket,&choice,sizeof(int),0);
        printf("Choice: %d\n",choice);
        if(choice==1)
            add_new_user(client_socket);
        else if(choice==2)
            add_book(client_socket);
        else if(choice==3)
            delete_book(client_socket);
        else if(choice==4)
            get_all_books_in_library(client_socket);
        else if(choice==5)
            set_copies(client_socket);
        else if(choice==6)
            get_all_books_issued_by_client(client_socket);
        else
            return 0 ;
    }
}

void *connection(void *args)
{
    int client_socket=*(int*)args;
    int authMode=authentication(client_socket);
    if(authMode==UNAUTHORISED)
    {
        close(client_socket);
        return NULL;
    }
    if(authMode==ADMIN)
    {
        admin_portal(client_socket);
    }
    else if(authMode==USER)
    {
        client_portal(client_socket);
    }
    return NULL;

}

void set_base_admin()
{
    struct User admin;
    memset(admin.username,0,sizeof(admin.username));
    memset(admin.password,0,sizeof(admin.password));
    printf("Enter the username of the admin:\n=>");
    scanf("%s",admin.username);
    printf("Enter the password of the admin:\n=>");
    scanf("%s",admin.password);
    struct flock lock;
    admin.isAdmin=1;
    lock.l_type=F_RDLCK;
    lock.l_whence=SEEK_SET;
    lock.l_start=0;
    lock.l_len=0;
    lock.l_pid=getpid();
    int fd=open("users.txt",O_RDWR|O_CREAT,0666);
    if(fcntl(fd,F_SETLKW,&lock)<0)
    {
        perror("fcntl\n");
        exit(1);
    }
    int sz=sizeof(struct User);
    if(fcntl(fd,F_SETLKW,&lock))
    {
        perror("fcntl\n");
        exit(1);
    }
    struct User temp;
    while(read(fd,&temp,sz))
    {
        if(strcmp(temp.username,admin.username)==0)
        {
            printf("Admin already exists\n");
            return;
        }
    }
    lock.l_type=F_UNLCK;
    if(fcntl(fd,F_SETLKW,&lock)<0)
    {
        perror("fcntl\n");
        exit(1);
    }
    lseek(fd,0,SEEK_END);
    lock.l_type=F_WRLCK;
    lock.l_start=lseek(fd,0,SEEK_END);
    lock.l_len=sz;
    if(fcntl(fd,F_SETLKW,&lock)<0)
    {
        perror("fcntl\n");
        exit(1);
    }
    write(fd,&admin,sz);
    lock.l_type=F_UNLCK;
    if(fcntl(fd,F_SETLKW,&lock)<0)
    {
        perror("fcntl\n");
        exit(1);
    }
    close(fd);
}

int main()
{
    set_base_admin();
    for(int i=0;i<MAX_BOOKS;i++)
    {
        sem_init(&mutex_book[i],0,1);
    }
    sem_init(&mutex_book_whole,0,1);
    sem_init(&mutex_user,0,1);
    sem_init(&mutex_user_book,0,1);
    int server_socket,client_socket;
    struct sockaddr_in server,client;
    server_socket=socket(AF_INET,SOCK_STREAM,0);
    if(server_socket<0)
    {
        perror("socket\n");
        exit(1);
    }
    bzero(&server,sizeof(server));
    server.sin_family=AF_INET;
    server.sin_port=htons(PORT);
    server.sin_addr.s_addr=htonl(INADDR_ANY);
    if(bind(server_socket,(struct sockaddr*)&server,sizeof(server))<0)
    {
        perror("bind\n");
        exit(1);
    }
    if(listen(server_socket,MAX_CLIENTS)<0)
    {
        perror("listen\n");
        exit(1);
    }
    printf("Server is listening on port %d\n",PORT);
    while(1)
    {
        int length=sizeof(client);
        client_socket=accept(server_socket,(struct sockaddr *)&client,&length);
        if(client_socket<0)
        {
            perror("accept\n");
            exit(1);
        }
        pthread_t tid;
        pthread_create(&tid,NULL,connection,(void *)&client_socket); 
    }
    close(server_socket);
    close(client_socket);
    for(int i=0;i<MAX_BOOKS;i++)
    {
        sem_destroy(&mutex_book[i]);
    }
    sem_destroy(&mutex_user);
    sem_destroy(&mutex_book_whole);
    sem_destroy(&mutex_user_book);
    return 0;
}