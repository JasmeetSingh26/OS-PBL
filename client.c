#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/types.h>

#define PORT 8080

typedef struct threadArgument {
    int socket;
    FILE* file;
} threadArgument;

static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

void* receive_file_segment(void* arg) {
    threadArgument* thread_arg = (threadArgument*)arg;
    int socket = thread_arg->socket;
    FILE* file = thread_arg->file;
    char buffer[1024];
    ssize_t bytes_received;

    pthread_mutex_lock(&mtx);
    while ((bytes_received = recv(socket, buffer, sizeof(buffer), 0)) > 0) {
        fwrite(buffer, 1, bytes_received, file);
        pthread_mutex_unlock(&mtx);
        printf("received: %s\n", buffer);
        pthread_mutex_lock(&mtx);
    }
    pthread_mutex_unlock(&mtx);

    if (bytes_received < 0) {
        perror("could not receive bytes");
    }

    pthread_exit(NULL);
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Please enter two command line arguments.\n");
        return -1;
    }

    char* file_name = argv[1];
    int num_threads = atoi(argv[2]);
    char* server_ip = "127.0.0.1";

    int client_fd;
    struct sockaddr_in serv_addr;

    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Could not create the socket.");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        return -1;
    }

    if (connect(client_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Could not establish the connection");
        return -1;
    }

    send(client_fd, file_name, strlen(file_name) + 1, 0);
    printf("Client: Sending num_threads = %d\n", num_threads);
    send(client_fd, &num_threads, sizeof(num_threads), 0);

    char output_file_name[256];
    snprintf(output_file_name, sizeof(output_file_name), "rcv_%s", file_name);

    FILE* output_file = fopen(output_file_name, "wb");
    if (output_file == NULL) {
        perror("File creation failed");
        close(client_fd);
        return -1;
    }

    pthread_t thread;
    threadArgument thread_arg = {client_fd, output_file};
    pthread_create(&thread, NULL, receive_file_segment, &thread_arg);
    pthread_join(thread, NULL);

    printf("File received and saved as '%s'.\n", output_file_name);

    fclose(output_file);
    close(client_fd);

    return 0;
}