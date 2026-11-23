#ifndef A7_ENTRY_ROUTE_H
#define A7_ENTRY_ROUTE_H
#include <stdint.h>
#include "menu_snapshot.h"
struct a7_route_api {
 a7_menu_read read; void *context;
 int (*cas)(void *,uint32_t,uint32_t,uint32_t);
 uint32_t view2,caution,property,original_target,flag,persistent_route;
 int (*select)(void *); void (*action)(void *,uint32_t); void (*set_flag)(uint8_t);
};
struct a7_route_report {uint32_t version,status,step,reads,bytes,nodes,matches,root,path[3],old[3],selected,restored,route_installed,route_restored,action_called,action_returned,exception,flag_before,flag_after,flag_restored,last_address,last_n,flag_restore_write;};
struct a7_route_node {uint32_t address,word[10];};
struct a7_route_context {struct a7_route_api api;struct a7_route_report report;
 uint32_t aa,aa_vptr,manager,root,property[12],saved_count,dirty,uncertain;
 struct a7_route_node saved[10];uint32_t path[3],old[3],parents[3];};
int a7_route_prepare(struct a7_route_context*,const struct a7_route_api*,uint32_t,uint32_t);
int a7_route_execute(struct a7_route_context*);
int a7_route_restore(struct a7_route_context*);
int a7_route_finish_flag(struct a7_route_context*);
int a7_route_select_caught(int(*)(void*),void*,uint32_t*);
int a7_route_action_caught(void(*)(void*,uint32_t),void*,uint32_t*);
#endif
