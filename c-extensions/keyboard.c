#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <windows.h>
#include <stdint.h>
#include <math.h>

static int l_wasPressed(lua_State *L)
{
    lua_Integer i = luaL_checkinteger(L, 1);
    luaL_argcheck(L, 0 < i && i < 0xFF, 1, "must satisfy 0 < i < 0xFF [127]");
    lua_pushboolean(L, (GetAsyncKeyState(i) & 0x01) != 0);
    return 1;
}

static DWORD timer_flags = (DWORD)0;

static int l_wait(lua_State *L)
{
    lua_Number d = luaL_checknumber(L, 1);
    luaL_argcheck(L, d >= 0, 1, "must satisfy d >= 0");
    /* sec -> nsec -> 100nsec */
    d *= 1000 * 1000 * 1000 / 100;
    int64_t period_100ns = ceil(d);
    if (period_100ns == 0) {
        Sleep(0);
        return 0;
    }
    LARGE_INTEGER relative_timeout;
    relative_timeout.QuadPart = -period_100ns;
    HANDLE timer = CreateWaitableTimerExW(NULL, NULL, timer_flags,
                                          TIMER_ALL_ACCESS);
    if (timer == NULL) {
        return luaL_error(L, "can't create windows timer");
    }

    if (!SetWaitableTimerEx(timer, &relative_timeout, 0, NULL, NULL, NULL, 0))
    {
        CloseHandle(timer);
        return luaL_error(L, "can't set windows timer");
    }

    DWORD rc = WaitForSingleObject(timer, INFINITE);
    CloseHandle(timer);

    if (rc == WAIT_FAILED) {
        return luaL_error(L, "windows timer failed");
    }

    return 0;
}

static int l_setup(lua_State *L)
{
    HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode;
    if (!GetConsoleMode(handle, &mode)) {
        return luaL_error(L, "console mode get failed");
    }
    mode |= ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    if (!SetConsoleMode(handle, mode)) {
        return luaL_error(L, "console mode set failed");
    }
    return 0;
}

static const luaL_Reg keyboard[] = {
    {"wasPressed", l_wasPressed},
    {"wait", l_wait},
    {"setup", l_setup},
    {NULL, NULL}
};

#define SET_IFIELD(NAME, NUM) \
    lua_pushinteger(L, (NUM)); \
    lua_setfield(L, -2, (NAME)); \

__declspec(dllexport) int luaopen_keyboard(lua_State *L)
{
    luaL_newlib(L, keyboard);

    SET_IFIELD("VK_BACK"      , 0x08)
    SET_IFIELD("VK_TAB"       , 0x09)
    SET_IFIELD("VK_CLEAR"     , 0x0C)
    SET_IFIELD("VK_RETURN"    , 0x0D)
    SET_IFIELD("VK_SHIFT"     , 0x10)
    SET_IFIELD("VK_CONTROL"   , 0x11)
    SET_IFIELD("VK_MENU"      , 0x12)
    SET_IFIELD("VK_PAUSE"     , 0x13)
    SET_IFIELD("VK_CAPITAL"   , 0x14)
    SET_IFIELD("VK_KANA"      , 0x15)
    SET_IFIELD("VK_HANGUL"    , 0x15)
    SET_IFIELD("VK_IME_ON"    , 0x16)
    SET_IFIELD("VK_JUNJA"     , 0x17)
    SET_IFIELD("VK_FINAL"     , 0x18)
    SET_IFIELD("VK_HANJA"     , 0x19)
    SET_IFIELD("VK_IME_OFF"   , 0x1A)
    SET_IFIELD("VK_ESCAPE"    , 0x1B)
    SET_IFIELD("VK_CONVERT"   , 0x1C)
    SET_IFIELD("VK_NONCONVERT", 0x1D)
    SET_IFIELD("VK_ACCEPT"    , 0x1E)
    SET_IFIELD("VK_MODECHANGE", 0x1F)
    SET_IFIELD("VK_SPACE"     , 0x20)
    SET_IFIELD("VK_PRIOR"     , 0x21)
    SET_IFIELD("VK_NEXT"      , 0x22)
    SET_IFIELD("VK_END"       , 0x23)
    SET_IFIELD("VK_HOME"      , 0x24)
    SET_IFIELD("VK_LEFT"      , 0x25)
    SET_IFIELD("VK_UP"        , 0x26)
    SET_IFIELD("VK_RIGHT"     , 0x27)
    SET_IFIELD("VK_DOWN"      , 0x28)
    SET_IFIELD("VK_SELECT"    , 0x29)
    SET_IFIELD("VK_PRINT"     , 0x2A)
    SET_IFIELD("VK_EXECUTE"   , 0x2B)
    SET_IFIELD("VK_SNAPSHOT"  , 0x2C)
    SET_IFIELD("VK_INSERT"    , 0x2D)
    SET_IFIELD("VK_DELETE"    , 0x2E)
    SET_IFIELD("VK_HELP"      , 0x2F)
    SET_IFIELD("VK_0", 0x30)
    SET_IFIELD("VK_1", 0x31)
    SET_IFIELD("VK_2", 0x32)
    SET_IFIELD("VK_3", 0x33)
    SET_IFIELD("VK_4", 0x34)
    SET_IFIELD("VK_5", 0x35)
    SET_IFIELD("VK_6", 0x36)
    SET_IFIELD("VK_7", 0x37)
    SET_IFIELD("VK_8", 0x38)
    SET_IFIELD("VK_9", 0x39)
    SET_IFIELD("VK_A", 0x41)
    SET_IFIELD("VK_B", 0x42)
    SET_IFIELD("VK_C", 0x43)
    SET_IFIELD("VK_D", 0x44)
    SET_IFIELD("VK_E", 0x45)
    SET_IFIELD("VK_F", 0x46)
    SET_IFIELD("VK_G", 0x47)
    SET_IFIELD("VK_H", 0x48)
    SET_IFIELD("VK_I", 0x49)
    SET_IFIELD("VK_J", 0x4A)
    SET_IFIELD("VK_K", 0x4B)
    SET_IFIELD("VK_L", 0x4C)
    SET_IFIELD("VK_M", 0x4D)
    SET_IFIELD("VK_N", 0x4E)
    SET_IFIELD("VK_O", 0x4F)
    SET_IFIELD("VK_P", 0x50)
    SET_IFIELD("VK_Q", 0x51)
    SET_IFIELD("VK_R", 0x52)
    SET_IFIELD("VK_S", 0x53)
    SET_IFIELD("VK_T", 0x54)
    SET_IFIELD("VK_U", 0x55)
    SET_IFIELD("VK_V", 0x56)
    SET_IFIELD("VK_W", 0x57)
    SET_IFIELD("VK_X", 0x58)
    SET_IFIELD("VK_Y", 0x59)
    SET_IFIELD("VK_Z", 0x5A)
    SET_IFIELD("VK_LWIN"      , 0x5B)
    SET_IFIELD("VK_RWIN"      , 0x5C)
    SET_IFIELD("VK_APPS"      , 0x5D)
    SET_IFIELD("VK_SLEEP"     , 0x5F)
    SET_IFIELD("VK_NUMPAD0"   , 0x60)
    SET_IFIELD("VK_NUMPAD1"   , 0x61)
    SET_IFIELD("VK_NUMPAD2"   , 0x62)
    SET_IFIELD("VK_NUMPAD3"   , 0x63)
    SET_IFIELD("VK_NUMPAD4"   , 0x64)
    SET_IFIELD("VK_NUMPAD5"   , 0x65)
    SET_IFIELD("VK_NUMPAD6"   , 0x66)
    SET_IFIELD("VK_NUMPAD7"   , 0x67)
    SET_IFIELD("VK_NUMPAD8"   , 0x68)
    SET_IFIELD("VK_NUMPAD9"   , 0x69)
    SET_IFIELD("VK_MULTIPLY"  , 0x6A)
    SET_IFIELD("VK_ADD"       , 0x6B)
    SET_IFIELD("VK_SEPARATOR" , 0x6C)
    SET_IFIELD("VK_SUBTRACT"  , 0x6D)
    SET_IFIELD("VK_DECIMAL"   , 0x6E)
    SET_IFIELD("VK_DIVIDE"    , 0x6F)
    SET_IFIELD("VK_F1" , 0x70)
    SET_IFIELD("VK_F2" , 0x71)
    SET_IFIELD("VK_F3" , 0x72)
    SET_IFIELD("VK_F4" , 0x73)
    SET_IFIELD("VK_F5" , 0x74)
    SET_IFIELD("VK_F6" , 0x75)
    SET_IFIELD("VK_F7" , 0x76)
    SET_IFIELD("VK_F8" , 0x77)
    SET_IFIELD("VK_F9" , 0x78)
    SET_IFIELD("VK_F10", 0x79)
    SET_IFIELD("VK_F11", 0x7A)
    SET_IFIELD("VK_F12", 0x7B)
    SET_IFIELD("VK_F13", 0x7C)
    SET_IFIELD("VK_F14", 0x7D)
    SET_IFIELD("VK_F15", 0x7E)
    SET_IFIELD("VK_F16", 0x7F)
    SET_IFIELD("VK_F17", 0x80)
    SET_IFIELD("VK_F18", 0x81)
    SET_IFIELD("VK_F19", 0x82)
    SET_IFIELD("VK_F20", 0x83)
    SET_IFIELD("VK_F21", 0x84)
    SET_IFIELD("VK_F22", 0x85)
    SET_IFIELD("VK_F23", 0x86)
    SET_IFIELD("VK_F24", 0x87)
    SET_IFIELD("VK_NUMLOCK"   , 0x90)
    SET_IFIELD("VK_SCROLL"    , 0x91)
    SET_IFIELD("VK_LSHIFT"    , 0xA0)
    SET_IFIELD("VK_RSHIFT"    , 0xA1)
    SET_IFIELD("VK_LCONTROL"  , 0xA2)
    SET_IFIELD("VK_RCONTROL"  , 0xA3)
    SET_IFIELD("VK_LMENU"     , 0xA4)
    SET_IFIELD("VK_RMENU"     , 0xA5)
    SET_IFIELD("VK_BROWSER_BACK"     , 0xA6)
    SET_IFIELD("VK_BROWSER_FORWARD"  , 0xA7)
    SET_IFIELD("VK_BROWSER_REFRESH"  , 0xA8)
    SET_IFIELD("VK_BROWSER_STOP"     , 0xA9)
    SET_IFIELD("VK_BROWSER_SEARCH"   , 0xAA)
    SET_IFIELD("VK_BROWSER_FAVORITES", 0xAB)
    SET_IFIELD("VK_BROWSER_HOME"     , 0xAC)
    SET_IFIELD("VK_VOLUME_MUTE", 0xAD)
    SET_IFIELD("VK_VOLUME_DOWN", 0xAE)
    SET_IFIELD("VK_VOLUME_UP"  , 0xAF)
    SET_IFIELD("VK_MEDIA_NEXT_TRACK", 0xB0)
    SET_IFIELD("VK_MEDIA_PREV_TRACK", 0xB1)
    SET_IFIELD("VK_MEDIA_STOP"      , 0xB2)
    SET_IFIELD("VK_MEDIA_PLAY_PAUSE", 0xB3)
    SET_IFIELD("VK_LAUNCH_MAIL"        , 0xB4)
    SET_IFIELD("VK_LAUNCH_MEDIA_SELECT", 0xB5)
    SET_IFIELD("VK_LAUNCH_APP1"        , 0xB6)
    SET_IFIELD("VK_LAUNCH_APP2"        , 0xB7)
    SET_IFIELD("VK_OEM_1"     , 0xBA)
    SET_IFIELD("VK_OEM_PLUS"  , 0xBB)
    SET_IFIELD("VK_OEM_COMMA" , 0xBC)
    SET_IFIELD("VK_OEM_MINUS" , 0xBD)
    SET_IFIELD("VK_OEM_PERIOD", 0xBE)
    SET_IFIELD("VK_OEM_2"     , 0xBF)
    SET_IFIELD("VK_OEM_3"     , 0xC0)
    SET_IFIELD("VK_OEM_4"     , 0xDB)
    SET_IFIELD("VK_OEM_5"     , 0xDC)
    SET_IFIELD("VK_OEM_6"     , 0xDD)
    SET_IFIELD("VK_OEM_7"     , 0xDE)
    SET_IFIELD("VK_OEM_8"     , 0xDF)
    SET_IFIELD("VK_OEM_102"   , 0xE2)
    SET_IFIELD("VK_PROCESSKEY", 0xE5)
    SET_IFIELD("VK_PACKET"    , 0xE7)
    SET_IFIELD("VK_ATTN"      , 0xF6)
    SET_IFIELD("VK_CRSEL"     , 0xF7)
    SET_IFIELD("VK_EXSEL"     , 0xF8)
    SET_IFIELD("VK_EREOF"     , 0xF9)
    SET_IFIELD("VK_PLAY"      , 0xFA)
    SET_IFIELD("VK_ZOOM"      , 0xFB)
    SET_IFIELD("VK_PA1"       , 0xFD)
    SET_IFIELD("VK_OEM_CLEAR" , 0xFE)

    return 1;
}
