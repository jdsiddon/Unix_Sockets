/* A simple server in the internet domain using TCP
   The port number is passed as an argument */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>       // Required for sockets to work.
#include <netinet/in.h>       // Required for internet domain access.

void error(const char *msg);  // Error function prototype.

// Server style socket configurations based off example at: http://www.linuxhowtos.org/C_C++/socket.htm
int main(int argc, char *argv[]) {
  int sockfd;               // Socket file descriptor.
  int newsockfd;            // Socket file descriptor for use once socket it bound to a port.
  int portno;               // Port we are accepting connections on.
  int pid;                  // PID of child processes.

  socklen_t clilen;         // Size of clients address, required by 'accept()'.
  char buffer[1000];         // Data buffer to read incoming messages into.

  struct sockaddr_in serv_addr;         // Address of the server (here).
  struct sockaddr_in cli_addr;          // Address of the client (who connects).

  int n;                    // Integer to hold number of characters read or write returns.
  int status;               // Status variable to communicate child process status.

  int portCounter = 0;     // Integer to increment port number by as clients connect.

  // Begin socket setup.

  // Check user passed in a port to set socket up on.
  if(argc < 2) {
    fprintf(stderr, "ERROR, no port provided\n");
    fflush(stderr);
    exit(1);
  }

  // Create new socket.
  sockfd = socket(AF_INET, SOCK_STREAM, 0);     // Socket uses unix domain, stream type socket, with TCP protocol.
  if(sockfd < 0)                                // -1 means socket created errored out.
    error("ERROR opening socket");

  bzero((char *) &serv_addr, sizeof(serv_addr));  // Set serv_addr struct to all 0's.

  portno = atoi(argv[1]);                       // Convert user entered port number to integer.

  // Set up server address struct.
  serv_addr.sin_family = AF_INET;               // Set to unix domain.
  serv_addr.sin_port = htons(portno);           // Set port number to network byte order.
  serv_addr.sin_addr.s_addr = INADDR_ANY;       // Set server address to it's own IP address. (INADDR_ANY returns current machines IP).

  // Bind socket to server address.
  if(bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    error("ERROR binding");

  listen(sockfd, 5);            // Listen on socket for connections.

  while(1) {                    // Loop forever waiting for connections.
    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);         // Accept connection, make new socket for connection.

    portCounter = portCounter + 1;  // Increment port increase variable, so they can communicate on new port #.

    if(newsockfd < 0)           // If newsockfd is less than 0, the accept call failed.
      error("ERROR on accept");

    // Connection was successful so fork a new process.
    pid = fork();
    if(pid < 0)
      error("ERROR forking process");

    if(pid == 0) {              // CHILD
      close(sockfd);            // Close the old socket.
      char childPort[50];        // Character array to hold port number child should talk on.
      int childsockfd;          // New socket.
      int newchildsockfd;       // New communication between child process and client. Sock bound to new port.

      childsockfd = socket(AF_INET, SOCK_STREAM, 0);     // Socket uses unix domain, stream type socket, with TCP protocol.

      sprintf(childPort, "%d", (portno + portCounter));   // Convert port integer to string and store it in childPort.

      // Set up new port to talk on, port is 5 characters.
      write(newsockfd, childPort, 5);

      // Copy old serv address to new one.
      struct sockaddr_in child_serv_addr;         // Address of the server (here).

      // Copy over master server address information to child.
      memcpy(&child_serv_addr, &serv_addr, sizeof(serv_addr));

      // Set port number of child.
      child_serv_addr.sin_port = htons((portno + portCounter));           // Set new port number.

      // Bind socket to child server address.
      if(bind(childsockfd, (struct sockaddr *) &child_serv_addr, sizeof(child_serv_addr)) < 0)
        error("ERROR binding");

      listen(childsockfd, 5);            // Listen on socket for connections.

      newchildsockfd = accept(childsockfd, (struct sockaddr *) &cli_addr, &clilen);         // Accept connection, make new socket for connection.
      if(newchildsockfd < 0)
        error("ERROR accepting connection from client");

      // First message read is to figure out the total message length that is coming in. (all of the streams).
      n = read(newchildsockfd, buffer, 999);
      printf("Total Message Length: %d", buffer[0]);
      fflush(stdout);
      if(n < 0)
        error("ERROR Reading from client");

      int totalMessLen = buffer[0];
      char *entireMessage = (char*) malloc(totalMessLen + 1);                   // Create space to store entire message.

      // Write back to client confiration, informs to send rest of data.
      printf("\n%d\n", totalMessLen);
      n = write(newchildsockfd, buffer, strlen(buffer));   // Write message to client.

      char buff[20];
      // Get all text from client.
      while(totalMessLen > 0) {
        n = read(newchildsockfd, buffer, 999);             // n will always be +1 to the actual string length since buff[0] is length of character string.
        printf("n: %d\n", n);
        printf("Message: %s\n", &buffer[1]);
        strcpy(buff, &buffer[1]);
        printf("buff: %s", buff);
        totalMessLen = totalMessLen - n;
      }


      // INSERT CALL TO CRYPT FUNCTION HERE.
      exit(0);
    } else {                    // PARENT
      waitpid(0, &status, WNOHANG);

      printf("Child exit: %d\n", WEXITSTATUS(status));
      fflush(stdout);
      close(newsockfd);
    }
  } /* End of while loop */

  close(sockfd);
  return 0;

}



// Returns the passed message to stderr.
void error(const char *msg) {
  perror(msg);
  exit(1);
}
