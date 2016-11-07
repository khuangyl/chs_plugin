// choice1.cpp : Defines the entry point for the DLL application.
// ���ʵ��

#include "stdafx.h"
#include <stdio.h>
#include "plugin.h"
#include "TCalcFuncSets.h"
#define PLUGIN_EXPORTS

char *pDataPath = "D:\\new_zx_allin1";

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
    }
    return TRUE;
}

PDATAIOFUNC	 g_pFuncCallBack;

BOOL Handle30MinData(char * Code, short nSetCode, short DataType);
//��ȡ�ص�����
void RegisterDataInterface(PDATAIOFUNC pfn)
{
	g_pFuncCallBack = pfn;
}

//ע������Ϣ
void GetCopyRightInfo(LPPLUGIN info)
{
	//��д������Ϣ
	strcpy(info->Name,"��MA�ߴ�Խ");
	strcpy(info->Dy,"�人");
	strcpy(info->Author,"ϵͳ");
	strcpy(info->Period,"����");
	strcpy(info->Descript,"��MA�ߴ�Խ");
	strcpy(info->OtherInfo,"�Զ���������MA�߽�洩Խ");
	//��д������Ϣ
	info->ParamNum = 2;
	strcpy(info->ParamInfo[0].acParaName,"MA����1");
	info->ParamInfo[0].nMin=5000;
	info->ParamInfo[0].nMax=10000;
	info->ParamInfo[0].nDefault=5000;
	strcpy(info->ParamInfo[1].acParaName,"MA����2");
	info->ParamInfo[1].nMin=5000;
	info->ParamInfo[1].nMax=10000;
	info->ParamInfo[1].nDefault=5000;
}

////////////////////////////////////////////////////////////////////////////////
struct PriceMap
{
	DWORD	nBase;
	DWORD	nNowPrice;
	DWORD   nOffset;
};

int g_nmapSize = 5;
PriceMap g_mapPrice[] = 
{
	{0,   0x3F800000, 0x400000},
	{4,   0x40800000, 0x200000},
	{8,   0x41000000, 0x100000},
	{16,  0x41800000, 0x80000},
	{32,  0x42000000, 0x40000},
	{64,  0x42800000, 0x20000},
	{128, 0x43000000, 0x10000},
	{256, 0x43800000, 0x8000}
};

float GetRealPrice(DWORD dwOrgPrice)
{
	int Index = 0;
	for(Index = g_nmapSize-1; Index >= 0; Index--)
	{
		if(dwOrgPrice >= g_mapPrice[Index].nNowPrice)
		{
			float fRet = (float)g_mapPrice[Index].nBase + (float)(dwOrgPrice - g_mapPrice[Index].nNowPrice)/(float)g_mapPrice[Index].nOffset;
			return fRet;
		}
	}

	return 0;
}

BOOL InputInfoThenCalc1(char * Code,short nSetCode,int Value[4],short DataType,short nDataNum,BYTE nTQ,unsigned long unused) //��������ݼ���
{
	if(strcmp("300274", Code) == 0)
	{
		if(IsDebuggerPresent() == TRUE)
		{
			//__asm int 3
		}
	}

	if(PER_MONTH == DataType)
	{
		//�����Ѿ���30���ӵ�����ռ���ˣ���Ϊ30���ӵ����ݲ���ֱ���У�������Ҫ
		return Handle30MinData(Code, nSetCode, DataType);
	}

	char sztmp[128] = {0};
	sprintf(sztmp, "[chs] Code=%s", Code);

	//if(strcmp(Code, "600119") == 0 || strcmp(Code, "300107") == 0)
	if(strcmp(Code, "300372") == 0 )
	{
		return FALSE;
	}
	OutputDebugStringA(sztmp);
	BOOL nRet = FALSE;
	
	char szPath[MAX_PATH] = {0};

	if(DataType == PER_MIN5 || DataType == PER_HOUR || DataType == PER_WEEK)//1Сʱ�ľ���Ԥ��5����
	{
		//5����
		if(nSetCode == 0)
		{
			//����
			sprintf(szPath, "%s\\vipdoc\\sz\\fzline\\sz%s.lc5", pDataPath, Code);
		}
		else
		{
			//����
			sprintf(szPath, "%s\\vipdoc\\sh\\fzline\\sh%s.lc5", pDataPath, Code);

		}
	}
	else if(DataType == PER_MIN1 || DataType == PER_MIN30 || DataType == PER_DAY)//30���Ӿ���Ԥ��1����
	{
		//1����
		if(nSetCode == 0)
		{
			//����
			sprintf(szPath, "%s\\vipdoc\\sz\\minline\\sz%s.lc1", pDataPath, Code);
		}
		else
		{
			//����
			sprintf(szPath, "%s\\vipdoc\\sh\\minline\\sh%s.lc1", pDataPath, Code);
		}
	}
	
	{
		char szTempp[MAX_PATH] = {0};
		sprintf(szTempp, "[chs] szPath=%s", szPath);
		OutputDebugStringA(szTempp);
	}

	
	HANDLE hFile = CreateFile(szPath,GENERIC_READ, FILE_SHARE_READ,NULL, OPEN_EXISTING, FILE_ATTRIBUTE_ARCHIVE, NULL);

	if(hFile != INVALID_HANDLE_VALUE)
	{
		DWORD dwHigh = 0;
		DWORD dwSize = GetFileSize(hFile, &dwHigh);

		char *szBuff  = new char[dwSize];

		DWORD dwOut = 0;
		ReadFile(hFile, szBuff, dwSize, &dwOut, NULL);

		int nCount = dwSize/32;

		

		float *pHigh = new float[nCount];
		float *pLow = new float[nCount];

		char *pbuf = szBuff;

		for (int n = 0; n < nCount; n++)
		{
			float f8 = GetRealPrice((*(DWORD*)&pbuf[8]));//��߼�
			float f12 = GetRealPrice((*(DWORD*)&pbuf[12]));//��ͼ�
			
			if(f8 > 512)
			{
				delete[] szBuff;
				delete[] pHigh;
				delete[] pLow;
				CloseHandle(hFile);
				return FALSE;
			}
			pHigh[n] = f8;
			pLow[n] = f12;

			pbuf = pbuf + 32;
		}

		float *pLowEx  = pLow;
		float *pHighEx = pHigh;
		//if(nCount > 20000)
		//{
		//	int nRemain = nCount - 20000;
		//	pLowEx = (float *)((char*)pLow + nRemain * 4);
		//	pHighEx = (float *)((char*)pHigh + nRemain * 4);
		//	nCount = 20000;
		//}

		__try
		{
			TestPlugin3( nCount, pHighEx, pLowEx);
			TestPlugin4( nCount, pHighEx, pLowEx);
			TestPlugin5( nCount, pHighEx, pLowEx);

			BOOL bRet = FALSE;
			
			if(DataType == PER_MIN1)
			{
				//1����
				bRet = ZhongShuAnalu_BeiLi();
			}
			else if(DataType == PER_MIN5)
			{
				//5_����
				bRet = ZhongShuAnalu_BeiLi();
				//bRet = ZhongShuAnalu_BeiLi_5Min();
			}
			else if(PER_MIN30 == DataType || DataType == PER_HOUR)
			{
				//1��Сʱ��Ԥ���
				bRet = ZhongShuAnaly_YuCe();
			}
			else if(DataType == PER_DAY)
			{
				bRet = ZhongShuAnaly_TuPo();//1���ӵ���k���������ص���
			}
			else if(DataType == PER_WEEK)
			{
				bRet = ZhongShuAnaly_TuPo(); //5���ӵ���k��û�������ص���
			}


			CloseHandle(hFile);
			delete[] szBuff;
			delete[] pHigh;
			delete[] pLow;
			return bRet;
		}
		__except(1)
		{
			OutputDebugStringA("[chs] ѡ�ɵ�ʱ������쳣");
			//if(IsDebuggerPresent())
			//{
			//	__asm int 3
			//}
			delete[] szBuff;
			delete[] pHigh;
			delete[] pLow;
			CloseHandle(hFile);
			return FALSE;
		}


	}
	else
	{
		char szbuff[256] = {0};
		sprintf(szbuff,"[chs] �򲻿��ļ� ������=%d", GetLastError());
		OutputDebugStringA(szbuff);
	}
	
	return FALSE;
}

BOOL InputInfoThenCalc2(char * Code,short nSetCode,int Value[4],short DataType,NTime time1,NTime time2,BYTE nTQ,unsigned long unused)  //ѡȡ����
{
	BOOL nRet = FALSE;
	
	return nRet;
}


//--------------------------------------------------------30���ӵĴ���-------------------------------------------------------------------
//����30����û��ֱ�ӵ����ݣ�������Ҫ��5���ӵ����ݣ������ϲ��õ�30���ӵ�����

BOOL Handle30MinData(char * Code, short nSetCode, short DataType)
{
	char sztmp[128] = {0};
	sprintf(sztmp, "[chs] Code=%s", Code);

	if(strcmp(Code, "300372") == 0 )
	{
		return FALSE;
	}
	OutputDebugStringA(sztmp);
	BOOL nRet = FALSE;

	char szPath[MAX_PATH] = {0};

	if(DataType == PER_MONTH)//1Сʱ�ľ���Ԥ��5����
	{
		//5����
		if(nSetCode == 0)
		{
			//����
			sprintf(szPath, "%s\\vipdoc\\sz\\fzline\\sz%s.lc5", pDataPath, Code);
		}
		else
		{
			//����
			sprintf(szPath, "%s\\vipdoc\\sh\\fzline\\sh%s.lc5", pDataPath, Code);

		}
	}
	

	{
		char szTempp[MAX_PATH] = {0};
		sprintf(szTempp, "[chs] szPath=%s", szPath);
		OutputDebugStringA(szTempp);
	}


	HANDLE hFile = CreateFile(szPath,GENERIC_READ, FILE_SHARE_READ,NULL, OPEN_EXISTING, FILE_ATTRIBUTE_ARCHIVE, NULL);

	if(hFile != INVALID_HANDLE_VALUE)
	{
		DWORD dwHigh = 0;
		DWORD dwSize = GetFileSize(hFile, &dwHigh);

		char *szBuff  = new char[dwSize];

		DWORD dwOut = 0;
		ReadFile(hFile, szBuff, dwSize, &dwOut, NULL);

		int nCount = dwSize/32;



		float *pHigh = new float[nCount];
		float *pLow = new float[nCount];

		char *pbuf = szBuff;

		int nnnkkk = 0;//������������ļ�������5���ӵ�����Ҫ�ϲ���30���ӵ����ݾ���Ҫ������

		float fMax = 0;
		float fMin = 10000.01;

		for (int n = 0; n < nCount; n++)
		{
			float f8 = GetRealPrice((*(DWORD*)&pbuf[8]));//��߼�
			float f12 = GetRealPrice((*(DWORD*)&pbuf[12]));//��ͼ�

			fMax = max(fMax, f8);
			fMin = min(fMin, f12);

			//�����ȡʱ��
			int ntime = (*(WORD*)&pbuf[2])%60;
			if(ntime == 0 || ntime == 30)
			{
				pHigh[nnnkkk] = fMax;
				pLow[nnnkkk] = fMin;
				nnnkkk++;
				

				fMax = 0;
				fMin = 10000.01;
			}
			
			if(f8 > 512)
			{
				delete[] szBuff;
				delete[] pHigh;
				delete[] pLow;
				CloseHandle(hFile);
				return FALSE;
			}
			//pHigh[n] = f8;
			//pLow[n] = f12;

			pbuf = pbuf + 32;
		}

		float *pLowEx  = pLow;
		float *pHighEx = pHigh;

		nCount = nnnkkk;
		//if(nCount > 20000)
		//{
		//	int nRemain = nCount - 20000;
		//	pLowEx = (float *)((char*)pLow + nRemain * 4);
		//	pHighEx = (float *)((char*)pHigh + nRemain * 4);
		//	nCount = 20000;
		//}

		__try
		{
			TestPlugin3( nCount, pHighEx, pLowEx);
			TestPlugin4( nCount, pHighEx, pLowEx);
			TestPlugin5( nCount, pHighEx, pLowEx);

			BOOL bRet = FALSE;

			if(DataType == PER_MONTH)
			{
				//1����
				bRet = ZhongShuAnalu_BeiLi();
			}

			CloseHandle(hFile);
			delete[] szBuff;
			delete[] pHigh;
			delete[] pLow;
			return bRet;
		}
		__except(1)
		{
			OutputDebugStringA("[chs] ѡ�ɵ�ʱ������쳣");
			//if(IsDebuggerPresent())
			//{
			//	__asm int 3
			//}
			delete[] szBuff;
			delete[] pHigh;
			delete[] pLow;
			CloseHandle(hFile);
			return FALSE;
		}


	}
	else
	{
		char szbuff[256] = {0};
		sprintf(szbuff,"[chs] �򲻿��ļ� ������=%d", GetLastError());
		OutputDebugStringA(szbuff);
	}

	return FALSE;
}