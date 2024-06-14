#include "../include/chatapp.h"

ChatApp::ChatApp(char *port) {
    memset(&machine_info.port_string, 0, 6);
    memset(&machine_info.hostname, 0, 35);
    memset(&machine_info.ip_addr, 0, INET_ADDRSTRLEN);
    
    strcpy(machine_info.port_string, port);
    machine_info.port_num = atoi(port);
    
    // Reference: https://beej.us/guide/bgnet/html/#connect
    // 10/12/22
    addrinfo hints, *res;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    
    getaddrinfo("8.8.8.8", "53", &hints, &res);  // google

    int info_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (info_socket < 0) {
        perror("Error in ChatApp constructor, create socket.");
        exit(EXIT_FAILURE);
    }
    
    if (connect(info_socket, res->ai_addr, res->ai_addrlen) < 0) {
        perror("Error in ChatApp constructor, connect.");
        exit(EXIT_FAILURE);
    }
    
    // Resource: https://gist.github.com/listnukira/4045436
    // Resource: https://stackoverflow.com/questions/25906593/getting-my-own-ip-address-using-getsockname
    // Accessed on: 10/2/22
    char hostname[35];                          // String size in list string
    gethostname(hostname, sizeof hostname);     
    strcpy(machine_info.hostname, hostname);
    // machine_info.hostname = hostname;            // Save to internal variable
    
    char ip[INET_ADDRSTRLEN];
    sockaddr_in my_addr;                     // Create a dummy socket to get IP address
    bzero(&my_addr, sizeof(my_addr));  
    socklen_t len = sizeof(my_addr);          // getsockname requires pointer to socklen_t
    getsockname(info_socket, (sockaddr *)&my_addr, &len);
    
    inet_ntop(AF_INET, &my_addr.sin_addr, ip, 16);
    
    freeaddrinfo(res);

    strcpy(machine_info.ip_addr, ip);

    // Defining constants to avoid one definition rule
    strcpy(STATUS_OUT, "logged-out");
    strcpy(STATUS_IN, "logged-in");

}
    

Command ChatApp::string_to_command(std::string stoc) {
    if (stoc == "AUTHOR") {
        // Server & Client Command
        // Stage 1 & 2
        return AUTHOR;
    } else if (stoc == "IP") {
        // Server & Client Command
        // Stage 1
        return IP;
    } else if (stoc == "PORT") {
        // Server & Client Command
        // Stage 1
        return PORT;
    } else if (stoc == "LIST") {
        // Server & Client Command
        // Stage 1
        return LIST;
    } else if (stoc == "STATISTICS") {
        // Server Command
        // Stage 2
        return STATISTICS;
    } else if (stoc == "BLOCKED") {
        // Server Command
        // Stage 2
        return BLOCKED;
    } else if (stoc == "EVENT") {
        // Server & Client Command
        // Stage 2
        return EVENT;
    } else if (stoc == "LOGIN"){
        // Client Command
        // Stage 1 & 2
        return LOGIN;
    } else if (stoc == "REFRESH"){
        // Client Command
        // Stage 1
        return REFRESH;
    } else if (stoc == "SEND"){
        // Client Command
        // Stage 2
        return SEND;
    } else if (stoc == "BROADCAST"){
        // Client Command
        // Stage 2
        return BROADCAST;
    } else if (stoc == "BLOCK"){
        // Client Command
        // Stage 2
        return BLOCK;
    } else if (stoc == "UNBLOCK"){
        // Client Command
        // Stage 2
        return UNBLOCK;
    } else if (stoc == "LOGOUT"){
        // Client Command
        // Stage 2
        return LOGOUT;
    } else if (stoc == "EXIT"){
        // Client Command
        // Stage 1
        return EXIT;
    } else if (stoc == "ENDLIST") {
        // Used to signal that has been received in full.
        return ENDLIST;
    } else {
        return ERROR;
    }
}

// EXPLORE THE POSSIBILITY OF USING A STRINGSTREAM AND GETLINE
Packet* ChatApp::process_user_input(bool server) {
    Packet *pkt = new Packet;
    std::string cmdString, ip_addr, port_no;
    Command cmd;
    std::string clientIP, message;

    std::cin >> cmdString;
    cmd = string_to_command(cmdString);
    pkt->command = cmd;
    const char* command_str = cmdString.c_str();

    switch (cmd) {
        case ERROR:
            strcpy(pkt->data.message_buffer, command_str);
            break;
        case AUTHOR:
            strcpy(pkt->data.message_buffer, command_str);
            break;
        case IP:
            strcpy(pkt->data.message_buffer, command_str);
            break;
        case PORT:
            strcpy(pkt->data.message_buffer, command_str);
            break;
        case LIST:
            strcpy(pkt->data.message_buffer, command_str);
            break;
        case LOGIN:
            if (server) {
                strcpy(pkt->data.message_buffer, command_str);
            } else {    
                std::cin >> ip_addr;
                std::cin >> port_no;
                strcpy(pkt->data.login_args.server_ip_addr, ip_addr.c_str());
                strcpy(pkt->data.login_args.server_port_number, port_no.c_str());
            }
            break;
        case REFRESH:
            strcpy(pkt->data.message_buffer, command_str);
            break;
        case EXIT:
            strcpy(pkt->data.message_buffer, command_str);
            break;
        case SEND:
            std::cin >> clientIP;
            std::cin >> message;
            strcpy(pkt->data.clientMessage.destinationIP , clientIP.c_str());
            strcpy(pkt->data.clientMessage.sourceIP,machine_info.ip_addr);
            strcpy(pkt->data.clientMessage.message,message.c_str());
            break;
        case BROADCAST:
            std::cin >> message;
            strcpy(pkt->data.clientMessage.sourceIP,machine_info.ip_addr);
            strcpy(pkt->data.clientMessage.destinationIP,"255.255.255.255");
            strcpy(pkt->data.clientMessage.message,message.c_str());
            break;
        default:
            perror("Something went really wrong. Process user input failed.");
    }
    return pkt;
}

Packet *ChatApp::receivePacket(int fd) {
    Incoming_Streams rfds, fd_t;
    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);

    Packet *pkt = new Packet;
    bzero(pkt, PACKET_SIZE);
    //Command *buffer = (Command*)buffer_t;
    size_t read_len = sizeof(PACKET_SIZE);
    recv(fd, pkt, read_len, 0);

    return pkt;
}

/*
    Printing Template:
         cse4589_print_and_log("[%s:SUCCESS]\n", command_str);
         cse4589_print_and_log();
         cse4589_print_and_log("[%s:END]\n", command_str);
    
    To use in a function fill in the empty cse4589_print_log() with
    the appropriate format string and any needed variables for the
    format string.
*/
void ChatApp::author(const char* command_str) {
    cse4589_print_and_log("[%s:SUCCESS]\n", "AUTHOR");
    cse4589_print_and_log("I, %s, have read and understood the course academic integrity policy.\n",UBIT);
    cse4589_print_and_log("[%s:END]\n", "AUTHOR");
}

void ChatApp::display_ip(const char* command_str){
    cse4589_print_and_log("[%s:SUCCESS]\n", "IP");
    cse4589_print_and_log("IP:%s\n",machine_info.ip_addr);
    cse4589_print_and_log("[%s:END]\n", "IP");
}

void ChatApp::print_port(const char* command_str){
    cse4589_print_and_log("[%s:SUCCESS]\n", "PORT");
    cse4589_print_and_log("PORT:%d\n",machine_info.port_num);
    cse4589_print_and_log("[%s:END]\n", "PORT");
}

void ChatApp::print_error(const char* command_str) {
    cse4589_print_and_log("[%s:ERROR]\n", command_str);
    cse4589_print_and_log("[%s:END]\n", command_str);
}

ssize_t ChatApp::send_packet(Server_FD fd, Packet *pkt){
    return send(fd,pkt,PACKET_SIZE,0);
}