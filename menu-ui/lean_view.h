#ifndef A7_LEAN_VIEW_H
#define A7_LEAN_VIEW_H
#include <stdint.h>
#include <stddef.h>
#include "graphics.h"
#include "page_control.h"
/* A source-level native ABI candidate, not a camera command. 27 primary slots
 * from original View address point libObj+0x1205470; opaque base spans 0x28.
 * All vtable storage and instances belong to this provider. */
struct a7_lean_api {
 void *(*sony_ctor)(void *,void *);
 void *(*sony_dtor)(void *);
 void (*release)(void *);
 uint32_t (*event_id)(void *);
 uintptr_t original_view_typeinfo;
};
struct a7_lean_report {uint32_t ctor,entry,exit,main,action,onload,onunload,dtor,deleting,last_event,layout_open,layout_close,layer,owner_tid,closed,entry_tid,dtor_tid,thread_errors;};
struct a7_lean_view {
 uint32_t base[10];
 const struct a7_lean_api *api;
 struct a7_lean_report *report;
 struct a7_gfx_context *graphics;
 struct mp_page *control;uint32_t serial;
 uintptr_t table[29]; /* offset-to-top, RTTI, then exactly 27 entries */
};
int a7_lean_construct(struct a7_lean_view *,const struct a7_lean_api *,struct a7_lean_report *,void *manager);
#endif
