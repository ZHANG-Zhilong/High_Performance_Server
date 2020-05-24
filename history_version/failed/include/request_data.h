//
// Created by 张志龙 on 2020/5/17.
//

#ifndef HIGH_PERFORMANCE_SERVER_REQUEST_DATA_H
#define HIGH_PERFORMANCE_SERVER_REQUEST_DATA_H

#include <iostream>
#include <unordered_map>
#include <queue>
#include <pthread.h>

#define ISspace(x) isspace((int) x)
#define SERVER_STRING "Server name"
#define STDIN 0
#define STDOUT 1
#define STDERR 2
constexpr int STATE_PARSE_URI = 1;
constexpr int STATE_PARSE_HEADERS = 2;
constexpr int STATE_RECV_BODY = 3;
constexpr int STATE_ANALYSIS = 4;
constexpr int STATE_FINISH = 5;
constexpr int MAX_BUFF = 4096;
constexpr int METHOD_POST = 1;
constexpr int METHOD_GET = 2;
constexpr int HTTP_10 = 1;
constexpr int HTTP_11 = 2;
constexpr int EPOLL_WAIT_TIME = 500;
constexpr int AGAIN_MAX_TIMES = 200;
constexpr int PARSE_URI_AGAIN = -1;
constexpr int PARSE_URI_ERROR = -2;
constexpr int PARSE_URI_SUCCESS = 0;
constexpr int PARSE_HEADER_AGAIN = -1;
constexpr int PARSE_HEADER_ERROR = -2;
constexpr int PARSE_HEADER_SUCCESS = 0;
constexpr int ANALYSIS_ERROR = -2;
constexpr int ANALYSIS_SUCCESS = 0;

struct mytimer;
struct requestData;
typedef struct mytimer mytimer;
typedef struct requestData requestData;


class MimeType {
private:
    static pthread_mutex_t lock;
    static std::unordered_map<std::string, std::string> mime;

    MimeType();

    ~MimeType();

    MimeType(const MimeType &m);

    static void init();

public:
    static std::string getMine(const std::string &suffix);

private:
    static pthread_once_t ponce_;
};

pthread_once_t MimeType::ponce_ = PTHREAD_ONCE_INIT;
std::unordered_map<std::string, std::string> MimeType::mime;

enum HeaderState {
    h_start = 0,
    h_key,
    h_colon,
    h_spaces_after_colon,
    h_value,
    h_CR,
    h_LF,
    h_end_CR,
    h_end_LF
};

struct requestData {
private:
    int fd;
    int again_times;
    int epollfd;
    int method;
    int http_version;
    int now_read_pos;
    int state;
    int h_state;
    bool is_finish;
    bool keep_alive;
    std::string path;
    std::string content;  //content的内容用完就清
    std::string file_name;
    std::unordered_map<std::string, std::string> headers;
    mytimer *timer;
private:
    int parse_uri();

    int parse_headers();

    int analysis_request();

public:
    requestData();

    requestData(int _epoolfd, int _fd, std::string _path);

    ~requestData();

    void add_timer(mytimer *mtimer);

    void reset();

    void separate_timer();

    int get_fd() const;

    void set_fd(int _fd);

    void handle_request();

    static void handle_error(int client, int err_no, std::string short_msg);
};

struct mytimer {
    bool deleted;
    size_t expired_time;
    requestData *request_data;

    mytimer(requestData *_request_data, int timeout);

    ~mytimer();

    void update(int timeout);

    bool is_valid();

    void clear_req();

    void set_deleted();

    bool is_deleted() const;

    size_t get_exp_time() const;
};

struct timerCmp {
    bool operator()(const mytimer *a, const mytimer *b) const;
};

void accept_request(void *arg);

void bad_request(int client);

void cannot_execute(int client);

void execute_cgi(int, const char *, const char *, const char *);

void headers(int, const char *);

void not_found(int);

void serv_file(int, const char *);

void unimplemented(int);


#endif //HIGH_PERFORMANCE_SERVER_REQUEST_DATA_H
