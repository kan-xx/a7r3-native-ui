#ifndef A7_REARM_H
#define A7_REARM_H
#include <stdint.h>
#include <stddef.h>
#include "snapshot_abi.h"
/* Core state machine; runtime.c owns original bindings and one worker.
 * Native adapter must implement all ownership preconditions in CONTRACT.txt.
 * One owner TID, one immutable resource/Kind10, one lifetime reply SID.
 * Reply ACK is not a producer return/unload fence. This module never releases
 * resource, SID, snapshot or module. Existing F3 remains untouched.
 */
enum {A7R_EMPTY,A7R_IDLE,A7R_WAIT,A7R_ACK_DRAIN,A7R_HOLD,A7R_DORMANT};
enum {A7R_OK=0,A7R_PENDING=1,A7R_SKIP_BUSY=2,A7R_SKIP_INELIGIBLE=3,
 A7R_BAD=-1,A7R_OWNER=-2,A7R_LOCKED=-3,A7R_FAULT=-4,A7R_LIMIT=-5};
struct a7r_resource {uint32_t epoch,source_sid,reply_sid,handle,physical,capacity,asset_id,audio_bytes,kind;};
struct a7r_ops {
 uint32_t(*tid)(void*);
 /* 0 means same exact qualified immutable DMM/table/source epoch AND naturally
  * path3 currently eligible. 1 means skip. Negative is identity/ownership fault.
  * This callback cannot SET_PATH, STOP, replace buffers or repair qualification. */
 int(*guard)(void*,const struct a7r_resource*);
 int(*alloc)(void*,uint32_t,void**,uint32_t,uint32_t);
 int(*send)(void*,uint32_t,void*);
 int(*recv)(void*,uint32_t,void**,int);
 int(*size)(void*,void*);
 int(*free)(void*,uint32_t,void*);
};
struct a7r_status {
 uint32_t phase,owner_tid,epoch,reply_sid,source_sid,kind,asset_id;
 uint32_t ticket,last_event,accepted,completed,skip_busy,skip_ineligible;
 uint32_t receive_count,free_attempts,held_message,stop_requested,last_source,last_capacity;
 int32_t reason,last_native;uint32_t last_reply[9];
 /* These must never be advertised as capture or hardware qualification. */
 uint32_t reply_visible_not_producer_return,resource_release_supported,source_qualified;
 uint32_t skipped_or_preempted,skipped_reserved;
};
struct a7r_owner {
 uint32_t lock,attempted;
 struct {uint32_t sid,active,kind,event,started_ms;} slots[2];struct a7r_ops ops;void*ctx;struct a7r_resource resource,resources[2];
 struct a7r_status status;struct a7_snapshot_manifest_v1 manifest;uint8_t immutable[A7_SNAPSHOT_BYTES];
};
/* Ownership of an already allocated/loopback-verified reply queue and READY
 * DMM is supplied by the caller. Never implicitly allocates queue/DMM. */
int a7r_retarget(struct a7r_owner*,const struct a7r_resource*,const struct a7_snapshot_manifest_v1*,const uint8_t*,size_t);
int a7r_init(struct a7r_owner*,const struct a7r_ops*,void*,const struct a7r_resource*,const struct a7_snapshot_manifest_v1*,const uint8_t*,size_t);
int a7r_secondary(struct a7r_owner*,uint32_t sid);
int a7r_offer(struct a7r_owner*,uint32_t event_audit,uint32_t asset,uint32_t now_ms);
int a7r_expired(struct a7r_owner*,uint32_t now_ms,uint32_t limit_ms);
int a7r_poll(struct a7r_owner*);
/* Closes acceptance. WAIT still must be polled; final DORMANT retains everything. */
int a7r_stop_accepting(struct a7r_owner*);
int a7r_status(struct a7r_owner*,struct a7r_status*);
#endif
