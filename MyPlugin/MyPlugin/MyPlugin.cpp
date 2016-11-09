// choice1.cpp : Defines the entry point for the DLL application.
// 插件实例
//使用说明

//1分钟选股:1分钟数据的3个中枢背驰模型 趋势是向下
//5分钟选股:5分钟数据的3个中枢背驰模型 趋势是向下
//30分钟选股:30分钟数据的3个中枢背驰模型 趋势是向下

//日线选股:1分钟数据分析，有两个中枢，趋势是向上，最下面的中枢超过12根
//周线选股:5分钟数据分析，有两个中枢，趋势是向上，最下面的中枢超过12根

//月线选股:5分钟数据分析，只管最后一个中枢后面有一个大的线段向上突破，并且超过中枢上沿的20%


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
BOOL Handle5Min3Buy(char * Code, short nSetCode, short DataType);
BOOL Handle30Min3Buy(char * Code, short nSetCode, short DataType);
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

BOOL InputInfoThenCalc1(char * Code,short nSetCode,int Value[4],short DataType,short nDataNum,BYTE nTQ,unsigned long unused) //按最近数据计算
{
	if(strcmp("300274", Code) == 0)
	{
		if(IsDebuggerPresent() == TRUE)
		{
			//__asm int 3
		}
	}

	if(PER_MIN30 == DataType)
	{
		//月线已经给30分钟的数据占用了，因为30分钟的数据不能直接有，所以需要
		return Handle30MinData(Code, nSetCode, DataType);
	}
	else if(PER_MONTH == DataType)
	{
		return Handle5Min3Buy(Code, nSetCode, DataType);
	}
	else if(PER_SEASON == DataType)
	{
		return Handle30Min3Buy(Code, nSetCode, DataType);
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

	if(DataType == PER_MIN5 || DataType == PER_WEEK)//1小时的就是预测5分钟
	{
		//5分钟
		if(nSetCode == 0)
		{
			//深市
			sprintf(szPath, "%s\\vipdoc\\sz\\fzline\\sz%s.lc5", pDataPath, Code);
		}
		else
		{
			//沪市
			sprintf(szPath, "%s\\vipdoc\\sh\\fzline\\sh%s.lc5", pDataPath, Code);

		}
	}
	else if(DataType == PER_MIN1 || DataType == PER_DAY)//30分钟就是预测1分钟
	{
		//1分钟
		if(nSetCode == 0)
		{
			//深市
			sprintf(szPath, "%s\\vipdoc\\sz\\minline\\sz%s.lc1", pDataPath, Code);
		}
		else
		{
			//沪市
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
			float f8 = GetRealPrice((*(DWORD*)&pbuf[8]));//最高价
			float f12 = GetRealPrice((*(DWORD*)&pbuf[12]));//最低价
			
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
		if(nCount > 20000)
		{
			int nRemain = nCount - 20000;
			pLowEx = (float *)((char*)pLow + nRemain * 4);
			pHighEx = (float *)((char*)pHigh + nRemain * 4);
			nCount = 20000;
		}

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
			else if(DataType == PER_DAY)
			{
				bRet = ZhongShuAnaly_ThreeBuy();//1分钟的三买
			}
			else if(DataType == PER_WEEK)
			{
				bRet = ZhongShuAnaly_ThreeBuy(); //5分钟的三买
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


//--------------------------------------------------------30分钟的处理-------------------------------------------------------------------
//由于30分钟没有直接的数据，所以需要把5分钟的数据，经过合并得到30分钟的数据

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

	if(DataType == PER_MIN30)//1小时的就是预测5分钟
	{
		//5分钟
		if(nSetCode == 0)
		{
			//深市
			sprintf(szPath, "%s\\vipdoc\\sz\\fzline\\sz%s.lc5", pDataPath, Code);
		}
		else
		{
			//沪市
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

		int nnnkkk = 0;//这个才是真正的计数器，5分钟的数据要合并成30分钟的数据就是要这样的

		float fMax = 0;
		float fMin = 10000.01;

		for (int n = 0; n < nCount; n++)
		{
			float f8 = GetRealPrice((*(DWORD*)&pbuf[8]));//最高价
			float f12 = GetRealPrice((*(DWORD*)&pbuf[12]));//最低价

			fMax = max(fMax, f8);
			fMin = min(fMin, f12);

			//这里获取时间
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

		
			//1分钟
			bRet = ZhongShuAnalu_BeiLi();
			

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

BOOL Handle5Min3Buy(char * Code, short nSetCode, short DataType)
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

	if(DataType == PER_MONTH)//1小时的就是预测5分钟
	{
		//5分钟
		if(nSetCode == 0)
		{
			//深市
			sprintf(szPath, "%s\\vipdoc\\sz\\fzline\\sz%s.lc5", pDataPath, Code);
		}
		else
		{
			//沪市
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

		int nnnkkk = 0;//这个才是真正的计数器，5分钟的数据要合并成30分钟的数据就是要这样的

		float fMax = 0;
		float fMin = 10000.01;

		for (int n = 0; n < nCount; n++)
		{
			float f8 = GetRealPrice((*(DWORD*)&pbuf[8]));//最高价
			float f12 = GetRealPrice((*(DWORD*)&pbuf[12]));//最低价

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
		if(nCount > 5000)
		{
			int nRemain = nCount - 5000;
			pLowEx = (float *)((char*)pLow + nRemain * 4);
			pHighEx = (float *)((char*)pHigh + nRemain * 4);
			nCount = 5000;
		}

		__try
		{
			TestPlugin3( nCount, pHighEx, pLowEx);
			TestPlugin4( nCount, pHighEx, pLowEx);
			TestPlugin5( nCount, pHighEx, pLowEx);

			BOOL bRet = FALSE;


			//向上突破的预测
			bRet = ZhongShuAnaly_YuCe_XiangShang();


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


BOOL Handle30Min3Buy(char * Code, short nSetCode, short DataType)
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

	if(DataType == PER_SEASON)//1小时的就是预测5分钟
	{
		//5分钟
		if(nSetCode == 0)
		{
			//深市
			sprintf(szPath, "%s\\vipdoc\\sz\\fzline\\sz%s.lc5", pDataPath, Code);
		}
		else
		{
			//沪市
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

		int nnnkkk = 0;//这个才是真正的计数器，5分钟的数据要合并成30分钟的数据就是要这样的

		float fMax = 0;
		float fMin = 10000.01;

		for (int n = 0; n < nCount; n++)
		{
			float f8 = GetRealPrice((*(DWORD*)&pbuf[8]));//最高价
			float f12 = GetRealPrice((*(DWORD*)&pbuf[12]));//最低价

			fMax = max(fMax, f8);
			fMin = min(fMin, f12);

			//这里获取时间
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
		if(nCount > 20000)
		{
			int nRemain = nCount - 20000;
			pLowEx = (float *)((char*)pLow + nRemain * 4);
			pHighEx = (float *)((char*)pHigh + nRemain * 4);
			nCount = 20000;
		}

		__try
		{
			TestPlugin3( nCount, pHighEx, pLowEx);
			TestPlugin4( nCount, pHighEx, pLowEx);
			TestPlugin5( nCount, pHighEx, pLowEx);

			BOOL bRet = FALSE;


			//1分钟
			bRet = ZhongShuAnaly_YuCe_XiangShang2();


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
