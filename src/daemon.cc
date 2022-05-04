#include <iostream>
#include <vector>
#include <cstring>

#include <pulse/pulseaudio.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>

#include "daemon.h"
#include "epoll.h"
#include "util.h"
#include "timer.h"
#include "pa_cb.h"
#include "dbus.h"

#define container_of(ptr, type, member) ((type *)((char *)(ptr) - offsetof(type, member)))

namespace sbdaemon {

pa_io_event *Daemon::on_pa_io_new(int fdn, pa_io_event_flags_t events,
                                  pa_io_event_cb_t cb, void *userdata)
{
    auto fd = new Fd;
    fd->type = FdType::PaIo;
    fd->pa_io.daemon = this;
    fd->pa_io.cb = cb;
    fd->pa_io.destroy_cb = nullptr;
    fd->pa_io.fd = fdn;
    fd->pa_io.userdata = userdata;

    epoll_event event;
    event.events = util::pa_event_flags_to_epoll_flags(events);
    event.data.ptr = fd;
    this->epoll.add(fdn, &event);

    return &fd->pa_io;
}

void Daemon::on_pa_io_enable(pa_io_event *e, pa_io_event_flags_t events) {
    epoll_event event;
    event.events = util::pa_event_flags_to_epoll_flags(events);
    event.data.ptr = container_of(e, Fd, pa_io);
    this->epoll.mod(e->fd, &event);
}

void Daemon::on_pa_io_free(pa_io_event *e) {
    if (e->destroy_cb != nullptr) {
        e->destroy_cb(&pa_vtable, e, e->userdata);
    }
    this->epoll.del(e->fd);
    delete container_of(e, Fd, pa_io);
}

void Daemon::on_pa_io_set_destroy(pa_io_event *e, pa_io_event_destroy_cb_t cb) {
    e->destroy_cb = cb;
}

pa_time_event *Daemon::on_pa_time_new(const struct timeval *tv, pa_time_event_cb_t cb,
                                      void *userdata)
{
    auto timer = Timer::create(CLOCK_REALTIME, TFD_CLOEXEC);
    if (tv != nullptr) {
        auto spec = util::timeval_to_itimerspec(tv);
        timer.set(TFD_TIMER_ABSTIME, &spec, nullptr);
    } else {
        tv = util::timeval_to_nonnull(tv);
    }

    auto fd = new Fd;
    fd->type = FdType::PaTime;
    fd->pa_time.daemon = this;
    fd->pa_time.cb = cb;
    fd->pa_time.destroy_cb = nullptr;
    fd->pa_time.userdata = userdata;
    fd->pa_time.tv = *tv;
    fd->pa_time.timer = timer;

    epoll_event event;
    event.events = EPOLLIN;
    event.data.ptr = fd;
    this->epoll.add(timer.fd, &event);

    return &fd->pa_time;
}

void Daemon::on_pa_time_set_destroy(pa_time_event *e, pa_time_event_destroy_cb_t cb) {
    e->destroy_cb = cb;
}

void Daemon::on_pa_time_restart(pa_time_event *e, const struct timeval *tv) {
    tv = util::timeval_to_nonnull(tv);
    auto spec = util::timeval_to_itimerspec(tv);
    e->timer.set(TFD_TIMER_ABSTIME, &spec, nullptr);
}

void Daemon::on_pa_time_free(pa_time_event *e) {
    if (e->destroy_cb != nullptr) {
        e->destroy_cb(&pa_vtable, e, e->userdata);
    }
    this->epoll.del(e->timer.fd);
    e->timer.close();
    delete container_of(e, Fd, pa_time);
}

pa_defer_event *Daemon::on_pa_defer_new(pa_defer_event_cb_t cb, void *userdata) {
    auto event = new pa_defer_event;
    event->daemon = this,
    event->enabled = true,
    event->cb = cb,
    event->userdata = userdata,
    event->destroy_cb = nullptr,
    event->freed = false,
    this->pa_deferred.push_back(event);
    return event;
}

void Daemon::on_pa_defer_enable(pa_defer_event *e, int b) {
    e->enabled = b != 0;
}

void Daemon::on_pa_defer_set_destroy(pa_defer_event *e, pa_defer_event_destroy_cb_t cb) {
    e->destroy_cb = cb;
}

void Daemon::on_pa_defer_free(pa_defer_event *e) {
    if (e->destroy_cb != nullptr) {
        e->destroy_cb(&pa_vtable, e, e->userdata);
    }
    e->freed = true;
    e->enabled = false;
}

void Daemon::handle_pa_ready() {
    this->pa_ctx.subscribe(PA_SUBSCRIPTION_MASK_SINK_INPUT, nullptr, nullptr);
    this->pa_ctx.get_sink_input_info_list(pa_cb::sink_input_info, this);
}

void Daemon::on_pa_context_notify() {
    auto state = this->pa_ctx.get_state();
    if (state == PA_CONTEXT_READY) {
        this->handle_pa_ready();
    }
}

void Daemon::handle_new_stream(uint32_t idx) {
    this->pa_ctx.get_sink_input_info(idx, pa_cb::sink_input_info, this);
}

void Daemon::on_pa_context_subscribe(pa::SubscriptionEventType t, uint32_t idx) {
    if (t.is_sink_input()) {
        if (t.is_new()) {
            this->handle_new_stream(idx);
        } else if (t.is_remove()) {
            this->handle_removed_stream(idx);
        }
    }
}

void Daemon::handle_removed_stream(uint32_t idx) {
    auto &streams = this->spotify_streams;
    for (int i = 0; i < streams.size(); i++) {
        if (streams[i] == idx) {
            streams.erase(streams.begin() + i);
            break;
        }
    }
}

void Daemon::handle_new_spotify_stream(const pa_sink_input_info *i) {
    this->spotify_streams.push_back(i->index);
    if (this->spotify_muted != i->mute) {
        this->set_stream_muted(i->index, this->spotify_muted);
        this->emit_muted_change();
    }
}

void Daemon::set_spotify_muted(bool muted) {
    for (auto stream : this->spotify_streams) {
        this->set_stream_muted(stream, muted);
    }
}

void Daemon::set_stream_muted(uint32_t idx, bool muted) {
    this->pa_ctx.set_sink_input_mute(idx, muted, nullptr, nullptr);
}

void Daemon::on_pa_sink_input_info(const pa_sink_input_info *i, int eol) {
    if (eol) {
        return;
    }

    pa::Proplist props = i->proplist;
    auto bin = props.gets("application.process.binary");
    if (bin && strcmp(bin, "spotify") == 0) {
        this->handle_new_spotify_stream(i);
    }
}

static bool trackid_is_ad(const char *c) {
    const char *magic = "/com/spotify/ad/";
    return strncmp(c, magic, strlen(magic)) == 0;
}

static void track_is_ad(dbus::Msg &m, bool *contains, bool *is) {
    *contains = false;

    m.skip("s");
    m.enter_container('a', "{sv}");
    while (!m.at_end(false)) {
        m.enter_container(0, nullptr);
        const char *name = nullptr;
        m.read_basic('s', &name);
        if (strcmp(name, "Metadata") == 0) {
            m.enter_container('v', "a{sv}");
            m.enter_container('a', "{sv}");
            while (!m.at_end(false)) {
                m.enter_container(0, nullptr);
                m.read_basic('s', &name);
                if (strcmp(name, "mpris:trackid") == 0) {
                    *contains = true;
                    m.enter_container('v', "s");
                    const char *value = nullptr;
                    m.read_basic('s', &value);
                    *is = trackid_is_ad(value);
                    return;
                }
                m.skip("v");
                m.exit_container();
            }
            return;
        }
        m.skip("v");
        m.exit_container();
    }
}

void Daemon::emit_muted_change() {
    char const * const props[] = { "Muted", nullptr };
    this->bus.emit_properties_changed_strv("/org/spotblock", "org.spotblock",
                                           (char **)props);
}

void Daemon::handle_ad_state_change(bool is_ad) {
    this->set_spotify_muted(is_ad);
    this->spotify_muted = is_ad;
    this->emit_muted_change();
}

void Daemon::handle_message(dbus::Msg &m) {
    bool contains_info;
    bool is_ad;
    track_is_ad(m, &contains_info, &is_ad);
    if (contains_info && this->spotify_muted != is_ad) {
        this->handle_ad_state_change(is_ad);
    }
}

static int spotify_properties_changed(sd_bus_message *m, void *userdata,
                                      sd_bus_error *ret_erro)
{
    auto *daemon = (sbdaemon::Daemon *)userdata;
    daemon->on_spotify_properties_changed(m, ret_erro);
    return 0;
}

void Daemon::on_spotify_properties_changed(dbus::Msg m, sd_bus_error *ret_erro) {
    try {
        this->handle_message(m);
    } catch (std::string s) {
        std::cerr << "unable to process dbus message: " << s << "\n";
    }
}

void Daemon::handle_pa_io(uint32_t flags, pa_io_event &e) {
    auto events = util::epoll_flags_to_pa_event_flags(flags);
    e.cb(&pa_vtable, &e, e.fd, events, e.userdata);
}

void Daemon::handle_pa_time(pa_time_event &e) {
    e.cb(&pa_vtable, &e, &e.tv, e.userdata);
}

void Daemon::run_pa_deferred() {
    auto &deferred = this->pa_deferred;

    for (size_t i = 0; i < deferred.size(); i++) {
        auto e = deferred[i];
        if (e->enabled) {
            e->cb(&pa_vtable, e, e->userdata);
        }
    }

    for (size_t i = 0; i < deferred.size(); i++) {
        if (deferred[i]->freed) {
            delete deferred[i];
            deferred.erase(deferred.begin() + i);
            i -= 1;
        }
    }
}

void Daemon::handle_event(epoll_event &event) {
    auto fd = (Fd *)event.data.ptr;
    switch (fd->type) {
        case FdType::PaIo:
            this->handle_pa_io(event.events, fd->pa_io);
            break;
        case FdType::PaTime:
            this->handle_pa_time(fd->pa_time);
            break;
        case FdType::Bus:
            break;
    }
}

void Daemon::run_bus() {
    while (this->bus.process(nullptr) > 0) {
    }
}

void Daemon::loop() {
    const int maxevents = 10;
    epoll_event events[maxevents];

    while (true) {
        this->run_bus();
        this->run_pa_deferred();

        int num_events = this->epoll.wait(events, maxevents);
        for (int i = 0; i < num_events; i++) {
            this->handle_event(events[i]);
        }
    }
}

void Daemon::init_pa_vtable() {
    auto &v = this->pa_vtable;
    memset(&v, 0, sizeof(v));

    v.userdata = this;

    v.io_new            = pa_cb::io_new;
    v.io_enable         = pa_cb::io_enable;
    v.io_free           = pa_cb::io_free;
    v.io_set_destroy    = pa_cb::io_set_destroy;

    v.time_new          = pa_cb::time_new;
    v.time_set_destroy  = pa_cb::time_set_destroy;
    v.time_restart      = pa_cb::time_restart;
    v.time_free         = pa_cb::time_free;

    v.defer_new         = pa_cb::defer_new;
    v.defer_set_destroy = pa_cb::defer_set_destroy;
    v.defer_enable      = pa_cb::defer_enable;
    v.defer_free        = pa_cb::defer_free;
}

void Daemon::init_pa() {
    this->init_pa_vtable();

    this->pa_ctx = pa::Context::create(&this->pa_vtable, "spotblock");
    this->pa_ctx.set_state_callback(pa_cb::context_notify, this);
    this->pa_ctx.set_subscribe_callback(pa_cb::context_subscribe, this);
    this->pa_ctx.connect(nullptr, PA_CONTEXT_NOFLAGS, nullptr);
}

int bus_get_muted(sd_bus *bus, const char *path, const char *interface,
                  const char *property, sd_bus_message *reply, void *userdata,
                  sd_bus_error *ret_error)
{
    auto daemon = (Daemon *)userdata;
    sd_bus_message_append_basic(reply, 'b', &daemon->spotify_muted);
    return 1;
}

void Daemon::init_bus_vtable() {
    auto &v = this->bus_vtable;

    memset(&v, 0, sizeof(v));

    v[0].type = _SD_BUS_VTABLE_START;
    v[0].x.start.element_size = sizeof(sd_bus_vtable);

    v[1].type = _SD_BUS_VTABLE_PROPERTY;
    v[1].flags = SD_BUS_VTABLE_PROPERTY_EMITS_CHANGE;
    v[1].x.property.member = "Muted";
    v[1].x.property.signature = "b";
    v[1].x.property.get = bus_get_muted;

    v[2].type = _SD_BUS_VTABLE_END;
}

void Daemon::init_bus() {
    this->init_bus_vtable();

    this->bus = dbus::Bus::default_user();
    auto &bus = this->bus;

    auto fd = new Fd;
    fd->type = FdType::Bus;

    epoll_event event;
    event.events = EPOLLIN;
    event.data.ptr = fd;
    this->epoll.add(bus.get_fd(), &event);

    const char *match = "type='signal',"
                        "sender='org.mpris.MediaPlayer2.spotify',"
                        "member='PropertiesChanged'";

    bus.add_match(nullptr, match, spotify_properties_changed, this);

    bus.add_object_vtable(nullptr, "/org/spotblock", "org.spotblock", this->bus_vtable,
                          this);

    bus.request_name("org.spotblock", 0);
}

void Daemon::init() {
    this->spotify_muted = false;
    this->epoll = Epoll::create(0);
    this->init_bus();
    this->init_pa();
}

}

// vim: et:sw=4:tw=90:ts=4:sts=4:cc=+1
