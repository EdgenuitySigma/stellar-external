#include "driver.hpp"

uintptr_t virtualaddy = 0;
HANDLE memory::driver_handle = nullptr;
INT32 memory::process_id = 0;