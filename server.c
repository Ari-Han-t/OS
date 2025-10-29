/*
 * Theater Booking System - Linux/Ubuntu (FINAL WORKING VERSION)
 * Compile: gcc -o server server.c -lpthread
 * Run: ./server
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#define PORT 8080
#define ROWS 5
#define COLS 8
#define TOTAL_SEATS (ROWS * COLS)
#define BUFFER_SIZE 16384
#define MAX_CLIENTS 50

// Global shared resources
int seats[ROWS][COLS];
int available_seats = TOTAL_SEATS;
sem_t rw_mutex, mutex_lock;
int read_count = 0;
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

// Booking mode tracking
int booking_user_id = 0;
pthread_mutex_t booking_mutex = PTHREAD_MUTEX_INITIALIZER;

// Activity log
#define LOG_SIZE 100
char activity_log[LOG_SIZE][512];
int log_index = 0;

typedef struct {
    int socket;
    int client_id;
} client_data_t;

void add_log(const char *message) {
    pthread_mutex_lock(&log_mutex);
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    snprintf(activity_log[log_index % LOG_SIZE], 512, 
             "[%02d:%02d:%02d] %s", t->tm_hour, t->tm_min, t->tm_sec, message);
    log_index++;
    printf("%s\n", activity_log[(log_index - 1) % LOG_SIZE]);
    fflush(stdout);
    pthread_mutex_unlock(&log_mutex);
}

void initialize_seats() {
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            seats[i][j] = 0;
        }
    }
    available_seats = TOTAL_SEATS;
}

void get_seats_json(char *buffer) {
    strcpy(buffer, "{\"seats\":[");
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            char seat[128];
            sprintf(seat, "{\"row\":\"%c\",\"col\":%d,\"status\":%d}", 
                   'A'+i, j+1, seats[i][j]);
            strcat(buffer, seat);
            if (!(i == ROWS-1 && j == COLS-1)) strcat(buffer, ",");
        }
    }
    
    pthread_mutex_lock(&booking_mutex);
    int booking_user = booking_user_id;
    pthread_mutex_unlock(&booking_mutex);
    
    sprintf(buffer + strlen(buffer), 
            "],\"available\":%d,\"read_count\":%d,\"booking_active\":%d}",
            available_seats, read_count, booking_user);
}

void get_log_json(char *buffer) {
    pthread_mutex_lock(&log_mutex);
    strcpy(buffer, "{\"logs\":[");
    int start = (log_index > LOG_SIZE) ? (log_index - LOG_SIZE) : 0;
    int count = (log_index > LOG_SIZE) ? LOG_SIZE : log_index;
    
    for (int i = 0; i < count; i++) {
        int idx = (start + i) % LOG_SIZE;
        if (i > 0) strcat(buffer, ",");
        strcat(buffer, "\"");
        
        char *src = activity_log[idx];
        char *dest = buffer + strlen(buffer);
        while (*src) {
            if (*src == '"' || *src == '\\') *dest++ = '\\';
            *dest++ = *src++;
        }
        *dest = '\0';
        strcat(buffer, "\"");
    }
    strcat(buffer, "]}");
    pthread_mutex_unlock(&log_mutex);
}

int book_seat(int row, int col, int user_id) {
    if (row < 0 || row >= ROWS || col < 0 || col >= COLS) return -1;
    if (seats[row][col] == 1) return 0;
    
    seats[row][col] = 1;
    available_seats--;
    
    char log_msg[256];
    sprintf(log_msg, "USER %d booked seat %c%d (Available: %d)", 
            user_id, 'A'+row, col+1, available_seats);
    add_log(log_msg);
    return 1;
}

int extract_user_id(const char *path) {
    char *user_param = strstr(path, "user=");
    if (user_param) {
        int id = atoi(user_param + 5);
        if (id > 0) return id;
    }
    return 1;
}

void handle_viewer(int client_socket, int user_id) {
    char log_msg[256];
    
    sem_wait(&mutex_lock);
    read_count++;
    if (read_count == 1) {
        sem_wait(&rw_mutex);
        sprintf(log_msg, "VIEWER (User %d): First reader - LOCKED rw_mutex", user_id);
        add_log(log_msg);
    } else {
        sprintf(log_msg, "VIEWER (User %d): Entered (Read Count: %d)", user_id, read_count);
        add_log(log_msg);
    }
    sem_post(&mutex_lock);
    
    char response[BUFFER_SIZE];
    get_seats_json(response);
    char http_response[BUFFER_SIZE];
    sprintf(http_response, 
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Content-Length: %ld\r\n"
            "\r\n%s", 
            strlen(response), response);
    write(client_socket, http_response, strlen(http_response));
    
    sleep(1);
    
    sem_wait(&mutex_lock);
    read_count--;
    if (read_count == 0) {
        sem_post(&rw_mutex);
        sprintf(log_msg, "VIEWER (User %d): Last reader - UNLOCKED rw_mutex", user_id);
        add_log(log_msg);
    } else {
        sprintf(log_msg, "VIEWER (User %d): Exited (Read Count: %d)", user_id, read_count);
        add_log(log_msg);
    }
    sem_post(&mutex_lock);
}

void handle_request_booking(int client_socket, int user_id) {
    char response[512];
    char http_response[1024];
    char log_msg[256];
    
    pthread_mutex_lock(&booking_mutex);
    
    if (booking_user_id != 0 && booking_user_id != user_id) {
        sprintf(response, "{\"status\":\"busy\",\"message\":\"USER %d is booking. Please wait.\",\"booking_user\":%d}", 
                booking_user_id, booking_user_id);
        sprintf(log_msg, "USER %d: Tried to book but USER %d already has the lock", 
                user_id, booking_user_id);
        add_log(log_msg);
    } else {
        booking_user_id = user_id;
        sprintf(response, "{\"status\":\"granted\",\"message\":\"Booking mode activated\"}");
        sprintf(log_msg, "USER %d: ACQUIRED BOOKING LOCK (exclusive)", user_id);
        add_log(log_msg);
    }
    
    pthread_mutex_unlock(&booking_mutex);
    
    sprintf(http_response, 
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "\r\n%s", response);
    write(client_socket, http_response, strlen(http_response));
}

void handle_release_booking(int client_socket, int user_id) {
    char response[512];
    char http_response[1024];
    char log_msg[256];
    
    pthread_mutex_lock(&booking_mutex);
    
    if (booking_user_id == user_id) {
        booking_user_id = 0;
        sprintf(response, "{\"status\":\"released\"}");
        sprintf(log_msg, "USER %d: RELEASED BOOKING LOCK", user_id);
        add_log(log_msg);
    } else {
        sprintf(response, "{\"status\":\"error\",\"message\":\"You don't have the lock\"}");
    }
    
    pthread_mutex_unlock(&booking_mutex);
    
    sprintf(http_response, 
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "\r\n%s", response);
    write(client_socket, http_response, strlen(http_response));
}

void handle_booker(int client_socket, int user_id, int row, int col) {
    char log_msg[256];
    char response[512];
    char http_response[1024];
    
    pthread_mutex_lock(&booking_mutex);
    int has_lock = (booking_user_id == user_id);
    int current_holder = booking_user_id;
    pthread_mutex_unlock(&booking_mutex);
    
    if (!has_lock) {
        sprintf(response, "{\"status\":\"error\",\"message\":\"You don't have booking lock! (Holder: USER %d)\"}", current_holder);
        sprintf(log_msg, "USER %d: Tried to book without lock", user_id);
        add_log(log_msg);
        
        sprintf(http_response, 
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: application/json\r\n"
                "Access-Control-Allow-Origin: *\r\n"
                "\r\n%s", response);
        write(client_socket, http_response, strlen(http_response));
        return;
    }
    
    sprintf(log_msg, "USER %d: Waiting for rw_mutex to book %c%d...", user_id, 'A'+row, col+1);
    add_log(log_msg);
    
    sem_wait(&rw_mutex);
    
    sprintf(log_msg, "USER %d: Acquired rw_mutex (exclusive)", user_id);
    add_log(log_msg);
    
    int result = book_seat(row, col, user_id);
    
    if (result == 1) {
        sprintf(response, "{\"status\":\"success\",\"message\":\"Seat %c%d booked!\"}",
                'A'+row, col+1);
    } else if (result == 0) {
        sprintf(response, "{\"status\":\"error\",\"message\":\"Seat already booked\"}");
    } else {
        sprintf(response, "{\"status\":\"error\",\"message\":\"Invalid seat\"}");
    }
    
    sprintf(http_response, 
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "\r\n%s", response);
    write(client_socket, http_response, strlen(http_response));
    
    sleep(1);
    
    sem_post(&rw_mutex);
    sprintf(log_msg, "USER %d: Released rw_mutex", user_id);
    add_log(log_msg);
    
    pthread_mutex_lock(&booking_mutex);
    if (booking_user_id == user_id) {
        booking_user_id = 0;
        sprintf(log_msg, "USER %d: RELEASED BOOKING LOCK", user_id);
        add_log(log_msg);
    }
    pthread_mutex_unlock(&booking_mutex);
}

void serve_file(int client_socket, const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (file) {
        fseek(file, 0, SEEK_END);
        long file_size = ftell(file);
        fseek(file, 0, SEEK_SET);
        
        char header[512];
        sprintf(header, 
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/html; charset=utf-8\r\n"
                "Content-Length: %ld\r\n"
                "\r\n", file_size);
        write(client_socket, header, strlen(header));
        
        char buffer[BUFFER_SIZE];
        size_t bytes_read;
        while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
            write(client_socket, buffer, bytes_read);
        }
        fclose(file);
    } else {
        const char *error = "HTTP/1.1 404 Not Found\r\n\r\n<h1>404</h1>";
        write(client_socket, error, strlen(error));
    }
}

void *handle_client(void *arg) {
    client_data_t *data = (client_data_t *)arg;
    int client_socket = data->socket;
    char buffer[BUFFER_SIZE];
    
    ssize_t bytes_read = read(client_socket, buffer, BUFFER_SIZE - 1);
    if (bytes_read <= 0) {
        close(client_socket);
        free(data);
        return NULL;
    }
    
    buffer[bytes_read] = '\0';
    
    char method[16], path[512];
    if (sscanf(buffer, "%s %s", method, path) != 2) {
        close(client_socket);
        free(data);
        return NULL;
    }
    
    int user_id = extract_user_id(path);
    
    if (strstr(path, "/view")) {
        handle_viewer(client_socket, user_id);
    }
    else if (strstr(path, "/request_booking")) {
        handle_request_booking(client_socket, user_id);
    }
    else if (strstr(path, "/release_booking")) {
        handle_release_booking(client_socket, user_id);
    }
    else if (strstr(path, "/book")) {
        char *row_param = strstr(path, "row=");
        char *col_param = strstr(path, "col=");
        
        if (row_param && col_param) {
            int row = atoi(row_param + 4);
            int col = atoi(col_param + 4);
            handle_booker(client_socket, user_id, row, col);
        }
    } 
    else if (strstr(path, "/log")) {
        char response[BUFFER_SIZE * 2];
        get_log_json(response);
        char http_response[BUFFER_SIZE * 2];
        sprintf(http_response, 
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: application/json\r\n"
                "Access-Control-Allow-Origin: *\r\n"
                "\r\n%s", response);
        write(client_socket, http_response, strlen(http_response));
    } 
    else if (strstr(path, "/reset")) {
        sem_wait(&rw_mutex);
        initialize_seats();
        read_count = 0;
        pthread_mutex_lock(&booking_mutex);
        booking_user_id = 0;
        pthread_mutex_unlock(&booking_mutex);
        sem_post(&rw_mutex);
        
        add_log("SYSTEM RESET");
        
        const char *success = 
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "\r\n{\"status\":\"success\"}";
        write(client_socket, success, strlen(success));
    } 
    else {
        serve_file(client_socket, "index.html");
    }
    
    close(client_socket);
    free(data);
    return NULL;
}

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    int client_counter = 0;
    
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║  Theater Booking System - FINAL WORKING VERSION          ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    
    sem_init(&mutex_lock, 0, 1);
    sem_init(&rw_mutex, 0, 1);
    printf("✓ Semaphores initialized\n");
    
    initialize_seats();
    add_log("Theater System Started");
    printf("✓ Theater initialized (%d seats)\n", TOTAL_SEATS);
    
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    
    bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(server_socket, MAX_CLIENTS);
    
    printf("✓ Server running on port %d\n", PORT);
    printf("\n📍 Open: http://localhost:%d\n\n", PORT);
    printf("════════════════════════════════════════════════════════════\n");
    printf("Activity Log:\n");
    printf("════════════════════════════════════════════════════════════\n\n");
    
    while (1) {
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_len);
        if (client_socket < 0) continue;
        
        client_data_t *data = malloc(sizeof(client_data_t));
        data->socket = client_socket;
        data->client_id = ++client_counter;
        
        pthread_t thread;
        pthread_create(&thread, NULL, handle_client, data);
        pthread_detach(thread);
    }
    
    return 0;
}
