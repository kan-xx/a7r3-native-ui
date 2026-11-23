#ifndef A7_MENU_POWER_H
#define A7_MENU_POWER_H
#include "power_lifecycle.h"
/* This UI component never sends IM completion, releases providers or grants
 * global suspend. Query establishes its own original timer-return boundary. */
struct mp_power_report {uint32_t abi,bytes,build,pid,owner,error,ui_epoch,wake_epoch,callback_returned,route_suspended,view_live,transaction_pending,reference_retained;struct lp_status control;};
struct mp_power_api {uint32_t abi,bytes,build;int(*pause)(const uint8_t*,uint32_t);int(*resume)(const uint8_t*,uint32_t);int(*query)(struct mp_power_report*);};
_Static_assert(sizeof(struct mp_power_report)==148,"menu power report");
#endif
