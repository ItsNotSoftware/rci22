#include "server_com.hpp"
#include "fd_handler.hpp"

extern FdHandler *fd_handler_ptr;
/**
 * Create a UDP connection with the node server
 *
 * @param ip server ip
 * @param port server port
 * @return true if connection was succesfully created, false otherwise.
 */
void Server::connect(const char *ip, const char *port)
{
    int errcode;
    fd = socket(AF_INET, SOCK_DGRAM, 0); // UDP socket

    if (fd == -1) // Error oppening socket
    {
        cout << "[SERVER CONNECTION ERROR]: Unable to cretate UDP socket!\n";
        online = false;
    }

    fd_handler_ptr->add(fd);

    memset(&hints, 0, sizeof hints); // Sets memory to 0
    hints.ai_family = AF_INET;       // IPv4
    hints.ai_socktype = SOCK_DGRAM;  // UDP socket

    errcode = getaddrinfo(ip, port, &hints, &res);

    if (errcode) // Error in getaddrinfo
    {
        cout << "[ADDR INFO ERROR]: " << gai_strerror(errcode) << endl;
        online = false;
    }

    online = true;
}

/**
 * Send commands in a UDP connection
 *
 * @param msg mensage that is sent
 * @return true if connection was succesfully created, false otherwise.
 */
bool Server::send_cmd(string msg)
{
    bytes_sent = sendto(fd, msg.c_str(), msg.length(), 0, res->ai_addr, res->ai_addrlen); // bytes that are sent

    if (bytes_sent == -1) // Error sending bytes
    {
        cout << "[ERROR SENDING UDP SERVER DATA]: " << strerror(errno) << endl;
        online = false;
        return false;
    }

    struct timeval timeout;
    timeout.tv_sec = 5; // 5 seconds
    timeout.tv_usec = 0;
    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0) // creates a timeout in case of the server stops responding
    {
        cout << "[SERVER TIMEOUT]: server is not responding" << endl;
        online = false;
        return false;
    }
    return true;
}

/**
 * Recives message from server
 *
 * @return true if a message was recieved, false otherwise.
 */
string Server::receive_msg()
{
    if (!online)
        return "";

    bytes_recived = recvfrom(fd, buffer, MSG_LEN, 0, (struct sockaddr *)&addr, &addrlen); // bytes that are recieved

    if (bytes_recived == -1) // Error in recieving bytes
    {
        cout << "[ERROR RECIEVING SERVER DATA]: " << strerror(errno) << endl;
        SAFE_EXIT(EXIT_FAILURE);
    }

    buffer[bytes_recived] = '\0'; // terminating string

    string msg(buffer);
    return msg;
}

/**
 * Free's addr info and closes socket connetion
 *
 * @return void.
 */
Server::~Server()
{
    freeaddrinfo(res);
    close(fd);
    cout << "Connection with server closed\n";
}
