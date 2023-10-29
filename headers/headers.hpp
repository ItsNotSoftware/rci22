#ifndef _HEADERS_H_
#define _HEADERS_H_

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <algorithm>
#include <random>

// Logging macros, used for debug:
#define logm(msg)                        \
    cout << "# " << msg << " #" << endl; \
    fflush(stdout)
#define logv(msg, var)                          \
    cout << "# " << msg << var << " #" << endl; \
    fflush(stdout)
#define separator()                                             \
    cout << "\n\n========================================\n\n"; \
    fflush(stdout)

// Leaves the net closes TCP and UDP connections and exits;
#define SAFE_EXIT(n) \
    cmd_leave();     \
    exit(n)

// Closes a TCP connection via class destructor
#define CLOSE_TCP(ptr) \
    {                  \
        delete ptr;    \
        ptr = nullptr; \
    }

using std::cin;
using std::cout;
using std::endl;
using std::mt19937;
using std::random_device;
using std::stoi;
using std::string;
using std::stringstream;
using std::to_string;
using std::uniform_int_distribution;
using std::vector;

typedef unsigned int uint;

void cmd_leave();

#endif
