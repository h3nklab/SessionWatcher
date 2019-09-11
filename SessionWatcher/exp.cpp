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