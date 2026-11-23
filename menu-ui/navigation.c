#include "navigation.h"
struct event32 { uint32_t *refs,id;uint8_t dest,category,pad[2];void *params; };
struct params32 { void *vector;uint32_t *refs;uint8_t *sorted;uint32_t *next;uint8_t *flag; };
struct vector32 { void **begin,**end,**capacity; };
struct param32 { void *vptr;uint32_t type,key,sequence,value; };
#if defined(__arm__)
_Static_assert(sizeof(struct event32)==16,"original Event ABI");
_Static_assert(sizeof(struct params32)==20,"original ParamList ABI");
_Static_assert(sizeof(struct vector32)==12,"original vector ABI");
_Static_assert(sizeof(struct param32)==20,"original ParamInt ABI");
#endif
void a7_navigation_target(const struct a7_navigation_api*a,uint32_t closing,uint32_t target,uint32_t mode,struct a7_navigation_result*r){
 for(size_t i=0;i<sizeof(*r);i++)((uint8_t*)r)[i]=0;
 r->version=1;r->event_id=closing?0x11012002:0x11012001;r->destination=4;r->target_view=target;r->mode=mode;
 if(closing>1||!a||a->qualified!=1||!a->event_manager||!a->param_ctor||!a->event_dtor||!a->push||!a->trylock||!a->unlock||!a->allocate||!a->release){r->status=A7_NAV_BAD_API;return;}
 /* All ownership allocations precede initialization/transfer. This exactly
  * reproduces Event(0x11012001,4,0), ParamList with capacity=2, and two
  * ParamInt additions. Avoids original ParamListC1's partial-allocation leak.
  * malloc is original Sony libc; original operator delete releases via free.
  * Only original ParamInt constructor sets its original vptr. */
 const size_t sizes[11]={sizeof(struct event32),4,sizeof(struct params32),sizeof(struct vector32),4,1,4,1,(closing?1:2)*sizeof(void*),sizeof(struct param32),sizeof(struct param32)};
 void *p[11]={0};
 unsigned count=closing?10:11;
 for(unsigned i=0;i<count;i++){p[i]=a->allocate(sizes[i]);if(!p[i]){r->status=A7_NAV_ALLOC;for(unsigned j=0;j<i;j++){a->release(p[j]);r->release_count++;}return;}r->allocation_count++;for(size_t j=0;j<sizes[i];j++)((uint8_t*)p[i])[j]=0;}
 struct event32*e=p[0];struct params32*l=p[2];struct vector32*v=p[3];struct param32*x=p[9],*y=p[10];
 e->refs=p[1];*e->refs=1;e->id=r->event_id;e->dest=4;e->category=0;e->params=l;
 l->vector=v;l->refs=p[4];*l->refs=1;l->sorted=p[5];l->next=p[6];*l->next=closing?2:3;l->flag=p[7];
 v->begin=p[8];v->begin[0]=x;if(!closing)v->begin[1]=y;v->end=v->begin+(closing?1:2);v->capacity=v->end;
 a->param_ctor(x,target);x->key=6;x->sequence=1;
 if(!closing){a->param_ctor(y,mode);y->key=0x1a;y->sequence=2;}r->prepared=1;
 /* An optimistic contention check reduces waiting. It is NOT a lock handoff
  * or a hard bound: original push obtains its own default mutex afterwards. */
 if(a->trylock((uint8_t*)a->event_manager+16)){r->precheck_busy=1;r->status=A7_NAV_BUSY;}
 else {a->unlock((uint8_t*)a->event_manager+16);r->status=a7_navigation_push_caught(a,e,r);}
 if(r->status==A7_NAV_OK){r->submitted=1;return;}
 /* Category0 failure before list insertion retains ownership locally.
  * No callbacks/function pointers from this DSO enter the Sony event. */
 a->event_dtor(e);a->release(e);r->release_count=count;
}

void a7_navigation_submit(const struct a7_navigation_api*a,uint32_t closing,struct a7_navigation_result*r){a7_navigation_target(a,closing,0xffc,closing?0:19,r);}
