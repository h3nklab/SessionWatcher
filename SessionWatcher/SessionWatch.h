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

    File name: SessionWatch.h
    Contact: hdmih@yahoo.com
    Created: 15-07-2019
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
#pragma once

typedef  enum _WATCH_STATE {
   WATCH_STOP,
   WATCH_RUN,
   WATCH_PAUSE
} WATCH_STATE;

class CSessionWatch
{
public:
   CSessionWatch(
      _In_ wchar_t   *pFile);

   ~CSessionWatch();

   HMODULE        m_hWtsApi;

   HANDLE
   Invoke(
      void);

   void
   EndWatching(
      void);

   void
   Run(
      void);

   static void
   Log(
      _In_  HANDLE   hFile,
      _In_  wchar_t  *pFunction,
      _In_  wchar_t  *pMsg,
      ...);

   static wchar_t *
   GetSessionState(
      _In_ WPARAM wParam);

   HANDLE
   GetLogHandle(
      void)
   {
      return m_hLog;
   }

   WATCH_STATE
   GetState(
      void)
   {
      return m_State;
   }

   void
   SetState(
      _In_ WATCH_STATE state)
   {
      m_State = state;
   }

protected:
   DWORD          m_dwLastError;
   HANDLE         m_hLog;
   HANDLE         m_hThread;
   DWORD          m_dwTid;
   WATCH_STATE    m_State;

   static DWORD WINAPI
   SessionWatcher(
      _In_ LPVOID pParam);

   static LRESULT CALLBACK
   WindowHandler(
      _In_ HWND     hwnd,
      _In_ UINT     uiMsg,
      _In_ WPARAM   wParam,
      _In_ LPARAM   lParam);
};