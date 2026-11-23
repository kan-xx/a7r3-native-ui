#ifndef A7_GRAPHICS_H
#define A7_GRAPHICS_H
#include <stdint.h>
#include <stddef.h>
#include "menu_snapshot.h"
#define A7_LAYER_ID 0xa7ffc007u
#define A7_GFX_MAX 128
#define A7_GFX_LABELS 8
struct a7_gfx_entry{uint32_t id,object;};
struct a7_gfx_snapshot{uint32_t status,reads,bytes,layers,screens,widget,engine,resource;
 struct a7_gfx_entry layer[A7_GFX_MAX],screen[A7_GFX_MAX];};
struct a7_gfx_api{
 a7_menu_read reader;void*reader_context;uint32_t widget_global,engine_global,resource_global,default_id,hidden_id,invisible_id;
 void*(*allocate)(size_t);void(*release)(void*);
 void*(*layer_create)(void*,const uint32_t*);void*(*layer_dtor)(void*);void(*layer_active)(void*,int);
 void*(*label_ctor)(void*);void*(*label_dtor)(void*);void(*label_layer)(void*,const uint32_t*);
 void(*label_size)(void*,float,float);void(*label_position)(void*,const float*);void(*label_text)(void*,const char*);void(*label_font)(void*,uint32_t);
 void*(*font_ctor)(void*);void*(*font_dtor)(void*);void(*font_resource)(void*,uint32_t);void(*font_size)(void*,uint8_t);void(*font_color)(void*,const void*);
 void*(*record_ctor)(void*);void*(*record_dtor)(void*);uint32_t(*record_id)(void*);int(*font_create)(const void*,void*);
};
struct a7_gfx_report{uint32_t version,stage,status,exception,uncertain,owner_tid,create_tid,destroy_tid,thread_errors;
 uint32_t pre_valid,post_valid,restored,layer,screen,label,font_id,constructed,active,destructed,dirty;
 uint32_t before_layers,before_screens,after_layers,after_screens,reads,bytes;};
struct a7_gfx_context{struct a7_gfx_api api;struct a7_gfx_report report;struct a7_gfx_snapshot before,work,pre_destroy;
 void*layer;void*label;void*labels[A7_GFX_LABELS];char text[A7_GFX_LABELS][64];uint32_t record[2],font[18];uint32_t font_live,record_live,label_live,layer_live;};
#ifdef __cplusplus
extern "C" {
#endif
int a7_gfx_snapshot(const struct a7_gfx_api*,struct a7_gfx_snapshot*);
int a7_gfx_prepare(struct a7_gfx_context*,const struct a7_gfx_api*,uint32_t);
int a7_gfx_create(struct a7_gfx_context*);
int a7_gfx_destroy(struct a7_gfx_context*);
void a7_gfx_observe(struct a7_gfx_context*);
int a7_gfx_update(struct a7_gfx_context*,uint32_t,uint32_t,uint32_t);
int a7_gfx_update_caught(struct a7_gfx_context*,uint32_t,uint32_t,uint32_t);
int a7_gfx_create_caught(struct a7_gfx_context*);
int a7_gfx_destroy_caught(struct a7_gfx_context*);
#ifdef __cplusplus
}
#endif
#endif
