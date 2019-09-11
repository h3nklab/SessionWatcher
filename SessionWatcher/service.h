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
#pragma once

class CService
{
public:
   CService(
      _In_  wchar_t  *pServiceName,
      _In_  wchar_t  *pLog);

   virtual
   ~CService();

   static BOOL
   Run(
      _In_ CService &service);

   wchar_t *
   GetName(
      void)
   {
      return m_sName;
   }

   wchar_t *
   GetLogPath(
      void)
   {
      return m_sLog;
   }

   void
   SetLogPath(
      _In_  wchar_t   *pPath)
   {
      wcscpy_s(m_sLog, sizeof(m_sLog) / sizeof(wchar_t), pPath);
   }

   static void WINAPI
   ServiceMain(
      _In_  DWORD    dwArgc,
      _In_  wchar_t  **pArgv);

   static DWORD WINAPI
   ServiceCtrlHandler(
      _In_  DWORD    dwCtrl,
      _In_  DWORD    dwEventType,
      _In_  LPVOID   pEventData,
      _In_  LPVOID   pContext);

   LPSERVICE_STATUS
   GetServiceStatus(
      void)
   {
      return &m_Status;
   }

   void
   SetServiceStatus(
      _In_  DWORD    dwCurrentState,
      _In_  DWORD    dwWin32ExitCode = NO_ERROR,
      _In_  DWORD    dwWaitHint = 0);

   SERVICE_STATUS_HANDLE
   GetServiceStatusHandle(
      void)
   {
      return m_hStatus;
   }


   void
   Start(
      _In_  DWORD    dwArgc,
      _In_  wchar_t  **pArgv);

   void
   Stop(
      void);

   void
   Pause(
      void);

   void
   Continue(
      void);

protected:
   wchar_t                 m_sName[64];
   wchar_t                 m_sLog[128];
   CSessionWatch           *m_pWatcher;
   SERVICE_STATUS          m_Status;
   SERVICE_STATUS_HANDLE   m_hStatus;

};