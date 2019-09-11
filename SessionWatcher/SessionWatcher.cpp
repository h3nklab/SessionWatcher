#include "pch.h"
#include "exp.h"
#include "SessionWatch.h"
#include "service.h"

#define  H3_SERVICE_NAME             L"h3nklab"
#define  H3_SERVICE_DISPLAY_NAME     L"H3nklab Session Watcher Demo"
#define  H3_SERVICE_START_TYPE       SERVICE_DEMAND_START
#define  H3_SERVICE_DEPENDENCIES     L"RPCSS\0"
#define  H3_DEFAULT_SERVICE_ACCOUNT  NULL
#define  H3_DEFAULT_SERVICE_PASSWORD NULL
#define  H3_LOG_FILE                 L"C:\\Temp\\watch.log"
#define  H3_SERVICE_DESCRIPTION      L"Session state watcher demo. Provided by H3nklab"


HRESULT
SetServiceDescription(
   _In_  SC_HANDLE   hService,
   _In_  WCHAR       *pDescription)
{
   HRESULT              hRes = S_OK;
   SERVICE_DESCRIPTION  sd;

   assert(hService);
   assert(pDescription);

   sd.lpDescription = pDescription;
   if (!ChangeServiceConfig2(hService, SERVICE_CONFIG_DESCRIPTION, &sd)) {
      hRes = HRESULT_FROM_WIN32(GetLastError());
   }

   return hRes;
}

void
InstallService(
   PWSTR          pServiceName,
   PWSTR          pServiceDisplayName,
   DWORD          dwStartType,
   PWSTR          pDependencies,
   PWSTR          pAccount,
   PWSTR          pPassword
)
{
   HRESULT        hRes = S_OK;
   wchar_t        sPath[MAX_PATH];
   SC_HANDLE      hSCManager = NULL;
   SC_HANDLE      hSCService = NULL;

   wprintf(L"Attempting to install %ws: %ws\n", pServiceName, pServiceDisplayName);

   if (GetModuleFileName(NULL, sPath, ARRAYSIZE(sPath)) == 0) {
      wprintf(L"[%ws] Unable to get module file name: 0x%08X\n",
         __FUNCTIONW__,
         GetLastError());

      goto Cleanup;
   }

   hSCManager = OpenSCManager(
      NULL,
      NULL,
      SC_MANAGER_CONNECT | SC_MANAGER_CREATE_SERVICE);

   if (hSCManager == NULL) {
      wprintf(L"[%ws] Failed opening service manager: 0x%08X\n",
         __FUNCTIONW__,
         GetLastError());

      goto Cleanup;
   }

   hSCService = CreateService(
      hSCManager,
      pServiceName,
      pServiceDisplayName,
      SERVICE_ALL_ACCESS,
      SERVICE_WIN32_OWN_PROCESS,
      dwStartType,
      SERVICE_ERROR_NORMAL,
      sPath,
      NULL,
      NULL,
      pDependencies,
      pAccount,
      pPassword);

   if (hSCService == NULL) {
      wprintf(L"[%ws] Unable to create service [%ws]: 0x%08X\n",
         __FUNCTIONW__,
         sPath,
         GetLastError());

      goto Cleanup;
   }

   hRes = SetServiceDescription(hSCService, H3_SERVICE_DESCRIPTION);
   if (hRes != S_OK) {
      wprintf(L"[%ws] Failed setting service desription: 0x%08X",
         __FUNCTIONW__,
         hRes);

      goto Cleanup;
   }

   wprintf(L"[%ws] Service [%ws] installed.\n", __FUNCTIONW__, sPath);

Cleanup:
   if (hSCService) {
      CloseServiceHandle(hSCService);
      hSCService = NULL;
   }
   if (hSCManager) {
      CloseServiceHandle(hSCManager);
      hSCManager = NULL;
   }
}


void
UninstallService(
   PWSTR          pServiceName
)
{
   SC_HANDLE         hSCManager = NULL;
   SC_HANDLE         hSCService = NULL;
   SERVICE_STATUS    svcStatus = {};
   BOOL              bRet;

   hSCManager = OpenSCManager(
      NULL,
      NULL,
      SC_MANAGER_CONNECT | SC_MANAGER_CREATE_SERVICE);

   if (hSCManager == NULL) {
      wprintf(L"[%ws] Failed opening service manager: 0x%08X\n",
         __FUNCTIONW__,
         GetLastError());

      goto Cleanup;
   }

   hSCService = OpenService(
      hSCManager,
      pServiceName,
      SERVICE_STOP | SERVICE_QUERY_STATUS | DELETE);

   if (hSCService == NULL) {
      wprintf(L"[%ws] Unable to open service [%ws]: 0x%08X\n",
         __FUNCTIONW__,
         pServiceName,
         GetLastError());

      goto Cleanup;
   }

   /*
    * Stop the service
    */
   bRet = QueryServiceStatus(hSCService, &svcStatus);
   if (bRet) {

      if (svcStatus.dwCurrentState != SERVICE_STOP_PENDING &&
         svcStatus.dwCurrentState != SERVICE_STOPPED) {

         bRet = ControlService(hSCService, SERVICE_CONTROL_STOP, &svcStatus);
         if (!bRet) {
            wprintf(L"[%ws] Failed invoking to stop service: 0x%8X\n",
               __FUNCTIONW__,
               GetLastError());

            goto Cleanup;
         }
         wprintf(L"Stopping %ws", pServiceName);
         Sleep(1000);

         while (QueryServiceStatus(hSCService, &svcStatus)) {
            if (svcStatus.dwCurrentState == SERVICE_STOP_PENDING) {
               wprintf(L".");
               Sleep(1000);
            }
            else {
               break;
            }
         }

         if (svcStatus.dwCurrentState == SERVICE_STOPPED) {
            wprintf(L"\n[%ws] Service %ws is stopped.\n",
               __FUNCTIONW__,
               pServiceName);
         }
         else {
            wprintf(L"\n[%ws] Failed stopping service %ws: 0x%08X\n",
               __FUNCTIONW__,
               pServiceName,
               GetLastError());
         }
      }
      else {
         wprintf(L"\n[%ws] Service has been stopped\n", __FUNCTIONW__);
      }
   }
   else {
      wprintf(L"[%ws] Failed getting service status: 0x%08X\n",
         __FUNCTIONW__,
         GetLastError());
      goto Cleanup;
   }

   if (DeleteService(hSCService)) {
      wprintf(L"[%ws] Service %ws is removed.\n",
         __FUNCTIONW__,
         pServiceName);
   }
   else {
      wprintf(L"[%ws] Failed deleting service %ws: 0x%08X\n",
         __FUNCTIONW__,
         pServiceName,
         GetLastError());
   }


Cleanup:
   if (hSCService) {
      CloseServiceHandle(hSCService);
      hSCService = NULL;
   }
   if (hSCManager) {
      CloseServiceHandle(hSCManager);
      hSCManager = NULL;
   }
}

CService *g_pService = NULL;

int wmain(int argc, wchar_t **argv)
{
   int      iRet = 0;
   wchar_t  *ptr = NULL;

   if (argc > 1) {
      if (argv[1][0] == L'-' || argv[1][0] == L'/') {
         ptr = argv[1];
         ptr++;
         if (_wcsicmp(L"install", ptr) == 0) {
            wprintf(L"Attempting to install the service\n");
            /*
            * Install service if -install or /install option is used
            */
            InstallService(
               H3_SERVICE_NAME,               // Name of service
               H3_SERVICE_DISPLAY_NAME,       // Name to display
               H3_SERVICE_START_TYPE,         // Service start type
               H3_SERVICE_DEPENDENCIES,       // Dependencies
               H3_DEFAULT_SERVICE_ACCOUNT,    // Service running account
               H3_DEFAULT_SERVICE_PASSWORD);  // Password of the account
         }
         else if (_wcsicmp(L"remove", ptr) == 0 ||
            _wcsicmp(L"uninstall", ptr) == 0 ||
            _wcsicmp(L"delete", ptr) == 0) {

            /*
            * Uninstall service if -remove or /remove option is used
            */
            UninstallService(H3_SERVICE_NAME);
         }
      }
   }
   else {
      try {
         CService svc(H3_SERVICE_NAME, H3_LOG_FILE);

         g_pService = &svc;
         if (!CService::Run(svc)) {
            wprintf(L"Failed running the service: %u", GetLastError());
         }
      }
      catch (CWatchExp &exp) {
         wprintf(exp.GetErrorMessage());
      }
   }
   return iRet;
}
