#include "headers.hpp"
#include "commands.hpp"
#include "server_com.hpp"
#include "node_handling.hpp"
#include "fd_handler.hpp"

#define IP_SIZE 16
#define PORT_SIZE 6
#define MAX_PORT 65535

#define NODE_LEFT() msg.compare("nodeleft");

FdHandler *fd_handler_ptr;
MasterNode *master_node_ptr;
Server *server_ptr;

bool running = true;

/**
 * Check program invocation args and SAFE_EXITs in case of invaild invocation.
 *
 * @param argc arg count.
 * @param argv arg vector.
 * @return void.
 */
void check_program_args(int argc, char **argv)
{
    struct in_addr ipv4_addr;

    if (!(argc == 3 || argc == 5)) // Check arg count
    {
        cout << "[INVAILD PROGRAM INVOCATION]: Program needs 2 or 4 args, was invoked with " << argc - 1 << " args!\n";
        SAFE_EXIT(EXIT_FAILURE);
    }

    if (inet_pton(AF_INET, argv[NODE_IP], &ipv4_addr) != 1) // Check IP adress
    {
        cout << "[INVALID TCP IP]: adress \"" << argv[NODE_IP] << "\" is not valid!\n";
        SAFE_EXIT(EXIT_FAILURE);
    }

    if (stoi(argv[NODE_PORT]) < 1 || stoi(argv[NODE_PORT]) > MAX_PORT) // Check port
    {
        cout << "[INVALID TCP PORT]: port \"" << argv[NODE_PORT] << "\" is not valid!\n";
        SAFE_EXIT(EXIT_FAILURE);
    }

    if (argc == 5)
    {
        if (inet_pton(AF_INET, argv[SERVER_IP], &ipv4_addr) != 1)
        {
            cout << "[INVALID UDP IP]: adress \"" << argv[SERVER_IP] << "\" is not valid!\n";
            SAFE_EXIT(EXIT_FAILURE);
        }

        if (stoi(argv[SERVER_PORT]) < 1 || stoi(argv[SERVER_PORT]) > MAX_PORT)
        {
            cout << "[INVALID UDP PORT]: port \"" << argv[SERVER_PORT] << "\" is not valid!\n";
            SAFE_EXIT(EXIT_FAILURE);
        }
    }
}

int main(int argc, char **argv)
{
    MasterNode master_node;
    master_node_ptr = &master_node;

    Server server;
    server_ptr = &server;

    FdHandler fd_handler;
    fd_handler_ptr = &fd_handler;

    TCP_Server tcp_server(argv[NODE_IP], argv[NODE_PORT]);
    char buffer[128] = {0};
    char *token;

    // bool node_in_net = false;

    check_program_args(argc, argv);

    char regIP[IP_SIZE] = "193.136.138.142";
    char regUDP[PORT_SIZE] = "59000";

    // Init regIP and regUDP in case they are given by user:
    if (argc == 5)
    {
        strcpy(regIP, argv[SERVER_IP]);
        strcpy(regUDP, argv[SERVER_PORT]);
    }

    server.connect(regIP, regUDP); // connecting to node server

    // Adding fd's:
    fd_handler.add(STDIN_FILENO);
    fd_handler.add(tcp_server.fd);

    fd_handler.tcp_server_fd = tcp_server.fd;

    while (running)
    {
        cout << "Command: ";
        fflush(stdout);

        fd_handler.check_select();
        for (int i = 0; i <= fd_handler.max_fd; i++)
        {
            if (fd_handler.is_set(i))
            {
                if (fd_handler.is_new_connection(i))
                {
                    Join::recieve_node(i);
                    ExpeditionTable::update_neighbors();
                }
                else if (fd_handler.is_user_cmd(i))
                {
                    InputHandler input; // Get command from user (class constructor gets command and divides its args)
                    process_user_cmd(input, argv);
                }
                else // node msg
                {
                    Node *node = fd_to_node(i);

                    if (node == nullptr)
                    {
                        cout << "Unrecognised node" << endl;
                        fd_handler.remove(i);
                        continue;
                    }

                    string msg = node->tcp_connection->receive_msg();

                    // Decomposing multiple messages:
                    strcpy(buffer, msg.c_str());
                    token = strtok(buffer, "\n");
                    while (token != NULL)
                    {
                        string msg = token;
                        msg += "\n";

                        if (msg.compare("nodeleft\n") == 0) // A node left the net
                        {
                            int id = stoi(node->id);

                            string cmd1 = "WITHDRAW " + node->id + "\n";

                            // update expedition table
                            master_node_ptr->expedtion_table[id] = -1;
                            for (uint i = 0; i < N_NODES; i++)
                            {
                                int val = master_node_ptr->expedtion_table[i];
                                master_node_ptr->expedtion_table[i] = val == id ? -1 : val;
                            }

                            LeaveHandler::rebuild_net(node);
                            master_node_ptr->send_to_neighbors(cmd1.c_str());
                            ExpeditionTable::remove_node(id);
                        }
                        else
                        {
                            MsgHadler::process_msg(msg, node);
                        }
                        token = strtok(NULL, "\n");
                    }
                }
            }
        }
    }
    return 0;
}
