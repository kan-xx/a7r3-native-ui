#ifndef CX_AUDIO_WORKER_H
#define CX_AUDIO_WORKER_H
typedef unsigned cx_u32;
enum cx_audio_phase { CX_EMPTY=0, CX_ALLOCATING=1, CX_OWN_OPEN=2, CX_PREPARED=3,
 CX_REGISTER_PENDING=4, CX_READY=5, CX_UNREGISTER_PENDING=6, CX_FREE_PENDING=7,
 CX_CLEANED=8, CX_POISONED=9 };
struct cx_audio_status {
 cx_u32 magic,version,phase,epoch,handle,physical,capacity,audio_bytes;
 cx_u32 last_native,reason,free_attempts,table_attempts,sid,send_rc,held_message,cleanup_verified;
};
struct ap_audio_api {int(*prepare)(unsigned,unsigned);int(*set_source)(unsigned);int(*register_table)(void);int(*refresh)(void);int(*status)(struct cx_audio_status*);int(*rebind_ready)(const unsigned*);void(*rebind_commit)(void);int(*domain_probe)(const unsigned*,const unsigned*,unsigned*);void(*domain_commit)(unsigned,const unsigned*,const unsigned*);int(*domain_close)(unsigned);int(*verify_snapshot)(const unsigned*,const char*);};
extern const struct ap_audio_api ap_audio[2];
#endif
