#include "../include/chatclient.h"

int ChatClient::run(char *port) {

    // Adapted from:
    // https://www.youtube.com/watch?v=Y6pFtgRdUts
    // Retrieval date: 10/1/22
    // https://man7.org/linux/man-pages/man2/select.2.html
    // Retrieval date: 10/1/22
    Incoming_Streams ready_rfds; // Set to be passed into select since it's destructive
    FD_ZERO(&rfds);              // Make sure that memory is cleared in set
    FD_SET(User_FD, &rfds);      // Add stdin to set

    // Loop variables we don't want to create everytime.
    bool connectionFlag = false;

    while (true)
    {
        // Reset stream set to curring incoming streams. This is because select is destructive.
        ready_rfds = rfds;

        // Get the set of streams ready to be read from.
        if (select(FD_SETSIZE, &ready_rfds, NULL, NULL, NULL) < 0)
        {
            perror("Error in chat client - select statement");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < FD_SETSIZE; i++)
        {
            if (FD_ISSET(i, &ready_rfds))
            {

                if (i == STDIN)
                { // Check STDIN
                    Packet *userInput;
                    userInput = process_user_input(false);
                    uint32_t temp_command = ntohl(userInput->command);
                    Command command = (Command)temp_command; // Abusive casting of integers
                    switch (command) {
                        case AUTHOR:
                            // Does not require client to be logged-in to server to use
                            author(userInput->data.message_buffer);
                            break;
                        case IP:
                            // Does not require client to be logged-in to server to use
                            display_ip(userInput->data.message_buffer);
                            break;
                        case PORT:
                            // Does not require client to be logged-in to server to use
                            print_port(userInput->data.message_buffer);
                            break;
                        case LOGIN:
                            // Sends & receives data to & from server
                            if (validate_ip_and_port(&userInput->data.login_args) && server_fd == -1) {
                                 if(login(&userInput->data.login_args)) {
                                    cse4589_print_and_log("[%s:SUCCESS]\n", "LOGIN");
                                    cse4589_print_and_log("[%s:END]\n", "LOGIN");
                                    } else {
                                        commandError(command);
                                    }
                            } else {
                               commandError(command);
                            }
                            break;
                        case LIST:
                            // Requires client be logged-in to server
                            if (server_fd == -1){
                                commandError(command);
                            } else {
                                print_list(userInput->data.message_buffer);
                            }
                            break;
                        case REFRESH:
                            // Sends & receives data to & from server
                            if (server_fd == -1){
                                commandError(command);
                            } else {
                                refresh();
                            }
                            break;
                        case SEND:
                            if(server_fd == -1){
                                commandError(command);
                            }else {
                                ChatClient::sendMsg(userInput);
                            }
                            break;
                        case BROADCAST:
                            if(server_fd == -1){
                                commandError(command);
                            }else {
                                ChatClient::broadcastMsg(userInput);
                            }
                            break;
                        case EXIT:
                            // Requires client be logged-in to server
                            if (server_fd == -1){
                                exit(0);
                            } else {
                                clientExit();
                            }
                            break;
                        default:
                            cse4589_print_and_log("[%s:ERROR]\n", userInput->data.message_buffer);
                            cse4589_print_and_log("[%s:END]\n", userInput->data.message_buffer);
                    }
                } else if (i == server_fd) {
                    // Parse incoming information
                    // Part 2
                    Packet *recvdPkt = receivePacket(i);
                    Command command = (Command)ntohl(recvdPkt->command);
                    switch (command) {
                        case SEND:
                            cse4589_print_and_log("msg from:%s\n[msg]:%s\n", recvdPkt->data.clientMessage.sourceIP, recvdPkt->data.clientMessage.message); // Print and log received message
                            break;
                    }
                }
            }
        }
    }
    return 0;
}

bool ChatClient::login(LoginArgs *args) {
    bool successful = connectToServer(args->server_ip_addr, args->server_port_number);

    if(successful){
        Packet *pkt = make_packet(LOGIN);
        ssize_t bytes_sent = ChatApp::send_packet(server_fd,pkt);
        
        // List comes in from server after successful connection. Parse incoming list;
        receive_list();
    }

    return successful;
}

bool ChatClient::validate_ip_and_port(LoginArgs *args) {
    // Information about converting IP strings to bits based on same source in constructor
    // Source: https://linux.die.net/man/3/inet_pton
    // Accessed: 10/3/22
    // Resource: https://gist.github.com/listnukira/4045436
    // Accessed: 10/2/22
    char ghetto_void_pointer[32];
    if (inet_pton(AF_INET, args->server_ip_addr, ghetto_void_pointer) != 1) {
        return false;
    }

    for (int i = 0; args->server_port_number[i] != 0; i++ ) {
        if (!isdigit(args->server_port_number[i])){
            return false;
        }
    }
    
    return true;
}   


void ChatClient::receive_list() {
    // IMPORTANT: WHEN RECEIVING INTEGERS NEED TO USE ntohl (32 bit) ntohs (16 bit)
    // IMPORTANT: WHEN SENDING INTEGERS NEED TO US htonl (32bit) htons (16bit)
    // Erase the client list to prepare for new list
    if (connectedClientList.size() != 0)
    {
        connectedClientList.clear();
    }

    // While loop variables
    Incoming_Streams list_, list_temp; // Socket sets
    FD_ZERO(&list_);
    FD_SET(server_fd, &list_);

    // Buffer for reading form the socket.
    char pkt[PACKET_SIZE];
    Packet *pktCopy = (Packet *)pkt; // Casting as a command packet
    size_t byte_count; // Number of bytes in packet
    size_t buffer_len = sizeof(Command) + sizeof(pktCopy->data.connectedClientInfo);

    Command comm = REFRESH; // Loop sentinel

    // Reading information taken from beej.us
    while (comm == REFRESH)
    {
        list_temp = list_;
        if (select(FD_SETSIZE, &list_temp, NULL, NULL, NULL) < 0)
        {
            perror("Error in receive_list - select statement");
            exit(EXIT_FAILURE);
        }
        // Check that socket has received new packet.
        if (FD_ISSET(server_fd, &list_temp))
        {
            bzero(pkt, PACKET_SIZE); // Clear buffer for writing
            recv(server_fd, pkt, buffer_len, 0);
            comm = (Command)(ntohl(pktCopy->command)); // Converting back to enum after receipt
            if (comm == REFRESH)
            { // REFRESH indicates that client list item was sent
                // Copy values of transmitted list item and push onto Client List
                connectedClientList.push_back(Client(
                    ntohs(pktCopy->data.connectedClientInfo.list_id),
                    pktCopy->data.connectedClientInfo.host_ip_addr,  
                    ntohs(pktCopy->data.connectedClientInfo.host_port_number),
                    pktCopy->data.connectedClientInfo.hostname));
            }
        }
    }
}

// Adapted from client.c of the posted demo code on pizza
bool ChatClient::connectToServer(char *ipAddr, char *port) {
    struct addrinfo hints, *res;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(ipAddr, port, &hints, &res) != 0) {
        perror("getaddrinfo failed");
        return false;
    }

    server_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (server_fd < 0){
        server_fd = -1;
        perror("Failed to create socket");
        return false;
    }

    if (connect(server_fd, res->ai_addr, res->ai_addrlen) < 0) {
        server_fd = -1;
        perror("Connect failed");
        return false;
    }

    FD_SET(server_fd, &rfds); // Add stream to set of file descriptors
    freeaddrinfo(res);

    return true;
}

void ChatClient::print_list(const char *command_str) {
    size_t len = connectedClientList.size(); // Get size of list
    cse4589_print_and_log("[%s:SUCCESS]\n", command_str);

    for (int i = 0; i < len; i++)
    {
        cse4589_print_and_log("%-5d%-35s%-20s%-8d\n", connectedClientList[i].list_id, connectedClientList[i].hostname, connectedClientList[i].ip_addr, connectedClientList[i].port_num);
    }

    cse4589_print_and_log("[%s:END]\n", command_str);
}

/**
 * @brief Sends command to server to receive updated connected clients list, receives list as well
 * 
 */
void ChatClient::refresh(){
    Packet *pkt = make_packet(REFRESH);
    ssize_t bytes_sent = ChatApp::send_packet(server_fd,pkt);

    receive_list();
    cse4589_print_and_log("[%s:SUCCESS]\n", "REFRESH");
    cse4589_print_and_log("[%s:END]\n", "REFRESH");
}


/**
 * @brief Sends an exit message to the server, closes the server's file descriptor,
 *        and exits with code 0
 * 
 */
void ChatClient::clientExit() {
    Packet *pkt = make_packet(EXIT);
    ssize_t bytes_sent = ChatApp::send_packet(server_fd,pkt);
    close(server_fd);
    exit(0);
}

/**
 * @brief Confirms that exit was received by the server.
 * 
 * Currently Unused!
 * 
 * @return true: Server received and processed the exit.
 * @return false: Server did not receive the request.
 */
bool ChatClient::confirmExit() {
    Incoming_Streams rfds, fd_t;
    FD_ZERO(&rfds);
    FD_SET(server_fd, &rfds);

    char buffer_t[PACKET_SIZE];
    bzero(buffer_t, PACKET_SIZE);
    Command *buffer = (Command*)buffer_t;
    size_t read_len = sizeof(Command);

    bool waiting_on_response;
    while (waiting_on_response) {
        fd_t = rfds;

        if (select(FD_SETSIZE, &fd_t, NULL, NULL, NULL) < 0) {
            perror("Error in ChatClient - confirm_exit - select statement");
            exit(EXIT_FAILURE);
        }

        if (FD_ISSET(server_fd, &fd_t)) {
            recv(server_fd, buffer_t, read_len, 0);

            Command c = (Command)ntohl(*buffer);
            if (c == EXIT){
                return true;
            }
        }

    }

    return false;

}

/**
 * @brief Constructs a CommandPacket with the correct data filled in for the type of command passed as param
 * 
 * @param cmd 
 * @return CommandPacket* 
 */
Packet *ChatClient::make_packet(Command cmd) {
    /*
        NOTE: Need to cast Command cmd using htonl() when assigning pkt->command.
    */
    Packet *pkt = new Packet;
    switch(cmd) {
        case EXIT:
            pkt->command = htonl(cmd);
            break;
        case LOGIN:
            pkt->command = htonl(cmd);
            strncpy(pkt->data.login_data.client_ip_addr,machine_info.ip_addr,INET_ADDRSTRLEN);
            strncpy(pkt->data.login_data.client_hostname,machine_info.hostname, HOST_NAME_LEN);
            pkt->data.login_data.client_port_number = machine_info.port_num;
            break;
        case REFRESH:
            pkt->command = htonl(cmd);
            break;
    }   
    return pkt;
}

void ChatClient::commandError(Command cmd) {
    cse4589_print_and_log("[%d:ERROR]\n", cmd);
    cse4589_print_and_log("[%d:END]\n", cmd);
}


void ChatClient::sendMsg(Packet *pkt) {
    ssize_t bytes_sent = ChatApp::send_packet(server_fd, pkt);
    cse4589_print_and_log("[%s:SUCCESS]\n", "SEND");
    cse4589_print_and_log("[%s:END]\n", "SEND");
}

void ChatClient::broadcastMsg(Packet *pkt) {
    ssize_t bytes_sent = ChatApp::send_packet(server_fd, pkt);
    cse4589_print_and_log("[%s:SUCCESS]\n", "BROADCAST");
    cse4589_print_and_log("[%s:END]\n", "BROADCAST");
}