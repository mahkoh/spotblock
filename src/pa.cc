#include <utility>

#include "pa.h"

namespace pa {

Context Context::create(pa_mainloop_api *mainloop, const char *name) {
    Context context { pa_context_new(mainloop, name) };
    assert(context.ctx != nullptr);
    return context;
}

void Context::set_state_callback(pa_context_notify_cb_t cb, void *userdata) {
    pa_context_set_state_callback(this->ctx, cb, userdata);
}

void Context::set_subscribe_callback(pa_context_subscribe_cb_t cb, void *userdata) {
    pa_context_set_subscribe_callback(this->ctx, cb, userdata);
}

int Context::connect(const char *server, pa_context_flags_t flags,
                     const pa_spawn_api *api)
{
    return pa_context_connect(this->ctx, server, flags, api);
}

Operation Context::subscribe(pa_subscription_mask_t m, pa_context_success_cb_t cb,
                             void *userdata)
{
    return pa_context_subscribe(this->ctx, m, cb, userdata);
}

Operation Context::get_sink_input_info_list(pa_sink_input_info_cb_t cb, void *userdata) {
    return pa_context_get_sink_input_info_list(this->ctx, cb, userdata);
}

Operation Context::get_sink_input_info(uint32_t idx, pa_sink_input_info_cb_t cb,
                                       void *userdata)
{
    return pa_context_get_sink_input_info(this->ctx, idx, cb, userdata);
}

pa_context_state_t Context::get_state() {
    return pa_context_get_state(this->ctx);
}

Operation Context::set_sink_input_mute(uint32_t idx, int mute, pa_context_success_cb_t cb,
                                       void *userdata)
{
    return pa_context_set_sink_input_mute(this->ctx, idx, mute, cb, userdata);
}

Operation::Operation(pa_operation *op) : op{op} {
}

Operation::~Operation() {
    if (this->op != nullptr) {
        pa_operation_unref(this->op);
    }
}

Operation::Operation(Operation&& o) : op(o.op) {
    o.op = nullptr;
}

Operation &Operation::operator=(Operation&& o) {
    std::swap(this->op, o.op);
    return *this;
}

SubscriptionEventType::SubscriptionEventType(pa_subscription_event_type_t t) :
    t{t}
{
}

bool SubscriptionEventType::is_new() {
    return (t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) == PA_SUBSCRIPTION_EVENT_NEW;
}

bool SubscriptionEventType::is_remove() {
    return (t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) == PA_SUBSCRIPTION_EVENT_REMOVE;
}

bool SubscriptionEventType::is_sink_input() {
    return (t & PA_SUBSCRIPTION_EVENT_FACILITY_MASK) == PA_SUBSCRIPTION_EVENT_SINK_INPUT;
}

Proplist::Proplist(pa_proplist *p) : p{p} {
}

const char *Proplist::gets(const char *c) {
    return pa_proplist_gets(this->p, c);
}

}

// vim: et:sw=4:tw=90:ts=4:sts=4:cc=+1
