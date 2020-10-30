//#include "stdafx.h"
#include "HardDriveSerialNumer.h"
#include <iostream>
#if defined(WIN64) || defined(WIN32)
#include <comdef.h>
#include <Wbemidl.h>
# pragma comment(lib, "wbemuuid.lib")
#include <atlbase.h>
#include <atlconv.h>
#include "iphlpapi.h"
#pragma comment ( lib, "Iphlpapi.lib" )
#pragma warning(disable: 4996)
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
//#include "base/fw/util/util.h"
//#include "base/fw/logref.h"



#define DEFAULT_CODE_PAGE 936

std::wstring ANSI2Unicode(const std::string &src) {
    auto lpszSrc = src.c_str();
    std::wstring sResult;
    int nUnicodeLen = MultiByteToWideChar( DEFAULT_CODE_PAGE, 0, lpszSrc, -1, NULL, 0 );
    LPWSTR pUnicode = new WCHAR[nUnicodeLen + 1];
    if( pUnicode != NULL ) {
        ZeroMemory( ( void* )pUnicode, ( nUnicodeLen + 1 ) * sizeof( WCHAR ) );
        MultiByteToWideChar( DEFAULT_CODE_PAGE, 0, lpszSrc, -1, pUnicode, nUnicodeLen );
        sResult = pUnicode;
        delete[] pUnicode;
    }
    return sResult;
}

void MasterHardDiskSerial::trims(char* data)           //去掉字符串中的空格
{
	int i = -1, j = 0;
	int ch = ' ';

	while (data[++i] != '\0')
	{
		if (data[i] != ch)
		{
			data[j++] = data[i];
		}
	}
	data[j] = '\0';
}
int MasterHardDiskSerial::readPhysicalDriveInNTWithAdminRights(void)
{
	//printf("ReadPhysicalDriveInNTWithAdminRights start\n");
	int iDone = FALSE;
	std::vector<int> vecSysDriveID;
	int iDrive = getSysDriveID(vecSysDriveID);

	for (std::size_t i = 0; i < vecSysDriveID.size(); i++)
	{
		iDrive = vecSysDriveID.at(i);
		//printf("ReadPhysicalDriveInNTWithAdminRights get %d drive\n", iDrive);
		HANDLE hPhysicalDriveIOCTL = 0;

		//  Try to get a handle to PhysicalDrive IOCTL, report failure
		//  and exit if can't.
		char cszDriveName[256];

		sprintf_s(cszDriveName, 256, "\\\\.\\PhysicalDrive%d", iDrive);

		//  Windows NT, Windows 2000, must have admin rights
		hPhysicalDriveIOCTL = CreateFileA(cszDriveName,
			GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
			OPEN_EXISTING, 0, NULL);
		if (hPhysicalDriveIOCTL == INVALID_HANDLE_VALUE)
		{
			SecureZeroMemory(m_cszErrorMessage, sizeof(m_cszErrorMessage));
			sprintf_s(m_cszErrorMessage, 256, "%d ReadPhysicalDriveInNTWithAdminRights ERROR ,CreateFileA(%s) returned INVALID_HANDLE_VALUE", __LINE__, cszDriveName);
			//printf("ReadPhysicalDriveInNTWithAdminRights get %d drive error: %s\n", iDrive,m_cszErrorMessage);
		}
		else
		{
			GETVERSIONOUTPARAMS VersionParams;
			DWORD               dwBytesReturned = 0;

			// Get the version, etc of PhysicalDrive IOCTL
			memset((void*)&VersionParams, 0, sizeof(VersionParams));

			if (!DeviceIoControl(hPhysicalDriveIOCTL, DFP_GET_VERSION,
				NULL,
				0,
				&VersionParams,
				sizeof(VersionParams),
				&dwBytesReturned, NULL))
			{

				DWORD dwErr = GetLastError();
				SecureZeroMemory(m_cszErrorMessage, sizeof(m_cszErrorMessage));
				sprintf_s(m_cszErrorMessage, 256, "%d ReadPhysicalDriveInNTWithAdminRights ERROR DeviceIoControl() %d, DFP_GET_VERSION) returned 0, error is %d\n", __LINE__, (int)hPhysicalDriveIOCTL, (int)dwErr);
				//printf("ReadPhysicalDriveInNTWithAdminRights get %d drive error: %s\n", iDrive, m_cszErrorMessage);
			}

			// If there is a IDE device at number "iI" issue commands
			// to the device
			if (VersionParams.bIDEDeviceMap <= 0)
			{
				SecureZeroMemory(m_cszErrorMessage, sizeof(m_cszErrorMessage));
				sprintf_s(m_cszErrorMessage, 256, "%d ReadPhysicalDriveInNTWithAdminRights ERROR No device found at iPosition %d (%d)", __LINE__, (int)iDrive, (int)VersionParams.bIDEDeviceMap);
				//printf("ReadPhysicalDriveInNTWithAdminRights get %d drive error: %s\n", iDrive, m_cszErrorMessage);
			}
			else
			{
				BYTE             bIDCmd = 0;   // IDE or ATAPI IDENTIFY cmd
				SENDCMDINPARAMS  scip;
				//SENDCMDOUTPARAMS OutCmd;

				// Now, get the ID sector for all IDE devices in the system.
				// If the device is ATAPI use the IDE_ATAPI_IDENTIFY command,
				// otherwise use the IDE_ATA_IDENTIFY command
				bIDCmd = (VersionParams.bIDEDeviceMap >> iDrive & 0x10) ? \
					IDE_ATAPI_IDENTIFY : IDE_ATA_IDENTIFY;

				memset(&scip, 0, sizeof(scip));
				memset(byIdOutCmd, 0, sizeof(byIdOutCmd));

				if (doIDENTIFY(hPhysicalDriveIOCTL,
					&scip,
					(PSENDCMDOUTPARAMS)&byIdOutCmd,
					(BYTE)bIDCmd,
					(BYTE)iDrive,
					&dwBytesReturned))
				{
					DWORD dwDiskData[256];
					int iIjk = 0;
					USHORT *punIdSector = (USHORT *)
						((PSENDCMDOUTPARAMS)byIdOutCmd)->bBuffer;

					for (iIjk = 0; iIjk < 256; iIjk++)
						dwDiskData[iIjk] = punIdSector[iIjk];

					printIdeInfo(iDrive, dwDiskData);

					iDone = TRUE;
					//printf("ReadPhysicalDriveInNTWithAdminRights get %d drive success:%s\n", iDrive, m_cszHardDriveSerialNumber);
				}
			}

			CloseHandle(hPhysicalDriveIOCTL);
		}
	}

	return iDone;
}

int MasterHardDiskSerial::readPhysicalDriveInNTUsingSmart(void)
{
	//printf("ReadPhysicalDriveInNTUsingSmart start\n");
	int iDone = FALSE;
	std::vector<int> vecSysDriveID;
	int iDrive = getSysDriveID(vecSysDriveID);

	for (std::size_t i = 0; i < vecSysDriveID.size(); i++)
	{
		iDrive = vecSysDriveID.at(i);
		HANDLE hPhysicalDriveIOCTL = 0;

		//  Try to get a handle to PhysicalDrive IOCTL, report failure
		//  and exit if can't.
		char cszDriveName[256];

		sprintf_s(cszDriveName, 256, "\\\\.\\PhysicalDrive%d", iDrive);

		//  Windows NT, Windows 2000, Windows Server 2003, Vista
		hPhysicalDriveIOCTL = CreateFileA(cszDriveName,
			GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL, OPEN_EXISTING, 0, NULL);
		// if (hPhysicalDriveIOCTL == INVALID_HANDLE_VALUE)
		//    printf ("Unable to open physical iDrive %d, error code: 0x%lX\n",
		//            iDrive, GetLastError ());

		if (hPhysicalDriveIOCTL == INVALID_HANDLE_VALUE)
		{
			SecureZeroMemory(m_cszErrorMessage, sizeof(m_cszErrorMessage));
			sprintf_s(m_cszErrorMessage, 256, "%d ReadPhysicalDriveInNTUsingSmart ERROR, CreateFileA(%s) returned INVALID_HANDLE_VALUE Error Code %d", __LINE__, cszDriveName, GetLastError());
			//printf("ReadPhysicalDriveInNTUsingSmart get %d drive error: %s\n", iDrive, m_cszErrorMessage);
		}
		else
		{
			GETVERSIONINPARAMS GetVersionParams;
			DWORD dwBytesReturned = 0;

			// Get the version, etc of PhysicalDrive IOCTL
			memset((void*)& GetVersionParams, 0, sizeof(GetVersionParams));

			if (!DeviceIoControl(hPhysicalDriveIOCTL, SMART_GET_VERSION,
				NULL,
				0,
				&GetVersionParams, sizeof(GETVERSIONINPARAMS),
				&dwBytesReturned, NULL))
			{
				DWORD dwErr = GetLastError();
				SecureZeroMemory(m_cszErrorMessage, sizeof(m_cszErrorMessage));
				sprintf_s(m_cszErrorMessage, 256, "\n%d ReadPhysicalDriveInNTUsingSmart ERROR DeviceIoControl(%d, SMART_GET_VERSION) returned 0, error is %d", __LINE__, (int)hPhysicalDriveIOCTL, (int)dwErr);
				//printf("ReadPhysicalDriveInNTUsingSmart get %d drive error: %s\n", iDrive, m_cszErrorMessage);
			}
			else
			{
				// Print the SMART version
				// PrintVersion (& GetVersionParams);
				// Allocate the command cszBuffer
				ULONG CommandSize = sizeof(SENDCMDINPARAMS) + IDENTIFY_BUFFER_SIZE;
				PSENDCMDINPARAMS Command = (PSENDCMDINPARAMS)malloc(CommandSize);
				// Retrieve the IDENTIFY data
				// Prepare the command
#define ID_CMD          0xEC            // Returns ID sector for ATA
				Command->irDriveRegs.bCommandReg = ID_CMD;
				DWORD BytesReturned = 0;
				if (!DeviceIoControl(hPhysicalDriveIOCTL,
					SMART_RCV_DRIVE_DATA, Command, sizeof(SENDCMDINPARAMS),
					Command, CommandSize,
					&BytesReturned, NULL))
				{
					SecureZeroMemory(m_cszErrorMessage, sizeof(m_cszErrorMessage));
					sprintf_s(m_cszErrorMessage, 256, "SMART_RCV_DRIVE_DATA IOCTL");
					//printf("ReadPhysicalDriveInNTUsingSmart get %d drive error: %s\n", iDrive, m_cszErrorMessage);
					// Print the error
					//PrintError ("SMART_RCV_DRIVE_DATA IOCTL", GetLastError());
				}
				else
				{
					// Print the IDENTIFY data
					DWORD dwDiskData[256];
					USHORT *punIdSector = (USHORT *)
						(PIDENTIFY_DATA)((PSENDCMDOUTPARAMS)Command)->bBuffer;

					for (int iIjk = 0; iIjk < 256; iIjk++)
						dwDiskData[iIjk] = punIdSector[iIjk];

					printIdeInfo(iDrive, dwDiskData);
					iDone = TRUE;
					//printf("ReadPhysicalDriveInNTUsingSmart get %d drive success:%s\n", iDrive, m_cszHardDriveSerialNumber);
				}
				// Done
				CloseHandle(hPhysicalDriveIOCTL);
				free(Command);
			}
		}
	}
	//printf("ReadPhysicalDriveInNTUsingSmart end\n");
	return iDone;
}

char * MasterHardDiskSerial::flipAndCodeBytes(int iPos, int iFlip, const char * pcszStr, char * pcszBuf)
{
	int iI;
	int iJ = 0;
	int iK = 0;

	pcszBuf[0] = '\0';
	if (iPos <= 0)
		return pcszBuf;

	if (!iJ)
	{
		char cP = 0;
		// First try to gather all characters representing hex digits only.
		iJ = 1;
		iK = 0;
		pcszBuf[iK] = 0;
		for (iI = iPos; iJ && !(pcszStr[iI] == '\0'); ++iI)
		{
			char cC = tolower(pcszStr[iI]);
			if (isspace(cC))
				cC = '0';
			++cP;
			pcszBuf[iK] <<= 4;

			if (cC >= '0' && cC <= '9')
				pcszBuf[iK] |= (char)(cC - '0');
			else if (cC >= 'a' && cC <= 'f')
				pcszBuf[iK] |= (char)(cC - 'a' + 10);
			else
			{
				iJ = 0;
				break;
			}

			if (cP == 2)
			{
				if (pcszBuf[iK] < 0) {
					iJ = 0;
					break;
				}
				if ((pcszBuf[iK] != '\0') && !isprint(pcszBuf[iK]))
				{
					iJ = 0;
					break;
				}
				++iK;
				cP = 0;
				pcszBuf[iK] = 0;
			}

		}
	}

	if (!iJ)
	{
		// There are non-digit characters, gather them as is.
		iJ = 1;
		iK = 0;
		for (iI = iPos; iJ && (pcszStr[iI] != '\0'); ++iI)
		{
			char cC = pcszStr[iI];

			if (!isprint(cC))
			{
				iJ = 0;
				break;
			}

			pcszBuf[iK++] = cC;
		}
	}

	if (!iJ)
	{
		// The characters are not there or are not printable.
		iK = 0;
	}

	pcszBuf[iK] = '\0';

	if (iFlip)
		// Flip adjacent characters
		for (iJ = 0; iJ < iK; iJ += 2)
		{
			char t = pcszBuf[iJ];
			pcszBuf[iJ] = pcszBuf[iJ + 1];
			pcszBuf[iJ + 1] = t;
		}

	// Trim any beginning and end space
	iI = iJ = -1;
	for (iK = 0; (pcszBuf[iK] != '\0'); ++iK)
	{
		if (!isspace(pcszBuf[iK]))
		{
			if (iI < 0)
				iI = iK;
			iJ = iK;
		}
	}

	if ((iI >= 0) && (iJ >= 0))
	{
		for (iK = iI; (iK <= iJ) && (pcszBuf[iK] != '\0'); ++iK)
			pcszBuf[iK - iI] = pcszBuf[iK];
		pcszBuf[iK - iI] = '\0';
	}

	return pcszBuf;
}

int MasterHardDiskSerial::readPhysicalDriveInNTWithZeroRights(void)
{
	//printf("ReadPhysicalDriveInNTWithZeroRights start\n");
	int iDone = FALSE;
	std::vector<int> vecSysDriveID;
	int iDrive = getSysDriveID(vecSysDriveID);

	for (std::size_t i = 0; i < vecSysDriveID.size(); i++)
	{
		iDrive = vecSysDriveID.at(i);
		HANDLE hPhysicalDriveIOCTL = 0;

		//  Try to get a handle to PhysicalDrive IOCTL, report failure
		//  and exit if can't.
		char cszDriveName[256];

		sprintf_s(cszDriveName, 256, "\\\\.\\PhysicalDrive%d", iDrive);

		//  Windows NT, Windows 2000, Windows XP - admin rights not required
		hPhysicalDriveIOCTL = CreateFileA(cszDriveName, 0,
			FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
			OPEN_EXISTING, 0, NULL);
		if (hPhysicalDriveIOCTL == INVALID_HANDLE_VALUE)
		{
			SecureZeroMemory(m_cszErrorMessage, sizeof(m_cszErrorMessage));
			sprintf_s(m_cszErrorMessage, 256, "%d ReadPhysicalDriveInNTWithZeroRights ERROR CreateFileA(%s) returned INVALID_HANDLE_VALUE", __LINE__, cszDriveName);
			//printf("ReadPhysicalDriveInNTWithZeroRights get %d drive error: %s\n", iDrive, m_cszErrorMessage);
		}
		else
		{
			STORAGE_PROPERTY_QUERY query;
			DWORD dwBytesReturned = 0;
			char cszBuffer[10000] = { 0 };

			memset((void *)& query, 0, sizeof(query));
			query.PropertyId = StorageDeviceProperty;
			query.QueryType = PropertyStandardQuery;

			memset(cszBuffer, 0, sizeof(cszBuffer));

			if (DeviceIoControl(hPhysicalDriveIOCTL, IOCTL_STORAGE_QUERY_PROPERTY,
				&query,
				sizeof(query),
				&cszBuffer,
				sizeof(cszBuffer),
				&dwBytesReturned, NULL))
			{
				STORAGE_DEVICE_DESCRIPTOR * descrip = (STORAGE_DEVICE_DESCRIPTOR *)& cszBuffer;
				char cszSerialNumber[1000] = { 0 };

				flipAndCodeBytes(descrip->SerialNumberOffset,0, cszBuffer, cszSerialNumber);
				trims(cszSerialNumber);

				if (/*0 == m_cszHardDriveSerialNumber[0] &&*/
					//  serial number must be alphanumeric
					//  (but there can be leading spaces on IBM drives)
					(iswalnum(cszSerialNumber[0]) || iswalnum(cszSerialNumber[19])))
				{
					strcpy_s(m_cszHardDriveSerialNumber, 1024, cszSerialNumber);
					std::string strTmpDsn = m_cszHardDriveSerialNumber;
					//在虚拟机中，硬盘的序列号可能为空，或者为0000000000000001之类的形式
                    if (strTmpDsn.empty() || strTmpDsn.find("00000000") != string::npos) {
                        continue;
                    }
					m_vecSysDirveSerNum.push_back(m_cszHardDriveSerialNumber);
					iDone = TRUE;
				
					//printf("ReadPhysicalDriveInNTWithZeroRights get %d drive success:%s\n", iDrive, m_cszHardDriveSerialNumber);
				}
				else {
					continue;
				}
				// Get the disk iDrive geometry.
				memset(cszBuffer, 0, sizeof(cszBuffer));
				if (!DeviceIoControl(hPhysicalDriveIOCTL,
					IOCTL_DISK_GET_DRIVE_GEOMETRY_EX,
					NULL,
					0,
					&cszBuffer,
					sizeof(cszBuffer),
					&dwBytesReturned,
					NULL))
				{
					SecureZeroMemory(m_cszErrorMessage, sizeof(m_cszErrorMessage));
					sprintf_s(m_cszErrorMessage, "%s ReadPhysicalDriveInNTWithZeroRights ERROR DeviceIoControl(), IOCTL_DISK_GET_DRIVE_GEOMETRY_EX) returned 0", cszDriveName);
					//printf("ReadPhysicalDriveInNTWithZeroRights get %d drive error: %s\n", iDrive, m_cszErrorMessage);
				}
				else
				{
					DISK_GEOMETRY_EX* geom = (DISK_GEOMETRY_EX*)&cszBuffer;
					int iFixed = (geom->Geometry.MediaType == FixedMedia);
					__int64 i64Size = geom->DiskSize.QuadPart;

				}
			}
			else
			{
				DWORD dwErr = GetLastError();
				SecureZeroMemory(m_cszErrorMessage, sizeof(m_cszErrorMessage));
				sprintf_s(m_cszErrorMessage, "DeviceIOControl IOCTL_STORAGE_QUERY_PROPERTY error = %d\n", dwErr);
				//printf("ReadPhysicalDriveInNTWithZeroRights get %d drive error: %s\n", iDrive, m_cszErrorMessage);
			}

			CloseHandle(hPhysicalDriveIOCTL);
		}
	}
	//printf("ReadPhysicalDriveInNTWithZeroRights end\n");
	return iDone;
}

BOOL MasterHardDiskSerial::doIDENTIFY(HANDLE hPhysicalDriveIOCTL, PSENDCMDINPARAMS pSCIP,
	PSENDCMDOUTPARAMS pSCOP, BYTE bIDCmd, BYTE bDriveNum,
	PDWORD lpcbBytesReturned)
{
	// Set up data structures for IDENTIFY command.
	pSCIP->cBufferSize = IDENTIFY_BUFFER_SIZE;
	pSCIP->irDriveRegs.bFeaturesReg = 0;
	pSCIP->irDriveRegs.bSectorCountReg = 1;
	//pSCIP -> irDriveRegs.bSectorNumberReg = 1;
	pSCIP->irDriveRegs.bCylLowReg = 0;
	pSCIP->irDriveRegs.bCylHighReg = 0;

	// Compute the iDrive number.
	pSCIP->irDriveRegs.bDriveHeadReg = 0xA0 | ((bDriveNum & 1) << 4);

	// The command can either be IDE identify or ATAPI identify.
	pSCIP->irDriveRegs.bCommandReg = bIDCmd;
	pSCIP->bDriveNumber = bDriveNum;
	pSCIP->cBufferSize = IDENTIFY_BUFFER_SIZE;

	return (DeviceIoControl(hPhysicalDriveIOCTL, DFP_RECEIVE_DRIVE_DATA,
		(LPVOID)pSCIP,
		sizeof(SENDCMDINPARAMS) - 1,
		(LPVOID)pSCOP,
		sizeof(SENDCMDOUTPARAMS) + IDENTIFY_BUFFER_SIZE - 1,
		lpcbBytesReturned, NULL));
}

int MasterHardDiskSerial::readIdeDriveAsScsiDriveInNT(void)
{
	//printf("ReadIdeDriveAsScsiDriveInNT start\n");
	int iDone = FALSE;
	int iController = 0;

	for (iController = 0; iController < 2; iController++)
	{
		HANDLE hScsiDriveIOCTL = 0;
		char   cszDriveName[256];

		//  Try to get a handle to PhysicalDrive IOCTL, report failure
		//  and exit if can't.
		sprintf_s(cszDriveName, "\\\\.\\Scsi%d:", iController);

		//  Windows NT, Windows 2000, any rights should do
		hScsiDriveIOCTL = CreateFileA(cszDriveName,
			GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
			OPEN_EXISTING, 0, NULL);


		if (hScsiDriveIOCTL != INVALID_HANDLE_VALUE)
		{
			std::vector<int> vecSysDriveID;
			int iDrive = getSysDriveID(vecSysDriveID);
			for (std::size_t i = 0; i < vecSysDriveID.size(); i++)
			{
				iDrive = vecSysDriveID.at(i);
				char cszBuffer[sizeof(SRB_IO_CONTROL) + SENDIDLENGTH];
				SRB_IO_CONTROL *cP = (SRB_IO_CONTROL *)cszBuffer;
				SENDCMDINPARAMS *pin =
					(SENDCMDINPARAMS *)(cszBuffer + sizeof(SRB_IO_CONTROL));
				DWORD dwDummy;

				memset(cszBuffer, 0, sizeof(cszBuffer));
				cP->HeaderLength = sizeof(SRB_IO_CONTROL);
				cP->Timeout = 10000;
				cP->Length = SENDIDLENGTH;
				cP->ControlCode = IOCTL_SCSI_MINIPORT_IDENTIFY;
				strncpy((char *)cP->Signature, "SCSIDISK", 8);

				pin->irDriveRegs.bCommandReg = IDE_ATA_IDENTIFY;
				pin->bDriveNumber = iDrive;

				if (DeviceIoControl(hScsiDriveIOCTL, IOCTL_SCSI_MINIPORT,cszBuffer,sizeof(SRB_IO_CONTROL) +sizeof(SENDCMDINPARAMS) - 1,cszBuffer,sizeof(SRB_IO_CONTROL) + SENDIDLENGTH,&dwDummy, NULL)){
					SENDCMDOUTPARAMS *pOut =
						(SENDCMDOUTPARAMS *)(cszBuffer + sizeof(SRB_IO_CONTROL));
					IDSECTOR *pId = (IDSECTOR *)(pOut->bBuffer);
					if (pId->sModelNumber[0]){
						DWORD dwDiskData[256];
						int iIjk = 0;
						USHORT *punIdSector = (USHORT *)pId;

						for (iIjk = 0; iIjk < 256; iIjk++)
							dwDiskData[iIjk] = punIdSector[iIjk];

						printIdeInfo(iController * 2 + iDrive, dwDiskData);

						iDone = TRUE;
						//printf("ReadIdeDriveAsScsiDriveInNT get %d drive success:%s\n", iDrive,m_cszHardDriveSerialNumber);
					}
				}
				else {
					//printf("ReadIdeDriveAsScsiDriveInNT get %d drive error\n", iDrive);
				}
			}
			CloseHandle(hScsiDriveIOCTL);
		}
		else {
			//printf("ReadIdeDriveAsScsiDriveInNT get %s error\n", cszDriveName);
		}
	}
	//printf("ReadIdeDriveAsScsiDriveInNT end\n");
	return iDone;
}

void MasterHardDiskSerial::printIdeInfo(int iDrive, DWORD dwDiskData[256])
{
	char cszSerialNumber[1024];
	char cszModelNumber[1024];
	char cszRevisionNumber[1024];
	char bufferSize[32];

	__int64 i64Sectors = 0;
	__int64 i64Byte = 0;

	//  copy the hard iDrive serial number to the cszBuffer
	convertToString(dwDiskData, 10, 19, cszSerialNumber);
	convertToString(dwDiskData, 27, 46, cszModelNumber);
	convertToString(dwDiskData, 23, 26, cszRevisionNumber);
	sprintf_s(bufferSize, 32, "%u", dwDiskData[21] * 512);

	if (/*0 == m_cszHardDriveSerialNumber[0] &&*/
		//  serial number must be alphanumeric
		//  (but there can be leading spaces on IBM drives)
		(isalnum(cszSerialNumber[0]) || isalnum(cszSerialNumber[19])))
	{
		strcpy_s(m_cszHardDriveSerialNumber, 1024, cszSerialNumber);
		std::string strTmpDsn = m_cszHardDriveSerialNumber;
		//在虚拟机中，硬盘的序列号可能为空，或者为0000000000000001之类的形式
		if (strTmpDsn.empty() || strTmpDsn.find("00000000") != string::npos) {
            return;
		}
		m_vecSysDirveSerNum.push_back(m_cszHardDriveSerialNumber);
		strcpy_s(m_cszHardDriveModelNumber, 1024, cszModelNumber);
	}
	else {
		//printf("PrintIdeInfo get %d drive error,sn = null\n", iDrive);
	}
}

long MasterHardDiskSerial::getHardDriveComputerID()
{
	int iDone = FALSE;
	// char string [1024];
	__int64 i64Id = 0;
	OSVERSIONINFO version;

	strcpy_s(m_cszHardDriveSerialNumber, 1024, "");
	m_vecSysDirveSerNum.clear();
	memset(&version, 0, sizeof(version));
	version.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&version);
	if (version.dwPlatformId == VER_PLATFORM_WIN32_NT)
	{
		//这个顺序获取是，无权限到有系统权限升高
		//  this works under WinNT4 or Win2K or WinXP if you have any rights
		iDone = readPhysicalDriveInNTWithZeroRights();

		//  this works under WinNT4 or Win2K or WinXP or Windows Server 2003 or Vista if you have any rights
		if (!iDone)
			iDone = readPhysicalDriveInNTUsingSmart();

		//  this should work in WinNT or Win2K if previous did not work
		//  this is kind of a backdoor via the SCSI mini port driver into
		//     the IDE drives
		if (!iDone)
			iDone = readIdeDriveAsScsiDriveInNT();

		//  this works under WinNT4 or Win2K if you have admin rights
		if (!iDone)
			iDone = readPhysicalDriveInNTWithAdminRights();
	}

	if (m_cszHardDriveSerialNumber[0] > 0)
	{
		char *cP = m_cszHardDriveSerialNumber;

		//  ignore first 5 characters from western digital hard drives if
		//  the first four characters are WD-W
		if (!strncmp(m_cszHardDriveSerialNumber, "WD-W", 4))
			cP += 5;
		for (; cP && *cP; cP++)
		{
			if ('-' == *cP)
				continue;
			i64Id *= 10;
			switch (*cP)
			{
			case '0': i64Id += 0; break;
			case '1': i64Id += 1; break;
			case '2': i64Id += 2; break;
			case '3': i64Id += 3; break;
			case '4': i64Id += 4; break;
			case '5': i64Id += 5; break;
			case '6': i64Id += 6; break;
			case '7': i64Id += 7; break;
			case '8': i64Id += 8; break;
			case '9': i64Id += 9; break;
			case 'a': case 'A': i64Id += 10; break;
			case 'b': case 'B': i64Id += 11; break;
			case 'c': case 'C': i64Id += 12; break;
			case 'd': case 'D': i64Id += 13; break;
			case 'e': case 'E': i64Id += 14; break;
			case 'f': case 'F': i64Id += 15; break;
			case 'g': case 'G': i64Id += 16; break;
			case 'h': case 'H': i64Id += 17; break;
			case 'i': case 'I': i64Id += 18; break;
			case 'j': case 'J': i64Id += 19; break;
			case 'k': case 'K': i64Id += 20; break;
			case 'l': case 'L': i64Id += 21; break;
			case 'm': case 'M': i64Id += 22; break;
			case 'n': case 'N': i64Id += 23; break;
			case 'o': case 'O': i64Id += 24; break;
			case 'p': case 'P': i64Id += 25; break;
			case 'q': case 'Q': i64Id += 26; break;
			case 'r': case 'R': i64Id += 27; break;
			case 's': case 'S': i64Id += 28; break;
			case 't': case 'T': i64Id += 29; break;
			case 'u': case 'U': i64Id += 30; break;
			case 'v': case 'V': i64Id += 31; break;
			case 'w': case 'W': i64Id += 32; break;
			case 'x': case 'X': i64Id += 33; break;
			case 'y': case 'Y': i64Id += 34; break;
			case 'z': case 'Z': i64Id += 35; break;
			}
		}
	}

	i64Id %= 100000000;
	if (strstr(m_cszHardDriveModelNumber, "IBM-"))
		i64Id += 300000000;
	else if (strstr(m_cszHardDriveModelNumber, "MAXTOR") ||
		strstr(m_cszHardDriveModelNumber, "Maxtor"))
		i64Id += 400000000;
	else if (strstr(m_cszHardDriveModelNumber, "WDC "))
		i64Id += 500000000;
	else
		i64Id += 600000000;

	return (long)i64Id;
}

//int MasterHardDiskSerial::GetSerialNo(std::vector<char> &serialNumber)
int MasterHardDiskSerial::getSerialNo(std::string &strSerialNumber)
{
	strSerialNumber.clear();
	getHardDriveComputerID();
	if (m_vecSysDirveSerNum.size() > 0) {
		strSerialNumber = m_vecSysDirveSerNum.at(0);
	}
	else {
		//如果上面的方法获取不了，则改用另外的方法获取
		std::vector<string> serialNumber;
		getSerialNo(serialNumber);
		if (serialNumber.size() > 0) {
			strSerialNumber = serialNumber.at(0);
		}
	}
	return 0;
}

char *MasterHardDiskSerial::convertToString(DWORD dwDiskData[256],
	int iFirstIndex,
	int iLastIndex,
	char* pcszBuf)
{
	int iIndex = 0;
	int iPosition = 0;

	//  each integer has two characters stored in it backwards

	// Removes the spaces from the serial no
	for (iIndex = iFirstIndex; iIndex <= iLastIndex; iIndex++)
	{
		//  get high byte for 1st character	
		char ctemp = (char)(dwDiskData[iIndex] / 256);
		char cszmyspace[] = " ";
		if (!(ctemp == *cszmyspace))
		{
			pcszBuf[iPosition++] = ctemp;
		}
		//  get low byte for 2nd character
		char ctemp1 = (char)(dwDiskData[iIndex] % 256);
		if (!(ctemp1 == *cszmyspace))
		{
			pcszBuf[iPosition++] = ctemp1;
		}
	}

	//  end the string 
	pcszBuf[iPosition] = '\0';

	//  cut off the trailing blanks
	for (iIndex = iPosition - 1; iIndex > 0 && isspace(pcszBuf[iIndex]); iIndex--)
		pcszBuf[iIndex] = '\0';
	return pcszBuf;
}

int MasterHardDiskSerial::getErrorMessage(TCHAR* tszErrorMessage)
{
	if (strlen(m_cszErrorMessage) != 0)
	{
		mbstowcs((wchar_t *)tszErrorMessage, m_cszErrorMessage, sizeof(m_cszErrorMessage));
		return 0;
	}
	else
		return -1;
}

int MasterHardDiskSerial::getSysDriveID(std::vector<int> &vecSysDriveID){
	vecSysDriveID.clear();
	try {
		HRESULT hres;
		//需要外部初始化和反初始化，否则会引起冲突
		::CoInitializeEx(0, COINIT_MULTITHREADED);
		::CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
		IWbemLocator *pLoc = NULL;
		hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID *)&pLoc);
		if (FAILED(hres)) {
			return 0;
		}
		IWbemServices *pSvc = NULL;
		hres = pLoc->ConnectServer(_bstr_t(L"ROOT\\CIMV2"), NULL, NULL, 0, NULL, 0, 0, &pSvc);
		if (FAILED(hres)) {
			pLoc->Release();
			return 0;
		}
		hres = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
		if (FAILED(hres)) {
			pSvc->Release();
			pLoc->Release();
			return 0;
		}
		IEnumWbemClassObject* pEnumerator = NULL;
		IWbemClassObject *pclsObj;
		ULONG uReturn = 0;

		//获取系统所在硬盘的ID
		int diskIndex = -1;
		hres = pSvc->ExecQuery(
			bstr_t("WQL"),
			bstr_t("SELECT * FROM Win32_DiskPartition WHERE Bootable = TRUE"),  //查找启动盘
			WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
			NULL,
			&pEnumerator);
		if (FAILED(hres)) {
			pSvc->Release();
			pLoc->Release();
			return 0;
		}
		while (pEnumerator)
		{
			HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
			if (FAILED(hr) || 0 == uReturn) {
				break;
			}
			VARIANT vtProp;
			hr = pclsObj->Get(L"DiskIndex", 0, &vtProp, 0, 0);
			diskIndex = vtProp.intVal;
			VariantClear(&vtProp);
			vecSysDriveID.push_back(diskIndex);
			pclsObj->Release();
		}
		if (pSvc)
			pSvc->Release();
		if (pLoc)
			pLoc->Release();
		if (pEnumerator)
			pEnumerator->Release();
	}
	catch (...) {
	}
//	FW_LOG_INFOA << "| find drive num="<<vecSysDriveID.size();
	return vecSysDriveID.size();
}
void MasterHardDiskSerial::getSerialNo(std::vector<string> &serialNumber) {
	serialNumber.clear();
	m_vecSysDirveSerNum.clear();
	try {
		HRESULT hres;
		::CoInitializeEx(0, COINIT_MULTITHREADED);
		::CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
		IWbemLocator *pLoc = NULL;
		hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID *)&pLoc);
		if (FAILED(hres)) {
			return;
		}
		IWbemServices *pSvc = NULL;
		hres = pLoc->ConnectServer(_bstr_t(L"ROOT\\CIMV2"), NULL, NULL, 0, NULL, 0, 0, &pSvc);
		if (FAILED(hres)) {
			pLoc->Release();
			return;
		}
		hres = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
		if (FAILED(hres)) {
			pSvc->Release();
			pLoc->Release();
			return;
		}
		IEnumWbemClassObject* pEnumerator = NULL;
		IWbemClassObject *pclsObj = NULL;
		ULONG uReturn = 0;
		//获取系统所在硬盘的ID
		int diskIndex = 0;
		hres = pSvc->ExecQuery(
			bstr_t("WQL"),
			bstr_t("SELECT * FROM Win32_DiskPartition WHERE Bootable = TRUE"),  //查找启动盘
			WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
			NULL,
			&pEnumerator);
		if (FAILED(hres)) {
			pSvc->Release();
			pLoc->Release();
			return;
		}
		vector<int> vecDiskIndex;
		while (pEnumerator) {
			HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
			if (FAILED(hr) || 0 == uReturn) {
				break;
			}
			VARIANT vtProp;
			hr = pclsObj->Get(L"DiskIndex", 0, &vtProp, 0, 0);
			diskIndex = vtProp.intVal;
			VariantClear(&vtProp);
			vecDiskIndex.push_back(diskIndex);
			pclsObj->Release();
		}
		if (pEnumerator) {
			pEnumerator->Release();
			pEnumerator = NULL;
		}
		//根据系统所在硬盘的ID查询序列号
		for (std::size_t i = 0; i < vecDiskIndex.size(); i++) {
			char index[10];
			string strQuery = "SELECT * FROM Win32_DiskDrive WHERE Index = ";
			diskIndex = vecDiskIndex.at(i);
			_itoa(diskIndex, index, 10);
			string indexStr(index);
			strQuery += indexStr;
			if (pEnumerator) {
				pEnumerator->Release();
				pEnumerator = NULL;
			}
			hres = pSvc->ExecQuery(bstr_t("WQL"), bstr_t(strQuery.c_str()), WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnumerator);
			if (FAILED(hres)) {
				pSvc->Release();
				pLoc->Release();
				continue;
			}
			int n = 0;
			while (pEnumerator && ++n<10) {
				HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
				if (FAILED(hr) || 0 == uReturn) {
					break;
				}
				VARIANT vtProp;
				hr = pclsObj->Get(L"SerialNumber", 0, &vtProp, 0, 0);
				if (FAILED(hr)) {
					pclsObj->Release();
					continue;
				}

				char szSystemDiskID[256] = { 0 };
				//wcstombs(szSystemDiskID, vtProp.bstrVal, sizeof(vtProp.bstrVal));
                std::string strTmp;
                if (vtProp.bstrVal && sizeof(vtProp.bstrVal) >0)
                    strTmp = _com_util::ConvertBSTRToString(vtProp.bstrVal);
			
				if (!strTmp.empty()) {
                    memcpy_s(szSystemDiskID, 256, strTmp.c_str(), min(255, strTmp.length()));
                    trims(szSystemDiskID);
                    strTmp = szSystemDiskID;
					m_vecSysDirveSerNum.push_back(strTmp);
				}
				else {
					//printf("获取硬盘%d序列号为空\n", diskIndex);
				}

				VariantClear(&vtProp);
				pclsObj->Release();

			}
			if (pEnumerator) {
				pEnumerator->Release();
				pEnumerator = NULL;
			}
		}
		if (pSvc) pSvc->Release();
		if (pLoc) pLoc->Release();
		if (pEnumerator) {
			pEnumerator->Release();
			pEnumerator = NULL;
		}
	}
	catch (...) {
	}
	serialNumber = m_vecSysDirveSerNum;
}

/**
 * @brief MAC类型
 */
enum eMacType {
    Ethernet, // 物理以太网类型
    Physical, // 物理网卡
    All,      // 所有网卡
};
/*

bool isLocalAdapter(const char *pAdapterName)
{
    bool ret_value = false;
#define NET_CARD_KEY _T("SYSTEM\\CurrentControlSet\\Control\\Network\\{4D36E972-E325-11CE-BFC1-08002BE10318}")
    TCHAR szDataBuf[MAX_PATH + 1];
    DWORD dwDataLen = MAX_PATH;
    DWORD dwType = REG_SZ;
    HKEY hNetKey = NULL;
    HKEY hLocalNet = NULL;

    if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE, NET_CARD_KEY, 0, KEY_QUERY_VALUE, &hNetKey))         {
//        FW_LOG_ERRORA << "hNetWorkKey get failed"<< pAdapterName;
        return false;
    }

    string strPath = pAdapterName;
    strPath.append("\\Connection");
    wstring strPathw = ANSI2Unicode(strPath.c_str());
    if (ERROR_SUCCESS != RegOpenKeyEx(hNetKey, strPathw.c_str(), 0, KEY_QUERY_VALUE, &hLocalNet)) {
//        FW_LOG_ERRORA << "hLocalKey get failed" << pAdapterName;
        RegCloseKey(hNetKey);
        return false;
    }

    dwDataLen = MAX_PATH;
    if (ERROR_SUCCESS == RegQueryValueEx(hLocalNet, L"PnpInstanceID", 0, &dwType, ( BYTE * )szDataBuf, &dwDataLen)) {
        if (wcsncmp(szDataBuf, L"PCI", wcslen(L"PCI")) == 0)
            ret_value = true;
    }
    else         {
//        FW_LOG_ERRORA << "PnpInstanceID get failed" << pAdapterName;
    }

//    FW_LOG_INFOA << pAdapterName<<" bLocal is "<< ret_value;
    RegCloseKey(hLocalNet);
    RegCloseKey(hNetKey);
    return ret_value;
}
*/

bool getMacImpl(eMacType eType, std::vector<taf::MAC_INFO> &in_vecMac)
{
    bool bRet = false;
    in_vecMac.clear();
    //PIP_ADAPTER_INFO结构体指针存储本机网卡信息
    PIP_ADAPTER_INFO pAdapter = NULL;

    //得到结构体大小,用于GetAdaptersInfo参数
    unsigned long stSize = sizeof(IP_ADAPTER_INFO);
    PIP_ADAPTER_INFO pIpAdapterInfo = (PIP_ADAPTER_INFO)new BYTE[stSize];
    //调用GetAdaptersInfo函数,填充pIpAdapterInfo指针变量;其中stSize参数既是一个输入量也是一个输出量
    int nRel = GetAdaptersInfo(pIpAdapterInfo, &stSize);
    if (ERROR_BUFFER_OVERFLOW == nRel) {
        //如果函数返回的是ERROR_BUFFER_OVERFLOW
        //则说明GetAdaptersInfo参数传递的内存空间不够,同时其传出stSize,表示需要的空间大小
        //这也是说明为什么stSize既是一个输入量也是一个输出量
        //释放原来的内存空间
        delete[](( BYTE* )pIpAdapterInfo);
        //重新申请内存空间用来存储所有网卡信息
        pIpAdapterInfo = (PIP_ADAPTER_INFO)new BYTE[stSize];
        //再次调用GetAdaptersInfo函数,填充pIpAdapterInfo指针变量
        nRel = GetAdaptersInfo(pIpAdapterInfo, &stSize);
    }

    if (ERROR_SUCCESS == nRel) {
        //输出网卡信息
        //可能有多网卡,因此通过循环去判断
        pAdapter = pIpAdapterInfo;
        while (pAdapter) {
            taf::MAC_INFO mac;
            {
				// 是否校验以太网或物理网卡
				bool check_ethernet = eType==eMacType::Ethernet;
				bool check_physical = (eType==eMacType::Ethernet) || (eType==eMacType::Physical);

                // 确保是以太网
                if (check_ethernet && pAdapter->Type != MIB_IF_TYPE_ETHERNET) {
                    pAdapter = pAdapter->Next;
                    continue;
                }
                //确保是物理网卡
//                if (check_physical && !isLocalAdapter(pAdapter->AdapterName)) {
                if (check_physical) {
                    pAdapter = pAdapter->Next;
                    continue;
                }

				// 格式是否正确
                if (pAdapter->AddressLength != 6) {
                    pAdapter = pAdapter->Next;
                    continue;
                }

                // 确保MAC地址的长度为 00-00-00-00-00-00		
                char acMAC[32];
                sprintf(acMAC, "%02X-%02X-%02X-%02X-%02X-%02X",
                    int(pAdapter->Address[0]),
                    int(pAdapter->Address[1]),
                    int(pAdapter->Address[2]),
                    int(pAdapter->Address[3]),
                    int(pAdapter->Address[4]),
                    int(pAdapter->Address[5]));

                //收集
                mac.dw_type_ = pAdapter->Type;
                mac.str_name_ = pAdapter->AdapterName;
                mac.str_description_ = pAdapter->Description;
                mac.str_mac_ = acMAC;
                in_vecMac.push_back(mac);

                bRet = true;
            }
            pAdapter = pAdapter->Next;
        }
    }

    //释放内存空间
    if (pIpAdapterInfo) {
        delete[](( BYTE* )pIpAdapterInfo);
    }

    return bRet;
}


//该接口获取的网卡可能是虚拟的 动态的。不建议做绑定key
bool MasterHardDiskSerial::getFirstActiveMAC(std::string &strMac) {
    std::vector<taf::MAC_INFO> in_vecMac;
    getMacImpl(eMacType::All, in_vecMac);
	if (in_vecMac.empty()) return false;

	strMac = in_vecMac[0].str_mac_;
	return true;
}
//该接口获取的网卡是物理的，ethernet的网卡 可做绑定key
bool MasterHardDiskSerial::getFristEthernetMac(std::string &strMac) {
    std::vector<taf::MAC_INFO> in_vecMac;
    getMacImpl(eMacType::Ethernet, in_vecMac);
	if (in_vecMac.empty()) return false;

	strMac = in_vecMac[0].str_mac_;
	return true;
}

//获取机器所有的mac 包含所有类型
bool MasterHardDiskSerial::getAllMAC(std::vector<taf::MAC_INFO> &in_vecMac) {
    return  getMacImpl(eMacType::All, in_vecMac);
}

bool MasterHardDiskSerial::getAllPhysicalMAC(std::vector<taf::MAC_INFO> &in_vecMac) {
    return  getMacImpl(eMacType::Physical, in_vecMac);
}

MasterHardDiskSerial::MasterHardDiskSerial()
{
	SecureZeroMemory(m_cszErrorMessage, sizeof(m_cszErrorMessage));
	SecureZeroMemory(m_cszHardDriveModelNumber, sizeof(m_cszHardDriveModelNumber));
	SecureZeroMemory(m_cszHardDriveSerialNumber, sizeof(m_cszHardDriveSerialNumber));
}

MasterHardDiskSerial::~MasterHardDiskSerial()
{
}

#endif