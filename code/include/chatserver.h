#ifndef CHATSERVER_H_
#define CHATSERVER_H_

#include "chatapp.h"

class ChatServer: public ChatApp {

public:
    ChatServer(char *port): ChatApp(port) {}
    // Stage one
    int run(char *port);
    // Stage two

protected:
    int listen_fd;
    Incoming_Streams master_list;
    std::vector<ClientListEntry> clientList;
    // Stage one
    int accept_login();
    void get_new_client_info(int);
    void send_connected_client_list(int);
    void sort_client_list();
    void print_list(const char*);
    void clientExited(int);
    void block_until_fd_is_ready_to_write(int);
    
    // Stage two
    // void statistics();
    // void blocked();
    Packet *make_packet(Command,ClientListEntry*);
    void forwardMessage(Packet*);
    void broadcast(Packet*);
    ClientListEntry *findClient(char *ip);
    

};

#endif