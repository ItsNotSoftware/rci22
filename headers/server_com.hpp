#ifndef _SERVER_COM_
#define _SERVER_COM_

#include "headers.hpp"

#define MSG_LEN 4096

// UDP Server class
class Server
{
public:
    bool online;
    int fd;
    ssize_t bytes_recived, bytes_sent;

    void connect(const char *ip, const char *port);
    bool send_cmd(string msg);
    string receive_msg();
    ~Server();

private:
    socklen_t addrlen;
    struct addrinfo hints, *res;
    struct sockaddr_in addr;
    char buffer[MSG_LEN];
};

#endif
