#include "product.h"
#include "menu_power.h"
static struct{struct lp_control power;uint32_t power_bound,power_ui_epoch,power_wake_epoch,power_work_epoch,power_work_phase,power_ticks;struct mp_native_api a;struct mp_report r;struct mp_audio_native backend;struct mat_context audio;struct mp_page page;struct mpr_context route;struct a7_gfx_context gfx;struct a7_lean_report view;struct a7_lean_api view_api;struct a7_menu_snapshot snapshot;struct mp_stock_result stock;struct md_context diagnostic;uint32_t diagnostic_requested;
 void*reference,*queued,*current;uint32_t initialized,admission,vm,aa,aa_vptr,inside_timer,stop_requested,close_submitted,remove_ticks,drained,host_busy;uint32_t cache_seq;struct mp_report cache;
} s;
static uint32_t syscall0(uint32_t n){register uint32_t r0 __asm__("r0");register uint32_t r7 __asm__("r7")=n;__asm__ volatile("svc 0":"=r"(r0):"r"(r7):"memory","cc");return r0;}
static uint32_t tid(void*p){(void)p;return syscall0(224);}static void*mutex(void){return(uint8_t*)s.a.manager+8;}static void*head(void){return(uint8_t*)s.a.manager+4;}
static int eq(const char*a,const char*b){for(unsigned i=0;i<96;i++){if(a[i]!=b[i])return 0;if(!a[i])return 1;}return 0;}
static void publish(void){s.r.reference=s.reference!=0;s.r.queued=s.queued!=0;s.r.backend_guard=s.backend.guard!=0;s.r.route=s.route.report;s.r.page=s.page.report;s.r.audio=s.audio.report;s.r.graphics=s.gfx.report;s.r.view=s.view;s.r.stock=s.stock;s.r.diagnostic=s.diagnostic.report;s.r.stop_requested=__atomic_load_n(&s.stop_requested,__ATOMIC_ACQUIRE);
 __atomic_add_fetch(&s.cache_seq,1,__ATOMIC_ACQ_REL);uint32_t*d=(void*)&s.cache;const uint32_t*p=(const void*)&s.r;for(unsigned i=0;i<sizeof(s.r)/4;i++)__atomic_store_n(d+i,p[i],__ATOMIC_RELAXED);__atomic_add_fetch(&s.cache_seq,1,__ATOMIC_RELEASE);
}
static int cached(struct mp_report*out){if(!out)return 3;for(unsigned attempt=0;attempt<3;attempt++){uint32_t a=__atomic_load_n(&s.cache_seq,__ATOMIC_ACQUIRE);if(a&1)continue;uint32_t*d=(void*)out;const uint32_t*p=(const void*)&s.cache;for(unsigned i=0;i<sizeof(*out)/4;i++)d[i]=__atomic_load_n(p+i,__ATOMIC_RELAXED);if(a==__atomic_load_n(&s.cache_seq,__ATOMIC_ACQUIRE))return 0;}return 2;}
static int take(void){a7_menu_take_snapshot(s.a.reader,s.a.reader_context,s.a.view_manager_global,s.a.view_registry_singleton,0xffc,&s.snapshot);return s.snapshot.status==0&&s.snapshot.version==1&&s.snapshot.configured_slots==39&&s.snapshot.registry_is_singleton;}
static int registry_ours(void){return s.snapshot.selected_count==1&&eq(s.snapshot.selected_module,MP_CORE_PATH)&&eq(s.snapshot.selected_factory,MP_FACTORY_NAME)&&(!s.r.registry_node||s.snapshot.selected_node==s.r.registry_node);}
static unsigned count(uint32_t id){unsigned n=0;for(unsigned i=0;i<s.snapshot.active_count;i++)if(s.snapshot.active[i].id==id)n++;return n;}
static int aa_current(void){uint32_t aa=0,vp=0,banks=0,n=0;for(unsigned i=0;i<s.snapshot.active_count;i++){const struct a7_menu_view*v=&s.snapshot.active[i];if(v->id!=0xaa)continue;if(v->bank>1||v->index!=19||!v->view||!v->registry_matches||!eq(v->factory,"ViewSettingMenuToInstance")||!eq(v->module,"viewUnified2.so")||(n&&(v->view!=aa||v->vptr!=vp)))return 0;aa=v->view;vp=v->vptr;banks|=1u<<v->bank;n++;}if(n!=2||banks!=3)return 0;s.aa=aa;s.aa_vptr=vp;return 1;}
static int row_clear(void){uint32_t m[10],row[39],slot[4],mode;
 if(!s.a.reader(0,s.a.view_mode_word,&mode,4)||mode||!s.a.reader(0,s.vm,m,40)||m[0]!=39||m[2]!=s.a.view_registry_singleton||!m[3]||m[3]>0xffffffffu-(56*39*4))return 0;
 if(!s.a.reader(0,m[3]+55*39*4,row,104)||!s.a.reader(0,m[3]+55*39*4+104,row+26,52))return 0;
 const int32_t expected[39]={-4,-4,-4,-4,-4,-4,-4,-4,-4,-2,-4,-4,-2,-2,-2,-2,-2,-4,-4,-4,-2,-2,-2,-2,-3,-2,-2,-4,-2,-2,-2,-2,-4,-2,-2,-2,-4,-4,-2};
 for(unsigned i=0;i<39;i++)if(row[i]!=(uint32_t)expected[i])return 0;
 for(unsigned b=0;b<2;b++){if(!m[8+b]||m[8+b]>0xffffffffu-39*16)return 0;for(unsigned i=0;i<39;i++)if(expected[i]==-2||i==24){if(!s.a.reader(0,m[8+b]+i*16,slot,16))return 0;if(!slot[0]&&!slot[1]&&slot[2]==0xffffffffu&&!slot[3])continue;int pb=0;if(i==13&&slot[2]==175)for(unsigned k=0;k<s.snapshot.active_count;k++){const struct a7_menu_view*v=&s.snapshot.active[k];if(v->bank==b&&v->index==13&&v->id==175&&v->view==slot[1]&&v->registry_matches&&eq(v->factory,"ViewSinglePbToInstance"))pb=1;}if(!pb)return 0;}}
 return 1;
}
static int own_live(void){uint32_t vm,m[10],slot[4],vp;if(!s.r.view_live||!s.r.object||!s.a.reader(0,s.a.view_manager_global,&vm,4)||vm!=s.vm||!s.a.reader(0,vm,m,40))return 0;for(unsigned b=0;b<2;b++){if(!m[8+b]||m[8+b]>0xffffffffu-39*16||!s.a.reader(0,m[8+b]+24*16,slot,16)||slot[1]!=s.r.object||slot[2]!=0xffc)return 0;}return s.a.reader(0,s.r.object,&vp,4)&&vp==s.r.vptr;}
static void hold(uint32_t error){lp_fault(&s.power,error);s.r.phase=MP_HOLD_PINNED;s.r.error=error;s.admission=0;}
static int schedule(void);
static int draw(void*p,uint32_t row,uint32_t selected,uint32_t state){(void)p;return a7_gfx_update_caught(&s.gfx,row,selected,state);}
static int close_page(void*p){(void)p;struct a7_navigation_result r={0};a7_navigation_target(&s.a.navigation,1,0xffc,0,&r);if(r.caught_exception){hold(31);return 0;}if(r.submitted)s.close_submitted=1;return r.submitted;}
static int stock_page(void*p){(void)p;if(!s.a.refresh()||!own_live()||!mpr_factory_context(&s.route,s.aa))return 0;int ok=mp_stock_open_caught(s.a.stock_open,&s.stock);if(!ok)hold(32);return ok;}
static void own_free(void*p){if((uintptr_t)p!=s.r.object||tid(0)!=s.r.owner){hold(33);return;}s.r.view_live=0;s.r.object=s.r.vptr=0;s.a.record_release(p);}
static void*factory_inner(uint32_t id,void*manager,uintptr_t lr){
 if(!s.initialized||tid(0)!=s.r.owner||syscall0(20)!=s.r.pid)return 0;s.r.factory_calls++;s.r.factory_lr=(uint32_t)lr;
 if(!s.admission||__atomic_load_n(&s.stop_requested,__ATOMIC_ACQUIRE)||s.r.phase!=MP_RESIDENT||s.r.view_live||s.gfx.report.uncertain||id!=0xffc||lr!=s.a.factory_return||(uintptr_t)manager!=s.vm||!s.a.refresh()||!take()||!registry_ours()||count(0xffc)||!aa_current()||!row_clear()||!mpr_factory_context(&s.route,s.aa)){s.r.factory_rejected++;schedule();return 0;}
 if(!a7_gfx_prepare(&s.gfx,&s.a.graphics,s.r.owner)||!schedule()){s.r.factory_rejected++;schedule();return 0;}
 struct a7_lean_view*v=s.a.record_allocate(1,sizeof(*v));if(!v){s.r.factory_rejected++;schedule();return 0;}for(unsigned i=0;i<sizeof s.view;i++)((uint8_t*)&s.view)[i]=0;s.view.owner_tid=s.r.owner;s.view_api=s.a.lean;s.view_api.release=own_free;
 if(!a7_lean_construct(v,&s.view_api,&s.view,manager)){s.a.record_release(v);s.r.factory_rejected++;return 0;}v->graphics=&s.gfx;v->control=&s.page;s.r.view_live=1;s.r.object=(uintptr_t)v;s.r.vptr=v->base[0];s.close_submitted=0;return v;
}
__attribute__((visibility("default"),noinline))void*A7ProductViewFactory21(uint32_t id,void*manager){
 uintptr_t lr=(uintptr_t)__builtin_return_address(0);if(lp_commit(&s.power,adm_word(&s.power.gate)))return 0;
 void*r=factory_inner(id,manager,lr);lp_sent(&s.power);return r;
}
static void remove_step(void){
 s.admission=0;s.r.phase=MP_REMOVING;
 if(++s.remove_ticks>240){hold(40);return;}
 if(!s.a.refresh()){hold(41);return;}
 if(s.r.error==53){hold(53);return;}if(s.route.report.installed&&!mpr_restore(&s.route)){hold(42);return;}if(s.diagnostic.report.stage==MD_HOLD){hold(72);return;}
 if(s.r.view_live){if(!own_live()){hold(43);return;}if(!s.close_submitted&&!close_page(0)){hold(44);return;}return;}
 if(s.gfx.report.uncertain||s.gfx.layer_live||s.gfx.label_live){hold(45);return;}
 if(mp_page_needs_tick(&s.page)){mp_page_tick(&s.page);return;}
 if(s.r.registry_added&&!s.r.registry_removed){if(!take()||!registry_ours()||count(0xffc)){hold(46);return;}if(!mp_registry_remove_caught(&s.a.registry,(void*)(uintptr_t)s.a.view_registry_singleton)){hold(47);return;}if(!take()||s.snapshot.selected_count||count(0xffc)){hold(48);return;}s.r.registry_removed=1;}
 if(mp_audio_ui_close(&s.backend)){hold(49);return;}s.r.phase=MP_STOPPED_PINNED;
}
static uint32_t diag_owner(void*p){(void)p;return s.r.owner;}
static int diag_snapshot(void*p,struct a7_menu_snapshot*out){(void)p;if(!take())return 0;*out=s.snapshot;return 1;}
static int diag_own(void*p){(void)p;return own_live();}
static int diag_entry(void*p){(void)p;return mpr_factory_context(&s.route,s.aa);}
static void diagnostic_step(void){
 uint32_t request=__atomic_exchange_n(&s.diagnostic_requested,0,__ATOMIC_ACQ_REL);
 if(request==1){struct md_port p={0,&s.a,&s.page,diag_owner,diag_snapshot,diag_own,close_page,diag_entry,s.route.port.route_literal};if(!md_start(&s.diagnostic,&p))s.r.error=70;}
 if(request==2)md_continue(&s.diagnostic);
 if(md_needs_tick(&s.diagnostic)&&!md_tick(&s.diagnostic))hold(71);
}
static int power_needs_tick(void){uint32_t p=lp_phase(&s.power),e=lp_epoch(&s.power);return (p==LP_CLOSING&&__atomic_load_n(&s.power_ui_epoch,__ATOMIC_ACQUIRE)!=e)||(p==LP_WAKING&&__atomic_load_n(&s.power_wake_epoch,__ATOMIC_ACQUIRE)!=e);}
static void power_ui_step(void){
 uint32_t phase=lp_phase(&s.power),epoch=lp_epoch(&s.power);
 if(s.power_work_epoch!=epoch||s.power_work_phase!=phase){s.power_work_epoch=epoch;s.power_work_phase=phase;s.power_ticks=0;}
 if(!power_needs_tick())return;if(++s.power_ticks>40){hold(80);return;}
 if(!s.a.refresh()){hold(81);return;}
 if(phase==LP_CLOSING){
  __atomic_store_n(&s.diagnostic_requested,0,__ATOMIC_RELEASE);
  mat_cancel(&s.audio);if(mp_page_needs_tick(&s.page)&&!mp_page_tick(&s.page)){hold(82);return;}
  if(md_needs_tick(&s.diagnostic)){md_abort(&s.diagnostic);if(!md_tick(&s.diagnostic))hold(83);return;}
  if(s.diagnostic.report.stage==MD_HOLD){hold(83);return;}
  if(s.r.view_live){if(!own_live()){hold(84);return;}if(!s.close_submitted&&!close_page(0))hold(85);return;}
  if(s.gfx.report.uncertain||s.gfx.layer_live||s.gfx.label_live){hold(86);return;}
  if(mp_page_needs_tick(&s.page))return;
  if(!mpr_restore(&s.route)||!mpr_restored_current(&s.route)||!take()||!registry_ours()||count(0xffc)){hold(87);return;}
  __atomic_store_n(&s.power_ui_epoch,epoch,__ATOMIC_RELEASE);
 }else if(phase==LP_WAKING){
  if(s.r.view_live||s.gfx.report.uncertain||s.gfx.layer_live||s.gfx.label_live||mp_page_needs_tick(&s.page)||md_needs_tick(&s.diagnostic)||!take()||!registry_ours()||count(0xffc)){hold(88);return;}
  int rc=mat_power_observe(&s.audio);if(rc==MAT_BUSY)return;if(rc){hold(89);return;}
  if(!mpr_reinstall(&s.route)){hold(90);return;}
  __atomic_store_n(&s.power_wake_epoch,epoch,__ATOMIC_RELEASE);
 }
}
static __attribute__((noinline))void timer_callback(void*record,struct cx_timespec32*due,void*context){(void)due;uintptr_t lr=(uintptr_t)__builtin_return_address(0);
 if(context!=&s||record!=s.queued)return;s.queued=0;s.current=record;s.inside_timer=1;s.r.callbacks++;
 s.r.callback_identity=tid(0)==s.r.owner&&syscall0(20)==s.r.pid&&lr==s.a.expected_return&&*(uint32_t*)((uint8_t*)s.a.manager+32)==1;
 if(!s.r.callback_identity)hold(50);
 else if(s.r.phase==MP_INSTALLING){
  if(!s.a.refresh()||!take()||s.snapshot.selected_count||count(0xffc)){hold(51);}
  else{s.vm=s.snapshot.manager;s.a.route.context=0;s.a.route.tid=tid;s.a.route.owner_tid=s.r.owner;static const char route_literal[]="@VFFC";s.a.route.route_literal=(uintptr_t)route_literal;
   if(!mpr_prepare(&s.route,&s.a.route))hold(52);
   else if(!mp_registry_add_caught(&s.a.registry,(void*)(uintptr_t)s.a.view_registry_singleton))hold(53);
   else{s.r.registry_added=1;if(!take()||!registry_ours()){hold(54);}else{s.r.registry_node=s.snapshot.selected_node;int route_ok=lp_closed(&s.power)?mpr_restore(&s.route):mpr_install(&s.route);if(!route_ok)hold(55);else{s.admission=1;s.r.phase=MP_RESIDENT;}}}
  }
 }
 else if(s.r.phase==MP_RESIDENT){
  if(__atomic_load_n(&s.stop_requested,__ATOMIC_ACQUIRE)){if(md_needs_tick(&s.diagnostic)){md_abort(&s.diagnostic);if(!md_tick(&s.diagnostic))hold(71);}else remove_step();}
  else if(lp_closed(&s.power))power_ui_step();
  else if(s.r.view_live){if(!s.a.refresh()||!own_live()){hold(56);}else if(!s.page.report.active||s.gfx.report.status){if(!s.close_submitted&&!close_page(0))hold(57);}else if(!mp_page_tick(&s.page))hold(58);else if(!s.gfx.report.post_valid)a7_gfx_observe(&s.gfx);}
  else if(mp_page_needs_tick(&s.page))mp_page_tick(&s.page);
  if(s.r.phase==MP_RESIDENT&&!lp_closed(&s.power)&&!__atomic_load_n(&s.stop_requested,__ATOMIC_ACQUIRE))diagnostic_step();
 }
 else if(s.r.phase==MP_REMOVING)remove_step();
 else if(s.r.phase==MP_HOLD_PINNED&&__atomic_load_n(&s.stop_requested,__ATOMIC_ACQUIRE)){if(md_needs_tick(&s.diagnostic)){md_abort(&s.diagnostic);md_tick(&s.diagnostic);}else remove_step();}
 if((s.r.phase==MP_RESIDENT&&(power_needs_tick()||(!lp_closed(&s.power)&&(s.r.view_live||mp_page_needs_tick(&s.page)||md_needs_tick(&s.diagnostic)||__atomic_load_n(&s.diagnostic_requested,__ATOMIC_ACQUIRE)))||__atomic_load_n(&s.stop_requested,__ATOMIC_ACQUIRE)))||s.r.phase==MP_REMOVING)if(!schedule())hold(59);
 s.a.remove_locked(head(),record);s.r.records_freed++;s.current=0;s.inside_timer=0;publish();
}
static int schedule(void){
 int locked=0;int on_ui=tid(0)==s.r.owner;if(!(on_ui&&s.inside_timer)){if(s.a.trylock(mutex()))return 0;locked=1;if(*(uint32_t*)((uint8_t*)s.a.manager+32)){s.a.unlock(mutex());return 0;}}
 if(s.queued){if(locked)s.a.unlock(mutex());return 1;}struct cx_timespec32 t={0};int ok=0;if(!s.a.now(&t)&&t.sec>=0&&t.sec<2147483647&&t.nsec>=0&&t.nsec<1000000000){t.nsec+=250000000;if(t.nsec>=1000000000){t.sec++;t.nsec-=1000000000;}void*r=s.a.record_allocate(1,24);if(r){s.r.records_allocated++;if(!s.a.insert_locked(head(),r,&t,timer_callback,&s)){s.queued=r;ok=1;if(((uint8_t*)r)[20]){s.a.remove_locked(head(),r);s.r.records_freed++;s.queued=0;ok=0;}}else{s.a.record_release(r);s.r.records_freed++;}}}
 if(locked)s.a.unlock(mutex());return ok;
}
static int start_mode(void*ref,const struct mp_bind*b,uint32_t*adopted,struct mp_report*out,int paused){if(!ref||!b||!adopted||!out||b->version!=1||b->bytes!=48||b->backend_kind>2||!b->owner_tid||tid(0)==b->owner_tid)return 3;*adopted=0;uint32_t zero=0;if(!__atomic_compare_exchange_n(&s.host_busy,&zero,1,0,__ATOMIC_ACQUIRE,__ATOMIC_RELAXED))return 2;
 if(s.initialized){__atomic_store_n(&s.host_busy,0,__ATOMIC_RELEASE);cached(out);return 4;}s.initialized=1;s.r.version=1;s.r.bytes=sizeof s.r;s.r.phase=MP_INSTALLING;s.r.pid=syscall0(20);s.r.owner=b->owner_tid;s.r.backend_kind=b->backend_kind;for(unsigned i=0;i<16;i++)s.r.nonce[i]=b->activation_nonce[i];
 if(paused&&lp_pause(&s.power,1)!=LP_OK){s.r.error=92;s.r.phase=MP_HOLD_PINNED;publish();cached(out);__atomic_store_n(&s.host_busy,0,__ATOMIC_RELEASE);return 92;}
 struct mat_port audioport;struct mp_page_port pageport={0,tid,draw,close_page,stock_page};int rc=0;
 if(!mp_native_bind(&s.a,&s.r.native))rc=20;else if(*(uint32_t*)((uint8_t*)s.a.manager+36)!=s.r.pid)rc=21;else if(mp_audio_host_bind(&s.backend,b->owner_tid,b->backend_kind,b->audio_instance,&audioport))rc=22;else if(mat_init(&s.audio,&audioport,b->owner_tid,b->activation_nonce)||!mp_page_init(&s.page,&pageport,&s.audio,b->owner_tid))rc=23;
 if(!rc){s.page.power=&s.power;s.reference=ref;*adopted=1;publish();if(!schedule())rc=24;}
 if(rc){s.r.error=rc;s.r.phase=MP_HOLD_PINNED;if(!s.queued&&!s.backend.ui_calls&&!s.backend.reference_uncertain){int ar=s.backend.guard?mp_audio_host_abort_unpublished(&s.backend):0;if(!ar){s.reference=0;*adopted=0;mp_native_close_unpublished();}}}
 if(!rc)__atomic_store_n(&s.power_bound,1,__ATOMIC_RELEASE);if(rc)publish();cached(out);__atomic_store_n(&s.host_busy,0,__ATOMIC_RELEASE);return rc;
}
static int start(void*ref,const struct mp_bind*b,uint32_t*adopted,struct mp_report*out){return start_mode(ref,b,adopted,out,0);}
static int start_paused(void*ref,const struct mp_bind*b,uint32_t*adopted,struct mp_report*out){return start_mode(ref,b,adopted,out,1);}
static int query(struct mp_report*out){if(!__atomic_load_n(&s.cache_seq,__ATOMIC_ACQUIRE))return 1;int rc=cached(out);if(!rc&&__atomic_load_n(&s.drained,__ATOMIC_ACQUIRE))out->backend_guard=0;return rc;}
static int request_remove(struct mp_report*out){if(!out||query(out)||!out->reference)return 3;lp_fault(&s.power,91);__atomic_store_n(&s.stop_requested,1,__ATOMIC_RELEASE);/* Host only arms the existing qualified original timer. */if(!schedule())return 2;return cached(out);}
static int drain(struct mp_report*out){if(!out||query(out)||!out->reference||tid(0)==out->owner)return 3;uint32_t zero=0;if(!__atomic_compare_exchange_n(&s.host_busy,&zero,1,0,__ATOMIC_ACQUIRE,__ATOMIC_RELAXED))return 2;
 if(s.a.trylock(mutex())){__atomic_store_n(&s.host_busy,0,__ATOMIC_RELEASE);return 2;}int safe=s.r.phase==MP_STOPPED_PINNED&&!s.queued&&!s.current&&!s.r.view_live;s.a.unlock(mutex());int rc=0;
 if(!safe){query(out);rc=1;}else if(!__atomic_load_n(&s.drained,__ATOMIC_ACQUIRE)){rc=mp_audio_host_release(&s.backend);if(!rc)__atomic_store_n(&s.drained,1,__ATOMIC_RELEASE);}
 if(safe)query(out);__atomic_store_n(&s.host_busy,0,__ATOMIC_RELEASE);return rc;/* Independent provider lifetime pin deliberately retained. */}

static int diagnostic_request(uint32_t op,struct mp_report*out){if(lp_closed(&s.power)||!out||query(out)||out->phase!=MP_RESIDENT||(op!=1&&op!=2))return 3;
 if((op==1&&out->diagnostic.stage)||(op==2&&out->diagnostic.stage!=MD_PAUSE_OWN&&out->diagnostic.stage!=MD_PAUSE_STOCK))return 4;
 uint32_t zero=0;if(!__atomic_compare_exchange_n(&s.diagnostic_requested,&zero,op,0,__ATOMIC_RELEASE,__ATOMIC_RELAXED))return 2;if(!schedule())return 2;return cached(out);
}
static int diagnostic_snapshot(uint32_t index,uint32_t offset,uint8_t*out,uint32_t capacity,uint32_t*copied){if(!out||!copied||index>3||!capacity||capacity>8192||offset>=sizeof(struct a7_menu_snapshot)||!__atomic_load_n(&s.cache_seq,__ATOMIC_ACQUIRE)||!s.a.manager)return 3;*copied=0;
 if(s.a.trylock(mutex()))return 2;uint32_t stage=s.diagnostic.report.stage;
 /* Only immutable completed snapshots may be streamed over multiple host
  * requests. Slot 3 is scratch until DONE/HOLD; stock slot 2 repeats while
  * waiting for both stock views to materialize. */
 int stable=index==0||(index==1&&stage>=MD_PAUSE_OWN)||(index==2&&stage>=MD_PAUSE_STOCK)||(index==3&&(stage==MD_DONE||stage==MD_HOLD));
 if(!stable||!(s.diagnostic.report.snapshots&(1u<<index))){s.a.unlock(mutex());return 1;}uint32_t n=sizeof(struct a7_menu_snapshot)-offset;if(n>capacity)n=capacity;const uint8_t*p=(const void*)&s.diagnostic.snap[index];for(unsigned i=0;i<n;i++)out[i]=p[offset+i];*copied=n;s.a.unlock(mutex());return 0;}
static const struct mp_api api={1,sizeof(struct mp_api),MP_BUILD,start,query,request_remove,drain,diagnostic_request,diagnostic_snapshot};
__attribute__((visibility("default")))const struct mp_api*a7r_menu_product_v21(void){return &api;}
static const struct mp_api boot_api={1,sizeof(struct mp_api),MP_BUILD,start_paused,query,request_remove,drain,diagnostic_request,diagnostic_snapshot};
__attribute__((visibility("default")))const struct mp_api*a7r_menu_bootstrap_v21(void){return &boot_api;}

static int power_identity(const uint8_t*id){if(!id||!__atomic_load_n(&s.power_bound,__ATOMIC_ACQUIRE)||tid(0)==s.r.owner||__atomic_load_n(&s.stop_requested,__ATOMIC_ACQUIRE))return 0;uint8_t diff=0;for(unsigned i=0;i<16;i++)diff|=id[i]^s.r.nonce[i];return !diff;}
static int power_pause(const uint8_t*id,uint32_t epoch){
 if(!power_identity(id))return LP_STATE;int r=lp_pause(&s.power,epoch);
 if(r==LP_BUSY||r==LP_ERROR||r==LP_STATE||r==LP_STALE)return r;
 if(power_needs_tick()&&!schedule())return LP_BUSY;return r;
}
static int power_resume(const uint8_t*id,uint32_t epoch){
 if(!power_identity(id))return LP_STATE;int r=lp_resume(&s.power,epoch);
 if(r==LP_BUSY||r==LP_ERROR||r==LP_STATE||r==LP_STALE)return r;
 if(power_needs_tick()&&!schedule())return LP_BUSY;return r;
}
static int power_query(struct mp_power_report*out){
 if(!out||!__atomic_load_n(&s.power_bound,__ATOMIC_ACQUIRE)||tid(0)==s.r.owner)return LP_STATE;
 uint32_t zero=0;if(!__atomic_compare_exchange_n(&s.host_busy,&zero,1,0,__ATOMIC_ACQUIRE,__ATOMIC_RELAXED))return LP_BUSY;
 if(s.a.trylock(mutex())){__atomic_store_n(&s.host_busy,0,__ATOMIC_RELEASE);return LP_BUSY;}
 struct mp_report cached_ui;if(cached(&cached_ui)){s.a.unlock(mutex());__atomic_store_n(&s.host_busy,0,__ATOMIC_RELEASE);return LP_BUSY;}
 for(unsigned i=0;i<sizeof*out;i++)((uint8_t*)out)[i]=0;
 out->abi=1;out->bytes=sizeof*out;out->build=MP_BUILD;out->pid=cached_ui.pid;out->owner=cached_ui.owner;out->error=cached_ui.error;
 out->ui_epoch=__atomic_load_n(&s.power_ui_epoch,__ATOMIC_ACQUIRE);out->wake_epoch=__atomic_load_n(&s.power_wake_epoch,__ATOMIC_ACQUIRE);
 out->route_suspended=cached_ui.route.restored;out->view_live=cached_ui.view_live;out->transaction_pending=cached_ui.page.active||(cached_ui.audio.state!=MAT_IDLE&&cached_ui.audio.state!=MAT_DONE&&cached_ui.audio.state!=MAT_FAILED);out->reference_retained=cached_ui.reference&&cached_ui.backend_guard;
 uint32_t phase=lp_phase(&s.power),epoch=lp_epoch(&s.power);
 int candidate=(phase==LP_CLOSING&&out->ui_epoch==epoch)||(phase==LP_WAKING&&out->wake_epoch==epoch)||phase==LP_PARKED;
 /* Only a closed gate + completed UI epoch permits reading live UI fields.
  * Otherwise a factory or destructor may still run outside the timer mutex;
  * the coherent cached report is informational and proves no callback return. */
 int safe=0;
 if(candidate&&(adm_word(&s.power.gate)&3)==1&&!*(volatile uint32_t*)((uint8_t*)s.a.manager+32)){
  int returned=!s.queued&&!s.current&&!s.inside_timer;
  safe=returned&&!s.r.view_live&&!mp_page_needs_tick(&s.page)&&!md_needs_tick(&s.diagnostic)&&!s.gfx.report.uncertain&&!s.gfx.layer_live&&!s.gfx.label_live&&s.r.phase==MP_RESIDENT&&!s.r.error;
  out->callback_returned=returned;out->route_suspended=s.route.report.restored;out->view_live=s.r.view_live;out->transaction_pending=mp_page_needs_tick(&s.page);out->error=s.r.error;
 }
 /* Capture all live UI facts before opening admission. No live UI reads
  * occur after lp_open; a new factory may then legitimately start. */
 if(safe&&phase==LP_CLOSING)lp_park(&s.power,epoch);
 if(safe&&phase==LP_WAKING)lp_open(&s.power,epoch);
 out->control.abi=1;out->control.bytes=sizeof out->control;out->control.build=MP_BUILD;lp_report(&s.power,&out->control);
 out->control.local_drained=safe&&out->control.phase==LP_PARKED&&out->ui_epoch==out->control.epoch;
 out->control.resume_qualified=out->control.phase==LP_ACTIVE&&out->wake_epoch==out->control.epoch&&out->control.epoch&&!(out->control.word&3);
 s.a.unlock(mutex());__atomic_store_n(&s.host_busy,0,__ATOMIC_RELEASE);return LP_OK;
}
static const struct mp_power_api power_api={1,sizeof(struct mp_power_api),MP_BUILD,power_pause,power_resume,power_query};
__attribute__((visibility("default")))const struct mp_power_api*a7r_menu_power_v21(void){return &power_api;}
