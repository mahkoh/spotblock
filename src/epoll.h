#ifndef spotblock_epoll_h
#define spotblock_epoll_h

struct Epoll {
    int fd;

    int add(int fd, struct epoll_event *event);
    int mod(int fd, struct epoll_event *event);
    int del(int fd);

    int wait(struct epoll_event *events, int maxevents);

    static Epoll create(int flags);
};

#endif

// vim: et:sw=4:tw=90:ts=4:sts=4:cc=+1
