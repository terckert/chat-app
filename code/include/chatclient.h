#ifndef CHATCLIENT_H_
#define CHATCLIENT_H_

#include "chatapp.h"

class ChatClient: public ChatApp {

public:
    ChatClient(char *port): ChatApp(port), server_fd(-1) {}
    // Stage one
    int run(char *port);

    // Stage two

protected:
    // Class variables
    Server_FD server_fd;      // Server file descriptor, -1 when logged out
    ConnectedClientList connectedClientList;

    // Stage one
    bool login(LoginArgs *args);
    void receive_list();
    bool connectToServer(char* ip_addr,char*  port);
    void print_list(const char*);
    void refresh();
    void clientExit();
    bool confirmExit();
    bool validate_ip_and_port(LoginArgs *args);
    void commandError(Command);

    // Stage two
    Packet *make_packet(Command);
    void sendMsg(Packet*);
    void broadcastMsg(Packet*);
    // void blockClient();
    // void unblockClient();
    
    
};

#endif