#ifndef MP_OPTIONS_H
#define MP_OPTIONS_H
#include <stdint.h>
enum{MP_NONE,MP_REDRAW,MP_CHOOSE,MP_REFRESH,MP_STOCK_AUDIO,MP_BACK,MP_EMPTY};
struct mp_options{uint32_t active,row,held,pending,last_event,ignored;};
struct mp_action{uint32_t action,row;};
void mp_options_open(struct mp_options*,uint32_t);
int mp_options_owns(uint32_t);
struct mp_action mp_options_event(struct mp_options*,uint32_t);
#endif
