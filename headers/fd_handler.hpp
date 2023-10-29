#ifndef FD_HANDLER_H
#define FD_HANDLER_H

#include "headers.hpp"
#include "node_handling.hpp"

// Manages file_descriptors and select() funtion
class FdHandler
{
public:
    int tcp_server_fd;
    int max_fd;

    vector<int> file_descriptors; // All current active file descriptors

    FdHandler();
    void add(int fd);
    void check_select();
    bool is_set(int fd);
    bool is_new_connection(int fd);
    bool is_user_cmd(int fd);
    void remove(int fd);

private:
    fd_set current_sockets, ready_sockets;
    void restore();
};

Node *fd_to_node(int fd);

#endif