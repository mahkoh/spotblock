#ifndef spotblock_dbus_h
#define spotblock_dbus_h

#include <systemd/sd-bus.h>

namespace dbus {

struct Bus {
    sd_bus *bus;

    int get_fd();
    int process(sd_bus_message **r);
    int add_match(sd_bus_slot **slot, const char *match,
                  sd_bus_message_handler_t callback, void *userdata);
    void request_name(const char *name, uint64_t flags);
    int add_object_vtable(sd_bus_slot **slot, const char *path, const char *interface,
                          const sd_bus_vtable *vtable, void *userdata);
    int emit_properties_changed_strv(const char *path, const char *interface,
                                     char **names);

    static Bus default_user();
};

struct Msg {
    sd_bus_message *m;
    bool owned;

    void read_basic(char type, void *p);
    void skip(const char *types);
    void enter_container(char type, const char *contents);
    bool at_end(bool complete);
    void exit_container();

    Msg(sd_bus_message *);
    Msg(const Msg&) =delete;
    Msg &operator=(const Msg&) =delete;
    Msg(Msg&&);
    Msg &operator=(Msg&&);
    ~Msg();
};

}

#endif

// vim: et:sw=4:tw=90:ts=4:sts=4:cc=+1
