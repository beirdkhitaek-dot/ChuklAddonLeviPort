#ifdef CHUKL_WITH_SDK
#include "mod/ChuklLeviMod.h"
#include <pl/Mod.hpp>
// SDK-CHECK: how PL_REGISTER_MOD takes the type and instance expression.
PL_REGISTER_MOD(chukl::ChuklLeviMod, chukl::ChuklLeviMod::get());
#endif
