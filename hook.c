#include <wchar.h>
#include <windows.h>
#include <stdio.h>
#include <assert.h>

const char default_titles[] =
	"LEGO® Batman™\n"
	"LEGO Batman™\n"
	"LEGO® Batman\n"
	"LEGO Batman\n"
	"LEGO� Batman�\n"
	"LEGO� Batman\n"
	"LEGO Batman�\n"
	"Fallout: New Vegas";

typedef struct title_t {
	char* data;
	size_t len;
	struct title_t* next;
} title_t;

typedef struct info_t {
	const char* title;
	int title_len;
	HWND hWnd;
} info_t;

DWORD debug(LPVOID lpThreadParameter) {
    MessageBoxA(NULL, (char*)lpThreadParameter, (char*)lpThreadParameter, MB_ICONERROR);
	return 0;
}

int createUTF8FromWideStringWin32(const WCHAR* source, char* output, size_t max) {
    int size = 0;
    if (source == NULL) {
        return 0;
	}
	size = WideCharToMultiByte(CP_UTF8, 0, source, -1, NULL, 0, NULL, NULL);
	if (!size) {
		return 0;
	}

	if (size > (int)max)
		size = (int)max;

	if (!WideCharToMultiByte(CP_UTF8, 0, source, -1, output, size, NULL, NULL)) {
		return 0;
	}

	output[size] = 0;
	return size;
}

BOOL CALLBACK checkWindow(HWND hWnd, LPARAM lParam) {
	info_t* info = (info_t*)lParam;

	if (!IsWindowVisible(hWnd) || GetWindow(hWnd, GW_OWNER) != NULL) {
		return TRUE;
	}

	char title[256];

	wchar_t wtitle[256];
	memset(wtitle, 0, sizeof(wtitle));

	GetWindowTextW(hWnd, wtitle, sizeof(wtitle) / sizeof(wchar_t));
	int count = createUTF8FromWideStringWin32(wtitle, title, sizeof(title)) - 1;

	if (count == 0 || count != info->title_len) {
		return TRUE;
	}

	if (strncmp(title, info->title, info->title_len) == 0) {
		info->hWnd  = hWnd;
		return FALSE;
	}

	return TRUE;
}

DWORD hook(LPVOID lpThreadParameter) {
    /* find the game window (by searching through other windows) */
	info_t info;
	info.hWnd = NULL;

	char* buffer = (char*)default_titles;
	size_t count = sizeof(default_titles);

	FILE* file = fopen("title_list.txt", "r");
	if (file != NULL) {
		fseek(file, 0, SEEK_END);
		count = ftell(file);
		fseek(file, 0, SEEK_SET);

		buffer = (char*)malloc(count);
		count = fread(buffer, 1, count, file);
		fclose(file);
	}

	title_t* root = (title_t*)malloc(sizeof(title_t));
	title_t* cur = root;
	memset(cur, 0, sizeof(*cur));

	size_t i = 0;
	while (1) {
		cur->data = &buffer[i];

		size_t index = i;
		for (; index < count && buffer[index] != '\n' && buffer[index] != '\0'; index++);

		cur->len = index - i;
		i = index + 1;

		if (i >= count) {
			break;
		}

		cur->next = (title_t*)malloc(sizeof(title_t));
		memset(cur->next, 0, sizeof(*cur));
		cur = cur->next;
	}

    while (info.hWnd == NULL) {
		for (title_t* cur = root; cur; cur = cur->next) {
			if (cur->len == 0) continue;

			info.title = cur->data;
			info.title_len = cur->len;
			EnumWindows(checkWindow, (LPARAM)&info);
		}

        if (info.hWnd == NULL) /* If no window is found, the game window probably isn't open, lets wait before we check again */
            Sleep(100);
    }

	if (buffer != default_titles) {
		free(buffer);
	}

	while (root != NULL) {
		title_t* next = root->next;
		free(root);
		root = next;
	}

	/* get the monitor that the window is on (or the primary monitor) and figure out its size */
	HMONITOR src = MonitorFromWindow(info.hWnd, MONITOR_DEFAULTTOPRIMARY);

	MONITORINFOEX  monitorInfo;

	monitorInfo.cbSize = sizeof(MONITORINFOEX);
	GetMonitorInfoA(src, (LPMONITORINFO)&monitorInfo);

	UINT width  = monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left;
	UINT height = monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top;

	LONG lStyle = GetWindowLong(info.hWnd, GWL_STYLE);
	lStyle &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU);

	/* Make the game's window borderless fullscreen */
	SetWindowLong(info.hWnd, GWL_STYLE, lStyle); // (WS_POPUP | WS_VISIBLE);
	SetWindowPos(info.hWnd, NULL, 0, 0, width, height, SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOOWNERZORDER);
	return 0;
}

HMODULE hRealBink = NULL;
void LoadRealBink();

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)hook, NULL, 0, NULL);
            LoadRealBink();
            break;
        case DLL_PROCESS_DETACH:
            if (hRealBink) FreeLibrary(hRealBink);
            break;
    }
    return TRUE;
}


/*
    All the function passthrough stuff you probably don't care about
*/

typedef void* HBINK;
typedef HBINK (__stdcall *BinkOpen_t)(const char*, UINT32);
typedef void (__stdcall *BinkSetVolume_t)(HBINK, int, int);
typedef void (__stdcall *BinkCopyToBuffer_t)(HBINK, void*, int, int, int, int, int);
typedef void (__stdcall *BinkCopyToBufferRect_t)(HBINK, void*, int, int, int, int, int, int, int, int, int);
typedef void (__stdcall *BinkGetRects_t)(HBINK, void*);
typedef void (__stdcall *BinkClose_t)(HBINK);
typedef void (__stdcall *BinkWait_t)(HBINK);
typedef void (__stdcall *BinkNextFrame_t)(HBINK);
typedef int  (__stdcall *BinkShouldSkip_t)(HBINK);
typedef void (__stdcall *BinkDoFrame_t)(HBINK);
typedef void (__stdcall *BinkGoto_t)(HBINK, int, int);
typedef void (__stdcall *BinkRegisterFrameBuffers_t)(HBINK, void*);
typedef void (__stdcall *BinkPause_t)(HBINK, int);
typedef void (__stdcall *BinkGetFrameBuffersInfo_t)(HBINK, void*);
typedef void (__stdcall *BinkSetSoundTrack_t)(int, void*);
typedef void (__stdcall *BinkSetSoundSystem_t)(void*, void*);
typedef void (__stdcall *BinkOpenDirectSound_t)(void*);

BinkOpen_t real_BinkOpen = NULL;
BinkSetVolume_t real_BinkSetVolume = NULL;
BinkCopyToBuffer_t real_BinkCopyToBuffer = NULL;
BinkCopyToBufferRect_t real_BinkCopyToBufferRect = NULL;
BinkGetRects_t real_BinkGetRects = NULL;
BinkClose_t real_BinkClose = NULL;
BinkWait_t real_BinkWait = NULL;
BinkNextFrame_t real_BinkNextFrame = NULL;
BinkShouldSkip_t real_BinkShouldSkip = NULL;
BinkDoFrame_t real_BinkDoFrame = NULL;
BinkGoto_t real_BinkGoto = NULL;
BinkRegisterFrameBuffers_t real_BinkRegisterFrameBuffers = NULL;
BinkPause_t real_BinkPause = NULL;
BinkGetFrameBuffersInfo_t real_BinkGetFrameBuffersInfo = NULL;
BinkSetSoundTrack_t real_BinkSetSoundTrack = NULL;
BinkSetSoundSystem_t real_BinkSetSoundSystem = NULL;
BinkOpenDirectSound_t real_BinkOpenDirectSound = NULL;

__declspec(dllexport) HBINK __stdcall BinkOpen(const char* filename, UINT32 flags) {
    return real_BinkOpen(filename, flags);
}

__declspec(dllexport) void __stdcall BinkSetVolume(HBINK bink, int trackID, int volume) { if (real_BinkSetVolume) real_BinkSetVolume(bink, trackID, volume); }
__declspec(dllexport) void __stdcall BinkCopyToBuffer(HBINK bink, void* dest, int destPitch, int destHeight, int destX, int destY, int flags) {
    if (real_BinkCopyToBuffer) real_BinkCopyToBuffer(bink, dest, destPitch, destHeight, destX, destY, flags);
}
__declspec(dllexport) void __stdcall BinkCopyToBufferRect(HBINK bink, void* dest, int destPitch, int destHeight, int destX, int destY, int srcX, int srcY, int width, int height, int flags) {
    if (real_BinkCopyToBufferRect) real_BinkCopyToBufferRect(bink, dest, destPitch, destHeight, destX, destY, srcX, srcY, width, height, flags);
}

__declspec(dllexport) void __stdcall BinkGetRects(HBINK bink, void* rects) {
    real_BinkGetRects(bink, rects);
}

__declspec(dllexport) void __stdcall BinkClose(HBINK bink) {
    real_BinkClose(bink);
}

__declspec(dllexport) void __stdcall BinkWait(HBINK bink) {
    real_BinkWait(bink);
}

__declspec(dllexport) void __stdcall  BinkNextFrame(HBINK bink) {
    real_BinkNextFrame(bink);
}
__declspec(dllexport) int __stdcall  BinkShouldSkip(HBINK bink) {
    return real_BinkShouldSkip(bink);
}
__declspec(dllexport) void __stdcall  BinkDoFrame(HBINK bink) {
    real_BinkDoFrame(bink);
}
__declspec(dllexport) void __stdcall  BinkGoto(HBINK bink, int frame, int flags) {
    real_BinkGoto(bink, frame, flags);
}
__declspec(dllexport) void __stdcall  BinkRegisterFrameBuffers(HBINK bink, void* buffers) {
    real_BinkRegisterFrameBuffers(bink, buffers);
}
__declspec(dllexport) void __stdcall  BinkPause(HBINK bink, int pause) {
    real_BinkPause(bink, pause);
}
__declspec(dllexport) void __stdcall  BinkGetFrameBuffersInfo(HBINK bink, void* info) {
    real_BinkGetFrameBuffersInfo(bink, info);
}
__declspec(dllexport) void __stdcall  BinkSetSoundTrack(int trackCount, void* tracks) {
    real_BinkSetSoundTrack(trackCount, tracks);
}
__declspec(dllexport) void __stdcall  BinkSetSoundSystem(void* system, void* param) {
    real_BinkSetSoundSystem(system, param);
}
__declspec(dllexport) void __stdcall  BinkOpenDirectSound(void* dsound) {
    real_BinkOpenDirectSound(dsound);
}

void LoadRealBink() {
    hRealBink = LoadLibraryA("binkw32_real.dll");
    if (!hRealBink) {
        DWORD error = GetLastError();
        char errorMsg[256];
        FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, error, 0, errorMsg, sizeof(errorMsg), NULL);
        MessageBoxA(NULL, "Failed to Load binkw32_real.dll", "Failed to Load binkw32_real.dll", MB_ICONERROR);
        exit(1);
    }

	static char err_str[256];

    /* Load Function Pointers */
    #define LOAD_FUNC(name, dll, fail) real_##name = (name##_t)GetProcAddress(hRealBink, #dll); \
                            if ( real_##name == NULL) { \
								sprintf(err_str, "Failed to load binkw32 function (%s)\n", #dll); \
								if (fail) { \
									MessageBoxA(NULL, (char*)err_str, (char*)err_str, MB_ICONERROR); \
									exit(0); \
								} else CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)debug, err_str, 0, NULL); \
                            }

    LOAD_FUNC(BinkOpen, _BinkOpen@8, 1)
    LOAD_FUNC(BinkSetVolume, _BinkSetVolume@12, 1)
    LOAD_FUNC(BinkCopyToBuffer, _BinkCopyToBuffer@28, 1)
    LOAD_FUNC(BinkCopyToBufferRect, _BinkCopyToBufferRect@44, 1)
    LOAD_FUNC(BinkGetRects, _BinkGetRects@8, 1)
    LOAD_FUNC(BinkClose, _BinkClose@4, 1)
    LOAD_FUNC(BinkWait, _BinkWait@4, 1)
    LOAD_FUNC(BinkNextFrame, _BinkNextFrame@4, 1)
    LOAD_FUNC(BinkShouldSkip, _BinkShouldSkip@4, 0)
    LOAD_FUNC(BinkDoFrame, _BinkDoFrame@4, 1)
    LOAD_FUNC(BinkGoto, _BinkGoto@12, 1)
    LOAD_FUNC(BinkRegisterFrameBuffers, _BinkRegisterFrameBuffers@8, 1)
    LOAD_FUNC(BinkPause, _BinkPause@8, 1)
    LOAD_FUNC(BinkGetFrameBuffersInfo, _BinkGetFrameBuffersInfo@8, 1)
    LOAD_FUNC(BinkSetSoundTrack, _BinkSetSoundTrack@8, 1)
    LOAD_FUNC(BinkSetSoundSystem, _BinkSetSoundSystem@8, 1)
    LOAD_FUNC(BinkOpenDirectSound, _BinkOpenDirectSound@4, 1)
}
