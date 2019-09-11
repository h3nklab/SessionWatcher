#include "pch.h"
#include "exp.h"
#include "SessionWatch.h"
#include "service.h"

extern CService       *g_pService;

CService::CService(
   _In_  wchar_t  *pServiceName,
   _In_  wchar_t  *pLog)
   : m_pWatcher(NULL)
{
   DWORD    dwControlsAccepted = SERVICE_ACCEPT_STOP |
                                 SERVICE_ACCEPT_SHUTDOWN |
                                 SERVICE_ACCEPT_PAUSE_CONTINUE;

   wcscpy_s(m_sName, sizeof(m_sName) / sizeof(wchar_t), pServiceName);
   wcscpy_s(m_sLog, sizeof(m_sLog) / sizeof(wchar_t), pLog);


   m_Status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
   m_Status.dwCurrentState = SERVICE_STOPPED;

   m_Status.dwControlsAccepted = dwControlsAccepted;
   m_Status.dwWin32ExitCode = NO_ERROR;
   m_Status.dwServiceSpecificExitCode = 0;
   m_Status.dwCheckPoint = 0;
   m_Status.dwWaitHint = 0;
}

CService::~CService()
{
}

BOOL
CService::Run(
   _In_ CService &service)
{
   SERVICE_TABLE_ENTRY ste[] =
   {
      {service.GetName(), ServiceMain},
      {NULL, NULL}
   };

   return StartServiceCtrlDispatcher(ste);
}

void WINAPI
CService::ServiceMain(
   _In_  DWORD    dwArgc,
   _In_  wchar_t  **pArgv)
{
   g_pService->m_hStatus = RegisterServiceCtrlHandlerEx(
      g_pService->GetName(),
      ServiceCtrlHandler,
      NULL);

   if (g_pService->m_hStatus) {
      g_pService->Start(dwArgc, pArgv);
   }
}

DWORD WINAPI
CService::ServiceCtrlHandler(
   _In_  DWORD    dwCtrl,
   _In_  DWORD    dwEventType,
   _In_  LPVOID   pEventData,
   _In_  LPVOID   pContext)
{
   DWORD    dwRet = NO_ERROR;

   switch (dwCtrl)
   {
   case SERVICE_CONTROL_STOP:
   case SERVICE_CONTROL_SHUTDOWN:
      g_pService->Stop();
      break;

   case SERVICE_CONTROL_PAUSE:
      g_pService->Pause();
      break;

   case SERVICE_CONTROL_CONTINUE:
      g_pService->Continue();
      break;

   case SERVICE_CONTROL_INTERROGATE:
   default:
      dwRet = ERROR_CALL_NOT_IMPLEMENTED;
      break;
   }

   return dwRet;
}

void
CService::SetServiceStatus(
   _In_  DWORD    dwCurrentState,
   _In_  DWORD    dwWin32ExitCode,
   _In_  DWORD    dwWaitHint)
{
   static DWORD      dwCheckPoint = 1;

   m_Status.dwCurrentState = dwCurrentState;
   m_Status.dwWin32ExitCode = dwWin32ExitCode;
   m_Status.dwWaitHint = dwWaitHint;

   m_Status.dwCheckPoint =
      ((dwCurrentState == SERVICE_RUNNING) ||
      (dwCurrentState == SERVICE_STOPPED)) ? 0 : dwCheckPoint++;

   ::SetServiceStatus(m_hStatus, &m_Status);
}

void
CService::Start(
   _In_  DWORD    dwArgc,
   _In_  wchar_t  **pArgv)
{
   DWORD    dwPreviousState = g_pService->GetServiceStatus()->dwCurrentState;

   try {
      SetServiceStatus(SERVICE_START_PENDING);
      m_pWatcher = new CSessionWatch(g_pService->GetLogPath());
      m_pWatcher->Run();
      SetServiceStatus(SERVICE_RUNNING);
   }
   catch (CWatchExp& exp) {
      SetServiceStatus(dwPreviousState, exp.GetError());
      delete m_pWatcher;
      m_pWatcher = NULL;
   }
   catch (DWORD dwError) {
      SetServiceStatus(dwPreviousState, dwError);
   }
   catch (...) {
      SetServiceStatus(SERVICE_STOPPED);
   }
}

void
CService::Stop(
   void)
{
   DWORD    dwOriginalState = m_Status.dwCurrentState;

   try {
      SetServiceStatus(SERVICE_STOP_PENDING);
      if (m_pWatcher) {
         m_pWatcher->SetState(WATCH_STOP);
         delete m_pWatcher;
         m_pWatcher = NULL;
      }
      SetServiceStatus(SERVICE_STOPPED);
   }
   catch (CWatchExp& exp) {
      SetServiceStatus(dwOriginalState, exp.GetError());
   }
   catch (DWORD dwError) {
      SetServiceStatus(dwOriginalState, dwError);
   }
   catch (...) {
      SetServiceStatus(dwOriginalState);
   }
}

void
CService::Pause(
   void)
{
   DWORD    dwOriginalState = m_Status.dwCurrentState;

   try {
      SetServiceStatus(SERVICE_PAUSE_PENDING);
      m_pWatcher->SetState(WATCH_PAUSE);
      SetServiceStatus(SERVICE_PAUSED);
   }
   catch (CWatchExp& exp) {
      SetServiceStatus(dwOriginalState, exp.GetError());
   }
   catch (DWORD dwError) {
      SetServiceStatus(dwOriginalState, dwError);
   }
   catch (...) {
      SetServiceStatus(dwOriginalState);
   }
}

void
CService::Continue(
   void)
{
   DWORD    dwOriginalState = m_Status.dwCurrentState;

   try {
      SetServiceStatus(SERVICE_CONTINUE_PENDING);
      m_pWatcher->SetState(WATCH_RUN);
      SetServiceStatus(SERVICE_RUNNING);
   }
   catch (CWatchExp& exp) {
      SetServiceStatus(dwOriginalState, exp.GetError());
   }
   catch (DWORD dwError) {
      SetServiceStatus(dwOriginalState, dwError);
   }
   catch (...) {
      SetServiceStatus(dwOriginalState);
   }
}