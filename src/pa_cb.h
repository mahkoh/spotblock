#ifndef spotblock_pa_cb_h
#define spotblock_pa_cb_h

#include "daemon.h"

namespace pa_cb {

pa_io_event *io_new(pa_mainloop_api *a, int fd, pa_io_event_flags_t events,
                    pa_io_event_cb_t cb, void *userdata);
void io_enable(pa_io_event *e, pa_io_event_flags_t events);
void io_free(pa_io_event *e);
void io_set_destroy(pa_io_event *e, pa_io_event_destroy_cb_t cb);
pa_time_event *time_new(pa_mainloop_api *a, const struct timeval *tv,
                        pa_time_event_cb_t cb, void *userdata);
void time_set_destroy(pa_time_event *e, pa_time_event_destroy_cb_t cb);
void time_restart(pa_time_event *e, const struct timeval *tv);
void time_free(pa_time_event *e);
pa_defer_event *defer_new(pa_mainloop_api *a, pa_defer_event_cb_t cb, void *userdata);
void defer_enable(pa_defer_event *e, int b);
void defer_set_destroy(pa_defer_event *e, pa_defer_event_destroy_cb_t cb);
void defer_free(pa_defer_event *e);
void context_notify(pa_context *c, void *userdata);
void sink_input_info(pa_context *c, const pa_sink_input_info *i, int eol, void *userdata);
void context_subscribe(pa_context *c, pa_subscription_event_type_t t, uint32_t idx,
                       void *userdata);

}

#endif

// vim: et:sw=4:tw=90:ts=4:sts=4:cc=+1
