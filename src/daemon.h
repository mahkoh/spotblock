#ifndef spotblock_daemon_h
#define spotblock_daemon_h

#include <vector>

#include <pulse/pulseaudio.h>

#include "timer.h"
#include "epoll.h"
#include "pa.h"
#include "dbus.h"

namespace daemon {

struct Daemon;

}

struct pa_io_event {
    daemon::Daemon *daemon;
    pa_io_event_cb_t cb;
    pa_io_event_destroy_cb_t destroy_cb;
    void *userdata;
    int fd;
};

struct pa_time_event {
    daemon::Daemon *daemon;
    pa_time_event_cb_t cb;
    pa_time_event_destroy_cb_t destroy_cb;
    void *userdata;
    timeval tv;
    Timer timer;
};

struct pa_defer_event {
    daemon::Daemon *daemon;
    pa_defer_event_cb_t cb;
    pa_defer_event_destroy_cb_t destroy_cb;
    void *userdata;
    bool enabled;
    bool freed;
};

namespace daemon {

enum class FdType {
    PaIo,
    PaTime,
    Bus,
};

struct Daemon;

struct Fd {
    FdType type;
    union {
        pa_io_event pa_io;
        pa_time_event pa_time;
    };
};

struct Daemon {
    Epoll epoll;
    pa::Context pa_ctx;
    std::vector<pa_defer_event *> pa_deferred;
    pa_mainloop_api pa_vtable;
    std::vector<uint32_t> spotify_streams;
    bool spotify_muted;
    dbus::Bus bus;
    sd_bus_vtable bus_vtable[3];

    pa_io_event *on_pa_io_new(int fd, pa_io_event_flags_t events, pa_io_event_cb_t cb,
                              void *userdata);
    void on_pa_io_enable(pa_io_event *e, pa_io_event_flags_t events);
    void on_pa_io_free(pa_io_event *e);
    void on_pa_io_set_destroy(pa_io_event *e, pa_io_event_destroy_cb_t cb);
    pa_time_event *on_pa_time_new(const struct timeval *tv, pa_time_event_cb_t cb,
                                  void *userdata);
    void on_pa_time_set_destroy(pa_time_event *e, pa_time_event_destroy_cb_t cb);
    void on_pa_time_restart(pa_time_event *e, const struct timeval *tv);
    pa_defer_event *on_pa_defer_new(pa_defer_event_cb_t cb, void *userdata);
    void on_pa_time_free(pa_time_event *e);
    void on_pa_defer_enable(pa_defer_event *e, int b);
    void on_pa_defer_set_destroy(pa_defer_event *e, pa_defer_event_destroy_cb_t cb);
    void on_pa_defer_free(pa_defer_event *e);

    void on_pa_context_notify();
    void on_pa_context_subscribe(pa::SubscriptionEventType t, uint32_t idx);
    void on_pa_sink_input_info(const pa_sink_input_info *i, int eol);

    void on_spotify_properties_changed(dbus::Msg m, sd_bus_error *ret_erro);

    void run_bus();
    void run_pa_deferred();

    void set_stream_muted(uint32_t idx, bool muted);
    void set_spotify_muted(bool muted);

    void handle_ad_state_change(bool is_ad);
    void handle_message(dbus::Msg &m);
    void handle_new_spotify_stream(const pa_sink_input_info *i);
    void handle_new_real_spotify_stream(const pa_sink_input_info *i);
    void handle_new_stream(uint32_t idx);
    void handle_removed_stream(uint32_t idx);
    void handle_pa_io(uint32_t events, pa_io_event &e);
    void handle_pa_time(pa_time_event &e);
    void handle_event(epoll_event &event);
    void handle_pa_ready();

    void emit_muted_change();

    void init_pa_vtable();
    void init_pa();
    void init_bus_vtable();
    void init_bus();
    void init();
    [[noreturn]] void loop();
};

}

#endif

// vim: et:sw=4:tw=90:ts=4:sts=4:cc=+1
