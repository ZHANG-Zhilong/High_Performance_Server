//
// Created by 张志龙 on 2020/5/17.
//
#include "request_data.h"
#include "util.h"
#include <sys/mman.h>
#include <string>
#include <vector>
#include <deque>
#include <cstring>
#include <cstdio>
#include "epoll_tool.h"
#include <utility>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/epoll.h>
#include <sys/time.h>
#include <sys/socket.h>

pthread_mutex_t qlock = PTHREAD_MUTEX_INITIALIZER;

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

std::string MimeType::getMine(const std::string &suffix) {
    pthread_once(&ponce_, &MimeType::init);
    if (mime.find(suffix) == mime.end()) {
        return mime["default"];
    } else {
        return mime[suffix];
    }
}

std::priority_queue<mytimer *, std::deque<mytimer *>, timerCmp> my_timer_queue;

requestData::requestData() :
        now_read_pos(0), state(STATE_PARSE_URI), h_state(h_start),
        keep_alive(false), again_times(0), timer(nullptr) {
    std::cout << "requestData constructed! " << std::endl;
}

requestData::requestData(int _epoolfd, int _fd, std::string _path) :
        now_read_pos(0), state(STATE_PARSE_URI), h_state(h_start),
        keep_alive(false), again_times(0), timer(nullptr),
        path(std::move(_path)), fd(_fd), epollfd(_epoolfd) {}

requestData::~requestData() {
    std::cout << "~requestData()" << std::endl;
    struct epoll_event ev{};     //超时的一定是读请求，没有"写"请求
    ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
    ev.data.ptr = (void *) this;
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, &ev);
    if (timer != nullptr) {
        timer->clear_req();
        timer = nullptr;
    }
    close(fd);
}

void requestData::add_timer(mytimer *mtimer) {
    if (timer == nullptr) {
        timer = mtimer;
    }
}

int requestData::get_fd() const {
    return fd;
}

void requestData::set_fd(int _fd) {
    fd = _fd;
}

void requestData::reset() {
    again_times = 0;
    content.clear();
    file_name.clear();
    path.clear();
    now_read_pos = 0;
    state = STATE_PARSE_URI;
    h_state = h_start;
    headers.clear();
    keep_alive = false;
}

void requestData::separate_timer() {
    if (timer != nullptr) {
        timer->clear_req();
        timer = nullptr;
    }
}

int requestData::analysis_request() {
    if (method == METHOD_POST) {
        //get content
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
        auto send_len = (size_t) send(fd, header, strlen(header), 0);
        if (send_len != strlen(header)) {
            perror("send header failed");
            return ANALYSIS_ERROR;
        }
        send_len = (size_t) send(fd, send_content, strlen(send_content), 0);
        if (send_len != strlen(send_content)) {
            perror("send content failed");
            return ANALYSIS_ERROR;
        }
        printf("content size == %zu\n", content.size());
        //cv part.
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
        if (dot_pos < 0) {
            filetype = MimeType::getMine("default").c_str();
        } else {
            filetype = MimeType::getMine(file_name.substr(dot_pos)).c_str();
        }
        struct stat sbuf{};
        if (stat(file_name.c_str(), &sbuf) < 0) {
            handle_error(fd, 404, "Not Found");
            return ANALYSIS_ERROR;
        }
        sprintf(header, "%sContent-type: %s\r\n", header, filetype);
        //通过content-length返回文件大
        sprintf(header, "%sContent-length: %ld\r\n", header, sbuf.st_size);
        sprintf(header, "%s\r\n", header);
        auto send_len = (size_t) send(fd, header, strlen(header), 0);
        if (send_len != strlen(header)) {
            perror("send header failed");
            return ANALYSIS_ERROR;
        }
        int src_fd = open(file_name.c_str(), O_RDONLY, 0);
        char *src_addr = static_cast<char *> (mmap(nullptr, sbuf.st_size, PROT_READ,
                                                   MAP_PRIVATE, src_fd, 0));
        close(src_fd);

        send_len = send(fd, src_addr, sbuf.st_size, 0);  //发送文件并校验完整性
        if (send_len != sbuf.st_size) {
            perror("send file failed");
            return ANALYSIS_ERROR;
        }
        munmap(src_addr, sbuf.st_size);
        return ANALYSIS_SUCCESS;
    } else {
        return ANALYSIS_ERROR;
    }
}


void requestData::handle_request() {
    char buf[MAX_BUFF];
    bool is_error = false;
    while (true) {
        int read_num = readn(this->fd, buf, MAX_BUFF);
        if (read_num < 0) {
            perror("1");
            is_error = true;
            break;
        } else if (read_num == 0) {
            perror("read_num == 0");
            if (errno == EAGAIN) {
                if (again_times > AGAIN_MAX_TIMES) {
                    is_error = true;
                } else {
                    ++again_times;
                }
            } else if (errno != 0) {
                is_error = true;
                break;
            }
        }
        std::string now_read(buf, buf + read_num);
        content += now_read;

        if (state == STATE_PARSE_URI) {
            int flag = this->parse_uri();
            if (flag == PARSE_URI_AGAIN) {
                break;
            } else if (flag == PARSE_URI_ERROR) {
                perror("2");
                is_error = true;
                break;
            }
        }

        if (state == STATE_PARSE_HEADERS) {
            int flag = this->parse_headers();
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
                content_length = std::stoi(headers["Content-length"]);
            } else {
                is_error = true;
                break;
            }
            if (content.size() < content_length) {
                continue;
            }
            state = STATE_ANALYSIS;
        }

        if (state == STATE_ANALYSIS) {
            int flag = this->analysis_request();

            if (flag == ANALYSIS_SUCCESS) {
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

    if (state == STATE_FINISH) {
        if (keep_alive) {
            printf("ok\n");
            this->reset();
        } else {
            delete this;
            return;
        }
    }
    pthread_mutex_lock(&qlock);
    auto *mtimer = new mytimer(this, 500);
    timer = mtimer;
    my_timer_queue.push(mtimer);
    pthread_mutex_unlock(&qlock);

    uint32_t epo_event = EPOLLIN | EPOLLET | EPOLLONESHOT;
    if ((epoll_mod(epollfd, fd, static_cast<void *>(this), epo_event)) < 0) {
        delete this;
        return;
    }
}

int requestData::parse_uri() {
    std::string &str = this->content; //one line
    int pos = str.find('\r', this->now_read_pos); //读到完整的一行，解析请求line
    if (pos < 0) {
        return PARSE_URI_AGAIN;
    }
    std::string request_line = str.substr(0, pos);  //去掉请求行占据空间
    if (str.size() > pos + 1) {
        str = str.substr(pos + 1);
    } else {
        str.clear();
    }
    pos = request_line.find("GET");   //get method
    if (pos > 0) {
        this->method = METHOD_GET;
    } else {
        pos = request_line.find("POST");
        if (pos < 0) {
            return PARSE_URI_ERROR;
        } else {
            this->method = METHOD_POST;
        }
    }
    pos = request_line.find('/', pos);
    if (pos == std::string::npos) {
        return PARSE_URI_ERROR;
    }
    int pos2 = request_line.find(' ', pos);
    if (pos2 == std::string::npos) {
        return PARSE_URI_ERROR;
    }
    if (pos2 - pos > 1) {
        this->file_name = request_line.substr(pos + 1, pos2 - pos - 1); //⚠️pos2-pos
        int pos3 = file_name.find('?');
        if (pos3 >= 0) {
            file_name = file_name.substr(0, pos3);
        }
    } else {
        file_name = "index.html";
    }
    pos = pos2;

    pos = request_line.find('/', pos);   //    HTTP/1.1
    if (pos == std::string::npos) {
        return PARSE_URI_ERROR;
    }
    if (request_line.size() - pos <= 3) {
        return PARSE_URI_ERROR;
    }
    std::string ver = request_line.substr(pos + 1, 3);
    if (ver == "1.0") {
        this->http_version = HTTP_10;
    } else if (ver == "1.1") {
        this->http_version = HTTP_11;
    } else {
        return PARSE_URI_ERROR;
    }
    state = STATE_PARSE_HEADERS;
    return PARSE_URI_SUCCESS;
}

int requestData::parse_headers() {
    std::string &str = content;  //one line
    int key_start = -1, key_end = -1;
    int value_start = -1, value_end = -1;
    int now_read_line_begin = 0;
    bool not_finish = true;
    for (int i = 0; i < str.size() && not_finish; i++) {
        switch (h_state) {
            case h_start: {
                if (str[i] == '\n' || str[i] == '\r') { break; }
                h_state = h_key;
                key_start = i;
                now_read_line_begin = i;
                break;
            }
            case h_key: {
                if (str[i] == ':') {
                    key_end = i;
                    if (key_end - key_start <= 0) { return PARSE_HEADER_ERROR; }
                    h_state = h_colon;
                } else if (str[i] == '\n' || str[i] == '\r') {
                    return PARSE_HEADER_ERROR;
                }
                break;
            }
            case h_colon: {
                if (str[i] == ' ') {
                    h_state = h_spaces_after_colon;
                } else {
                    return PARSE_HEADER_ERROR;
                }
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
                    if (value_end - value_start <= 0) { return PARSE_HEADER_ERROR; }
                } else if (i - value_start > 255) {
                    return PARSE_HEADER_ERROR;
                }
                break;
            }
            case h_CR: {
                if (str[i] == '\n') {
                    h_state = h_LF;
                    std::string key(str.begin() + key_start, str.begin() + key_end);
                    std::string value(str.begin() + value_start, str.begin() + value_end);
                    headers[key] = value;
                    now_read_line_begin = i;
                } else {
                    return PARSE_HEADER_ERROR;
                }
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
                } else {
                    return PARSE_HEADER_ERROR;
                }
                break;
            }
            case h_end_LF: {
                not_finish = false;
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

void requestData::handle_error(int client, int err_no, std::string short_msg) {
    short_msg = " " + short_msg;
    char send_buff[MAX_BUFF];
    std::string body_buff, header_buff;
    body_buff += "<html><title>TKeed Error</title>";
    body_buff += "<body bgcolor=\"ffffff\">";
    body_buff += std::to_string(err_no) + short_msg;
    body_buff += "<hr><em> ZHANG's Web Server</em>\n</body></html>";

    header_buff += "HTTP/1.1 " + std::to_string(err_no) + short_msg + "\r\n";
    header_buff += "Content-type: text/html\r\n";
    header_buff += "Connection: close\r\n";
    header_buff += "Content-length: " + std::to_string(body_buff.size()) + "\r\n";
    header_buff += "\r\n";

    sprintf(send_buff, "%s", header_buff.c_str());
    send(client, send_buff, strlen(send_buff), 0);
    sprintf(send_buff, "%s", body_buff.c_str());
    send(client, send_buff, strlen(send_buff), 0);
}

mytimer::mytimer(requestData *_request_data, int timeout) :
        deleted(false), request_data(_request_data) {
    struct timeval now{};
    gettimeofday(&now, nullptr);
    expired_time = (now.tv_sec * 1000 + now.tv_usec / 1000) + timeout;
}

mytimer::~mytimer() {
    std::cout << "~mytimer()" << std::endl;
    if (request_data != nullptr) {
        std::cout << "request_data=" << request_data << std::endl;
        delete request_data;
        request_data = nullptr;
    }
}

void mytimer::update(int timeout) {
    struct timeval now{};
    gettimeofday(&now, nullptr);
    expired_time = (now.tv_sec * 1000 + now.tv_usec / 1000) + timeout;
}

bool mytimer::is_valid() {
    struct timeval now{};
    gettimeofday(&now, nullptr);
    size_t temp = now.tv_sec * 1000 + now.tv_usec / 1000;
    if (temp < expired_time) {
        return true;
    } else {
        this->set_deleted();
        return false;
    }
}

void mytimer::clear_req() {
    request_data = nullptr;
    this->set_deleted();
}

void mytimer::set_deleted() {
    deleted = true;
}

bool mytimer::is_deleted() const {
    return deleted;
}

size_t mytimer::get_exp_time() const {
    return expired_time;
}

bool timerCmp::operator()(const mytimer *a, const mytimer *b) const {
    return a->get_exp_time() > b->get_exp_time();
}

void accept_request(void *arg) {
    int client = (intptr_t) arg;
    char buf[1024];
    size_t numchars;
    char method[1024];
    char url[1024];
    char path[1024];
    size_t i, j;
    struct stat st{};
    bool cgi = false;  //becomes true is src decides this is a cgi program.
    char *query_string = nullptr;
    numchars = get_line(client, buf, sizeof(buf));
    i = 0;
    while (!ISspace(buf[i]) && i < sizeof(method) - 1) {
        method[i] = buf[i];
        i++;
    }
    j = i;
    method[i] = '\0';
    if (strcasecmp(method, "GET") != 0 || strcasecmp(method, "POST") != 0) {
        unimplemented(client);
        return;
    }
    if (strcasecmp(method, "POST") == 0) {
        cgi = true;
    }

    i = 0;
    while (ISspace(buf[j]) && j < numchars - 1) {
        j++;
    }
    while (!ISspace(buf[j]) && j < numchars - 1 && i < sizeof(url) - 1) {
        url[i++] = buf[j++];
    }
    url[i] = '\0';
    //http://www.xxx.com/Show.asp?id=77&nameid=2905210001&page=1
    //此处问号用于传递参数，将show.asp文件和后面的id、nameid、page等连接起来
    if (strcasecmp(method, "GET") == 0) {
        query_string = url;
        while ((*query_string) != '?' && (*query_string) != '\0') {
            query_string++;
        }
        if (*query_string == '?') {
            cgi = true;
            *query_string = '\0';
            query_string++;
        }
    }
    sprintf(path, "htdocs%s", url);
    if (path[strlen(path) - 1] == '/') {
        strcat(path, "index.html");
    }
    if (stat(path, &st) == -1) {
        while (numchars > 0 && strcmp("\r\n", buf) != 0) {  /*read and discard request headers*/
            numchars = get_line(client, buf, sizeof(buf));
        }
        not_found(client);
    } else {
        if (S_ISDIR(st.st_mode)) {
            strcat(path, "/index.html");
        }
        if (S_IXOTH & st.st_mode || S_IXUSR & st.st_mode || S_IXGRP & st.st_mode) {
            cgi = true;
        }
        if (!cgi) {
            serv_file(client, path);
        } else {
            execute_cgi(client, path, method, query_string);
        }
        close(client);
    }
}

void bad_request(int client) {
    char buf[1024];
    sprintf(buf, "HTTP/1.0 400 BAD REQUEST\r\r");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "Content-type:text/html\r\n");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "<P>Your browser sent a bad request, ");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "such as a POST without a Content-Length.\r\n");
    send(client, buf, sizeof(buf), 0);
}

void cannot_execute(int client) {
    char buf[1024];
    sprintf(buf, "HTTP/1.0 500 Internal Server Error\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-type:text/html\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<P>Error prohibited CGI execution.\r\n");
    send(client, buf, strlen(buf), 0);
}

void headers(int client, const char *filename) {
    char buf[2014];
    (void) filename;    //could use filename to determine file type.
    strcpy(buf, "HTTP/1.0 200 OK\r\n");
    send(client, buf, strlen(buf), 0);
    strcpy(buf, SERVER_STRING);
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    strcpy(buf, "\r\n");
    send(client, buf, strlen(buf), 0);

}

void execute_cgi(int, const char *, const char *, const char *) {}

void not_found(int client) {
    char buf[1024];
    sprintf(buf, "HTTP/1.0 404 NOT FOUND\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, SERVER_STRING);
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type:html\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<HTML><TITLE>Not Found</TITLE>\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<BODY><P>The src could not fulfill\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "your request because the resource specified\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "is unavailable or nonexistent.\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "</BODY></HTML>\r\n");
    send(client, buf, strlen(buf), 0);
}

/*****
 * send a regular file to the client. use headers and report
 * errors to client if they occur.
 * @param client a pointer to the file structure produced from
 * the socket file descriptor
 * @param filename the name of the file to the serv
 */
void serv_file(int client, const char *filename) {

    FILE *resource = nullptr;
    int numchars = 1;
    char buf[2014];
    buf[0] = 'A';
    buf[1] = '\0';

    while (numchars > 0 && strcmp("\n", buf) != 0) {  //read and discard headers.
        numchars = get_line(client, buf, sizeof(buf));
    }
    resource = fopen(filename, "r");
    if (resource == nullptr) {
        not_found(client);
    } else {
        headers(client, filename);
        cat(client, resource);
    }
    fclose(resource);
}

void unimplemented(int client) {

    char buf[1024];

    sprintf(buf, "HTTP/1.0 501 Method Not Implemented\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, SERVER_STRING);
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<HTML><HEAD><TITLE>Method Not Implemented\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "</TITLE></HEAD>\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<BODY><P>HTTP request method not supported.\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "</BODY></HTML>\r\n");
    send(client, buf, strlen(buf), 0);
}
