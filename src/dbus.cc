#include <string>
#include <cstring>

#include "dbus.h"

namespace dbus {

Bus Bus::default_user() {
    Bus bus;
    int rv = sd_bus_default_user(&bus.bus);
    if (rv < 0) {
        throw "unable to acquire default bus: " + std::string(strerror(-rv));
    }
    return bus;
}

int Bus::get_fd() {
    return sd_bus_get_fd(this->bus);
}

int Bus::process(sd_bus_message **r) {
    return sd_bus_process(this->bus, r);
}

int Bus::add_match(sd_bus_slot **slot, const char *match,
                   sd_bus_message_handler_t callback, void *userdata)
{
    return sd_bus_add_match(this->bus, slot, match, callback, userdata);
}

void Bus::request_name(const char *name, uint64_t flags) {
    int rv = sd_bus_request_name(this->bus, name, flags);
    if (rv <= 0) {
        throw "unable to acquire dbus bus name: " + std::string(strerror(-rv));
    }
}

int Bus::add_object_vtable(sd_bus_slot **slot, const char *path, const char *interface,
                           const sd_bus_vtable *vtable, void *userdata)
{
    return sd_bus_add_object_vtable(this->bus, slot, path, interface, vtable, userdata);
}

int Bus::emit_properties_changed_strv(const char *path, const char *interface,
                                      char **names)
{
    return sd_bus_emit_properties_changed_strv(this->bus, path, interface, names);
}

Msg::Msg(sd_bus_message *m) : m{m}, owned{false} {
}

Msg::Msg(Msg&& m) : m{m.m}, owned{m.owned} {
    m.owned = false;
}

Msg &Msg::operator=(Msg&& m) {
    std::swap(this->m, m.m);
    std::swap(this->owned, m.owned);
    return *this;
}

Msg::~Msg() {
    if (this->m != nullptr && this->owned) {
        sd_bus_message_unref(this->m);
    }
}

static void check_rv(const char *name, int rv) {
    if (rv <= 0) {
        throw name + std::string(" failed: ") + std::string(strerror(-rv));
    }
}

void Msg::read_basic(char type, void *p) {
    check_rv("read_basic", sd_bus_message_read_basic(this->m, type, p));
}

void Msg::skip(const char *types) {
    check_rv("skip", sd_bus_message_skip(this->m, types));
}

void Msg::enter_container(char type, const char *contents) {
    check_rv("enter_container", sd_bus_message_enter_container(this->m, type, contents));
}

bool Msg::at_end(bool complete) {
    return sd_bus_message_at_end(this->m, complete);
}

void Msg::exit_container() {
    check_rv("exit_container", sd_bus_message_exit_container(this->m));
}

}

// vim: et:sw=4:tw=90:ts=4:sts=4:cc=+1
