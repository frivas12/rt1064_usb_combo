/**
 * @file helper.h
 *
 * @brief Functions for ???
 *
 */

#ifndef SRC_SYSTEM_HELPER_HELPER_H_
#define SRC_SYSTEM_HELPER_HELPER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "Debugging.h"

/****************************************************************************
 * Defines
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

/************************************************************************************
 * Public Function Prototypes
 ************************************************************************************/
void reverse(char *str, int len);
int intToStr(int x, char str[], int d);
void ftoa(float n, char *res, int afterpoint);
double atof_m(char* num);
void rev_memcpy(void *dest, void *src, size_t n);

/**
 * Turns any ASCII English characters into a lower case.
 * The original array is transformed.
 * \param c_str A null-terminated string.
 */
void cstring_to_lowercase(char * c_str);

/**
 * Turns any ASCII English characters into a upper case.
 * The original array is transformed.
 * \param c_str A null-terminated string.
 */
void cstring_to_uppercase(char * c_str);


#ifdef __cplusplus
}
#endif

#endif /* SRC_SYSTEM_HELPER_HELPER_H_ */
