#ifndef spotblock_util_h
#define spotblock_util_h

#include <cstdint>

#include <sys/timerfd.h>
#include <sys/epoll.h>
#include <pulse/pulseaudio.h>

namespace util {

itimerspec timeval_to_itimerspec(const struct timeval *tv);
const struct timeval *timeval_to_nonnull(const struct timeval *tv);
uint32_t pa_event_flags_to_epoll_flags(pa_io_event_flags_t events);
pa_io_event_flags_t epoll_flags_to_pa_event_flags(uint32_t flags);

}

#endif

// vim: et:sw=4:tw=90:ts=4:sts=4:cc=+1
