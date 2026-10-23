
#define _POSIX_C_SOURCE 200809L

#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 5000
#define BUFFER_SIZE 16384

static sqlite3 *db = NULL;
static pthread_mutex_t db_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t state_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t state_cond = PTHREAD_COND_INITIALIZER;

static int active_readers = 0;
static int active_writer = 0;
static int waiting_writers = 0;
static int total_reads = 0;
static int total_writes = 0;
static int next_event_id = 1;

static char events[100][256];
static int event_count = 0;

static void log_event(const char *message) {
    time_t now = time(NULL);
    struct tm tm_now;
    localtime_r(&now, &tm_now);

    char timebuf[16];
    strftime(timebuf, sizeof(timebuf), "%H:%M:%S", &tm_now);

    pthread_mutex_lock(&state_mutex);

    if (event_count < 100) {
        snprintf(events[event_count], sizeof(events[0]),
                 "%s - %s", timebuf, message);
        event_count++;
    } else {
        memmove(events, events + 1, 99 * sizeof(events[0]));
        snprintf(events[99], sizeof(events[99]),
                 "%s - %s", timebuf, message);
    }

    next_event_id++;
    pthread_mutex_unlock(&state_mutex);
}

static void reader_enter(const char *name) {
    pthread_mutex_lock(&state_mutex);

    while (active_writer || waiting_writers > 0)
        pthread_cond_wait(&state_cond, &state_mutex);

    active_readers++;

    char message[160];
    snprintf(message, sizeof(message),
             "%s started reading (%d active reader(s))",
             name, active_readers);
    pthread_mutex_unlock(&state_mutex);

    log_event(message);
}

static void reader_exit(const char *name) {
    pthread_mutex_lock(&state_mutex);

    active_readers--;

    char message[160];
    snprintf(message, sizeof(message),
             "%s finished reading (%d reader(s) remain)",
             name, active_readers);

    if (active_readers == 0)
        pthread_cond_broadcast(&state_cond);

    pthread_mutex_unlock(&state_mutex);
    log_event(message);
}

static void writer_enter(const char *name) {
    pthread_mutex_lock(&state_mutex);
    waiting_writers++;

    char message[160];
    snprintf(message, sizeof(message), "%s is waiting for exclusive access", name);
    pthread_mutex_unlock(&state_mutex);
    log_event(message);

    pthread_mutex_lock(&state_mutex);

    while (active_writer || active_readers > 0)
        pthread_cond_wait(&state_cond, &state_mutex);

    waiting_writers--;
    active_writer = 1;

    snprintf(message, sizeof(message), "%s acquired exclusive write access", name);
    pthread_mutex_unlock(&state_mutex);
    log_event(message);
}

static void writer_exit(const char *name) {
    pthread_mutex_lock(&state_mutex);
    active_writer = 0;

    char message[160];
    snprintf(message, sizeof(message), "%s released exclusive access", name);

    pthread_cond_broadcast(&state_cond);
    pthread_mutex_unlock(&state_mutex);
    log_event(message);
}

static void json_escape(const char *src, char *dst, size_t size) {
    size_t j = 0;

    for (size_t i = 0; src[i] && j + 2 < size; i++) {
        unsigned char c = (unsigned char)src[i];

        if (c == '"' || c == '\\') {
            dst[j++] = '\\';
            dst[j++] = (char)c;
        } else if (c == '\n') {
            dst[j++] = '\\';
            dst[j++] = 'n';
        } else if (c >= 32) {
            dst[j++] = (char)c;
        }
    }

    dst[j] = '\0';
}

static void send_response(int client, int status,
                          const char *content_type, const char *body) {
    char header[1024];

    const char *status_text =
        status == 200 ? "OK" :
        status == 400 ? "Bad Request" :
        status == 404 ? "Not Found" :
        status == 409 ? "Conflict" : "Internal Server Error";

    int body_length = (int)strlen(body);

    snprintf(header, sizeof(header),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s; charset=utf-8\r\n"
        "Content-Length: %d\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type\r\n"
        "Connection: close\r\n\r\n",
        status, status_text, content_type, body_length);

    send(client, header, strlen(header), 0);
    send(client, body, body_length, 0);
}

static void send_json(int client, const char *body) {
    send_response(client, 200, "application/json", body);
}

static void get_json_string(const char *body, const char *key,
                            char *out, size_t out_size) {
    out[0] = '\0';

    char pattern[100];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);

    const char *p = strstr(body, pattern);
    if (!p) return;

    p = strchr(p + strlen(pattern), ':');
    if (!p) return;

    p++;
    while (*p == ' ' || *p == '\t') p++;

    if (*p != '"') return;
    p++;

    size_t i = 0;

    while (*p && *p != '"' && i + 1 < out_size) {
        if (*p == '\\' && p[1]) p++;
        out[i++] = *p++;
    }

    out[i] = '\0';
}

static void handle_state(int client) {
    char body[BUFFER_SIZE];
    size_t used = 0;

    pthread_mutex_lock(&state_mutex);

    int n = snprintf(body, sizeof(body),
        "{\"state\":{\"active_readers\":%d,"
        "\"writer_active\":%s,\"waiting_writers\":%d},"
        "\"stats\":{\"reads\":%d,\"writes\":%d},"
        "\"records\":[",
        active_readers,
        active_writer ? "true" : "false",
        waiting_writers, total_reads, total_writes);

    if (n < 0) n = 0;
    used = (size_t)n;

    pthread_mutex_unlock(&state_mutex);

    pthread_mutex_lock(&db_mutex);

    sqlite3_stmt *stmt = NULL;

    if (sqlite3_prepare_v2(db,
        "SELECT id,key,value,updated_at FROM records ORDER BY id",
        -1, &stmt, NULL) == SQLITE_OK) {

        int first = 1;

        while (sqlite3_step(stmt) == SQLITE_ROW &&
               used < sizeof(body) - 800) {

            char key[256], value[512], updated[128];

            snprintf(key, sizeof(key), "%s",
                (const char *)sqlite3_column_text(stmt, 1));
            snprintf(value, sizeof(value), "%s",
                (const char *)sqlite3_column_text(stmt, 2));
            snprintf(updated, sizeof(updated), "%s",
                (const char *)sqlite3_column_text(stmt, 3));

            char ek[512], ev[1024], eu[256];

            json_escape(key, ek, sizeof(ek));
            json_escape(value, ev, sizeof(ev));
            json_escape(updated, eu, sizeof(eu));

            n = snprintf(body + used, sizeof(body) - used,
                "%s{\"id\":%d,\"key\":\"%s\","
                "\"value\":\"%s\",\"updated_at\":\"%s\"}",
                first ? "" : ",",
                sqlite3_column_int(stmt, 0), ek, ev, eu);

            if (n < 0 || (size_t)n >= sizeof(body) - used)
                break;

            used += (size_t)n;
            first = 0;
        }
    }

    sqlite3_finalize(stmt);
    pthread_mutex_unlock(&db_mutex);

    n = snprintf(body + used, sizeof(body) - used, "],\"events\":[");
    if (n > 0) used += (size_t)n;

    pthread_mutex_lock(&state_mutex);

    for (int i = event_count - 1; i >= 0 &&
         used < sizeof(body) - 400; i--) {

        char escaped[512];
        json_escape(events[i], escaped, sizeof(escaped));

        n = snprintf(body + used, sizeof(body) - used,
            "%s{\"kind\":\"system\",\"message\":\"%s\","
            "\"worker\":\"simulator\",\"time\":\"\"}",
            i == event_count - 1 ? "" : ",", escaped);

        if (n < 0 || (size_t)n >= sizeof(body) - used)
            break;

        used += (size_t)n;
    }

    pthread_mutex_unlock(&state_mutex);

    snprintf(body + used, sizeof(body) - used, "]}");
    send_json(client, body);
}

typedef struct {
    char worker[64];
    char key[128];
    char value[256];
    int is_writer;
} Operation;

static void *operation_thread(void *arg) {
    Operation *op = arg;

    if (op->is_writer) {
        writer_enter(op->worker);

        pthread_mutex_lock(&db_mutex);

        sqlite3_stmt *stmt = NULL;
        int result = sqlite3_prepare_v2(db,
            "INSERT INTO records(key,value,updated_at) "
            "VALUES(?,?,datetime('now','localtime')) "
            "ON CONFLICT(key) DO UPDATE SET "
            "value=excluded.value,updated_at=excluded.updated_at",
            -1, &stmt, NULL);

        if (result == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, op->key, -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 2, op->value, -1, SQLITE_TRANSIENT);
            sqlite3_step(stmt);
        }

        sqlite3_finalize(stmt);
        pthread_mutex_unlock(&db_mutex);

        pthread_mutex_lock(&state_mutex);
        total_writes++;
        pthread_mutex_unlock(&state_mutex);

        writer_exit(op->worker);
    } else {
        reader_enter(op->worker);

        pthread_mutex_lock(&db_mutex);

        sqlite3_stmt *stmt = NULL;
        sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM records",
                           -1, &stmt, NULL);
        if (stmt) {
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }

        pthread_mutex_unlock(&db_mutex);

        struct timespec delay = {1, 0};
        nanosleep(&delay, NULL);

        pthread_mutex_lock(&state_mutex);
        total_reads++;
        pthread_mutex_unlock(&state_mutex);

        reader_exit(op->worker);
    }

    free(op);
    return NULL;
}

static void serve_operation(int client, const char *body, int is_writer) {
    Operation *op = calloc(1, sizeof(*op));

    if (!op) {
        send_response(client, 500, "application/json",
                      "{\"ok\":false,\"error\":\"Out of memory\"}");
        return;
    }

    get_json_string(body, "worker", op->worker, sizeof(op->worker));

    if (!op->worker[0])
        snprintf(op->worker, sizeof(op->worker), "%s",
                 is_writer ? "Writer" : "Reader");

    op->is_writer = is_writer;

    if (is_writer) {
        get_json_string(body, "key", op->key, sizeof(op->key));
        get_json_string(body, "value", op->value, sizeof(op->value));

        if (!op->key[0] || !op->value[0]) {
            free(op);
            send_response(client, 400, "application/json",
                "{\"ok\":false,\"error\":\"Key and value are required\"}");
            return;
        }
    }

    pthread_t thread;

    if (pthread_create(&thread, NULL, operation_thread, op) != 0) {
        free(op);
        send_response(client, 500, "application/json",
                      "{\"ok\":false,\"error\":\"Thread creation failed\"}");
        return;
    }

    pthread_detach(thread);

    char response[512];

    snprintf(response, sizeof(response),
        "{\"ok\":true,\"message\":\"%s operation started for %s\"}",
        is_writer ? "Write" : "Read", op->worker);

    send_json(client, response);
}

static void handle_client(int client) {
    char request[BUFFER_SIZE];
    size_t received = 0;

    while (received < sizeof(request) - 1) {
        ssize_t n = recv(client, request + received,
                         sizeof(request) - received - 1, 0);

        if (n <= 0) break;

        received += (size_t)n;
        request[received] = '\0';

        if (strstr(request, "\r\n\r\n")) {
            char *header_end = strstr(request, "\r\n\r\n");
            size_t header_length = (size_t)(header_end - request) + 4;

            int content_length = 0;
            char *cl = strcasestr(request, "Content-Length:");

            if (cl)
                content_length = atoi(cl + strlen("Content-Length:"));

            if (received >= header_length + (size_t)content_length)
                break;
        }
    }

    request[received] = '\0';

    char method[16] = {0}, path[256] = {0};

    sscanf(request, "%15s %255s", method, path);

    char *body = strstr(request, "\r\n\r\n");
    body = body ? body + 4 : request + received;

    if (strcmp(method, "OPTIONS") == 0) {
        send_response(client, 200, "text/plain", "");
    } else if (strcmp(method, "GET") == 0 &&
               strcmp(path, "/api/state") == 0) {
        handle_state(client);
    } else if (strcmp(method, "POST") == 0 &&
               strcmp(path, "/api/read") == 0) {
        serve_operation(client, body, 0);
    } else if (strcmp(method, "POST") == 0 &&
               strcmp(path, "/api/write") == 0) {
        serve_operation(client, body, 1);
    } else if (strcmp(method, "POST") == 0 &&
               strcmp(path, "/api/reset") == 0) {
        pthread_mutex_lock(&state_mutex);

        if (active_writer || active_readers) {
            pthread_mutex_unlock(&state_mutex);
            send_response(client, 409, "application/json",
                "{\"ok\":false,\"error\":\"Wait for operations to finish\"}");
        } else {
            pthread_mutex_lock(&db_mutex);
            sqlite3_exec(db, "DELETE FROM records", NULL, NULL, NULL);
            sqlite3_exec(db,
                "INSERT INTO records(key,value,updated_at) VALUES"
                "('project_status','Running',datetime('now','localtime')),"
                "('room_temperature','27 C',datetime('now','localtime')),"
                "('system_message','Readers and writers synchronized',"
                "datetime('now','localtime'))",
                NULL, NULL, NULL);
            pthread_mutex_unlock(&db_mutex);

            total_reads = total_writes = 0;
            event_count = 0;

            pthread_mutex_unlock(&state_mutex);

            log_event("Simulator reset");
            send_json(client, "{\"ok\":true}");
        }
    } else if (strcmp(method, "GET") == 0 &&
               (strcmp(path, "/") == 0 ||
                strcmp(path, "/index.html") == 0)) {
        FILE *file = fopen("index.html", "rb");

        if (!file) {
            send_response(client, 404, "text/plain",
                          "index.html not found");
        } else {
            char html[BUFFER_SIZE * 4];
            size_t n = fread(html, 1, sizeof(html) - 1, file);
            fclose(file);
            html[n] = '\0';
            send_response(client, 200, "text/html", html);
        }
    } else if (strcmp(method, "GET") == 0 &&
               strncmp(path, "/static/", 8) == 0 &&
               !strstr(path, "..")) {
        char filename[300];
        snprintf(filename, sizeof(filename), ".%s", path);

        FILE *file = fopen(filename, "rb");

        if (!file) {
            send_response(client, 404, "text/plain", "File not found");
        } else {
            char data[BUFFER_SIZE * 4];
            size_t n = fread(data, 1, sizeof(data) - 1, file);
            fclose(file);
            data[n] = '\0';

            const char *type = strstr(path, ".css") ? "text/css" :
                               strstr(path, ".js") ? "application/javascript" :
                               "text/plain";

            send_response(client, 200, type, data);
        }
    } else {
        send_response(client, 404, "application/json",
                      "{\"ok\":false,\"error\":\"Endpoint not found\"}");
    }

    close(client);
}

int main(void) {
    signal(SIGPIPE, SIG_IGN);

    if (sqlite3_open("simulator.db", &db) != SQLITE_OK) {
        fprintf(stderr, "Cannot open SQLite database: %s\n",
                sqlite3_errmsg(db));
        return 1;
    }

    const char *schema =
        "CREATE TABLE IF NOT EXISTS records("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "key TEXT UNIQUE NOT NULL,"
        "value TEXT NOT NULL,"
        "updated_at TEXT NOT NULL);"
        "INSERT OR IGNORE INTO records(key,value,updated_at) VALUES"
        "('project_status','Running',datetime('now','localtime')),"
        "('room_temperature','27 C',datetime('now','localtime')),"
        "('system_message','Readers and writers synchronized',"
        "datetime('now','localtime'));";

    char *error = NULL;

    if (sqlite3_exec(db, schema, NULL, NULL, &error) != SQLITE_OK) {
        fprintf(stderr, "Database error: %s\n", error);
        sqlite3_free(error);
        sqlite3_close(db);
        return 1;
    }

    log_event("Readers-Writers simulator started");

    int server = socket(AF_INET, SOCK_STREAM, 0);

    if (server < 0) {
        perror("socket");
        sqlite3_close(db);
        return 1;
    }

    int reuse = 1;
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    struct sockaddr_in address = {0};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(PORT);

    if (bind(server, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind");
        close(server);
        sqlite3_close(db);
        return 1;
    }

    if (listen(server, 32) < 0) {
        perror("listen");
        close(server);
        sqlite3_close(db);
        return 1;
    }

    printf("Readers-Writers C server running at http://127.0.0.1:%d\n", PORT);
    printf("Press Ctrl+C to stop the server.\n");

    while (1) {
        int client = accept(server, NULL, NULL);

        if (client < 0) {
            if (errno == EINTR) continue;
            perror("accept");
            continue;
        }

        handle_client(client);
    }

    close(server);
    sqlite3_close(db);
    return 0;
}
