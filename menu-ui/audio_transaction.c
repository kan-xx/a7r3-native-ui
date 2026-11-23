#include "audio_transaction.h"
static int same(const uint8_t*a,const uint8_t*b){uint32_t x=0;for(unsigned i=0;i<16;i++)x|=a[i]^b[i];return !x;}
static int owner(struct mat_context*c){return c&&c->port.tid&&c->port.tid(c->port.context)==c->report.owner_tid;}
static int terminal(uint32_t s){return s==MAT_IDLE||s==MAT_DONE||s==MAT_FAILED;}
static int closed(const struct mat_observation*o){
 /* The exact fresh-v3 backend exposes stage 5 before any audio allocation,
  * SID, or registration. Its owner remains zero-initialized until source
  * creation succeeds. This admits cancellation of a waiting menu request;
  * it is neither source drain nor a global power-completion fence. */
 int idle=o->owner_phase==1||(o->empty_audio&&o->owner_phase==0&&o->phase==1&&o->stage==5);
 return o->valid&&(o->gate&1)&&!(o->gate&2)&&idle&&!o->posted;
}
static int fail(struct mat_context*c,uint32_t code){c->report.code=code;c->report.state=MAT_FAILED;return code;}
static int abort_request(struct mat_context*c,uint32_t code){if(!c->aborting)c->report.code=code?code:MAT_CONFLICT;c->aborting=1;c->report.state=MAT_ABORT;return MAT_BUSY;}
int mat_init(struct mat_context*c,const struct mat_port*p,uint32_t tid,const uint8_t seed[16]){
 if(!c||!p||!p->tid||!p->dispatch||!tid||!seed)return MAT_BAD;uint8_t nz=0;for(unsigned i=0;i<16;i++)nz|=seed[i];if(!nz)return MAT_BAD;
 for(unsigned i=0;i<sizeof(*c);i++)((uint8_t*)c)[i]=0;c->port=*p;c->report.version=1;c->report.owner_tid=tid;c->report.observed_desired=0xffffffffu;
 for(unsigned i=0;i<16;i++)c->seed[i]=seed[i];return MAT_OK;
}
int mat_submit(struct mat_context*c,uint32_t asset,uint32_t seq){
 if(!owner(c))return MAT_OWNER;if(asset>3||!seq||seq<=c->last_seq)return MAT_BAD;if(!terminal(c->report.state))return MAT_BUSY;
 c->last_seq=seq;uint32_t tid=c->report.owner_tid;for(unsigned i=0;i<sizeof(c->report);i++)((uint8_t*)&c->report)[i]=0;
 c->report.version=1;c->report.owner_tid=tid;c->report.request_seq=seq;c->report.asset=asset;c->report.target=asset==1?0xffffffffu:asset?asset-1:0;c->report.observed_desired=0xffffffffu;c->report.state=MAT_REVOKE;c->revoked=c->aborting=0;
 /* Per-activation random seed + strictly monotonic32bit sequence is injective.
  * Never reuse the activation seed after replacing this resident core. */
 for(unsigned i=0;i<16;i++)c->report.nonce[i]=c->seed[i];for(unsigned i=0;i<4;i++)c->report.nonce[12+i]^=(uint8_t)(seq>>(8*i));uint8_t nz=0;for(unsigned i=0;i<16;i++)nz|=c->report.nonce[i];if(!nz)return fail(c,MAT_BAD);return MAT_OK;
}
int mat_tick(struct mat_context*c){
 if(!owner(c))return MAT_OWNER;struct mat_report*r=&c->report;if(terminal(r->state))return MAT_OK;
 if(r->ticks==0xffffffffu){if(!c->aborting)abort_request(c,MAT_BAD);}else r->ticks++;r->query_fresh=0;
 if(r->ticks>200&&r->state!=MAT_ABORT&&r->state!=MAT_ABORT_DRAIN){abort_request(c,MAT_TIMEOUT);}
 struct mat_request q={0};struct mat_observation o={0};
 q.op=(r->state==MAT_REVOKE||r->state==MAT_ABORT)?5:r->state==MAT_REBIND?6:2;
 if(q.op==6){q.control=r->control;q.asset=r->asset;for(unsigned i=0;i<16;i++)q.nonce[i]=r->nonce[i];}
 r->calls++;int rc=c->port.dispatch(c->port.context,&q,&o);
 /* REVOKE can close atomically then return BUSY without writing a report. */
 if(q.op==5){if(rc==0||rc==1||rc==2){r->state=c->aborting?MAT_ABORT_DRAIN:MAT_DRAIN;return MAT_BUSY;}return abort_request(c,(uint32_t)rc);}
 if(rc==2)return MAT_BUSY;
 if(q.op==6){
  if(rc!=1)return abort_request(c,(uint32_t)rc); /* duplicate/stale never means ours */
  if(!o.valid||o.control!=r->control)return abort_request(c,MAT_CONFLICT);
  r->accepted=1;r->state=MAT_VERIFY;return MAT_BUSY;
 }
 if(rc||!o.valid)return abort_request(c,rc?(uint32_t)rc:MAT_CONFLICT);
 r->query_fresh=1;r->observed_desired=o.desired;r->qualified=o.qualified;r->gate=o.gate;r->automatic=o.automatic;r->new_generation=o.generation;r->capture=o.capture;r->observed_control=o.control;
 if(r->state==MAT_ABORT_DRAIN){if(closed(&o))return fail(c,r->code?r->code:MAT_TIMEOUT);if(!(o.gate&1))r->state=MAT_ABORT;return MAT_BUSY;}
 if(o.stop||o.fatal||o.phase==4||o.phase==5||o.phase==6){if(closed(&o))return fail(c,MAT_CLOSED);return abort_request(c,MAT_CLOSED);}
 if(r->state==MAT_DRAIN){
  if(!(o.gate&1)){r->state=MAT_REVOKE;return MAT_BUSY;}
  if(!closed(&o))return MAT_BUSY;
  if(o.control==0xffffffffu||o.desired>2||same(o.snapshot_nonce,r->nonce))return fail(c,MAT_CONFLICT);
  r->control=o.control+1;r->old_generation=o.generation;if(r->asset==1)r->target=o.desired;c->revoked=1;r->state=MAT_REBIND;return MAT_BUSY;
 }
 if(r->state!=MAT_VERIFY||!c->revoked||!r->accepted)return abort_request(c,MAT_BAD);
 if(o.control!=r->control)return abort_request(c,MAT_CONFLICT);
 /* OFF acknowledges closed semantics even without a readable card. This is
  * never displayed as cache READY. No opaque phase claims F4 worker state. */
 int noerrors=!o.error&&!o.supervisor_error&&!o.fatal;
 if(r->asset==0&&noerrors&&o.desired==0&&closed(&o)){r->completed=1;r->state=MAT_DONE;r->code=0;return MAT_OK;}
 if(noerrors&&o.qualified&&!(o.gate&3)&&o.generation>r->old_generation&&same(o.snapshot_nonce,r->nonce)&&o.desired==r->target){r->completed=1;r->state=MAT_DONE;r->code=0;return MAT_OK;}
 if((o.error||o.supervisor_error)&&!o.rebinding){return abort_request(c,o.error?o.error:o.supervisor_error);}
 return MAT_BUSY;
}

int mat_cancel(struct mat_context*c){if(!owner(c))return MAT_OWNER;if(terminal(c->report.state))return MAT_OK;if(c->aborting)return MAT_BUSY;return abort_request(c,MAT_CLOSED);}
static int observe(struct mat_context*c,int power){
 if(!owner(c))return MAT_OWNER;if(!terminal(c->report.state))return MAT_BUSY;
 struct mat_request q={.op=2};struct mat_observation o={0};c->report.query_fresh=0;c->report.calls++;
 int rc=c->port.dispatch(c->port.context,&q,&o);if(rc)return rc;if(!o.valid)return MAT_CONFLICT;
 /* An idle status refresh is read-only. A stopped/faulted owner cannot
  * produce a fresh eligible-state claim from otherwise retained fields. */
 if(power&&o.disabled_audio&&o.phase==6&&!o.stop&&!o.fatal&&!o.automatic&&!o.qualified&&closed(&o)){
  c->report.query_fresh=1;c->report.observed_desired=o.desired;c->report.qualified=0;
  c->report.gate=o.gate;c->report.automatic=0;c->report.new_generation=o.generation;
  c->report.capture=o.capture;c->report.observed_control=o.control;
  return MAT_OK; /* Menu/power may resume; sound remains explicitly unavailable. */
 }
 if(o.stop||o.fatal||o.phase==4||o.phase==5||o.phase==6)return MAT_CLOSED;
 if(o.error||o.supervisor_error)return MAT_CONFLICT;
 c->report.query_fresh=1;c->report.observed_desired=o.desired;c->report.qualified=o.qualified;c->report.gate=o.gate;c->report.automatic=o.automatic;c->report.new_generation=o.generation;c->report.capture=o.capture;c->report.observed_control=o.control;return 0;
}

int mat_observe(struct mat_context*c){return observe(c,0);}
int mat_power_observe(struct mat_context*c){return observe(c,1);}
