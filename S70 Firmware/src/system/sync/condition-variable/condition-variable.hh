// condition-variable.hh

/**************************************************************************//**
 * \file condition-variable.hh
 * \author Sean Benish
 * \brief FreeRTOS implementation of a condition variable.
 *
 * Condition variables will block until a certain implementation is met.
 * This implementation does not guarantee ordering.
 *****************************************************************************/
#pragma once

#include "FreeRTOS.h"
#include "Queue.h"
#include "task.h"

/*****************************************************************************
 * Defines
 *****************************************************************************/

/*****************************************************************************
 * Data Types
 *****************************************************************************/
#ifdef configSUPPORT_STATIC_ALLOCATION
// TODO: Static allocation CVs
#endif

class condition_variable
{
private:
    const QueueHandle_t _semaphore;

public:
    condition_variable();

    condition_variable(const condition_variable& r_other);
    condition_variable& operator=(const condition_variable& r_other) = delete;
    ~condition_variable();

    /**
     * Block the thread until the synchronized condition is met.
     * \tparam inputs_vt The inputs to the callback function.  Defaults to void.
     * \param[in]       fp_callback Function pointer to a boolean-return function.
     *      Execution of the callback can only occur when the underlying lock is obtained.
     *      When the callback returns true, the wait ends.
     */
    template<typename... inputs_vt = void> 
    void wait(bool (*const fp_callback) (inputs_vt... inputs_v));

    /**
     * Block the thread until the synchronized condition is met.
     * \tparam inputs_vt The inputs to the callback function.  Defaults to void.
     * \param[in]       fp_callback Function pointer to a boolean-return function.
     *      Execution of the callback can only occur when the underlying lock is obtained.
     *      When the callback returns true, the wait ends.
     * \param[in]       timeout The amount of time to wait for the condition.
     * \return true The condition was successfully met within the timeout.
     * \return false The condition was not met within the timeout.
     */
    template<typename... inputs_vt = void> 
    bool wait_for(bool (*const fp_callback) (inputs_vt... inputs_v), const TickType_t timeout);

    /**
     * Notifies all waiting threads to wake up.
     */
    void notify_all();

};

/*****************************************************************************
 * Constants
 *****************************************************************************/

/*****************************************************************************
 * Public Functions
 *****************************************************************************/

template<typename... inputs_vt> 
void condition_variable::wait(bool (*const fp_callback) (inputs_vt... inputs_v))
{
    xQueueReceive(_semaphore, portMAX_DELAY);
    while (! fp_callback(inputs_v...))
    {
        xQueueSend(_semaphore, nullptr, portMAX_DELAY);
        vTaskDelay(0);
        xQueueReceive(_semaphore, portMAX_DELAY);
    }
}

template<typename... inputs_vt> 
bool condition_variable::wait_for(bool (*const fp_callback) (inputs_vt... inputs_v), const TickType_t timeout)
{
    TickType_t current_time = xTaskGetTickCount();
    xQueueReceive(_semaphore, timeout);
    TickType_t time_elapsed = (xTaskGetTickCount() - timeout);
    if (time_elapsed >= timeout)
    {
        return false;
    }
    while (! fp_callback(inputs_v...))
    {
        xQueueSend(_semaphore, nullptr, portMAX_DELAY);
        vTaskDelay(0);
        current_time = xTaskGetTickCount();
        xQueueReceive(_semaphore, timeout - time_elapsed);
        time_elapsed += (xTaskGetTickCount() - timeout);
        if (time_elapsed >= timeout)
        {
            return false;
        }
    }

    return true;
}

//EOF
