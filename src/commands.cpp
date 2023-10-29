#include "headers.hpp"
#include "commands.hpp"
#include "fd_handler.hpp"
#include "node_handling.hpp"

extern FdHandler *fd_handler_ptr;
extern MasterNode *master_node_ptr;
extern Server *server_ptr;
extern bool running;

/**
 * Checks for invalid number of args in command, prints out error message and continues loop.
 *
 * @param n_args number of arguments in command.
 * @param expected_n_args Expected nuber of args in command.
 * @return void.
 */
#define check_arg_numb_error(n_args, expected_n_args)                                                              \
    if (n_args != expected_n_args)                                                                                 \
    {                                                                                                              \
        cout << "[INVALID NUMBER OF ARGS]: expected " << expected_n_args << " args, got " << n_args << " arg\n\n"; \
        return;                                                                                                    \
    }

/**
 * Porcesses a command invocation by the user via stdin.
 *
 * @param input class that stores input.
 * @param argv invocation args vector.
 * @return void.
 */
void process_user_cmd(InputHandler &input, char **argv)
{
    if ((input.get_arg(0).compare("join")) == 0)
    {
        string ip(argv[NODE_IP]);
        string tcp(argv[NODE_PORT]);

        check_arg_numb_error(input.size, 3);
        master_node_ptr->active = Join::cmd_join(input.get_arg(1), input.get_arg(2), ip, tcp);

        if (!master_node_ptr->active) // Server ofline
            return;

        ExpeditionTable::update_neighbors();
    }
    else if ((input.get_arg(0).compare("djoin")) == 0)
    {
        string ip(argv[NODE_IP]);
        string tcp(argv[NODE_PORT]);

        check_arg_numb_error(input.size, 6);
        master_node_ptr->active = Join::cmd_djoin(input.get_arg(1), input.get_arg(2), ip, tcp, input.get_arg(3), input.get_arg(4), input.get_arg(5));
        ExpeditionTable::update_neighbors();
    }
    else if ((input.get_arg(0).compare("st") == 0))
    {
        check_arg_numb_error(input.size, 1);
        cmd_show_topology();
    }
    else if ((input.get_arg(0).compare("create") == 0))
    {
        check_arg_numb_error(input.size, 2);
        Contents::cmd_create_name(input.get_arg(1));
    }
    else if ((input.get_arg(0).compare("delete") == 0))
    {
        check_arg_numb_error(input.size, 2);
        Contents::cmd_delete_name(input.get_arg(1));
    }
    else if ((input.get_arg(0).compare("get") == 0))
    {
        check_arg_numb_error(input.size, 3);
        Contents::cmd_get(input.get_arg(1), input.get_arg(2));
    }
    else if ((input.get_arg(0).compare("sn") == 0) && (input.size == 1))
    {
        check_arg_numb_error(input.size, 1);
        Contents::cmd_show_names();
    }
    else if ((input.get_arg(0).compare("cr") == 0))
    {
        check_arg_numb_error(input.size, 1);
        Contents::cmd_clear_routing();
    }
    else if ((input.get_arg(0).compare("sr") == 0))
    {
        check_arg_numb_error(input.size, 1);
        Contents::cmd_show_routing();
    }
    else if ((input.get_arg(0).compare("leave") == 0))
    {
        check_arg_numb_error(input.size, 1);
        cmd_leave();
    }
    else if ((input.get_arg(0).compare("exit") == 0))
    {
        check_arg_numb_error(input.size, 1);
        cmd_leave();
        running = false;
    }
    else /* command doesn't exist */
    {
        cout << "[INVALID COMMAND]:  command \"" << input.get_arg(0) << "\" not found!\n\n";
    }
}

/**
 * Gets user input and separates its arrgs into a vector of args.
 *
 * @return void.
 */
InputHandler::InputHandler()
{
    string input;
    getline(cin, input);

    std::istringstream iss(input);
    string arg;

    // Adding strings to vector
    while (iss >> arg)
    {
        args.push_back(arg);
    }
    size = args.size();
}

/**
 * Gets an arg from the input
 *
 * @param i arg number
 * @return the given arg or NULL if the argument doesn't exist.
 */
string InputHandler::get_arg(uint i)
{

    if (i >= size) // Check for ivalid i
        return NULL;

    return args[i];
}

/**
 * Separates a sting in args delimeted by a space.
 *
 * @param string string separate.
 * @return vector of args.
 */
vector<string> string_to_args(string str)
{
    vector<string> args;

    std::istringstream iss(str);
    string arg;

    while (iss >> arg)
    {
        args.push_back(arg);
    }

    return args;
}

// Join and djoin functions, aswell as all axiliary functions for each
namespace Join
{
    /**
     * Checks for valid args to execute the join command
     *
     * @param net desired net
     * @param id node id
     * @return true if the given arguments are invalid, false if valid
     */
    bool invalid_cmd_join_args(string net, string id)
    {
        if (stoi(net) < 0 || stoi(net) > 999)
        {
            cout << "[INVALID ARG]: net \"" << net << "\" is not between 000 and 999 \n\n";
            return true;
        }

        if (stoi(id) < 0 || stoi(id) > 99)
        {
            cout << "[INVALID ARG]: id \"" << id << "\" is not between 00 and 99 \n\n";
            return true;
        }

        return false;
    }

    /**
     * Creates vector with all ids in a given net
     *
     * @param net desired net
     * @return vector with all ids ocupied on net
     */
    vector<Node *> get_net_nodes(string net)
    {
        vector<Node *> net_nodes;
        string line;
        string id, ip, tcp;

        // Send command to get all nodes in a net:
        string cmd = "NODES " + net;

        if (!server_ptr->send_cmd(cmd))
            return net_nodes;

        string msg = server_ptr->receive_msg();
        stringstream lines(msg);

        getline(lines, line); // skip first line
        while (getline(lines, line))
        {
            std::istringstream iss(line);
            getline(iss, id, ' ');
            getline(iss, ip, ' ');
            getline(iss, tcp, ' ');

            Node *n = new Node;
            n->id = id;
            n->ip = ip;
            n->port = tcp;
            net_nodes.push_back(n);
        }

        return net_nodes;
    }

    /**
     * Assigns a new id to a node
     *
     * @param id original id form node
     * @param used_ids vector with all used ids from a net
     * @return the given id or NULL if the argument doesn't exist.
     */
    int assign_id(uint id, vector<uint> used_ids)
    {
        bool id_exists = false;

        // verifies if the id given by the client already exists
        for (uint i = 0; i < used_ids.size(); i++)
        {
            if (id == used_ids[i])
                id_exists = true;
        }

        if (!id_exists)
            return id;

        // if the id already exists this function gives the node a new id
        for (uint i = 0; i < 100; i++)
        {
            id_exists = false;
            for (uint j = 0; j < used_ids.size(); j++)
            {
                if (i == used_ids[j])
                    id_exists = true;
            }

            if (!id_exists)
            {
                cout << "Id \"" << std::setw(2) << std::setfill('0') << id << "\" was already in use, assigned id \"" << std::setw(2) << std::setfill('0') << i << "\" to node!\n";
                return i;
            }
        }
        return -1;
    }

    /**
     * Registers a node in a net
     *
     * @param net  node net
     * @param id node net
     * @param tcp tcp port
     * @return true if success false otherwise
     */
    bool register_node(string net, string id, string ip, string tcp)
    {

        // Ask server to register:
        string cmd = "REG " + net + " " + id + " " + ip + " " + tcp;
        server_ptr->send_cmd(cmd);

        string msg = server_ptr->receive_msg();

        if (msg.compare("OKREG") == 0) // Succesfull register
        {
            cout << "The node joined the net." << endl;
            return true;
        }
        else
        {
            cout << "[JOIN ERROR]: The node was not able to join the net. :( " << endl;
            return false;
        }
    }

    /**
     * Conecting to a node on the net
     *
     * @param id our node id
     * @param ip our node ip
     * @param tcp our node potr
     * @param boot_id node to join id
     * @param boot_ip node to join ip
     * @param boot_tcp node  to join potr
     * @return true if the connection was sucessfull flase otherwise;
     */
    bool connect_to_node(string id, string ip, string tcp, string boot_id, string boot_ip, string boot_tcp)
    {
        Node *ext_n = &master_node_ptr->external_node;

        ext_n->tcp_connection = new TCP_Connection;

        if (!ext_n->tcp_connection->create(boot_ip.c_str(), boot_tcp.c_str()))
            return false;

        string cmd = "NEW " + id + " " + ip + " " + tcp + "\n";

        ext_n->tcp_connection->send_msg(cmd.c_str());

        ext_n->id = boot_id;
        ext_n->ip = boot_ip;
        ext_n->port = boot_tcp;

        return true;
    }

    /**
     * djoin command
     *
     * @param net the net we want to join
     * @param id our node id
     * @param ip our node ip
     * @param tcp our node potr
     * @param boot_id node to join id
     * @param boot_ip node to join ip
     * @param boot_tcp node  to join potr
     * @return true if the connection was sucessfull flase otherwise;
     */
    bool cmd_djoin(string net, string id, string ip, string tcp, string boot_id, string boot_ip, string boot_tcp)
    {
        if (id.size() == 1)
            id = '0' + id;

        if (boot_id.size() == 1)
            boot_id = '0' + boot_id;

        if (net.size() == 1)
        {
            net = "00" + net;
        }
        else if (net.size() == 2)
        {
            net = '0' + net;
        }

        // initializing node info with default values (my_node info):
        master_node_ptr->id = id;
        master_node_ptr->ip = ip;
        master_node_ptr->port = tcp;
        master_node_ptr->net = net;
        master_node_ptr->external_node.id = id;
        master_node_ptr->external_node.ip = ip;
        master_node_ptr->external_node.port = tcp;
        master_node_ptr->backup_node.id = id;
        master_node_ptr->backup_node.ip = ip;
        master_node_ptr->backup_node.port = tcp;

        if (id.compare(boot_id) != 0)
        {
            return connect_to_node(id, ip, tcp, boot_id, boot_ip, boot_tcp); // connect to boot_node
        }                                                                    // not joining an empty net

        return true;
    }

    /**
     * Node tries to join the net
     *
     * @param net desired net
     * @param id node id
     * @param ip node ip
     * @param tcp node port
     * @return true if the command was sucessfull false otherwise.
     */
    bool cmd_join(string net, string id, string ip, string tcp)
    {
        int new_id;
        vector<uint> ids;
        string boot_id, boot_ip, boot_tcp;

        if (net.size() == 1)
        {
            net = "00" + net;
        }
        else if (net.size() == 2)
        {
            net = '0' + net;
        }

        if (invalid_cmd_join_args(net, id))
            return false;

        vector<Node *> net_nodes = get_net_nodes(net);

        if (!server_ptr->online)
        {
            cout << "[JOIN ERROR]: unable to reach server." << endl;
            return false;
        }

        for (int i = 0; i < (int)net_nodes.size(); i++)
            ids.push_back(stoi(net_nodes[i]->id));

        if ((new_id = assign_id(stoi(id), ids)) == -1)
        {
            cout << "[ERROR: NET IS FULL]: No more ids available! " << endl;
            return false;
        }

        id = to_string(new_id);

        if (net_nodes.size() == 0) // no nodes on net
        {
            boot_id = id;
            boot_ip = ip;
            boot_tcp = tcp;
        }
        else
        {
            random_device rd;
            mt19937 gen(rd());

            uniform_int_distribution<> dis(0, net_nodes.size() - 1);

            int pos = dis(gen);

            boot_id = net_nodes[pos]->id;
            boot_ip = net_nodes[pos]->ip;
            boot_tcp = net_nodes[pos]->port;
        }

        // free memory
        for (uint i = 0; i < net_nodes.size(); i++)
            delete net_nodes[i];

        if (id.size() == 1)
            id = '0' + id;

        if (boot_id.size() == 1)
            boot_id = '0' + boot_id;

        if (!register_node(net, id, ip, tcp)) // Register node on node server
            return false;

        return cmd_djoin(net, id, ip, tcp, boot_id, boot_ip, boot_tcp);
    }

    /**
     * Accepts a new node connection
     *
     * @param server_fd tcp_server_fd
     * @return true if succesfull.
     */
    bool recieve_node(int server_fd)
    {
        Node *new_node = new Node;
        new_node->tcp_connection = new TCP_Connection;

        new_node->tcp_connection->accept_new(server_fd);

        string msg = new_node->tcp_connection->receive_msg();

        vector<string> args = string_to_args(msg);
        string wd;

        // NEW + withdraw
        if (args.size() != 4)
        {
            wd = args[5];
            int id = stoi(wd);

            master_node_ptr->expedtion_table[id] = -1;
            for (uint i = 0; i < N_NODES; i++)
            {
                int val = master_node_ptr->expedtion_table[i];
                master_node_ptr->expedtion_table[i] = val == id ? -1 : val;
            }
        }

        if (args[0].compare("NEW") == 0)
        {
            if (master_node_ptr->id.size() == 1)
                master_node_ptr->id = '0' + master_node_ptr->id;

            string cmd = "EXTERN " + master_node_ptr->external_node.id + " " + master_node_ptr->external_node.ip + " " + master_node_ptr->external_node.port + "\n";
            new_node->tcp_connection->send_msg(cmd.c_str());

            new_node->id = args[1];
            new_node->ip = args[2];
            new_node->port = args[3];

            if (master_node_ptr->id == master_node_ptr->external_node.id) // node was alone on net
            {
                master_node_ptr->external_node.id = new_node->id;
                master_node_ptr->external_node.ip = new_node->ip;
                master_node_ptr->external_node.port = new_node->port;
                master_node_ptr->external_node.tcp_connection = new_node->tcp_connection;
            }
            else // internal node
            {
                master_node_ptr->internal_nodes.push_back(new_node);
                cout << "The node connected with sucess." << endl;
                return true;
            }

            delete new_node;
            cout << "The node connected with sucess." << endl;
            return true;
        }
        else
        {
            cout << "[RECIEVING CONNECTION ERROR]: Your node was not able to connect with the other node. :(" << endl;
            delete new_node;
            return false;
        }
    }
} // --------------------------------- END Join --------------------------------- //

/**
 * Shows node info
 *
 * @return true if succesfull.
 */
void cmd_show_topology()
{

    cout << "\n------------------ST----------------------\n";
    cout << "|Your node: " << endl;
    cout << "|   Id-> " << master_node_ptr->id << endl;
    cout << "|   Ip-> " << master_node_ptr->ip << endl;
    cout << "|   Tcp port-> " << master_node_ptr->port << endl;
    cout << "|External:" << master_node_ptr->external_node.id << endl;
    cout << "|   Id-> " << master_node_ptr->external_node.id << endl;
    cout << "|   Ip-> " << master_node_ptr->external_node.ip << endl;
    cout << "|   Tcp port-> " << master_node_ptr->external_node.port << endl;
    cout << "|Backup node: " << endl;
    cout << "|   Id-> " << master_node_ptr->backup_node.id << endl;
    cout << "|   Ip-> " << master_node_ptr->backup_node.ip << endl;
    cout << "|   Tcp port-> " << master_node_ptr->backup_node.port << endl;
    cout << endl;

    for (uint i = 0; i < master_node_ptr->internal_nodes.size(); i++)
    {
        cout << "|Internal node " << i << ": " << endl;
        cout << "|   Id-> " << master_node_ptr->internal_nodes[i]->id << endl;
        cout << "|   Ip-> " << master_node_ptr->internal_nodes[i]->ip << endl;
        cout << "|   Tcp port-> " << master_node_ptr->internal_nodes[i]->port << endl;
    }

    cout << "------------------END---------------------\n\n";
}

/**
 * Node leave's the net
 *
 * @param net desired net
 * @param id node id
 * @return void.
 */
void cmd_leave()
{
    if (!master_node_ptr->active)
    {
        cout << "No node to remove!" << endl;
        return;
    }

    string cmd2 = "UNREG " + master_node_ptr->net + " " + master_node_ptr->id + "\n";
    server_ptr->send_cmd(cmd2);

    string msg = server_ptr->receive_msg();

    if (msg.compare("OKUNREG") == 0)
        cout << "The node left the net." << endl;
    else
        cout << "No node to remove! " << endl;

    CLOSE_TCP(master_node_ptr->external_node.tcp_connection);
    CLOSE_TCP(master_node_ptr->backup_node.tcp_connection);

    for (uint i = 0; i < master_node_ptr->internal_nodes.size(); i++)
    {
        CLOSE_TCP(master_node_ptr->internal_nodes[i]);
        delete master_node_ptr->internal_nodes[i];
    }

    master_node_ptr->active = false;
}

namespace Contents
{
    /**
     * Creates new content.
     *
     * @param name_input content name.
     * @return void.
     */
    void cmd_create_name(string name_input)
    {
        if (name_input.size() <= 100) // Invalid name
        {
            master_node_ptr->names.push_back(name_input);
            cout << "Created: " << name_input << endl;
        }
        else
            cout << "[CONTENT ERROR]: Your content can't have more than 100 caracters." << endl;
    }

    /**
     * Deletes a content.
     *
     * @param name content name.
     * @return void.
     */
    void cmd_delete_name(string name)
    {
        for (uint i = 0; i < master_node_ptr->names.size(); i++)
        {
            if (master_node_ptr->names[i].compare(name) == 0)
            {
                master_node_ptr->names.erase(master_node_ptr->names.begin() + i);
                cout << "Deleted: " << name << endl;
                return;
            }
        }
        cout << "[CONTENT ERROR]: The content does not exist." << endl;
    }

    /**
     * Shows content names.
     *
     * @return void.
     */
    void cmd_show_names()
    {

        cout << "\n------------------SN----------------------\n";
        cout << "|Your content names are: " << endl;

        for (uint i = 0; i < master_node_ptr->names.size(); i++)
        {
            cout << "| " << master_node_ptr->names[i] << endl;
        }

        cout << "------------------END---------------------\n\n";
    }

    /**
     * Clears expedition table.
     *
     * @return void.
     */
    void cmd_clear_routing()
    {
        for (int i = 0; i < N_NODES; master_node_ptr->expedtion_table[i++] = -1)
            ;
    }

    /**
     * Shows expedition table.
     *
     * @return void.
     */
    void cmd_show_routing()
    {
        cout << "\n Destination   |   Neighbor" << endl;
        cout << "- - - - - - - - - - - - - - -" << endl;
        for (int i = 0; i < N_NODES; i++)
        {
            if (master_node_ptr->expedtion_table[i] != -1)
            {
                string r1 = i < 10 ? "0" + to_string(i) : to_string(i);
                string r2 = master_node_ptr->expedtion_table[i] < 10 ? "0" + to_string(master_node_ptr->expedtion_table[i]) : to_string(master_node_ptr->expedtion_table[i]);
                cout << "      " << r1 << "       |       " << r2 << endl;
            }
        }
    }

    /**
     * Startes search for the content name in the net
     *
     * @param dest final node.
     * @param name content name.
     * @return void.
     */
    void cmd_get(string dest, string name)
    {
        if (stoi(dest) < 0 || stoi(dest) > 100)
        {
            cout << "[ARG ERROR]: node id \"" << dest << "\" is invalidd!";
            return;
        }

        string msg = "QUERY " + dest + " " + master_node_ptr->id + " " + name + "\n";

        Node *node_to_msg = ExpeditionTable::get_node_to_msg(dest); // Search for node in expedition table

        if (node_to_msg == nullptr)                          // node is not on table
            master_node_ptr->send_to_neighbors(msg.c_str()); // send to all
        else
            node_to_msg->tcp_connection->send_msg(msg.c_str()); // send to node on table
    }
}
