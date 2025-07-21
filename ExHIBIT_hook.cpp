/**
 * @file soranana_chs.cpp
 * @brief 游戏汉化Hook模块
 * 
 * 主要功能：
 * - 优先从rld_chs目录加载脚本文件
 * - 自动将游戏字体修改为黑体
 * - 修改窗口标题显示汉化信息
 * 
 * 采用Detours技术实现API Hook
 */

#include <windows.h>
#include "detours/detours.h"
#include <stdio.h>
#include <stdlib.h>

/**
 * @brief 日志文件句柄 
 */
static HANDLE hLogFile = INVALID_HANDLE_VALUE;

/**
 * @brief 原始API函数指针类型定义
 */
typedef HFONT (WINAPI *PCreateFontIndirectA)(const LOGFONTA*);
typedef HFONT (WINAPI *PCreateFontIndirectW)(const LOGFONTA*);
typedef HANDLE (WINAPI *PCreateFileA)(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);
typedef HWND (WINAPI *PCreateWindowExA)(DWORD, LPCSTR, LPCSTR, DWORD, int, int, int, int, HWND, HMENU, HINSTANCE, LPVOID);
typedef HWND (WINAPI *PCreateWindowExW)(DWORD, LPCWSTR, LPCWSTR, DWORD, int, int, int, int, HWND, HMENU, HINSTANCE, LPVOID);
static PCreateFileA OriginalCreateFileA = NULL;
static PCreateWindowExA OriginalCreateWindowExA = NULL;
static PCreateWindowExW OriginalCreateWindowExW = NULL;

// 原始函数指针
PCreateFontIndirectA OriginalCreateFontIndirectA = NULL;

/**
 * @brief UTF-8日志输出函数
 * @param format 日志格式字符串
 * @param ... 可变参数
 * 
 * 支持记录宽字符路径，自动转换为UTF-8编码
 * 日志文件带BOM头确保编码正确
 */ 
void LogMessage(const char* format, ...) {
    if(hLogFile == INVALID_HANDLE_VALUE) return;
    
    va_list args;
    va_start(args, format);
    
    // 处理时间戳
    SYSTEMTIME st;
    GetLocalTime(&st);
    char timestamp[64];
    snprintf(timestamp, sizeof(timestamp), 
        "[%04d-%02d-%02d %02d:%02d:%02d] ",
        st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

    // 处理日志内容
    char msgBuf[1024];
    vsnprintf(msgBuf, sizeof(msgBuf), format, args);
    
    // 转换可能存在的宽字符路径参数为UTF-8
    char utf8Buf[1024] = {0};
    int wcount = 0;
    for(const char* p = msgBuf; *p; p++) {
        if(*p == '%' && *(p+1) == 's') {
            const char* path = va_arg(args, const char*);
            int len = WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)path, -1, 
                utf8Buf, sizeof(utf8Buf), NULL, NULL);
            if(len > 0) {
                wcount++;
                break;
            }
        }
    }
    va_end(args);
    va_start(args, format);
    
    // 最终日志行
    char output[2048];
    if(wcount > 0) {
        vsnprintf(output, sizeof(output), format, args);
    } else {
        vsnprintf(output, sizeof(output), format, args);
    }
    
    // 输出日志
    DWORD written;
    static bool bomWritten = false;
    if(!bomWritten) {
        const BYTE bom[] = {0xEF,0xBB,0xBF};
        WriteFile(hLogFile, bom, sizeof(bom), &written, NULL);
        bomWritten = true;
    }
    
    // 写入时间戳+内容
    WriteFile(hLogFile, timestamp, (DWORD)strlen(timestamp), &written, NULL);
    WriteFile(hLogFile, output, (DWORD)strlen(output), &written, NULL);
    WriteFile(hLogFile, "\n", 1, &written, NULL);
    
    // 控制台输出(调试用)
    printf("%s%s\n", timestamp, output);
    OutputDebugStringA(timestamp);
    OutputDebugStringA(output);
    OutputDebugStringA("\n");
    
    va_end(args);
}

/**
 * @brief 获取修改后的窗口标题
 * @return 返回固定标题字符串
 */
static LPCWSTR GetPatchedTitle() {
    return L"らぶらぶ♥プリンセス ～お姫さまがいっぱい！もっとエッチなハーレム生活!!～ claude-3-5-sonnet-20241022翻译补丁 || 作者：natsumerin@御爱同萌==雨宮ゆうこ@moyu || 允许转载但严禁倒卖和冒充人工汉化发布";
}

HWND WINAPI HookedCreateWindowExW(DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName, 
    DWORD dwStyle, int X, int Y, int nWidth, int nHeight, 
    HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam) {
    LogMessage("[TitleHook] Set window title");
    return OriginalCreateWindowExW(dwExStyle, lpClassName, GetPatchedTitle(), 
        dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
}

HWND WINAPI HookedCreateWindowExA(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName,
    DWORD dwStyle, int X, int Y, int nWidth, int nHeight,
    HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam) {
    WCHAR wszClassName[256] = {0};
    if (lpClassName) {
        MultiByteToWideChar(CP_ACP, 0, lpClassName, -1, wszClassName, 256);
    }
    return HookedCreateWindowExW(dwExStyle, lpClassName ? wszClassName : NULL,
        GetPatchedTitle(), dwStyle, X, Y, nWidth, nHeight, 
        hWndParent, hMenu, hInstance, lpParam);
}

/**
 * @brief CreateFileA Hook函数 - 优先加载中文脚本
 * 
 * 将ANSI路径转为Unicode处理，支持：
 * - 优先从rld_chs目录加载文件
 * - 日志记录完整UTF-8路径
 * 
 * @param lpFileName 文件名(ANSI)
 * @param ... 其他CreateFileA参数
 * @return 文件句柄
 */
HANDLE WINAPI HookedCreateFileA(
    LPCSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile) {
        
    if(!lpFileName) {
        return OriginalCreateFileA(lpFileName, dwDesiredAccess, dwShareMode,
            lpSecurityAttributes, dwCreationDisposition,
            dwFlagsAndAttributes, hTemplateFile);
    }

    // 将ANSI路径转为Unicode
    WCHAR wszFileName[MAX_PATH] = {0};
    MultiByteToWideChar(CP_ACP, 0, lpFileName, -1, wszFileName, MAX_PATH);

    WCHAR newPath[MAX_PATH] = {0};
    const WCHAR* origFileName = wszFileName;
    bool pathModified = false;
    
    // 只处理包含rld路径的文件
    if (wcsstr(wszFileName, L"rld")) {
        // 查找rld目录位置
        const WCHAR* rldPos = wcsstr(wszFileName, L"rld\\");
        if (rldPos && (rldPos - wszFileName) < MAX_PATH) {
            // 构建chs路径
            wcsncpy_s(newPath, MAX_PATH, wszFileName, rldPos - wszFileName);
            newPath[rldPos - wszFileName] = L'\0';
            wcsncat_s(newPath, MAX_PATH, L"rld_chs\\", _TRUNCATE);
            wcsncat_s(newPath, MAX_PATH, rldPos + 4, _TRUNCATE);
            
            // 检查CHS路径是否存在
            WIN32_FIND_DATAW findData;
            HANDLE hFind = FindFirstFileW(newPath, &findData);
            if (hFind != INVALID_HANDLE_VALUE) {
                FindClose(hFind);
                wcscpy_s(wszFileName, MAX_PATH, newPath);
                pathModified = true;
            }
        }
    }

    // 记录路径变动情况(转换为UTF-8记录)
    if(pathModified) {
        char utf8Orig[1024] = {0};
        char utf8New[1024] = {0};
        WideCharToMultiByte(CP_UTF8, 0, origFileName, -1, utf8Orig, 1024, NULL, NULL);
        WideCharToMultiByte(CP_UTF8, 0, newPath, -1, utf8New, 1024, NULL, NULL);
        LogMessage("[FileHook] Redirected: %s -> %s", utf8Orig, utf8New);
    }

    // 调用CreateFileW处理Unicode路径
    return CreateFileW(wszFileName, dwDesiredAccess, dwShareMode,
        lpSecurityAttributes, dwCreationDisposition,
        dwFlagsAndAttributes, hTemplateFile);
}

/**
 * @brief CreateFontIndirectA Hook函数 - 修改游戏字体
 * 
 * 自动将游戏字体替换为黑体，支持中文字符显示
 * 
 * @param lplf 原始字体参数
 * @return 新字体句柄
 */
HFONT WINAPI HookedCreateFontIndirectA(const LOGFONTA* lplf) {
    if (!lplf) {
        return OriginalCreateFontIndirectA(lplf);
    }

    // 转换ANSI字体结构到Wide版本
    LOGFONTW lfW;
    memset(&lfW, 0, sizeof(lfW));
    
    // 复制基本字段
    lfW.lfHeight = lplf->lfHeight;
    lfW.lfWidth = lplf->lfWidth;
    lfW.lfEscapement = lplf->lfEscapement;
    lfW.lfOrientation = lplf->lfOrientation;
    lfW.lfWeight = lplf->lfWeight;
    lfW.lfItalic = lplf->lfItalic;
    lfW.lfUnderline = lplf->lfUnderline;
    lfW.lfStrikeOut = lplf->lfStrikeOut;
    lfW.lfCharSet = 0x86; // GB2312字符集
    lfW.lfOutPrecision = lplf->lfOutPrecision;
    lfW.lfClipPrecision = lplf->lfClipPrecision;
    lfW.lfQuality = lplf->lfQuality;
    lfW.lfPitchAndFamily = lplf->lfPitchAndFamily;
    
    // 设置黑体
    wcscpy_s(lfW.lfFaceName, L"黑体");

    // 转换源字体名(CP932)到UTF-8并记录修改信息
    char srcFontNameUtf8[64] = {0};
    if (lplf->lfFaceName[0]) {
        int wlen = MultiByteToWideChar(932, 0, lplf->lfFaceName, -1, NULL, 0);
        WCHAR* wbuffer = (WCHAR*)malloc(wlen * sizeof(WCHAR));
        MultiByteToWideChar(932, 0, lplf->lfFaceName, -1, wbuffer, wlen);
        WideCharToMultiByte(CP_UTF8, 0, wbuffer, -1, srcFontNameUtf8, sizeof(srcFontNameUtf8), NULL, NULL);
        free(wbuffer);
    }
    LogMessage("[FontHook] Font modified: Charset:%d->%d, Face:%s->黑体",
        lplf->lfCharSet, lfW.lfCharSet, srcFontNameUtf8[0] ? srcFontNameUtf8 : lplf->lfFaceName);

    // 调用Wide版本的函数
    return CreateFontIndirectW(&lfW);
}

/**
 * @brief 安装所有API Hook
 * @return 是否安装成功
 * 
 * 安装以下Hook:
 * - CreateFontIndirectA(字体修改)
 * - CreateFileA(文件重定向) 
 * - CreateWindowExA/W(窗口标题修改)
 */
__declspec(dllexport) BOOL WINAPI InstallFontHook() {
    // 获取原始函数地址
    OriginalCreateFontIndirectA = (PCreateFontIndirectA)GetProcAddress(GetModuleHandleA("gdi32.dll"), "CreateFontIndirectA");
    OriginalCreateFileA = (PCreateFileA)GetProcAddress(GetModuleHandleA("kernel32.dll"), "CreateFileA");
    OriginalCreateWindowExA = (PCreateWindowExA)GetProcAddress(GetModuleHandleA("user32.dll"), "CreateWindowExA");
    OriginalCreateWindowExW = (PCreateWindowExW)GetProcAddress(GetModuleHandleA("user32.dll"), "CreateWindowExW");
    if (!OriginalCreateFontIndirectA || !OriginalCreateFileA || !OriginalCreateWindowExA || !OriginalCreateWindowExW) {
        return FALSE;
    }

    // 开始事务
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    
    // 安装hook
    DetourAttach(&(PVOID&)OriginalCreateFontIndirectA, HookedCreateFontIndirectA);
    DetourAttach(&(PVOID&)OriginalCreateFileA, HookedCreateFileA);
    DetourAttach(&(PVOID&)OriginalCreateWindowExA, HookedCreateWindowExA);
    DetourAttach(&(PVOID&)OriginalCreateWindowExW, HookedCreateWindowExW);
    
    // 提交事务
    DWORD error = DetourTransactionCommit();
    if (error != NO_ERROR) {
        return FALSE;
    }

    return TRUE;
}

/**
 * @brief 卸载所有API Hook
 * @return 是否卸载成功
 */
__declspec(dllexport) BOOL WINAPI RemoveFontHook() {
    // 开始事务
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    
    // 卸载hook
    DetourDetach(&(PVOID&)OriginalCreateFontIndirectA, HookedCreateFontIndirectA);
    DetourDetach(&(PVOID&)OriginalCreateFileA, HookedCreateFileA);
    DetourDetach(&(PVOID&)OriginalCreateWindowExA, HookedCreateWindowExA);
    DetourDetach(&(PVOID&)OriginalCreateWindowExW, HookedCreateWindowExW);
    
    // 提交事务
    DWORD error = DetourTransactionCommit();
    if (error != NO_ERROR) {
        return FALSE;
    }

    return TRUE;
}

/**
 * @brief 初始化日志文件
 * @return 是否初始化成功
 */
BOOL InitLogFile() {
    hLogFile = CreateFileA("ExHIBIT_hook.log", GENERIC_WRITE, 
        FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    return hLogFile != INVALID_HANDLE_VALUE;
}

/**
 * @brief 关闭日志文件
 */
VOID CloseLogFile() {
    if (hLogFile != INVALID_HANDLE_VALUE) {
        CloseHandle(hLogFile);
        hLogFile = INVALID_HANDLE_VALUE;
    }
}

/**
 * @brief DLL入口函数
 * 
 * @param hinstDLL DLL实例句柄
 * @param fdwReason 调用原因
 * @param lpvReserved 保留参数
 * @return 是否加载成功
 * 
 * 处理流程:
 * - DLL_PROCESS_ATTACH: 初始化日志并安装Hook
 * - DLL_PROCESS_DETACH: 关闭日志并卸载Hook
 */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
            if (!InitLogFile()) {
                return FALSE;
            }
            return InstallFontHook();
        case DLL_PROCESS_DETACH:
            CloseLogFile();
            return RemoveFontHook();
    }
    return TRUE;
}
