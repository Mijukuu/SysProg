//This program is a client for the server 
//created as server.c.
//Once connected, the client can send
//messages to the server and the server
//will respond with "Message received".

#include<stdio.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<sys/un.h>
#include<string.h>
#include<unistd.h>
#include<signal.h>
#include<stdlib.h>
#include<errno.h>

#define MAXLEN 128

// Added a function handler incase the server just crashes.
void handle_server_disconnect(int signum) {
    printf("\nServer connection lost unexpectedly!\n");
    exit(1);
}

int main(void) {
    //variables
    const char sockname[] = "/tmp/socket1";
    int fd;
    struct sockaddr_un addr;
    char sendbuffer[MAXLEN];
    char recvbuffer[MAXLEN];

    // Set up signal handler for server shut down (broken pipe) 
    signal(SIGPIPE, handle_server_disconnect);

    //create socket file descriptor
    fd = socket(AF_UNIX, SOCK_SEQPACKET, 0);

    if (fd == -1) {
        perror("Create socket failed");
        return 1;
    }

    //set address family and socket path
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, sockname);

    //use connect() to connect to the server
    //notice the connect() uses the same arguments
    //as the bind() function
    if ((connect(fd, (const struct sockaddr*) &addr,
        sizeof(struct sockaddr_un))) == -1) {
            perror("Can't connect to the server");
            fprintf(stderr, "Is the server down?\n");
            return 1;
    }

    // Get initial welcome message from server, can be seen in server.c
    if ((read(fd, recvbuffer, MAXLEN)) == -1) {
        perror("Cannot read server welcome");
        return 1;
    }
    printf("%s\n", recvbuffer); // Prints welcome message to client with PID.
    
    //main loop
    while (1) {
        //send message to the server
        printf("Message to send: ");
        if (fgets(sendbuffer, sizeof(sendbuffer), stdin) == NULL) { 
            break;
        }
        
        //convert the newline character to null terminator
        sendbuffer[strcspn(sendbuffer, "\n")] = '\0';

        //Checks if the client used typed quit
        if (strcmp(sendbuffer, "quit") == 0) {
             printf("Disconnecting from server...\n");
             break;
        }
        
        //write the buffer to the file descriptor
        if ((write(fd, sendbuffer,
            strlen(sendbuffer) + 1)) == -1) {
                //checks if server crashed/ended.
                if (errno == EPIPE) {
                    printf("Server connection lost!\n");
                    break;
                }
                perror("Could not write");
                break;
        }

        //read response from server 
        if ((read(fd, recvbuffer, MAXLEN)) == -1) {
            perror("Cannot read from server");
            break;
        }
        else if ((read(fd, recvbuffer, MAXLEN)) == 0) {
            printf("Server closed the connection.\n");
            break;
        }

        printf("Server: %s\n", recvbuffer);
    }//end while
    close(fd);
    return 0;
}
