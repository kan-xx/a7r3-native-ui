#ifndef MENU_AUDIO_TRANSACTION_H
#define MENU_AUDIO_TRANSACTION_H
#include <stdint.h>
enum {MAT_OK=0,MAT_BUSY=2,MAT_BAD=0x201,MAT_OWNER=0x202,MAT_CLOSED=0x203,MAT_CONFLICT=0x204,MAT_TIMEOUT=0x205};
enum {MAT_IDLE,MAT_REVOKE,MAT_DRAIN,MAT_REBIND,MAT_VERIFY,MAT_DONE,MAT_FAILED,MAT_ABORT,MAT_ABORT_DRAIN};
/* Explicit one-shot source status. Zero version means this backend has no
 * capture tail; never infer continuous automatic playback from this record. */
struct mat_capture {uint32_t version,bytes,phase,error,bound,base,entry,source_generation,source_pid,floor,last_order,arm_control,offer_control,armed_serial,observed,eligible,accepted,skipped,old,duplicates,busy,audio_phase,audio_result,dropped,bad_message,last_capture,last_sequence,last_media,last_tid,last_revoke,held_reference,source_matched;};
_Static_assert(sizeof(struct mat_capture)==128,"capture observation ABI");
/* A normalized fixed report, populated only after an AP_OK/AP_PENDING reply
 * whose full F520 instance matches the host-bound owner. BUSY writes nothing. */
struct mat_observation {
 uint32_t valid,control,phase,stage,error,stop,fatal,gate,owner_phase,posted;
 uint32_t desired,qualified,generation,rebinding,supervisor_error,automatic;
 uint8_t snapshot_nonce[16];
 struct mat_capture capture;
 /* Internal adapter evidence, not an on-wire field. Only the fixed fresh-v3
  * backend may assert this before any resource initialization was accepted. */
 uint32_t empty_audio,disabled_audio;
};
struct mat_request {uint32_t op,control,asset;uint8_t nonce[16];};
struct mat_port {void*context;uint32_t(*tid)(void*);int(*dispatch)(void*,const struct mat_request*,struct mat_observation*);};
struct mat_report {
 uint32_t version,state,owner_tid,request_seq,asset,target,accepted,completed,code;
 uint32_t control,old_generation,ticks,calls,query_fresh,observed_desired,qualified,gate,automatic,new_generation;
 uint8_t nonce[16];
 struct mat_capture capture;
 uint32_t observed_control;
};
struct mat_context {struct mat_port port;struct mat_report report;uint8_t seed[16];uint32_t last_seq,revoked,aborting;};
int mat_init(struct mat_context*,const struct mat_port*,uint32_t,const uint8_t[16]);
/* This accepts UI work only; report.accepted means F520 accepted REBIND. */
int mat_submit(struct mat_context*,uint32_t asset,uint32_t sequence);
/* Exactly one port call maximum; no F4/USB/disk, no allocation, no new thread. */
int mat_tick(struct mat_context*);
int mat_cancel(struct mat_context*);
/* One fresh QUERY for idle visible-page status. It submits no configuration. */
int mat_observe(struct mat_context*);
/* Read-only power observation permits a verified live disabled backend. */
int mat_power_observe(struct mat_context*);
#endif
