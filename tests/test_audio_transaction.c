#include "audio_transaction.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
static unsigned checks;static void ok(int x){checks++;if(!x){fprintf(stderr,"FAIL %u\n",checks);exit(1);}}
struct model{uint32_t tid,calls,lastop,closed_ack;int revoke_busy,rebind_busy,query_busy,finalize,off,query_error,invalid_report,rebind_error_after_effect;struct mat_observation o;};
static uint32_t tid(void*p){return((struct model*)p)->tid;}
static int port(void*p,const struct mat_request*q,struct mat_observation*out){struct model*m=p;m->calls++;m->lastop=q->op;
 if(q->op==5){m->o.gate|=1;if(m->revoke_busy){m->revoke_busy=0;return 2;}*out=m->o;return 1;}
 if(q->op==6){ok(m->closed_ack);ok((m->o.gate&3)==1);if(m->rebind_busy){m->rebind_busy=0;return 2;}m->o.control=q->control;m->off=q->asset==0;for(unsigned i=0;i<16;i++)m->o.snapshot_nonce[i]=q->nonce[i];m->o.desired=q->asset==1?m->o.desired:q->asset?q->asset-1:0;m->o.rebinding=1;*out=m->o;if(m->rebind_error_after_effect){m->rebind_error_after_effect=0;return 0x101;}return 1;}
 ok(q->op==2);if(m->query_error){int e=m->query_error;m->query_error=0;return e;}if(m->query_busy){m->query_busy=0;return 2;}m->closed_ack=(m->o.gate&3)==1&&m->o.owner_phase==1&&!m->o.posted;
 if(m->finalize&&m->o.rebinding){m->o.rebinding=0;if(!m->off){m->o.gate=0;m->o.qualified=1;m->o.generation++;}}*out=m->o;if(m->invalid_report){out->valid=0;m->invalid_report=0;}return 0;
}
static void init(struct mat_context*c,struct model*m){memset(m,0,sizeof(*m));m->tid=392;m->o.valid=1;m->o.control=20;m->o.phase=2;m->o.owner_phase=1;m->o.desired=1;m->o.generation=4;m->o.qualified=1;uint8_t seed[16]={0x72,0x42,0x11};struct mat_port p={m,tid,port};ok(mat_init(c,&p,392,seed)==0);}
static void tick(struct mat_context*c,struct model*m){unsigned before=m->calls;mat_tick(c);ok(m->calls<=before+1);}
int main(void){struct mat_context c;struct model m;
 init(&c,&m);m.revoke_busy=1;m.query_busy=1;m.rebind_busy=1;ok(mat_submit(&c,3,1)==0);ok(mat_submit(&c,2,2)==2);for(unsigned i=0;i<7;i++)tick(&c,&m);ok(c.report.accepted&&!c.report.completed);m.finalize=1;tick(&c,&m);ok(c.report.completed&&c.report.target==2&&m.o.desired==2);ok(c.report.automatic==0);ok(mat_submit(&c,2,1)==MAT_BAD);
 uint8_t old[16];memcpy(old,c.report.nonce,16);ok(mat_submit(&c,1,2)==0);for(unsigned i=0;i<5&&!c.report.completed;i++)tick(&c,&m);ok(c.report.completed&&c.report.target==2&&memcmp(old,c.report.nonce,16));
 init(&c,&m);m.o.qualified=0;ok(mat_submit(&c,0,1)==0);m.finalize=1;for(unsigned i=0;i<6&&!c.report.completed;i++)tick(&c,&m);ok(c.report.completed&&c.report.observed_desired==0&&!c.report.qualified&&(c.report.gate&1));
 init(&c,&m);m.o.owner_phase=2;m.o.gate=2;ok(mat_submit(&c,2,1)==0);for(unsigned i=0;i<4;i++)tick(&c,&m);ok(!c.report.accepted&&m.lastop==2);m.o.owner_phase=1;m.o.gate=1;m.finalize=1;for(unsigned i=0;i<5&&!c.report.completed;i++)tick(&c,&m);ok(c.report.completed);
 init(&c,&m);ok(mat_submit(&c,2,1)==0);for(unsigned i=0;i<3;i++)tick(&c,&m);m.o.control++;tick(&c,&m);ok(c.report.state==MAT_ABORT&&!c.report.completed);tick(&c,&m);tick(&c,&m);ok(c.report.state==MAT_FAILED&&c.report.code==MAT_CONFLICT&&(m.o.gate&1));
 init(&c,&m);ok(mat_submit(&c,2,1)==0);for(unsigned i=0;i<201;i++)tick(&c,&m);ok(c.report.state==MAT_ABORT_DRAIN&&m.lastop==5&&!c.report.completed);tick(&c,&m);ok(c.report.state==MAT_FAILED&&c.report.code==MAT_TIMEOUT);
 init(&c,&m);ok(mat_observe(&c)==0&&c.report.query_fresh&&c.report.observed_desired==1);m.query_busy=1;ok(mat_observe(&c)==2&&!c.report.query_fresh);ok(mat_submit(&c,2,1)==0);unsigned before=m.calls;ok(mat_observe(&c)==MAT_BUSY&&m.calls==before);
 init(&c,&m);memset(c.seed,0,16);c.seed[12]=1;ok(mat_submit(&c,2,1)==MAT_BAD&&c.report.state==MAT_FAILED&&!m.calls);ok(mat_submit(&c,2,2)==0);
 init(&c,&m);ok(mat_submit(&c,2,1)==0);for(unsigned i=0;i<3;i++)tick(&c,&m);m.o.rebinding=0;m.o.error=0x104;tick(&c,&m);ok(c.report.state==MAT_ABORT);tick(&c,&m);tick(&c,&m);ok(c.report.state==MAT_FAILED&&c.report.code==0x104);
 for(unsigned fault=0;fault<3;fault++){init(&c,&m);ok(mat_submit(&c,2,1)==0);for(unsigned i=0;i<3;i++)tick(&c,&m);m.o.gate=0;m.o.rebinding=0;if(fault==0)m.query_error=0x106;else if(fault==1)m.invalid_report=1;else m.o.control++;tick(&c,&m);ok(c.report.state==MAT_ABORT&&!c.report.completed);uint32_t error=c.report.code;tick(&c,&m);ok(m.lastop==5&&c.report.state==MAT_ABORT_DRAIN);m.query_busy=1;tick(&c,&m);ok(c.report.state==MAT_ABORT_DRAIN&&!c.report.completed);tick(&c,&m);ok(c.report.state==MAT_FAILED&&c.report.code==error&&(m.o.gate&1));}
 init(&c,&m);m.rebind_error_after_effect=1;ok(mat_submit(&c,2,1)==0);for(unsigned i=0;i<3;i++)tick(&c,&m);ok(c.report.state==MAT_ABORT&&!c.report.accepted&&m.o.control==21);m.o.gate=0;tick(&c,&m);ok(m.lastop==5);tick(&c,&m);ok(c.report.state==MAT_FAILED&&c.report.code==0x101&&(m.o.gate&1));
 init(&c,&m);ok(mat_submit(&c,2,1)==0);for(unsigned i=0;i<3;i++)tick(&c,&m);m.query_error=0x106;tick(&c,&m);tick(&c,&m);for(unsigned i=0;i<500;i++){m.query_busy=1;tick(&c,&m);}ok(c.report.state==MAT_ABORT_DRAIN&&!c.report.completed);m.query_busy=0;tick(&c,&m);ok(c.report.state==MAT_FAILED&&c.report.code==0x106);
 /* A suspend can arrive before REVOKE, during drain, or after REBIND.
  * Repeated cancellation must not restart ABORT_DRAIN or claim completion. */
 for(unsigned stage=0;stage<4;stage++){
  init(&c,&m);ok(mat_cancel(&c)==MAT_OK&&!m.calls);ok(mat_submit(&c,2,1)==MAT_OK);
  for(unsigned i=0;i<stage;i++)tick(&c,&m);
  unsigned accepted=c.report.accepted,before=m.calls;
  m.tid=999;ok(mat_cancel(&c)==MAT_OWNER&&m.calls==before);m.tid=392;
  ok(mat_cancel(&c)==MAT_BUSY&&c.report.state==MAT_ABORT);
  for(unsigned i=0;i<8&&c.report.state!=MAT_FAILED;i++){
   unsigned state=c.report.state;ok(mat_cancel(&c)==MAT_BUSY&&c.report.state==state);tick(&c,&m);
  }
  ok(c.report.state==MAT_FAILED&&c.report.code==MAT_CLOSED&&!c.report.completed);
  ok(c.report.accepted==accepted&&(m.o.gate&1));before=m.calls;ok(mat_cancel(&c)==MAT_OK&&m.calls==before);
 }
 /* No-card cancellation needs the exact adapter's empty-audio evidence.
  * Merely presenting stage5 or an uninitialized owner is not sufficient. */
 for(unsigned proof=0;proof<4;proof++){
  init(&c,&m);m.o.owner_phase=0;m.o.phase=1;m.o.stage=5;m.o.qualified=0;
  m.o.empty_audio=proof!=0;if(proof==2)m.o.stage=6;if(proof==3)m.o.posted=1;
  ok(mat_submit(&c,2,1)==MAT_OK);ok(mat_cancel(&c)==MAT_BUSY);
  for(unsigned i=0;i<5;i++)tick(&c,&m);
  ok(!c.report.accepted&&!c.report.completed);
  ok(c.report.state==(proof==1?MAT_FAILED:MAT_ABORT_DRAIN));
 }
 init(&c,&m);m.tid=999;ok(mat_submit(&c,2,1)==MAT_OWNER);ok(!m.calls);printf("MAT tests passed %u assertions\n",checks);return 0;}
