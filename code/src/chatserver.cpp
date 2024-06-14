#include "../include/chatserver.h"
/**
 * @brief 
 * 
 * @param port
 * @return int 
 * 
 * TODO:
 * [x] Determine if the printf statements are okay
 * [x] Determine if the perror statements are okay
 * [] Add in handling for console input *SEE OTHER COMMENTS*
 * [] Add in support for client request *SEE OTHER COMMENTS*
 * [] Maybe need to add something to support multiple clients 
 */
int ChatServer::run(char *port){
    //Adapted from server.c of the posted demo code on pizza
    int head_socket, selret, sock_index, fdaccept = 0, caddr_len;
    struct sockaddr_in client_addr;
    struct addrinfo hints, *res;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if(getaddrinfo(NULL,port,&hints,&res)!= 0){
        perror("getaddrinfo failed");
    }

    listen_fd = socket(res->ai_family,res->ai_socktype,res->ai_protocol);
    if(listen_fd < 0){
        perror("Cannot create socket");
    }

    if(bind(listen_fd,res->ai_addr,res->ai_addrlen) < 0){
        perror("Bind failed");
    }

    freeaddrinfo(res);

    if (listen(listen_fd,BACKLOG) == -1)
    {
        perror("Unable to listen on port");
        exit(1);
    }

    Incoming_Streams watch_list;
    FD_ZERO(&master_list);

    FD_SET(STDIN,&master_list);
    FD_SET(listen_fd, &master_list);

    fflush(stdin);
    while(true) {
        watch_list = master_list;


        // Get streams to be read from
        if (select(FD_SETSIZE, &watch_list, NULL, NULL, NULL) < 0) {
            perror("Error in chat server - select statement");
            exit(EXIT_FAILURE);
        }


        for (int i = 0; i < FD_SETSIZE; i++){
            
            if (FD_ISSET(i, &watch_list)) { 

                if (i == STDIN) {         // User input
                    Packet *user_input = process_user_input(true);
                    switch (user_input->command){
                        case AUTHOR:
                            // Does not require a client to be logged-in to server to use
                            author(user_input->data.message_buffer);
                            break;
                        case IP:
                            // Does not require a client to be logged-in to server to use
                            display_ip(user_input->data.message_buffer);
                            break;
                        case PORT:
                            // Does not require a client to be logged-in to server to use
                            print_port(user_input->data.message_buffer);
                            break;
                        case LIST:
                            // Does not require a client to be logged-in to server to use
                            print_list(user_input->data.message_buffer);
                            break;
                        case EXIT:
                            // Does not require a client to be logged-in to server to use
                            clientExited(i);
                            break;
                        default:
                            cse4589_print_and_log("[%s:ERROR]\n", user_input->data.message_buffer);
                            cse4589_print_and_log("[%s:END]\n", user_input->data.message_buffer);
                            break;
                    }
                    fflush(stdin);
                } else if (i == listen_fd) {        // New connection request.
                    printf("Login requested\n");
                    int new_fd = accept_login();    // Get connection request                    
                    printf("Connection established\n");
                    FD_SET(new_fd, &master_list);   // Add new connection to poll list
                    get_new_client_info(new_fd);    // Await incoming login packet
                    printf("Received new client info\n");
                    send_connected_client_list(new_fd); // Sends data to client
                    printf("Sent list contents.\n");
                }
                else {
                    Packet *recvdPkt = receivePacket(i);
                    Command command = (Command)ntohl(recvdPkt->command);
                    switch (command) {
                        case REFRESH:
                            printf("received refresh command");
                            send_connected_client_list(i); // Sends data to client
                            break;
                        case SEND:
                            forwardMessage(recvdPkt);
                            break;
                        case EXIT:
                            clientExited(i);
                            break;
                   }
                }

            }
        }
    
    }
    return 0;
}

int ChatServer::accept_login() {
    sockaddr_storage their_addr;
    socklen_t addr_size = sizeof their_addr;
    int new_fd = accept(listen_fd, (sockaddr *)&their_addr, &addr_size);
    return new_fd;
}

/**
 * @brief Blocks until the stream is ready for writing.
 * 
 * @param fd stream we will be checking.
 */
void ChatServer::block_until_fd_is_ready_to_write(int fd) {
    fd_set wfds, fd_t;                              // Declare sets
    FD_ZERO(&fd_t);                                 
    FD_SET(fd, &fd_t);                              // Create set
    while (true) {                                  // Block until writeable.
        wfds = fd_t;
        if (select(FD_SETSIZE, NULL, &wfds, NULL, NULL) < 0)
        {
            perror("Error in chat client - select statement");
            exit(EXIT_FAILURE);
        }
        if (FD_ISSET(fd, &wfds)) {
            return;
        }
    }

}


void ChatServer::get_new_client_info(int fd) {
    Incoming_Streams local_list, watch_list;       // One item set
    FD_ZERO(&local_list);
    FD_SET(fd, &local_list);
    
    char pkt[PACKET_SIZE];                      
    bzero(pkt, PACKET_SIZE);

    bool waiting_on_login_info = true;      
    while (waiting_on_login_info) {
        watch_list = local_list;
        // Get streams to be read from
        if (select(FD_SETSIZE, &watch_list, NULL, NULL, NULL) < 0) {
            perror("Error in get_new_client_info - select statement");
            exit(EXIT_FAILURE);
        }

        if (FD_ISSET(fd, &watch_list)) {
            recv(fd, pkt, PACKET_SIZE, 0);
            Packet *pktCopy = (Packet*)pkt;
            Command comm = (Command)(ntohl(pktCopy->command));
            
            if (comm != LOGIN) {  // Eliminate bad info received.
                perror("Error in get_new_client_info - comm != LOGIN\n");
                printf("Command: %d", comm);
                exit(EXIT_FAILURE);
            }

            // Add info to server's clientList
            clientList.push_back(ClientListEntry(
                0,
                pktCopy->data.login_data.client_ip_addr,
                ntohs(pktCopy->data.login_data.client_port_number),
                pktCopy->data.login_data.client_hostname,
                fd
            ));

            waiting_on_login_info = false;          // Sentinel removed
        }

    }
    sort_client_list();                             // Sort the client list, update values
}

void ChatServer::sort_client_list() {
    // Use STL sort
    std::sort(clientList.begin(), clientList.end());

    // Re-number client list_ids
    int len = clientList.size();
    for (int i = 0; i < len; i++) {
        clientList[i].client.list_id = i+1;
    }

}

// Sends data to client
void ChatServer::send_connected_client_list(int fd) {
    int list_len = clientList.size();
    for (int i = 0; i < list_len; i++) {
        Packet *pkt = make_packet(REFRESH, &clientList[i]); // Make packet containing client_list_entry information
        ssize_t bytes_sent = ChatApp::send_packet(fd,pkt); // Sending packet
    } 

    // Notify client that list has been fully sent.
    Packet *pkt = make_packet(ENDLIST,NULL);
    ssize_t bytes_sent = ChatApp::send_packet(fd,pkt);
}   

void ChatServer::print_list(const char *command_str) {
    size_t len = clientList.size();                // Get size of list
    cse4589_print_and_log("[%s:SUCCESS]\n", "LIST");
    
    for (int i = 0; i < len; i++) {
        cse4589_print_and_log(
            "%-5d%-35s%-20s%-8d\n"
            , clientList[i].client.list_id
            , clientList[i].client.hostname
            , clientList[i].client.ip_addr
            , clientList[i].client.port_num
        );
    }
        
    cse4589_print_and_log("[%s:END]\n", "LIST");

}

void ChatServer::clientExited(int clientStream){
    /*
        What we need to do here:
            [x] Remove the listing of of the client.
            [x] Re-sort the list
            [] ...
    */
    for(int i = 0; i < clientList.size(); i++){
        if(clientList[i].client_stream == clientStream){
            clientList.erase(clientList.begin()+i);
            FD_CLR(clientStream,&master_list);
        }
    }
    sort_client_list();
}

Packet *ChatServer::make_packet(Command cmd, ClientListEntry *entry) {
     /*
        NOTE: Need to cast Command cmd using htonl() when assigning pkt->command.
    */
    Packet *pkt = new Packet;
    switch(cmd){
        case REFRESH:
            pkt->command = htonl(cmd);
            pkt->data.connectedClientInfo.list_id = htons(entry->client.list_id);
            pkt->data.connectedClientInfo.host_port_number = htons(entry->client.port_num);
            strncpy(pkt->data.connectedClientInfo.host_ip_addr,entry->client.ip_addr,INET_ADDRSTRLEN);
            strncpy(pkt->data.connectedClientInfo.hostname,entry->client.hostname,HOST_NAME_LEN);
            break;
        case ENDLIST:
            pkt->command =htonl(cmd);
            break;
    }
    return pkt;
}

void ChatServer::forwardMessage(Packet *pkt){
    // TODO: Add checking fro blocked clients
    ClientListEntry *dest = findClient(pkt->data.clientMessage.destinationIP);
    // Check if we found the destination, if so send packet and report success. Otherwise, report failure.
    if(dest != NULL){
        ssize_t bytes_sent = send_packet(dest->client_stream,pkt);
        cse4589_print_and_log("[%s:SUCCESS]\n", "RELAYED");
        cse4589_print_and_log("msg from:%s, to:%s\n[msg]:%s\n",pkt->data.clientMessage.sourceIP,pkt->data.clientMessage.destinationIP,pkt->data.clientMessage.message);
        cse4589_print_and_log("[%s:END]\n", "RELAYED");
    } else {
        cse4589_print_and_log("[%s:ERROR]\n", "RELAYED");
        cse4589_print_and_log("[%s:END]\n", "RELAYED");
    } 
}

void ChatServer::broadcast(Packet *pkt){
    // TODO: Add checking fro blocked clients
    int len = clientList.size();
    // Send to all the clients that are not the source of the message
    for(int i = 0;i < len; i++){
        if(strcmp(pkt->data.clientMessage.sourceIP,clientList[i].client.ip_addr) == 0){
            continue;
        } else {
            send_packet(clientList[i].client_stream,pkt);
        }
    }
    // Log completion
    cse4589_print_and_log("[%s:SUCCESS]\n", "RELAYED");
    cse4589_print_and_log("msg from:%s, to:%s\n[msg]:%s\n",pkt->data.clientMessage.sourceIP,pkt->data.clientMessage.destinationIP,pkt->data.clientMessage.message);
    cse4589_print_and_log("[%s:END]\n", "RELAYED");
}

ClientListEntry *ChatServer::findClient(char *ip) {
    int len = clientList.size();
    for(int i = 0; i < len; i++){
        if(strcmp(ip,clientList[i].client.ip_addr)== 0){
            return &clientList[i];
        } else continue;
    }

    return NULL;
}