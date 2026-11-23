#ifndef MP_PAGE_CONTROL_H
#define MP_PAGE_CONTROL_H
#include "audio_transaction.h"
#include "options.h"
#include "power_lifecycle.h"
struct mp_page_port {void*context;uint32_t(*tid)(void*);int(*draw)(void*,uint32_t,uint32_t,uint32_t);int(*close_own)(void*);int(*open_stock_audio)(void*);};
struct mp_page_report {uint32_t version,owner,serial,opens,exits,active,close_queued,original_queued,event_count,ticks,draws,requests,error;struct mp_options options;};
struct mp_page {struct mp_page_port port;struct mat_context*audio;struct mp_page_report report;uint32_t last_seq;struct lp_control*power;uint32_t empty_slot;};
int mp_page_init(struct mp_page*,const struct mp_page_port*,struct mat_context*,uint32_t);
int mp_page_open(struct mp_page*);
int mp_page_event(struct mp_page*,uint32_t serial,uint32_t event);
int mp_page_tick(struct mp_page*);
int mp_page_exit(struct mp_page*,uint32_t serial);
int mp_page_needs_tick(const struct mp_page*);
#endif
