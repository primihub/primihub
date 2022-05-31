// Copyright [2021] <primihub.com>
#include "src/primihub/util/log.h"

#ifdef _MSC_VER
#include <windows.h>
#endif

namespace primihub {
    std::chrono::time_point<std::chrono::system_clock> gStart = std::chrono::system_clock::now();


	ostreamLocker lout(std::cout);
    std::mutex gIoStreamMtx;

    void setThreadName(const std::string name)
    {
        setThreadName(name.c_str());
    }
    void setThreadName(const char* name)
    {
#ifndef NDEBUG
#ifdef _MSC_VER
    const DWORD MS_VC_EXCEPTION = 0x406D1388;

#pragma pack(push,8)
        typedef struct tagTHREADNAME_INFO
        {
            DWORD dwType; // Must be 0x1000.
            LPCSTR szName; // Pointer to name (in user addr space).
            DWORD dwThreadID; // Thread ID (-1=caller thread).
            DWORD dwFlags; // Reserved for future use, must be zero.
        } THREADNAME_INFO;
#pragma pack(pop)


        THREADNAME_INFO info;
        info.dwType = 0x1000;
        info.szName = name;
        info.dwThreadID = -1;
        info.dwFlags = 0;

        __try
        {
            RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        }
#endif
#endif
    }

    const Color ColorDefault([]() -> Color {
#ifdef _MSC_VER
        CONSOLE_SCREEN_BUFFER_INFO   csbi;
        HANDLE m_hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        GetConsoleScreenBufferInfo(m_hConsole, &csbi);

        return (Color)(csbi.wAttributes & 255);
#else
        return Color::White;
#endif

    }());

#ifdef _MSC_VER
    static const HANDLE __m_hConsole(GetStdHandle(STD_OUTPUT_HANDLE));
#endif
#define RESET   "\033[0m"
#define BLACK   "\033[30m"      /* Black */
#define RED     "\033[31m"      /* Red */
#define GREEN   "\033[32m"      /* Green */
#define YELLOW  "\033[33m"      /* Yellow */
#define BLUE    "\033[34m"      /* Blue */
#define MAGENTA "\033[35m"      /* Magenta */
#define CYAN    "\033[36m"      /* Cyan */
#define WHITE   "\033[37m"      /* White */
#define BOLDBLACK   "\033[1m\033[30m"      /* Bold Black */
#define BOLDRED     "\033[1m\033[31m"      /* Bold Red */
#define BOLDGREEN   "\033[1m\033[32m"      /* Bold Green */
#define BOLDYELLOW  "\033[1m\033[33m"      /* Bold Yellow */
#define BOLDBLUE    "\033[1m\033[34m"      /* Bold Blue */
#define BOLDMAGENTA "\033[1m\033[35m"      /* Bold Magenta */
#define BOLDCYAN    "\033[1m\033[36m"      /* Bold Cyan */
#define BOLDWHITE   "\033[1m\033[37m"      /* Bold White */

    std::array<const char*,16> colorMap
    {
        "",         //    -- = 0,
        "",         //    -- = 1,
        GREEN,      //    LightGreen = 2,
        BLACK,      //    LightGrey = 3,
        RED,        //    LightRed = 4,
        WHITE,      //    OffWhite1 = 5,
        WHITE,      //    OffWhite2 = 6,
        "",         //         = 7
        BLACK,      //    Grey = 8,
        "",         //    -- = 9,
        BOLDGREEN,  //    Green = 10,
        BOLDBLUE,   //    Blue = 11,
        BOLDRED,    //    Red = 12,
        BOLDCYAN,   //    Pink = 13,
        BOLDYELLOW, //    Yellow = 14,
        RESET       //    White = 15
    };

    std::ostream& operator<<(std::ostream& out, Color tag)
    {
        if (tag == Color::Default)
            tag = ColorDefault;
#ifdef _MSC_VER
        SetConsoleTextAttribute(__m_hConsole, (WORD)tag | (240 & (WORD)ColorDefault) );
#else

        out << colorMap[15 & (char)tag];
#endif
        return out;
    }


    std::ostream& operator<<(std::ostream& out, IoStream tag)
    {
        if (tag == IoStream::lock)
        {
            gIoStreamMtx.lock();
        }
        else
        {
            gIoStreamMtx.unlock();
        }

        return out;
    }
}
