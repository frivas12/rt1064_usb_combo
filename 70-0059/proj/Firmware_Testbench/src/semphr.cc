#include "semphr.h"

#include <stdexcept>
#include <string>

bool xSemaphoreTake (SemaphoreHandle_t a, TickType_t b)
{
    if (b == 0 && *a == 0)
    {
        return false;
    }

    --(*a);

    if (*a < 0)
    {
        throw std::underflow_error(std::string("Bad value of mutex: ") + std::to_string(*a) + ".");
    }

    return true;
}

void xSemaphoreGive (SemaphoreHandle_t a)
{
    ++(*a);

    if (*a > 1)
    {
        throw std::overflow_error(std::string("Bad value of mutex: ") + std::to_string(*a) + ".");
    }
}


SemaphoreHandle_t xSemaphoreCreateMutex ()
{
    SemaphoreHandle_t ptr = new int;
    *ptr = 1;

    return ptr;
}
void vSemaphoreDelete (SemaphoreHandle_t a)
{
    if (a == 0)
    {
        throw std::logic_error("Mutex was locked on release.");
    }

    delete a;
}