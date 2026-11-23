#include "map_reader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks;static void ok(int v){checks++;if(!v){fprintf(stderr,"FAIL %u\n",checks);exit(1);}}
struct port{unsigned opened,closed,reads,preads;int fail_open,fail_read,fail_close,unterminated;unsigned pos;};
static const char text[]="00001000-00002000 rw-p 00000000 00:00 0 [heap]\n00002000-00003000 rw-s 00000000 00:00 1 /dev/stream\n";
static int op(void*p,const char*path){struct port*s=p;ok(!strcmp(path,"/proc/self/maps"));s->opened++;s->pos=0;return s->fail_open?-1:44;}
static int rd(void*p,int fd,void*b,uint32_t n){struct port*s=p;ok(fd==44);s->reads++;if(s->fail_read)return-1;if(s->unterminated){memset(b,'x',n);return n;}unsigned left=sizeof(text)-1-s->pos;if(!left)return 0;if(left>13)left=13;if(left>n)left=n;memcpy(b,text+s->pos,left);s->pos+=left;return left;}
static int cl(void*p,int fd){struct port*s=p;ok(fd==44);s->closed++;return s->fail_close?-1:0;}
static int pr(void*p,uint32_t a,void*b,uint32_t n){struct port*s=p;(void)a;s->preads++;memset(b,0,n);return n;}
int main(void){struct mp_map_reader*r=calloc(1,sizeof(*r));struct port p={0};r->io=(struct mp_map_io){&p,op,rd,cl,pr};uint32_t out;
 for(unsigned i=0;i<3;i++){ok(mp_maps_refresh(r)&&r->valid&&r->generation==i+1&&r->active.count==1&&r->active.lines==2);ok(mp_maps_read(r,0x1000,&out,4));ok(!mp_maps_read(r,0x2000,&out,4));}ok(p.opened==3&&p.closed==3&&p.preads==3);
 p.fail_read=1;ok(!mp_maps_refresh(r)&&!r->valid&&r->error==2);ok(!mp_maps_read(r,0x1000,&out,4));p.fail_read=0;p.fail_close=1;ok(!mp_maps_refresh(r)&&!r->valid&&r->error==4);p.fail_close=0;p.unterminated=1;ok(!mp_maps_refresh(r)&&!r->valid&&r->error==5);p.unterminated=0;ok(mp_maps_refresh(r)&&r->valid&&r->generation==4);ok(p.opened==p.closed);free(r);printf("MAP_READER passed %u assertions fresh FD and complete EOF\n",checks);return 0;}
