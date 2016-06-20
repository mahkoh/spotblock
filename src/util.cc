#include "util.h"

namespace util {

itimerspec timeval_to_itimerspec(const struct timeval *tv) {
    return (itimerspec) {
        .it_interval = (timespec) { 0, 0 },
        .it_value = (timespec) {
            .tv_sec = tv->tv_sec,
            .tv_nsec = tv->tv_usec * 1000,
        },
    };
}

const struct timeval *timeval_to_nonnull(const struct timeval *tv) {
    if (tv == nullptr) {
        static const timeval zero = { 0, 0 };
        tv = &zero;
    }
    return tv;
}

uint32_t pa_event_flags_to_epoll_flags(pa_io_event_flags_t events) {
    uint32_t flags = 0;
    if (events & PA_IO_EVENT_INPUT)  { flags |= EPOLLIN;  }
    if (events & PA_IO_EVENT_OUTPUT) { flags |= EPOLLOUT; }
    return flags;
}

pa_io_event_flags_t epoll_flags_to_pa_event_flags(uint32_t flags) {
    int events = PA_IO_EVENT_NULL;
    if (flags & EPOLLIN)  { events |= PA_IO_EVENT_INPUT;  }
    if (flags & EPOLLOUT) { events |= PA_IO_EVENT_OUTPUT; }
    if (flags & EPOLLHUP) { events |= PA_IO_EVENT_HANGUP; }
    if (flags & EPOLLERR) { events |= PA_IO_EVENT_ERROR;  }
    return (pa_io_event_flags_t)events;
}

}

// vim: et:sw=4:tw=90:ts=4:sts=4:cc=+1
