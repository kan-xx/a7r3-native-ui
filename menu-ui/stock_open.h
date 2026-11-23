#ifndef MP_STOCK_OPEN_H
#define MP_STOCK_OPEN_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
struct mp_stock_result {uint32_t attempted,returned,exception;};
/* Original API copies target/mode into its native Event before returning.
 * Returned means enqueue call returned; only later registry evidence proves
 * the original AudioSignals page opened. It is not pixel/setting acceptance. */
int mp_stock_open_caught(void(*)(const char*,int),struct mp_stock_result*);
#ifdef __cplusplus
}
#endif
#endif
