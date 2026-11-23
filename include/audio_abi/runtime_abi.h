#ifndef AP_RUNTIME_ABI_H
#define AP_RUNTIME_ABI_H
#include "rearm.h"
#include "audio_worker.h"
#include "source_guard.h"
#include "supervisor.h"
#include "capture_source.h"
#include "deferred_offer.h"
#define AP_ABI 1u
#define AP_BUILD 0x3110d10cu
#define AP_PATH "/usr/lib/liba7r_capture_fresh_v12.so"
#define AP_SYMBOL "a7r_capture_fresh_v12"
#define AP_REQUEST_BYTES 320u
#define AP_REPLY_BYTES 1216u
#define AP_NOLOAD 6
/* Fresh resources and continuous capture admission; no host PLAY/ARM.
 * Cold START: source_generation=0, source_control=4, media all zero, plus
 * nonzero source instance and a fresh nonce. The exact pristine F4 object
 * is verified before any refresh. It grants no cached source or playback.
 * START reserved[5]=1 pauses epoch 1 before thread creation; value 0 retains
 * diagnostic startup behavior. Other operations require this field zero. */
enum {AP_START=1,AP_QUERY=2,AP_PLAY=3,AP_STOP=4,AP_REVOKE=5,AP_REBIND=6};
enum {AP_COLD,AP_STARTING,AP_READY,AP_WAITING,AP_DORMANT_PINNED,AP_HOLD_PINNED,AP_DISABLED_PINNED};
enum {AP_OK=0,AP_PENDING=1,AP_BUSY=2,AP_SKIP=3,AP_DUPLICATE=4,AP_ABSENT=0x100,AP_ABI_ERROR=0x101,AP_STATE_ERROR=0x102,AP_BIND_ERROR=0x103,AP_SOURCE_ERROR=0x104,AP_NATIVE_ERROR=0x105,AP_INSTANCE_ERROR=0x106,AP_STALE=0x107,AP_CLOSE_ERROR=0x108};
struct ab_identity {uint8_t instance[16],source_instance[16],source_nonce[16],media[24];uint32_t source_generation,source_control,epoch,worker_tid;};
struct ap_request {uint32_t op,control,asset,epoch;uint8_t instance[16],source_instance[16],source_nonce[16];uint32_t source_generation,source_control;uint8_t media[24];uint32_t reserved[8];struct ab_identity retired,predecessor;};
_Static_assert(sizeof(struct ap_request)==304,"control ABI");
struct ap_runtime_status {uint32_t abi,bytes,phase,stage,error,thread_created,thread_exited,thread_joined,worker_tid,stop_requested,owns_self,owns_provider_mask,automatic_admission,bind_provider,bind_symbol,bind_detail;uint32_t control,last_play_control,posted_control,skipped_posts,control_result,source_copied,resource_pinned,loop_sid,loop_phase,loop_received,loop_free_failed,worker_ticks,wait_ticks,timeout_ms,ref_uncertain,posted_asset;uint8_t instance[16];uint32_t provider_bases[4];};
_Static_assert(sizeof(struct ap_runtime_status)==160,"status ABI");
struct ap_policy_status {uint32_t path_guard,path,adf_guard,adf_state,active,cached48,cached4c,pending,deferred,delay,stable,anchors,glee_guard,glee_vptr,output_vptr,output_mask,queue_count,queue_head_tail,power20,power22;};
struct fr_status {uint32_t pinned,qualified,checks,last_detail,allocation_attempts,registration_attempts,prepared_count,registered_count,partial_hold,table_checks,proc_checks,payload_checks,stock_checks,owned_mode,gate,local_revoke_serial;};
struct ap_report {struct ap_runtime_status runtime;struct cx_audio_status audio[2];struct a7r_status owner;struct ap_f4_source_meta source;struct ap_policy_status policy;struct fr_status resources;struct sv_status supervisor;struct ca_status capture;struct df_status deferred;};
_Static_assert(sizeof(struct ap_report)==1052,"report ABI");
_Static_assert(offsetof(struct ap_report,source)==428,"source offset");
_Static_assert(offsetof(struct ap_report,policy)==636,"policy offset");
struct ap_api {uint32_t abi,bytes,build;int(*start)(void*,const struct ap_request*,uint32_t*,struct ap_report*);int(*dispatch)(const struct ap_request*,struct ap_report*);};
typedef const struct ap_api*(*ap_get_api)(void);
struct ap_packet {const uint8_t*input;uint32_t size,capacity;const uint8_t*output;uint32_t length;void(*freefn)(void*);};
#endif
