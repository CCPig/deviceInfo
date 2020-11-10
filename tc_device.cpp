#include    "tc_device.h"

#if         TARGET_PLATFORM_WINDOWS
#include    <sstream>
#include    <codecvt>
#include    <comdef.h>
#pragma     comment(lib, "ws2_32.lib")
#include    "iphlpapi.h"
#pragma     comment ( lib, "Iphlpapi.lib" )
#include    <Wbemidl.h>
#pragma     comment(lib, "wbemuuid.lib")

#else

#include    "util/tc_common.h"
#include    <cstring>
#include    <stdexcept>
#include    <limits>
#include    <tuple>
#include    <algorithm>
#include    <sys/ioctl.h>
#include    <sys/socket.h>
#include    <netinet/in.h>
#include    <arpa/inet.h>
#include    <net/if.h>
#endif

namespace taf
{

#if TARGET_PLATFORM_WINDOWS
typedef void(__stdcall* NTPROC)(DWORD*, DWORD*, DWORD*);

//将PULARGE_INTEGER类型的字节(B)数转化为(GB)单位
#define GB(x) (x.HighPart << 2) + (x.LowPart >> 20) / 1024.0

#define DEFAULT_CODE_PAGE 936

std::wstring utf8_to_wstring(const std::string& str)
{
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t> > strCnv;
	return strCnv.from_bytes(str);
}

std::string wstring_to_utf8(const std::wstring& str)
{
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t> > strCnv;
	return strCnv.to_bytes(str);
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
			DWORD dwBytesReturned = 0;

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
				sprintf_s(m_cszErrorMessage,
				256,
				"%d ReadPhysicalDriveInNTWithAdminRights ERROR DeviceIoControl() %d, DFP_GET_VERSION) returned 0, error is %d\n",
				__LINE__,
				(int)hPhysicalDriveIOCTL,
				(int)dwErr);
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
				BYTE bIDCmd = 0;   // IDE or ATAPI IDENTIFY cmd
				SENDCMDINPARAMS scip;
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
					USHORT* punIdSector = (USHORT*)
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
			memset((void*)&GetVersionParams, 0, sizeof(GetVersionParams));

			if (!DeviceIoControl(hPhysicalDriveIOCTL, SMART_GET_VERSION,
			NULL,
			0,
			&GetVersionParams, sizeof(GETVERSIONINPARAMS),
			&dwBytesReturned, NULL))
			{
				DWORD dwErr = GetLastError();
				SecureZeroMemory(m_cszErrorMessage, sizeof(m_cszErrorMessage));
				sprintf_s(m_cszErrorMessage,
				256,
				"\n%d ReadPhysicalDriveInNTUsingSmart ERROR DeviceIoControl(%d, SMART_GET_VERSION) returned 0, error is %d",
				__LINE__,
				(int)hPhysicalDriveIOCTL,
				(int)dwErr);
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
					USHORT* punIdSector = (USHORT*)
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

char* MasterHardDiskSerial::flipAndCodeBytes(int iPos, int iFlip, const char* pcszStr, char* pcszBuf)
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
				if (pcszBuf[iK] < 0)
				{
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

			memset((void*)&query, 0, sizeof(query));
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
				STORAGE_DEVICE_DESCRIPTOR* descrip = (STORAGE_DEVICE_DESCRIPTOR*)&cszBuffer;
				char cszSerialNumber[1000] = { 0 };

				flipAndCodeBytes(descrip->SerialNumberOffset, 0, cszBuffer, cszSerialNumber);
				trims(cszSerialNumber);

				if (/*0 == m_cszHardDriveSerialNumber[0] &&*/
					//  serial number must be alphanumeric
					//  (but there can be leading spaces on IBM drives)
				(iswalnum(cszSerialNumber[0]) || iswalnum(cszSerialNumber[19])))
				{
					strcpy_s(m_cszHardDriveSerialNumber, 1024, cszSerialNumber);
					std::string strTmpDsn = m_cszHardDriveSerialNumber;
					//在虚拟机中，硬盘的序列号可能为空，或者为0000000000000001之类的形式
					if (strTmpDsn.empty() || strTmpDsn.find("00000000") != string::npos)
					{
						continue;
					}
					m_vecSysDirveSerNum.push_back(m_cszHardDriveSerialNumber);
					iDone = TRUE;

					//printf("ReadPhysicalDriveInNTWithZeroRights get %d drive success:%s\n", iDrive, m_cszHardDriveSerialNumber);
				}
				else
				{
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
		char cszDriveName[256];

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
				SRB_IO_CONTROL* cP = (SRB_IO_CONTROL*)cszBuffer;
				SENDCMDINPARAMS* pin =
				(SENDCMDINPARAMS*)(cszBuffer + sizeof(SRB_IO_CONTROL));
				DWORD dwDummy;

				memset(cszBuffer, 0, sizeof(cszBuffer));
				cP->HeaderLength = sizeof(SRB_IO_CONTROL);
				cP->Timeout = 10000;
				cP->Length = SENDIDLENGTH;
				cP->ControlCode = IOCTL_SCSI_MINIPORT_IDENTIFY;
				strncpy((char*)cP->Signature, "SCSIDISK", 8);

				pin->irDriveRegs.bCommandReg = IDE_ATA_IDENTIFY;
				pin->bDriveNumber = iDrive;

				if (DeviceIoControl(hScsiDriveIOCTL,
				IOCTL_SCSI_MINIPORT,
				cszBuffer,
				sizeof(SRB_IO_CONTROL) + sizeof(SENDCMDINPARAMS) - 1,
				cszBuffer,
				sizeof(SRB_IO_CONTROL) + SENDIDLENGTH,
				&dwDummy,
				NULL))
				{
					SENDCMDOUTPARAMS* pOut =
					(SENDCMDOUTPARAMS*)(cszBuffer + sizeof(SRB_IO_CONTROL));
					IDSECTOR* pId = (IDSECTOR*)(pOut->bBuffer);
					if (pId->sModelNumber[0])
					{
						DWORD dwDiskData[256];
						int iIjk = 0;
						USHORT* punIdSector = (USHORT*)pId;

						for (iIjk = 0; iIjk < 256; iIjk++)
							dwDiskData[iIjk] = punIdSector[iIjk];

						printIdeInfo(iController * 2 + iDrive, dwDiskData);

						iDone = TRUE;
						//printf("ReadIdeDriveAsScsiDriveInNT get %d drive success:%s\n", iDrive,m_cszHardDriveSerialNumber);
					}
				}
				else
				{
					//printf("ReadIdeDriveAsScsiDriveInNT get %d drive error\n", iDrive);
				}
			}
			CloseHandle(hScsiDriveIOCTL);
		}
		else
		{
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
		if (strTmpDsn.empty() || strTmpDsn.find("00000000") != string::npos)
		{
			return;
		}
		m_vecSysDirveSerNum.push_back(m_cszHardDriveSerialNumber);
		strcpy_s(m_cszHardDriveModelNumber, 1024, cszModelNumber);
	}
	else
	{
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
		char* cP = m_cszHardDriveSerialNumber;

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
			case '0':
				i64Id += 0;
				break;
			case '1':
				i64Id += 1;
				break;
			case '2':
				i64Id += 2;
				break;
			case '3':
				i64Id += 3;
				break;
			case '4':
				i64Id += 4;
				break;
			case '5':
				i64Id += 5;
				break;
			case '6':
				i64Id += 6;
				break;
			case '7':
				i64Id += 7;
				break;
			case '8':
				i64Id += 8;
				break;
			case '9':
				i64Id += 9;
				break;
			case 'a':
			case 'A':
				i64Id += 10;
				break;
			case 'b':
			case 'B':
				i64Id += 11;
				break;
			case 'c':
			case 'C':
				i64Id += 12;
				break;
			case 'd':
			case 'D':
				i64Id += 13;
				break;
			case 'e':
			case 'E':
				i64Id += 14;
				break;
			case 'f':
			case 'F':
				i64Id += 15;
				break;
			case 'g':
			case 'G':
				i64Id += 16;
				break;
			case 'h':
			case 'H':
				i64Id += 17;
				break;
			case 'i':
			case 'I':
				i64Id += 18;
				break;
			case 'j':
			case 'J':
				i64Id += 19;
				break;
			case 'k':
			case 'K':
				i64Id += 20;
				break;
			case 'l':
			case 'L':
				i64Id += 21;
				break;
			case 'm':
			case 'M':
				i64Id += 22;
				break;
			case 'n':
			case 'N':
				i64Id += 23;
				break;
			case 'o':
			case 'O':
				i64Id += 24;
				break;
			case 'p':
			case 'P':
				i64Id += 25;
				break;
			case 'q':
			case 'Q':
				i64Id += 26;
				break;
			case 'r':
			case 'R':
				i64Id += 27;
				break;
			case 's':
			case 'S':
				i64Id += 28;
				break;
			case 't':
			case 'T':
				i64Id += 29;
				break;
			case 'u':
			case 'U':
				i64Id += 30;
				break;
			case 'v':
			case 'V':
				i64Id += 31;
				break;
			case 'w':
			case 'W':
				i64Id += 32;
				break;
			case 'x':
			case 'X':
				i64Id += 33;
				break;
			case 'y':
			case 'Y':
				i64Id += 34;
				break;
			case 'z':
			case 'Z':
				i64Id += 35;
				break;
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
int MasterHardDiskSerial::getSerialNo(std::string& strSerialNumber)
{
	strSerialNumber.clear();
	getHardDriveComputerID();
	if (m_vecSysDirveSerNum.size() > 0)
	{
		strSerialNumber = m_vecSysDirveSerNum.at(0);
	}
	else
	{
		//如果上面的方法获取不了，则改用另外的方法获取
		std::vector<string> serialNumber;
		getSerialNo(serialNumber);
		if (serialNumber.size() > 0)
		{
			strSerialNumber = serialNumber.at(0);
		}
	}
	return 0;
}

char* MasterHardDiskSerial::convertToString(DWORD dwDiskData[256],
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

int MasterHardDiskSerial::getSysDriveID(std::vector<int>& vecSysDriveID)
{
	vecSysDriveID.clear();
	try
	{
		HRESULT hres;
		//需要外部初始化和反初始化，否则会引起冲突
		::CoInitializeEx(0, COINIT_MULTITHREADED);
		::CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
		IWbemLocator* pLoc = NULL;
		hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&pLoc);
		if (FAILED(hres))
		{
			return 0;
		}
		IWbemServices* pSvc = NULL;
		hres = pLoc->ConnectServer(_bstr_t(L"ROOT\\CIMV2"), NULL, NULL, 0, NULL, 0, 0, &pSvc);
		if (FAILED(hres))
		{
			pLoc->Release();
			return 0;
		}
		hres = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
		if (FAILED(hres))
		{
			pSvc->Release();
			pLoc->Release();
			return 0;
		}
		IEnumWbemClassObject* pEnumerator = NULL;
		IWbemClassObject* pclsObj;
		ULONG uReturn = 0;

		//获取系统所在硬盘的ID
		int diskIndex = -1;
		hres = pSvc->ExecQuery(
		bstr_t("WQL"),
		bstr_t("SELECT * FROM Win32_DiskPartition WHERE Bootable = TRUE"),  //查找启动盘
		WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
		NULL,
		&pEnumerator);
		if (FAILED(hres))
		{
			pSvc->Release();
			pLoc->Release();
			return 0;
		}
		while (pEnumerator)
		{
			HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
			if (FAILED(hr) || 0 == uReturn)
			{
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
	catch (...)
	{
	}
	//	FW_LOG_INFOA << "| find drive num="<<vecSysDriveID.size();
	return vecSysDriveID.size();
}

void MasterHardDiskSerial::getSerialNo(std::vector<string>& serialNumber)
{
	serialNumber.clear();
	m_vecSysDirveSerNum.clear();
	try
	{
		HRESULT hres;
		::CoInitializeEx(0, COINIT_MULTITHREADED);
		::CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
		IWbemLocator* pLoc = NULL;
		hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&pLoc);
		if (FAILED(hres))
		{
			return;
		}
		IWbemServices* pSvc = NULL;
		hres = pLoc->ConnectServer(_bstr_t(L"ROOT\\CIMV2"), NULL, NULL, 0, NULL, 0, 0, &pSvc);
		if (FAILED(hres))
		{
			pLoc->Release();
			return;
		}
		hres = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
		if (FAILED(hres))
		{
			pSvc->Release();
			pLoc->Release();
			return;
		}
		IEnumWbemClassObject* pEnumerator = NULL;
		IWbemClassObject* pclsObj = NULL;
		ULONG uReturn = 0;
		//获取系统所在硬盘的ID
		int diskIndex = 0;
		hres = pSvc->ExecQuery(
		bstr_t("WQL"),
		bstr_t("SELECT * FROM Win32_DiskPartition WHERE Bootable = TRUE"),  //查找启动盘
		WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
		NULL,
		&pEnumerator);
		if (FAILED(hres))
		{
			pSvc->Release();
			pLoc->Release();
			return;
		}
		vector<int> vecDiskIndex;
		while (pEnumerator)
		{
			HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
			if (FAILED(hr) || 0 == uReturn)
			{
				break;
			}
			VARIANT vtProp;
			hr = pclsObj->Get(L"DiskIndex", 0, &vtProp, 0, 0);
			diskIndex = vtProp.intVal;
			VariantClear(&vtProp);
			vecDiskIndex.push_back(diskIndex);
			pclsObj->Release();
		}
		if (pEnumerator)
		{
			pEnumerator->Release();
			pEnumerator = NULL;
		}
		//根据系统所在硬盘的ID查询序列号
		for (std::size_t i = 0; i < vecDiskIndex.size(); i++)
		{
			char index[10];
			string strQuery = "SELECT * FROM Win32_DiskDrive WHERE Index = ";
			diskIndex = vecDiskIndex.at(i);
			_itoa(diskIndex, index, 10);
			string indexStr(index);
			strQuery += indexStr;
			if (pEnumerator)
			{
				pEnumerator->Release();
				pEnumerator = NULL;
			}
			hres = pSvc->ExecQuery(bstr_t("WQL"), bstr_t(strQuery.c_str()), WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnumerator);
			if (FAILED(hres))
			{
				pSvc->Release();
				pLoc->Release();
				continue;
			}
			int n = 0;
			while (pEnumerator && ++n < 10)
			{
				HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
				if (FAILED(hr) || 0 == uReturn)
				{
					break;
				}
				VARIANT vtProp;
				hr = pclsObj->Get(L"SerialNumber", 0, &vtProp, 0, 0);
				if (FAILED(hr))
				{
					pclsObj->Release();
					continue;
				}

				char szSystemDiskID[256] = { 0 };
				//wcstombs(szSystemDiskID, vtProp.bstrVal, sizeof(vtProp.bstrVal));
				std::string strTmp;
				if (vtProp.bstrVal && sizeof(vtProp.bstrVal) > 0)
					strTmp = _com_util::ConvertBSTRToString(vtProp.bstrVal);

				if (!strTmp.empty())
				{
					memcpy_s(szSystemDiskID, 256, strTmp.c_str(), min(255, strTmp.length()));
					trims(szSystemDiskID);
					strTmp = szSystemDiskID;
					m_vecSysDirveSerNum.push_back(strTmp);
				}
				else
				{
					//printf("获取硬盘%d序列号为空\n", diskIndex);
				}

				VariantClear(&vtProp);
				pclsObj->Release();

			}
			if (pEnumerator)
			{
				pEnumerator->Release();
				pEnumerator = NULL;
			}
		}
		if (pSvc) pSvc->Release();
		if (pLoc) pLoc->Release();
		if (pEnumerator)
		{
			pEnumerator->Release();
			pEnumerator = NULL;
		}
	}
	catch (...)
	{
	}
	serialNumber = m_vecSysDirveSerNum;
}

/**
* @brief MAC类型
*/
enum eMacType
{
	Ethernet, // 物理以太网类型
	Physical, // 物理网卡
	All,      // 所有网卡
};

bool IsLocalAdapter(const char* pAdapterName)
{
	bool ret_value = false;
#ifdef UNICODE
#define NET_CARD_KEY _T("SYSTEM\\CurrentControlSet\\Control\\Network\\{4D36E972-E325-11CE-BFC1-08002BE10318}")
#else
#define NET_CARD_KEY ("SYSTEM\\CurrentControlSet\\Control\\Network\\{4D36E972-E325-11CE-BFC1-08002BE10318}")
#endif // !UNICODE

	TCHAR szDataBuf[MAX_PATH + 1];
	DWORD dwDataLen = MAX_PATH;
	DWORD dwType = REG_SZ;
	HKEY hNetKey = NULL;
	HKEY hLocalNet = NULL;

	if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE, NET_CARD_KEY, 0, KEY_QUERY_VALUE, &hNetKey))
	{
		return false;
	}

	string strPath = pAdapterName;
	strPath.append("\\Connection");
#ifdef UNICODE
	wstring strPathw = utf8_to_wstring(strPath.c_str());
		if (ERROR_SUCCESS != RegOpenKeyEx(hNetKey, strPathw.c_str(), 0, KEY_QUERY_VALUE, &hLocalNet)) {
			RegCloseKey(hNetKey);
			return false;
		}
#else
	if (ERROR_SUCCESS != RegOpenKeyEx(hNetKey, strPath.c_str(), 0, KEY_QUERY_VALUE, &hLocalNet))
	{
		RegCloseKey(hNetKey);
		return false;
	}
#endif // !UNICODE

	dwDataLen = MAX_PATH;
#ifdef UNICODE
	if (ERROR_SUCCESS == RegQueryValueEx(hLocalNet, L"PnpInstanceID", 0, &dwType, (BYTE*)szDataBuf, &dwDataLen))
	{
		if (wcsncmp(szDataBuf, L"PCI", wcslen(L"PCI")) == 0)
			ret_value = true;
	}
#else
	if (ERROR_SUCCESS == RegQueryValueEx(hLocalNet, "PnpInstanceID", 0, &dwType, (BYTE*)szDataBuf, &dwDataLen))
	{
		if (strncmp(szDataBuf, "PCI", strlen("PCI")) == 0)
			ret_value = true;
	}
#endif // !UNICODE

	
	RegCloseKey(hLocalNet);
	RegCloseKey(hNetKey);
	return ret_value;
}

bool getMacImpl(eMacType eType, std::vector<taf::MAC_INFO>& in_vecMac)
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
	if (ERROR_BUFFER_OVERFLOW == nRel)
	{
		//如果函数返回的是ERROR_BUFFER_OVERFLOW
		//则说明GetAdaptersInfo参数传递的内存空间不够,同时其传出stSize,表示需要的空间大小
		//这也是说明为什么stSize既是一个输入量也是一个输出量
		//释放原来的内存空间
		delete[]((BYTE*)pIpAdapterInfo);
		//重新申请内存空间用来存储所有网卡信息
		pIpAdapterInfo = (PIP_ADAPTER_INFO)new BYTE[stSize];
		//再次调用GetAdaptersInfo函数,填充pIpAdapterInfo指针变量
		nRel = GetAdaptersInfo(pIpAdapterInfo, &stSize);
	}

	if (ERROR_SUCCESS == nRel)
	{
		//输出网卡信息
		//可能有多网卡,因此通过循环去判断
		pAdapter = pIpAdapterInfo;
		while (pAdapter)
		{
			taf::MAC_INFO mac;
			{
				// 是否校验以太网或物理网卡
				bool check_ethernet = eType == eMacType::Ethernet;
				bool check_physical = (eType == eMacType::Ethernet) || (eType == eMacType::Physical);

				// 确保是以太网
				if (check_ethernet && pAdapter->Type != MIB_IF_TYPE_ETHERNET)
				{
					pAdapter = pAdapter->Next;
					continue;
				}
				//确保是物理网卡
				//                if (check_physical && !isLocalAdapter(pAdapter->AdapterName)) {
				if (check_physical && !IsLocalAdapter(pAdapter->AdapterName))
				{
					pAdapter = pAdapter->Next;
					continue;
				}

				// 格式是否正确
				if (pAdapter->AddressLength != 6)
				{
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
	if (pIpAdapterInfo)
	{
		delete[]((BYTE*)pIpAdapterInfo);
	}

	return bRet;
}

//该接口获取的网卡可能是虚拟的 动态的。不建议做绑定key
bool MasterHardDiskSerial::getFirstActiveMAC(taf::MAC_INFO& strMac)
{
	std::vector<taf::MAC_INFO> in_vecMac;
	getMacImpl(eMacType::All, in_vecMac);
	if (in_vecMac.empty()) return false;

	strMac = in_vecMac[0];
	return true;
}

//该接口获取的网卡是物理的，ethernet的网卡 可做绑定key
bool MasterHardDiskSerial::getFristEthernetMac(taf::MAC_INFO& strMac)
{
	std::vector<taf::MAC_INFO> in_vecMac;
	getMacImpl(eMacType::Ethernet, in_vecMac);
	if (in_vecMac.empty()) return false;

	strMac = in_vecMac[0];
	return true;
}

//获取机器所有的mac 包含所有类型
bool MasterHardDiskSerial::getAllMAC(std::vector<taf::MAC_INFO>& in_vecMac)
{
	return getMacImpl(eMacType::All, in_vecMac);
}

bool MasterHardDiskSerial::getAllPhysicalMAC(std::vector<taf::MAC_INFO>& in_vecMac)
{
	return getMacImpl(eMacType::Physical, in_vecMac);
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

//字符串格式化函数
std::string format(const char* fmt, ...)
{
	std::string strResult = "";
	if (NULL != fmt)
	{
		va_list marker = NULL;
		va_start(marker, fmt);                            //初始化变量参数
		size_t nLength = _vscprintf(fmt, marker) + 1;    //获取格式化字符串长度
		std::vector<char> vBuffer(nLength, '\0');        //创建用于存储格式化字符串的字符数组
		int nWritten = _vsnprintf_s(&vBuffer[0], vBuffer.size(), nLength, fmt, marker);
		if (nWritten > 0)
		{
			strResult = &vBuffer[0];
		}
		va_end(marker);                                    //重置变量参数
	}
	return strResult;
}

void getcpuidex(unsigned int* CPUInfo, unsigned int InfoType, unsigned int ECXValue)
{
#if defined(_MSC_VER) // MSVC
#if defined(_WIN64) // 64位下不支持内联汇编. 1600: VS2010, 据说VC2008 SP1之后才支持__cpuidex.
	__cpuidex((int*)(void*)CPUInfo, (int)InfoType, (int)ECXValue);
#else
	if (NULL == CPUInfo)
		return;
	_asm {
		// load. 读取参数到寄存器.
		mov edi, CPUInfo;
		mov eax, InfoType;
		mov ecx, ECXValue;
		// CPUID
		cpuid;
		// save. 将寄存器保存到CPUInfo
		mov[edi], eax;
		mov[edi + 4], ebx;
		mov[edi + 8], ecx;
		mov[edi + 12], edx;
	}
#endif
#endif
}

void getcpuid(unsigned int* CPUInfo, unsigned int InfoType)
{
#if defined(__GNUC__)// GCC
	__cpuid(InfoType, CPUInfo[0], CPUInfo[1], CPUInfo[2], CPUInfo[3]);
#elif defined(_MSC_VER)// MSVC
#if _MSC_VER >= 1400 //VC2005才支持__cpuid
	__cpuid((int*)(void*)CPUInfo, (int)(InfoType));
#else //其他使用getcpuidex
	getcpuidex(CPUInfo, InfoType, 0);
#endif
#endif
}

void get_cpuId(std::string& str_cpu)
{
	char pCpuId[32] = "";
	int dwBuf[4];
	getcpuid((unsigned int*)dwBuf, 1);
	sprintf(pCpuId, "%08X", dwBuf[3]);
	sprintf(pCpuId + 8, "%08X", dwBuf[0]);
	str_cpu = pCpuId;
}

#elif TARGET_PLATFORM_LINUX

static bool get_cpuId(std::string& cpu_id)
{
	cpu_id.clear();

	unsigned int s1 = 0;
	unsigned int s2 = 0;
	asm volatile
	(
	"movl $0x01, %%eax; \n\t"
	"xorl %%edx, %%edx; \n\t"
	"cpuid; \n\t"
	"movl %%edx, %0; \n\t"
	"movl %%eax, %1; \n\t"
	: "=m"(s1), "=m"(s2)
	);

	if (0 == s1 && 0 == s2)
	{
		throw std::logic_error("get cpuid failed!!!");
	}

	char cpu[32] = { 0 };
	snprintf(cpu, sizeof(cpu), "%08X%08X", htonl(s2), htonl(s1));
	std::string(cpu).swap(cpu_id);
	return (true);
}

char* getName(char* name, char* p)
{
	while (isspace(*p))
		p++;
	while (*p)
	{
		if (isspace(*p))
			break;
		if (*p == ':')
		{    /* could be an alias */
			char* dot = p, * dotname = name;
			*name++ = *p++;
			while (isdigit(*p))
				*name++ = *p++;
			if (*p != ':')
			{    /* it wasn't, backup */
				p = dot;
				name = dotname;
			}
			if (*p == '\0')
				return NULL;
			p++;
			break;
		}
		*name++ = *p++;
	}
	*name++ = '\0';
	return p;
}

std::string getIpFromEthx(const std::string& interface, std::string& macaddr)
{
	if (interface.size() == 0)
	{
		throw std::logic_error("input the eroor interface");
	}

	struct sockaddr_in* addr;
	struct ifreq ifr;
	int sockfd;

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	strncpy(ifr.ifr_name, interface.c_str(), IFNAMSIZ - 1);
	if (ioctl(sockfd, SIOCGIFADDR, &ifr) == -1)
	{
		return std::numeric_limits<std::string>::quiet_NaN();
	}
	addr = (struct sockaddr_in*)&(ifr.ifr_addr);
	std::string ip(inet_ntoa(addr->sin_addr));

	if (ioctl(sockfd, SIOCGIFHWADDR, &ifr) == -1)
	{
		return std::numeric_limits<std::string>::quiet_NaN();
	}
	unsigned char* mac;
	mac = (unsigned char*)(ifr.ifr_hwaddr.sa_data);
	char mac_addr[32] = {};
	sprintf(mac_addr, "%2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x\0", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	macaddr.assign(mac_addr);
	return ip;
}

#endif

std::vector<std::string> TC_Device::getLocalHosts()
{
	static vector<string> hosts;
	if (!hosts.empty())
	{
		return hosts;
	}

#if TARGET_PLATFORM_WINDOWS
	WORD wVersionRequested = MAKEWORD(2, 2);

	WSADATA wsaData;
	if (WSAStartup(wVersionRequested, &wsaData) != 0)
		return hosts;

	char local[255] = { 0 };
	gethostname(local, sizeof(local));
	hostent* ph = gethostbyname(local);
	if (ph == NULL)
		return hosts;
	in_addr addr;
	if (ph->h_addrtype == AF_INET)
	{
		int i = 0;
		while (ph->h_addr_list[i] != 0)
		{
			addr.s_addr = *(u_long*)ph->h_addr_list[i++];
			hosts.emplace_back(inet_ntoa(addr));
		}
	}
	else
	{
		return hosts;
	}
	WSACleanup();
	return hosts;

#else
	auto net_cards = getAllNetCard();
	if (net_cards.empty())
	{
		return hosts;
	}

	for (auto& item : net_cards)
	{
		hosts.push_back(item.second.str_ip_);
	}
	return hosts;
#endif
}

std::unordered_map<std::string, taf::MAC_INFO> TC_Device::getAllNetCard()
{
	static std::unordered_map<std::string, taf::MAC_INFO> interface_ip_mac;
	if (!interface_ip_mac.empty())
	{
		return interface_ip_mac;
	}

#if TARGET_PLATFORM_WINDOWS
	interface_ip_mac.clear();
	static std::vector<MAC_INFO> s_vecMac;
	if (s_vecMac.size() > 0)
	{
		for (const auto& it : s_vecMac)
			interface_ip_mac[it.str_description_] = it;
		return interface_ip_mac;
	}
	MasterHardDiskSerial mhds;
	mhds.getAllMAC(s_vecMac);
	for (const auto& it : s_vecMac)
		interface_ip_mac[it.str_description_] = it;
	return interface_ip_mac;
#else
	FILE* fh;
	char buf[512] = { 0 };
	fh = fopen("/proc/net/dev", "r");
	if (!fh)
	{
		throw std::logic_error("get localhosts failed");
	}

	/* eat title lines */
	fgets(buf, sizeof buf, fh);
	fgets(buf, sizeof buf, fh);
	int icnt = 1;
	while (fgets(buf, sizeof buf, fh))
	{
		char name[IFNAMSIZ] = { 0 };
		getName(name, buf);
		std::string interface(name);
		if (interface == "lo")
		{
			continue;
		}
		std::string macaddr;
		auto ip = getIpFromEthx(interface, macaddr);
		if (ip == std::numeric_limits<std::string>::quiet_NaN())
		{
			continue;
		}
		taf::MAC_INFO info;
		info.str_ip_ = ip;
		info.str_mac_ = macaddr;
		info.str_name_ = interface;
		interface_ip_mac.insert(std::make_pair(interface, info));
	}
	fclose(fh);
	return interface_ip_mac;
#endif
}

std::string TC_Device::getLIP()
{
#if TARGET_PLATFORM_WINDOWS
	static std::string localIP;
	if (!localIP.empty())
	{
		return localIP;
	}
	WORD wVersionRequested = MAKEWORD(2, 2);

	WSADATA wsaData;
	if (WSAStartup(wVersionRequested, &wsaData) != 0)
		return "";

	char local[255] = { 0 };
	gethostname(local, sizeof(local));
	hostent* ph = gethostbyname(local);
	if (ph == NULL)
		return "";

	in_addr addr;
	memcpy(&addr, ph->h_addr_list[0], sizeof(in_addr)); // 这里仅获取第一个ip

	localIP.assign(inet_ntoa(addr));

	WSACleanup();
	return localIP;
#elif TARGET_PLATFORM_LINUX
	auto net_card = getAllPhysicalNetCard();
	if (!net_card.empty())
	{
		return net_card.begin()->second.str_ip_;
	}
	return "";
#else
	throw std::logic_error("not complete");
#endif
}

taf::MAC_INFO TC_Device::getFirstActiveNetCard()
{
#if TARGET_PLATFORM_WINDOWS
	static taf::MAC_INFO ActiveMac;
	if (!ActiveMac.str_mac_.empty())
	{
		return ActiveMac;
	}
	MasterHardDiskSerial mhds;
	mhds.getFirstActiveMAC(ActiveMac);
	return ActiveMac;
#elif TARGET_PLATFORM_LINUX
	char buf_ps[128];
	std::string cmd = "ip a | grep \"state UP\" |sed -n \"1p\" | awk -F ':' '{print $2}' | sed 's/ //g'";
	FILE* ptr = NULL;

	if ((ptr = popen(cmd.c_str(), "r")) != NULL)
	{
		if (fgets(buf_ps, 128, ptr) == NULL)
		{
			std::logic_error("get all active net card failed!!!");
		}
		pclose(ptr);
		ptr = NULL;
	}
//	auto interface_ip_mac = getLocalHosts();
	std::string info(buf_ps);
	std::string name = info.substr(0, info.length() - 1);
	auto tmp = getAllNetCard();
	if (tmp.find(name) != tmp.end())
	{
		return tmp.at(name);
	}
	else
	{
		return taf::MAC_INFO();
	}

#else
	throw std::logic_error("not complete");
#endif
}

taf::MAC_INFO TC_Device::getFristEthernetNetCard()
{
#if TARGET_PLATFORM_WINDOWS
	static taf::MAC_INFO FristEthernetMac;
	if (!FristEthernetMac.str_mac_.empty())
	{
		return FristEthernetMac;
	}
	MasterHardDiskSerial mhds;
	mhds.getFristEthernetMac(FristEthernetMac);
	return FristEthernetMac;
#elif TARGET_PLATFORM_LINUX
	auto net_cards = getAllPhysicalNetCard();
	if (!net_cards.empty())
	{
		return net_cards.begin()->second;
	}
	else
	{
		return taf::MAC_INFO();
	}
#else
	throw std::logic_error("not complete");
#endif
}

std::unordered_map<std::string, taf::MAC_INFO> TC_Device::getAllPhysicalNetCard()
{
	static std::unordered_map<std::string, taf::MAC_INFO> net_card;
	if (!net_card.empty())
	{
		return net_card;
	}

#if TARGET_PLATFORM_WINDOWS
	net_card.clear();
	static std::vector<MAC_INFO> s_vecMac;
	if (s_vecMac.size() > 0)
	{
		for (const auto& it : s_vecMac)
			net_card[it.str_description_] = it;
		return net_card;
	}
	MasterHardDiskSerial mhds;
	mhds.getAllPhysicalMAC(s_vecMac);
	for (const auto& it : s_vecMac)
		net_card[it.str_description_] = it;
	return net_card;
#elif TARGET_PLATFORM_LINUX

	char buf_ps[128];
	std::string cmd = "ls /sys/class/net/ |grep -v \"`ls /sys/devices/virtual/net/`\"";
	FILE* ptr = NULL;
	if ((ptr = popen(cmd.c_str(), "r")) != NULL)
	{
		if (fgets(buf_ps, 128, ptr) == NULL)
		{
			std::logic_error("get all physical mac failed!!!");
		}
		pclose(ptr);
		ptr = NULL;
	}
	auto interface_ip_mac = getLocalHosts();

	//获取到的字符串中换行符需要进行处理
	std::string info(buf_ps);
	auto interfaces = taf::TC_Common::sepstr<std::string>(info.substr(0, info.length() - 1), " ");
	interfaces.erase(std::remove(interfaces.begin(), interfaces.end(), "lo"), interfaces.end());
	auto tmp = getAllNetCard();
	for (auto& name : interfaces)
	{
		auto iter = tmp.find(name);
		if (iter != tmp.end())
		{

			net_card.insert(std::make_pair(name, iter->second));
		}
		else
		{
			continue;
		}
	}
	return net_card;

#else
	throw std::logic_error("not complete");
#endif
}

std::tuple<std::string, std::string> TC_Device::getHD()
{
#if TARGET_PLATFORM_WINDOWS
	MasterHardDiskSerial mhds;
	std::string serialNumber = "";
	mhds.getSerialNo(serialNumber);
	return std::make_tuple(" ", serialNumber);
#elif TARGET_PLATFORM_LINUX
	auto pi = taf::TC_Common::sepstr<string>(getPI(), "^");
	std::string name = pi.front();
	std::vector<std::string> result;

	char buf_ps[128];
	std::string cmd = "/sbin/udevadm info --query=property --name=" + name + "|grep -E \"ID_SERIAL_SHORT=|ID_MODEL=\"";
	FILE* ptr = NULL;
	if ((ptr = popen(cmd.c_str(), "r")) != NULL)
	{
		while (fgets(buf_ps, 128, ptr) != NULL)
		{
			std::string info(buf_ps);
			auto tmp = taf::TC_Common::sepstr<std::string>(info.substr(0, info.length() - 1), "=");
			result.push_back(tmp[1]);
			memset(buf_ps, 0, sizeof(buf_ps));
		}
		if (result.size() != 2)
		{
			throw std::logic_error("get HD failed");
		}
		pclose(ptr);
		ptr = NULL;
	}
	return std::make_tuple(result[0], result[1]);


/*
	int fd = open(PI.front().c_str(), open_flags);
	if (fd < 0)
	{
		throw std::logic_error("open the dev file failed, please checkout the existence of " + PI.front() + ", if exist, try use root right to excute!!!");
	}

	ioctl(fd, HDIO_GET_IDENTITY, &id);
	printf("Model = %s\nFwRev = %s\nSerialNo = %s\n",id.model,id.fw_rev,id.serial_no);
	std::string model;
	std::string serial_id;
	//长度是固定的不要随意修改
	for (int i = 0; i < 40; i++)
	{
		char a = id.model[i];
		if (a == ' ')
		{
			continue;
		}
		model += a;
	}

	for (int i = 0; i < 20; i++)
	{
		char a = id.serial_no[i];
		if (a == ' ')
		{
			continue;
		}
		serial_id += a;
	}*/

//	return std::make_tuple(model, serial_id);
#else
	//	throw std::logic_error("not complete");
#endif
}

std::string TC_Device::getPI()
{
#if TARGET_PLATFORM_WINDOWS
	vector<PartitionInfo> partition_info;
	int count = readDisk(partition_info);
	stringstream stream;
	string pi_info;
	for (auto& it : partition_info)
	{
		if (it.name.empty())
		{
			continue;
		}
		stream << it.name << "^" << "^"<< GB(it.totalBytes) << "G,";
	}
	pi_info = stream.str();
	pi_info.pop_back();
	return pi_info;

#elif TARGET_PLATFORM_LINUX
	char buf_ps[128];
	std::string cmd = R"(df -Tlh  -x tmpfs -x devtmpfs | awk -F ' ' '{if($7 == "/") printf("%s^%s^%s",$1,$2,$3)}')";
	FILE* ptr = NULL;
	if ((ptr = popen(cmd.c_str(), "r")) != NULL)
	{
		if (fgets(buf_ps, 128, ptr) == NULL)
		{
			std::logic_error("get Unix OS PI!!!");
		}
		pclose(ptr);
		ptr = NULL;
	}
	std::string info(buf_ps);
	return info;
#else
	throw std::logic_error("not complete");

#endif
}

#if TARGET_PLATFORM_WINDOWS

// 读取磁盘,返回获取到的分区数量
int TC_Device::readDisk(vector<PartitionInfo>& partition_info)
{
	int DiskCount = 0;
	DWORD DiskInfo = GetLogicalDrives(); //获取系统中逻辑驱动器的数量，函数返回的是一个32位无符号整型数据。

	while (DiskInfo)
	{//通过循环操作查看每一位数据是否为1，如果为1则磁盘为真,如果为0则磁盘不存在。
		if (DiskInfo & 1)//通过位运算的逻辑与操作，判断是否为1
			++DiskCount;

		DiskInfo = DiskInfo >> 1;//通过位运算的右移操作保证每循环一次所检查的位置向右移动一位。
	}
	DWORD dw = GetLogicalDriveStrings(0, NULL);
	LPTSTR lpDriveStrings = (LPTSTR)HeapAlloc(GetProcessHeap(), 0, dw * sizeof(TCHAR));
	GetLogicalDriveStrings(dw, lpDriveStrings);

	// 存储分区信息
	partition_info.resize(DiskCount);

	// 循环将分区信息赋值给分区信息列表
	while (NULL != lpDriveStrings && '\0' != *lpDriveStrings)
	{
		partition_info.push_back(getPartitionInfo(lpDriveStrings));
		lpDriveStrings += _tcslen(lpDriveStrings) + 1;
	}

	// 返回分区数目
	return DiskCount;
}

// 获取分区信息
PartitionInfo TC_Device::getPartitionInfo(LPTSTR lpDriveStrings)
{
	PartitionInfo pi;
	pi.name = *lpDriveStrings;

	int DiskType; // 磁盘类型
	BOOL fResult;
	ULARGE_INTEGER i64FreeBytesToCaller;
	ULARGE_INTEGER i64TotalBytes;
	ULARGE_INTEGER i64FreeBytes;

	fResult = GetDiskFreeSpaceEx(
	lpDriveStrings,
	&i64FreeBytesToCaller,
	&i64TotalBytes,
	&i64FreeBytes);
	//GetDiskFreeSpaceEx函数，可以获取驱动器磁盘的空间状态,函数返回的是个BOOL类型数据

	if (fResult)
	{//通过返回的BOOL数据判断驱动器是否在工作状态
		pi.totalBytes = i64TotalBytes;
		pi.freeBytes = i64FreeBytesToCaller;
	}

	return pi;
}

#endif

std::string TC_Device::getCPU()
{
	static std::string cpuid;
	if (!cpuid.empty())
	{
		return cpuid;
	}
#if TARGET_PLATFORM_WINDOWS
	get_cpuId(cpuid);
	return cpuid;
#elif TARGET_PLATFORM_LINUX
	get_cpuId(cpuid);
	return cpuid;
#else
	throw std::logic_error("not complete");
#endif
}

#if (TARGET_PLATFORM_LINUX || TARGET_PLATFORM_IOS)

std::tuple<std::string, std::string> TC_Device::getUnixOsInfo()
{
	char buf_ps[128];
	std::string cmd = "uname -a|awk -F ' ' '{printf(\"%s %s\",$1,$3)}'";
	FILE* ptr = NULL;
	if ((ptr = popen(cmd.c_str(), "r")) != NULL)
	{
		if (fgets(buf_ps, 128, ptr) == NULL)
		{
			std::logic_error("get Unix OS info failed!!!");
		}
		pclose(ptr);
		ptr = NULL;
	}
	std::string info(buf_ps);
	auto pos = info.find(' ');
	if (pos == std::string::npos)
	{
		throw std::logic_error("get Unix OS info failed!!!");
	}
	auto osname = info.substr(0, pos);
	auto osversion = info.substr(pos + 1);

	return std::make_tuple(osname, osversion);
}

#endif

std::string TC_Device::getPCN()
{
	std::string osname = "";
#if TARGET_PLATFORM_WINDOWS
	stringstream stream;
#ifdef UNICODE
	HINSTANCE hinst = LoadLibrary(L"ntdll.dll");
#else
	HINSTANCE hinst = LoadLibrary("ntdll.dll");
#endif // !UNICODE

	DWORD dwMajor, dwMinor, dwBuildNumber;
	NTPROC proc = (NTPROC)GetProcAddress(hinst, "RtlGetNtVersionNumbers");
	proc(&dwMajor, &dwMinor, &dwBuildNumber);
	dwBuildNumber &= 0xffff;
	stream << "Windows " << dwMajor;
	FreeLibrary(hinst);
	osname = stream.str();
	return osname;
#else
	auto osinfo = getUnixOsInfo();
	return std::get<0>(osinfo);
#endif
}

std::string TC_Device::getOSV()
{
#if TARGET_PLATFORM_WINDOWS
	static std::string str_osv;
	if (!str_osv.empty())
	{
		return str_osv;
	}
	OSVERSIONINFO Version;
	Version.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&Version);
	str_osv = std::to_string(Version.dwMajorVersion) + "." + std::to_string(Version.dwMinorVersion) + "."
	+ std::to_string(Version.dwBuildNumber);
	return str_osv;
#else
	auto osinfo = getUnixOsInfo();
	return std::get<1>(osinfo);
#endif
}

std::string TC_Device::getIMEI()
{
#if TARGET_PLATFORM_WINDOWS
	static std::string strSystemSerialNo;
	if (!strSystemSerialNo.empty())
	{
		return strSystemSerialNo;
	}
	MasterHardDiskSerial mhds;
	mhds.getSerialNo(strSystemSerialNo);
	return strSystemSerialNo;
#else
	return get<1>(getHD());
	throw std::logic_error("not complete");
#endif
}

std::string TC_Device::getVOL()
{
#if TARGET_PLATFORM_WINDOWS
	static std::string str_vol;
	if (!str_vol.empty())
	{
		return str_vol;
	}
	char szTmp[MAX_PATH] = { 0 };
	GetSystemDirectoryA(szTmp, MAX_PATH);
	str_vol = szTmp[0];
	return str_vol;
#else
	//暂时不支持
	return "";
#endif
}

#if TARGET_PLATFORM_WINDOWS

// 参考链接:
// https://docs.microsoft.com/en-us/windows/win32/wmisdk/example--getting-wmi-data-from-the-local-computer
int TC_Device::WmiQuery(const std::string& key, std::string& val)
{
	HRESULT hres;

	// Step 1: --------------------------------------------------
	// Initialize COM. ------------------------------------------

	hres = CoInitializeEx(0, COINIT_MULTITHREADED);
	if (FAILED(hres))
	{
//	        std::cout << "Failed to initialize COM library. Error code = 0x"
//	            << hex << hres;
		return 1;                  // Program has failed.
	}

	/*
	// Step 2: --------------------------------------------------
	// Set general COM security levels --------------------------

	hres =  CoInitializeSecurity(
		NULL,
		-1,                          // COM authentication
		NULL,                        // Authentication services
		NULL,                        // Reserved
		RPC_C_AUTHN_LEVEL_DEFAULT,   // Default authentication
		RPC_C_IMP_LEVEL_IMPERSONATE, // Default Impersonation
		NULL,                        // Authentication info
		EOAC_NONE,                   // Additional capabilities
		NULL                         // Reserved
		);


	if (FAILED(hres))
	{
		FW_LOG_INFOA << "Failed to initialize security. Error code = 0x"
			<< hex << hres;
		CoUninitialize();
		return 1;                    // Program has failed.
	}*/

	// Step 3: ---------------------------------------------------
	// Obtain the initial locator to WMI -------------------------

	IWbemLocator* pLoc = NULL;

	hres = CoCreateInstance(
	CLSID_WbemLocator,
	0,
	CLSCTX_INPROC_SERVER,
	IID_IWbemLocator, (LPVOID*)&pLoc);

	if (FAILED(hres))
	{
//	        FW_LOG_INFOA << "Failed to create IWbemLocator object."
//	            << " Err code = 0x"
//	            << hex << hres;
		CoUninitialize();
		return 1;                 // Program has failed.
	}

	// Step 4: -----------------------------------------------------
	// Connect to WMI through the IWbemLocator::ConnectServer method

	IWbemServices* pSvc = NULL;

	// Connect to the root\cimv2 namespace with
	// the current user and obtain pointer pSvc
	// to make IWbemServices calls.
	hres = pLoc->ConnectServer(
	_bstr_t(L"ROOT\\CIMV2"), // Object path of WMI namespace
	NULL,                    // User name. NULL = current user
	NULL,                    // User password. NULL = current
	0,                       // Locale. NULL indicates current
	NULL,                    // Security flags.
	0,                       // Authority (for example, Kerberos)
	0,                       // Context object
	&pSvc                    // pointer to IWbemServices proxy
	);

	if (FAILED(hres))
	{
//	        FW_LOG_INFOA << "Could not connect. Error code = 0x"
//	             << hex << hres;
		pLoc->Release();
		CoUninitialize();
		return 1;                // Program has failed.
	}

//	    FW_LOG_INFOA << "Connected to ROOT\\CIMV2 WMI namespace";


	// Step 5: --------------------------------------------------
	// Set security levels on the proxy -------------------------

	hres = CoSetProxyBlanket(
	pSvc,                        // Indicates the proxy to set
	RPC_C_AUTHN_WINNT,           // RPC_C_AUTHN_xxx
	RPC_C_AUTHZ_NONE,            // RPC_C_AUTHZ_xxx
	NULL,                        // Server principal name
	RPC_C_AUTHN_LEVEL_CALL,      // RPC_C_AUTHN_LEVEL_xxx
	RPC_C_IMP_LEVEL_IMPERSONATE, // RPC_C_IMP_LEVEL_xxx
	NULL,                        // client identity
	EOAC_NONE                    // proxy capabilities
	);

	if (FAILED(hres))
	{
//	        FW_LOG_INFOA << "Could not set proxy blanket. Error code = 0x"
//	            << hex << hres;
		pSvc->Release();
		pLoc->Release();
		CoUninitialize();
		return 1;               // Program has failed.
	}

	// Step 6: --------------------------------------------------
	// Use the IWbemServices pointer to make requests of WMI ----

	// For example, get the name of the operating system
	IEnumWbemClassObject* pEnumerator = NULL;
	hres = pSvc->ExecQuery(
	bstr_t("WQL"),
	//bstr_t("SELECT * FROM Win32_OperatingSystem"),
	bstr_t("SELECT * FROM Win32_ComputerSystem"),
	WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
	NULL,
	&pEnumerator);

	if (FAILED(hres))
	{
//	        FW_LOG_INFOA << "Query for operating system name failed."
//	            << " Error code = 0x"
//	            << hex << hres;
		pSvc->Release();
		pLoc->Release();
		CoUninitialize();
		return 1;               // Program has failed.
	}

	// Step 7: -------------------------------------------------
	// get the data from the query in step 6 -------------------

	IWbemClassObject* pclsObj = NULL;
	ULONG uReturn = 0;

	while (pEnumerator)
	{
		HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1,
		&pclsObj, &uReturn);

		if (0 == uReturn)
		{
			break;
		}

		VARIANT vtProp;

		// get the value of the Name property
		//hr = pclsObj->get(L"Name", 0, &vtProp, 0, 0);
		//wcout << " OS Name : " << vtProp.bstrVal << endl;
		hr = pclsObj->Get(utf8_to_wstring(key).c_str(), 0, &vtProp, 0, 0);
		if (vtProp.bstrVal)
		{
			val = wstring_to_utf8(vtProp.bstrVal);
		}
//	        FW_LOG_INFOA << key << "|" << val << endl;
		VariantClear(&vtProp);

		pclsObj->Release();
	}

	// Cleanup
	// ========

	pSvc->Release();
	pLoc->Release();
	pEnumerator->Release();
	CoUninitialize();

	return 0;   // Program successfully completed.
}

#endif

std::string TC_Device::getVendor()
{
#if TARGET_PLATFORM_WINDOWS
	std::string manufacturer;
	TC_Device::WmiQuery("Manufacturer", manufacturer);
	return manufacturer;
#elif TARGET_PLATFORM_LINUX
	char buf_ps[128];
	std::string cmd = "cat /sys/class/dmi/id/board_vendor";
	FILE* ptr = NULL;
	if ((ptr = popen(cmd.c_str(), "r")) != NULL)
	{
		if (fgets(buf_ps, 128, ptr) == NULL)
		{
			std::logic_error("get product vendor info failed!!!");
		}
		pclose(ptr);
		ptr = NULL;
	}
	std::string info(buf_ps);
	return info.substr(0, info.length() - 1);
#else
	throw std::logic_error("not complete");
#endif
}

std::string TC_Device::getModel()
{
#if TARGET_PLATFORM_WINDOWS
	std::string model;
	TC_Device::WmiQuery("Model", model);
	return model;
#elif TARGET_PLATFORM_LINUX
	char buf_ps[128];
	std::string cmd = "cat /sys/class/dmi/id/board_name";
	FILE* ptr = NULL;
	if ((ptr = popen(cmd.c_str(), "r")) != NULL)
	{
		if (fgets(buf_ps, 128, ptr) == NULL)
		{
			std::logic_error("get product model info failed!!!");
		}
		pclose(ptr);
		ptr = NULL;
	}
	std::string info(buf_ps);
	return info.substr(0, info.length() - 1);
#else
	throw std::logic_error("not complete");
#endif
}
}

