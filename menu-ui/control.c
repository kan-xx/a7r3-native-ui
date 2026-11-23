#include "product.h"
extern void*dlopen(const char*,int),*dlsym(void*,const char*);extern int dlclose(void*);extern void*malloc(size_t);extern void free(void*);
struct packet{const uint8_t*input;uint32_t input_size,input_capacity;const uint8_t*output;uint32_t output_size;void(*output_free)(void*);};
static uint32_t word(const uint8_t*p){return p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static void put(uint8_t*p,uint32_t x){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(x>>(8*i));}
static int same(const uint8_t*a,const uint8_t*b,unsigned n){unsigned x=0;for(unsigned i=0;i<n;i++)x|=a[i]^b[i];return !x;}
__attribute__((visibility("default")))uint8_t commandBC10(struct packet*p){
 if(!p)return 0x81;p->output=0;p->output_size=0;p->output_free=0;if(!p->input||p->input_size<32||p->input_capacity<p->input_size||!same(p->input,(const uint8_t*)"A7M16S01",8))return 0x81;
 uint32_t op=word(p->input+8),offset=word(p->input+12),index=op==7&&p->input_size==36?word(p->input+32):0xffffffffu;uint8_t nz=0;for(unsigned i=0;i<16;i++)nz|=p->input[16+i];
 if((op!=2&&op!=5&&op!=6&&op!=7)||!nz||p->input_size!=(op==1?80u:op==7?36u:32u)||(op!=7&&offset)||(op==7&&(index>3||offset>=sizeof(struct a7_menu_snapshot))))return 0x81;
 uint32_t capacity=1280+(op==7?8192:0);uint8_t*out=malloc(capacity);if(!out)return 0x82;for(unsigned i=0;i<capacity;i++)out[i]=0;struct mp_report r={0};uint32_t adopted=0,copied=0,present=0;int status=21;
 void*h=dlopen(MP_CORE_PATH,6);if(h){const struct mp_api*(*get)(void)=dlsym(h,MP_SYMBOL);const struct mp_api*a=get?get():0;present=1;status=3;
  if(a&&a->version==1&&a->bytes==sizeof(*a)&&a->build==MP_BUILD&&a->query&&a->diagnostic&&a->snapshot){
   {status=a->query(&r);if(!status&&(r.version!=1||r.bytes!=sizeof r||!same(r.nonce,p->input+16,16)))status=39;
    if(!status){if(op==5||op==6)status=a->diagnostic(op==5?1:2,&r);else if(op==7)status=a->snapshot(index,offset,out+1280,8192,&copied);}}
  }
  if(h&&dlclose(h))status=22;
 }
 for(unsigned i=0;i<8;i++)out[i]=(uint8_t)"A7M16R01"[i];put(out+8,1);put(out+12,op);put(out+16,status);put(out+20,sizeof r);put(out+24,index);put(out+28,offset);put(out+32,copied);put(out+36,op==7?sizeof(struct a7_menu_snapshot):0);put(out+40,present|(adopted<<1));put(out+44,MP_BUILD);for(unsigned i=0;i<16;i++)out[48+i]=p->input[16+i];for(unsigned i=0;i<sizeof r;i++)out[64+i]=((uint8_t*)&r)[i];p->output=out;p->output_size=1280+copied;p->output_free=free;return 1;
}
