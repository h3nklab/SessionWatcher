/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    Copyright (C) 2019 H3nklab Team

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
#include "pch.h"
#include "exp.h"
#include "SessionWatch.h"

typedef BOOL(WINAPI * PWTS_REGISTER_SESSION_NOTIFICATION) (
   HWND  hwnd,
   DWORD dwFlags);

typedef BOOL(WINAPI * PWTS_UNREGISTER_SESSION_NOTIFICATION) (
   HWND  hwnd);

CSessionWatch  *g_pWatcher = NULL;

CSessionWatch::CSessionWatch(
   _In_ wchar_t   *pFile)
   : m_hLog(INVALID_HANDLE_VALUE)
   , m_hThread(NULL)
   , m_State(WATCH_STOP)
{
   wchar_t       sMessage[256] = { L'\0' };

   m_hWtsApi = LoadLibrary(L"wtsapi32.dll");
   if (m_hWtsApi == NULL) {
      m_dwLastError = GetLastError();

      StringCchPrintf(
         sMessage,
         sizeof(sMessage) / sizeof(wchar_t),
         L"Failed loading WTSAPI32.DLL: %d",
         m_dwLastError);

      throw CWatchExp(m_dwLastError, sMessage);
   }

   m_hLog = CreateFile(
      pFile,
      GENERIC_ALL,
      FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
      NULL,
      CREATE_ALWAYS,
      FILE_ATTRIBUTE_NORMAL,
      NULL);

   if (m_hLog == INVALID_HANDLE_VALUE) {
      m_dwLastError = GetLastError();
      StringCchPrintf(
         sMessage,
         sizeof(sMessage) / sizeof(wchar_t),
         L"Failed opening file %ws: %u",
         pFile,
         m_dwLastError);

      throw CWatchExp(m_dwLastError, sMessage);
   }

   g_pWatcher = this;

   Log(m_hLog, __FUNCTIONW__, L"*************** H3nklab Session State Watcher ***************");
}

CSessionWatch::~CSessionWatch()
{
   EndWatching();
   g_pWatcher = NULL;

   if (m_hWtsApi) {
      FreeLibrary(m_hWtsApi);
      m_hWtsApi = NULL;
   }
   Log(m_hLog, __FUNCTIONW__, L"******************** End of session watch *******************");

   if (m_hLog != INVALID_HANDLE_VALUE) {
      CloseHandle(m_hLog);
      m_hLog = INVALID_HANDLE_VALUE;
   }
}

void
CSessionWatch::Run(
   void)
{
   wchar_t  sMessage[256] = { L'\0' };

   m_hThread = CreateThread(
      NULL,
      0,
      SessionWatcher,
      this,
      0,
      &m_dwTid);

   if (m_hThread == NULL) {
      m_dwLastError = GetLastError();

      StringCchPrintf(
         sMessage,
         sizeof(sMessage) / sizeof(wchar_t),
         L"Failed invoking watcher thread: %u",
         m_dwLastError);

      throw CWatchExp(m_dwLastError, sMessage);
   }
}

DWORD WINAPI
CSessionWatch::SessionWatcher(
   _In_ LPVOID pParam)
{
   CSessionWatch                          *pWatcher = (CSessionWatch *)pParam;
   wchar_t                                sMessage[256] = { L'\0' };
   PWTS_REGISTER_SESSION_NOTIFICATION     pRegister = NULL;
   PWTS_UNREGISTER_SESSION_NOTIFICATION   pUnregister = NULL;
   WNDCLASS                               wndClass = { 0 };
   HWND                                   hWnd = NULL;
   MSG                                    msg = { 0 };
   int                                    iRet = 0;

   pRegister = (PWTS_REGISTER_SESSION_NOTIFICATION)GetProcAddress(
      pWatcher->m_hWtsApi,
      "WTSRegisterSessionNotification");

   if (pRegister == NULL) {
      pWatcher->m_dwLastError = GetLastError();
      StringCchPrintf(
         sMessage,
         sizeof(sMessage) / sizeof(wchar_t),
         L"Failed to find WTSRegisterSessionNotification: 0x%08X",
         HRESULT_FROM_WIN32(pWatcher->m_dwLastError));

      goto Cleanup;
   }

   pUnregister = (PWTS_UNREGISTER_SESSION_NOTIFICATION)GetProcAddress(
      pWatcher->m_hWtsApi,
      "WTSUnRegisterSessionNotification");

   if (pUnregister == NULL) {
      pWatcher->m_dwLastError = GetLastError();
      StringCchPrintf(
         sMessage,
         sizeof(sMessage) / sizeof(wchar_t),
         L"Failed to find WTSUnRegisterSessionNotification: 0x%08X",
         HRESULT_FROM_WIN32(pWatcher->m_dwLastError));

      goto Cleanup;
   }

   wndClass.lpfnWndProc = WindowHandler;
   wndClass.hInstance = GetModuleHandle(0);
   wndClass.lpszClassName = L"h3SessionWatcher";

   if (!RegisterClass(&wndClass)) {
      pWatcher->m_dwLastError = GetLastError();
      StringCchPrintf(
         sMessage,
         sizeof(sMessage) / sizeof(wchar_t),
         L"Failed to register window class: 0x%08X",
         pWatcher->m_dwLastError);

      goto Cleanup;
   }

   hWnd = CreateWindow(
      wndClass.lpszClassName,
      L"",
      WS_POPUP,
      0,
      0,
      0,
      0,
      HWND_MESSAGE,
      0,
      wndClass.hInstance,
      NULL);
   
   if (hWnd == NULL) {
      pWatcher->m_dwLastError = GetLastError();
      StringCchPrintf(
         sMessage,
         sizeof(sMessage) / sizeof(wchar_t),
         L"Failed to create window: 0x%08X",
         pWatcher->m_dwLastError);

      goto Cleanup;
   }

   if (!pRegister(hWnd, NOTIFY_FOR_ALL_SESSIONS)) {
      pWatcher->m_dwLastError = GetLastError();
      StringCchPrintf(
         sMessage,
         sizeof(sMessage) / sizeof(wchar_t),
         L"Failed to register session changes notification: 0x%08X",
         pWatcher->m_dwLastError);

      DestroyWindow(hWnd);
      goto Cleanup;
   }

   pWatcher->SetState(WATCH_RUN);

   while (((iRet = GetMessage(&msg, NULL, 0, 0)) != 0) &&
          (iRet != -1) &&
          (msg.message != WM_CLOSE)) {

      DispatchMessage(&msg);
   }

   if ((msg.message != WM_CLOSE)) {
      StringCchPrintf(
         sMessage,
         sizeof(sMessage) / sizeof(WCHAR),
         L"Unexpected session watcher termination: %u",
         pWatcher->m_dwLastError);

      Log(pWatcher->GetLogHandle(), __FUNCTIONW__, sMessage);
   }
   else {
      Log(pWatcher->GetLogHandle(), __FUNCTIONW__, L"Stopped session watcher");
   }

   pUnregister(hWnd);
   DestroyWindow(hWnd);

Cleanup:
   return pWatcher->m_dwLastError;
}

LRESULT CALLBACK
CSessionWatch::WindowHandler(
   _In_ HWND     hwnd,
   _In_ UINT     uiMsg,
   _In_ WPARAM   wParam,
   _In_ LPARAM   lParam)
{
   wchar_t  sMessage[2048] = { L'\0' };

   if (uiMsg == WM_WTSSESSION_CHANGE) {
      StringCchPrintf(
         sMessage,
         sizeof(sMessage) / sizeof(wchar_t),
         L"%ws at session-%u",
         GetSessionState(wParam),
         (DWORD)lParam);

      if (g_pWatcher->GetState() == WATCH_RUN) {
         Log(g_pWatcher->GetLogHandle(), __FUNCTIONW__, sMessage);
      }
   }

   return DefWindowProc(hwnd, uiMsg, wParam, lParam);
}

void
CSessionWatch::EndWatching(
   void)
{
   PostThreadMessage(m_dwTid, WM_CLOSE, 0, 0);
   WaitForSingleObject(m_hThread, INFINITE);
   m_hThread = NULL;
}

void
CSessionWatch::Log(
   _In_  HANDLE   hFile,
   _In_  wchar_t  *pFunction,
   _In_  wchar_t  *pMsg,
   ...)
{
   SYSTEMTIME     currentTime = { 0 };
   va_list        pArg;
   wchar_t        sTmp[2048] = { L'\0' };
   wchar_t        sMsg[4096] = { L'\0' };
   DWORD          dwWrote = 0;
   DWORD          dwLength = 0;
   BOOL           bRet = FALSE;

   va_start(pArg, pMsg);
   StringCchVPrintf(sTmp, sizeof(sTmp) / sizeof(wchar_t), pMsg, pArg);
   va_end(pArg);

   GetLocalTime(&currentTime);

   StringCchPrintf(
      sMsg,
      sizeof(sMsg) / sizeof(wchar_t),
      L"%.4d-%.2d-%.2d %.2d:%.2d:%.2d.%.3d [%ws] %ws\r\n",
      currentTime.wYear,
      currentTime.wMonth,
      currentTime.wDay,
      currentTime.wHour,
      currentTime.wMinute,
      currentTime.wSecond,
      currentTime.wMilliseconds,
      pFunction,
      sTmp);

   dwLength = (DWORD)wcslen(sMsg);

   do {
      dwWrote = 0;

      bRet = WriteFile(hFile, sMsg, dwLength * sizeof(wchar_t), &dwWrote, NULL);
      dwLength -= dwWrote;
   } while (dwLength > 0 && bRet);
}

wchar_t *
CSessionWatch::GetSessionState(
   _In_ WPARAM wParam)
{
   static wchar_t sState[64] = { L'\0' };

   switch (wParam) {
   case WTS_CONSOLE_CONNECT:
      wcscpy_s(sState, sizeof(sState) / sizeof(wchar_t), L"WTS_CONSOLE_CONNECT");
      break;
   case WTS_CONSOLE_DISCONNECT:
      wcscpy_s(sState, sizeof(sState) / sizeof(wchar_t), L"WTS_CONSOLE_DISCONNECT");
      break;
   case WTS_REMOTE_CONNECT:
      wcscpy_s(sState, sizeof(sState) / sizeof(wchar_t), L"WTS_REMOTE_CONNECT");
      break;
   case WTS_REMOTE_DISCONNECT:
      wcscpy_s(sState, sizeof(sState) / sizeof(wchar_t), L"WTS_REMOTE_DISCONNECT");
      break;
   case WTS_SESSION_LOGON:
      wcscpy_s(sState, sizeof(sState) / sizeof(wchar_t), L"WTS_SESSION_LOGON");
      break;
   case WTS_SESSION_LOGOFF:
      wcscpy_s(sState, sizeof(sState) / sizeof(wchar_t), L"WTS_SESSION_LOGOFF");
      break;
   case WTS_SESSION_LOCK:
      wcscpy_s(sState, sizeof(sState) / sizeof(wchar_t), L"WTS_SESSION_LOCK");
      break;
   case WTS_SESSION_UNLOCK:
      wcscpy_s(sState, sizeof(sState) / sizeof(wchar_t), L"WTS_SESSION_UNLOCK");
      break;
   case WTS_SESSION_REMOTE_CONTROL:
      wcscpy_s(sState, sizeof(sState) / sizeof(wchar_t), L"WTS_SESSION_REMOTE_CONTROL");
      break;
   case WTS_SESSION_CREATE:
      wcscpy_s(sState, sizeof(sState) / sizeof(wchar_t), L"WTS_SESSION_CREATE");
      break;
   case WTS_SESSION_TERMINATE:
      wcscpy_s(sState, sizeof(sState) / sizeof(wchar_t), L"WTS_SESSION_TERMINATE");
      break;
   default:
      wcscpy_s(sState, sizeof(sState) / sizeof(wchar_t), L"WTS_UNDEFINED");
      break;
   }

   return sState;
}