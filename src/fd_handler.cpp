#include "fd_handler.hpp"

extern MasterNode *master_node_ptr;

/**
 * Creates FdHAndler class
 *
 * @return void.
 */
FdHandler::FdHandler()
{
    max_fd = 0;
    FD_ZERO(&current_sockets);
}

/**
 * Restores fd's for select
 *
 * @return void.
 */
void FdHandler::restore()
{
    ready_sockets = current_sockets;
}

/**
 * Adds a new fd to set and to fd vector
 *
 * @param fd new fd
 * @return void.
 */
void FdHandler::add(int fd)
{
    // add fd
    file_descriptors.push_back(fd);
    FD_SET(fd, &current_sockets);

    max_fd = fd > max_fd ? fd : max_fd; // update max_fd
}

/**
 * Checks if the fd is ready for reading
 *
 * @param fd fd to check
 * @return true if is set false otherwise.
 */
bool FdHandler::is_set(int fd)
{
    return FD_ISSET(fd, &ready_sockets);
}

/**
 * Checks select function
 *
 * @return void.
 */
void FdHandler::check_select()
{
    restore();
    if (select(max_fd + 1, &ready_sockets, NULL, NULL, NULL) < 0)
    {
        cout << "[SELECT ERROR]: " << strerror(errno) << "\n";
        cmd_leave();
        exit(EXIT_FAILURE);
    }
}

/**
 * Checks if there is a new conection
 *
 * @param fd fd to check
 * @return true if new connection false otherwise
 */
bool FdHandler::is_new_connection(int fd)
{
    return fd == tcp_server_fd;
}

/**
 * Checks if tthere is a user command to be processed
 *
 * @param fd fd to check
 * @return true if there is stdin to read false otherwise.
 */
bool FdHandler::is_user_cmd(int fd)
{
    return fd == STDIN_FILENO;
}

/**
 * Checks if tthere is a user command to be processed
 *
 * @param fd fd to check
 * @return true if there is stdin to read false otherwise.
 */
void FdHandler::remove(int fd)
{
    file_descriptors.erase(std::remove(file_descriptors.begin(), file_descriptors.end(), fd), file_descriptors.end());
    close(fd);
    FD_CLR(fd, &current_sockets);

    max_fd = 0;
    for (uint i = 0; i < file_descriptors.size(); i++)
    {
        max_fd = file_descriptors[i] > max_fd ? file_descriptors[i] : max_fd;
    }
}

/**
 * Gets a pointer to the node correspoding to the given fd
 *
 * @param fd fd to check
 * @return ptr to node or nullptr if it doenst exist
 */
Node *fd_to_node(int fd)
{
    if (fd == master_node_ptr->external_node.tcp_connection->fd)
    {
        return &master_node_ptr->external_node;
    }
    // Search all intenal nodes
    for (uint i = 0; i < master_node_ptr->internal_nodes.size(); i++)
    {
        if (fd == master_node_ptr->internal_nodes[i]->tcp_connection->fd)
        {
            return master_node_ptr->internal_nodes[i];
        }
    }

    return nullptr;
}
