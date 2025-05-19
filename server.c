#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <math.h>

#define PORT 8080
#define BUFFER_SIZE 1024

typedef struct {
    int socket;
    char *file_name;
    long start_byte;
    long end_byte;
} ThreadArg;

static pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t send_mutex = PTHREAD_MUTEX_INITIALIZER;

void *send_file_segment(void *arg) {
    ThreadArg *targ = (ThreadArg *)arg;
    FILE *file = fopen(targ->file_name, "rb");
    if (!file) {
        perror("Error opening file in thread");
        pthread_exit(NULL);
    }

    long bytes_left = targ->end_byte - targ->start_byte + 1;
    char buffer[BUFFER_SIZE];

    pthread_mutex_lock(&file_mutex);
    if (fseek(file, targ->start_byte, SEEK_SET) != 0) {
        perror("fseek failed");
        pthread_mutex_unlock(&file_mutex);
        fclose(file);
        pthread_exit(NULL);
    }
    pthread_mutex_unlock(&file_mutex);

    while (bytes_left > 0) {
        size_t chunk_size = (bytes_left < BUFFER_SIZE) ? bytes_left : BUFFER_SIZE;

        pthread_mutex_lock(&file_mutex);
        size_t bytes_read = fread(buffer, 1, chunk_size, file);
        pthread_mutex_unlock(&file_mutex);

        if (bytes_read <= 0) {
            if (feof(file)) break;
            perror("Read error in thread");
            break;
        }

        size_t total_sent = 0;
        while (total_sent < bytes_read) {
            pthread_mutex_lock(&send_mutex);
            ssize_t sent = send(targ->socket, buffer + total_sent, bytes_read - total_sent, 0);
            pthread_mutex_unlock(&send_mutex);

            if (sent <= 0) {
                perror("Send error in thread");
                fclose(file);
                pthread_exit(NULL);
            }
            total_sent += sent;
        }

        bytes_left -= bytes_read;
    }

    fclose(file);
    printf("Thread: Sent segment [%ld - %ld]\n", targ->start_byte, targ->end_byte);
    pthread_exit(NULL);
}

int main() {
    int server_fd, client_socket;
    struct sockaddr_in server_addr;
    socklen_t addrlen = sizeof(server_addr);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on port %d...\n", PORT);

    while (1) {
        printf("\nWaiting for a client...\n");

        if ((client_socket = accept(server_fd, (struct sockaddr *)&server_addr, &addrlen)) < 0) {
            perror("Accept failed");
            continue;
        }

        printf("Client connected.\n");

        char file_name[256] = {0};
        int num_threads;

        if (recv(client_socket, file_name, sizeof(file_name), 0) <= 0) {
            perror("Failed to receive file name");
            close(client_socket);
            continue;
        }

        if (recv(client_socket, &num_threads, sizeof(num_threads), 0) <= 0) {
            perror("Failed to receive thread count");
            close(client_socket);
            continue;
        }

        printf("Requested file: %s | Threads: %d\n", file_name, num_threads);

        FILE *file = fopen(file_name, "rb");
        if (!file) {
            perror("File not found");
            close(client_socket);
            continue;
        }

        fseek(file, 0, SEEK_END);
        long file_size = ftell(file);
        fclose(file);

        long segment_size = ceil((double)file_size / num_threads);
        printf("File size: %ld | Segment size: %ld\n", file_size, segment_size);

        pthread_t threads[num_threads];
        ThreadArg thread_args[num_threads];

        for (int i = 0; i < num_threads; i++) {
            thread_args[i].socket = client_socket;
            thread_args[i].file_name = file_name;
            thread_args[i].start_byte = i * segment_size;
            thread_args[i].end_byte = (i == num_threads - 1) ? file_size - 1 : (i + 1) * segment_size - 1;

            printf("Thread %d: [%ld - %ld]\n", i + 1,
                   thread_args[i].start_byte, thread_args[i].end_byte);

            pthread_create(&threads[i], NULL, send_file_segment, &thread_args[i]);
        }

        for (int i = 0; i < num_threads; i++) {
            pthread_join(threads[i], NULL);
        }

        printf("File transfer complete.\n");
        close(client_socket);
    }

    close(server_fd);
    return 0;
}
