#include "map_reader.h"
int mp_maps_refresh(struct mp_map_reader*r){
 if(!r||!r->io.open||!r->io.read||!r->io.close||!r->io.pread)return 0;r->valid=0;r->bytes=0;r->calls=0;r->error=0;r->staging.count=0;r->staging.lines=0;
 int fd=r->io.open(r->io.context,"/proc/self/maps");if(fd<0){r->error=1;return 0;}int eof=0;
 while(r->bytes<sizeof r->text&&r->calls<1024){uint32_t room=(uint32_t)sizeof r->text-r->bytes;r->calls++;int n=r->io.read(r->io.context,fd,r->text+r->bytes,room);if(n<0){r->error=2;break;}if(!n){eof=1;break;}if((uint32_t)n>room){r->error=3;break;}r->bytes+=(uint32_t)n;}
 if(r->io.close(r->io.context,fd)){r->error=4;return 0;}if(!eof||r->error){if(!r->error)r->error=5;return 0;}
 if(!a7_menu_parse_maps(r->text,r->bytes,&r->staging)){r->error=6;return 0;}
 if(r->generation==0xffffffffu){r->error=7;return 0;}r->active=r->staging;r->generation++;r->valid=1;return 1;
}
int mp_maps_read(void*p,uint32_t a,void*d,uint32_t n){struct mp_map_reader*r=p;if(!r||!r->valid||!d||!a||!n||n>112||!a7_menu_maps_contains(&r->active,a,n))return 0;return r->io.pread(r->io.context,a,d,n)==(int)n;}
