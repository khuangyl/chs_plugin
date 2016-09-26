// choice1.cpp : Defines the entry point for the DLL application.
// 插件实例

#include "stdafx.h"
#include <stdio.h>
#include "plugin.h"
#include "TCalcFuncSets.h"
#define PLUGIN_EXPORTS

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

//获取回调函数
void RegisterDataInterface(PDATAIOFUNC pfn)
{
	g_pFuncCallBack = pfn;
}

//注册插件信息
void GetCopyRightInfo(LPPLUGIN info)
{
	//填写基本信息
	strcpy(info->Name,"两MA线穿越");
	strcpy(info->Dy,"武汉");
	strcpy(info->Author,"系统");
	strcpy(info->Period,"短线");
	strcpy(info->Descript,"两MA线穿越");
	strcpy(info->OtherInfo,"自定义天数两MA线金叉穿越");
	//填写参数信息
	info->ParamNum = 2;
	strcpy(info->ParamInfo[0].acParaName,"MA天数1");
	info->ParamInfo[0].nMin=5000;
	info->ParamInfo[0].nMax=10000;
	info->ParamInfo[0].nDefault=5000;
	strcpy(info->ParamInfo[1].acParaName,"MA天数2");
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
	{128, 0x43000000, 0x10000}
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

BOOL InputInfoThenCalc1(char * Code,short nSetCode,int Value[4],short DataType,short nDataNum,BYTE nTQ,unsigned long unused) //按最近数据计算
{
	if(strcmp("600703", Code) == 0)
	{
		if(IsDebuggerPresent() == TRUE)
		{
			__asm int 3
		}
	}
	char sztmp[128] = {0};
	sprintf(sztmp, "[chs] Code=%s", Code);

	if(strcmp(Code, "600119") == 0 || strcmp(Code, "300107") == 0)
	{
		return FALSE;
	}
	OutputDebugStringA(sztmp);
	BOOL nRet = FALSE;
	
	char szPath[MAX_PATH] = {0};

	if(DataType == PER_MIN5)
	{
		//5分钟
		if(nSetCode == 0)
		{
			//深市
			sprintf(szPath, "C:\\88-personal\\stock\\tdx\\vipdoc\\sz\\fzline\\sz%s.lc5", Code);
		}
		else
		{
			//沪市
			sprintf(szPath, "C:\\88-personal\\stock\\tdx\\vipdoc\\sz\\fzline\\sh%s.lc5", Code);

		}
	}
	else if(DataType == PER_MIN1)
	{
		//1分钟
		if(nSetCode == 0)
		{
			//深市
			sprintf(szPath, "C:\\88-personal\\stock\\tdx\\vipdoc\\sz\\minline\\sz%s.lc1", Code);
		}
		else
		{
			//沪市
			sprintf(szPath, "C:\\88-personal\\stock\\tdx\\vipdoc\\sh\\minline\\sh%s.lc1", Code);
		}
	}
	else if(DataType == PER_DAY)
	{
		//日k 暂时没有选股的
		return FALSE;
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
		//if(nCount > 6000)
		//{
		//	int nStart = nCount - 6000;
		//	pbuf = szBuff + nStart*32;
		//	nCount = 6000;
		//}

		for (int n = 0; n < nCount; n++)
		{
			float f8 = GetRealPrice((*(DWORD*)&pbuf[8]));//最高价
			float f12 = GetRealPrice((*(DWORD*)&pbuf[12]));//最低价
			
			if(f8 > 256)
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

		__try
		{
			TestPlugin3( nCount, pHigh, pLow);
			TestPlugin4( nCount, pHigh, pLow);
			TestPlugin5( nCount, pHigh, pLow);

			BOOL bRet = FALSE;
			
			if(DataType == PER_MIN1)
			{
				//1分钟
				bRet = ZhongShuAnalu_BeiLi();
			}
			else if(DataType == PER_MIN5)
			{
				//5_分钟
				//bRet = ZhongShuAnalu_BeiLi();
				bRet = ZhongShuAnalu_BeiLi_5Min();
			}

			CloseHandle(hFile);
			delete[] szBuff;
			delete[] pHigh;
			delete[] pLow;
			return bRet;
		}
		__except(1)
		{
			OutputDebugStringA("[chs] 选股的时候出现异常");
			//if(IsDebuggerPresent())
			//{
			//	__asm int 3
			//}
			delete[] szBuff;
			delete[] pHigh;
			delete[] pLow;
			return FALSE;
		}


	}
	
	return FALSE;
}

BOOL InputInfoThenCalc2(char * Code,short nSetCode,int Value[4],short DataType,NTime time1,NTime time2,BYTE nTQ,unsigned long unused)  //选取区段
{
	BOOL nRet = FALSE;
	
	return nRet;
}
