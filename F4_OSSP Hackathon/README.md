
# Readers-Writers Concurrent Database Simulator

## Project Overview

A C-language implementation of the Readers-Writers synchronization
problem with a browser-based frontend and SQLite database.

## Technologies

- C
- POSIX threads
- Mutexes and condition variables
- SQLite3
- HTML, CSS and JavaScript
- Ubuntu and Visual Studio Code

## Compile

```bash
make
```

## Run

```bash
./simulator
```

Open http://127.0.0.1:5000 in a browser.

## Demonstration

1. Start several read operations.
2. Start a write while readers are active.
3. Observe synchronization events.
4. Modify a record and check the database table.
5. Reset the demo after operations finish.

## Synchronization

Multiple readers can access the database concurrently.
Writers require exclusive access and wait for active readers to finish.
The implementation uses POSIX mutexes and condition variables.
