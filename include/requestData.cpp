#include "requestData.h"

#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <cstring>
#include <string>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

#include <queue>
#include <unordered_map>

#include "this_epoll.h"
#include "this_util.h"

#include <iostream>
#include <utility>

using namespace std;

pthread_mutex_t qlock = PTHREAD_MUTEX_INITIALIZER;
priority_queue<mytimer *, deque<mytimer *>, timerCmp> myTimerQueue;
pthread_mutex_t MimeType::lock = PTHREAD_MUTEX_INITIALIZER;
pthread_once_t MimeType::ponce_ = PTHREAD_ONCE_INIT;
std::unordered_map<std::string, std::string> MimeType::mime;

void MimeType::init() {
    mime[".html"] = "text/html";
    mime[".avi"] = "video/x-msvideo";
    mime[".bmp"] = "image/bmp";
    mime[".c"] = "text/plain";
    mime[".doc"] = "application/msword";
    mime[".gif"] = "image/gif";
    mime[".gz"] = "application/x-gzip";
    mime[".htm"] = "text/html";
    mime[".ico"] = "application/x-ico";
    mime[".jpg"] = "image/jpeg";
    mime[".png"] = "image/png";
    mime[".txt"] = "text/plain";
    mime[".mp3"] = "audio/mp3";
    mime["default"] = "text/html";
}

std::string MimeType::getMime(const std::string &suffix) {
    pthread_once(&ponce_, MimeType::init);
    if (mime.find(suffix) == mime.end())
        return mime["default"];
    else
        return mime[suffix];
}


requestData::requestData()
        : now_read_pos(0),
          state(STATE_PARSE_URI),
          h_state(h_start),
          keep_alive(false),
          againTimes(0),
          timer(nullptr) {
    cout << "requestData constructed !" << endl;
}

requestData::requestData(int _epollfd, int _fd, std::string _path)
        : now_read_pos(0),
          state(STATE_PARSE_URI),
          h_state(h_start),
          keep_alive(false),
          againTimes(0),
          timer(nullptr),
          path(std::move(_path)),
          fd(_fd),
          epollfd(_epollfd) {}

requestData::~requestData() {
    cout << "~requestData()" << endl;
    struct epoll_event ev{};
    // 超时的一定都是读请求，没有"被动"写。
    ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
    ev.data.ptr = (void *) this;
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, &ev);
    if (timer != nullptr) {
        timer->clearReq();
        timer = nullptr;
    }
    close(fd);
}

void requestData::addTimer(mytimer *mtimer) {
    if (timer == nullptr) timer = mtimer;
}

int requestData::get_fd() const { return fd; }

void requestData::set_fd(int fd) { this->fd = fd; }

void requestData::reset() {
    againTimes = 0;
    content.clear();
    file_name.clear();
    path.clear();
    now_read_pos = 0;
    state = STATE_PARSE_URI;
    h_state = h_start;
    headers.clear();
    keep_alive = false;
}


void requestData::handleRequest() {
    char buff[MAX_BUFF];
    bool is_error = false;
    while (true) {
        int read_num = readn(fd, buff, MAX_BUFF);
        if (read_num < 0) {
            perror("1");
            is_error = true;
            break;
        } else if (read_num == 0) {
            perror("read_num == 0");
            if (errno == EAGAIN) {
                if (againTimes > AGAIN_MAX_TIMES)
                    is_error = true;
                else
                    ++againTimes;
            } else if (errno != 0)
                is_error = true;
            break;
        }
        string now_read(buff, buff + read_num);
        content += now_read;

        if (state == STATE_PARSE_URI) {
            int flag = this->parse_URI();
            if (flag == PARSE_URI_AGAIN) {
                break;
            } else if (flag == PARSE_URI_ERROR) {
                perror("2");
                is_error = true;
                break;
            }
        }
        if (state == STATE_PARSE_HEADERS) {
            int flag = this->parse_Headers();
            if (flag == PARSE_HEADER_AGAIN) {
                break;
            } else if (flag == PARSE_HEADER_ERROR) {
                perror("3");
                is_error = true;
                break;
            }
            if (method == METHOD_POST) {
                state = STATE_RECV_BODY;
            } else {
                state = STATE_ANALYSIS;
            }
        }
        if (state == STATE_RECV_BODY) {
            int content_length = -1;
            if (headers.find("Content-length") != headers.end()) {
                content_length = stoi(headers["Content-length"]);
            } else {
                is_error = true;
                break;
            }
            if (content.size() < content_length) continue;
            state = STATE_ANALYSIS;
        }
        if (state == STATE_ANALYSIS) {
            int flag = this->analysisRequest();
            if (flag < 0) {
                is_error = true;
                break;
            } else if (flag == ANALYSIS_SUCCESS) {
                state = STATE_FINISH;
                break;
            } else {
                is_error = true;
                break;
            }
        }
    }

    if (is_error) {
        delete this;
        return;
    }
    // 加入epoll继续
    if (state == STATE_FINISH) {
        if (keep_alive) {
            printf("ok\n");
            this->reset();
        } else {
            delete this;
            return;
        }
    }

    // 一定要先加时间信息，否则可能会出现刚加进去，下个in触发来了，然后分离失败后，又加入队列，
    // 最后超时被删，然后正在线程中进行的任务出错，double free错误。
    // 新增时间信息
    pthread_mutex_lock(&qlock);
    auto *mtimer = new mytimer(this, 500);
    timer = mtimer;
    myTimerQueue.push(mtimer);
    pthread_mutex_unlock(&qlock);

    __uint32_t _epo_event = EPOLLIN | EPOLLET | EPOLLONESHOT;
    int ret = epoll_mod(epollfd, fd, static_cast<void *>(this), _epo_event);
    if (ret < 0) {
        // 返回错误处理
        delete this;
        return;
    }
}

int requestData::parse_URI() {
    string &str = content;
    // 读到完整的请求行再开始解析请求
    int pos = str.find('\r', now_read_pos);
    if (pos < 0) {
        return PARSE_URI_AGAIN;
    }
    // 去掉请求行所占的空间，节省空间
    string request_line = str.substr(0, pos);
    if (str.size() > pos + 1)
        str = str.substr(pos + 1);
    else
        str.clear();
    // Method
    pos = request_line.find("GET");
    if (pos < 0) {
        pos = request_line.find("POST");
        if (pos < 0) {
            return PARSE_URI_ERROR;
        } else {
            method = METHOD_POST;
        }
    } else {
        method = METHOD_GET;
    }
    pos = request_line.find('/', pos);
    if (pos == std::string::npos) {
        return PARSE_URI_ERROR;
    } else {
        int pos2 = request_line.find(' ', pos);
        if (pos2 == std::string::npos)
            return PARSE_URI_ERROR;
        else {
            if (pos2 - pos > 1) {
                file_name = request_line.substr(pos + 1, pos2 - pos - 1);
                int pos3 = file_name.find('?');
                if (pos3 >= 0) {
                    file_name = file_name.substr(0, pos3);
                }
            } else
                file_name = "index.html";
        }
        pos = pos2;
    }
    // HTTP 版本号
    pos = request_line.find('/', pos);
    if (pos < 0) {
        return PARSE_URI_ERROR;
    } else {
        if (request_line.size() - pos <= 3) {
            return PARSE_URI_ERROR;
        } else {
            string ver = request_line.substr(pos + 1, 3);
            if (ver == "1.0")
                HTTPversion = HTTP_10;
            else if (ver == "1.1")
                HTTPversion = HTTP_11;
            else
                return PARSE_URI_ERROR;
        }
    }
    state = STATE_PARSE_HEADERS;
    return PARSE_URI_SUCCESS;
}

int requestData::parse_Headers() {
    string &str = content;
    int key_start = -1, key_end = -1;
    int value_start = -1, value_end = -1;
    int now_read_line_begin = 0;
    bool notFinish = true;

    for (int i = 0; i < str.size() && notFinish; ++i) {
        switch (h_state) {
            case h_start: {
                if (str[i] == '\n' || str[i] == '\r') break;
                h_state = h_key;
                key_start = i;
                now_read_line_begin = i;
                break;
            }
            case h_key: {
                if (str[i] == ':') {
                    key_end = i;
                    if (key_end - key_start <= 0) return PARSE_HEADER_ERROR;
                    h_state = h_colon;
                } else if (str[i] == '\n' || str[i] == '\r')
                    return PARSE_HEADER_ERROR;
                break;
            }
            case h_colon: {
                if (str[i] == ' ') {
                    h_state = h_spaces_after_colon;
                } else
                    return PARSE_HEADER_ERROR;
                break;
            }
            case h_spaces_after_colon: {
                h_state = h_value;
                value_start = i;
                break;
            }
            case h_value: {
                if (str[i] == '\r') {
                    h_state = h_CR;
                    value_end = i;
                    if (value_end - value_start <= 0) return PARSE_HEADER_ERROR;
                } else if (i - value_start > 255)
                    return PARSE_HEADER_ERROR;
                break;
            }
            case h_CR: {
                if (str[i] == '\n') {
                    h_state = h_LF;
                    string key(str.begin() + key_start, str.begin() + key_end);
                    string value(str.begin() + value_start, str.begin() + value_end);
                    headers[key] = value;
                    now_read_line_begin = i;
                } else
                    return PARSE_HEADER_ERROR;
                break;
            }
            case h_LF: {
                if (str[i] == '\r') {
                    h_state = h_end_CR;
                } else {
                    key_start = i;
                    h_state = h_key;
                }
                break;
            }
            case h_end_CR: {
                if (str[i] == '\n') {
                    h_state = h_end_LF;
                } else
                    return PARSE_HEADER_ERROR;
                break;
            }
            case h_end_LF: {
                notFinish = false;
                key_start = i;
                now_read_line_begin = i;
                break;
            }
        }
    }

    if (h_state == h_end_LF) {
        str = str.substr(now_read_line_begin);
        return PARSE_HEADER_SUCCESS;
    }
    str = str.substr(now_read_line_begin);
    return PARSE_HEADER_AGAIN;
}

void requestData::separateTimer() {
    if (timer != nullptr) {
        timer->clearReq();
        timer = nullptr;
    }
}

int requestData::analysisRequest() {
    if (method == METHOD_POST) {
        // get content
        char header[MAX_BUFF];
        sprintf(header, "HTTP/1.1 %d %s\r\n", 200, "OK");
        if (headers.find("Connection") != headers.end() &&
            headers["Connection"] == "keep-alive") {
            keep_alive = true;
            sprintf(header, "%sConnection: keep-alive\r\n", header);
            sprintf(header, "%sKeep-Alive: timeout=%d\r\n", header, EPOLL_WAIT_TIME);
        }
        const char *send_content = "I have received this.";
        sprintf(header, "%sContent-length: %zu\r\n", header, strlen(send_content));
        sprintf(header, "%s\r\n", header);
        auto send_len = (size_t) writen(fd, header, strlen(header));
        if (send_len != strlen(header)) {
            perror("Send header failed");
            return ANALYSIS_ERROR;
        }

        send_len = (size_t) writen(fd, (void *) send_content, strlen(send_content));
        if (send_len != strlen(send_content)) {
            perror("Send content failed");
            return ANALYSIS_ERROR;
        }
        cout << "content size ==" << content.size() << endl;
        vector<char> data(content.begin(), content.end());
        return ANALYSIS_SUCCESS;
    } else if (method == METHOD_GET) {
        char header[MAX_BUFF];
        sprintf(header, "HTTP/1.1 %d %s\r\n", 200, "OK");
        if (headers.find("Connection") != headers.end() &&
            headers["Connection"] == "keep-alive") {
            keep_alive = true;
            sprintf(header, "%sConnection: keep-alive\r\n", header);
            sprintf(header, "%sKeep-Alive: timeout=%d\r\n", header, EPOLL_WAIT_TIME);
        }
        int dot_pos = file_name.find('.');
        const char *filetype;
        if (dot_pos < 0)
            filetype = MimeType::getMime("default").c_str();
        else
            filetype = MimeType::getMime(file_name.substr(dot_pos)).c_str();
        struct stat sbuf{};
        if (stat(file_name.c_str(), &sbuf) < 0) {
            handleError(fd, 404, "Not Found!");
            return ANALYSIS_ERROR;
        }

        sprintf(header, "%sContent-type: %s\r\n", header, filetype);
        // 通过Content-length返回文件大小
        sprintf(header, "%sContent-length: %ld\r\n", header, sbuf.st_size);

        sprintf(header, "%s\r\n", header);
        auto send_len = (size_t) writen(fd, header, strlen(header));
        if (send_len != strlen(header)) {
            perror("Send header failed");
            return ANALYSIS_ERROR;
        }
        int src_fd = open(file_name.c_str(), O_RDONLY, 0);
        char *src_addr = static_cast<char *>(
                mmap(nullptr, sbuf.st_size, PROT_READ, MAP_PRIVATE, src_fd, 0));
        close(src_fd);

        // 发送文件并校验完整性
        send_len = writen(fd, src_addr, sbuf.st_size);
        if (send_len != sbuf.st_size) {
            perror("Send file failed");
            return ANALYSIS_ERROR;
        }
        munmap(src_addr, sbuf.st_size);
        return ANALYSIS_SUCCESS;
    } else
        return ANALYSIS_ERROR;
}

void requestData::handleError(int client, int err_num, string short_msg) {
    short_msg = " " + short_msg;
    char send_buff[MAX_BUFF];
    string body_buff, header_buff;
    body_buff += "<html><title>TKeed Error</title>";
    body_buff += "<body bgcolor=\"ffffff\">";
    body_buff += to_string(err_num) + short_msg;
    body_buff += "<hr><em> zhilong's Web Server</em>\n</body></html>";

    header_buff += "HTTP/1.1 " + to_string(err_num) + short_msg + "\r\n";
    header_buff += "Content-type: text/html\r\n";
    header_buff += "Connection: close\r\n";
    header_buff += "Content-length: " + to_string(body_buff.size()) + "\r\n";
    header_buff += "\r\n";
    sprintf(send_buff, "%s", header_buff.c_str());
    writen(client, send_buff, strlen(send_buff));
}

mytimer::mytimer(requestData *_request_data, int timeout)
        : deleted(false), request_data(_request_data) {
    struct timeval now{};
    gettimeofday(&now, nullptr);
    // 以毫秒计
    expired_time = ((now.tv_sec * 1000) + (now.tv_usec / 1000)) + timeout;
}

mytimer::~mytimer() {
    cout << "~mytimer()" << endl;
    if (request_data != nullptr) {
        cout << "request_data=" << request_data << endl;
        delete request_data;
        request_data = nullptr;
    }
}

void mytimer::update(int timeout) {
    struct timeval now{};
    gettimeofday(&now, nullptr);
    expired_time = ((now.tv_sec * 1000) + (now.tv_usec / 1000)) + timeout;
}

bool mytimer::isvalid() {
    struct timeval now{};
    gettimeofday(&now, nullptr);
    size_t temp = ((now.tv_sec * 1000) + (now.tv_usec / 1000));
    if (temp < expired_time) {
        return true;
    } else {
        this->setDeleted();
        return false;
    }
}

void mytimer::clearReq() {
    request_data = nullptr;
    this->setDeleted();
}

void mytimer::setDeleted() { deleted = true; }

bool mytimer::isDeleted() const { return deleted; }

size_t mytimer::getExpTime() const { return expired_time; }

bool timerCmp::operator()(const mytimer *a, const mytimer *b) const {
    return a->getExpTime() > b->getExpTime();
}