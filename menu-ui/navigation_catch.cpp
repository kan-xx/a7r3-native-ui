#include "navigation.h"
extern "C" int a7_navigation_push_caught(const a7_navigation_api*a,void*e,a7_navigation_result*r){
 try {
  int rc=a->push(a->event_manager,e,0);r->push_return=(uint32_t)rc;
  return rc==0?A7_NAV_OK:A7_NAV_PUSH_FAILED;
 } catch (...) {
  /* Qualified original category-0 path has only one throw site: operator
   * new(12) before the list node is linked. The original frame unwinds with
   * no cleanup and still owns manager+16. Release that exact lock here.
   * This is NOT a generic wrapper for other events/callback categories. */
  r->caught_exception=1;r->exception_unlock=(uint32_t)a->unlock((uint8_t*)a->event_manager+16);
  return A7_NAV_EXCEPTION;
 }
}
