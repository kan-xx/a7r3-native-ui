#include "graphics.h"
extern "C" int a7_gfx_create_caught(struct a7_gfx_context*c){try{return a7_gfx_create(c);}catch(...){c->report.exception=1;c->report.uncertain=1;c->report.status=90;return 0;}}
extern "C" int a7_gfx_destroy_caught(struct a7_gfx_context*c){try{return a7_gfx_destroy(c);}catch(...){c->report.exception=1;c->report.uncertain=1;c->report.status=91;return 0;}}

extern "C" int a7_gfx_update_caught(struct a7_gfx_context*c,uint32_t r,uint32_t s,uint32_t state){try{return a7_gfx_update(c,r,s,state);}catch(...){c->report.exception=1;c->report.uncertain=1;c->report.status=92;return 0;}}
