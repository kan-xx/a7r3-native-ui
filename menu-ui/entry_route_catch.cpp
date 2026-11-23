#include <stdint.h>
extern "C" int a7_route_select_caught(int(*f)(void*),void*p,uint32_t*exception){try{return f(p);}catch(...){*exception=1;return 0;}}
extern "C" int a7_route_action_caught(void(*f)(void*,uint32_t),void*p,uint32_t*exception){try{f(p,10);return 1;}catch(...){*exception=1;return 0;}}
