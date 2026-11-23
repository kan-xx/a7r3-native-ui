#ifndef MP_NATIVE_PORT_H
#define MP_NATIVE_PORT_H
#include "menu_snapshot.h"
#include "navigation.h"
#include "lean_view.h"
#include "resident_route.h"
#include "entry_route.h"
struct cx_timespec32{int32_t sec,nsec;};
typedef void(*cx_timer_cb)(void*,struct cx_timespec32*,void*);
struct mp_registry_api{void(*add)(void*,int,const char*,const char*);void(*remove)(void*,int);};
struct mp_native_api{
 uint32_t version,qualified;void*manager;int(*trylock)(void*),(*unlock)(void*),(*now)(struct cx_timespec32*);
 int(*insert_locked)(void*,void*,struct cx_timespec32*,cx_timer_cb,void*);void(*remove_locked)(void*,void*);uintptr_t expected_return;
 a7_menu_read reader;void*reader_context;int(*refresh)(void);uint32_t view_manager_global,view_registry_singleton,view_mode_word,setting_status_byte;
 struct a7_navigation_api navigation;struct mpr_port route;
 void*(*record_allocate)(size_t,size_t);void(*record_release)(void*);struct mp_registry_api registry;struct a7_lean_api lean;uintptr_t factory_return;struct a7_gfx_api graphics;
 void(*stock_open)(const char*,int);struct a7_route_api test_route;void*(*event_ctor)(void*,uint32_t,unsigned char,unsigned char);void(*input_dispatch)(void*,void*);
};
struct mp_native_report{uint32_t bind_stage,obj_base,manager,expected_return,map_count,map_lines,map_bytes,map_error;};
int mp_native_bind(struct mp_native_api*,struct mp_native_report*);
int mp_native_refresh(void);
void mp_native_close_unpublished(void);
#ifdef __cplusplus
extern "C" {
#endif
int mp_registry_add_caught(const struct mp_registry_api*,void*);
int mp_registry_remove_caught(const struct mp_registry_api*,void*);
#ifdef __cplusplus
}
#endif
#define MP_CORE_PATH "/usr/lib/liba7r_menu_product_v21.so"
#define MP_FACTORY_NAME "A7ProductViewFactory21"
#endif
