#ifndef MP_MAP_READER_H
#define MP_MAP_READER_H
#include "menu_maps.h"
struct mp_map_io {void*context;int(*open)(void*,const char*);int(*read)(void*,int,void*,uint32_t);int(*close)(void*,int);int(*pread)(void*,uint32_t,void*,uint32_t);};
struct mp_map_reader {struct mp_map_io io;struct a7_menu_maps active,staging;uint32_t valid,generation,bytes,calls,error;char text[A7_MENU_MAP_TEXT_MAX+1];};
/* Fresh FD each qualification. Never reuse proc seq_file after EOF; native old
 * Sony kernel returns EOF on pread0 AND lseek0 after consuming the same FD. */
int mp_maps_refresh(struct mp_map_reader*);
int mp_maps_read(void*,uint32_t,void*,uint32_t);
#endif
