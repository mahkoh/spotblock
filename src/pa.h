#ifndef spotblock_pa_h
#define spotblock_pa_h

#include <pulse/pulseaudio.h>

namespace pa {

struct SubscriptionEventType {
    pa_subscription_event_type_t t;

    bool is_new();
    bool is_remove();
    bool is_sink_input();

    SubscriptionEventType(pa_subscription_event_type_t);
};

struct Operation {
    pa_operation *op;

    Operation(pa_operation *);
    Operation(const Operation&) =delete;
    Operation &operator=(const Operation&) =delete;
    Operation(Operation&&);
    Operation &operator=(Operation&&);
    ~Operation();
};

struct Proplist {
    pa_proplist *p;

    const char *gets(const char *key);

    Proplist(pa_proplist *);
};

struct Context {
    pa_context *ctx;

    void set_state_callback(pa_context_notify_cb_t cb, void *userdata);
    void set_subscribe_callback(pa_context_subscribe_cb_t cb, void *userdata);
    int connect(const char *server, pa_context_flags_t flags, const pa_spawn_api *api);
    Operation subscribe(pa_subscription_mask_t m, pa_context_success_cb_t cb,
                        void *userdata);
    Operation get_sink_input_info_list(pa_sink_input_info_cb_t cb, void *userdata);
    Operation get_sink_input_info(uint32_t idx, pa_sink_input_info_cb_t cb,
                                  void *userdata);
    pa_context_state_t get_state();
    Operation set_sink_input_mute(uint32_t idx, int mute, pa_context_success_cb_t cb,
                                  void *userdata);

    static Context create(pa_mainloop_api *mainloop, const char *name);
};

}

#endif

// vim: et:sw=4:tw=90:ts=4:sts=4:cc=+1
