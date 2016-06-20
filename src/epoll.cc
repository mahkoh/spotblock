#include <string>
#include <cstring>

#include <sys/epoll.h>
#include <unistd.h>

#include "epoll.h"

int Epoll::add(int fd, struct epoll_event *event) {
    return epoll_ctl(this->fd, EPOLL_CTL_ADD, fd, event);
}

int Epoll::mod(int fd, struct epoll_event *event) {
    return epoll_ctl(this->fd, EPOLL_CTL_MOD, fd, event);
}

int Epoll::del(int fd) {
    return epoll_ctl(this->fd, EPOLL_CTL_DEL, fd, nullptr);
}

int Epoll::wait(struct epoll_event *events, int maxevents) {
    return epoll_wait(this->fd, events, maxevents, -1);
}

Epoll Epoll::create(int flags) {
    Epoll epoll = { epoll_create1(flags) };
    if (epoll.fd == -1) {
        throw "unable to create epoll: " + std::string(strerror(errno));
    }
    return epoll;
}

// vim: et:sw=4:tw=90:ts=4:sts=4:cc=+1
