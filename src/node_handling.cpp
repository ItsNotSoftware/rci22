#include "node_handling.hpp"
#include "fd_handler.hpp"
#include "commands.hpp"

#define MAX_NUMBER_OF_NODES 100

extern FdHandler *fd_handler_ptr;
extern MasterNode *master_node_ptr;

/**
 * Initializes the expedition table
 *
 * @return void
 */
MasterNode::MasterNode()
{
    active = false;
    external_node.tcp_connection = nullptr;
    backup_node.tcp_connection = nullptr;

    for (int i = 0; i < N_NODES; expedtion_table[i++] = -1) // initializes the table with -1
        ;
}

/**
 * Send mensages to node's neighbours
 *
 * @param msg mensage to be sent
 * @return void
 */
void MasterNode::send_to_neighbors(const char *msg)
{
    if (external_node.tcp_connection != nullptr)
        external_node.tcp_connection->send_msg(msg); // send mensage to externall node

    for (uint i = 0; i < internal_nodes.size(); i++) // send mensage to to all internall nodes
    {
        internal_nodes[i]->tcp_connection->send_msg(msg);
    }
}

/**
 * Create a TCP server
 *
 * @param ip server ip
 * @param port server port
 * @return void
 */
TCP_Server::TCP_Server(const char *ip, const char *port)
{
    int errcode;

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) // Error oppening socket
    {
        cout << "[SERVER LAUNCH ERROR]: Unable to cretate TCP socket!\n";
        SAFE_EXIT(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(ip);
    address.sin_port = htons(stoi(port));
    if ((errcode = bind(fd, (struct sockaddr *)&address, sizeof(address))) == -1)
    {
        cout << "[BIND FAILED]: " << gai_strerror(errcode) << endl;
        SAFE_EXIT(EXIT_FAILURE);
    }

    if ((errcode = listen(fd, MAX_NUMBER_OF_NODES)) < 0) // Error in bind
    {
        cout << "[LISTEN ERROR]: " << gai_strerror(errcode) << endl;
        SAFE_EXIT(EXIT_FAILURE);
    }

    cout << "TCP server listening on ip " << ip << "  port " << port << endl;
}

/**
 * Free's addr info and closes socket connetion
 *
 * @return void.
 */
TCP_Server::~TCP_Server()
{
    close(fd);
    cout << "Connection tcp server closed\n";
}

/**
 * Create a TCP connection
 *
 * @param ip server ip
 * @param port server port
 * @return true if sucessfull
 */
bool TCP_Connection::create(const char *ip, const char *port)
{
    int n;
    // TCP socket
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) // Error oppening socket
    {
        cout << "[SOCKER ERROR]: Unable to cretate TCP socket!\n";
        return false;
    }

    fd_handler_ptr->add(fd);

    addr.sin_family = AF_INET;
    addr.sin_port = htons(stoi(port));
    addr.sin_addr.s_addr = inet_addr(ip);

    if ((n = connect(fd, (sockaddr *)&addr, sizeof(addr))) == -1) // Error in getaddrinfo
    {
        cout << "[CREATING CONNECTION ERROR]: " << gai_strerror(n) << endl;
        return false;
    }

    active = true;
    return true;
}

/**
 * Sends a tcp message.
 *
 * @param msg message to send.
 * @return void.
 */
void TCP_Connection::send_msg(const char *msg)
{
    int n = 0;
    int nleft = 0;

    while (strlen(msg) > 0)
    {
        n = write(fd, msg, strlen(msg));

        if (n <= 0)
        {
            cout << "[ERROR SENDING TCP SERVER DATA]: " << strerror(n) << endl;
            SAFE_EXIT(EXIT_FAILURE);
        }
        nleft -= n;
        msg += n;
    }
}

/**
 * Recieves a message.
 *
 * @return mssg or "nodeleft" in case a node leaves.
 */
string TCP_Connection::receive_msg()
{
    int n, total_bytes_received = 0;

    while (true)
    {
        n = read(fd, buffer + total_bytes_received, 128);

        if (n == 0) // a node left the net
        {
            return "nodeleft\n";
        }

        if (n == -1)
        {
            cout << "[ERROR RECIEVING SERVER DATA]: " << strerror(errno) << endl;
            SAFE_EXIT(EXIT_FAILURE);
        }

        total_bytes_received += n;

        // Ignoring '\0' for saftey reasons:
        if (buffer[total_bytes_received - 1] == '\0')
            total_bytes_received--;

        if (buffer[total_bytes_received - 1] == '\n')
        {
            buffer[total_bytes_received] = '\0';
            return buffer;
        }
    }
}

/**
 * Accept new tcp connection
 *
 * @param server_fd tcp server fd.
 * @return void.
 */
void TCP_Connection::accept_new(int server_fd)
{
    if ((fd = accept(server_fd, (sockaddr *)&addr, &addrlen)) < 0)
    {
        cout << "[ERROR ACCEPTING CONNECTION]:  Unable to accept TCP connection" << endl;
        SAFE_EXIT(EXIT_FAILURE);
    }

    fd_handler_ptr->add(fd);
    cout << "Accepted new connection!" << endl;
    active = true;
}

/**
 * Closes tcp connection
 *
 * @return void.
 */
TCP_Connection::~TCP_Connection()
{
    if (!active)
        return;

    fd_handler_ptr->remove(fd);

    freeaddrinfo(res);
    close(fd);
    cout << "Tcp conection closed\n";
}

namespace ExpeditionTable
{
    /**
     * Updates expedition table based on tcp connections .
     *
     * @return void.
     */
    void update_neighbors()
    {
        int *const table = master_node_ptr->expedtion_table;

        int id = stoi(master_node_ptr->id), ext_id = stoi(master_node_ptr->external_node.id), back_id = stoi(master_node_ptr->backup_node.id), int_id;

        table[ext_id] = ext_id != id ? ext_id : -1;   // update external
        table[back_id] = back_id != id ? ext_id : -1; // update backup

        // update internals
        for (uint i = 0; i < master_node_ptr->internal_nodes.size(); i++)
        {
            int_id = stoi(master_node_ptr->internal_nodes[i]->id);
            table[int_id] = int_id;
        }
    }

    /**
     * Removes node from expedition table
     *
     * @param id id from node that left the net.
     * @return void.
     */
    void remove_node(int id)
    {
        int *const table = master_node_ptr->expedtion_table;

        table[id] = -1;
        update_neighbors();
    }
    /**
     * Checks the expedition table on wich node to msg to reach destination.
     *
     * @param dest_id destination node id.
     * @return node to msg or null if node is not on table.
     */
    Node *get_node_to_msg(string dest_id)
    {
        int neighbor_id = master_node_ptr->expedtion_table[stoi(dest_id)];

        if (neighbor_id == -1)
        {
            return nullptr;
        }

        string n_id = to_string(neighbor_id);
        if (n_id.size() == 1)
            n_id = "0" + n_id;

        if (master_node_ptr->external_node.id.compare(n_id) == 0)
        {
            return &master_node_ptr->external_node;
        }

        for (uint i = 0; i < master_node_ptr->internal_nodes.size(); i++)
        {
            if (master_node_ptr->internal_nodes[i]->id.compare(n_id) == 0)
            {
                return master_node_ptr->internal_nodes[i];
            }
        }

        return nullptr;
    }
}

namespace MsgHadler
{
    enum ext_args
    {
        EXTERN,
        ID,
        IP,
        TCP,
    };

    enum query_args
    {
        QUERY,
        DEST,
        SRC,
        NAME,
    };

    /**
     * Handles the EXTERN message
     *
     * @param args arguments of the mensage
     * @param msg mensage that was sent
     * @return void.
     */
    void process_extern_msg(vector<string> args, string msg)
    {
        if (args.size() != 4)
        {
            cout << "[EXTERN MSG ERROR]: recived -> " << msg << endl;
            return;
        }

        if (master_node_ptr->external_node.id.compare(args[ID]) == 0) // Anchor
        {
            master_node_ptr->external_node.id = args[ID];
            master_node_ptr->external_node.ip = args[IP];
            master_node_ptr->external_node.port = args[TCP];
        }
        else
        {
            master_node_ptr->backup_node.id = args[ID];
            master_node_ptr->backup_node.ip = args[IP];
            master_node_ptr->backup_node.port = args[TCP];
        }
    }

    /**
     * Handles when node is the QUERY destination.
     *
     * @param dest destination id.
     * @param content_name content name.
     * @param node node that sent the message.
     * @return void.
     */
    void query_dest(string dest, string content_name, Node *node)
    {
        for (uint i = 0; i < master_node_ptr->names.size(); i++)
        {
            if (master_node_ptr->names[i].compare(content_name) == 0) // name exists on this node
            {
                string msg = "CONTENT " + dest + " " + master_node_ptr->id + " " + content_name + "\n";
                node->tcp_connection->send_msg(msg.c_str());
                return;
            }
        }
        string msg = "NOCONTENT " + dest + " " + master_node_ptr->id + " " + content_name + "\n";
        node->tcp_connection->send_msg(msg.c_str());
    }

    /**
     * Sends a message to everyone but the node that sent us a message.
     *
     * @param msg message to send.
     * @param src node that sent us the message .
     * @return void.
     */
    void sendto_everyone_but_src(string msg, Node *src)
    {
        if ((master_node_ptr->external_node.tcp_connection != nullptr) && (master_node_ptr->external_node.id.compare(src->id) != 0))
            master_node_ptr->external_node.tcp_connection->send_msg(msg.c_str());

        for (uint i = 0; i < master_node_ptr->internal_nodes.size(); i++)
        {
            if (master_node_ptr->internal_nodes[i]->id.compare(src->id) != 0)
                master_node_ptr->internal_nodes[i]->tcp_connection->send_msg(msg.c_str());
        }
    }

    /**
     * Handles QUERY mensage
     *
     * @param args arguments of the mensage.
     * @param msg message that was sent
     * @param node node that sent us the message .
     * @return void.
     */
    void process_query_msg(vector<string> args, string msg, Node *node)
    {
        if (args.size() != 4)
        {
            cout << "[QUERY MSG INVALID]: recived -> " << msg << endl;
            return;
        }

        if (master_node_ptr->id.compare(args[DEST]) == 0) // dest = node_id
            query_dest(args[SRC], args[NAME], node);
        else
            sendto_everyone_but_src(msg, node);

        master_node_ptr->expedtion_table[stoi(args[SRC])] = stoi(node->id); // update expedition table
    }

    /**
     * Handles content msg.
     *
     * @param args arg vector
     * @param msg message to send.
     * @param node node that sent us the message .
     * @return void.
     */
    void process_content_msg(vector<string> args, string msg, Node *node)
    {
        if (args.size() != 4)
        {
            cout << "[CONTENT MSG INVALID]: recived -> " << msg << endl;
            return;
        }

        Node *node_to_send = ExpeditionTable::get_node_to_msg(args[DEST]); // check expedition table

        if (master_node_ptr->id.compare(args[DEST]) == 0)
        {
            master_node_ptr->expedtion_table[stoi(args[SRC])] = stoi(node->id);

            if (args[0].compare("CONTENT") == 0)
                cout << "Recieved content: " << args[NAME] << endl;
            else
                cout << "Content does not exist on node " << args[SRC] << endl;

            return;
        }

        if (node_to_send != nullptr)
        {
            node_to_send->tcp_connection->send_msg(msg.c_str());
        }
        else
        {
            cout << "[CONTENT MSG ERROR]: unable to resolve path" << endl;
        }
    }

    /**
     * Handles withdraw msg adn updates expedition table.
     *
     * @param args arg vector
     * @param msg message to send.
     * @param node node that sent us the message .
     * @return void.
     */
    void process_withdraw_msg(vector<string> args, string msg, Node *node)
    {

        if (args.size() != 2)
        {
            cout << "[WITHDRAW MSG INVALID]: recived -> " << msg << endl;
            return;
        }

        sendto_everyone_but_src(msg, node);

        // update expedition table
        master_node_ptr->expedtion_table[stoi(args[1])] = -1;
        for (uint i = 0; i < N_NODES; i++)
        {
            int val = master_node_ptr->expedtion_table[i];
            master_node_ptr->expedtion_table[i] = val == stoi(args[1]) ? -1 : val;
        }
    }

    /**
     * Processes recived msg.
     *
     * @param msg message that was sent.
     * @param node node that sent us the message .
     * @return void.
     */
    void process_msg(string msg, Node *node)
    {
        vector<string> args = string_to_args(msg);

        if (args[0].compare("EXTERN") == 0)
        {
            process_extern_msg(args, msg);
            ExpeditionTable::update_neighbors();
        }
        else if (args[0].compare("WITHDRAW") == 0)
        {
            process_withdraw_msg(args, msg, node);
        }
        else if (args[0].compare("QUERY") == 0)
        {
            process_query_msg(args, msg, node);
        }

        else if (args[0].compare("CONTENT") == 0)
        {
            process_content_msg(args, msg, node);
        }
        else if (args[0].compare("NOCONTENT") == 0)
        {
            process_content_msg(args, msg, node);
        }
    }
}

// Functions that handle a node leaving the net
namespace LeaveHandler
{

    /*
     * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
     * There are 4 cases for rebuilding the net after a node leaves:                                                                                         *
     *                                                                                                                                                       *
     *   1 - An internal node leaves -> remove him from internal list.                                                                                       *
     *                                                                                                                                                       *
     *   2 - An external leaves                                                                                                                              *
     *       2.1 - the node was not an achor -> connect backup, becomes extern & send EXTERN msg to internal nodes.                                          *
     *       2.2 - the node that left was formed an anchor with me -> create an achor with an internal node & send EXTERN msg to internal nodes.             *
     *       2.3 - there's just 2 nodes on net -> extern and backup become myself.                                                                           *
     *                                                                                                                                                       *
     * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
     */

    /**
     * Handles leave of an internal node.
     *
     * @param node_that_left node that left the net .
     * @return true if it was an internal node.
     */
    bool internal_handler(Node *node_that_left)
    {
        for (uint i = 0; i < master_node_ptr->internal_nodes.size(); i++)
        {
            if (master_node_ptr->internal_nodes[i]->id == node_that_left->id) // case 1
            {
                delete master_node_ptr->internal_nodes[i];
                master_node_ptr->internal_nodes.erase(master_node_ptr->internal_nodes.begin() + i);
                return true;
            }
        }
        return false;
    }

    /**
     * Handles case 2.1.
     *
     * @return void.
     */
    void external_handler_1()
    {
        string net, id, ip, tcp, boot_id, boot_ip, boot_tcp;
        string msg;

        net = master_node_ptr->net;
        id = master_node_ptr->id;
        ip = master_node_ptr->ip;
        tcp = master_node_ptr->port;

        boot_id = master_node_ptr->backup_node.id;
        boot_ip = master_node_ptr->backup_node.ip;
        boot_tcp = master_node_ptr->backup_node.port;
        master_node_ptr->backup_node.ip = master_node_ptr->ip;
        master_node_ptr->backup_node.port = master_node_ptr->port;
        master_node_ptr->backup_node.tcp_connection = nullptr;
        Join::cmd_djoin(net, id, ip, tcp, boot_id, boot_ip, boot_tcp);

        msg = "EXTERN " + master_node_ptr->external_node.id + " " + master_node_ptr->external_node.ip + " " + master_node_ptr->external_node.port + "\n";

        for (uint i = 0; i < master_node_ptr->internal_nodes.size(); i++)
            master_node_ptr->internal_nodes[i]->tcp_connection->send_msg(msg.c_str());
    }

    /**
     * Handles case 2.2.
     *
     * @return void.
     */
    void external_handler_2()
    {
        string msg;

        master_node_ptr->external_node.id = master_node_ptr->internal_nodes[0]->id;
        master_node_ptr->external_node.ip = master_node_ptr->internal_nodes[0]->ip;
        master_node_ptr->external_node.port = master_node_ptr->internal_nodes[0]->port;
        master_node_ptr->external_node.tcp_connection = master_node_ptr->internal_nodes[0]->tcp_connection;
        master_node_ptr->backup_node.id = master_node_ptr->id;
        master_node_ptr->backup_node.ip = master_node_ptr->ip;
        master_node_ptr->backup_node.port = master_node_ptr->port;

        msg = "EXTERN " + master_node_ptr->external_node.id + " " + master_node_ptr->external_node.ip + " " + master_node_ptr->external_node.port + "\n";
        master_node_ptr->external_node.tcp_connection->send_msg(msg.c_str());

        for (uint i = 0; i < master_node_ptr->internal_nodes.size(); i++)
            master_node_ptr->internal_nodes[i]->tcp_connection->send_msg(msg.c_str());

        master_node_ptr->internal_nodes.erase(master_node_ptr->internal_nodes.begin());
    }

    /**
     * Handles case 2.3.
     *
     * @return void.
     */
    void external_handler_3()
    {
        master_node_ptr->external_node.id = master_node_ptr->id;
        master_node_ptr->external_node.ip = master_node_ptr->ip;
        master_node_ptr->external_node.port = master_node_ptr->port;
        master_node_ptr->external_node.tcp_connection = nullptr;
        master_node_ptr->backup_node.id = master_node_ptr->id;
        master_node_ptr->backup_node.ip = master_node_ptr->ip;
        master_node_ptr->backup_node.port = master_node_ptr->port;
        master_node_ptr->backup_node.tcp_connection = nullptr;
    }

    /**
     * Rebuilds net whena  node leaves.
     *
     * @param node_that_left node that left the net .
     * @return void.
     */
    void rebuild_net(Node *node_that_left)
    {
        if (node_that_left->id == master_node_ptr->id)
        {
            fd_handler_ptr->remove(node_that_left->tcp_connection->fd);
            return;
        }

        string net, id, ip, tcp, boot_id, boot_ip, boot_tcp;

        net = master_node_ptr->net;
        id = master_node_ptr->id;
        ip = master_node_ptr->ip;
        tcp = master_node_ptr->port;

        CLOSE_TCP(node_that_left->tcp_connection);

        if (internal_handler(node_that_left)) // case 1
            return;

        // Case 2:
        if (master_node_ptr->backup_node.id != master_node_ptr->id) // 2.1
            external_handler_1();
        else if (master_node_ptr->backup_node.id == master_node_ptr->id && master_node_ptr->internal_nodes.size() != 0) // 2.2
            external_handler_2();

        else // 2.3
            external_handler_3();
    }

}
