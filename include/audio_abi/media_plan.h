#ifndef A7_MEDIA_PLAN_H
#define A7_MEDIA_PLAN_H
#include "media_sample.h"
#include "ap_f4_source.h"
/* Pure serialized-supervisor plan. No native calls, shared gate, epoch producer,
 * audio submission or file access. Caller owns all storage and supplies trusted
 * F4/copy receipts obtained through exact pinned APIs, never SD/user claims. */
enum {MP_OK,MP_STALE,MP_BUSY,MP_ARGUMENT,MP_NEEDS_CANCEL,MP_CLOSED,MP_HOLD};
enum {MP_WAIT,MP_NEED_REFRESH,MP_READING,MP_POSTCHECK,MP_VERIFIED,MP_FAILED,MP_HELD};
enum {MP_USB=1,MP_MEDIA=2,MP_POWER=3,MP_REFRESH=4,MP_SOURCE=5,MP_GAP=6,MP_SHUTDOWN=7};
struct mp_ticket {uint32_t serial,prior_generation,request_control,expected_control;uint8_t f4_instance[16],nonce[16];};
struct mp_plan {uint32_t initialized,owner,serial,phase,readable,cancel_needed,last_sequence,reason,prior_generation,request_control,source_verified;uint8_t f4_instance[16];struct mr_sample last;struct mp_ticket ticket;struct ap_f4_source_meta source;};
int mp_init(struct mp_plan*,uint32_t owner,const uint8_t f4_instance[16]);
/* Callback must first close the actual audio owner's own gate through its proven
 * bounded revocation input. Then supervisor applies this plan notification.
 * serial is a LOCAL cancellation serial, never original media identity/epoch. */
int mp_revoke(struct mp_plan*,uint32_t owner,uint32_t reason);
/* issued_serial is captured before starting the current native sample. A revoke
 * during that sample makes it stale. Sequence gaps close permanently. Replayed
 * samples cannot qualify. All sample calls are on the same supervisor worker. */
int mp_sample(struct mp_plan*,uint32_t owner,uint32_t issued_serial,uint32_t sequence,const struct mr_sample*);
/* Receipt of exact same F4 instance: CANCEL succeeded and QUERY says valid=0,
 * no pending/native worker or HOLD/STOP. Input adapter enforces those conditions;
 * this pure planner stores its observed generation/control after that barrier. */
int mp_cancel_drained(struct mp_plan*,uint32_t owner,uint32_t serial,uint32_t f4_generation,uint32_t f4_control);
int mp_begin(struct mp_plan*,uint32_t owner,const uint8_t fresh_nonce[16],struct mp_ticket*);
/* Copy must follow the newly accepted F4 refresh, ap_f4_copy success, and full
 * known content validation. It does NOT publish audio. One later fresh native
 * sample on this same owner is required before source_verified becomes1. */
int mp_copied(struct mp_plan*,uint32_t owner,const struct mp_ticket*,const struct ap_f4_source_meta*);
int mp_fail(struct mp_plan*,uint32_t owner,uint32_t serial);
#endif
