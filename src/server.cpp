// Server side C program to demonstrate simple HTTP server functionality
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <errno.h>

#define PORT 8081  // Define the port number the server will listen on

// Function prototypes
char* parse_method(char line[], const char symbol[]);
char* parse(char line[], const char symbol[]);
int send_message(int fd, char image_path[], char head[]);

// HTTP header to be sent for successful GET requests
char http_header[25] = "HTTP/1.1 200 Ok\r\n";

int main(int argc, char const *argv[])
{
    int server_fd, new_socket, pid;  // File descriptors and process ID
    long valread;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    
    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("In sockets");
        exit(EXIT_FAILURE);
    }
    
    // Setting up the address struct for the server
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);  // Convert port number to network byte order
    
    memset(address.sin_zero, '\0', sizeof address.sin_zero);  // Zero the rest of the struct

    // Bind the socket to the address
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("In bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, 10) < 0)
    {
        perror("In listen");
        exit(EXIT_FAILURE);
    }

    // Server loop: Accept and handle connections indefinitely
    while(1)
    {
        printf("\n+++++++ Waiting for new connection ++++++++\n\n");
        
        // Accept a new connection
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0)
        {
            perror("In accept");
            exit(EXIT_FAILURE);
        }

        // Create a child process to handle the new connection
        pid = fork();
        if(pid < 0){
            perror("Error on fork");
            exit(EXIT_FAILURE);
        }
        
        if(pid == 0){
            // Child process handles the request
            char buffer[30000] = {0};  // Buffer to store request data
            valread = read(new_socket, buffer, 30000);  // Read the request

            printf("\n buffer message: %s \n ", buffer);

            // Parse the request path
            char *parse_string = parse(buffer, " ");
            printf("Client ask for path: %s\n", parse_string);

            // Parse the request method (e.g., GET)
            char *parse_string_method = parse_method(buffer, " ");
            printf("Client method: %s\n", parse_string_method);

            // Prepare the HTTP response header
            char *copy_head = (char *)malloc(strlen(http_header) + 200);
            strcpy(copy_head, http_header);

            if(parse_string_method[0] == 'G' && parse_string_method[1] == 'E' && parse_string_method[2] == 'T'){
                // Handle GET request
                if(strcmp(parse_string, "/index.html") == 0) {
                    char path_head[500] = ".";
                    strcat(path_head, parse_string);
                    strcat(copy_head, "Content-Type: text/html\r\n\r\n");
                    send_message(new_socket, path_head, copy_head);  // Send response
                }
                else if (strcmp(parse_string, "/kotek.html") == 0)
                {
                    char path_head[500] = ".";
                    strcat(path_head, parse_string);
                    strcat(copy_head, "Content-Type: text/html\r\n\r\n");
                    send_message(new_socket, path_head, copy_head);  // Send response
                }
                else {
                    send_message(new_socket, "./path_error.html", copy_head);  // Handle unknown paths
                }
            }
            else {
                // Handle other methods (not GET)
                send_message(new_socket, "./error.html", copy_head);
            }

            close(new_socket);  // Close the socket
            free(copy_head);  // Free the allocated memory
        }
        else{
            // Parent process closes the socket and waits for new connections
            printf(">>>>>>>>>>Parent create child with pid: %d <<<<<<<<<", pid);
            close(new_socket);
        }
    }
    close(server_fd);  // Close the server socket
    return 0;
}

// Function to parse the HTTP method from the request line
char* parse_method(char line[], const char symbol[])
{
    char *copy = (char *)malloc(strlen(line) + 1);
    strcpy(copy, line);
        
    char *message;
    char * token = strtok(copy, symbol);  // Tokenize the string
    int current = 0;

    while(token != NULL) {
      if(current == 0){
          message = token;
          if(message == NULL){
              message = "";
          }
          return message;  // Return the method (first token)
      }
      current = current + 1;
   }
   free(copy);
   free(token);
   return message;
}

// Function to parse the path from the request line
char* parse(char line[], const char symbol[])
{
    char *copy = (char *)malloc(strlen(line) + 1);
    strcpy(copy, line);
    
    char *message;
    char * token = strtok(copy, symbol);  // Tokenize the string
    int current = 0;

    while(token != NULL) {
      token = strtok(NULL, " ");  // Get the next token
      if(current == 0){
          message = token;
          if(message == NULL){
              message = "";
          }
          return message;  // Return the path (second token)
      }
      current = current + 1;
   }
   free(token);
   free(copy);
   return message;
}

// Function to send a file with a response header
int send_message(int fd, char image_path[], char head[])
{
    struct stat stat_buf;  // To hold information about the file

    write(fd, head, strlen(head));  // Write the response header

    int fdimg = open(image_path, O_RDONLY);  // Open the file
    
    if(fdimg < 0){
        printf("Cannot Open file path : %s with error %d\n", image_path, fdimg); 
    }
     
    fstat(fdimg, &stat_buf);  // Get file status
    int img_total_size = stat_buf.st_size;
    int block_size = stat_buf.st_blksize;

    if(fdimg >= 0){
        ssize_t sent_size;

        while(img_total_size > 0){
              int send_bytes = ((img_total_size < block_size) ? img_total_size : block_size);
              int done_bytes = sendfile(fd, fdimg, NULL, block_size);  // Send file contents
              img_total_size -= done_bytes;
        }
        if(sent_size >= 0){
            printf("send file: %s \n" , image_path);
        }
        close(fdimg);  // Close the file
    }
}