#pragma once

namespace code_promise
{

/**
* A syntatic adapter that adds the semantic promise
* that the the underlying type is initialized at runtime.
* This works by allowing default construction for classes
* with protected default constructors.
*/
template<typename T>
class startup_initialized : public T
{
public:
    constexpr startup_initialized() = default;
};
}

