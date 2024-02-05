#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int server_socket = 0, client_socket = 0;

void on_sigint(int sig) {
  printf("Caught signal %d\n", sig);
  close(server_socket);
  exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
  char *addr = "a8:a1:59:e8:b3:16";
  int port = 54321;

  if (argc > 1) {
    if (strlen(argv[1]) != 17) {
      perror("Invalid MAC address");
      exit(EXIT_FAILURE);
    }
    addr = argv[1];
  }

  // TODO: use raw sockets to send the magic packet
  char cmd[100] = {0};
  strcpy(cmd, "wol ");
  strncat(cmd, addr, 17);

  printf("will run \"%s\" command\n", cmd);

  if (argc > 2) {
    port = strtoul(argv[2], NULL, 10);
    if (errno == ERANGE) {
      perror("Invalid port number");
      exit(EXIT_FAILURE);
    }
    if (port > 65535) {
      perror("Port number out of range");
      exit(EXIT_FAILURE);
    }
  }

  if (signal(SIGINT, on_sigint) == SIG_ERR) {
    perror("signal");
    exit(EXIT_FAILURE);
  }

  if (signal(SIGTERM, on_sigint) == SIG_ERR) {
    perror("signal");
    exit(EXIT_FAILURE);
  }

  struct sockaddr_in server_addr, client_addr;
  socklen_t client_addr_len = sizeof(client_addr);
  char buffer[1024] = {0};

  // Create a socket
  server_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (server_socket == -1) {
    perror("Socket creation failed");
    exit(EXIT_FAILURE);
  }

  // Configure server address structure
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port); // Port number
  server_addr.sin_addr.s_addr = INADDR_ANY;

  // Bind the socket to the specified IP address and port
  if (bind(server_socket, (struct sockaddr *)&server_addr,
           sizeof(server_addr)) < 0) {
    perror("Binding failed");
    close(server_socket);
    exit(EXIT_FAILURE);
  }

  // Listen for incoming connections
  if (listen(server_socket, 5) < 0) {
    perror("Listening failed");
    close(server_socket);
    exit(EXIT_FAILURE);
  }

  printf("Server listening on port %d, sending to mac %s...\n", port, addr);

  // Accept incoming connections and handle data
  while (1) {
    client_socket = accept(server_socket, (struct sockaddr *)&client_addr,
                           &client_addr_len);
    if (client_socket == -1) {
      perror("Accepting connection failed");
      continue;
    }

    FILE *f = popen(cmd, "r");
    if (f == NULL) {
      perror("Error opening command");
      close(client_socket);
      continue;
    }

    // Read the output of the command
    while (fgets(buffer, sizeof(buffer), f) != NULL) {
      printf("%s", buffer);
      send(client_socket, buffer, strlen(buffer), 0);
    }

    if (pclose(f) == -1) {
      perror("Error executing command");
      close(client_socket);
      continue;
    }
    // Close the client socket
    close(client_socket);
    printf("Client disconnected\n");
  }

  // Close the server socket
  close(server_socket);

  return 0;
}
