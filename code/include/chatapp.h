#ifndef CHATAPP_H_
#define CHATAPP_H_

#include <string>
#include <logger.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <netdb.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <iostream>
#include <string.h>
#include <vector>
#include <algorithm>

#define BACKLOG 5
#define STDIN 0
#define User_FD 0

// Defining sizes in Bytes for data sizes
#define INPUT_SIZE 257
#define COMMAND_SIZE 4
#define MESSAGE_SIZE 257
#define PACKET_SIZE 300
#define HOST_NAME_LEN 35



typedef int Server_FD;
typedef fd_set Incoming_Streams;
typedef fd_set Outgoing_Streams;


/* 
    Definitons for machine state data types. HostInfo is stored local to each host.
    Client is the local data type that holds connected client information.
    ClientListEntry is the server data type that holds connected client information.
    ConnectedClientList and ServerClientList are typedefs of vectors that contain Client
    and ClientListEntry respectively. ConnectedClientList is used in clients, and 
    ServerClientList is used in the server.
 */
struct HostInfo {                      // Necessary machine information
       uint16_t port_num;               // Port number in integer form
       char port_string[6],             // Port number in string form
            ip_addr[INET_ADDRSTRLEN],   // IP string
            hostname[35];               // Host (computer) name
};

struct Client { // Part one data fields, can be composited for part two
    uint16_t list_id; 
    char ip_addr[16]; 
    uint16_t port_num; 
    char hostname[35]; 
    // bool operator<(Client& rhs) {
    //     return this->port_num < rhs.port_num;
    // }
    Client() {} // Default constructor, takes no arguments
    Client(uint16_t li, const char *ip, uint16_t pn, const char *hn)
        : list_id(li), port_num(pn) {
        strcpy(ip_addr, ip);
        strcpy(hostname, hn);
    }
};

struct ClientListEntry {  
    Client client;
    int client_stream;
    /* DATA FIELDS NEEDED IN STAGE 2
    int num_msg_sent;  //integer number of messages sent by the client
    int num_msg_rcv;  //integer number of messages received by the client
    char *status;  //null terminated char array containing "logged-in" or "logged-out"

    bool blocked;
    std::vector<std::string> block_list;
    std::vector<packet data type> message_buffer;
    */
    friend bool operator<(const ClientListEntry &lhs, const ClientListEntry &rhs) {
        return lhs.client.port_num < rhs.client.port_num;
    }
    ClientListEntry() {} // Default constructor, takes no arguments
    ClientListEntry(uint16_t li, const char *ip, uint16_t pn, const char *hn, int cs)
        : client(li, ip, pn, hn), client_stream(cs) {} // Parameterized to build in place.
};

typedef std::vector<Client> ConnectedClientList;
//typedef std::vector<ClientListEntry> ServerClientList;

/* 
    Definitions for messages and message types 
 */
enum Command {
        ERROR, AUTHOR, IP, PORT, LIST, STATISTICS, BLOCKED, EVENT, LOGIN,
        REFRESH, SEND, BROADCAST, BLOCK, UNBLOCK, LOGOUT, EXIT, ENDLIST, RELAYED
};

// Used to pass data from run to connectToServer()
struct LoginArgs{
    char server_ip_addr[16]; // null-terminated string that contains the IP address for the server. 16 Bytes
    char server_port_number[6]; // null-terminated string that contains the port number for the server. 6 Bytes 
    // 22 Bytes total
};

// Used to pass client information from the client to the server during login procedure
struct LoginData{
    char client_ip_addr[16]; // null-terminated string that contains the IP address of the client. 16 bytes
    char client_hostname[35]; // null-terminated string that contains the host name of the client 35 bytes
    uint16_t client_port_number; // 2 bytes
    // 53 Bytes
};

// Used to pass host information to and from a host connected to the server.
// When sending host information from host to server we do not use list_id field, 
// so the server can ignore that field when it receives this message type.
struct ConnectedHost {
    uint16_t list_id; // 2 Bytes
    uint16_t host_port_number; // 2 Bytes
    char host_ip_addr[16]; // 16 Bytes
    char hostname[35]; // 35 Bytes
    // 55 Bytes
};

// ClientMessage contains the IP address for the receiving client, and the text of the message.
struct ClientMessage {
    char destinationIP[INET_ADDRSTRLEN+1]; // 17 Bytes
    char sourceIP[INET_ADDRSTRLEN+1]; // 17 Bytes
    char message[MESSAGE_SIZE]; // 257 Bytes
    // 291 Bytes
};

// Unions that contains needed data that will be returned from process_user_input()
union Data {
    char message_buffer[MESSAGE_SIZE]; // 257 Bytes
    LoginArgs login_args; // 22 Bytes
    LoginData login_data; // 53 Bytes
    ConnectedHost connectedClientInfo; // 55 Bytes
    ClientMessage clientMessage; // 291 Bytes
};

// Packet is used to send data between hosts 
struct Packet {
    uint32_t command; // Need to cast command using htonl(Command) 4 Bytes
    Data data; // 274 Bytes
    // 291 Bytes
    char cowardPadding[9]; // 9 Bytes
    // 300 Bytes
};

class ChatApp {

public:
    ChatApp(char*);
    
    // Stage one
    virtual int run(char *port) { return 0; };
    
    // Stage two

protected:
    // Class variables
    Incoming_Streams rfds;          // Readable streams
    HostInfo machine_info;

    char STATUS_OUT[11];
    char STATUS_IN[10];
    char UBIT[9];
    // Storing commands as enums to make packet parsing easier. 
    
    Command string_to_command(std::string); // Convert string to enum
 
    // Stage one
    Packet* process_user_input(bool);
    Packet *receivePacket(int);
    void author(const char*);
    void display_ip(const char*);
    void print_port(const char*);
    void print_error(const char*);
    
    virtual Packet* make_packet(Command){return  NULL;}
    ssize_t send_packet(Server_FD fd, Packet *pkt);
   
};

#endif