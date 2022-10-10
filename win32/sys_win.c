/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// sys_win.h

#include "../qcommon/qcommon.h"
#include "../win32/conproc.h"
#include "resource.h"
#include "winquake.h"
#include <conio.h>
#include <direct.h>
#include <errno.h>
#include <fcntl.h>
#include <float.h>
#include <io.h>
#include <stdio.h>

#include <GLFW/glfw3.h>

#define MINIMUM_WIN_MEMORY 0x0a00000
#define MAXIMUM_WIN_MEMORY 0x1000000

//#define DEMO

bool s_win95;

int starttime;
int ActiveApp;
bool Minimized;

static HANDLE hinput, houtput;

unsigned sys_msg_time;
unsigned sys_frame_time;

static HANDLE qwclsemaphore;

#define MAX_NUM_ARGVS 128
int argc;
char *argv[MAX_NUM_ARGVS];

/*
===============================================================================

SYSTEM IO

===============================================================================
*/

void Sys_Error(char *error, ...) {
  va_list argptr;
  char text[1024];

  CL_Shutdown();
  Qcommon_Shutdown();

  va_start(argptr, error);
  vsprintf(text, error, argptr);
  va_end(argptr);

  MessageBox(NULL, text, "Error", 0 /* MB_OK */);

  if(qwclsemaphore)
    CloseHandle(qwclsemaphore);

  // shut down QHOST hooks if necessary
  DeinitConProc();

  exit(1);
}

void Sys_Quit(void) {
  timeEndPeriod(1);

  CL_Shutdown();
  Qcommon_Shutdown();
  CloseHandle(qwclsemaphore);
  if(dedicated && dedicated->value)
    FreeConsole();

  // shut down QHOST hooks if necessary
  DeinitConProc();

  exit(0);
}

void WinError(void) {
  LPVOID lpMsgBuf;

  FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(),
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                (LPTSTR)&lpMsgBuf, 0, NULL);

  // Display the string.
  MessageBox(NULL, lpMsgBuf, "GetLastError", MB_OK | MB_ICONINFORMATION);

  // Free the buffer.
  LocalFree(lpMsgBuf);
}

//================================================================

static void windows_uv_idle_cb(uv_idle_t *handle) {
  extern GLFWwindow *glfw_window;

  if(glfw_window != NULL && glfwWindowShouldClose(glfw_window)) {
    Com_Quit();
  }

  sys_msg_time = Sys_Milliseconds();

  glfwPollEvents();
}

/*
================
Sys_Init
================
*/
void Sys_Init(void) {
  static uv_idle_t windows_uv_idle;
  uv_idle_init(&global_uv_loop, &windows_uv_idle);
  uv_idle_start(&windows_uv_idle, windows_uv_idle_cb);

  OSVERSIONINFO vinfo;

  timeBeginPeriod(1);

  vinfo.dwOSVersionInfoSize = sizeof(vinfo);

  if(!GetVersionEx(&vinfo))
    Sys_Error("Couldn't get OS info");

  if(vinfo.dwMajorVersion < 4)
    Sys_Error("Quake2 requires windows version 4 or greater");
  if(vinfo.dwPlatformId == VER_PLATFORM_WIN32s)
    Sys_Error("Quake2 doesn't run on Win32s");
  else if(vinfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
    s_win95 = true;

  if(dedicated->value) {
    // if(!AllocConsole())
    //   Sys_Error("Couldn't create dedicated server console");
    AllocConsole();
    hinput = GetStdHandle(STD_INPUT_HANDLE);
    houtput = GetStdHandle(STD_OUTPUT_HANDLE);

    // let QHOST hook in
    InitConProc(argc, argv);
  }
}

static char console_text[256];
static int console_textlen;

/*
================
Sys_ConsoleInput
================
*/
char *Sys_ConsoleInput(void) {
  INPUT_RECORD recs[1024];
  DWORD dummy;
  int ch;
  DWORD numread;
  DWORD numevents;

  if(!dedicated || !dedicated->value)
    return NULL;

  for(;;) {
    if(!GetNumberOfConsoleInputEvents(hinput, &numevents))
      Sys_Error("Error getting # of console events");

    if(numevents <= 0)
      break;

    if(!ReadConsoleInput(hinput, recs, 1, &numread))
      Sys_Error("Error reading console input");

    if(numread != 1)
      Sys_Error("Couldn't read console input");

    if(recs[0].EventType == KEY_EVENT) {
      if(!recs[0].Event.KeyEvent.bKeyDown) {
        ch = recs[0].Event.KeyEvent.uChar.AsciiChar;

        switch(ch) {
        case '\r':
          WriteFile(houtput, "\r\n", 2, &dummy, NULL);

          if(console_textlen) {
            console_text[console_textlen] = 0;
            console_textlen = 0;
            return console_text;
          }
          break;

        case '\b':
          if(console_textlen) {
            console_textlen--;
            WriteFile(houtput, "\b \b", 3, &dummy, NULL);
          }
          break;

        default:
          if(ch >= ' ') {
            if(console_textlen < sizeof(console_text) - 2) {
              WriteFile(houtput, &ch, 1, &dummy, NULL);
              console_text[console_textlen] = ch;
              console_textlen++;
            }
          }

          break;
        }
      }
    }
  }

  return NULL;
}

/*
================
Sys_ConsoleOutput

Print text to the dedicated console
================
*/
void Sys_ConsoleOutput(char *string) {
  DWORD dummy;
  char text[256];

  if(!dedicated || !dedicated->value)
    return;

  if(console_textlen) {
    text[0] = '\r';
    memset(&text[1], ' ', console_textlen);
    text[console_textlen + 1] = '\r';
    text[console_textlen + 2] = 0;
    WriteFile(houtput, text, console_textlen + 2, &dummy, NULL);
  }

  WriteFile(houtput, string, strlen(string), &dummy, NULL);

  if(console_textlen)
    WriteFile(houtput, console_text, console_textlen, &dummy, NULL);
}

/*
================
Sys_SendKeyEvents

Send Key_Event calls
================
*/
void Sys_SendKeyEvents(void) {
  extern GLFWwindow *glfw_window;

  if(glfw_window != NULL && glfwWindowShouldClose(glfw_window)) {
    Com_Quit();
  }

  glfwPollEvents();

  // grab frame time
  sys_frame_time = Sys_Milliseconds(); // FIXME: should this be at start?
}

/*
================
Sys_GetClipboardData

================
*/
char *Sys_GetClipboardData(void) {
  char *data = NULL;
  char *cliptext;

  if(OpenClipboard(NULL) != 0) {
    HANDLE hClipboardData;

    if((hClipboardData = GetClipboardData(CF_TEXT)) != 0) {
      if((cliptext = GlobalLock(hClipboardData)) != 0) {
        data = malloc(GlobalSize(hClipboardData) + 1);
        strcpy(data, cliptext);
        GlobalUnlock(hClipboardData);
      }
    }
    CloseClipboard();
  }
  return data;
}

/*
==============================================================================

 WINDOWS CRAP

==============================================================================
*/

/*
=================
Sys_AppActivate
=================
*/
void Sys_AppActivate(void) {
  // ShowWindow(cl_hwnd, SW_RESTORE);
  // SetForegroundWindow(cl_hwnd);
}

/*
========================================================================

GAME DLL

========================================================================
*/

/*
=================
Sys_UnloadGame
=================
*/
void Sys_UnloadGame(void) {}

/*
=================
Sys_GetGameAPI

Loads the game dll
=================
*/
void *Sys_GetGameAPI(void *parms) {
  extern void *GetGameAPI(void *);
  return GetGameAPI(parms);
}

//=======================================================================

/*
==================
ParseCommandLine

==================
*/
void ParseCommandLine(LPSTR lpCmdLine) {
  argc = 1;
  argv[0] = "exe";

  while(*lpCmdLine && (argc < MAX_NUM_ARGVS)) {
    while(*lpCmdLine && ((*lpCmdLine <= 32) || (*lpCmdLine > 126)))
      lpCmdLine++;

    if(*lpCmdLine) {
      argv[argc] = lpCmdLine;
      argc++;

      while(*lpCmdLine && ((*lpCmdLine > 32) && (*lpCmdLine <= 126)))
        lpCmdLine++;

      if(*lpCmdLine) {
        *lpCmdLine = 0;
        lpCmdLine++;
      }
    }
  }
}

/*
==================
WinMain

==================
*/
HINSTANCE global_hInstance;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
  // MSG msg;
  // int time, oldtime, newtime;
  // char *cddir;

  /* previous instances do not exist in Win32 */
  if(hPrevInstance)
    return 0;

  global_hInstance = hInstance;

  ParseCommandLine(lpCmdLine);

  Qcommon_Init(argc, argv);
  return Qcommon_RunFrames();
}
