#include "pch.h"
#include "exp.h"

CWatchExp::CWatchExp(
   _In_ DWORD dwError,
   _In_ wchar_t *pMessage)
{
   m_dwError = dwError;
   wcscpy_s(m_sError, sizeof(m_sError) / sizeof(wchar_t), pMessage);
}

CWatchExp::~CWatchExp()
{
}