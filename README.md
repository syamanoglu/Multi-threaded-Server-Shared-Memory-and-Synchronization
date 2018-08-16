# Multi-threaded-Server-Shared-Memory-and-Synchronization
Multi-threaded server that will process keyword search queries from clients

* This project is done for CS342 - Operating Systems course at Bilkent University

**Objectives:** Practice developing multiple-threaded applications, practice inter-process
communication (IPC) via shared memory, practice solving synchronization problems,
and practice use of semaphores.

**Assignment Prompt:**

In this project you will implement a multi-threaded server that will process keyword
search queries from clients. You will implement both the server program and the
client program. Multiple clients can request service from the server concurrently.
The service given will be a keyword search service. Upon receiving a keyword from a
client, the server will search the keyword in an input text file and will send back the
line numbers of the lines having the keyword at least once (i.e., line numbers of
matching lines). 

Inter-process communication between clients and server will be
achieved via shared memory. For each client, the server will create a thread to
handle the request of the client. The client program will just send one request and
will wait for the results to arrive. Then it will write the results to the screen and will
terminate. Several client processes can be started concurrently to run in the
background using the & sign, or by use of a different Linux terminal window for each
different client.


The server program will be called “server” and will take the following parameters.

    server <shm_name> <input_filename> <sem_name>

* <shm_name> is the name of the shared memory segment that will be created by the
server and that will be used by the clients and server to share data and communicate
with each other. The name of the shared segment is just a filename (for example
/mysharedsegment).
* <sem_name> is a prefix for naming your semaphores.
* <input_filename> is the name of an input text file (ascii) which
will be searched upon receiving a request (query) from a client.
