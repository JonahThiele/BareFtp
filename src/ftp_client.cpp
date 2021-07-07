#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x501

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <fstream>
#include <ios>


// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")


#define DEFAULT_BUFLEN 200
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
int __cdecl main(int argc, char **argv) 
{
    WSADATA wsaData;
    SOCKET ConnectSocket = INVALID_SOCKET;
    struct addrinfo *result = NULL,
                    *ptr = NULL,
                    hints;
    char recvbuf[DEFAULT_BUFLEN];
    int iResult;
    int recvbuflen = DEFAULT_BUFLEN;
    
    // get input to grab or get from server
    bool state_revc;
    std::string operation;
    std::string file;
    std::string str_addr;
    std::cout << "address:";
    std::cin >> str_addr;
    char* c_addr = const_cast<char*>(str_addr.c_str());
    std::cout << "\noperation:";
    std::cin >> operation;
    std::cout << "\nfile:";
    std::cin >> file;
    std::string operationFile = operation + file;
    char sendbuf[operationFile.length() + 1] = {'\0'}; // sets the file name to less than 36 chars
    strcpy(sendbuf, operationFile.c_str());
    printf("OperationFile:%s", sendbuf);
    if (operation == "receive"){
        state_revc = true;
    }
    else if (operation == "send"){
        state_revc = false;
    }

    // Validate the parameters
    /*if (argc != 2) {
        printf("usage: %s server-name\n", argv[0]);
        return 1;
    }*/

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    ZeroMemory( &hints, sizeof(hints) );
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Resolve the server address and port
    iResult = getaddrinfo(c_addr, DEFAULT_PORT, &hints, &result);
    if ( iResult != 0 ) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Attempt to connect to an address until one succeeds
    for(ptr=result; ptr != NULL ;ptr=ptr->ai_next) {

        // Create a SOCKET for connecting to server
        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, 
            ptr->ai_protocol);
        if (ConnectSocket == INVALID_SOCKET) {
            printf("socket failed with error: %ld\n", WSAGetLastError());
            WSACleanup();
            return 1;
        }

        // Connect to server.
        iResult = connect( ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            closesocket(ConnectSocket);
            ConnectSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if (ConnectSocket == INVALID_SOCKET) {
        printf("Unable to connect to server!\n");
        WSACleanup();
        return 1;
    }
    if(state_revc){
        iResult = send( ConnectSocket, sendbuf, (int)strlen(sendbuf), 0 );
        recv_file(ConnectSocket, recvbuf, iResult, file);
    }
    else{
        iResult = send( ConnectSocket, sendbuf, (int)strlen(sendbuf), 0 );
        send_file(file, iResult, ConnectSocket);
    }
    
    
    // cleanup
    closesocket(ConnectSocket);
    WSACleanup();

    return 0;
}