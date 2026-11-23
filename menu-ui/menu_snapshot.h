#ifndef A7_MENU_SNAPSHOT_H
#define A7_MENU_SNAPSHOT_H
#include <stdint.h>
#include <stddef.h>
#define A7_MENU_MAX_SLOTS 64u
#define A7_MENU_MAX_NODES 512u
#define A7_MENU_TEXT 96u
/* Native little-endian ARM addresses. read must fail without dereferencing an
 * inaccessible address; it must not change target data or call a Sony getter. */
typedef int (*a7_menu_read)(void *,uint32_t,void *,uint32_t);
enum a7_menu_snapshot_status { A7_MENU_OK=0,A7_MENU_BAD_ARG=1,
 A7_MENU_UNREADABLE=2,A7_MENU_BAD_MANAGER=3,A7_MENU_BAD_LINK=4,
 A7_MENU_NODE_LIMIT=5,A7_MENU_DUPLICATE_ID=6,A7_MENU_NO_MANAGER=7,
 A7_MENU_READ_BUDGET=8 };
struct a7_menu_view {uint32_t bank,index,handle,view,id,library,vptr,registry_matches,registered_module,registered_factory;
 char module[A7_MENU_TEXT],factory[A7_MENU_TEXT];};
struct a7_menu_snapshot {
 uint32_t version,status,reads,bytes,failed_address;
 uint32_t manager,registry,app_config,registry_vptr,registry_count;
 uint32_t configured_slots,active_count,sentinel,tail,registry_is_singleton;
 uint32_t selected_id,selected_count,selected_node,selected_module_ptr,selected_factory_ptr;
 char selected_module[A7_MENU_TEXT],selected_factory[A7_MENU_TEXT];
 struct a7_menu_view active[2*A7_MENU_MAX_SLOTS];
};
_Static_assert(sizeof(struct a7_menu_view)==232,"wire view size");
_Static_assert(sizeof(struct a7_menu_snapshot)==29968,"wire snapshot size");
/* Execute ONLY in the verified UI owner callback, while libObj's identity and
 * pViewManager symbol address have been qualified independently. No functions
 * in the Sony object graph are invoked. Output is caller-owned and exclusive. */
int a7_menu_take_snapshot(a7_menu_read,void *,uint32_t pviewmanager,
 uint32_t expected_singleton,uint32_t selected_id,struct a7_menu_snapshot *);
#endif
