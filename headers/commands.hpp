#ifndef _COMMANDS_H_
#define _COMMANDS_H_

#include "headers.hpp"
#include "server_com.hpp"
#include "node_handling.hpp"

// Handling user input:
class InputHandler
{
private:
    vector<string> args; // User command with args separed in individual strings

public:
    uint size; // number of args on command

    InputHandler();
    string get_arg(uint i);
};

enum args
{
    PROG_NAME,
    NODE_IP,
    NODE_PORT,
    SERVER_IP,
    SERVER_PORT
};

namespace Join // Functions for handling a node joining the net
{
    bool cmd_join(string net, string id, string ip, string tcp);
    bool cmd_djoin(string net, string id, string ip, string tcp, string boot_id, string boot_ip, string boot_tcp);
    bool recieve_node(int server_fd);
}

namespace Contents // Content and expedition table managemnet
{
    void cmd_show_names();
    void cmd_create_name(string input_name);
    void cmd_delete_name(string name);
    void cmd_clear_routing();
    void cmd_show_routing();
    void cmd_get(string dest, string name);
}

void process_user_cmd(InputHandler &input, char **argv);
void cmd_show_topology();
vector<string> string_to_args(string str);

#endif
