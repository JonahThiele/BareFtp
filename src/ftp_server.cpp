#undef UNICODE

#define _WIN32_WINNT 0x501
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>


// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
// #pragma comment (lib, "Mswsock.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"

void recv_file(SOCKET ConnectSocket, char recvbuf[DEFAULT_BUFLEN], int iResult, std::string filename)
{  
   std::cout << "Filename:" << filename << std::endl;
   std::ofstream ifile;
   ifile.open(filename, std::ofstream::out | std::ofstream::trunc);
   if(ifile) {
      printf("rewriting file\n");
      ifile.close();
      ifile.open(filename, std::ofstream::app);
   } 

   do {
        iResult = recv(ConnectSocket, recvbuf, DEFAULT_BUFLEN, 0);
        // extract the new bytes
        std::string in_bytes;
        printf("Recvbuf:%s", recvbuf);
        for( int i = 0; i < iResult; i++){
            if(recvbuf[i] != '\n'){
                in_bytes += recvbuf[i];
            }
            else if(recvbuf[i] == '\n'){
                std::cout << "in_bytes:" << in_bytes << std::endl;
                in_bytes += '\n';
                ifile << in_bytes;
                in_bytes.clear();
            }
        }

        if ( iResult > 0 )
            printf("Bytes received: %d\n", iResult);
        else if ( iResult == 0 )
            printf("Connection closed\n");
        else
            printf("recv failed with error: %d\n", WSAGetLastError());

    } while( iResult > 0 );  

}
void send_file(std::string filename, int iResult, SOCKET ConnectSocket)
{
    std::ifstream read_file;
    int number_of_lines;
    read_file.open(filename);
    std::string line;
    while (std::getline(read_file, line)){
        line += '\n';
        std::cout << "line:" << line << std::endl;
        char* c = const_cast<char*>(line.c_str());
        printf("Line Change:%s", c);
        iResult = send( ConnectSocket, c, (int)strlen(c), 0 );
        if (iResult == SOCKET_ERROR) {
            printf("send failed with error: %d\n", WSAGetLastError());
            closesocket(ConnectSocket);
            WSACleanup();
        }
    }

}

int __cdecl main(void) 
{
    WSADATA wsaData;
    int iResult;

    SOCKET ListenSocket = INVALID_SOCKET;
    SOCKET ClientSocket = INVALID_SOCKET;

    struct addrinfo *result = NULL;
    struct addrinfo hints;

    int iSendResult;
    char recvbuf[DEFAULT_BUFLEN] = {0};
    int recvbuflen = DEFAULT_BUFLEN;
    
    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the server address and port
    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    if ( iResult != 0 ) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Create a SOCKET for connecting to server
    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    // Setup the TCP listening socket
    iResult = bind( ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(result);

    iResult = listen(ListenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    // Accept a client socket
    ClientSocket = accept(ListenSocket, NULL, NULL);
    if (ClientSocket == INVALID_SOCKET) {
        printf("accept failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    // No longer need server socket
    closesocket(ListenSocket);

    // Receive until the peer shuts down the connection
    do {

        iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
        if (iResult > 0) {
            printf("Bytes received: %d\n", iResult);
            char operation_c[5] = { recvbuf[0], recvbuf[1], recvbuf[2], recvbuf[3]};
            printf("operation:%s\n", operation_c);
            std::string file;
            int obj_in_buf = sizeof(recvbuf) / sizeof(recvbuf[0]);
            for( int i = 4; i < obj_in_buf - 1; i++){
                if(recvbuf[i] != 0 ){
                    file += recvbuf[i];
                }else{
                  break; 
                }
            }
            std::string operation(operation_c);
            if(operation == "send"){
                recv_file(ClientSocket, recvbuf, iResult, file);
            }
            else if(operation == "recv"){
                send_file(file, iResult, ClientSocket);
            }

        }
        else if (iResult == 0)
            printf("Connection closing...\n");
        else  {
            printf("recv failed with error: %d\n", WSAGetLastError());
            closesocket(ClientSocket);
            WSACleanup();
            return 1;
        }

    } while (iResult > 0);

    // shutdown the connection since we're done
    iResult = shutdown(ClientSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(ClientSocket);
        WSACleanup();
        return 1;
    }

    // cleanup
    closesocket(ClientSocket);
    WSACleanup();

    return 0;
}