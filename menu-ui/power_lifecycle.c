#include "power_lifecycle.h"
static uint32_t ld(const uint32_t*p){return __atomic_load_n(p,__ATOMIC_ACQUIRE);}
static void st(uint32_t*p,uint32_t v){__atomic_store_n(p,v,__ATOMIC_RELEASE);}
static int enter(struct lp_control*c){return !__atomic_exchange_n(&c->request_lock,1,__ATOMIC_ACQUIRE);}
static int leave(struct lp_control*c,int r){st(&c->request_lock,0);return r;}
uint32_t lp_phase(const struct lp_control*c){return ld(&c->phase);}
uint32_t lp_epoch(const struct lp_control*c){return ld(&c->epoch);}
int lp_closed(const struct lp_control*c){return lp_phase(c)!=LP_ACTIVE||(adm_word(&c->gate)&3u)||ld(&c->gate.fatal);}
int lp_pause(struct lp_control*c,uint32_t epoch){
 if(!c||!epoch)return LP_STALE;if(!enter(c)){
  /* Worker may be granting wake concurrently. Revoke now, then require the
   * caller to retry acceptance; a saved wake word cannot undo this close. */
  if(adm_revoke(&c->gate,3)){lp_fault(c,1);return LP_ERROR;}return LP_BUSY;
 }
 uint32_t old=lp_epoch(c),p=lp_phase(c);
 if(epoch==old)return leave(c,(p==LP_CLOSING||p==LP_PARKED)?LP_ALREADY:LP_STALE);
 if(old==UINT32_MAX||epoch!=old+1)return leave(c,LP_STALE);
 /* A newer native pause also invalidates a still-parked bootstrap grant.
  * Revalidate the local drain at the new epoch before acknowledging it. */
 if(p!=LP_ACTIVE&&p!=LP_WAKING&&p!=LP_CLOSING&&p!=LP_PARKED)return leave(c,LP_STATE);
 /* Linearizes against the actual native SEND. An already committed send is
  * retained and drained; a later attempt cannot pass this word. */
 if(adm_revoke(&c->gate,3)){lp_fault(c,1);return leave(c,LP_ERROR);}
 st(&c->epoch,epoch);st(&c->phase,LP_CLOSING);__atomic_fetch_add(&c->pauses,1,__ATOMIC_RELAXED);
 return leave(c,LP_OK);
}
int lp_resume(struct lp_control*c,uint32_t epoch){
 if(!c||!epoch)return LP_STALE;
 /* Capture before the request lock. A concurrent busy pause revokes this
  * exact word; no later read can silently adopt the revocation as a grant. */
 uint32_t grant=adm_word(&c->gate);if(!enter(c))return LP_BUSY;
 if(epoch!=lp_epoch(c))return leave(c,LP_STALE);
 uint32_t p=lp_phase(c);if(p==LP_WAKING)return leave(c,LP_ALREADY);
 if(p!=LP_PARKED||!(grant&1u)||adm_word(&c->gate)!=grant)return leave(c,LP_STATE);
 st(&c->resume_word,grant);st(&c->phase,LP_WAKING);__atomic_fetch_add(&c->resumes,1,__ATOMIC_RELAXED);return leave(c,LP_OK);
}
int lp_commit(struct lp_control*c,uint32_t word){return lp_phase(c)==LP_ACTIVE?adm_commit(&c->gate,word):-1;}
void lp_sent(struct lp_control*c){adm_sent(&c->gate);}
int lp_park(struct lp_control*c,uint32_t epoch){
 if(lp_epoch(c)!=epoch||(adm_word(&c->gate)&3u)!=1u)return LP_STATE;
 uint32_t expected=LP_CLOSING;return __atomic_compare_exchange_n(&c->phase,&expected,LP_PARKED,0,__ATOMIC_ACQ_REL,__ATOMIC_ACQUIRE)?LP_OK:LP_STATE;
}
int lp_open(struct lp_control*c,uint32_t epoch){
 /* Serialize with callbacks; no I/O or wait in this critical section. */
 if(!enter(c))return LP_BUSY;
 if(lp_epoch(c)!=epoch||lp_phase(c)!=LP_WAKING)return leave(c,LP_STALE);
 if(adm_open(&c->gate,ld(&c->resume_word))){
  if(ld(&c->gate.fatal)){lp_fault(c,2);return leave(c,LP_ERROR);}
  return leave(c,LP_BUSY);
 }
 uint32_t expected=LP_WAKING;
 if(!__atomic_compare_exchange_n(&c->phase,&expected,LP_ACTIVE,0,__ATOMIC_ACQ_REL,__ATOMIC_ACQUIRE)||ld(&c->gate.fatal)){lp_fault(c,2);return leave(c,LP_ERROR);}
 return leave(c,lp_closed(c)?LP_BUSY:LP_OK);
}
void lp_fault(struct lp_control*c,uint32_t why){
 st(&c->gate.fatal,1);adm_revoke(&c->gate,3);uint32_t zero=0;__atomic_compare_exchange_n(&c->error,&zero,why?why:1,0,__ATOMIC_ACQ_REL,__ATOMIC_ACQUIRE);st(&c->phase,LP_FAULT);
}
void lp_report(const struct lp_control*c,struct lp_status*s){
 s->phase=lp_phase(c);s->epoch=lp_epoch(c);s->error=ld(&c->error);s->word=adm_word(&c->gate);s->revokes=ld(&c->gate.revokes);s->commits=ld(&c->gate.commits);s->rejected=ld(&c->gate.rejected);s->pauses=ld(&c->pauses);s->resumes=ld(&c->resumes);
}
