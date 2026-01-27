/**
 * @file otm.h
 *
 * @brief Functions for ???
 *
 */

#ifndef SRC_SYSTEM_BOARDS_OTM_OTM_H_
#define SRC_SYSTEM_BOARDS_OTM_OTM_H_

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************
 * Defines
 ****************************************************************************/
#define OTM_FOCUS_LL					100
#define OTM_FOCUS_UL					5700
#define OTM_FOCUS_COLISION_THRESHHOLD	500



/****************************************************************************
 * Public Data
 ****************************************************************************/

/************************************************************************************
 * Public Function Prototypes
 ************************************************************************************/
void otm_init(void);

#ifdef __cplusplus
}
#endif

#endif /* SRC_SYSTEM_BOARDS_OTM_OTM_H_ */
