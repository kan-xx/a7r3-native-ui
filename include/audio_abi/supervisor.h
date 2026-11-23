#ifndef AB2_SUPERVISOR_H
#define AB2_SUPERVISOR_H
#include "admission.h"
#include "media_plan.h"
struct sv_status {uint32_t phase,error,word,fatal,revokes,commits,rejected,desired,bind_generation,bind_control,samples,media_reason,media_available,f4_calls,refreshes,qualified,usb_queue,usb_callbacks,usb_active,usb_target,usb_error,last_reason,transaction_word,rebinding;};
struct sv_context {struct adm_gate gate;struct mp_plan plan;struct sv_status s;struct mp_ticket ticket;uint32_t seen_word,next_sample,nonce_counter,nonce_pending,explicit_rebind,cancel_sent,manual_pause,stop_cancel_sent,desired_target,desired_override,off_ack,power_paused,power_cancel_sent;uint8_t nonce[16],seed[16];};
extern struct sv_context supervisor;
int sv_init(const uint8_t instance[16],const uint8_t f4[16]);
int sv_tick(uint32_t now_ms,int audio_pending);
int sv_before_play(unsigned asset,uint32_t *open_word);
int sv_commit(uint32_t open_word);void sv_sent(void);
int sv_revoke(uint32_t reason);int sv_rebind(const uint8_t nonce[16]);
void sv_request_desired(uint32_t);
int sv_stop(void);void sv_report(struct sv_status*);
/* Owner-thread only. Pause retains the USB subscription and provider handles.
 * Once tick returns0 the caller parks and must not call any source API until
 * resume. Resume invalidates the prior ticket and requires a new full read. */
int sv_pause_begin(void);int sv_pause_tick(void);int sv_resume(void);
#endif
