#include "native_port.h"
#include "map_reader.h"
extern void*dlsym(void*,const char*);struct dlinfo{const char*file;void*base;const char*symbol;void*address;};extern int dladdr(const void*,struct dlinfo*);
extern int pthread_mutex_trylock(void*),pthread_mutex_unlock(void*),open(const char*,int,...),close(int),read(int,void*,unsigned),pread64(int,void*,unsigned,long long);
extern void*malloc(size_t),*calloc(size_t,size_t);extern void free(void*);
static struct mp_map_reader mr;static int memory_fd=-1;static struct mp_native_report report;static uint32_t map_capture_bytes;
static int io_open(void*p,const char*s){(void)p;return open(s,0);}static int io_read(void*p,int fd,void*b,uint32_t n){(void)p;return read(fd,b,n);}static int io_close(void*p,int fd){(void)p;return close(fd);}static int io_pread(void*p,uint32_t a,void*b,uint32_t n){(void)p;return memory_fd<0?-1:pread64(memory_fd,b,n,(long long)a);}
int mp_native_refresh(void){int ok=mp_maps_refresh(&mr);map_capture_bytes=mr.bytes;return ok;}
static int collect_maps(void){mr.io=(struct mp_map_io){0,io_open,io_read,io_close,io_pread};return mp_native_refresh();}
static int read_memory(void*p,uint32_t a,void*b,uint32_t n){(void)p;return mp_maps_read(&mr,a,b,n);}
static int route_cas(void*p,uint32_t a,uint32_t old,uint32_t value){(void)p;if(!mr.valid||(a&3)||!a7_menu_maps_writable(&mr.active,a,4))return 0;return __sync_bool_compare_and_swap((uint32_t*)(uintptr_t)a,old,value);}
static int bytes(uintptr_t p,const uint8_t*s,unsigned n){for(unsigned i=0;i<n;i++)if(((const uint8_t*)p)[i]!=s[i])return 0;return 1;}
/* FW3.10 original ResourceImpl IDs and indexed TextLayout names; bounded reads only. */
static int chinese_font_identity(uint32_t b){
 uint32_t resource=0,ids_ptr=0,ids[5]={0},store=0,node=0,seen=0,hits=0;uint8_t count=0;
 if(!read_memory(0,b+0x129f3b8,&resource,4)||!resource||!read_memory(0,resource+0x1c,&ids_ptr,4)||!ids_ptr||!read_memory(0,resource+0x20,&count,1)||count!=5||!read_memory(0,ids_ptr,ids,sizeof ids))return 0;
 for(unsigned i=0;i<5;i++)if(ids[i]!=0x02066000u+i)return 0;
 if(!read_memory(0,b+0x129f6fc,&store,4)||!store||!read_memory(0,store,&node,4))return 0;
 for(unsigned i=0;i<5;i++){
  uint32_t row[5];char name[14];if(!node||!read_memory(0,node,row,sizeof row))return 0;
  unsigned index=((uint8_t*)row)[5];if(index>=5||(seen&(1u<<index)))return 0;seen|=1u<<index;
  if(index==3){if(!row[4]||!read_memory(0,row[4],name,sizeof name))return 0;static const char wanted[14]="FONT_S_CHINESE";for(unsigned j=0;j<14;j++)if(name[j]!=wanted[j])return 0;hits++;}
  node=row[0];
 }
 return !node&&seen==31&&hits==1;
}
static int bind(struct mp_native_api*a){
#ifdef MP_NATIVE_GUEST_FIXTURE
 int(*fn)(struct mp_native_api*,struct mp_native_report*)=dlsym(0,"mp_fixture_native_bind");return fn?fn(a,&report):0;
#else
 static const uint8_t add_bytes[16]={0x2d,0xe9,0xf0,0x41,0x84,0xb0,0x0d,0x46,0x02,0xaf,0x14,0x46,0x98,0x46,0x06,0x46};
 static const uint8_t remove_bytes[16]={0xb0,0xb5,0x0c,0x46,0x00,0xaf,0xff,0xf7,0xed,0xff,0x24,0xb1,0x20,0x46,0xbd,0xe8};
 static const uint8_t dispatch_bytes[16]={0xb0,0xb5,0x04,0x46,0x00,0xaf,0xff,0xf7,0x63,0xff,0x01,0x23,0x20,0x1d,0x23,0x62};
 static const uint8_t caller_bytes[16]={0x33,0x69,0x06,0xf1,0x08,0x01,0x72,0x68,0x05,0x46,0x30,0x46,0x98,0x47,0xeb,0xe7};
 void **slot=dlsym(0,"timerManager"); struct dlinfo info={0};report.bind_stage=1;
 if(!slot || !dladdr(slot,&info) || !info.base ||
   (uintptr_t)slot-(uintptr_t)info.base!=0x12d4accu)return 0;
 uintptr_t b=(uintptr_t)info.base;report.obj_base=(uint32_t)b;report.bind_stage=2;
 if(!bytes(b+0x82c0aa,add_bytes,16)||!bytes(b+0x82befe,remove_bytes,16)||
    !bytes(b+0x82e816,dispatch_bytes,16)||!bytes(b+0x82c258,caller_bytes,16))return 0;
 void *now=dlsym(0,"_Z10timeGetNowP8timespec");
 if((uintptr_t)now!=b+0x82bf17)return 0;
 report.bind_stage=3;void *manager=*slot;
 if(!manager || (uintptr_t)manager%4)return 0;
 /* Parent preflight must confirm this 40B live object is readable. This is a
  * current object, not the historical 2026-10-28 AudioSignals object. */
 report.manager=(uint32_t)(uintptr_t)manager;
 report.expected_return=(uint32_t)b+0x82c267;report.bind_stage=4;
 a->version=1;a->qualified=1;a->manager=manager;
 a->trylock=pthread_mutex_trylock;a->unlock=pthread_mutex_unlock;a->now=now;
 a->insert_locked=(void*)(b+0x82c039);a->remove_locked=(void*)(b+0x82beff);
 a->expected_return=b+0x82c267;
 report.bind_stage=5;void **view_slot=dlsym(0,"pViewManager");
 if((uintptr_t)view_slot!=b+0x12d4c04)return 0;
 #include "navigation_anchors.inc"
 #include "minimal_anchors.inc"
 #include "options_anchors.inc"
 #include "graphics_anchors.inc"
 report.bind_stage=6;int maps_ok=collect_maps();report.map_count=mr.active.count;
 report.map_lines=mr.active.lines;report.map_bytes=map_capture_bytes;if(!maps_ok)return 0;
 memory_fd=open("/proc/self/mem",0);report.bind_stage=7;
 uint32_t vm=0,em=0,table=0,q=0,links[2]={0};
 if(memory_fd<0||!read_memory(0,(uint32_t)(uintptr_t)view_slot,&vm,4)||!vm||
    !read_memory(0,vm+16,&em,4)||!em||!read_memory(0,em,&table,4)||!table||
    !read_memory(0,table,&q,4)||!q||!read_memory(0,q,links,8)||!links[0]||!links[1]||
    !a7_menu_maps_writable(&mr.active,em,40)||!a7_menu_maps_writable(&mr.active,q,8)){
  if(memory_fd>=0)close(memory_fd);memory_fd=-1;return 0;
 }
 report.bind_stage=8;if(!chinese_font_identity((uint32_t)b))return 0;
 void*setting_get=dlsym(0,"_ZN27CmnForUseMultipleViewStatus47getSettingMenuExistenceForCommonShootingSettingEv");struct dlinfo vi={0};
 if(!setting_get||!dladdr(setting_get,&vi)||!vi.base||(uintptr_t)setting_get!=(uintptr_t)vi.base+0x2e67ed)return 0;
 uintptr_t vb=(uintptr_t)vi.base;
 static const uint8_t setting_code[16]={0x03,0x4b,0x04,0x4a,0x7b,0x44,0x80,0xb5,0x00,0xaf,0x9b,0x58,0x18,0x78,0x80,0xbd};
 static const uint8_t input_code[16]={0x2d,0xe9,0xf0,0x4f,0x83,0xb0,0x4,0x46,0x0,0xaf,0x8,0x46,0x89,0x46,0xcc,0xf4};
 if(!bytes(vb+0x2e67ec,setting_code,16)||!bytes(b+0x8305bc,input_code,16))return 0;
 a->setting_status_byte=(uint32_t)vb+0xa6cd34;
 void*selection=dlsym(0,"_ZN18CmnViewSettingNode15setItemSelectedEv");struct dlinfo ci={0};
 if(!selection||!dladdr(selection,&ci)||!ci.base||(uintptr_t)selection!=(uintptr_t)ci.base+0x673725)return 0;
 uintptr_t cb=(uintptr_t)ci.base;
 #include "route_anchors.inc"
 if(!a7_menu_maps_writable(&mr.active,vb+0xa3821c,4)||!a7_menu_maps_writable(&mr.active,vb+0xa6cd34,1))return 0;
 a->route.read=read_memory;a->route.context=0;a->route.cas=route_cas;a->route.view2=(uint32_t)vb;a->route.caution=(uint32_t)cb;
 a->route.property=(uint32_t)vb+0xa381f4;a->route.flag=(uint32_t)vb+0xa6cd34;



 a->reader=read_memory;a->reader_context=0;a->refresh=mp_native_refresh;
 a->view_manager_global=(uint32_t)(uintptr_t)view_slot;
 a->view_registry_singleton=(uint32_t)b+0x12d4e2c;a->view_mode_word=(uint32_t)b+0x12d4e28;
 a->record_allocate=calloc;a->record_release=free;
 a->registry.add=(void*)(b+0x82ee65);a->registry.remove=(void*)(b+0x82ed83);
 a->lean.sony_ctor=(void*)(b+0x82cdd1);a->lean.sony_dtor=(void*)(b+0x82ccc9);a->lean.release=free;
 a->lean.event_id=dlsym(0,"_ZNK5Event5getIdEv");a->lean.original_view_typeinfo=b+0x12054e0;a->factory_return=b+0x831f07;
 if((uintptr_t)a->lean.event_id!=b+0x34610d)return 0;
 a->graphics.reader=read_memory;a->graphics.reader_context=0;a->graphics.allocate=malloc;a->graphics.release=free;
 a->graphics.widget_global=(uint32_t)b+0x129f470;
 a->graphics.engine_global=(uint32_t)b+0x129f40c;
 a->graphics.resource_global=(uint32_t)b+0x129f3b8;
 a->graphics.default_id=(uint32_t)b+0x129f460;
 a->graphics.hidden_id=(uint32_t)b+0x129f434;
 a->graphics.invisible_id=(uint32_t)b+0x129f418;
 a->graphics.layer_create=(void*)(b+0x5ccad1);
 a->graphics.layer_dtor=(void*)(b+0x5cae15);
 a->graphics.layer_active=(void*)(b+0x5cacb7);
 a->graphics.label_ctor=(void*)(b+0x3b9add);
 a->graphics.label_dtor=(void*)(b+0x3ba1d5);
 a->graphics.label_layer=(void*)(b+0x5c9095);
 a->graphics.label_size=(void*)(b+0x3ba129);
 a->graphics.label_position=(void*)(b+0x5c8fe5);
 a->graphics.label_text=(void*)(b+0x3b9717);
 a->graphics.label_font=(void*)(b+0x3b97f9);
 a->graphics.font_ctor=(void*)(b+0x777e53);
 a->graphics.font_dtor=(void*)(b+0x777e77);
 a->graphics.font_resource=(void*)(b+0x777eaf);
 a->graphics.font_size=(void*)(b+0x777ecd);
 a->graphics.font_color=(void*)(b+0x777f09);
 a->graphics.record_ctor=(void*)(b+0x798cf5);
 a->graphics.record_dtor=(void*)(b+0x798d23);
 a->graphics.record_id=(void*)(b+0x798d31);
 a->graphics.font_create=(void*)(b+0x5c6091);
 a->navigation.event_manager=(void*)em;
 a->navigation.param_ctor=(void*)(b+0x319291);
 a->navigation.event_dtor=(void*)(b+0x82cffb);
 a->navigation.push=(void*)(b+0x82e551);
 a->navigation.trylock=pthread_mutex_trylock;a->navigation.unlock=pthread_mutex_unlock;
 a->navigation.allocate=malloc;a->navigation.release=free;a->navigation.qualified=1;
 static const uint8_t stock_code[16]={128,181,0,175,255,247,144,255,189,232,128,64,28,245,126,187};
 a->stock_open=dlsym(0,"_ZN13viewManagerIf8openViewEPKci");
 if((uintptr_t)a->stock_open!=b+0x3e0da9||!bytes(b+0x3e0da8,stock_code,16))return 0;
 a->test_route=(struct a7_route_api){.read=read_memory,.cas=route_cas,.view2=(uint32_t)vb,.caution=(uint32_t)cb,.property=(uint32_t)vb+0xa381f4,.flag=(uint32_t)vb+0xa6cd34,.select=selection,.action=(void*)(vb+0x2179db),.set_flag=(void*)(vb+0x2e6805),.persistent_route=1};
 a->event_ctor=(void*)(b+0x82cfa5);a->input_dispatch=(void*)(b+0x8305bd);
 report.bind_stage=9;return 1;
#endif
}
int mp_native_bind(struct mp_native_api*a,struct mp_native_report*out){if(!a||!out||memory_fd>=0)return 0;int ok=bind(a);report.map_count=mr.active.count;report.map_lines=mr.active.lines;report.map_bytes=mr.bytes;report.map_error=mr.error;*out=report;if(!ok)mp_native_close_unpublished();return ok;}
void mp_native_close_unpublished(void){mr.valid=0;if(memory_fd>=0)close(memory_fd);memory_fd=-1;}
