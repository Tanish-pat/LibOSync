#include "client_server.h"

void print_book(struct Book *b)
{
    printf("ID: %d\n",b->book_Id);
    printf("Title: %s\n",b->title);
    printf("Author: %s\n",b->author);
    printf("Copies: %d\n",b->copies);
    printf("Valid: %d\n",b->valid);
    printf("\n");
    printf("\n");
    printf("\n");
}

int admin_portal(int server_socket)
{
    printf("Choose options from below:\n");
    printf("1. Add new user\n");
    printf("2. Add new book\n");
    printf("3. Delete a book\n");
    printf("4. Get list of all books in the library\n");
    printf("5. Set copies of a book\n");
    printf("6. Get list of all users with books issued\n");
    printf("7. Exit\n");
    printf("=>\n");
    int choice;
    scanf("%d",&choice);
    send(server_socket,&choice,sizeof(int),0);

    if(choice==1) // Add new user
    {
        struct User newUser;
        memset(newUser.username,0,sizeof(newUser.username));
        memset(newUser.password,0,sizeof(newUser.password));
        printf("Enter the username of the new user:\n=>");
        scanf("%s",newUser.username);
        printf("Enter the password of the new user:\n=>");
        scanf("%s",newUser.password);
        send(server_socket,&newUser,sizeof(struct User),0);
        int flag;
        recv(server_socket,&flag,sizeof(int),0);
        if(flag==DUPLICATE)
        {
            printf("User already exists\n");
        }
        else if(flag==OK)
        {
            printf("User added successfully\n");
        }
        return 1;
    }
    else if (choice==2) // Add new book
    {
        struct Book b;
        int copies;
        char title[100];
        char author[100];

        bzero(&b, sizeof(struct Book));
        bzero(title, sizeof(title));
        bzero(author, sizeof(author));

        printf("Enter the title of the book:\n=> ");
        scanf("%99s", title);
        printf("Enter the author of the book:\n=> ");
        scanf("%99s", author);
        printf("Enter the number of copies of the book:\n=> ");
        scanf("%d", &copies);

        b.copies = copies;
        b.valid = 1;
        strncpy(b.title, title, sizeof(b.title) - 1);
        strncpy(b.author, author, sizeof(b.author) - 1);

        if (send(server_socket, &b, sizeof(struct Book), 0) <= 0) {
            perror("send");
            return 1;
        }

        int id;
        if (recv(server_socket, &id, sizeof(int), 0) <= 0) {
            perror("recv");
            return 1;
        }

        printf("Book added with book id: %d\n", id);
        return 1;
    }
    else if(choice==3) // Delete a book
    {
        printf("Enter the book id to delete:\n=>");
        int id;
        scanf("%d",&id);
        send(server_socket,&id,sizeof(int),0);
        int flag;
        recv(server_socket,&flag,sizeof(int),0);
        if(flag==OK)
        {
            printf("Book deleted successfully\n");
        }
        else
        {
            printf("Book not found\n");
        }
        return 1;
    }
    else if (choice==4) // Get list of all books in the library
    {
        int max_books;
        if (recv(server_socket, &max_books, sizeof(int), 0) <= 0) {
            perror("recv");
            return 0;
        }

        printf("Number of books: %d\n", max_books);
        for (int i = 0; i < max_books; i++) {
            struct Book b;
            if (recv(server_socket, &b, sizeof(struct Book), 0) <= 0) {
                perror("recv");
                return 0;
            }
            if (b.valid == 1) {
                print_book(&b);
            }
        }
        return 1;
    }
    else if(choice==5) // Add more copies of a book
    {
        int id;
        int copies;
        printf("Enter the book id to set copies:\n=>");
        scanf("%d",&id);
        printf("Enter the number of copies to set:\n=>");
        scanf("%d",&copies);
        if(copies<0)
        {
            printf("Invalid number of copies\n");
            return 1;
        }
        send(server_socket,&id,sizeof(int),0);
        send(server_socket,&copies,sizeof(int),0);
        int flag;
        recv(server_socket,&flag,sizeof(int),0);
        if(flag==OK)
        {
            printf("Copies set successfully\n");
        }
        else
        {
            printf("Book not found\n");
        }
        return 1;
    }
    else if(choice==6) // Get list of all users with books issued
    {
        int max_books;
        recv(server_socket,&max_books,sizeof(int),0);
        for(int i=0;i<max_books;i++)
        {
            struct User_Book ub;
            recv(server_socket,&ub,sizeof(struct User_Book),0);
            struct Book b;
            recv(server_socket,&b,sizeof(struct Book),0);
            printf("Username: %s\n",ub.username);
            printf("Book id: %d\n",ub.book_Id);
            print_book(&b);
        }
        return 1;
    }
    else if(choice==7)// Exit
    {
        printf("Exiting\n");
        return 0;
    }
}

int user_portal(int server_socket, struct User *user)
{
    printf("Choose options from below:\n");
    printf("1. Get list of all books in the library\n");
    printf("2. Issue a book under your id\n");
    printf("3. Return a book\n");
    printf("4. View all books issued to you\n");
    printf("5. Exit\n");
    int choice;
    scanf("%d",&choice);
    send(server_socket,&choice,sizeof(int),0);
    if(choice==1)
    {
        int max_books;
        if (recv(server_socket, &max_books, sizeof(int), 0) <= 0) 
        {
            perror("recv");
            return 0;
        }

        printf("Number of books: %d\n", max_books);
        for (int i = 0; i < max_books; i++) {
            struct Book b;
            if (recv(server_socket, &b, sizeof(struct Book), 0) <= 0) {
                perror("recv");
                return 0;
            }
            if (b.valid == 1) {
                print_book(&b);
            }
        }
        return 1;
    }
    else if(choice==2)
    {
        int id;
        printf("Enter the book id to issue:\n=>");
        struct User_Book ub;
        scanf("%d",&id);
        ub.book_Id=id;
        // memz(ub.username);
        memset(ub.username,0,sizeof(ub.username));
        strcpy(ub.username,user->username);
        write(server_socket,&ub,sizeof(struct User_Book));
        int flag;
        read(server_socket,&flag,sizeof(int));
        if(flag==OK)
        {
            printf("Book issued successfully\n");
        }
        else if(flag==DUPLICATE)
        {
            printf("Book already issued\n");
        }
        else
        {
            printf("Book not found or copies not available\n");
        }
        return 1;
    }
    else if(choice==3)
    {
        int id;
        printf("Enter the book id to return:\n=>");
        scanf("%d",&id);
        struct User_Book ub;
        ub.book_Id=id;
        // memz(ub.username);
        memset(ub.username,0,sizeof(ub.username));
        strcpy(ub.username,user->username);
        write(server_socket,&ub,sizeof(struct User_Book));
        int flag;
        read(server_socket,&flag,sizeof(int));
        if(flag==OK)
        {
            printf("Book returned successfully\n");
        }
        else
        {
            printf("Bad Request\n");
        }
        return 1;
    }
    else if(choice==4)
    {
        printf("You have chosen to view all books issued to you\n");
        printf("Books issued to you:\n");
        int max_books;
        read(server_socket,&max_books,sizeof(int));
        printf("max_books: %d\n",max_books);
        for(int i=0;i<max_books;i++)
        {
            struct User_Book ub;
            read(server_socket,&ub,sizeof(struct User_Book));
            struct Book b;
            read(server_socket,&b,sizeof(struct Book));
            if(strcmp(ub.username,user->username)==0) 
            {
                printf("Book id: %d\n",ub.book_Id);
                print_book(&b);
            }
        }
        return 1;
    }
    else
    {
        return 0;
    }

    
}

int main()
{
    struct sockaddr_in serv;
    int server_socket=socket(AF_INET,SOCK_STREAM,0);
    if(server_socket<0)
    {
        perror("socket");
        exit(1);
    }
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) 
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    bzero(&serv,sizeof(serv));
    serv.sin_family=AF_INET;
    serv.sin_port=htons(PORT);
    serv.sin_addr.s_addr=inet_addr("127.0.0.1");
    if(connect(server_socket,(struct sockaddr*)&serv,sizeof(serv))<0)
    {
        perror("connect");
        exit(1);
    }
    struct User login;
    memset(login.username,0,sizeof(login.username));
    memset(login.password,0,sizeof(login.password));
    printf("Enter the username:\n=>");
    scanf("%s",login.username);
    printf("Enter the password:\n=>");
    scanf("%s",login.password);
    send(server_socket,&login,sizeof(struct User),0);
    int authMode;
    recv(server_socket,&authMode,sizeof(int),0);
    if(authMode==ADMIN)
    {
        printf("Admin\n");
        int useChoice=1;
        while(useChoice)
        {
            useChoice=admin_portal(server_socket);
        }
    }
    else if(authMode==USER)
    {
        printf("User\n");
        int useChoice=1;
        while(useChoice)
        {
            useChoice=user_portal(server_socket,&login);
        }
    }
    else
    {
        printf("Unauthorised\n");
    }
    close(server_socket);
}
