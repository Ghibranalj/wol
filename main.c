#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int server_socket = 0, client_socket = 0;
struct sockaddr_in dest_addr = {0};

void on_sigint(int sig) {

  printf("Caught signal %d\n", sig);
  if (close(server_socket) < 0) {
    perror("can't close server socket");
    exit(EXIT_FAILURE);
  }
  exit(EXIT_SUCCESS);
}

int mac_string_to_binary(const char *mac_str, unsigned char *mac_bin) {
  if (sscanf(mac_str, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &mac_bin[0], &mac_bin[1],
             &mac_bin[2], &mac_bin[3], &mac_bin[4], &mac_bin[5]) != 6) {
    return -1;
  }
  return 0;
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

  unsigned char mac_address[6];
  if (mac_string_to_binary(addr, mac_address) == -1) {
    fprintf(stderr, "Invalid MAC address format\n");
    exit(EXIT_FAILURE);
  }

  struct sockaddr_in server_addr, client_addr;
  socklen_t client_addr_len = sizeof(client_addr);
  char buffer[144] = {0};

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

  dest_addr.sin_family = AF_INET;
  dest_addr.sin_port = htons(40000);
  dest_addr.sin_addr.s_addr = INADDR_BROADCAST;

  for (int i = 0; i < 6; i++) {
    buffer[i] = 0xFF;
  }
  for (int i = 0; i < 16; i++) {
    memcpy(buffer + (i * 6) + 6, mac_address, 6);
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

    printf("Client connected from %s\n", inet_ntoa(client_addr.sin_addr));

    client_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_socket == -1) {
      perror("can't create client socket");
      continue;
    }

    struct linger lin = {.l_onoff = 0, .l_linger = 0};
    if (setsockopt(client_socket, SOL_SOCKET, SO_LINGER, &lin, sizeof(lin))) {
      perror("setsockopt(SO_LINGER) failed");
      close(client_socket);
      continue;
    }

    int yes = 1;
    if (setsockopt(client_socket, SOL_SOCKET, SO_BROADCAST, &yes,
                   sizeof(yes))) {
      perror("setsockopt(SO_BROADCAST) failed");
      close(client_socket);
      continue;
    }

    if (setsockopt(client_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) <
        0) {
      perror("setsockopt(SO_REUSEADDR) failed");
      close(client_socket);
      continue;
    }

    if (setsockopt(client_socket, SOL_SOCKET, SO_REUSEPORT, &yes, sizeof(yes)) <
        0) {
      perror("setsockopt(SO_REUSEPORT) failed");
      close(client_socket);
      continue;
    }

    if (setsockopt(client_socket, SOL_SOCKET, SO_BROADCAST, &yes,
                   sizeof(yes))) {
      perror("setsockopt(SO_BROADCAST) failed");
      close(client_socket);
      continue;
    }

    if (sendto(client_socket, buffer, sizeof(buffer), 0,
               (struct sockaddr *)&dest_addr, sizeof(dest_addr)) == -1) {
      perror("sendto failed");
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
