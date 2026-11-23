#include "lean_view.h"
#if defined(__arm__)
_Static_assert(offsetof(struct a7_lean_view,api)==0x28,"original base extent");
#endif
/* Exact native key IDs are consumed only while this owned View is active. */
static uint32_t current_tid(void){register uint32_t r0 __asm__("r0");register uint32_t r7 __asm__("r7")=224;__asm__ volatile("svc 0":"=r"(r0):"r"(r7):"memory","cc");return r0;}
static uint32_t owner(struct a7_lean_view*v){uint32_t t=current_tid();if(v->report->owner_tid&&v->report->owner_tid!=t)v->report->thread_errors++;return t;}
static void entry(struct a7_lean_view*v){v->report->entry_tid=owner(v);v->report->entry++;if(v->graphics&&a7_gfx_create_caught(v->graphics)&&v->control&&mp_page_open(v->control))v->serial=v->control->report.serial;}
static void exit_view(struct a7_lean_view*v){owner(v);v->report->exit++;if(v->control&&v->serial)mp_page_exit(v->control,v->serial);if(v->graphics)a7_gfx_destroy_caught(v->graphics);}
static void main_view(struct a7_lean_view*v,int32_t*next,int32_t*action){
 owner(v);v->report->main++;if(next)*next=-10;if(action)*action=-1;
 if(v->base[5]){v->report->last_event=v->api->event_id((void*)(uintptr_t)v->base[5]);if(v->control)mp_page_event(v->control,v->serial,v->report->last_event);}
}
static void constructor(struct a7_lean_view*v,int32_t*next){v->report->ctor++;if(next)*next=0;}
static void destructor(struct a7_lean_view*v){v->report->closed=1;}
static void action_view(struct a7_lean_view*v,int32_t action){(void)action;v->report->action++;}
static void nop(struct a7_lean_view*v){(void)v;}
static int zero(struct a7_lean_view*v){(void)v;return 0;}
static void*dtor(struct a7_lean_view*v){v->report->dtor_tid=owner(v);v->report->dtor++;v->report->closed=1;return v->api->sony_dtor(v);}
static void*deleting(struct a7_lean_view*v){const struct a7_lean_api*a=v->api;v->report->deleting++;dtor(v);a->release(v);return v;}
static int onload(struct a7_lean_view*v){owner(v);v->report->onload++;return 55;}
static int onunload(struct a7_lean_view*v){owner(v);v->report->onunload++;return -1;}
static void config(struct a7_lean_view*v,void*c){(void)v;(void)c;}
static void layer(struct a7_lean_view*v,int n){v->report->layer=n;}
static void changed(struct a7_lean_view*v,int n){(void)v;(void)n;}
struct layout {uint32_t family,variant,container;};
static void layout_empty(struct layout*out){for(unsigned i=0;i<6;i++){out[i].family=0x0d5abf77;out[i].variant=0;out[i].container=0x35cc4d2e;}}
static void layout_open(struct a7_lean_view*v,int*a,int*b,struct layout*out,void**animations){(void)a;(void)b;(void)animations;owner(v);v->report->layout_open++;layout_empty(out);}
static void layout_close(struct a7_lean_view*v,int*a,int*b,struct layout*out,void**animations){(void)a;(void)b;(void)animations;owner(v);v->report->layout_close++;layout_empty(out);}
static int excluded(struct a7_lean_view*v,void*e){(void)v;(void)e;return 0;}
static uint32_t pass_input(struct a7_lean_view*v,uint32_t id){return v->control&&v->control->report.active&&v->control->report.serial==v->serial&&mp_options_owns(id)?id:0x11012005u;}
static uint32_t identity(struct a7_lean_view*v,uint32_t id){(void)v;return id;}
static uint32_t identity_params(struct a7_lean_view*v,uint32_t id,void*p){(void)p;return pass_input(v,id);}
static void log_event(struct a7_lean_view*v,uint32_t id,const void*e){(void)v;(void)id;(void)e;}
int a7_lean_construct(struct a7_lean_view*v,const struct a7_lean_api*a,struct a7_lean_report*r,void*m){
 if(!v||!a||!r||!m||!a->sony_ctor||!a->sony_dtor||!a->release||!a->event_id||!a->original_view_typeinfo)return 0;
 for(unsigned i=0;i<sizeof(*v);i++)((unsigned char*)v)[i]=0;
 a->sony_ctor(v,m);v->api=a;v->report=r;
 const uintptr_t f[27]={(uintptr_t)entry,(uintptr_t)exit_view,(uintptr_t)main_view,(uintptr_t)constructor,(uintptr_t)destructor,(uintptr_t)action_view,(uintptr_t)nop,(uintptr_t)zero,(uintptr_t)dtor,(uintptr_t)deleting,(uintptr_t)onload,(uintptr_t)onunload,(uintptr_t)config,(uintptr_t)layer,(uintptr_t)changed,(uintptr_t)layout_open,(uintptr_t)layout_close,(uintptr_t)zero,(uintptr_t)excluded,(uintptr_t)pass_input,(uintptr_t)identity,(uintptr_t)identity_params,(uintptr_t)identity,(uintptr_t)zero,(uintptr_t)zero,(uintptr_t)log_event,(uintptr_t)zero};
 v->table[0]=0;v->table[1]=a->original_view_typeinfo;
 for(unsigned i=0;i<27;i++)v->table[i+2]=f[i];
 v->base[0]=(uint32_t)(uintptr_t)(v->table+2);v->base[7]=0; /* model request false; reserved bytes were zeroed */
 return 1;
}
