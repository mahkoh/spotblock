#include <iostream>
#include <cstring>

#include <sys/timerfd.h>
#include <unistd.h>

#include "timer.h"

int Timer::set(int flags, const struct itimerspec *new_value,
               struct itimerspec *old_value)
{
    return timerfd_settime(this->fd, flags, new_value, old_value);
}

int Timer::get(struct itimerspec *curr_value) {
    return timerfd_gettime(this->fd, curr_value);
}

Timer Timer::create(int clockid, int flags) {
    Timer timer { timerfd_create(clockid, flags) };
    if (timer.fd == -1) {
        std::cerr << "unable to create timer: " << strerror(errno) << "\n";
    }
    return timer;
}

void Timer::close() {
    ::close(this->fd);
    this->fd = -1;
}

// vim: et:sw=4:tw=90:ts=4:sts=4:cc=+1
