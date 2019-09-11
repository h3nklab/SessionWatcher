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