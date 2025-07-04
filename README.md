## 1. Project Overview

A multithreaded TCP-based library management solution with role-based access control.

* **Server**: Manages user authentication, book inventory, issue/return workflows.
* **Client**: Provides CLI portals for Admin and User operations.

## 2. Architecture & Design

* **Transport**: IPv4 TCP sockets on port 6004.
* **Concurrency**: POSIX threads (`pthread_create`) per client session.
* **Synchronization**:

  * Per-book semaphores (`sem_t mutex_book[MAX_BOOKS]`) for copy updates.
  * Global semaphores for user and user-book tables.
* **Persistence**:

  * Flat files (`users.txt`, `books.txt`, `user_books.txt`).
  * Advisory file locks (`fcntl`–based) for atomic authentication.

## 3. Prerequisites

* GCC toolchain (>= gcc 9.0)
* POSIX threads & real-time library (`-pthread`, `-lrt`)
* Linux environment (glibc, syscall compliance)

## 4. Directory Layout

```
├── client.c                 # Client CLI implementation
├── server.c                 # Server core logic
├── client_server.h          # Shared definitions & prototypes
├── users.txt                # User database (auto-created)
├── books.txt                # Book inventory (auto-created)
└── user_books.txt           # Issue records (auto-created)
```

## 5. Build & Deployment

### Native (Local Machine)

```bash
make all                # Generates ./server & ./client
```

* **Cleanup**:

```bash
make clean             # Removes ./server & ./client
```

### Docker (Containerized)

#### Step 1: Build the Docker image

```bash
docker build -t lib-app .
```

#### Step 2: Run the Server

```bash
docker run -it --rm -p 6004:6004 --name lib-server lib-app
```

#### Step 3: Run the Client (separate terminal)

```bash
docker run -it --rm --network host lib-app ./client
```

> On non-Linux systems, replace `--network host` with appropriate Docker network settings or use Docker Compose.

#### Optional: Docker Compose (lib-server + lib-client)

Create `docker-compose.yml`:

```yaml
version: '3.8'
services:
  server:
    build: .
    container_name: lib-server
    ports:
      - "6004:6004"
    tty: true
    stdin_open: true

  client:
    build: .
    container_name: lib-client
    depends_on:
      - server
    entrypoint: ["./client"]
    stdin_open: true
    tty: true
```

Then execute:

```bash
docker-compose up --build
```

## 6. Runtime Workflow

1. **Server**

   ```bash
   ./server
   ```

   * Prompts initial admin setup (username/password).
   * Listens on TCP port 6004; spawns a thread per connection.

2. **Client**

   ```bash
   ./client
   ```

   * CLI prompts for credentials; negotiates Admin or User portal.
   * Menu-driven operations via integer choices.

## 7. Functional Features

### 7.1 Admin Portal

* **Add New User** (ensures no duplicates)
* **Add/Delete Book** (assigns auto-increment IDs; soft-delete via `valid` flag)
* **Set Book Copies** (overwrite copy count)
* **List Inventory** (streams all valid books)
* **View All Issued Books**

### 7.2 User Portal

* **Browse Inventory**
* **Issue Book** (deduplication + copy decrement)
* **Return Book** (atomic swap-and-truncate in issue log)
* **View My Issued Books**

## 8. Protocol Specification

* **Message Framing**: Raw `send()/recv()` of structs and integers; no delimiters.
* **Error Codes** (int flags):

  * `OK (200)`, `BAD_REQUEST (400)`, `UNAUTHORISED (401)`, `NOT_ALLOWED (403)`, `DUPLICATE (402)`

## 9. Data Schema

```c
struct User {
    char username[100];
    char password[100];
    bool isAdmin;
};
struct Book {
    int book_Id;
    int copies;
    char title[100];
    char author[100];
    bool valid;
};
struct User_Book {
    int book_Id;
    char username[100];
};
```

## 10. Error Handling & Logging

* Per-call **`perror()`** on system failures.
* **Return codes** propagated to client for user-level messaging.

## 11. Future Enhancements

* Migrate to **SQLite** or **NoSQL** for scalable persistence.
* Implement **TLS** for encrypted transport.
* Introduce **JSON-based** framing for extensibility.
* Add **unit/integration tests** and **CI/CD** pipeline.

---
