#ifndef APT_DEPRECATION_H
#define APT_DEPRECATION_H

#ifdef __cplusplus
#if __cplusplus >= 201103L
#define APT_DEPRECATION_ERROR(msg) static_assert(false, msg)
#endif
#endif

#ifndef APT_DEPRECATION_ERROR
#ifdef __STDC__
#if __STDC_VERSION__ >= 201112L
#define APT_DEPRECATION_ERROR(msg) _Static_assert(0, msg)
#endif
#endif
#endif

#ifndef APT_DEPRECATION_ERROR
#define APT_DEPRECATION_ERROR(msg) ((msg), )
#endif

#endif