#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define dlopen lm_open
#define dlsym lm_sym
#define dlclose lm_close
#include "control.c"
static unsigned checks,opens,closes,queries,diagnostics,snapshots,absent;static uint8_t nonce[16]={1};static const unsigned char dummy[8]={0};
#define C(x) do{checks++;if(!(x)){fprintf(stderr,"FAIL %u %s\n",__LINE__,#x);exit(1);}}while(0)
static int query(struct mp_report*r){queries++;memset(r,0,sizeof*r);r->version=1;r->bytes=sizeof*r;r->pid=151;memcpy(r->nonce,nonce,16);return 0;}
static int diagnostic(uint32_t op,struct mp_report*r){C(op==1||op==2);diagnostics++;return query(r);}
static int snapshot(uint32_t index,uint32_t offset,uint8_t*out,uint32_t cap,uint32_t*n){C(index<4&&offset<sizeof(struct a7_menu_snapshot)&&cap==8192);snapshots++;*n=sizeof(struct a7_menu_snapshot)-offset;if(*n>cap)*n=cap;memset(out,0,*n);return 0;}
static const struct mp_api api={.version=1,.bytes=sizeof(api),.build=MP_BUILD,.query=query,.diagnostic=diagnostic,.snapshot=snapshot};
static const struct mp_api*get(void){return &api;}
void*lm_open(const char*p,int f){C(!strcmp(p,MP_CORE_PATH)&&f==6);opens++;return absent?0:(void*)dummy;}
void*lm_sym(void*h,const char*n){C(h==dummy&&!strcmp(n,MP_SYMBOL));return (void*)get;}
int lm_close(void*h){C(h==dummy);closes++;return 0;}
static uint8_t in[80];static struct packet p;
static void request(unsigned op,unsigned len){memset(in,0,sizeof in);memcpy(in,"A7M16S01",8);put(in+8,op);memcpy(in+16,nonce,16);p=(struct packet){in,len,sizeof in,0,0,0};}
int main(void){
 for(unsigned op=0;op<10;op++)if(op!=2&&op!=5&&op!=6&&op!=7){request(op,op==1?80:32);unsigned before=opens;C(commandBC10(&p)==0x81&&opens==before&&!p.output);}
 for(unsigned op=2;op<=7;op++)if(op!=3&&op!=4){request(op,op==7?36:32);C(commandBC10(&p)==1&&word(p.output+16)==0&&!(word(p.output+40)&2));p.output_free((void*)p.output);}
 C(diagnostics==2&&snapshots==1&&opens==closes);
 request(5,32);in[16]=2;unsigned before=diagnostics;C(commandBC10(&p)==1&&word(p.output+16)==39&&diagnostics==before);p.output_free((void*)p.output);
 request(7,36);put(in+12,sizeof(struct a7_menu_snapshot));before=opens;C(commandBC10(&p)==0x81&&opens==before);
 request(7,36);put(in+32,4);C(commandBC10(&p)==0x81&&opens==before);
 request(5,32);memset(in+16,0,16);C(commandBC10(&p)==0x81&&opens==before);
 request(5,32);absent=1;C(commandBC10(&p)==1&&word(p.output+16)==21&&diagnostics==2);p.output_free((void*)p.output);
 printf("LIMITED_MENU_CONTROL checks=%u forbidden_start_remove_drain=1 noload_only=1 nonce_bound=1\n",checks);return 0;
}
