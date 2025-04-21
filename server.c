//This program is a modified server.c file from Canvas. 
//It has been modified to handle multiple clients.
//It will receive messages from a client and respond with
//"Message received" every time a message is received.
//When the server or client exits, cleanup will execute.

// Include files to be ran through preprocess...
#include<stdio.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<sys/un.h>
#include<string.h>
#include<unistd.h>
#include<signal.h>
#include<stdlib.h>
#include<errno.h>

// Global Max Length of client messages
#define MAXLEN 128      //maximum length of message

void cleanup(int); // Prototype of cleanup function
void handle_client(int client_fd); // Prototype to handle each client

// Designates the destination of our server
const char sockname[] = "/tmp/socket1";
int connfd; // Connection  file descriptor, or listener. 
// Please note that the Data file descriptor was deleted. Children will handle socketing.

int main(void) {
   struct sockaddr_un addr;     //contains the socket type and file path
   struct sigaction action;     
   // Moved Ret/Buffer to the client handler.

   //prepare for sigaction
   action.sa_handler = cleanup;
   sigfillset(&action.sa_mask);
   action.sa_flags = SA_RESTART;

   //register all signals to handle
   sigaction(SIGTERM, &action, NULL);
   sigaction(SIGINT, &action, NULL);
   sigaction(SIGQUIT, &action, NULL);
   sigaction(SIGABRT, &action, NULL);
   sigaction(SIGPIPE, &action, NULL);
   sigaction(SIGCHLD, &action, NULL); // Added to catch the caught SIGCHLD signal from clients.

   //create socket file descriptor
   connfd = socket(AF_UNIX, SOCK_SEQPACKET, 0);

   if(connfd == -1) {
      perror("Create socket failed");
      return 1;
   }

   //set address family and socket path
   addr.sun_family = AF_UNIX;
   strcpy(addr.sun_path, sockname);

   //bind the socket
   //cast sockaddr_un to sockaddr
   if((bind(connfd, (const struct sockaddr*) &addr,
      sizeof(struct sockaddr_un))) == -1) {
         perror("Binding socket failed");
         return 1;
   }

   //prepare for accepting connections
   //Arguments: 1) socket file descriptor
   //2) buffer size for backlog
   if((listen(connfd, 20)) == -1) {
      perror("Listen error");
      return 1;
   }
   
   //Socket is listening...
   printf("Server started. Waiting for clients...\n");
   
   //main loop, forks each time a client connects.
   while (1) {         
       int client_fd;   //socket for the client
       client_fd = accept(connfd, NULL, NULL); // Waits for client to connect and creates socket.
       
       // Check for failure...
       if (client_fd == -1) {
           perror("Error...client could not connect to server.");
           continue;
       }
       
       // Uses fork to create new process for each client
       pid_t pid = fork();
       
       // Standard check for fork success
       if (pid < 0) {
           perror("Error in creating fork process...");
           close(client_fd);
           continue;  
       }
       else if (pid == 0) {
           // Child process
           close(connfd); // Child should be connected to the client, can close this listener.
           handle_client(client_fd); // Uses our handler to handle messsages.
           exit(0); // Once finished, exits.
       }
       else {
           // Parent process
           close(client_fd); // Parent does not need the client socket.
           printf("New client connected (PID: %d)\n", pid);
       }    
   } 
   return 0;
}

//A client handler function is logically helpful here.
//Buffer and ret utilized in this function 
void handle_client(int client_fd) {
    char buffer[MAXLEN]; 
    char welcome[MAXLEN];
    
    // Send Welcome message with PID
    snprintf(welcome, MAXLEN, "Connected to server! Your handler PID: %d", getpid());
    if (write(client_fd, welcome, strlen(welcome) + 1) == -1) { 
        perror("Error sending welcome message");
        close(client_fd);
        return;
    }
  
    while (1) {
        int ret = read(client_fd, buffer, MAXLEN);
        
        if (ret == -1) { // if the buffer couldn't be read
            perror("Error reading from client");
            break;
        }
        else if (ret == 0) { // Ret zero is only if the socket ends/client exits program.
            printf("Client disconnected (PID: %d)\n", getpid());
            break;
        }
        // If we passed the above, then we have a message to read from buffer
        printf("Message from client (PID: %d): %s\n", getpid(), buffer);
        
        if (write(client_fd, "Message received\n", 18) == -1) { // Lets the client know that we received a message.
            perror("Error writing 'Message received' to client");
            break;
        }
    }
    close(client_fd); // Housekeeping to prevent resource waste or socket issues.        
}
    
//cleanup function
void cleanup(int signum) {
   if (signum == SIGCHLD) { 
       // Zombie check and cleanup
       while (waitpid(-1, NULL, WNOHANG) > 0);
       return;
   } 
   
   printf("Quitting and cleaning up\n");
   close(connfd);
   unlink(sockname);
   exit(0);
}

