#include "native_port.h"
extern "C" int mp_registry_add_caught(const struct mp_registry_api*a,void*r){try{a->add(r,0xffc,MP_CORE_PATH,MP_FACTORY_NAME);return 1;}catch(...){return 0;}}
extern "C" int mp_registry_remove_caught(const struct mp_registry_api*a,void*r){try{a->remove(r,0xffc);return 1;}catch(...){return 0;}}
