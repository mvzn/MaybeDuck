#include <xmmintrin.h>
#include <pmmintrin.h>

inline void enableFTZ_DAZ()
{
    _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
    _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
}
