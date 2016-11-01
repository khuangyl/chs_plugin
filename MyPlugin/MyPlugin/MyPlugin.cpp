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
	if(strcmp("300274", Code) == 0)
	{
		if(IsDebuggerPresent() == TRUE)
		{
			//__asm int 3
		}
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

	if(DataType == PER_MIN5 || DataType == PER_HOUR)//1小时的就是预测5分钟
	{
		//5分钟
		if(nSetCode == 0)
		{
			//深市
			sprintf(szPath, "D:\\new_zx_allin1\\vipdoc\\sz\\fzline\\sz%s.lc5", Code);
		}
		else
		{
			//沪市
			sprintf(szPath, "D:\\new_zx_allin1\\vipdoc\\sh\\fzline\\sh%s.lc5", Code);

		}
	}
	else if(DataType == PER_MIN1 || DataType == PER_MIN30)//30分钟就是预测1分钟
	{
		//1分钟
		if(nSetCode == 0)
		{
			//深市
			sprintf(szPath, "D:\\new_zx_allin1\\vipdoc\\sz\\minline\\sz%s.lc1", Code);
		}
		else
		{
			//沪市
			sprintf(szPath, "D:\\new_zx_allin1\\vipdoc\\sh\\minline\\sh%s.lc1", Code);
		}
	}
	else if(DataType == PER_DAY)
	{
		//1分钟 //小时线只预测找出中枢上突破的模型
		if(nSetCode == 0)
		{
			//深市
			sprintf(szPath, "D:\\new_zx_allin1\\vipdoc\\sz\\minline\\sz%s.lc1", Code);
		}
		else
		{
			//沪市
			sprintf(szPath, "D:\\new_zx_allin1\\vipdoc\\sh\\minline\\sh%s.lc1", Code);
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

		float *pLowEx  = pLow;
		float *pHighEx = pHigh;
		//if(nCount > 15000)
		//{
		//	int nRemain = nCount - 15000;
		//	pLowEx = (float *)((char*)pLow + nRemain * 4);
		//	pHighEx = (float *)((char*)pHigh + nRemain * 4);
		//	nCount = 15000;
		//}

		__try
		{
			TestPlugin3( nCount, pHighEx, pLowEx);
			TestPlugin4( nCount, pHighEx, pLowEx);
			TestPlugin5( nCount, pHighEx, pLowEx);

			BOOL bRet = FALSE;
			
			if(DataType == PER_MIN1)
			{
				//1分钟
				bRet = ZhongShuAnalu_BeiLi();
			}
			else if(DataType == PER_MIN5)
			{
				//5_分钟
				bRet = ZhongShuAnalu_BeiLi();
				//bRet = ZhongShuAnalu_BeiLi_5Min();
			}
			else if(PER_MIN30 == DataType || DataType == PER_HOUR)
			{
				//1个小时是预测的
				bRet = ZhongShuAnaly_YuCe();
			}
			else if(DataType == PER_DAY)
			{
				bRet = ZhongShuAnaly_TuPo();
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
			CloseHandle(hFile);
			return FALSE;
		}


	}
	else
	{
		char szbuff[256] = {0};
		sprintf(szbuff,"[chs] 打不开文件 错误码=%d", GetLastError());
		OutputDebugStringA(szbuff);
	}
	
	return FALSE;
}

BOOL InputInfoThenCalc2(char * Code,short nSetCode,int Value[4],short DataType,NTime time1,NTime time2,BYTE nTQ,unsigned long unused)  //选取区段
{
	BOOL nRet = FALSE;
	
	return nRet;
}
