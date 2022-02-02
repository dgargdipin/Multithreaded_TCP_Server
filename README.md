# Assignment 1 -  Parallel TCP/IP Socket Server with client
- **Dipin Garg**

This is a TCP/IP server which can receive multiple client requests at the same time and entertain each client request in parallel so that no client will have to wait for server time. 

TCP/IP server has been designed with multi-threading for parallel processing(in C++17), with each thread handling a new connection. Whenever a request comes to the server, the server’s main thread will create a thread and pass the client request to that thread with respective descriptors.

## Default behaviour
The `server.cpp` file contains class definition of `TCP_SERVER` and `TCPtimeServer` classes. The constructor of `TCP_SERVER` takes arguments `int port` and `std::function<const char*(void) > callback`, where callback is responsible for `TCP_SERVER` response. `TCPtimeServer` is inherited from `TCP_SERVER`, and contains callback function definition which returns the current time string. Running `server.cpp`, will instantiate the `TCPtimeServer` class, and therefore each response sent by the `TCP_SERVER` will contain the current time.


## Compiling and running
The server can be compiled using: 
```bash
g++ server.cpp -o server -std=c++17 -lpthread
```
The client can be compiled using:
```
g++ client.cpp -o client -std=c++17                                                                         
```

By default the server runs on port `3491`, which can be changed by simply changing the value passed into the constructor of `TCP_SERVER` class in main().
To run the client 2 additional arguments namely hostname and port are required. Example usage includes `./client 127.0.0.1 3491`.





