#include "stock_open.h"
extern "C" int mp_stock_open_caught(void(*fn)(const char*,int),struct mp_stock_result*r){if(!fn||!r)return 0;r->attempted++;try{fn("view/COMMON_DEDICATED_SCREEN_BASE",19);r->returned++;return 1;}catch(...){r->exception++;return 0;}}
