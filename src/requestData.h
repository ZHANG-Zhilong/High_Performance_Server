#ifndef REQUESTDATA
#define REQUESTDATA

#include <string>
#include <unordered_map>

const int STATE_PARSE_URI = 1;
const int STATE_PARSE_HEADERS = 2;
const int STATE_RECV_BODY = 3;
const int STATE_ANALYSIS = 4;
const int STATE_FINISH = 5;

const int MAX_BUFF = 4096;

// 有请求出现但是读不到数据,可能是Request Aborted,
// 或者来自网络的数据没有达到等原因,
// 对这样的请求尝试超过一定的次数就抛弃
const int MAX_READ_TIMES = 200;
const int PARSE_URI_AGAIN = -1;
const int PARSE_URI_ERROR = -2;
const int PARSE_URI_SUCCESS = 0;

const int PARSE_HEADER_AGAIN = -1;
const int PARSE_HEADER_ERROR = -2;
const int PARSE_HEADER_SUCCESS = 0;

const int ANALYSIS_ERROR = -2;
const int ANALYSIS_SUCCESS = 0;

const int METHOD_POST = 1;
const int METHOD_GET = 2;
const int HTTP_10 = 1;
const int HTTP_11 = 2;

const int EPOLL_WAIT_TIME = 500;

class MimeType {
private:
    static pthread_once_t ponce_;
    static std::unordered_map<std::string, std::string> mime;

    static void init();

    MimeType() = default;


public:
    MimeType(const MimeType &m) = delete;

    MimeType &operator=(const MimeType &) = delete;

    static std::string getMime(const std::string &suffix);
};

class Method {
private:
    static std::unordered_map<std::string, int> method_code;
    static pthread_once_t ponce_;

    Method() = default;

    static void init();

public:
    Method(const Method &) = delete;

    Method &operator=(const Method &) = delete;

    static int getMethod(const std::string &method);
};


struct timer_stamp;
struct requestData;

struct requestData {
private:
    int readTimes;
    int fd{};
    int epollfd{};
    // content的内容用完就清
    std::string path;
    std::string content;
    std::string file_name;
    int method;
    int HTTPversion{};
    int now_read_pos;
    int state;
    int h_state;
    bool is_finish{};
    bool keep_alive;
    std::unordered_map<std::string, std::string> headers;
    timer_stamp *timer;

private:
    int parse_request_line();

    int parse_request_head();

    int analysisRequest();


public:

    ~requestData();

    requestData();

    requestData(int _epollfd, int _fd, std::string _path);

    void addTimer(timer_stamp *mtimer);

    void reset();

    void separateTimer();

    int get_fd() const;

    void set_fd(int _fd);

    void handleRequest();

    static void handleError(int client, int err_num, std::string short_msg);
};

struct timer_stamp {
    bool deleted;
    size_t expired_time;
    requestData *request_data;

    timer_stamp(requestData *_request_data, int timeout);

    ~timer_stamp();

    void update(int timeout);

    bool isvalid();

    void clearReq();

    void setDeleted();

    bool isDeleted() const;

    size_t getExpTime() const;
};

struct timerCmp {
    bool operator()(const timer_stamp *a, const timer_stamp *b) const;
};

#endif