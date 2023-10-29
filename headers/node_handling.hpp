#ifndef _NODE_COM_
#define _NODE_COM_

#include "headers.hpp"

#define MSG_LEN 4096
#define N_NODES 100

// Class for handling incoming TCP connections
class TCP_Server
{
public:
    int fd;
    ssize_t n, nw;
    TCP_Server(const char *ip, const char *port);
    ~TCP_Server();

private:
    struct sockaddr_in address;
};

// Class for managing TCP connections
class TCP_Connection
{
public:
    int fd;
    bool create(const char *ip, const char *port);
    void accept_new(int server_fd);
    void send_msg(const char *msg);
    string receive_msg();
    ~TCP_Connection();

private:
    bool active; // true if the tcp connection is active
    sockaddr_in addr;
    socklen_t addrlen;
    struct addrinfo *res;
    char buffer[MSG_LEN];
};

// Struct that stores all needed information of and internal external or backup node
struct Node
{
    string id;
    string ip;
    string port;
    TCP_Connection *tcp_connection;
};

// Master node correspondes to the node launched by this program
class MasterNode
{
public:
    bool active;
    string id;
    string net;
    string ip;
    string port;

    Node external_node;
    Node backup_node;

    vector<Node *> internal_nodes; // vector with all internal nodes
    vector<string> names;          // Content names on this node
    int expedtion_table[N_NODES];

    MasterNode();
    void send_to_neighbors(const char *msg);
};

namespace ExpeditionTable
{
    void update_neighbors();
    void remove_node(int id);
    Node *get_node_to_msg(string dest_id);
}

// Functions to handle a node leaving the net rebuilding the net
namespace LeaveHandler
{
    bool internal_handler(Node *node_that_left);
    void external_handler_1();
    void external_handler_2();
    void external_handler_3();
    void rebuild_net(Node *node_that_left);
}

// Funtions to hadle recived messages from other nodes
namespace MsgHadler
{
    void process_msg(string msg, Node *node);
    void process_extern_msg(vector<string> args, string msg);
    // void process_withdraw_msg(vector<string> args, string msg, Node *node);
    // void process_query_msg(vector<string> args, string msg, Node *node);
}

Node *get_node_by_id(int node_id);

#endif
