
// RTSSSharedMemorySampleDlg.cpp : implementation file
//
// created by Unwinder
/////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "RTSSSharedMemory.h"
#include "RTSSCoreControl.h"
#include "GroupedString.h"
/////////////////////////////////////////////////////////////////////////////
#include <shlwapi.h>
#include <float.h>
#include <io.h>

/////////////////////////////////////////////////////////////////////////////
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

RTSSCoreControl::RTSSCoreControl()
{
  m_strInstallPath = "";

  m_bMultiLineOutput = TRUE;
  m_bFormatTags = TRUE;
  m_bFillGraphs = FALSE;
  m_bConnected = FALSE;
}

/////////////////////////////////////////////////////////////////////////////
DWORD RTSSCoreControl::GetSharedMemoryVersion()
{
  DWORD dwResult = 0;

  HANDLE hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, "RTSSSharedMemoryV2");

  if (hMapFile)
  {
    LPVOID pMapAddr = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, 0);
    LPRTSS_SHARED_MEMORY pMem = (LPRTSS_SHARED_MEMORY)pMapAddr;

    if (pMem)
    {
      if ((pMem->dwSignature == 'RTSS') &&
        (pMem->dwVersion >= 0x00020000))
        dwResult = pMem->dwVersion;

      UnmapViewOfFile(pMapAddr);
    }

    CloseHandle(hMapFile);
  }

  return dwResult;
}
/////////////////////////////////////////////////////////////////////////////
DWORD RTSSCoreControl::EmbedGraph(DWORD dwOffset, FLOAT* lpBuffer, DWORD dwBufferPos, DWORD dwBufferSize, LONG dwWidth, LONG dwHeight, LONG dwMargin, FLOAT fltMin, FLOAT fltMax, DWORD dwFlags)
{
  DWORD dwResult = 0;

  HANDLE hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, "RTSSSharedMemoryV2");

  if (hMapFile)
  {
    LPVOID pMapAddr = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, 0);
    LPRTSS_SHARED_MEMORY pMem = (LPRTSS_SHARED_MEMORY)pMapAddr;

    if (pMem)
    {
      if ((pMem->dwSignature == 'RTSS') &&
        (pMem->dwVersion >= 0x00020000))
      {
        for (DWORD dwPass = 0; dwPass < 2; dwPass++)
          //1st pass : find previously captured OSD slot
          //2nd pass : otherwise find the first unused OSD slot and capture it
        {
          for (DWORD dwEntry = 1; dwEntry < pMem->dwOSDArrSize; dwEntry++)
            //allow primary OSD clients (i.e. EVGA Precision / MSI Afterburner) to use the first slot exclusively, so third party
            //applications start scanning the slots from the second one
          {
            RTSS_SHARED_MEMORY::LPRTSS_SHARED_MEMORY_OSD_ENTRY pEntry = (RTSS_SHARED_MEMORY::LPRTSS_SHARED_MEMORY_OSD_ENTRY)((LPBYTE)pMem + pMem->dwOSDArrOffset + dwEntry * pMem->dwOSDEntrySize);

            if (dwPass)
            {
              if (!strlen(pEntry->szOSDOwner))
                strcpy_s(pEntry->szOSDOwner, sizeof(pEntry->szOSDOwner), "RTSSSharedMemorySample");
            }

            if (!strcmp(pEntry->szOSDOwner, "RTSSSharedMemorySample"))
            {
              if (pMem->dwVersion >= 0x0002000c)
                //embedded graphs are supported for v2.12 and higher shared memory
              {
                if (dwOffset + sizeof(RTSS_EMBEDDED_OBJECT_GRAPH) + dwBufferSize * sizeof(FLOAT) > sizeof(pEntry->buffer))
                  //validate embedded object offset and size and ensure that we don't overrun the buffer
                {
                  UnmapViewOfFile(pMapAddr);

                  CloseHandle(hMapFile);

                  return 0;
                }

                LPRTSS_EMBEDDED_OBJECT_GRAPH lpGraph = (LPRTSS_EMBEDDED_OBJECT_GRAPH)(pEntry->buffer + dwOffset);
                //get pointer to object in buffer

                lpGraph->header.dwSignature = RTSS_EMBEDDED_OBJECT_GRAPH_SIGNATURE;
                lpGraph->header.dwSize = sizeof(RTSS_EMBEDDED_OBJECT_GRAPH) + dwBufferSize * sizeof(FLOAT);
                lpGraph->header.dwWidth = dwWidth;
                lpGraph->header.dwHeight = dwHeight;
                lpGraph->header.dwMargin = dwMargin;
                lpGraph->dwFlags = dwFlags;
                lpGraph->fltMin = fltMin;
                lpGraph->fltMax = fltMax;
                lpGraph->dwDataCount = dwBufferSize;

                if (lpBuffer && dwBufferSize)
                {
                  for (DWORD dwPos = 0; dwPos < dwBufferSize; dwPos++)
                  {
                    FLOAT fltData = lpBuffer[dwBufferPos];

                    lpGraph->fltData[dwPos] = (fltData == FLT_MAX) ? 0 : fltData;

                    dwBufferPos = (dwBufferPos + 1) & (dwBufferSize - 1);
                  }
                }

                dwResult = lpGraph->header.dwSize;
              }

              break;
            }
          }

          if (dwResult)
            break;
        }
      }

      UnmapViewOfFile(pMapAddr);
    }

    CloseHandle(hMapFile);
  }

  return dwResult;
}
/////////////////////////////////////////////////////////////////////////////
BOOL RTSSCoreControl::UpdateOSD(LPCSTR lpText)
{
  BOOL bResult = FALSE;

  HANDLE hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, "RTSSSharedMemoryV2");

  if (hMapFile)
  {
    LPVOID pMapAddr = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, 0);
    LPRTSS_SHARED_MEMORY pMem = (LPRTSS_SHARED_MEMORY)pMapAddr;

    if (pMem)
    {
      if ((pMem->dwSignature == 'RTSS') &&
        (pMem->dwVersion >= 0x00020000))
      {
        for (DWORD dwPass = 0; dwPass < 2; dwPass++)
          //1st pass : find previously captured OSD slot
          //2nd pass : otherwise find the first unused OSD slot and capture it
        {
          for (DWORD dwEntry = 1; dwEntry < pMem->dwOSDArrSize; dwEntry++)
            //allow primary OSD clients (i.e. EVGA Precision / MSI Afterburner) to use the first slot exclusively, so third party
            //applications start scanning the slots from the second one
          {
            RTSS_SHARED_MEMORY::LPRTSS_SHARED_MEMORY_OSD_ENTRY pEntry = 
              (RTSS_SHARED_MEMORY::LPRTSS_SHARED_MEMORY_OSD_ENTRY)((LPBYTE)pMem + pMem->dwOSDArrOffset + dwEntry * pMem->dwOSDEntrySize);

            if (dwPass)
            {
              if (!strlen(pEntry->szOSDOwner))
                strcpy_s(pEntry->szOSDOwner, sizeof(pEntry->szOSDOwner), "RTSSSharedMemorySample");
            }

            if (!strcmp(pEntry->szOSDOwner, "RTSSSharedMemorySample"))
            {
              if (pMem->dwVersion >= 0x00020007)
                //use extended text slot for v2.7 and higher shared memory, it allows displaying 4096 symbols
                //instead of 256 for regular text slot
              {
                if (pMem->dwVersion >= 0x0002000e)
                  //OSD locking is supported on v2.14 and higher shared memory
                {
                  DWORD dwBusy = _interlockedbittestandset(&pMem->dwBusy, 0);
                  //bit 0 of this variable will be set if OSD is locked by renderer and cannot be refreshed
                  //at the moment

                  if (!dwBusy)
                  {
                    strncpy_s(pEntry->szOSDEx, sizeof(pEntry->szOSDEx), lpText, sizeof(pEntry->szOSDEx) - 1);

                    pMem->dwBusy = 0;
                  }
                }
                else
                  strncpy_s(pEntry->szOSDEx, sizeof(pEntry->szOSDEx), lpText, sizeof(pEntry->szOSDEx) - 1);

              }
              else
                strncpy_s(pEntry->szOSD, sizeof(pEntry->szOSD), lpText, sizeof(pEntry->szOSD) - 1);

              pMem->dwOSDFrame++;

              bResult = TRUE;

              break;
            }
          }

          if (bResult)
            break;
        }
      }

      UnmapViewOfFile(pMapAddr);
    }

    CloseHandle(hMapFile);
  }

  return bResult;
}
/////////////////////////////////////////////////////////////////////////////
void RTSSCoreControl::ReleaseOSD()
{
  HANDLE hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, "RTSSSharedMemoryV2");

  if (hMapFile)
  {
    LPVOID pMapAddr = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, 0);

    LPRTSS_SHARED_MEMORY pMem = (LPRTSS_SHARED_MEMORY)pMapAddr;

    if (pMem)
    {
      if ((pMem->dwSignature == 'RTSS') &&
        (pMem->dwVersion >= 0x00020000))
      {
        for (DWORD dwEntry = 1; dwEntry < pMem->dwOSDArrSize; dwEntry++)
        {
          RTSS_SHARED_MEMORY::LPRTSS_SHARED_MEMORY_OSD_ENTRY pEntry = (RTSS_SHARED_MEMORY::LPRTSS_SHARED_MEMORY_OSD_ENTRY)((LPBYTE)pMem + pMem->dwOSDArrOffset + dwEntry * pMem->dwOSDEntrySize);

          if (!strcmp(pEntry->szOSDOwner, "RTSSSharedMemorySample"))
          {
            memset(pEntry, 0, pMem->dwOSDEntrySize);
            pMem->dwOSDFrame++;
          }
        }
      }

      UnmapViewOfFile(pMapAddr);
    }

    CloseHandle(hMapFile);
  }
}
/////////////////////////////////////////////////////////////////////////////
DWORD RTSSCoreControl::GetClientsNum()
{
  DWORD dwClients = 0;

  HANDLE hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, "RTSSSharedMemoryV2");

  if (hMapFile)
  {
    LPVOID pMapAddr = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, 0);

    LPRTSS_SHARED_MEMORY pMem = (LPRTSS_SHARED_MEMORY)pMapAddr;

    if (pMem)
    {
      if ((pMem->dwSignature == 'RTSS') &&
        (pMem->dwVersion >= 0x00020000))
      {
        for (DWORD dwEntry = 0; dwEntry < pMem->dwOSDArrSize; dwEntry++)
        {
          RTSS_SHARED_MEMORY::LPRTSS_SHARED_MEMORY_OSD_ENTRY pEntry = (RTSS_SHARED_MEMORY::LPRTSS_SHARED_MEMORY_OSD_ENTRY)((LPBYTE)pMem + pMem->dwOSDArrOffset + dwEntry * pMem->dwOSDEntrySize);

          if (strlen(pEntry->szOSDOwner))
            dwClients++;
        }
      }

      UnmapViewOfFile(pMapAddr);
    }

    CloseHandle(hMapFile);
  }

  return dwClients;
}

void RTSSCoreControl::Refresh()
{
  //init RivaTuner Statistics Server installation path

  if (m_strInstallPath.IsEmpty())
  {
    HKEY hKey;

    if (ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE, "Software\\Unwinder\\RTSS", &hKey))
    {
      char buf[MAX_PATH];

      DWORD dwSize = MAX_PATH;
      DWORD dwType;

      if (ERROR_SUCCESS == RegQueryValueEx(hKey, "InstallPath", 0, &dwType, (LPBYTE)buf, &dwSize))
      {
        if (dwType == REG_SZ)
          m_strInstallPath = buf;
      }

      RegCloseKey(hKey);
    }
  }

  //validate RivaTuner Statistics Server installation path

  if (_taccess(m_strInstallPath, 0))
    m_strInstallPath = "";

  //init profile interface 

  if (!m_strInstallPath.IsEmpty())
  {
    if (!m_profileInterface.IsInitialized())
      m_profileInterface.Init(m_strInstallPath);
  }

  //init shared memory version

  DWORD dwSharedMemoryVersion = GetSharedMemoryVersion();

  //init max OSD text size, we'll use extended text slot for v2.7 and higher shared memory, 
  //it allows displaying 4096 symbols /instead of 256 for regular text slot

  DWORD dwMaxTextSize = (dwSharedMemoryVersion >= 0x00020007) ? sizeof(RTSS_SHARED_MEMORY::RTSS_SHARED_MEMORY_OSD_ENTRY().szOSDEx) 
    : sizeof(RTSS_SHARED_MEMORY::RTSS_SHARED_MEMORY_OSD_ENTRY().szOSD);

  CGroupedString groupedString(dwMaxTextSize - 1);
  // RivaTuner based products use similar CGroupedString object for convenient OSD text formatting and length control
  // You may use it to format your OSD similar to RivaTuner's one or just use your own routines to format OSD text

  BOOL bFormatTagsSupported = (dwSharedMemoryVersion >= 0x0002000b);
  //text format tags are supported for shared memory v2.11 and higher
  BOOL bObjTagsSupported = (dwSharedMemoryVersion >= 0x0002000c);
  //embedded object tags are supporoted for shared memory v2.12 and higher

  CString strOSD;

  if (bFormatTagsSupported && m_bFormatTags)
  {
    if (GetClientsNum() == 1)
      strOSD += "<P=0,10>";
    //move to position 0,10 (in zoomed pixel units)

    //Note: take a note that position is specified in absolute coordinates so use this tag with caution because your text may
    //overlap with text slots displayed by other applications, so in this demo we explicitly disable this tag usage if more than
    //one client is currently rendering something in OSD

    strOSD += "<A0=-5>";
    //define align variable A[0] as right alignment by 5 symbols (positive is left, negative is right)
    strOSD += "<A1=4>";
    //define align variable A[1] as left alignment by 4 symbols (positive is left, negative is right)
    strOSD += "<C0=FFA0A0>";
    //define color variable C[0] as R=FF,G=A0 and B=A0
    strOSD += "<C1=FF00A0>";
    //define color variable C[1] as R=FF,G=00 and B=A0
    strOSD += "<C2=FFFFFF>";
    //define color variable C[1] as R=FF,G=FF and B=FF
    strOSD += "<S0=-50>";
    //define size variable S[0] as 50% subscript (positive is superscript, negative is subscript)
    strOSD += "<S1=50>";
    //define size variable S[0] as 50% supercript (positive is superscript, negative is subscript)

    strOSD += "\r";
    //add \r just for this demo to make tagged text more readable in demo preview window, OSD ignores \r anyway

  //Note: we could apply explicit alignment,size and color definitions when necerrary (e.g. <C=FFFFFF>, however
  //variables usage makes tagged text more compact and readable
  }
  else
    strOSD = "";

  // Add CX label
  groupedString.Add("CX OSD", "", "\n", " ");

  if (ShowCaptureTimer)
  {
    CString CaptureTimerValueStr;
    CaptureTimerValueStr.Format("%d s", CaptureTimerValue);
      
    groupedString.Add(CaptureTimerValueStr, "Capture timer ", "\n", " ");
  }

  if (bFormatTagsSupported && m_bFormatTags)
  {
    groupedString.Add("<A0><FR><A><A1><S1> FPS<S><A>", "<C2><APP><C>", "\n", m_bFormatTags ? " " : ", ");
    groupedString.Add("<A0><FT><A><A1><S1> ms<S><A>", "<C2><APP><C>", "\n", m_bFormatTags ? " " : ", ");
    //print application-specific 3D API, framerate and frametime using tags
  }
  else
  {
    groupedString.Add("%FRAMERATE%", "", "\n");
    groupedString.Add("%FRAMETIME%", "", "\n");
    //print application-specific 3D API, framerate and frametime using deprecated macro
  }

  BOOL bTruncated = FALSE;

  strOSD += groupedString.Get(bTruncated, FALSE, m_bFormatTags ? "\t" : " \t: ");

  if (!strOSD.IsEmpty())
  {
    BOOL bResult = UpdateOSD(strOSD);;
    m_bConnected = bResult;
  }
}

/////////////////////////////////////////////////////////////////////////////
void RTSSCoreControl::IncProfileProperty(LPCSTR lpProfile, LPCSTR lpProfileProperty, LONG dwIncrement)
{
  if (m_profileInterface.IsInitialized())
  {
    m_profileInterface.LoadProfile(lpProfile);

    LONG dwProperty = 0;

    if (m_profileInterface.GetProfileProperty(lpProfileProperty, (LPBYTE)&dwProperty, sizeof(dwProperty)))
    {
      dwProperty += dwIncrement;

      m_profileInterface.SetProfileProperty(lpProfileProperty, (LPBYTE)&dwProperty, sizeof(dwProperty));
      m_profileInterface.SaveProfile(lpProfile);
      m_profileInterface.UpdateProfiles();
    }
  }
}
/////////////////////////////////////////////////////////////////////////////
void RTSSCoreControl::SetProfileProperty(LPCSTR lpProfile, LPCSTR lpProfileProperty, DWORD dwProperty)
{
  if (m_profileInterface.IsInitialized())
  {
    m_profileInterface.LoadProfile(lpProfile);
    m_profileInterface.SetProfileProperty(lpProfileProperty, (LPBYTE)&dwProperty, sizeof(dwProperty));
    m_profileInterface.SaveProfile(lpProfile);
    m_profileInterface.UpdateProfiles();
  }
}
/////////////////////////////////////////////////////////////////////////////
