#pragma once

class CWatchExp
{
public:
   CWatchExp(
      _In_ DWORD dwError,
      _In_ wchar_t *pMessage);

   virtual
   ~CWatchExp();

   DWORD
   GetError(
      void)
   {
      return m_dwError;
   }

   wchar_t *
   GetErrorMessage(
      void)
   {
      return m_sError;
   }

protected:
   DWORD    m_dwError;
   wchar_t  m_sError[512];
};