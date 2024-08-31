#pragma once

#ifndef DRIVER_HPP
#define DRIVER_HPP

#include <Windows.h>
#include <TlHelp32.h>
#include <cstdint>
#include <iostream>
#include <vector>
#include <utility>
#include <chrono>

extern uintptr_t virtualaddy;

#define code_rw CTL_CODE(FILE_DEVICE_UNKNOWN, 0x1645, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define code_ba CTL_CODE(FILE_DEVICE_UNKNOWN, 0x1646, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define code_get_guarded_region CTL_CODE(FILE_DEVICE_UNKNOWN, 0x1647, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define code_rw_batch CTL_CODE(FILE_DEVICE_UNKNOWN, 0x1648, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define code_security 0x85b3b69

typedef struct _rw {
    INT32 security;
    INT32 process_id;
    ULONGLONG address;
    ULONGLONG buffer;
    ULONGLONG size;
    BOOLEAN write;
} rw, * prw;

typedef struct _ba {
    INT32 security;
    INT32 process_id;
    ULONGLONG* address;
} ba, * pba;

typedef struct _ga {
    INT32 security;
    ULONGLONG* address;
} ga, * pga;

typedef struct _rw_batch {
    INT32 security;
    INT32 process_id;
    struct {
        ULONGLONG address;
        ULONGLONG buffer;
        ULONGLONG size;
    } requests[64];
    ULONGLONG request_count;
} rw_batch, * prw_batch;

namespace memory {
    extern HANDLE driver_handle;
    extern INT32 process_id;

    inline bool find_driver() {
        driver_handle = CreateFileW(L"\\\\.\\nevermissbetter", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

        if (driver_handle == INVALID_HANDLE_VALUE) {
            return false;
        }

        return true;
    }

    inline bool is_valid(std::uint64_t address) {
        if (address <= 0x400000 || address == 0xCCCCCCCCCCCCCCCC || reinterpret_cast<void*>(address) == nullptr || address > 0x7FFFFFFFFFFFFFFF)
            return false;

        return true;
    }

    inline void read_physical(PVOID address, PVOID buffer, DWORD size) {
        _rw arguments = { 0 };

        arguments.security = code_security;
        arguments.address = (ULONGLONG)address;
        arguments.buffer = (ULONGLONG)buffer;
        arguments.size = size;
        arguments.process_id = process_id;
        arguments.write = FALSE;

        DeviceIoControl(driver_handle, code_rw, &arguments, sizeof(arguments), nullptr, 0, nullptr, nullptr);

    }

    inline void write_physical(PVOID address, PVOID buffer, DWORD size) {
        _rw arguments = { 0 };

        arguments.security = code_security;
        arguments.address = (ULONGLONG)address;
        arguments.buffer = (ULONGLONG)buffer;
        arguments.size = size;
        arguments.process_id = process_id;
        arguments.write = TRUE;

        DeviceIoControl(driver_handle, code_rw, &arguments, sizeof(arguments), nullptr, 0, nullptr, nullptr);
    }

    inline void read_physical_batch(const std::vector<std::pair<PVOID, PVOID>>& addresses_and_buffers, DWORD size) {
        rw_batch arguments = { 0 };

        arguments.security = code_security;
        arguments.process_id = process_id;
        arguments.request_count = addresses_and_buffers.size();

        for (size_t i = 0; i < addresses_and_buffers.size(); ++i) {
            arguments.requests[i].address = (ULONGLONG)addresses_and_buffers[i].first;
            arguments.requests[i].buffer = (ULONGLONG)addresses_and_buffers[i].second;
            arguments.requests[i].size = size;
        }


        DeviceIoControl(driver_handle, code_rw_batch, &arguments, sizeof(arguments), nullptr, 0, nullptr, nullptr);

    }

    inline uintptr_t find_image() {
        uintptr_t image_address = { NULL };
        _ba arguments = { NULL };

        arguments.security = code_security;
        arguments.process_id = process_id;
        arguments.address = (ULONGLONG*)&image_address;

        DeviceIoControl(driver_handle, code_ba, &arguments, sizeof(arguments), nullptr, NULL, NULL, NULL);

        return image_address;
    }

    inline uintptr_t get_guarded_region() {
        uintptr_t guarded_region_address = { NULL };
        _ga arguments = { NULL };

        arguments.security = code_security;
        arguments.address = (ULONGLONG*)&guarded_region_address;

        DeviceIoControl(driver_handle, code_get_guarded_region, &arguments, sizeof(arguments), nullptr, NULL, NULL, NULL);

        return guarded_region_address;
    }

    inline INT32 find_process(LPCTSTR process_name) {
        PROCESSENTRY32 pt;
        HANDLE hsnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        pt.dwSize = sizeof(PROCESSENTRY32);
        if (Process32First(hsnap, &pt)) {
            do {
                if (!lstrcmpi(pt.szExeFile, process_name)) {
                    CloseHandle(hsnap);
                    process_id = pt.th32ProcessID;
                    return pt.th32ProcessID;
                }
            } while (Process32Next(hsnap, &pt));
        }
        CloseHandle(hsnap);

        return { NULL };
    }
}

template <typename T>
inline T read(uint64_t address) {
    if (!memory::is_valid(address))
        return T();

    T buffer{ };

    memory::read_physical((PVOID)address, &buffer, sizeof(T));
    return buffer;
}

template <typename T>
inline void write(uint64_t address, const T& buffer) {
    if (!memory::is_valid(address))
        return;

    memory::write_physical((PVOID)address, (PVOID)&buffer, sizeof(T));
}

template<typename T>
inline bool read_array(uintptr_t address, T out[], size_t len) {
    if (!memory::is_valid(address))
        return false;

    std::vector<std::pair<PVOID, PVOID>> address_and_buffer_pairs;
    for (size_t i = 0; i < len; ++i) {
        address_and_buffer_pairs.emplace_back((PVOID)(address + i * sizeof(T)), (PVOID)&out[i]);
    }

    memory::read_physical_batch(address_and_buffer_pairs, sizeof(T));
    return true;
}

#endif