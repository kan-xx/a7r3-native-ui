#ifndef MP_RESIDENT_ROUTE_H
#define MP_RESIDENT_ROUTE_H
#include <stdint.h>
struct mpr_port {void*context;uint32_t(*tid)(void*);int(*read)(void*,uint32_t,void*,uint32_t);int(*cas)(void*,uint32_t,uint32_t,uint32_t);uint32_t owner_tid,view2,caution,property,flag,route_literal;};
struct mpr_report {uint32_t version,status,prepared,installed,restored,reads,bytes,checks,aa,root,leaf,last_address,last_n,conflict;};
struct mpr_context {struct mpr_port port;struct mpr_report report;uint32_t property[12];};
/* Core must retain its own reference and register its exact provider BEFORE
 * installing the route. This module never mutates tree selection or flag. */
int mpr_prepare(struct mpr_context*,const struct mpr_port*);
int mpr_install(struct mpr_context*);
/* Fresh AA current object, not saved from a previous opening. Caller separately
 * qualifies original ViewManager/slot55, empty ownslot24, factory LR/UI TID. */
int mpr_factory_context(struct mpr_context*,uint32_t aa);
int mpr_restore(struct mpr_context*);
int mpr_restored_current(struct mpr_context*);
int mpr_reinstall(struct mpr_context*);
#endif
