#pragma once

#include <type_traits>

#include "core/hle/kernel/session.h"

namespace IPC {
struct HandleParam {
    bool copy;
    std::vector<Handle> handles;
};

struct CallingPidParam {
    int place_holder;
};

struct StaticBufferParam {
    unsigned int buffer_id;
    std::vector<u8> data;
};

struct MappingBufferParam {
    MappedBufferPermissions permissions;
    u32 size;
    VAddr address;
};

////////////////////////////////////////////////////////////////////////////////
// unsigned /*word_length*/ ReadRegularParam<T>(u32* cmd_buff, T& dest)
// return 0 for translate param

template <typename T>
unsigned ReadRegularParam(u32* cmd_buff, T& dest) {
    static_assert(std::is_pod<T>::value, "Reqular param must be POD!");
    unsigned word_length = (sizeof(T) - 1) / 4 + 1;
    std::memcpy(&dest, cmd_buff, word_length * 4);
    return word_length;
}

template <>
unsigned ReadRegularParam(u32* cmd_buff, HandleParam& dest) {
    return 0;
}

template <>
unsigned ReadRegularParam(u32* cmd_buff, CallingPidParam& dest) {
    return 0;
}

template <>
unsigned ReadRegularParam(u32* cmd_buff, StaticBufferParam& dest) {
    return 0;
}

template <>
unsigned ReadRegularParam(u32* cmd_buff, MappingBufferParam& dest) {
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// unsigned /*word_length*/ ReadTranslateParam(u32* cmd_buff, T& dest)
// return 0 for reqular param

template <typename T>
unsigned ReadTranslateParam(u32* cmd_buff, T& dest) {
    return 0;
}

template <>
unsigned ReadTranslateParam(u32* cmd_buff, HandleParam& dest) {
    u32 descriptor = *(cmd_buff++);
    ASSERT_MSG((descriptor & 0x2F) == 0, "Wrong descriptor for handle param!");
    dest.copy = ((descriptor & 0x10) != 0);
    unsigned handle_count = (descriptor >> 26) + 1;
    dest.handles.assign((Handle*)cmd_buff, (Handle*)cmd_buff + handle_count);
    return handle_count + 1;
}

template <>
unsigned ReadTranslateParam(u32* cmd_buff, CallingPidParam& dest) {
    ASSERT_MSG(cmd_buff[0] == 0x20, "Wrong descriptor for calling PID param!");
    return 2;
}

template <>
unsigned ReadTranslateParam(u32* cmd_buff, StaticBufferParam& dest) {
    u32 descriptor = *(cmd_buff++);
    ASSERT_MSG((descriptor & 0xF) == 2, "Wrong descriptor for static buffer param!");
    dest.buffer_id = (descriptor >> 10) & 0xF;
    u32 size = descriptor >> 14;
    u8* ptr = Memory::GetPointer((VAddr)*cmd_buff); // no GetPointer!
    dest.data.assign(ptr, ptr + size);
    return 2;
}

template <>
unsigned ReadTranslateParam(u32* cmd_buff, MappingBufferParam& dest) {
    u32 descriptor = *(cmd_buff++);
    ASSERT_MSG((descriptor & 0x8) == 0x8, "Wrong descriptor for mapping buffer param!");
    dest.permissions = (MappedBufferPermissions)(descriptor & 0x7);
    dest.size = descriptor >> 4;
    dest.address = (VAddr)*cmd_buff;
    return 2;
}

////////////////////////////////////////////////////////////////////////////////
// Wrap

template<typename FuncType>
void WrapHelper(FuncType& f, u32 *cmd_buff, unsigned regular_length, unsigned translate_length) {
    ASSERT_MSG(regular_length == 0 && translate_length == 0, "Didn't read all params!"); // DEBUG_ASSERT
    f();
}

template<typename FuncType, typename T0, typename...Ts>
void WrapHelper(FuncType&f, u32 *cmd_buff, unsigned regular_length, unsigned translate_length) {
    typename std::remove_const<typename std::remove_reference<T0>::type>::type param;
    unsigned read_length = ReadRegularParam(cmd_buff, param);
    if (read_length == 0) {
        ASSERT_MSG(regular_length == 0, "Didn't read all regular params!"); // DEBUG_ASSERT
        read_length = ReadTranslateParam(cmd_buff, param);
        translate_length -= read_length;
        ASSERT_MSG(translate_length >= 0, "Read too much translate params!"); // DEBUG_ASSERT
    } else {
        regular_length -= read_length;
        ASSERT_MSG(regular_length >= 0, "Read too much regular params!"); // DEBUG_ASSERT
    }
    auto g = [&f, &param](Ts...params) {
        f(param, std::forward<Ts>(params)...);
    };
    WrapHelper<decltype(g), Ts...>(g, cmd_buff + read_length, regular_length, translate_length);
}

template<typename FuncType, FuncType& func, typename...Ts>
void Wrap(Service::Interface* self) {
    u32* cmd_buff = Kernel::GetCommandBuffer();
    u32 regular_length = ((*cmd_buff) >> 6) & 0x3F;
    u32 translate_length = (*cmd_buff) & 0x3F;
    WrapHelper<FuncType, Ts...>(func, cmd_buff + 1, regular_length, translate_length);
}
////////////////////////////////////////////////////////////////////////////////
// unsigned /*word_length*/ WriteRegularParam<T>(u32* cmd_buff, const T& src)
// return 0 for translate param

template <typename T>
unsigned WriteRegularParam(u32* cmd_buff, const T& src) {
    static_assert(std::is_pod<typename std::remove_reference<T>::type>::value, "Regular param must be POD!");
    unsigned word_length = (sizeof(T) - 1) / 4 + 1;
    std::memcpy(cmd_buff, &src, word_length * 4);
    return word_length;
}

template <>
unsigned WriteRegularParam(u32* cmd_buff, const HandleParam& src) {
    return 0;
}

template <>
unsigned WriteRegularParam(u32* cmd_buff, const CallingPidParam& src) {
    return 0;
}

template <>
unsigned WriteRegularParam(u32* cmd_buff, const StaticBufferParam& src) {
    return 0;
}

template <>
unsigned WriteRegularParam(u32* cmd_buff, const MappingBufferParam& src) {
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// unsigned /*word_length*/ WriteTranslateParam(u32* cmd_buff, const T& src)
// return 0 for reqular param

template <typename T>
unsigned WriteTranslateParam(u32* cmd_buff, const T& src) {
    return 0;
}

template <>
unsigned WriteTranslateParam(u32* cmd_buff, const HandleParam& dest) {
    if (dest.copy)
        *cmd_buff = CopyHandleDesc(dest.handles.size());
    else
        *cmd_buff = MoveHandleDesc(dest.handles.size());
    std::copy(dest.handles.begin(), dest.handles.end(), cmd_buff + 1);
    return dest.handles.size() + 1;
}

template <>
unsigned WriteTranslateParam(u32* cmd_buff, const CallingPidParam& dest) {
    UNIMPLEMENTED();
    return 2;
}

template <>
unsigned WriteTranslateParam(u32* cmd_buff, const StaticBufferParam& dest) {
    UNIMPLEMENTED();
    return 2;
}

template <>
unsigned WriteTranslateParam(u32* cmd_buff, const MappingBufferParam& dest) {
    UNIMPLEMENTED();
    return 2;
}

////////////////////////////////////////////////////////////////////////////////
// Return

void ReturnHelper(u32 *cmd_buff, unsigned int& regular_length, unsigned int& translate_length) {
    return;
}

template<typename T0, typename...Ts>
void ReturnHelper(u32 *cmd_buff, unsigned int& regular_length, unsigned int& translate_length, T0&& param0, Ts&&...params) {
    unsigned write_length = WriteRegularParam(cmd_buff, param0);
    if (write_length == 0) {
        write_length = WriteTranslateParam(cmd_buff, param0);
        translate_length += write_length;
    } else {
        ASSERT_MSG(translate_length == 0, "Write regular param after translate param!"); // DEBUG_ASSERT
        regular_length += write_length;
    }
    ReturnHelper(cmd_buff + write_length, regular_length, translate_length, std::forward<Ts>(params)...);
}

template<typename...Ts>
void Return(Ts&&...params) {
    u32* cmd_buff = Kernel::GetCommandBuffer();
    u16 command_id = (*cmd_buff) >> 16;
    unsigned int regular_length = 0, translate_length = 0;
    ReturnHelper(cmd_buff + 1, regular_length, translate_length, std::forward<Ts>(params)...);
    *cmd_buff = MakeHeader(command_id, regular_length, translate_length);
}

}
