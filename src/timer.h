#ifndef spotblock_timer_h
#define spotblock_timer_h

struct Timer {
    int fd;

    int set(int flags, const struct itimerspec *new_value, struct itimerspec *old_value);
    int get(struct itimerspec *curr_value);

    void close();

    static Timer create(int clockid, int flags);
};

#endif

// vim: et:sw=4:tw=90:ts=4:sts=4:cc=+1
