#include "audio_native.h"
#include "../include/audio_abi/runtime_abi.h"
extern void*dlopen(const char*,int);extern void*dlsym(void*,const char*);extern int dlclose(void*);
struct dinfo {const char*name;void*base;const char*symbol;void*address;};extern int dladdr(const void*,struct dinfo*);
extern int open(const char*,int,...),read(int,void*,unsigned),close(int);
extern long syscall(long,...);
static uint32_t tid(void*p){(void)p;register uint32_t r0 __asm__("r0");register uint32_t r7 __asm__("r7")=224;__asm__ volatile("svc 0":"=r"(r0):"r"(r7):"memory","cc");return r0;}
static int eq(const void*a,const void*b,unsigned n){const uint8_t*x=a,*y=b;unsigned d=0;for(unsigned i=0;i<n;i++)d|=x[i]^y[i];return !d;}
static int streq(const char*a,const char*b){if(!a||!b)return 0;for(unsigned i=0;i<128;i++){if(a[i]!=b[i])return 0;if(!a[i])return 1;}return 0;}
struct backend_identity{const char*path,*symbol;uint32_t build;};
#include "audio_identities.inc"
_Static_assert(sizeof(struct ap_request)==304,"native request ABI");
_Static_assert(sizeof(struct ap_report)==1052&&offsetof(struct ap_report,capture)==876,"full D510 report buffer");
_Static_assert(sizeof(struct ca_status)==sizeof(struct mat_capture),"capture tail ABI");
static int file_identity(const struct backend_identity*id){
 static const uint8_t elf_arm32[7]={0x7f,'E','L','F',1,1,1};
 int fd=open(id->path,0);if(fd<0)return 0;
 uint8_t data[4096],hdr[52]={0};unsigned total=0;int good=1;
 while(total<=1048576u){
  int n=read(fd,data,sizeof data);if(n<0){good=0;break;}
  if(!n)break;
  if((unsigned)n>sizeof data){good=0;break;}
  for(unsigned i=0;i<(unsigned)n&&total+i<52u;i++)hdr[total+i]=data[i];
  total+=(unsigned)n;
 }
 if(close(fd))good=0;
 if(!good||total<52u||total>1048576u||!eq(hdr,elf_arm32,7))return 0;
 unsigned e_type=hdr[16]|((unsigned)hdr[17]<<8),e_machine=hdr[18]|((unsigned)hdr[19]<<8);
 return e_type==3u&&e_machine==40u;
}
static int validate(const struct mp_audio_native*n,const struct ap_report*r){if(r->runtime.abi!=1||r->runtime.bytes!=160||!eq(r->runtime.instance,n->instance,16))return 0;
 if(n->backend_kind==2){const struct df_status*d=&r->deferred;if(d->version!=1||d->bytes!=48||d->phase>5||d->max_ms!=1500)return 0;const struct ca_status*c=&r->capture;if(c->version!=1||c->bytes!=128||c->phase>4||c->bound>1||c->held_reference>1||c->audio_phase>5)return 0;}
 return 1;}
static void normalize(const struct mp_audio_native*n,const struct ap_report*r,struct mat_observation*o){
 o->empty_audio=n->backend_kind==2&&r->runtime.phase==AP_STARTING&&r->runtime.stage==5&&!r->runtime.source_copied&&!r->runtime.loop_sid&&!r->runtime.resource_pinned&&!r->resources.pinned&&!r->resources.allocation_attempts&&!r->resources.registration_attempts&&!r->resources.prepared_count&&!r->resources.registered_count&&!r->owner.phase;
 o->disabled_audio=n->backend_kind==2&&r->runtime.phase==AP_DISABLED_PINNED&&r->runtime.stage==9&&r->runtime.error==(uint32_t)-9464&&r->runtime.control_result==(uint32_t)-9464&&r->runtime.thread_created&&!r->runtime.thread_exited&&!r->runtime.thread_joined&&!r->runtime.stop_requested&&r->runtime.owns_self&&!r->runtime.automatic_admission&&!r->runtime.posted_control&&!r->runtime.ref_uncertain&&r->runtime.loop_phase==3&&!r->runtime.loop_free_failed&&r->owner.phase==A7R_IDLE&&!r->owner.held_message&&!r->owner.stop_requested&&!r->supervisor.fatal&&(!r->supervisor.error||r->supervisor.error==18)&&(r->supervisor.word&3u)==1u&&!r->supervisor.qualified;
 o->valid=1;o->control=r->runtime.control;o->phase=r->runtime.phase;o->stage=r->runtime.stage;o->error=r->runtime.error;o->stop=r->runtime.stop_requested;o->fatal=r->supervisor.fatal;o->gate=r->supervisor.word;o->owner_phase=r->owner.phase;o->posted=r->runtime.posted_control;o->desired=r->supervisor.desired;
 o->qualified=r->supervisor.qualified&&r->source.generation==r->supervisor.bind_generation&&r->source.control==r->supervisor.bind_control&&r->source.status==0&&r->source.copied==AP_F4_COPY_BYTES&&!r->source.close_uncertain;
 o->generation=r->source.generation;o->rebinding=r->supervisor.rebinding;o->supervisor_error=r->supervisor.error;o->automatic=r->runtime.automatic_admission;for(unsigned i=0;i<16;i++)o->snapshot_nonce[i]=r->source.snapshot_nonce[i];
 for(unsigned i=0;i<sizeof o->capture;i++)((uint8_t*)&o->capture)[i]=n->backend_kind==2?((const uint8_t*)&r->capture)[i]:0;
}
static int dispatch(void*p,const struct mat_request*m,struct mat_observation*out){struct mp_audio_native*n=p;if(!n||!out||!m||tid(0)!=n->owner_tid)return MAT_OWNER;n->ui_calls++;if(!n->admission||n->closed||!n->guard||!n->api)return MAT_CLOSED;if(m->op!=2&&m->op!=5&&m->op!=6)return MAT_BAD;
 struct ap_request q={0};struct ap_report r={0};q.op=m->op;q.control=m->control;q.asset=m->asset;for(unsigned i=0;i<16;i++){q.instance[i]=n->instance[i];q.source_nonce[i]=m->nonce[i];}
 int rc=((const struct ap_api*)n->api)->dispatch(&q,&r);if(rc==AP_BUSY)return rc;if(!validate(n,&r))return MAT_CONFLICT;normalize(n,&r,out);return rc;
}
int mp_audio_host_bind(struct mp_audio_native*n,uint32_t owner,uint32_t kind,const uint8_t instance[16],struct mat_port*p){
 if(!n||!owner||kind>2||!instance||!p||tid(0)==owner||n->guard||n->api||n->reference_uncertain)return MAT_BAD;uint8_t nz=0;for(unsigned i=0;i<16;i++)nz|=instance[i];const struct backend_identity*id=&identities[kind];if(!nz||!file_identity(id))return MAT_BAD;
 void*h=dlopen(id->path,AP_NOLOAD);if(!h)return MAT_CLOSED;void*symbol=dlsym(h,id->symbol);struct dinfo d={0},dd={0};int good=0;const struct ap_api*a=0;
 if(symbol&&dladdr(symbol,&d)&&d.base&&(streq(d.name,id->path)||(d.name&&d.name[0]=='/'&&d.name[1]=='/'&&streq(d.name+1,id->path)))){
  a=((ap_get_api)symbol)();
  good=a&&a->abi==1&&a->bytes==20&&a->build==id->build&&a->dispatch&&dladdr((const void*)(uintptr_t)a->dispatch,&dd)&&dd.base==d.base;
 }

 if(good){
  n->owner_tid=owner;n->backend_kind=kind;for(unsigned i=0;i<16;i++)n->instance[i]=instance[i];struct ap_request q={0};q.op=AP_QUERY;for(unsigned i=0;i<16;i++)q.instance[i]=instance[i];good=0;
  /* START transfers a reference before its worker initializes the supervisor.
   * This is a host-side bind before any timer/view is published. Hold only our
   * loader reference and wait for a coherent query; never replay START or
   * grant audio admission. BUSY supplies no report. Bound the total attempts
   * so an unresponsive backend remains a guarded startup failure. */
  for(unsigned attempt=0;attempt<100;attempt++){
   struct ap_report r={0};int rc=a->dispatch(&q,&r);int pending=rc==AP_BUSY;
   if(!rc){
    if(!validate(n,&r)||r.runtime.error||r.runtime.stop_requested||!r.runtime.owns_self||!r.runtime.thread_created)break;
    good=r.runtime.phase==AP_READY||(r.runtime.phase==AP_STARTING&&r.runtime.stage==5);
    if(good)break;
    pending=kind==2&&r.runtime.phase==AP_STARTING&&r.runtime.stage==4;
   }
   if(!pending||attempt==99)break;
   long delay[2]={0,20000000};if(syscall(162,delay,0))break;
  }
 }
 if(!good){if(dlclose(h)){n->guard=h;n->reference_uncertain=1;return MAT_CONFLICT;}return MAT_CLOSED;}
 n->backend_kind=kind;n->guard=h;n->api=a;n->base=(uintptr_t)d.base;n->admission=1;p->context=n;p->tid=tid;p->dispatch=dispatch;return MAT_OK;
}
int mp_audio_ui_close(struct mp_audio_native*n){if(!n||tid(0)!=n->owner_tid)return MAT_OWNER;n->ui_calls++;n->admission=0;n->closed=1;return 0;}
int mp_audio_host_release(struct mp_audio_native*n){if(!n||tid(0)==n->owner_tid||!n->closed||n->admission||n->reference_uncertain)return MAT_BAD;if(!n->guard)return 0;if(dlclose(n->guard)){n->reference_uncertain=1;return MAT_CONFLICT;}n->guard=0;n->api=0;return 0;}
int mp_audio_host_abort_unpublished(struct mp_audio_native*n){if(!n||tid(0)==n->owner_tid||n->ui_calls||n->reference_uncertain)return MAT_BAD;n->admission=0;n->closed=1;return mp_audio_host_release(n);}
