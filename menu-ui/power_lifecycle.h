#ifndef A7_POWER_LIFECYCLE_H
#define A7_POWER_LIFECYCLE_H
#include "admission.h"
/* This is the audio owner's local pause/resume port. It sends no IM completion.
 * A future registered IM adapter must separately qualify the native producer
 * return boundary before treating local drain as a system suspend grant. */
enum {LP_ACTIVE,LP_CLOSING,LP_PARKED,LP_WAKING,LP_FAULT};
enum {LP_OK,LP_BUSY,LP_ALREADY,LP_STALE,LP_STATE,LP_ERROR};
struct lp_control {struct adm_gate gate;uint32_t request_lock,epoch,phase,error,pauses,resumes,resume_word;};
struct lp_status {uint32_t abi,bytes,build,phase,epoch,error,word,revokes,commits,rejected,pauses,resumes,worker_epoch,wake_epoch,pending_audio,pending_source,pending_registration,pending_loop,local_drained,native_quiet,native_producer_return_proven,resume_qualified,retained_resources,reserved;};
_Static_assert(sizeof(struct lp_status)==96,"power status");
struct ap_power_api {uint32_t abi,bytes,build;int(*pause)(const uint8_t*,uint32_t);int(*resume)(const uint8_t*,uint32_t);int(*query)(struct lp_status*);};
/* LP_BUSY is not acceptance: caller must retry the same epoch. A contended
 * pause still closes SEND immediately; it never waits inside an IM callback. */
int lp_pause(struct lp_control*,uint32_t);
int lp_resume(struct lp_control*,uint32_t);
uint32_t lp_phase(const struct lp_control*);
uint32_t lp_epoch(const struct lp_control*);
int lp_closed(const struct lp_control*);
int lp_commit(struct lp_control*,uint32_t);
void lp_sent(struct lp_control*);
int lp_park(struct lp_control*,uint32_t);
int lp_open(struct lp_control*,uint32_t);
void lp_fault(struct lp_control*,uint32_t);
void lp_report(const struct lp_control*,struct lp_status*);
#endif
