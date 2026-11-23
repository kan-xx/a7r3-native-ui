#ifndef A7_NAVIGATION_H
#define A7_NAVIGATION_H
#include <stdint.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
struct a7_navigation_api {
 void *event_manager;
 void *(*param_ctor)(void *,int);
 void *(*event_dtor)(void *);
 int (*push)(void *,void *,unsigned char);
 int (*trylock)(void *),(*unlock)(void *);
 void *(*allocate)(size_t);
 void (*release)(void *);
 uint32_t qualified;
};
struct a7_navigation_result {
 uint32_t version,status,prepared,submitted;
 uint32_t event_id,destination,category,target_view;
 uint32_t mode,allocation_count,release_count,push_return;
 uint32_t caught_exception,exception_unlock,precheck_busy,reserved;
};
enum { A7_NAV_OK=0,A7_NAV_IDENTITY=30,A7_NAV_ALLOC=31,A7_NAV_BUSY=32,
 A7_NAV_EXCEPTION=33,A7_NAV_PUSH_FAILED=34,A7_NAV_BAD_API=35 };
void a7_navigation_target(const struct a7_navigation_api *,uint32_t closing,uint32_t target,uint32_t mode,struct a7_navigation_result *);
void a7_navigation_submit(const struct a7_navigation_api *,uint32_t,struct a7_navigation_result *);
int a7_navigation_push_caught(const struct a7_navigation_api *,void *,struct a7_navigation_result *);
#ifdef __cplusplus
}
#endif
#endif
