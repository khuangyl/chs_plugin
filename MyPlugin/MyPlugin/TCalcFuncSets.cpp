#include "stdafx.h"
#include "TCalcFuncSets.h"
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <time.h>
#include <vector>

using namespace std;

#define TMPFILE "D:\\tdx_tmp.txt"
#define LOGMAIN "D:\\log_main.txt"




enum KDirection
{
	DOWN = -1,
	NODIRECTION = 0,
	UP =1
};

#define MAX(a,b)(a>b)?a:b
#define MIN(a,b)(a>b)?b:a
#define SET_TOPFX(a) { a.prop=1; top_k=a;}
#define SET_BOTTOMFX(a) {a.prop=-1; bottom_k=a;}
#define CLEAR_FX(a) {a.prop=0;}

typedef struct _klineExtern
{
	int   nProp;		//��־�����ֻ��ߵ׷ֱ�־ 1 ����-1 ��
	int   nMegre;		//�ϲ���־ 1��ʾ���ϲ���0��ʾû��
	float MegreHigh;	//�ϲ���ߵ�
	float MegreLow;	    //�ϲ���͵�
	int   nDirector;    //����,��ǰһ���ȣ�1 ���ϣ�0��û�з��� -1 ����
}klineExtern;


//k line struct
typedef struct kline
{
	//float open;
	//float close;
	float high; //high price
	float low;  //low price
	int index; // index the number of k line
	float prop; // 1:up, -1: down, 0: normal
	klineExtern Ext;
} KLine;

typedef struct _Bi_Point
{
	int nIndex; //�ʵĵĵ�����ݵ�����
	float fVal;	//����Ǹ߾�ȡ���ֵ������ǵ͵��ȡ��Сֵ
	int nprop;  //֮ǰ�ʵĸߵ͵�

	int MegerIndex;//���ʱ���Ǻϲ��ģ�ָ���µ���������
}Bi_Point;

typedef struct _Bi_Line
{
	Bi_Point PointLow; //�ʵĵ͵�
	Bi_Point PointHigh;//�ʵĸߵ�
	KDirection Bi_Direction;//�ʵķ���

	int XianDuan_nprop;//����������߶εĸߵ͵� 
	int nMeger;

	int nMegerx2;

	_Bi_Line()
	{
		nMegerx2 = 0;
	}

}Bi_Line;

typedef struct _Xianduan_Line
{
	Bi_Point PointLow; //�ʵĵ͵�
	Bi_Point PointHigh;//�ʵĸߵ�
	KDirection Bi_Direction;//�ʵķ���

	int XianDuan_nprop;//����ı�־��1������㣬2����
	int nMeger;

	float fMax;
	float fMin;

}Xianduan_Line;

Bi_Line *g_Bl;
int      g_nBlSize;

int g_nSize = 0;
KLine* g_tgKs = NULL;

int g_orgDataLen = 0;
KLine* g_orgKs = NULL;

void AnalyXD(Bi_Line *Bl, int nLen);

//determine two Neighboring k lines is included or not, 
//1:left included, -1: right included, 0:not included
int isIncluded(KLine kleft, KLine kright)
{
	if (((kleft.high>=kright.high) && (kleft.low<=kright.low))){
		return -1;
	}else if((kleft.high<=kright.high) && (kleft.low>=kright.low)){
		return 1;
	}else{
		return 0;
	};
};



//merge two Neighboring k lines depend on directon
KLine kMerge(KLine kleft, KLine kright, int Direction){
	KLine value=kright;
	if(Direction == UP)
	{
		value.high = MAX(kleft.high, kright.high);
		value.low = MAX(kleft.low, kright.low);
	}
	else if(Direction == DOWN)
	{
		value.high = MIN(kleft.high, kright.high);
		value.low = MIN(kleft.low, kright.low);
	};

	return value;	
};

//if two Neighboring k lines is not inclued, determine the direction.
//1: up, -1: down
KDirection isUp(KLine kleft, KLine kright){
	if((kleft.high>kright.high) && (kleft.low>kright.low)){
		return DOWN;
	}else if((kleft.high<kright.high) && (kleft.low<kright.low)){
		return UP;
	};

	return NODIRECTION; 
};

//����Ǿ��������������жϣ�
BOOL isUp_Ex2(KLine kleft, KLine kright)
{
	if((kleft.high<kright.high))
	{
		return TRUE;
	};

	return FALSE; 
};

BOOL isDown_Ex2(KLine kleft, KLine kright)
{
	if((kleft.low > kright.low))
	{
		return TRUE;
	};

	return FALSE; 
};

void checkOuts(int DataLen, float* Out){
    ofstream file;
	file.open(TMPFILE);

	for(int i=0;i<DataLen;i++){
			file << Out[i] << "\n";
		};
	file.close();
};

void checkKs(int DataLen, KLine* Ks){
	ofstream ks_file;
	ks_file.open(TMPFILE);
	for(int i=0;i<DataLen;i++){
		ks_file << "index: " <<Ks[i].index << " high: " 
				<<Ks[i].high <<" low: " <<Ks[i].low 
				<<" prop: " <<Ks[i].prop <<"\n";
		};
	ks_file.close();
};

//�ж�ȱ�ڣ�һ���������ȱ�ھʹ����ȱ��, ��������һ����ȱ��
//�������������У������һ��������µ�ȱ�ڣ����ҷ���
int Handle_One_Pen_QueKou_Up(KLine* ks, int nlow, int nhigh)
{
	
	for(int n = nlow+2; n < nhigh; n++)
	{
		if( ks[n-1].low >= ks[nlow].high  &&  ks[n].high < ks[nlow].low)
		{
			//�Ѿ���һ����ȱ��
			ks[n-1].prop = 1;
			ks[n].prop = -1;
			return n;
		}
	}
	return 0;
}

int Handle_One_Pen_QueKou_Down(KLine* ks, int nlow, int nhigh)
{
	
	for(int n = nlow+2; n < nhigh; n++)
	{
		if(ks[n-1].high <= ks[nlow].low  &&  ks[n].low > ks[nlow].high)
		{
			//�Ѿ���һ����ȱ��
			ks[n-1].prop = -1;
			ks[n].prop = 1;
			return TRUE;
		}
	}
	return 0;
}

//���Ƚ���2�����ͽ����жϣ���ʱ����Ͳ�okҲ������һ��
BOOL Is_FengXing_Ok(KLine* ks, int nlow, int nhigh, KDirection Direction)
{
	if(Direction == UP)
	{
		int kl1 = 0; //�׷��͵ĸ�һ��
		int kh1 = 0; //�����͵ĵ�һ��


		int kl_before = 0; //���ϱʣ��׷��͵�ǰһ��k��
		int kh_after = 0; //���ϱʣ������͵ĺ�һ��k��

		for(int n = nlow-1; n > 0; n--)
		{
			if(ks[n].Ext.nMegre != -1)
			{
				kl_before = n;
				break;
			}
		}

		for(int n = nhigh+1; n < nhigh+100; n++)
		{
			if(ks[n].Ext.nMegre != -1)
			{
				kh_after = n;
				break;
			}
		}

		if(ks[kl_before].high >= ks[nhigh].high)
		{
			return FALSE;
		}

		if(ks[kh_after].low <= ks[nlow].low)
		{
			return FALSE;
		}




		for(int n = nlow+1; n < nhigh; n++)
		{
			if(ks[n].Ext.nMegre != -1)
			{
				kl1 = n;
				break;
			}
		}


		for(int n = nhigh-1; n > nlow; n--)
		{
			if(ks[n].Ext.nMegre != -1)
			{
				kh1 = n;
				break;
			}
		}

		//1�������͵���ߵ��kl1����ߵ�Ҫ��
		//2, �׷��͵���͵��kh1����͵��
		if(ks[nhigh].high > ks[kl1].high  && ks[nlow].low < ks[kh1].low)
		{
			return TRUE;
		}


	}
	else
	{
		//���������µ�
		int kl1 = 0; //�׷��͵ĸ�һ��
		int kh1 = 0; //�����͵ĵ�һ��

		int kh_before = 0; //���±ʣ������͵�ǰһ��k��
		int kl_after = 0; //���±ʣ��׷��͵ĺ�һ��k��


		for(int n = nlow-1; n > 0; n--)
		{
			if(ks[n].Ext.nMegre != -1)
			{
				kh_before = n;
				break;
			}
		}

		for(int n = nhigh+1; n < nhigh+100; n++)
		{
			if(ks[n].Ext.nMegre != -1)
			{
				kl_after = n;
				break;
			}
		}

		if(ks[kh_before].low <= ks[nhigh].low)
		{
			return FALSE;
		}

		if(ks[kl_after].high >= ks[nlow].high)
		{
			return FALSE;
		}

		for(int n = nlow+1; n < nhigh; n++)
		{
			if(ks[n].Ext.nMegre != -1)
			{
				kh1 = n;
				break;
			}
		}


		for(int n = nhigh-1; n > nlow; n--)
		{
			if(ks[n].Ext.nMegre != -1)
			{
				kl1 = n;
				break;
			}
		}

		//1�������͵���ߵ��kl1����ߵ�Ҫ��
		//2, �׷��͵���͵��kh1����͵��
		if(ks[nlow].high > ks[kl1].high  && ks[nhigh].low < ks[kh1].low)
		{
			return TRUE;
		}
	}

	return FALSE;
}
//**********************************************�±�*******************************************************/
//���⴦����������Ƿ��͵�һ���ƻ��˶���֮��Ĺ�ϵ���ʱ����Ҫ����������
void SpecHandleUp(KLine* ks, int nlow, int nhigh)
{
	int kl1 = 0; //�׷��͵ĸ�һ��
	int kh1 = 0; //�����͵ĵ�һ��

	for(int n = nlow+1; n < nhigh; n++)
	{
		if(ks[n].Ext.nMegre != -1)
		{
			kl1 = n;
			break;
		}
	}


	for(int n = nhigh-1; n > nlow; n--)
	{
		if(ks[n].Ext.nMegre != -1)
		{
			kh1 = n;
			break;
		}
	}

	//�������kl1����kh1��ʱ�������⴦��
	if(ks[kl1].high >= ks[kh1].high)
	{

	}
}


//���Ƚ���2�����ͽ����жϣ���ʱ����Ͳ�okҲ������һ��
BOOL Is_FengXing_Ok_NewOpen_ZuiGaoZuiDi(KLine* ks, int nlow, int nhigh, KDirection Direction)
{
	if(Direction == UP)
	{
		int kl1 = 0; //�׷��͵ĸ�һ��
		int kh1 = 0; //�����͵ĵ�һ��

		kl1 = nlow+1;

		for(int n = nhigh-1; n > nlow; n--)
		{
			if(ks[n].Ext.nMegre != -1)
			{
				kh1 = n;
				break;
			}
		}

		for(int n = kl1; n <= kh1; n++)
		{
			if(ks[n].high >= ks[nhigh].high || ks[n].low <= ks[nlow].low)
			{
				return FALSE;
			}
		}

		return TRUE;

	}
	else
	{
		//���������µ�
		int kl1 = 0; //�׷��͵ĸ�һ��
		int kh1 = 0; //�����͵ĵ�һ��


		kh1 = nlow+1;

		for(int n = nhigh-1; n > nlow; n--)
		{
			if(ks[n].Ext.nMegre != -1)
			{
				kl1 = n;
				break;
			}
		}

		for(int n = kh1; n <= kl1; n++)
		{
			if(ks[n].high >= ks[nhigh].high || ks[n].low <= ks[nlow].low)
			{
				return FALSE;
			}
		}

		return TRUE;
	}

	return FALSE;
}


//���Ƚ���2�����ͽ����жϣ���ʱ����Ͳ�okҲ������һ��
BOOL Is_FengXing_Ok_NewOpen(KLine* ks, int nlow, int nhigh, KDirection Direction)
{
	if(Direction == UP)
	{
		int kl1 = 0; //�׷��͵ĸ�һ��
		int kh1 = 0; //�����͵ĵ�һ��

		for(int n = nlow+1; n < nhigh; n++)
		{
			if(ks[n].Ext.nMegre != -1)
			{
				kl1 = n;
				break;
			}
		}

		kl1 = nlow+1;


		for(int n = nhigh-1; n > nlow; n--)
		{
			if(ks[n].Ext.nMegre != -1)
			{
				kh1 = n;
				break;
			}
		}

		//1�������͵���ߵ��kl1����ߵ�Ҫ��
		//2, �׷��͵���͵��kh1����͵��
		if(ks[nhigh].high > ks[kl1].high  && ks[nlow].low < ks[kh1].low)
		{
			return TRUE;
		}


	}
	else
	{
		//���������µ�
		int kl1 = 0; //�׷��͵ĸ�һ��
		int kh1 = 0; //�����͵ĵ�һ��


		for(int n = nlow+1; n < nhigh; n++)
		{
			if(ks[n].Ext.nMegre != -1)
			{
				kh1 = n;
				break;
			}
		}
		kh1 = nlow+1;

		for(int n = nhigh-1; n > nlow; n--)
		{
			if(ks[n].Ext.nMegre != -1)
			{
				kl1 = n;
				break;
			}
		}

		//1�������͵���ߵ��kl1����ߵ�Ҫ��
		//2, �׷��͵���͵��kh1����͵��
		if(ks[nlow].high > ks[kl1].high  && ks[nhigh].low < ks[kh1].low)
		{
			return TRUE;
		}
	}

	return FALSE;
}

//�ж��Ƿ��Ƿ���һ������,�Ƿ������ϵ�һ��
//�±ʣ�ֻ��Ҫ�ж��������;�ok������
BOOL bIsOne_Open_Up_NewOpen(KLine* ks, int nlow, int nhigh)
{
	if(nhigh - nlow < 4)
	{
		return FALSE;
	}


	//�Ƚ��з����ж�
	if(Is_FengXing_Ok_NewOpen(ks,  nlow,  nhigh, UP) == FALSE)
	{

		//�����������һ��k��̫���ֵĻ�����Ҫ���k�߼���ȥ�ɣ�
		//�������£�����ҵ�һ����͵��������µĶ������γ�һ������һ�ʺ����ϵ�һ�ʵĻ�������


		return FALSE;
	}			



	int kl1 = 0; //�׷��͵ĸ�һ��
	int kh1 = 0; //�����͵ĵ�һ��

	for(int n = nlow+1; n < nhigh; n++)
	{
		if(ks[n].Ext.nMegre != -1)
		{
			kl1 = n;
			break;
		}
	}

	kl1 = nlow+1;


	for(int n = nhigh-1; n > nlow; n--)
	{
		if(ks[n].Ext.nMegre != -1)
		{
			kh1 = n;
			break;
		}
	}


	if(kl1 == kh1)
	{
		return FALSE;
	}
	else if(kl1 + 1 == kh1)
	{
		return FALSE;
	}
	else if(kh1 == 0)
	{
		return FALSE;
	}


	if(isUp(ks[kl1], ks[kh1]) == UP)
	{
		//˳����������һ��ȱ�ڵĴ������ks[nhigh+1]��ks[nhigh]�и���ȱ�ڣ����Ѿ�������һ�ʣ��������
		if(g_orgKs[nhigh+1].high < g_orgKs[nlow].low)
		{
			if((g_orgKs[nhigh].low - g_orgKs[nhigh+1].high) > (g_orgKs[nhigh].high - g_orgKs[nlow].low))
			{
				ks[nhigh+1].prop = -1;
			}
		}

		return TRUE;
	}


	return FALSE;
}

//�ж��Ƿ��Ƿ���һ������,�Ƿ������µ�һ��
BOOL bIsOne_Open_Down_NewOpen(KLine* ks, int nlow, int nhigh)
{

	if(nhigh - nlow < 4)
	{
		return FALSE;
	}

	if(Is_FengXing_Ok_NewOpen(ks,  nlow,  nhigh, DOWN) == FALSE)
	{
		return FALSE;
	}	


	int kl1 = 0; //�׷��͵ĸ�һ��
	int kh1 = 0; //�����͵ĵ�һ��

	for(int n = nlow+1; n < nhigh; n++)
	{
		if(ks[n].Ext.nMegre != -1)
		{
			kh1 = n;
			break;
		}
	}

	kh1 = nlow+1;

	for(int n = nhigh-1; n > nlow; n--)
	{
		if(ks[n].Ext.nMegre != -1)
		{
			kl1 = n;
			break;
		}
	}


	if(kl1 == kh1)
	{
		return FALSE;
	}
	else if(kh1 + 1 == kl1)
	{
		return FALSE;
	}
	else if(kl1 == 0)
	{
		return FALSE;
	}

	if(isUp(ks[kh1], ks[kl1]) == DOWN)
	{
		//˳����������һ��ȱ�ڵĴ������ks[nhigh+1]��ks[nhigh]�и���ȱ�ڣ����Ѿ�������һ�ʣ��������
		if(g_orgKs[nhigh+1].low > g_orgKs[nlow].high)
		{
			if((g_orgKs[nhigh+1].low - g_orgKs[nhigh].high) > (g_orgKs[nlow].high - g_orgKs[nhigh].low))
			{
				ks[nhigh+1].prop = 1;
			}
		}

		return TRUE;
	}


	return FALSE;
}

//***********************************************�±�end*********************************************************/

//�ж��Ƿ��Ƿ���һ������,�Ƿ������ϵ�һ��
BOOL bIsOne_Open_Up(KLine* ks, int nlow, int nhigh)
{
	//�Ƚ��з����ж�
	if(Is_FengXing_Ok(ks,  nlow,  nhigh, UP) == FALSE)
	{
		return FALSE;
	}			

	//�����ټ�һ������������м�ֻ��4���ߣ����������׷��ͣ��������¸߻����µͶ�������
	if(nhigh - nlow <= 8)
	{
		for (int n = nlow + 1; n < nhigh; n++)
		{
			if(ks[n].high > ks[nhigh].high)
			{
				return FALSE;
			}
		}
	}

	if(1)
	{
		int nTop = nlow;
		for(int n = nlow+1; n <= nhigh; n++)
		{
			if(ks[n].Ext.nMegre != -1)
			{
				if(ks[n].low < ks[nTop].low)
				{
					nTop = n;
				}
			}
		}
		if(nTop != nlow)
		{
			ks[nlow].prop = 0;
			ks[nTop].prop = -1;

			nlow = nTop;
		}
	}

	int kl1 = 0; //�׷��͵ĸ�һ��
	int kh1 = 0; //�����͵ĵ�һ��

	for(int n = nlow+1; n < nhigh; n++)
	{
		if(ks[n].Ext.nMegre != -1)
		{
			kl1 = n;
			break;
		}
	}


	for(int n = nhigh-1; n > nlow; n--)
	{
		if(ks[n].Ext.nMegre != -1)
		{
			kh1 = n;
			break;
		}
	}

	for(int n = kl1+1; n < kh1; n++)
	{
		if(ks[n].Ext.nMegre != -1)
		{
			if(isUp(ks[kl1],ks[n]) == UP  && ks[n].Ext.nDirector == UP)
			{
				//�ж�����ͺ�����ȱ���ж�,��Ҫ�ж�nlow, nhigh, nhigh+1֮��Ĺ�ϵ
				if(ks[nhigh].low >= ks[nlow].high && ks[nhigh+1].high <= ks[nlow].low)
				{
					ks[nhigh+1].prop = -1;//ֱ��˵���ǵ׷�����
					return TRUE;
				}

			}
		}
	}

	if(1)
	{
		//�������ڷſ��������������ֻҪ��5��k���������ж���
		int nValid = 0;
		//KLine kln[300];
		//ZeroMemory((char*)kln, 300*sizeof(KLine));
		static KLine *kln = NULL;
		if(kln == NULL)
		{
			kln = new KLine[2000];
		}
		ZeroMemory((char*)kln, 2000*sizeof(KLine));

		kln[nValid] = ks[nlow];
		nValid++;
		for(int n = nlow+1; n <= nhigh; n++)
		{
			//if(ks[n].Ext.nMegre != -1)
			//if(ks[n].Ext.nMegre == -1 && isIncluded(ks[n], ks[n+1]) == -1)
			//{
			//	;
			//}
			//else
			if(ks[n].Ext.nMegre == -1 || (ks[n].high == ks[n+1].high && ks[n].low == ks[n+1].low))
			{
				;
			}
			else
			{
				if(ks[n].Ext.nDirector == UP)
				{
					kln[nValid] = ks[n];
					nValid++;
					if(nValid > 1990)
					{
						return FALSE;
					}
				}
			}
		}

		if(nValid < 5)
		{
			return FALSE;
		}

		for(int a = 0; a <= nValid-4; a++)
		{
			for(int b = a+1; b <= nValid-3; b++)
			{
				for(int c = b+1; c <= nValid-2; c++)
				{
					for(int d = c+1; d <= nValid-1; d++)
					{
						for(int e = d+1; e <= nValid; e++)
						{
							if(isUp(kln[a], kln[b]) == UP && isUp(kln[b], kln[c]) == UP && isUp(kln[c], kln[d]) == UP && isUp(kln[d], kln[e]) == UP)
							{
								//�ж�����ͺ�����ȱ���ж�,��Ҫ�ж�nlow, nhigh, nhigh+1֮��Ĺ�ϵ
								if(ks[nhigh].low >= ks[nlow].high && ks[nhigh+1].high <= ks[nlow].low)
								{
									ks[nhigh+1].prop = -1;//ֱ��˵���ǵ׷�����
								}
								return TRUE;
							}
						}
					}

				}
			}
		}
	}



	return FALSE;
}

//�ж��Ƿ��Ƿ���һ������,�Ƿ������µ�һ��
BOOL bIsOne_Open_Down(KLine* ks, int nlow, int nhigh)
{

	if(Is_FengXing_Ok(ks,  nlow,  nhigh, DOWN) == FALSE)
	{
		return FALSE;
	}	

	//�����ټ�һ������������м�ֻ��4���ߣ����������׷��ͣ��������¸߻����µͶ�������
	if(nhigh - nlow <= 8)
	{
		for (int n = nlow + 1; n < nhigh; n++)
		{
			if(ks[n].low < ks[nhigh].low)
			{
				return FALSE;
			}
		}
	}




	if(1)
	{
		int nTop = nlow;
		for(int n = nlow+1; n <= nhigh; n++)
		{
			if(ks[n].Ext.nMegre != -1)
			{
				if(ks[n].high > ks[nTop].high)
				{
					nTop = n;
				}
			}
		}
		if(nTop != nlow)
		{
			ks[nlow].prop = 0;
			ks[nTop].prop = 1;

			nlow = nTop;
		}
	}

	int kl1 = 0; //�׷��͵ĸ�һ��
	int kh1 = 0; //�����͵ĵ�һ��

	for(int n = nlow+1; n < nhigh; n++)
	{
		if(ks[n].Ext.nMegre != -1)
		{
			kh1 = n;
			break;
		}
	}


	for(int n = nhigh-1; n > nlow; n--)
	{
		if(ks[n].Ext.nMegre != -1)
		{
			kl1 = n;
			break;
		}
	}

	for(int n = kh1+1; n < kl1; n++)
	{
		if(ks[n].Ext.nMegre != -1 && ks[n].Ext.nDirector == DOWN)
		{
			if(isUp(ks[kh1],ks[n]) == DOWN)
			{
				//�ж�����ͺ�����ȱ���ж���Ҫ�ж�nlow, nhigh, nhigh+1֮��Ĺ�ϵ
				if(ks[nhigh].high <= ks[nlow].low && ks[nhigh+1].low >= ks[nlow].high)
				{
					ks[nhigh+1].prop = 1;//ֱ��˵���Ƕ�������
					return TRUE;
				}

			}
		}
	}


	if(1)
	{
		//�������ڷſ��������������ֻҪ��5��k���������ж���
		int nValid = 0;

		/*KLine kln[300];
		ZeroMemory((char*)kln, 300*sizeof(KLine));*/
		static KLine *kln = NULL;
		if(kln == NULL)
		{
			kln = new KLine[2000];
		}

		ZeroMemory((char*)kln, 2000*sizeof(KLine));

		kln[nValid] = ks[nlow];
		nValid++;
		for(int n = nlow+1; n <= nhigh; n++)
		{
			//if(ks[n].Ext.nMegre != -1)
			//if(ks[n].Ext.nMegre == -1 && isIncluded(ks[n], ks[n+1]) == -1)
			//{
			//	;
			//}
			//else
			//if(ks[n].high == ks[n+1].high && ks[n].low == ks[n+1].low)
			if(ks[n].Ext.nMegre == -1 || (ks[n].high == ks[n+1].high && ks[n].low == ks[n+1].low))
			{
				;
			}
			else
			{
				if(ks[n].Ext.nDirector == DOWN)
				{
					kln[nValid] = ks[n];
					nValid++;
					if(nValid > 1990)
					{
						return FALSE;
					}
				}
			}
		}

		if(nValid < 5)
		{
			return FALSE;
		}
		for(int a = 0; a <= nValid-4; a++)
		{
			for(int b = a+1; b <= nValid-3; b++)
			{
				for(int c = b+1; c <= nValid-2; c++)
				{
					for(int d = c+1; d <= nValid-1; d++)
					{
						for(int e = d+1; e <= nValid; e++)
						{
							if(isUp(kln[a], kln[b]) == DOWN && isUp(kln[b], kln[c]) == DOWN && isUp(kln[c], kln[d]) == DOWN && isUp(kln[d], kln[e]) == DOWN)
							{
								//�ж�����ͺ�����ȱ���ж���Ҫ�ж�nlow, nhigh, nhigh+1֮��Ĺ�ϵ
								if(ks[nhigh].high <= ks[nlow].low && ks[nhigh+1].low >= ks[nlow].high)
								{
									ks[nhigh+1].prop = 1;//ֱ��˵���Ƕ�������
								}
								return TRUE;
							}
						}
					}

				}
			}
		}
	}

	return FALSE;
}



/***********************************************************************************************************/
/***********************************************************************************************************/
//�����滻�����⣬Ҳ����˵����һ��������һ��������
//�����ж�������֮���Ƿ����һ���ף�����ױ���һ���׻��ͣ���������׺��¶�֮����ֱ���γ�һ��
//���һ����������γɵĻ���i��ֵ��Ҫ�ı��

int Handle_Top_Top_Replace(KLine* ks, int DataLen, int nOldTop, int nNewTop, int *Outi)
{
	//if(nNewTop - nOldTop < 5)
	//{
	//	return 0;
	//}

	int nLastBottom = 0;
	//���ҵ���һ���׵�λ��
	for(int n = nOldTop - 1; n > 0 ; n--)
	{
		if(ks[n].prop == -1)
		{
			nLastBottom = n;
			break ;
		}
	}

	if(nNewTop - nOldTop < 5 || nLastBottom == 0 )
	{
		ks[nOldTop].prop = 0;
		ks[nOldTop].Ext.nProp = 0;
		ks[nNewTop].prop = 1;
		ks[nNewTop].Ext.nProp = 1;

		//Ȼ���ٴ���ȱ�ڵ����⣬�¶��滻�ɶ����������ȱ�ڵ�����,ȱ�����⻹�ǹ����ģ�
		if(g_orgKs[nNewTop+1].high < g_orgKs[nLastBottom].low)
		{
			if((g_orgKs[nNewTop].low - g_orgKs[nNewTop+1].high) > (g_orgKs[nNewTop].high - g_orgKs[nLastBottom].low))
			{
				ks[nNewTop+1].prop = -1;
			}
		}
		return 0;
	}

	//Ȼ�������Ƿ���1�� ������һ���ף�����У������ж��Ƿ���ʺ��¶�ֱ���Ƿ���һ��
	for(int n = nOldTop + 1; n < nNewTop; n++)
	{
		if(ks[n].low < ks[nLastBottom].low && ks[n].Ext.nDirector == DOWN)//���ǣ���һ�ʵķ���϶������µ�
		{
			for(int k = n+1; k < nNewTop; k++)
			{
				//�ҵ�һ���������ϵ�
				if(ks[k].Ext.nDirector == UP)
				{
					//Ȼ�����жϣ��������¶�֮���Ƿ���һ��
					if(bIsOne_Open_Up_NewOpen(ks, k-1, nNewTop) == TRUE)
					{
						//�������k-1�� nNewTop+1֮���prop��־λ�����������µ�����
						for(int j = k-1; j <= nNewTop+1; j++)
						{
							ks[j].prop = 0;
							ks[j].Ext.nProp = 0;
						}

						//Ȼ���ڰ� k-1 ��� nProp��׼Ϊ�µף� �� nOldTop �Ķ���־ȥ���� nLastBottom�ĵױ�־ȥ����i��ֵ�ı�
						ks[nOldTop].prop = 0;
						ks[nOldTop].Ext.nProp = 0;
						ks[nLastBottom].prop = 0;
						ks[nLastBottom].Ext.nProp = 0;

						ks[k-1].prop = -1;
						ks[k-1].Ext.nProp = 0;
						*Outi = k;
						return 0;
					}
					n = k;
					break;
				}
			}
		}
	}

	//�ߵ����˵�����Գɹ����¶����ɶ�
	ks[nOldTop].prop = 0;
	ks[nOldTop].Ext.nProp = 0;
	ks[nNewTop].prop = 1;
	ks[nNewTop].Ext.nProp = 1;

	//Ȼ���ٴ���ȱ�ڵ����⣬�¶��滻�ɶ����������ȱ�ڵ�����,ȱ�����⻹�ǹ����ģ�
	if(g_orgKs[nNewTop+1].high < g_orgKs[nLastBottom].low)
	{
		if((g_orgKs[nNewTop].low - g_orgKs[nNewTop+1].high) > (g_orgKs[nNewTop].high - g_orgKs[nLastBottom].low))
		{
			ks[nNewTop+1].prop = -1;
		}
	}

}


//����׵��滻������
int Handle_Bottom_Bottom_Replace(KLine* ks, int DataLen, int nOldBottom, int nNewBottom, int *Outi)
{
	//if(nNewBottom - nOldBottom < 5)
	//{
	//	return 0;
	//}

	int nLastTop = 0;
	//���ҵ���һ������λ��
	for(int n = nOldBottom - 1; n > 0 ; n--)
	{
		if(ks[n].prop == 1)
		{
			nLastTop = n;
			break ;
		}
	}

	if(nNewBottom - nOldBottom < 5 || nLastTop == 0)
	{
		ks[nOldBottom].prop = 0;
		ks[nOldBottom].Ext.nProp = 0;
		ks[nNewBottom].prop = -1;
		ks[nNewBottom].Ext.nProp = -1;
		//Ȼ���ٴ���ȱ�ڵ����⣬�¶��滻�ɶ����������ȱ�ڵ�����,ȱ�����⻹�ǹ����ģ�
		if(g_orgKs[nNewBottom+1].low > g_orgKs[nLastTop].high)
		{
			if((g_orgKs[nNewBottom+1].low - g_orgKs[nNewBottom].high ) > (g_orgKs[nLastTop].high - g_orgKs[nNewBottom].low))
			{
				ks[nNewBottom+1].prop = 1;
			}
		}
		return 0;
	}

	//Ȼ�������Ƿ���1�� ������һ����������У������ж��Ƿ���ʺ��µ�ֱ���Ƿ���һ��
	for(int n = nOldBottom + 1; n < nNewBottom; n++)
	{
		if(ks[n].high > ks[nLastTop].high && ks[n].Ext.nDirector == UP)//���ǣ���һ�ʵķ���϶������µ�
		{
			for(int k = n+1; k < nNewBottom; k++)
			{
				//�ҵ�һ���������ϵ�
				if(ks[k].Ext.nDirector == DOWN)
				{
					//Ȼ�����жϣ��������¶�֮���Ƿ���һ��
					if(bIsOne_Open_Down_NewOpen(ks, k-1, nNewBottom) == TRUE)
					{
						//�������k-1�� nNewTop+1֮���prop��־λ�����������µ�����
						for(int j = k-1; j <= nNewBottom+1; j++)
						{
							ks[j].prop = 0;
							ks[j].Ext.nProp = 0;
						}

						//Ȼ���ڰ� k-1 ��� nProp��׼Ϊ�µף� �� nOldTop �Ķ���־ȥ���� nLastBottom�ĵױ�־ȥ����i��ֵ�ı�
						ks[nOldBottom].prop = 0;
						ks[nOldBottom].Ext.nProp = 0;
						ks[nLastTop].prop = 0;
						ks[nLastTop].Ext.nProp = 0;

						ks[k-1].prop = 1;
						ks[k-1].Ext.nProp = 0;
						*Outi = k;
						return 0;
					}
					n = k;
					break;
				}
			}
		}
	}

	//�ߵ����˵�����Գɹ����µ׻��ɵ�
	ks[nOldBottom].prop = 0;
	ks[nOldBottom].Ext.nProp = 0;
	ks[nNewBottom].prop = -1;
	ks[nNewBottom].Ext.nProp = -1;

	//Ȼ���ٴ���ȱ�ڵ����⣬�¶��滻�ɶ����������ȱ�ڵ�����,ȱ�����⻹�ǹ����ģ�
	if(g_orgKs[nNewBottom+1].low > g_orgKs[nLastTop].high)
	{
		if((g_orgKs[nNewBottom+1].low - g_orgKs[nNewBottom].high ) > (g_orgKs[nLastTop].high - g_orgKs[nNewBottom].low))
		{
			ks[nNewBottom+1].prop = 1;
		}
	}
}


/***********************************************************************************************************/
/***********************************************************************************************************/


//����һ�����ͣ�Ȼ����ݴ������ķ����ҵ���������ǲ�����5��ͬ�������ȷ������
int Handle_FenXing(KLine* ks, int DataLen, int i, KDirection Direction, int *Outi)
{
	int nValid = 0;
	int n = i;
	if(Direction == UP)//������
	{
		//��Ϊ�����ϵķ����ȵ��������ҵ���һ���׷��ͣ�Ȼ�����ж��ǲ����㹻��5������ͬ���ϵ�k��
		while(n)
		{
			n--;
			if(n <= 0)
			{
				break;
			}

			if(ks[n].prop)
			{
				KLine k1 = ks[n];
				KLine k2 = ks[i];

				//if(Handle_IS_QueKou(ks, n, i))
				//{
				//	return 0;
				//}

				if(ks[n].prop == 1)//������
				{
					//�����ͱ�־λ, �����������ҵ�Ŀ�꣬�������������Ա�
					if(k1.high >= k2.high)
					{
						//��ߵı��ұߵĸ�,ֱ�ӷ���
						return 0;
					}
					else
					{
						Handle_Top_Top_Replace(ks, DataLen, n, i, Outi);

						return 0;
						//��ߵ�û���ұߵĸߣ������ߵ�
						ks[n].prop = 0;
						ks[n].Ext.nProp = 0;

						ks[i].prop = 1;      //��ʱ��ȥ��
						ks[i].Ext.nProp = 1; //��ʱ��ȥ��

						for (int kkk = n; kkk > 0; kkk--)
						{
							if(ks[kkk].prop == -1)
							{
								if(g_orgKs[i+1].high < g_orgKs[kkk].low)
								{
									if((g_orgKs[i].low - g_orgKs[i+1].high) > (g_orgKs[i].high - g_orgKs[kkk].low))
									{
										ks[i+1].prop = -1;
									}
								}
								break;
							}
						}
						return 0;
					}
				}
				else //�׷��� 
				{
					//�ҵ��׷����Ժ����жϣ����������Ϸ���
					if(isUp(k1, k2) == UP)
					{
						if(bIsOne_Open_Up_NewOpen(ks, n, i))
						{
							ks[i].prop = 1;
							ks[i].Ext.nProp = 1;
							return 0;
						}
						else
						{
							//�ж����������һ�ʵĻ������н��д���
						}
					}
					else
					{
						//������򶼲��������ϣ�������ɶ������
						return 0;
					}

				}

				return 0;
			}
		}



		//�������Ժ������û���ҵ��κ�һ������, ֱ�ӱ�Ƕ����ͣ�Ȼ�󷵻�
		ks[i].prop = 1;
		ks[i].Ext.nProp = 1;
		return 0;
	}
	else if(Direction == DOWN)
	{
		//��Ϊ�����µķ����ȵ��������ҵ���һ�������ͣ�Ȼ�����ж��ǲ����㹻��5������ͬ���µ�k��
		while(n)
		{
			n--;
			if(n <= 0)
			{
				break;
			}

			if(ks[n].prop)
			{
				KLine k1 = ks[n];
				KLine k2 = ks[i];

				//if(Handle_IS_QueKou(ks, n, i))
				//{
				//	return 0;
				//}

				if(ks[n].prop == -1)//�׷���
				{
					//�׷��ͱ�־λ, �����������ҵ�Ŀ�꣬�����׷������Ա�
					if(k1.low <= k2.low)
					{
						//��ߵı��ұߵĵ�,ֱ�ӷ���
						return 0;
					}
					else
					{
						//��ߵ�û���ұߵĵͣ������ߵ�
						Handle_Bottom_Bottom_Replace(ks, DataLen, n, i, Outi);

						return 0;

						ks[n].prop = 0;
						ks[n].Ext.nProp = 0;

						ks[i].prop = -1;
						ks[i].Ext.nProp = -1;

						for (int kkk = n; kkk > 0; kkk--)
						{
							if(ks[kkk].prop == 1)  //���ʱ����kkk�Ķ�����
							{
								if(g_orgKs[i+1].low > g_orgKs[kkk].high)
								{
									if((g_orgKs[i+1].low - g_orgKs[i].high) > (g_orgKs[kkk].high - g_orgKs[i].low))
									{
										ks[i+1].prop = 1;
									}
								}
								break;
							}
						}
						return 0;
					}
				}
				else //������
				{
					//�ҵ��׷����Ժ����жϣ����������Ϸ���
					if(isUp(k1, k2) == DOWN)
					{
						if(bIsOne_Open_Down_NewOpen(ks, n, i))
						{
							ks[i].prop = -1;
							ks[i].Ext.nProp = -1;
							return 0;
						}
						else
						{
							//�ж����������һ�ʵĻ������н��д���
							//��Ҫ�ٴ��ж���εĵ׺��ϴεĵױȽϣ�����ҵ��ˣ�
							//Handle_No_Pen_Down(ks, i);
						}
					}
					else
					{
						//������򶼲��������ϣ�������ɶ������
						return 0;
					}

				}
				return 0;
			}
		}



		//�������Ժ������û���ҵ��κ�һ������, ֱ�ӱ�Ƕ����ͣ�Ȼ�󷵻�
		ks[i].prop = -1;
		ks[i].Ext.nProp = -1;
		return 0;
	}

}

void Handle_QueKou(KLine* ks, int DataLen)
{
	int k_low = 0;
	int k_high = 0;


	for(int n = 0; n  < DataLen; n++)
	{
		if(ks[n].prop)
		{
			if(k_low == 0)
			{
				k_low = n;
				for(int k = n+1; k < DataLen; k++)
				{
					if(ks[k].prop)
					{
						k_high = k;
					}
				}
			}
			else
			{
				k_high = n;
			}
		
			if(isUp(ks[k_low], ks[k_high]) == UP)
			{
				Handle_One_Pen_QueKou_Up(ks, k_low, k_high);
			}
			else if(isUp(ks[k_low], ks[k_high]) == DOWN)
			{
				Handle_One_Pen_QueKou_Down(ks, k_low, k_high);
			}

			k_low = n;
		}
	}
}

void TestPlugin3(int DataLen,float* High,float* Low)
{
	OutputDebugStringA("[chs] TestPlugin3");
	KDirection direction= NODIRECTION; //1:up, -1:down, 0:no drection��
	KLine* ks = new KLine[DataLen];
	KLine up_k; //������ʱK��
	KLine down_k; //������ʱk��
	KLine top_k=ks[0]; //����Ķ�����
	KLine bottom_k=ks[0]; //����ĵ׷���
	KLine tmp_k; //��ʼ��ʱk��
	int down_k_valid=0; //������Чk����
	int up_k_valid=0; //������Чk����
	KDirection up_flag = NODIRECTION; //���Ǳ�־

	//init ks lines by using the import datas
	for(int i=0;i<DataLen;i++){
		ks[i].index = i;
		ks[i].high = High[i];
		ks[i].low = Low[i];
		ks[i].prop = 0;
		ZeroMemory(&ks[i].Ext, sizeof(klineExtern));//����0
	};

	//k�����ݿ���
	if(DataLen > g_orgDataLen)
	{
		if(g_orgKs)
		{
			//KLine* g_tgKs = new KLine[DataLen]
			delete g_orgKs;
		}

		g_orgKs = new KLine[DataLen];

	}
	g_orgDataLen = DataLen;
	memcpy((char*)g_orgKs, (char*)ks, DataLen*sizeof(KLine));

	//��ks�������ݽ��з��ʹ���
	tmp_k = ks[0];
	
	for(int i=1;i<DataLen;i++)
	{
		KLine curr_k=ks[i];
		KLine last=ks[i-1];

		if(curr_k.high >= 4.419 && 
			curr_k.high < 4.421 &&
			curr_k.low >= 4.349   &&
			curr_k.low <  4.351)
		{

		}

		/*
		�ӵڶ���K������Ҫ�͵�һ��K����ȣ�����һ����ʼ����
		��������K��Ϊȫ����������δ�з���tmp_kΪ������K�ߡ�
		*/
		direction = (KDirection)last.Ext.nDirector;
		tmp_k = last;
		if(tmp_k.Ext.nMegre)
		{
			//����кϲ����ͰѺϲ��ĸߵ͵��tmp_k
			tmp_k.high = tmp_k.Ext.MegreHigh;
			tmp_k.low = tmp_k.Ext.MegreLow;
		}

		switch(direction)
		{
			case NODIRECTION:
			{
				int include_flag = isIncluded(tmp_k, curr_k);
				if(include_flag > 0)//������������Ƿ�Ӧ���� include_flag != 0 Ҳ���ǲ������Ұ�������ͬ������
				{
					//tmp_k = curr_k;�ոտ�ʼ��ʱ���а��������ǲ�֪����ô�����
				}
				else if(include_flag == 0)//������
				{
					//������ǣ����tmp_k����Ϊ�׷��ͣ�����������Ϊ������
					if (isUp(tmp_k,curr_k))
					{
						ks[i].Ext.nDirector = (int)UP;
						//direction = UP;
					}
					else
					{
						ks[i].Ext.nDirector = (int)DOWN;
					};
				};
				break;
			}
			case UP:
			{
			
				//�Ƿ����,tmp_k����һ�����µģ�����а���Ҳ�����µ�
				if (isIncluded(tmp_k, curr_k)!=0)
				{
					
					//��������ȡ���ϰ���,ͬʱ��ֵ
					up_k = kMerge(tmp_k, curr_k, direction);

					ks[i].high = up_k.high;
					ks[i].low = up_k.low;

					ks[i].Ext.MegreHigh = up_k.high;
					ks[i].Ext.MegreLow = up_k.low;
					ks[i].Ext.nDirector = (int)UP; //���������ϰ���
					ks[i].Ext.nMegre = 1; //������ʶ
					ks[i-1].Ext.nMegre = -1; //-1����������Ե�
					if(ks[i-1].prop)
					{
						ks[i].prop = ks[i-1].prop;
						 ks[i-1].prop = 0;
					}
				}
				else
				{//û�а��������
				
					//�ж��Ƿ�����
					up_flag = isUp(tmp_k, curr_k);
					if(up_flag==UP)
					{
						
						//����������ѵ�ǰK���û�Ϊ������ʱK��
						//���������ͬ,ֻ�ѷ����ʶλ�ı䣬�����ľͲ���ȥ��
						ks[i].Ext.nDirector = (int)UP;
					}
					else if(up_flag == DOWN)
					{
						ks[i].Ext.nDirector = (int)DOWN;
						//����������
						//�ж϶������Ƿ����
						Handle_FenXing(ks, DataLen, i-1, UP, &i);
					};
				};
				break;
			}
			case DOWN: 
			{			
				//�Ƿ����,tmp_k����һ�����µģ�����а���Ҳ�����µ�
				if (isIncluded(tmp_k, curr_k)!=0)
				{

					//��������ȡ���ϰ���,ͬʱ��ֵ
					down_k = kMerge(tmp_k, curr_k, direction);

					ks[i].high = down_k.high;
					ks[i].low = down_k.low;

					ks[i].Ext.MegreHigh = down_k.high;
					ks[i].Ext.MegreLow = down_k.low;
					ks[i].Ext.nDirector = (int)DOWN; //���������ϰ���
					ks[i].Ext.nMegre = 1; //������ʶ
					ks[i-1].Ext.nMegre = -1; //-1����������Ե�
					if(ks[i-1].prop)
					{
						ks[i].prop = ks[i-1].prop;
						ks[i-1].prop = 0;
					}

				}
				else
				{//û�а��������

					//�ж��Ƿ�����µ�
					up_flag = isUp(tmp_k, curr_k);
					if(up_flag==DOWN)
					{

						//���������ͬ,ֻ�ѷ����ʶλ�ı䣬�����ľͲ���ȥ��
						curr_k.Ext.nDirector = (int)DOWN;
						ks[i].Ext.nDirector = (int)DOWN;
					}
					else if(up_flag == UP)
					{
						ks[i].Ext.nDirector = (int)UP;
						//�������µ�
						//�ж϶������Ƿ����
						Handle_FenXing(ks, DataLen, i-1, DOWN, &i);
					};
				};

				break;
			};
			default:
				break;
		}
	};


	//�������һ��ȱ�ڵĴ���
	//Handle_QueKou(ks, DataLen);


	for(int i=0;i<DataLen;i++)
	{
		//Out[i] = ks[i].prop;
		if(ks[i].prop)
		{
			//char szbuff[128] = {0};
			//sprintf(szbuff, "[chs] index=%d, prop=%.2f, high=%.2f, low=%.2f", ks[i].index, ks[i].prop, ks[i].high, ks[i].low);

			//OutputDebugStringA(szbuff);
		}
	};

	//k�����ݿ���
	if(DataLen > g_nSize)
	{
		if(g_tgKs)
		{
			//KLine* g_tgKs = new KLine[DataLen]
			delete g_tgKs;
		}

		g_tgKs = new KLine[DataLen];
	}

	memcpy(g_tgKs, ks, sizeof(KLine)*DataLen);
	g_nSize = DataLen;
	
	delete []ks;
}


//-----------------------------------------------------�߶εĴ�����---------------------------------------------------
/********************************************************************************************************************************/
/********************************************************************************************************************************/

//determine two Neighboring k lines is included or not, 
//1:left included, -1: right included, 0:not included
int isIncluded_XianDuan(Bi_Line kleft, Bi_Line kright)
{
	//if (((kleft.high>=kright.high) && (kleft.low<=kright.low))){
	//	return -1;
	//}else if((kleft.high<=kright.high) && (kleft.low>=kright.low)){
	//	return 1;
	//}else{
	//	return 0;
	//};

	if (((kleft.PointHigh.fVal >= kright.PointHigh.fVal) && (kleft.PointLow.fVal <= kright.PointLow.fVal)))
	{
		return -1;//�����
	}
	else if((kleft.PointHigh.fVal <= kright.PointHigh.fVal) && (kleft.PointLow.fVal >= kright.PointLow.fVal))
	{
		return 1;//�Ұ���
	}
	else
	{
		return 0;//������
	};
};



//merge two Neighboring k lines depend on directon
Bi_Line kMerge_XianDuan(Bi_Line kleft, Bi_Line kright, int Direction)
{
	Bi_Line value = kright;
	if(Direction == UP)
	{
		value.PointHigh.fVal = MAX(kleft.PointHigh.fVal, kright.PointHigh.fVal);
		value.PointLow.fVal = MAX(kleft.PointLow.fVal, kright.PointLow.fVal);
	}
	else if(Direction == DOWN)
	{
		value.PointHigh.fVal = MIN(kleft.PointHigh.fVal, kright.PointHigh.fVal);
		value.PointLow.fVal = MIN(kleft.PointLow.fVal, kright.PointLow.fVal);
	};

	return value;	
};

//if two Neighboring k lines is not inclued, determine the direction.
//1: up, -1: down
KDirection isUp_XianDuan(Bi_Line kleft, Bi_Line kright)
{
	if((kleft.PointHigh.fVal > kright.PointHigh.fVal) && (kleft.PointLow.fVal > kright.PointLow.fVal))
	{
		return DOWN;
	}
	else if((kleft.PointHigh.fVal < kright.PointHigh.fVal) && (kleft.PointLow.fVal < kright.PointLow.fVal))
	{
		return UP;
	};

	return NODIRECTION; 
};


//����1���Ѿ������������������
BOOL Is_BiPoHuai(Bi_Line *Blx, int k, KDirection Direction)
{
	if(Direction == UP)
	{
		Bi_Line bl = Blx[2*k+1];

		for (int n = k; n > 0; n--)
		{
			if(Blx[2*k-1].nMeger != -1)
			{
				if(bl.PointLow.fVal < Blx[2*k-1].PointHigh.fVal)
				{
					return TRUE;
				}
			}
		}
	}
	else
	{
		Bi_Line bl = Blx[2*k+1];

		for (int n = k; n > 0; n--)
		{
			if(Blx[2*k-1].nMeger != -1)
			{
				if(bl.PointHigh.fVal > Blx[2*k-1].PointLow.fVal)
				{
					return TRUE;
				}
			}
		}
	}

	return FALSE;
}



//�������кϲ����ѽ��������
int Te_Zheng_XuLie_Meger(Bi_Line *Bl, int nStartk, int nLen, Bi_Line *BlxOut)
{
	//BlxOut = new Bi_Line[nLen];
	memcpy(BlxOut, Bl, sizeof(Bi_Line)*nLen);

	//���ϵ��߶Σ�
	if(BlxOut[nStartk].Bi_Direction == UP)
	{
		//�����������н��а��������ҵ�
		for (int k = 0; nStartk+k*2+3 < nLen; k++)
		{
			int nRet = isIncluded_XianDuan(BlxOut[nStartk+k*2+1],  BlxOut[nStartk+k*2+3]);
			if( nRet== -1)//ֻ��������Ŵ���
			{
				//�������Ұ�������ȡ��
				Bi_Line temp =  kMerge_XianDuan(BlxOut[nStartk+k*2+1], BlxOut[nStartk+k*2+3], UP);
				BlxOut[nStartk+k*2+1].nMeger = -1;
				BlxOut[nStartk+k*2+3].nMeger = 1;


				//if(BlxOut[nStartk+k*2+3].PointHigh.fVal != temp.PointHigh.fVal)
				{
					//˵����ǰһ���ߵ��
					BlxOut[nStartk+k*2+3].PointHigh.MegerIndex = BlxOut[nStartk+k*2+3].PointHigh.nIndex;//����ԭ���ĵ㣬
					BlxOut[nStartk+k*2+3].PointHigh.nIndex = BlxOut[nStartk+k*2+1].PointHigh.nIndex;//���µ긳ֵ����				 
				}

				BlxOut[nStartk+k*2+3].PointHigh.fVal = temp.PointHigh.fVal;
				BlxOut[nStartk+k*2+3].PointLow.fVal = temp.PointLow.fVal;
			}
			else if(nRet == 1)
			{
				//�Ұ���
				BlxOut[nStartk+k*2+1].nMegerx2 = 1;
			}

		}
	}
	else
	{
		//�����������н��а��������ҵ�
		for (int k = 0; nStartk+k*2+3 < nLen; k++)
		{
			int nRet = isIncluded_XianDuan(BlxOut[nStartk+k*2+1],  BlxOut[nStartk+k*2+3]);
			if(nRet == -1)
			{
				//�������Ұ�������ȡ��
				Bi_Line temp =  kMerge_XianDuan(BlxOut[nStartk+k*2+1], BlxOut[nStartk+k*2+3], DOWN);
				BlxOut[nStartk+k*2+1].nMeger = -1;
				BlxOut[nStartk+k*2+3].nMeger = 1;

				//if(BlxOut[nStartk+k*2+3].PointLow.fVal != temp.PointLow.fVal)
				{
					//˵����ǰһ���ߵ��
					BlxOut[nStartk+k*2+3].PointLow.MegerIndex = BlxOut[nStartk+k*2+3].PointLow.nIndex;//����ԭ���ĵ㣬
					BlxOut[nStartk+k*2+3].PointLow.nIndex = BlxOut[nStartk+k*2+1].PointLow.nIndex;//���µ긳ֵ����
				}

				BlxOut[nStartk+k*2+3].PointHigh.fVal = temp.PointHigh.fVal;
				BlxOut[nStartk+k*2+3].PointLow.fVal = temp.PointLow.fVal;
			}
			else if(nRet == 1)
			{
				//�Ұ���
				BlxOut[nStartk+k*2+1].nMegerx2 = 1;
			}

		}
	}

	return 0;
}

//��ȡ���ƻ�һ��(�յ�)ǰһ����(��ߣ�������ͣ���Ϊ����α�߶εĴ��ڣ���ʱ�������ƻ��ʵ�ǰһ����߻�����͵����ж�)
//nStart�ǿ�ʼ��λ�� �����ж��ǵ�һ���ƻ����ǵڶ�����
int Get_GuaiDian_Before_Bi(Bi_Line *Bl, int k, KDirection nDirect, int nStart)
{
	//k�ǹյ�㣬���������ǰ���㣬�ҵ�һ����־λXianDuan_nprop,˵����
	if(nDirect == UP)
	{
		BOOL bFlag = FALSE;
		float fVal = 0;//Bl[k].PointHigh.fVal;
		int n = k;
		int nRet = 0;

		k = k - 2;
		while(k  > nStart)
		{
			if(Bl[k].XianDuan_nprop)
			{
				break;
			}

			if(bFlag== FALSE && Bl[k].PointHigh.fVal != Bl[n].PointHigh.fVal )
			{
				fVal = Bl[k].PointHigh.fVal;
				bFlag = TRUE;
				nRet = k;
			}
			else
			{
				if(Bl[k].PointHigh.fVal > fVal)
				{
					fVal = Bl[k].PointHigh.fVal;
					nRet = k;
				}
			}

			k = k - 2;

		}

		return nRet;
	}


	//����������
	if(nDirect == DOWN)
	{
		BOOL bFlag = FALSE;
		float fVal = 0;//Bl[k].PointHigh.fVal;
		int n = k;
		int nRet = 0;

		k = k - 2;
		while(k  > nStart)
		{
			if(Bl[k].XianDuan_nprop)
			{
				break;
			}

			if(bFlag== FALSE && Bl[k].PointLow.fVal != Bl[n].PointLow.fVal )
			{
				fVal = Bl[k].PointLow.fVal;
				bFlag = TRUE;
				nRet = k;
			}
			else
			{
				if(Bl[k].PointLow.fVal < fVal)
				{
					fVal = Bl[k].PointLow.fVal;
					nRet = k;
				}
			}

			k = k - 2;

		}

		return nRet;
	}

}


int Get_GuaiDian_Real(Bi_Line *Bl, int k, KDirection nDirect, int nStart)
{
	if(nDirect == UP)
	{
		int n = k;
		k = k - 2;
		while(k  > nStart)
		{
			if(Bl[k].XianDuan_nprop)
			{
				break;
			}

			if( Bl[k].PointHigh.fVal != Bl[n].PointHigh.fVal )
			{
				return k+2;
			}

			k = k - 2;
		}
	}
	else
	{
		int n = k;
		k = k - 2;
		while(k  > nStart)
		{
			if(Bl[k].XianDuan_nprop)
			{
				break;
			}

			if(Bl[k].PointLow.fVal != Bl[n].PointLow.fVal )
			{
				return k+2;
			}

			k = k - 2;
		}
	}

	return 0;
}


int Te_Zheng_XuLie_Meger_For_FengXing(Bi_Line *Bl, int nStartk, int nLen, Bi_Line *BlxOut)
{
	//BlxOut = new Bi_Line[nLen];
	memcpy(BlxOut, Bl, sizeof(Bi_Line)*nLen);


	KDirection Directionxx = Bl[nStartk].Bi_Direction;

	Bi_Line Temp = BlxOut[nStartk+1];

	for (int k = 0; nStartk+k*2+3 < nLen; k++)
	{
		int nRet = isIncluded_XianDuan(Temp,  BlxOut[nStartk+k*2+3]);
		if(nRet)
		{
			if(Directionxx == NODIRECTION)
			{
			}
			else if(Directionxx == UP)
			{
				Bi_Line tempx =  kMerge_XianDuan(BlxOut[nStartk+k*2+1], BlxOut[nStartk+k*2+3], UP);
				BlxOut[nStartk+k*2+1].nMeger = -1;
				BlxOut[nStartk+k*2+3].nMeger = 1;
				{
					//˵����ǰһ���ߵ��
					BlxOut[nStartk+k*2+3].PointHigh.MegerIndex = BlxOut[nStartk+k*2+3].PointHigh.nIndex;//����ԭ���ĵ㣬
					BlxOut[nStartk+k*2+3].PointHigh.nIndex = BlxOut[nStartk+k*2+1].PointHigh.nIndex;//���µ긳ֵ����				 
				}

				BlxOut[nStartk+k*2+3].PointHigh.fVal = tempx.PointHigh.fVal;
				BlxOut[nStartk+k*2+3].PointLow.fVal = tempx.PointLow.fVal;
			}
			else if(Directionxx == DOWN)
			{
				Bi_Line temp =  kMerge_XianDuan(BlxOut[nStartk+k*2+1], BlxOut[nStartk+k*2+3], DOWN);
				BlxOut[nStartk+k*2+1].nMeger = -1;
				BlxOut[nStartk+k*2+3].nMeger = 1;
				{
					//˵����ǰһ���ߵ��
					BlxOut[nStartk+k*2+3].PointLow.MegerIndex = BlxOut[nStartk+k*2+3].PointLow.nIndex;//����ԭ���ĵ㣬
					BlxOut[nStartk+k*2+3].PointLow.nIndex = BlxOut[nStartk+k*2+1].PointLow.nIndex;//���µ긳ֵ����
				}

				BlxOut[nStartk+k*2+3].PointHigh.fVal = temp.PointHigh.fVal;
				BlxOut[nStartk+k*2+3].PointLow.fVal = temp.PointLow.fVal;
			}
		}
		else
		{
			//��������ֻ�ı䷽��
			Directionxx = isUp_XianDuan(Temp,  BlxOut[nStartk+k*2+3]);

		}

		Temp = BlxOut[nStartk+k*2+3];

	}

	//���ϵ��߶Σ�
	//if(BlxOut[nStartk].Bi_Direction == UP)
	//{
	//	//�����������н��а��������ҵ�
	//	for (int k = 0; nStartk+k*2+3 < nLen; k++)
	//	{
	//		int nRet = isIncluded_XianDuan(BlxOut[nStartk+k*2+1],  BlxOut[nStartk+k*2+3]);
	//		if( nRet)//ֻ��������Ŵ���, �ҷ��ͣ����Ұ�����Ҫ����
	//		{
	//			//�������Ұ�������ȡ��
	//			Bi_Line temp =  kMerge_XianDuan(BlxOut[nStartk+k*2+1], BlxOut[nStartk+k*2+3], UP);
	//			BlxOut[nStartk+k*2+1].nMeger = -1;
	//			BlxOut[nStartk+k*2+3].nMeger = 1;


	//			//if(BlxOut[nStartk+k*2+3].PointHigh.fVal != temp.PointHigh.fVal)
	//			{
	//				//˵����ǰһ���ߵ��
	//				BlxOut[nStartk+k*2+3].PointHigh.MegerIndex = BlxOut[nStartk+k*2+3].PointHigh.nIndex;//����ԭ���ĵ㣬
	//				BlxOut[nStartk+k*2+3].PointHigh.nIndex = BlxOut[nStartk+k*2+1].PointHigh.nIndex;//���µ긳ֵ����				 
	//			}

	//			BlxOut[nStartk+k*2+3].PointHigh.fVal = temp.PointHigh.fVal;
	//			BlxOut[nStartk+k*2+3].PointLow.fVal = temp.PointLow.fVal;
	//		}
	//		//else if(nRet == 1)
	//		//{
	//		//	//�Ұ���
	//		//	//BlxOut[nStartk+k*2+1].nMegerx2 = 1;

	//		//	//�ҷ��͵�ʱ���Ұ���ҲҪ����


	//		//}

	//	}
	//}
	//else
	//{
	//	//�����������н��а��������ҵ�
	//	for (int k = 0; nStartk+k*2+3 < nLen; k++)
	//	{
	//		int nRet = isIncluded_XianDuan(BlxOut[nStartk+k*2+1],  BlxOut[nStartk+k*2+3]);
	//		if(nRet)
	//		{
	//			//�������Ұ�������ȡ��
	//			Bi_Line temp =  kMerge_XianDuan(BlxOut[nStartk+k*2+1], BlxOut[nStartk+k*2+3], DOWN);
	//			BlxOut[nStartk+k*2+1].nMeger = -1;
	//			BlxOut[nStartk+k*2+3].nMeger = 1;

	//			//if(BlxOut[nStartk+k*2+3].PointLow.fVal != temp.PointLow.fVal)
	//			{
	//				//˵����ǰһ���ߵ��
	//				BlxOut[nStartk+k*2+3].PointLow.MegerIndex = BlxOut[nStartk+k*2+3].PointLow.nIndex;//����ԭ���ĵ㣬
	//				BlxOut[nStartk+k*2+3].PointLow.nIndex = BlxOut[nStartk+k*2+1].PointLow.nIndex;//���µ긳ֵ����
	//			}

	//			BlxOut[nStartk+k*2+3].PointHigh.fVal = temp.PointHigh.fVal;
	//			BlxOut[nStartk+k*2+3].PointLow.fVal = temp.PointLow.fVal;
	//		}
	//		//else if(nRet == 1)
	//		//{
	//		//	//�Ұ���
	//		//	BlxOut[nStartk+k*2+1].nMegerx2 = 1;
	//		//}

	//	}
	//}

	return 0;
}

//�Ӹõ����ҷ���,���ط��͵�λ��
int Is_XianDuan_FenXing(Bi_Line *Bl, int nStartk, int nLen, KDirection nDirect)
{
	Bi_Line *BlxOut = new Bi_Line[nLen];;
	Te_Zheng_XuLie_Meger_For_FengXing(Bl, nStartk, nLen, BlxOut);

	KDirection nDirectxx = NODIRECTION;

	if(DOWN == nDirect)//�������£��ҳ�������
	{
		if(BlxOut[nStartk].Bi_Direction != DOWN)
		{
			//���ж�һ�°ɱ������ɶ����
			delete BlxOut;
			return 0;
		}

		int nCount = 0;
		Bi_Line  Temp = BlxOut[nStartk +1];
		for (int k = 1; nStartk + 2*k+1 < nLen; k++)
		{
			//�ҵ��������еķ���
			//if(BlxOut[nStartk + 2*k+1].nMeger != -1)
			{
				KDirection Direct = isUp_XianDuan(Temp, BlxOut[nStartk + 2*k+1]);
				if(nDirectxx == NODIRECTION)
				{
					nDirectxx = Direct;
				}	
				else if(nDirectxx == DOWN)
				{
					if(Direct == UP)
					{
						//return nStartk + 2*k-1;
						if(BlxOut[nStartk + 2*k-1].nMeger != -1)
						{
							delete BlxOut;
							return nStartk + 2*k-1;
						}
						else
						{
							for(int m = k ;m > 1; m--)
							{
								if(BlxOut[nStartk + 2*m-1].nMeger != -1)
								{
									delete BlxOut;
									return nStartk + 2*m+1;
								}
							}
						}
					}

				}
				else if(nDirectxx == UP)
				{
					;
				}
				if(Direct != NODIRECTION)
				{
					nDirectxx = Direct;
				}
				Temp = BlxOut[nStartk + 2*k+1];
			}
		}
	}
	else
	{
		//if(DOWN == nDirect)//�������ϣ��ҳ��׷���
		{
			if(BlxOut[nStartk].Bi_Direction != UP)
			{
				//���ж�һ�°ɱ������ɶ����
				delete BlxOut;
				return 0;
			}

			int nCount = 0;
			Bi_Line  Temp = BlxOut[nStartk +1];
			for (int k = 1; nStartk + 2*k+1 < nLen; k++)
			{
				//�ҵ��������еķ���
				//if(BlxOut[nStartk + 2*k+1].nMeger != -1)
				{
					KDirection Direct = isUp_XianDuan(Temp, BlxOut[nStartk + 2*k+1]);
					if(nDirectxx == NODIRECTION)
					{
						nDirectxx = Direct;
					}	
					else if(nDirectxx == DOWN)
					{


					}
					else if(nDirectxx == UP)
					{
						if(Direct == DOWN)
						{
							//return nStartk + 2*k-1;
							if(BlxOut[nStartk + 2*k-1].nMeger != -1)
							{
								delete BlxOut;
								return nStartk + 2*k-1;
							}
							else
							{
								for(int m = k ;m > 1; m--)
								{
									if(BlxOut[nStartk + 2*m-1].nMeger != -1)
									{
										delete BlxOut;
										return nStartk + 2*m+1;
									}
								}
							}

						}
					}
					if(Direct != NODIRECTION)
					{
						nDirectxx = Direct;
					}

					Temp = BlxOut[nStartk + 2*k+1];
				}
			}
		}
	}
	delete BlxOut;
	return 0;
}


//��ȡ��û�кϲ���һ��
int Get_No_Meger_Bi(Bi_Line *Bl, int nStart)
{
	for(int i = 1; nStart-2*i >0; i++)
	{
		if(Bl[nStart-2*i].nMeger != -1)
		{
			return (nStart-2*i+2);
		}
	}
	return 0;
}

//�����Bl[0]���߶εĿ�ʼ�㣬�����������һ����ʵ��
//һ��Ҫȷ����һ���߶ξ����ڵ�����²��ܷ���
//�������ҵ��յ㣬Ȼ��ȷ�Ϲյ����ȷʵ���γ��߶�
//�ƻ�����һ�ʺ���Ҫ��
int Lookup_Next_XianDuan(Bi_Line *Bl, int nStartk, int nLen)
{
	BOOL bFlagGuaiDian = FALSE;
	//�����
	//Draw_StartPoint( Bl,  nStartk);

	Bi_Line *BlxOut = new Bi_Line[nLen];
	Te_Zheng_XuLie_Meger(Bl, nStartk, nLen, BlxOut);

	if(Bl[nStartk].Bi_Direction == UP)
	{
		Bi_Line Temp = BlxOut[nStartk+1];
		for(int k = 0; nStartk+2*k+3 < nLen; k++)
		{
			//if(/*BlxOut[nStartk+2*k+3].nMeger != -1 &&*/ BlxOut[nStartk+2*k+3].nMegerx2 == 0)
			{
				//���ҵ�һ���������е�һ���յ㣬���ж����ǲ�����������1����2
				if(BlxOut[nStartk+2*k+3].PointHigh.fVal < Temp.PointHigh.fVal)
				{
					bFlagGuaiDian = TRUE;

					//DrawGuaiDian(BlxOut,  nStartk,  k);//���յ�ı�

					//����յ�ģ�˵���Ѿ������˵�һ�����ߵڶ���������
					//�ж��ǵ�һ��������ߵڶ������
					//2*k+1�ĵ͵� < 2*k-1�ĸߵ�,���ǵ�һ����������֣����ʱ��Ҫ���������У������жϺ�����ǲ����ܹ��γ�һ���߶�
					//����Ӧ�ò�Ҫ����2*k+1, 2*k-1��Meger�����˰ɣ�������Ҫ����2*k-1������
					//int np = Get2k_dec_1(BlxOut, nStartk+2*k+1);
					int np = Get_GuaiDian_Before_Bi(BlxOut, nStartk+2*k+1, UP, nStartk);

					//DrawGuaiDian_Before_bi(BlxOut,  nStartk, np);//���յ�ǰ��һ��
					//�����ҵ������Ĺյ��
					int nreal_gd = Get_GuaiDian_Real(BlxOut, nStartk+2*k+1, UP, nStartk);//�յ�ǰ��һ��

					if(np)
					{
						if(BlxOut[nreal_gd].PointLow.fVal <= BlxOut[np].PointHigh.fVal)
						{
							int nRet = 0;

							//�����߶�֮ǰ---Ҫ���ж��Ƿ���meger,��Ϊ�������ԭ������Ҫ�ҳ���nStartk+2*k+1 meger����ǰ���һ����
							//Ȼ������ʾ��Ǹ��߶����һ��
							if(BlxOut[nStartk+2*k+1].nMeger != 0)
							{
								//ȷʵ֤ʵ����meger
								int n_no_meger_bi = Get_No_Meger_Bi(BlxOut, nStartk+2*k+1);
								nRet = n_no_meger_bi;


								//��Ҫ������������⣬���Ǵ���Bl������
								Bl[nStartk+2*k+1].PointHigh.nIndex = Bl[n_no_meger_bi].PointHigh.nIndex;
								Bl[nStartk+2*k+1].PointHigh.fVal = Bl[n_no_meger_bi].PointHigh.fVal;

								//���д�����ʾ������
								//vecData[nStartk+2*k+1].pt1.x = vecData[n_no_meger_bi].pt1.x;
								//vecData[nStartk+2*k+1].pt1.y = vecData[n_no_meger_bi].pt1.y;

							}	
							else
							{
								nRet = nStartk+2*k+1;
							}


							//Draw_QueDing_Xianduan_First_UP(BlxOut, nStartk, nRet);//����һ����ı�

							//ȷ���ǵ�һ������ĳ����ˣ����ʱ�����2*k+1�ĸߵ�Ϊ�߶ε���㣬�ҵ����µ��߶Σ���һ���ҵ��յ������һ���߶ε����				
							Bl[nStartk+2*k+1].XianDuan_nprop = 1;
							delete BlxOut;
							return nStartk+2*k+1;

						}
						else
						{
							//����ǵڶ�������£�Ҳ����һ�ʲ�û���ƻ��ˡ�
							//����2*k+1Ϊ��㣬����ҵ��׷��͵��������У������������һ���߶εĿ�ʼ
							//Is_XianDuan_FenXing����������ص׷�����͵�λ�õ�k
							int npos = Is_XianDuan_FenXing(BlxOut, nStartk+2*k+1, nLen, DOWN);
							if(npos)
							{
								//Draw_Second_FengXing(BlxOut, npos);//�����ڶ�������ķ���

								//�ҵ��׷����ˣ�Ȼ�󿴿�����׷��ͺ͹յ�֮���Ƿ��Ѿ���һ���߶�
								//�жϷ������յ�䵽npos����бʸ߹��յ㣬��˵����յ㲻��һ���߶εĵ㣬��Ҫ���ø�һ��ĵ�������
								int j = k+1;
								//for (int j = nStartk+2*k+1; j <= npos; j++)
								BOOL bFlagRet = FALSE;
								for (; nStartk+2*j < npos; j++)
								{
									//����е����ĳ���ʵĵ���ڹյ��ǰһ���㣬Ҳ�ܹ���
									if(Bl[nStartk+2*j].PointHigh.fVal > Bl[nStartk+2*k+1].PointHigh.fVal)
									{
										k = j-1;
										bFlagRet = TRUE;
										Temp = BlxOut[nStartk+2*k+3];
										break;
									}

								}

								if( bFlagRet == FALSE)
								{
									//�Ѿ���ȷ���γ���

									if(BlxOut[nStartk+2*k+1].nMeger)
									{
										while(1)
										{
											k--;

											if(BlxOut[nStartk+2*k+1].nMeger != -1)
											{
												k++;
												//Draw_QueDing_Xianduan_Second_UP(BlxOut,  nStartk,  k);//���ڶ����߶ε��γ�

												//Ҳ�������γ��߶�
												Bl[nStartk+2*k+1].XianDuan_nprop = 1;
												delete BlxOut;
												return nStartk+2*k+1;
											}
										}
									}
									else
									{
										//Draw_QueDing_Xianduan_Second_UP(BlxOut,  nStartk,  k);//���ڶ����߶ε��γ�

										//Ҳ�������γ��߶�
										Bl[nStartk+2*k+1].XianDuan_nprop = 1;
										delete BlxOut;
										return nStartk+2*k+1;
									}


								}
								else
								{
									//::MessageBoxA(m_hWnd, "�ҵ�����Ҳ�����γ��߶�", NULL, MB_OK);
									Temp = BlxOut[nStartk+2*k+3];
								}

							}
							else
							{
								//::MessageBoxA(m_hWnd, "�Ҳ����ڶ�������ķ���", NULL, MB_OK);
								//�������п���˵���ǿ�����ˣ�Ӧ��Ҫ�ý��������������
								//��Ϊ�����µķ�������ֱ������͵ıʣ��ڿ����Ƿ��ܹ���Ϊһ��
								//�������Ѿ��йյ�����ˣ�˵���յ��п����γ�һ���߶Σ�
								//�����������
								int d = k;
								for(; nStartk+2*d+1 < nLen; d++)
								{
									if(BlxOut[nStartk+2*d+1].PointHigh.fVal > BlxOut[nStartk+2*k+1].PointHigh.fVal)
									{
										k = d;
									}
								}
								if(BlxOut[nStartk+2*k+1].PointHigh.fVal > BlxOut[nStartk].PointHigh.fVal)
								{
									//�Ѿ��γ��߶��߶�
									int  nRet = 0;
									if(BlxOut[nStartk+2*k+1].nMeger != 0)
									{
										//ȷʵ֤ʵ����meger
										int n_no_meger_bi = Get_No_Meger_Bi(BlxOut, nStartk+2*k+1);
										nRet = n_no_meger_bi;

										//��Ҫ������������⣬���Ǵ���Bl������
										Bl[nStartk+2*k+1].PointHigh.nIndex = Bl[n_no_meger_bi].PointHigh.nIndex;
										Bl[nStartk+2*k+1].PointHigh.fVal = Bl[n_no_meger_bi].PointHigh.fVal;

										//���д�����ʾ������
										//vecData[nStartk+2*k+1].pt1.x = vecData[n_no_meger_bi].pt1.x;
										//vecData[nStartk+2*k+1].pt1.y = vecData[n_no_meger_bi].pt1.y;

									}
									else
									{
										nRet = nStartk+2*k+1;
									}

									//Draw_QueDing_Xianduan_First_UP(BlxOut, nStartk, nRet);//����һ����ı�

									Bl[nStartk+2*k+1].XianDuan_nprop = 1;
									delete BlxOut;
									return nStartk+2*k+1;
								}
							}
						}



					}
					else
					{
						Temp = BlxOut[nStartk+2*k+3];
					}

				}
				else
				{
					Temp = BlxOut[nStartk+2*k+3];
				}
			}


		}

		// û���ҵ��յ㣬Ҳ����һ��ֱ�ߣ�
		if(bFlagGuaiDian  == FALSE)
		{
			int k = 1;
			float Vfloat = Bl[nStartk].PointHigh.fVal;
			int n = 0;
			while(nStartk+k < nLen)
			{
				if( Vfloat <  Bl[nStartk+k].PointHigh.fVal && Bl[nStartk+k].Bi_Direction == UP)
				{
					n = k;
					Vfloat = Bl[nStartk+k].PointHigh.fVal;
				}
				k++;
			}
			k--;
			if(Vfloat >  Bl[nStartk].PointHigh.fVal && n >=2 )
			{
				{
					//Draw_QueDing_Xianduan_First_UP(Bl, nStartk, nStartk+n);//����һ����ı�
					Bl[nStartk+n].XianDuan_nprop = 1;
				}
			}
		}
		else
		{
			int k = 1;
			float Vfloat = Bl[nStartk].PointHigh.fVal;
			int n = 0;
			while(nStartk+k < nLen)
			{
				if( Vfloat <  Bl[nStartk+k].PointHigh.fVal && Bl[nStartk+k].Bi_Direction == UP)
				{
					n = k;
					Vfloat = Bl[nStartk+k].PointHigh.fVal;
				}
				k++;
			}
			k--;
			if(Vfloat >  Bl[nStartk].PointHigh.fVal && n >=2 )
			{
				{
					//Draw_QueDing_Xianduan_First_UP(Bl, nStartk, nStartk+n);//����һ����ı�
					Bl[nStartk+n].XianDuan_nprop = 1;
				}
			}
		}
	}


	/*-----------------------------------------------------------------------*/
	if(Bl[nStartk].Bi_Direction == DOWN)
	{
		Bi_Line Temp = BlxOut[nStartk+1];
		for(int k = 0; nStartk+2*k+3 < nLen; k++)
		{

			//if(/*BlxOut[nStartk+2*k+3].nMeger != -1 &&*/ BlxOut[nStartk+2*k+3].nMegerx2 == 0)
			{
				//���ҵ�һ���������е�һ���յ㣬���ж����ǲ�����������1����2
				if(BlxOut[nStartk+2*k+3].PointLow.fVal > Temp.PointLow.fVal)
				{

					bFlagGuaiDian = TRUE;
					//DrawGuaiDian(BlxOut,  nStartk,  k);//���յ�ı�

					//����յ�ģ�˵���Ѿ������˵�һ�����ߵڶ���������
					//�ж��ǵ�һ��������ߵڶ������
					//2*k+1�ĵ͵� < 2*k-1�ĸߵ�,���ǵ�һ����������֣����ʱ��Ҫ���������У������жϺ�����ǲ����ܹ��γ�һ���߶�
					//����Ӧ�ò�Ҫ����2*k+1, 2*k-1��Meger�����˰ɣ�������Ҫ����2*k-1������
					//int np = Get2k_dec_1(BlxOut, nStartk+2*k+1);
					int np = Get_GuaiDian_Before_Bi(BlxOut, nStartk+2*k+1, DOWN, nStartk);

					//DrawGuaiDian_Before_bi(BlxOut,  nStartk, np);//���յ�ǰ��һ��
					//�����ҵ������Ĺյ��
					int nreal_gd = Get_GuaiDian_Real(BlxOut, nStartk+2*k+1, DOWN, nStartk);//�յ�ǰ��һ��

					if(np)
					{
						if(BlxOut[nreal_gd].PointHigh.fVal >= BlxOut[np].PointLow.fVal)//�Ƿ������һ�����
						{
							int nRet = 0;

							//�����߶�֮ǰ---Ҫ���ж��Ƿ���meger,��Ϊ�������ԭ������Ҫ�ҳ���nStartk+2*k+1 meger����ǰ���һ����
							//Ȼ������ʾ��Ǹ��߶����һ��
							if(BlxOut[nStartk+2*k+1].nMeger != 0)
							{
								//ȷʵ֤ʵ����meger
								int n_no_meger_bi = Get_No_Meger_Bi(BlxOut, nStartk+2*k+1);
								nRet = n_no_meger_bi;


								//��Ҫ������������⣬���Ǵ���Bl������
								Bl[nStartk+2*k+1].PointLow.nIndex = Bl[n_no_meger_bi].PointLow.nIndex;
								Bl[nStartk+2*k+1].PointLow.fVal = Bl[n_no_meger_bi].PointLow.fVal;

								//���д�����ʾ������
								//vecData[nStartk+2*k+1].pt1.x = vecData[n_no_meger_bi].pt1.x;
								//vecData[nStartk+2*k+1].pt1.y = vecData[n_no_meger_bi].pt1.y;

							}	
							else
							{
								nRet = nStartk+2*k+1;
							}

							Bl[nStartk+2*k+1].XianDuan_nprop = -1;

							//Draw_QueDing_Xianduan_First_DOWN(BlxOut, nStartk, nRet);//����һ����ı�

							//ȷ���ǵ�һ������ĳ����ˣ����ʱ�����2*k+1�ĸߵ�Ϊ�߶ε���㣬�ҵ����µ��߶Σ���һ���ҵ��յ������һ���߶ε����				
							delete BlxOut;
							return nStartk+2*k+1;

						}
						else
						{
							//����ǵڶ�������£�Ҳ����һ�ʲ�û���ƻ��ˡ�
							//����2*k+1Ϊ��㣬����ҵ��׷��͵��������У������������һ���߶εĿ�ʼ
							//Is_XianDuan_FenXing����������ص׷�����͵�λ�õ�k
							int npos = Is_XianDuan_FenXing(BlxOut, nStartk+2*k+1, nLen, UP);
							if(npos)
							{
								//Draw_Second_FengXing(Bl, npos);//�����ڶ�������ķ���

								//�ҵ��׷����ˣ�Ȼ�󿴿�����׷��ͺ͹յ�֮���Ƿ��Ѿ���һ���߶�
								//�жϷ������յ�䵽npos����бʸ߹��յ㣬��˵����յ㲻��һ���߶εĵ㣬��Ҫ���ø�һ��ĵ�������
								int j = k+1;
								//for (int j = nStartk+2*k+1; j <= npos; j++)
								BOOL bFlagRet = FALSE;
								for (; nStartk+2*j < npos; j++)
								{
									//����е����ĳ���ʵĵ���ڹյ��ǰһ���㣬Ҳ�ܹ���
									if(Bl[nStartk+2*j].PointLow.fVal < Bl[nStartk+2*k+1].PointLow.fVal)
									{
										k = j-1;
										bFlagRet = TRUE;
										Temp = BlxOut[nStartk+2*k+3];
										break;
									}

								}

								if( bFlagRet == FALSE)
								{
									//�Ѿ�ȷ�����γ��߶��ˣ���Ҫ���ľ���k�Ƿ���meger�ı�־���ҵ�û�б�־����k��Ȼ���k����Ҫ�ҵ�
									if(BlxOut[nStartk+2*k+1].nMeger)
									{
										while(1)
										{
											k--;

											if(BlxOut[nStartk+2*k+1].nMeger != -1)
											{
												k++;
												//Draw_QueDing_Xianduan_Second_DWON(BlxOut,  nStartk,  k);//���ڶ����߶ε��γ�
												//Ҳ�������γ��߶�
												Bl[nStartk+2*k+1].XianDuan_nprop = -1;
												delete BlxOut;
												return nStartk+2*k+1;
											}
										}
									}
									else
									{
										//Draw_QueDing_Xianduan_Second_DWON(Bl,  nStartk,  k);//���ڶ����߶ε��γ�
										//Ҳ�������γ��߶�
										Bl[nStartk+2*k+1].XianDuan_nprop = -1;
										delete BlxOut;
										return nStartk+2*k+1;
									}

								}
								else
								{
									//::MessageBoxA(m_hWnd, "�ҵ�����Ҳ�����γ��߶�", NULL, MB_OK);
									Temp = BlxOut[nStartk+2*k+3];
								}

							}
							else
							{
								//::MessageBoxA(m_hWnd, "�Ҳ����ڶ�������ķ���", NULL, MB_OK);
								//�������п���˵���ǿ�����ˣ�Ӧ��Ҫ�ý��������������
								//��Ϊ�����µķ�������ֱ������͵ıʣ��ڿ����Ƿ��ܹ���Ϊһ��
								//�������Ѿ��йյ�����ˣ�˵���յ��п����γ�һ���߶Σ�
								//�����������
								int d = k;
								for(; nStartk+2*d+1 < nLen; d++)
								{
									if(BlxOut[nStartk+2*d+1].PointLow.fVal < BlxOut[nStartk+2*k+1].PointLow.fVal)
									{
										k = d;
									}
								}
								if(BlxOut[nStartk+2*k+1].PointLow.fVal < BlxOut[nStartk].PointLow.fVal)
								{
									//�Ѿ��γ��߶��߶�
									int nRet = 0;
									if(BlxOut[nStartk+2*k+1].nMeger != 0)
									{
										//ȷʵ֤ʵ����meger
										int n_no_meger_bi = Get_No_Meger_Bi(BlxOut, nStartk+2*k+1);
										nRet = n_no_meger_bi;


										//��Ҫ������������⣬���Ǵ���Bl������
										Bl[nStartk+2*k+1].PointLow.nIndex = Bl[n_no_meger_bi].PointLow.nIndex;
										Bl[nStartk+2*k+1].PointLow.fVal = Bl[n_no_meger_bi].PointLow.fVal;

										//���д�����ʾ������
										//vecData[nStartk+2*k+1].pt1.x = vecData[n_no_meger_bi].pt1.x;
										//vecData[nStartk+2*k+1].pt1.y = vecData[n_no_meger_bi].pt1.y;

									}	
									else
									{
										nRet = nStartk+2*k+1;
									}

									Bl[nStartk+2*k+1].XianDuan_nprop = -1;

									//Draw_QueDing_Xianduan_First_DOWN(BlxOut, nStartk, nRet);//����һ����ı�


									Bl[nStartk+2*k+1].XianDuan_nprop = -1;
									delete BlxOut;
									return nStartk+2*k+1;
								}

							}
						}
					}
					else
					{
						Temp = BlxOut[nStartk+2*k+3];
					}

				}
				else
				{
					Temp = BlxOut[nStartk+2*k+3];
				}
			}


		}

		// û���ҵ��յ㣬Ҳ����һ��ֱ�ߣ�
		if(bFlagGuaiDian  == FALSE)
		{
			int k = 1;
			float Vfloat = Bl[nStartk].PointLow.fVal;
			int n = 0;
			while(nStartk+k < nLen)
			{
				if( Vfloat >  Bl[nStartk+k].PointLow.fVal && Bl[nStartk+k].Bi_Direction == DOWN)
				{
					n = k;
					Vfloat = Bl[nStartk+k].PointLow.fVal;
				}
				k++;
			}
			k--;
			if(Vfloat <  Bl[nStartk].PointLow.fVal && n >=2 )
			{
				{
					//Draw_QueDing_Xianduan_First_DOWN(Bl, nStartk, nStartk+n);//����һ����ı�
					Bl[nStartk+n].XianDuan_nprop = -1;
				}
			}
		}
		else
		{
			//�����ҵ��յ��ˣ�����ֱ�ӻ��ڹյ�����Ϊֹ
			int k = 1;
			float Vfloat = Bl[nStartk].PointLow.fVal;
			int n = 0;
			while(nStartk+k < nLen)
			{
				if( Vfloat >  Bl[nStartk+k].PointLow.fVal && Bl[nStartk+k].Bi_Direction == DOWN)
				{
					n = k;
					Vfloat = Bl[nStartk+k].PointLow.fVal;
				}
				k++;
			}
			k--;
			if(Vfloat <  Bl[nStartk].PointLow.fVal && n >=2 )
			{
				{
					//Draw_QueDing_Xianduan_First_DOWN(Bl, nStartk, nStartk+n);//����һ����ı�
					Bl[nStartk+n].XianDuan_nprop = -1;
				}
			}
		}
	}

	delete BlxOut;
	return 0;
}



int Lookup_Next_XianDuan_EX(Bi_Line *Bl, int nStartk, int nLen)
{
	BOOL bFlagGuaiDian = FALSE;
	//�����
	//Draw_StartPoint( Bl,  nStartk);

	Bi_Line *BlxOut = new Bi_Line[nLen];
	//Te_Zheng_XuLie_Meger_For_New(Bl, nStartk, nLen, BlxOut);
	//OutputDebugStringA("[chs] ��ʼ �ҳ����е���������");

	Te_Zheng_XuLie_Meger(Bl, nStartk, nLen, BlxOut);

	//OutputDebugStringA("[chs] ��ʼ ���� ��������");


	if(Bl[nStartk].Bi_Direction == UP)
	{
		//OutputDebugStringA("[chs] ��������");
		Bi_Line Temp = BlxOut[nStartk+1];
		for(int k = 0; nStartk+2*k+3 < nLen; k++)
		{
			//if(/*BlxOut[nStartk+2*k+3].nMeger != 1 &&*/ BlxOut[nStartk+2*k+3].nMegerx2 == 0)
			{
				//���ҵ�һ���������е�һ���յ㣬���ж����ǲ�����������1����2
				if(BlxOut[nStartk+2*k+3].PointHigh.fVal < Temp.PointHigh.fVal)
				{
					bFlagGuaiDian = TRUE;

					//DrawGuaiDian(BlxOut,  nStartk,  k);//���յ�ı�

					//����յ�ģ�˵���Ѿ������˵�һ�����ߵڶ���������
					//�ж��ǵ�һ��������ߵڶ������
					//2*k+1�ĵ͵� < 2*k-1�ĸߵ�,���ǵ�һ����������֣����ʱ��Ҫ���������У������жϺ�����ǲ����ܹ��γ�һ���߶�
					//����Ӧ�ò�Ҫ����2*k+1, 2*k-1��Meger�����˰ɣ�������Ҫ����2*k-1������
					//OutputDebugStringA("[chs] ��ʼ��np");
					int np = Get_GuaiDian_Before_Bi(BlxOut, nStartk+2*k+1, UP, nStartk);

					//DrawGuaiDian_Before_bi(BlxOut,  nStartk, np);//���յ�ǰ��һ��

					//OutputDebugStringA("[chs] ��ʼ��nreal_gd");
					//�����ҵ������Ĺյ��
					int nreal_gd = Get_GuaiDian_Real(BlxOut, nStartk+2*k+1, UP, nStartk);//�յ�ǰ��һ��


					if(np)
					{
						if(BlxOut[nreal_gd].PointLow.fVal <= BlxOut[np].PointHigh.fVal)
						{
							int nRet = 0;

							//�����߶�֮ǰ---Ҫ���ж��Ƿ���meger,��Ϊ�������ԭ������Ҫ�ҳ���nStartk+2*k+1 meger����ǰ���һ����
							//Ȼ������ʾ��Ǹ��߶����һ��
							if(BlxOut[nStartk+2*k+1].nMeger != 0)
							{
								//ȷʵ֤ʵ����meger
								int n_no_meger_bi = Get_No_Meger_Bi(BlxOut, nStartk+2*k+1);
								nRet = n_no_meger_bi;


								//��Ҫ������������⣬���Ǵ���Bl������
								Bl[nStartk+2*k+1].PointHigh.nIndex = Bl[n_no_meger_bi].PointHigh.nIndex;
								Bl[nStartk+2*k+1].PointHigh.fVal = Bl[n_no_meger_bi].PointHigh.fVal;

								//���д�����ʾ������
								//vecData[nStartk+2*k+1].pt1.x = vecData[n_no_meger_bi].pt1.x;
								//vecData[nStartk+2*k+1].pt1.y = vecData[n_no_meger_bi].pt1.y;

							}	
							else
							{
								nRet = nStartk+2*k+1;
							}


							//OutputDebugStringA("[chs] ����һ����ı�");
							//Draw_QueDing_Xianduan_First_UP(BlxOut, nStartk, nreal_gd);//����һ����ı�

							//ȷ���ǵ�һ������ĳ����ˣ����ʱ�����2*k+1�ĸߵ�Ϊ�߶ε���㣬�ҵ����µ��߶Σ���һ���ҵ��յ������һ���߶ε����				
							Bl[nreal_gd].XianDuan_nprop = 1;
							delete BlxOut;
							return nStartk+2*k+1;

						}
						else
						{
							//OutputDebugStringA("[chs] �����ǵڶ�������� ��ʼ�ҷ���");
							//����ǵڶ�������£�Ҳ����һ�ʲ�û���ƻ��ˡ�
							//����2*k+1Ϊ��㣬����ҵ��׷��͵��������У������������һ���߶εĿ�ʼ
							//Is_XianDuan_FenXing����������ص׷�����͵�λ�õ�k
							int npos = Is_XianDuan_FenXing(BlxOut, nreal_gd, nLen, DOWN);
							if(npos)
							{
								//Draw_Second_FengXing(BlxOut, npos);//�����ڶ�������ķ���

								//�ҵ��׷����ˣ�Ȼ�󿴿�����׷��ͺ͹յ�֮���Ƿ��Ѿ���һ���߶�
								//�жϷ������յ�䵽npos����бʸ߹��յ㣬��˵����յ㲻��һ���߶εĵ㣬��Ҫ���ø�һ��ĵ�������
								int j = k+1;
								//for (int j = nStartk+2*k+1; j <= npos; j++)
								BOOL bFlagRet = FALSE;
								for (; nStartk+2*j < npos; j++)
								{
									//����е����ĳ���ʵĵ���ڹյ��ǰһ���㣬Ҳ�ܹ���
									if(Bl[nStartk+2*j].PointHigh.fVal > Bl[nreal_gd].PointHigh.fVal)
									{
										k = j-1;
										bFlagRet = TRUE;
										Temp = BlxOut[nStartk+2*k+3];
										break;
									}

								}

								if( bFlagRet == FALSE)
								{
									//OutputDebugStringA("[chs] �ҵ����ͣ� ���ڶ������");
									//�Ѿ���ȷ���γ���
									//Draw_QueDing_Xianduan_Second_UP_EX_New(BlxOut,  nStartk,  nreal_gd);

									if(BlxOut[nStartk+2*k+1].nMeger != 0)
									{
										//ȷʵ֤ʵ����meger
										int n_no_meger_bi = Get_No_Meger_Bi(BlxOut, nStartk+2*k+1);

										//��Ҫ������������⣬���Ǵ���Bl������
										Bl[nStartk+2*k+1].PointHigh.nIndex = Bl[n_no_meger_bi].PointHigh.nIndex;
										Bl[nStartk+2*k+1].PointHigh.fVal = Bl[n_no_meger_bi].PointHigh.fVal;

										//���д�����ʾ������
										//vecData[nStartk+2*k+1].pt1.x = vecData[n_no_meger_bi].pt1.x;
										//vecData[nStartk+2*k+1].pt1.y = vecData[n_no_meger_bi].pt1.y;

									}

									//OutputDebugStringA("[chs] �ҵ����ͣ� ���ڶ������2");
									Bl[nreal_gd].XianDuan_nprop = 1;
									delete BlxOut;
									return nStartk+2*k+1;

								}
								else
								{
									//::MessageBoxA(m_hWnd, "�ҵ�����Ҳ�����γ��߶�", NULL, MB_OK);
									//OutputDebugStringA("[chs] �ҵ�����Ҳ�����γ��߶�");
									Temp = BlxOut[nStartk+2*k+3];
								}

							}
							else
							{
								//::MessageBoxA(m_hWnd, "�Ҳ����ڶ�������ķ���", NULL, MB_OK);
								//�������п���˵���ǿ�����ˣ�Ӧ��Ҫ�ý��������������
								//��Ϊ�����µķ�������ֱ������͵ıʣ��ڿ����Ƿ��ܹ���Ϊһ��
								//�������Ѿ��йյ�����ˣ�˵���յ��п����γ�һ���߶Σ�
								//�����������
								//OutputDebugStringA("[chs] �Ҳ����ڶ�������ķ���");
								int d = k;
								for(; nStartk+2*d+1 < nLen; d++)
								{
									if(BlxOut[nStartk+2*d+1].PointHigh.fVal > BlxOut[nStartk+2*k+1].PointHigh.fVal)
									{
										k = d;
									}
								}

								if(BlxOut[nStartk+2*k+1].PointHigh.fVal > BlxOut[nStartk].PointHigh.fVal)
								{
									//�Ѿ��γ��߶��߶�
									int  nRet = 0;
									if(BlxOut[nStartk+2*k+1].nMeger != 0)
									{
										//ȷʵ֤ʵ����meger
										int n_no_meger_bi = Get_No_Meger_Bi(BlxOut, nStartk+2*k+1);
										nRet = n_no_meger_bi;

										//��Ҫ������������⣬���Ǵ���Bl������
										Bl[nStartk+2*k+1].PointHigh.nIndex = Bl[n_no_meger_bi].PointHigh.nIndex;
										Bl[nStartk+2*k+1].PointHigh.fVal = Bl[n_no_meger_bi].PointHigh.fVal;

										//���д�����ʾ������
										//vecData[nStartk+2*k+1].pt1.x = vecData[n_no_meger_bi].pt1.x;
										//vecData[nStartk+2*k+1].pt1.y = vecData[n_no_meger_bi].pt1.y;

									}
									else
									{
										nRet = nStartk+2*k+1;
									}

									//Draw_QueDing_Xianduan_First_UP(BlxOut, nStartk, nRet);//����һ����ı�
									//OutputDebugStringA("[chs] ���������");
									Bl[nStartk+2*k+1].XianDuan_nprop = 1;
									delete BlxOut;
									return nStartk+2*k+1;
								}
							}
						}

					}
					else
					{
						Temp = BlxOut[nStartk+2*k+3];
					}

				}
				else
				{
					Temp = BlxOut[nStartk+2*k+3];
				}
			}


		}

		//OutputDebugStringA("[chs] û���ҵ��յ㣬Ҳ����һ��ֱ��");
		// û���ҵ��յ㣬Ҳ����һ��ֱ�ߣ�
		if(bFlagGuaiDian  == FALSE)
		{		
			int k = 1;
			float Vfloat = Bl[nStartk].PointHigh.fVal;
			int n = 0;
			while(nStartk+k < nLen)
			{
				if( Vfloat <  Bl[nStartk+k].PointHigh.fVal && Bl[nStartk+k].Bi_Direction == UP)
				{
					n = k;
					Vfloat = Bl[nStartk+k].PointHigh.fVal;
				}
				k++;
			}
			k--;
			if(Vfloat >  Bl[nStartk].PointHigh.fVal && n >=2 )
			{
				{
					//Draw_QueDing_Xianduan_First_UP(Bl, nStartk, nStartk+n);//����һ����ı�
					Bl[nStartk+n].XianDuan_nprop = 1;
				}
			}
		}
		else
		{
			int k = 1;
			float Vfloat = Bl[nStartk].PointHigh.fVal;
			int n = 0;
			while(nStartk+k < nLen)
			{
				if( Vfloat <  Bl[nStartk+k].PointHigh.fVal && Bl[nStartk+k].Bi_Direction == UP)
				{
					n = k;
					Vfloat = Bl[nStartk+k].PointHigh.fVal;
				}
				k++;
			}
			k--;
			if(Vfloat >  Bl[nStartk].PointHigh.fVal && n >=2 )
			{
				{
					//Draw_QueDing_Xianduan_First_UP(Bl, nStartk, nStartk+n);//����һ����ı�
					Bl[nStartk+n].XianDuan_nprop = 1;
				}
			}
		}

		//OutputDebugStringA("[chs] �����˳�");
	}


	/*-----------------------------------------------------------------------*/
	if(Bl[nStartk].Bi_Direction == DOWN)
	{
		//OutputDebugStringA("[chs] ��������");
		Bi_Line Temp = BlxOut[nStartk+1];
		for(int k = 0; nStartk+2*k+3 < nLen; k++)
		{
			//if(/*BlxOut[nStartk+2*k+3].nMeger != 1 &&*/ BlxOut[nStartk+2*k+3].nMegerx2 == 0)
			{
				//���ҵ�һ���������е�һ���յ㣬���ж����ǲ�����������1����2
				if(BlxOut[nStartk+2*k+3].PointLow.fVal > Temp.PointLow.fVal)
				{

					//OutputDebugStringA("[chs] �ҵ��յ�");
					bFlagGuaiDian = TRUE;
					//DrawGuaiDian(BlxOut,  nStartk,  k);//���յ�ı�

					//����յ�ģ�˵���Ѿ������˵�һ�����ߵڶ���������
					//�ж��ǵ�һ��������ߵڶ������
					//2*k+1�ĵ͵� < 2*k-1�ĸߵ�,���ǵ�һ����������֣����ʱ��Ҫ���������У������жϺ�����ǲ����ܹ��γ�һ���߶�
					//����Ӧ�ò�Ҫ����2*k+1, 2*k-1��Meger�����˰ɣ�������Ҫ����2*k-1������
					//int np = Get2k_dec_1(BlxOut, nStartk+2*k+1);
					//OutputDebugStringA("[chs] ��ʼ��np");
					int np = Get_GuaiDian_Before_Bi(BlxOut, nStartk+2*k+1, DOWN, nStartk);//�յ�ǰ��һ��

					//DrawGuaiDian_Before_bi(BlxOut,  nStartk, np);//���յ�ǰ��һ��

					//OutputDebugStringA("[chs] ��ʼ��nreal_gd");
					//�����ҵ������Ĺյ��
					int nreal_gd = Get_GuaiDian_Real(BlxOut, nStartk+2*k+1, DOWN, nStartk);//
					if(np)
					{
						if(BlxOut[nreal_gd].PointHigh.fVal >= BlxOut[np].PointLow.fVal)//�Ƿ������һ�����//chsgggai
						{
							int nRet = 0;

							//OutputDebugStringA("[chs] �����һ�����");

							//�����߶�֮ǰ---Ҫ���ж��Ƿ���meger,��Ϊ�������ԭ������Ҫ�ҳ���nStartk+2*k+1 meger����ǰ���һ����
							//Ȼ������ʾ��Ǹ��߶����һ��
							if(BlxOut[nStartk+2*k+1].nMeger != 0)
							{
								//ȷʵ֤ʵ����meger
								int n_no_meger_bi = Get_No_Meger_Bi(BlxOut, nStartk+2*k+1);
								nRet = n_no_meger_bi;


								//��Ҫ������������⣬���Ǵ���Bl������
								Bl[nStartk+2*k+1].PointLow.nIndex = Bl[n_no_meger_bi].PointLow.nIndex;
								Bl[nStartk+2*k+1].PointLow.fVal = Bl[n_no_meger_bi].PointLow.fVal;

								//���д�����ʾ������
								//vecData[nStartk+2*k+1].pt1.x = vecData[n_no_meger_bi].pt1.x;
								//vecData[nStartk+2*k+1].pt1.y = vecData[n_no_meger_bi].pt1.y;

							}	
							else
							{
								nRet = nStartk+2*k+1;
							}

							//OutputDebugStringA("[chs] ����һ�����");
							Bl[nreal_gd].XianDuan_nprop = -1;//chsgggai

							//Draw_QueDing_Xianduan_First_DOWN(BlxOut, nStartk, nreal_gd);//����һ����ı�//chsgggai

							//ȷ���ǵ�һ������ĳ����ˣ����ʱ�����2*k+1�ĸߵ�Ϊ�߶ε���㣬�ҵ����µ��߶Σ���һ���ҵ��յ������һ���߶ε����				
							delete BlxOut;
							return nStartk+2*k+1;

						}
						else
						{
							//����ǵڶ�������£�Ҳ����һ�ʲ�û���ƻ��ˡ�
							//����2*k+1Ϊ��㣬����ҵ��׷��͵��������У������������һ���߶εĿ�ʼ
							//Is_XianDuan_FenXing����������ص׷�����͵�λ�õ�k
							//OutputDebugStringA("[chs] �ڶ������ ��npos");
							int npos = Is_XianDuan_FenXing(BlxOut, nreal_gd, nLen, UP);//chsgggai
							if(npos)
							{
								//Draw_Second_FengXing(Bl, npos);//�����ڶ�������ķ���

								//�ҵ��׷����ˣ�Ȼ�󿴿�����׷��ͺ͹յ�֮���Ƿ��Ѿ���һ���߶�
								//�жϷ������յ�䵽npos����бʸ߹��յ㣬��˵����յ㲻��һ���߶εĵ㣬��Ҫ���ø�һ��ĵ�������
								int j = k+1;
								//for (int j = nStartk+2*k+1; j <= npos; j++)
								BOOL bFlagRet = FALSE;
								for (; nStartk+2*j < npos; j++)//chsgggai
								{
									//����е����ĳ���ʵĵ���ڹյ��ǰһ���㣬Ҳ�ܹ���
									if(Bl[nStartk+2*j].PointLow.fVal < Bl[nreal_gd].PointLow.fVal)//chsgggai
									{
										k = j-1;
										bFlagRet = TRUE;
										Temp = BlxOut[nStartk+2*k+3];
										break;
									}

								}

								//OutputDebugStringA("[chs] �ڶ�������Ѿ����������");

								if( bFlagRet == FALSE)
								{

									//���·������������жϺϲ�����
									//Draw_QueDing_Xianduan_Second_DWON_EX_New(BlxOut,  nStartk,  nreal_gd);//chsgggai
									Bl[nreal_gd].XianDuan_nprop = -1;//chsgggai
									;
									if(BlxOut[nStartk+2*k+1].nMeger != 0)
									{
										//ȷʵ֤ʵ����meger
										int n_no_meger_bi = Get_No_Meger_Bi(BlxOut, nStartk+2*k+1);

										//��Ҫ������������⣬���Ǵ���Bl������
										Bl[nStartk+2*k+1].PointLow.nIndex = Bl[n_no_meger_bi].PointLow.nIndex;
										Bl[nStartk+2*k+1].PointLow.fVal = Bl[n_no_meger_bi].PointLow.fVal;

										//���д�����ʾ������
										//vecData[nStartk+2*k+1].pt1.x = vecData[n_no_meger_bi].pt1.x;
										//vecData[nStartk+2*k+1].pt1.y = vecData[n_no_meger_bi].pt1.y;

									}	
									//OutputDebugStringA("[chs] �ڶ�������Ѿ��γ�");
									delete BlxOut;
									return nStartk+2*k+1;

								}
								else
								{
									OutputDebugStringA("[chs] �ҵ�����Ҳ�����γ��߶�");
									//::MessageBoxA(m_hWnd, "�ҵ�����Ҳ�����γ��߶�", NULL, MB_OK);
									Temp = BlxOut[nStartk+2*k+3];
								}

							}
							else
							{
								//OutputDebugStringA("[chs] �Ҳ����ڶ�������ķ���");
								//::MessageBoxA(m_hWnd, "�Ҳ����ڶ�������ķ���", NULL, MB_OK);
								//�������п���˵���ǿ�����ˣ�Ӧ��Ҫ�ý��������������
								//��Ϊ�����µķ�������ֱ������͵ıʣ��ڿ����Ƿ��ܹ���Ϊһ��
								//�������Ѿ��йյ�����ˣ�˵���յ��п����γ�һ���߶Σ�
								//�����������
								int d = k;
								for(; nStartk+2*d+1 < nLen; d++)
								{
									if(BlxOut[nStartk+2*d+1].PointLow.fVal < BlxOut[nStartk+2*k+1].PointLow.fVal)
									{
										k = d;
									}
								}

								if(BlxOut[nStartk+2*k+1].PointLow.fVal < BlxOut[nStartk].PointLow.fVal)
								{
									//�Ѿ��γ��߶��߶�
									int nRet = 0;
									if(BlxOut[nStartk+2*k+1].nMeger != 0)
									{
										//ȷʵ֤ʵ����meger
										int n_no_meger_bi = Get_No_Meger_Bi(BlxOut, nStartk+2*k+1);
										nRet = n_no_meger_bi;


										//��Ҫ������������⣬���Ǵ���Bl������
										Bl[nStartk+2*k+1].PointLow.nIndex = Bl[n_no_meger_bi].PointLow.nIndex;
										Bl[nStartk+2*k+1].PointLow.fVal = Bl[n_no_meger_bi].PointLow.fVal;

										//���д�����ʾ������
										//vecData[nStartk+2*k+1].pt1.x = vecData[n_no_meger_bi].pt1.x;
										//vecData[nStartk+2*k+1].pt1.y = vecData[n_no_meger_bi].pt1.y;

									}	
									else
									{
										nRet = nStartk+2*k+1;
									}

									Bl[nStartk+2*k+1].XianDuan_nprop = -1;

									//Draw_QueDing_Xianduan_First_DOWN(BlxOut, nStartk, nRet);//����һ����ı�
									OutputDebugStringA("[chs] ����һ����ı�");


									Bl[nStartk+2*k+1].XianDuan_nprop = -1;
									delete BlxOut;
									return nStartk+2*k+1;
								}

							}
						}
					}
					else
					{
						Temp = BlxOut[nStartk+2*k+3];
					}

				}
				else
				{
					Temp = BlxOut[nStartk+2*k+3];
				}
			}
		}

		//OutputDebugStringA("[chs] û���ҵ��յ㣬Ҳ����һ��ֱ��");
		// û���ҵ��յ㣬Ҳ����һ��ֱ�ߣ�,Ӧ���Ǻ���ļ����߶Σ������Ҫ�������ݣ��߽�����Ҫ�������������Ҫ��
		if(bFlagGuaiDian  == FALSE)
		{
			int k = 1;
			float Vfloat = Bl[nStartk].PointLow.fVal;
			int n = 0;
			while(nStartk+k < nLen)
			{
				if( Vfloat >  Bl[nStartk+k].PointLow.fVal && Bl[nStartk+k].Bi_Direction == DOWN)
				{
					n = k;
					Vfloat = Bl[nStartk+k].PointLow.fVal;
				}
				k++;
			}
			k--;
			if(Vfloat <  Bl[nStartk].PointLow.fVal && n >=2 )
			{
				{
					//Draw_QueDing_Xianduan_First_DOWN(Bl, nStartk, nStartk+n);//����һ����ı�
					Bl[nStartk+n].XianDuan_nprop = -1;
				}
			}
		}
		else
		{
			//�����ҵ��յ��ˣ�����ֱ�ӻ��ڹյ�����Ϊֹ
			int k = 1;
			float Vfloat = Bl[nStartk].PointLow.fVal;
			int n = 0;
			while(nStartk+k < nLen)
			{
				if( Vfloat >  Bl[nStartk+k].PointLow.fVal && Bl[nStartk+k].Bi_Direction == DOWN)
				{
					n = k;
					Vfloat = Bl[nStartk+k].PointLow.fVal;
				}
				k++;
			}
			k--;
			if(Vfloat <  Bl[nStartk].PointLow.fVal && n >=2 )
			{
				{
					//Draw_QueDing_Xianduan_First_DOWN(Bl, nStartk, nStartk+n);//����һ����ı�
					Bl[nStartk+n].XianDuan_nprop = -1;
				}
			}
		}
		//OutputDebugStringA("[chs] �����˳�");
	}

	delete BlxOut;
	return 0;
}






//�߶η�������
void AnalyXD(Bi_Line *Bl, int nLen)
{
	//CClientDC dc(this);
	//CPen pen1;
	//pen1.CreatePen(PS_SOLID,2,RGB(0,0,255));
	//CPen *oldPen=dc.SelectObject(&pen1);
	//�����ҳ���һ�������ص��ıʣ����������һ���߶ε���ʼ
	int i = 0;
	for( i=0; i<nLen-2; i++ )
	{
		if(Bl[i].Bi_Direction == UP)
		{
			if( (Bl[i].PointLow.fVal < Bl[i+1].PointLow.fVal) && (Bl[i+2].PointHigh.fVal > Bl[i+1].PointHigh.fVal) )
			{
				Bl[i].XianDuan_nprop = -1;

				//char szContent[128] = {0};
				//sprintf(szContent, "�ҵ���ʼ��:%d low=%.2f high=%.2f", i, Bl[i].PointLow.fVal, Bl[i].PointHigh.fVal);

				//dc.MoveTo(vecData[i].pt1.x, vecData[i].pt1.y);
				//dc.LineTo(vecData[i].pt2.x, vecData[i].pt2.y);
				//::MessageBoxA(m_hWnd, szContent, NULL, MB_OK);

				break;
			}
		}
		else
		{
			if( (Bl[i].PointHigh.fVal > Bl[i+1].PointHigh.fVal) && (Bl[i+2].PointLow.fVal < Bl[i+1].PointLow.fVal) )
			{
				Bl[i].XianDuan_nprop = 1;

				//char szContent[128] = {0};
				//sprintf(szContent, "�ҵ���ʼ��:%d low=%.2f high=%.2f", i, Bl[i].PointLow.fVal, Bl[i].PointHigh.fVal);

				//dc.MoveTo(vecData[i].pt1.x, vecData[i].pt1.y);
				//dc.LineTo(vecData[i].pt2.x, vecData[i].pt2.y);
				//::MessageBoxA(m_hWnd, szContent, NULL, MB_OK);

				break;
			}
		}

	};


	int k = i;
	while(1)
	{
		k = Lookup_Next_XianDuan_EX(Bl, k, nLen );
		if(k == 0)
		{
			break;
		}
	}


}


void TestPlugin4(int DataLen,float* High,float* Low)
{

	OutputDebugStringA("[chs] TestPlugin4");
	if(DataLen != g_nSize)
	{
		return;
	}

	//������˵���Ѿ�������ʣ������߶εĴ���ϵ���ˣ�
	Bi_Line *Bl = new Bi_Line[DataLen];
	ZeroMemory(Bl, sizeof(Bi_Line)*DataLen);

	KLine klTmp ;
	int   nblCount = 0;
	BOOL  bFlagx = FALSE;

	for(int i=0; i<DataLen; i++)
	{
		if(g_tgKs[i].prop)
		{

			if(g_tgKs[i].prop == 1)
			{
				g_tgKs[i].low = g_tgKs[i].high;

			}
			else
			{
				g_tgKs[i].high = g_tgKs[i].low;
			}


			if(bFlagx == FALSE)
			{
				klTmp = g_tgKs[i];
				bFlagx = TRUE;
			}
			else
			{
				if(g_tgKs[i].prop == 1)
				{
					//��������ϵ�һ��
					Bl[nblCount].Bi_Direction = UP;   //���ϵ�һ��
					Bl[nblCount].PointHigh.nIndex = i;
					Bl[nblCount].PointHigh.fVal = g_tgKs[i].high;
					Bl[nblCount].PointHigh.nprop = g_tgKs[i].prop;//�ʵĸߵ͵�

					Bl[nblCount].PointLow.nIndex = klTmp.index;
					Bl[nblCount].PointLow.fVal = klTmp.low;
					Bl[nblCount].PointLow.nprop = klTmp.prop; //�ʵĸߵ͵�
	
					Bl[nblCount].XianDuan_nprop = 0;
				}
				else
				{
					//��������µ�һ��
					Bl[nblCount].Bi_Direction = DOWN;   //���ϵ�һ��
					Bl[nblCount].PointLow.nIndex = i;
					Bl[nblCount].PointLow.fVal = g_tgKs[i].high;
					Bl[nblCount].PointLow.nprop = g_tgKs[i].prop;

					Bl[nblCount].PointHigh.nIndex = klTmp.index;
					Bl[nblCount].PointHigh.fVal = klTmp.low;
					Bl[nblCount].PointHigh.nprop = klTmp.prop;

					Bl[nblCount].XianDuan_nprop = 0;
				}

				klTmp = g_tgKs[i];
				nblCount++;
			}
		}
	};
	DeleteFileA("c:\\csdata.ini");
	if(Bl[0].Bi_Direction == UP)
	{
		WritePrivateProfileStringA("data", "direct", "up", "c:\\csdata.ini");
	}
	else
	{
		WritePrivateProfileStringA("data", "direct", "down", "c:\\csdata.ini");
	}

	char szContent[128] = {0};
	for (int n = 0; n < nblCount; n++)
	{
		sprintf(szContent, "%f,%f", Bl[n].PointLow.fVal, Bl[n].PointHigh.fVal);
		char szIndex[32];
		sprintf(szIndex, "%d", n);
		WritePrivateProfileStringA("data", szIndex, szContent, "c:\\csdata.ini");
	}

	AnalyXD(Bl, nblCount);


	for(int i=0;i<DataLen;i++)
	{
		g_tgKs[i].prop = 0;
	};

	for(int i=0;i<nblCount;i++)
	{
		if(Bl[i].XianDuan_nprop)
		{
			if (Bl[i].XianDuan_nprop == 1)
			{
				g_tgKs[Bl[i].PointHigh.nIndex].prop = 1;

			}
			else
			{
				g_tgKs[Bl[i].PointLow.nIndex].prop = -1;

			}	
		}
	};

	//for(int i=0;i<DataLen;i++)
	//{
	//	Out[i] = g_tgKs[i].prop;
	//};

	if(g_nBlSize < nblCount)
	{
		if(g_Bl)
		{
			delete[] g_Bl;
		}
		g_Bl = new Bi_Line[nblCount];
	}

	g_nBlSize = nblCount;
	memcpy((char*)g_Bl, (char*)Bl, sizeof(Bi_Line)*nblCount);

	delete []Bl;
}


/************************************************************************************************************************/
/************************************************************************************************************************/
/******************************************************����**************************************************************/
/************************************************************************************************************************/
/************************************************************************************************************************/
Xianduan_Line *g_xd_l = NULL;
int			   nSize_xd_l = 0;

BOOL Is_XD_ChongDie(float leftlow, float lefthigh, float rightlow, float righthigh)
{
	if(lefthigh <= rightlow || leftlow >= righthigh)
	{
		return FALSE;
	}

	return TRUE;
}


int GetBL_Index_By_Point(Bi_Line *Bl, int nLen, Bi_Point point)
{

	for (int n = 0; n < nLen; n++)
	{
		//
		if(point.nprop == 1)
		{
			if(point.nIndex == Bl[n].PointHigh.nIndex)
			{
				return n;
			}
		}
		else
		{
			if(point.nIndex == Bl[n].PointLow.nIndex)
			{
				return n;
			}
		}
	}

	return 0;
}



int AnalyZShu(Xianduan_Line *xd, int nStart, int nLen)
{
	float fmax = xd[nStart+1].PointHigh.fVal;
	float fmin = xd[nStart+1].PointLow.fVal;

	Bi_Point PointMax = xd[nStart+1].PointHigh;
	Bi_Point PointMin = xd[nStart+1].PointLow;


	//���жϣ����ǰ����4��û���ص��Ͳ�����
	if(nStart + 3 < nLen)
	{
		BOOL bRet = Is_XD_ChongDie(xd[nStart+1].PointLow.fVal, xd[nStart+1].PointHigh.fVal, xd[nStart+3].PointLow.fVal, xd[nStart+3].PointHigh.fVal);
		BOOL bRet2 = Is_XD_ChongDie(xd[nStart].PointLow.fVal, xd[nStart].PointHigh.fVal, xd[nStart+2].PointLow.fVal, xd[nStart+2].PointHigh.fVal);
		if(bRet == TRUE && bRet2 == TRUE)
		{
			
		}
		else
		{
			return nStart+1;
		}
	}
	else
	{
		return 0;
	}


	int n = 1;
	BOOL bFlag = FALSE;
	BOOL bGuaiDian = FALSE;
	for(; nStart+2*n+1 < nLen; n++)
	{
		BOOL bRet = Is_XD_ChongDie(fmin, fmax, xd[nStart+2*n+1].PointLow.fVal, xd[nStart+2*n+1].PointHigh.fVal);
		if(bRet)
		{
			bFlag = TRUE;

			fmax = min(fmax, xd[nStart+2*n+1].PointHigh.fVal);
			fmin = max(fmin, xd[nStart+2*n+1].PointLow.fVal);
		}
		if(bRet == FALSE)
		{
			bGuaiDian = TRUE;//�йյ�
			break;
		}
	}

	//if(nStart+2*n+1 > nLen)
	//{
	//	n= n-1;
	//}

	if(bFlag)
	{

		if(bGuaiDian)
		{
			BOOL bRet = Is_XD_ChongDie(fmin, fmax, xd[nStart+2*n].PointLow.fVal, xd[nStart+2*n].PointHigh.fVal);
			if(bRet)
			{
				xd[nStart+1].XianDuan_nprop = 1;
				xd[nStart+1].fMax = fmax;
				xd[nStart+1].fMin = fmin;

				xd[nStart+2*n-1].XianDuan_nprop = 2;
				xd[nStart+2*n-1].fMax = fmax;
				xd[nStart+2*n-1].fMin = fmin;

				return nStart+2*n;
			}
			else
			{
				xd[nStart+1].XianDuan_nprop = 1;
				xd[nStart+1].fMax = fmax;
				xd[nStart+1].fMin = fmin;

				xd[nStart+2*n-2].XianDuan_nprop = 2;
				xd[nStart+2*n-2].fMax = fmax;
				xd[nStart+2*n-2].fMin = fmin;

				return nStart+2*n-1;
			}
		}
		else
		{
			//û�йյ�
			xd[nStart+1].XianDuan_nprop = 1;
			xd[nStart+1].fMax = fmax;
			xd[nStart+1].fMin = fmin;

			xd[nLen-1].XianDuan_nprop = 2;
			xd[nLen-1].fMax = fmax;
			xd[nLen-1].fMin = fmin;

			return 0;
		}
	}
	else
	{
		if(nStart+1 < nLen)
		{
			return nStart+1;
		}
		else
		{
			return 0;
		}
	}

	return 0;

}


void TestPlugin5(int DataLen,float* High,float* Low) 
{
	OutputDebugStringA("[chs] TestPlugin5");
	//�����Ȱ��߶�ת��һ�����ݽṹ
	if(g_nSize == 0)
	{
		return;
	}

	g_xd_l = new Xianduan_Line[g_nBlSize];

	ZeroMemory(g_xd_l, sizeof(Xianduan_Line)*g_nBlSize);


	int nIndex = 0;
	Bi_Point temp;
	BOOL bFlag = FALSE;

	for (int n = 0; n < g_nBlSize; n++)
	{
		if(g_Bl[n].XianDuan_nprop)
		{
			if(nIndex == 0 && bFlag == FALSE)
			{
				bFlag = TRUE;
			}
			else
			{
				if(g_Bl[n].XianDuan_nprop == -1)
				{
					g_xd_l[nIndex].PointLow = g_Bl[n].PointLow;
					g_xd_l[nIndex].PointHigh = temp;
					g_xd_l[nIndex].Bi_Direction = DOWN;

				}
				else if(g_Bl[n].XianDuan_nprop == 1)
				{
					g_xd_l[nIndex].PointLow = temp;
					g_xd_l[nIndex].PointHigh = g_Bl[n].PointHigh;
					g_xd_l[nIndex].Bi_Direction = UP;
				}

				nIndex++;
			}

			if(g_Bl[n].XianDuan_nprop == -1)
			{
				temp = g_Bl[n].PointLow;
			}
			else if(g_Bl[n].XianDuan_nprop == 1)
			{
				temp = g_Bl[n].PointHigh;
			}
		}
	}

	nSize_xd_l = nIndex;

	int nStart = 0;
	while(1)
	{
		nStart = AnalyZShu(g_xd_l, nStart, nSize_xd_l);
		if(nStart == 0)
		{
			break;
		}
	}
	

}

typedef struct _ZhongShuData
{
	int nIn_Index;  // ������������
	int nOut_Index; // �������������

	float fIn_Price;  //�������ļ�Ǯ
	float fOut_Price; //��������ļ�Ǯ

	float fMax;		  //��������ļ�ǮMAX
	float fMin;		  //���������ǮMIN
	float fkk_Max;    //���� �������� ��������ֵ  ���������ж�
	float fkk_Min;    //���� �������� �������Сֵ

	
	//----------------��¼�߶ε�����߶κ�ǰһ��
	int nXDStart_Index;				//��¼�߶ε����
	int nXDStart_Before_Index;       //��¼�߶ε�ǰһ���߶�
	
	int nXDEnd_Index;

}ZhongShuData;


typedef struct _Pair_Data
{
	ZhongShuData d1; //ǰһ������
	ZhongShuData d2; //��һ������

	float fXieLv;
	float fChangDu;
	int   nTimeIndexCount;
	int   fPriceDistance;

}Pair_Data;

//��5���ӵ�k����һ������һ����������
BOOL ZhongShuAnalu_BeiLi_5Min()
{

	OutputDebugStringA("[chs] ZhongShuAnalu_BeiLi");
	std::vector<ZhongShuData> g_vecZhongshu;

	BOOL bFlag = FALSE;

	ZhongShuData ZSData;

	float fkk_max;
	float fkk_min;

	for (int n = 0; n < nSize_xd_l; n++)
	{
		if(g_xd_l[n].XianDuan_nprop == 1)
		{
			bFlag = TRUE;
			ZSData.fMax = g_xd_l[n].fMax;
			ZSData.fMin = g_xd_l[n].fMin;

			ZSData.nXDStart_Index = n;
			ZSData.nXDStart_Before_Index = n-1;

			fkk_max = g_xd_l[n].PointHigh.fVal;
			fkk_min = g_xd_l[n].PointLow.fVal;
			//��㣬������ߵĵ�
			if(g_xd_l[n].PointHigh.nIndex < g_xd_l[n].PointLow.nIndex)
			{
				ZSData.nIn_Index = g_xd_l[n].PointHigh.nIndex;
				ZSData.fIn_Price = g_xd_l[n].PointHigh.fVal;

			}
			else
			{
				ZSData.nIn_Index = g_xd_l[n].PointLow.nIndex;
				ZSData.fIn_Price = g_xd_l[n].PointLow.fVal;

			}
		}

		if(g_xd_l[n].XianDuan_nprop == 2)
		{
			bFlag = FALSE;
			fkk_max = max(fkk_max, g_xd_l[n].PointHigh.fVal);
			fkk_min = min(fkk_min, g_xd_l[n].PointLow.fVal);

			ZSData.fkk_Max = fkk_max;
			ZSData.fkk_Min = fkk_min;
	
			//
			ZSData.nXDEnd_Index = n;

			//��㣬������ߵĵ�
			if(g_xd_l[n].PointHigh.nIndex > g_xd_l[n].PointLow.nIndex)
			{
				ZSData.nOut_Index = g_xd_l[n].PointHigh.nIndex;
				ZSData.fOut_Price = g_xd_l[n].PointHigh.fVal;
			}
			else
			{
				ZSData.nOut_Index = g_xd_l[n].PointLow.nIndex;
				ZSData.fOut_Price = g_xd_l[n].PointLow.fVal;
			}

			g_vecZhongshu.push_back(ZSData);
		}

		if(bFlag == TRUE)
		{
			fkk_max = max(fkk_max, g_xd_l[n].PointHigh.fVal);
			fkk_min = min(fkk_min, g_xd_l[n].PointLow.fVal);
		}
	}

	//�����ռ�����Ժ󣬿�ʼ����
	int nSize = g_vecZhongshu.size();

	if(nSize > 2)
	{
		std::vector<Pair_Data>  pairdata;
		//�ҵ���һ�����࣬����������ϵģ�����û�������ص��Ļ���ֱ���˳�

		ZSData = g_vecZhongshu[nSize-1];

		int nCount = 0; //�������ж������ǲ��ص�
		for (int n = nSize-2; n >= 0; n--)
		{
			BOOL bChongDie = FALSE;
			if(g_vecZhongshu[n].fMin > ZSData.fMax)
			{
				if(g_vecZhongshu[n].fkk_Min < ZSData.fkk_Max)
				{
					if(nCount == 0)
					{
						return FALSE;
					}
					bChongDie = TRUE;
				}
				else
				{
					Pair_Data pd;
					pd.d1 = g_vecZhongshu[n];
					pd.d2 = ZSData;
					pd.fXieLv = (pd.d1.fOut_Price - pd.d2.fIn_Price)/(pd.d2.nIn_Index-pd.d1.nOut_Index);
					pd.nTimeIndexCount = pd.d2.nIn_Index-pd.d1.nOut_Index;
					pd.fPriceDistance = pd.d1.fOut_Price - pd.d2.fIn_Price;

					if(ZSData.nXDEnd_Index - ZSData.nXDStart_Index >= 8 && pairdata.size() < 2)
					{
						return FALSE;
					}
					pairdata.push_back(pd);

					nCount++;
				}
				
				
			}
			else
			{
				//break;
				//�ж�һ���Ƿ����Ѿ������ཻ��������оͺ���,
				if ( ZSData.fMin > g_vecZhongshu[n].fMax &&   ZSData.fkk_Min >  g_vecZhongshu[n].fkk_Max )
				{
					break;
				}
				else
				{
					if(nCount == 0)
					{
						return FALSE;
					}
					bChongDie = TRUE;
				}

			}

			int nIndex = ZSData.nOut_Index;
			float fval = ZSData.fOut_Price;
			int xdtmp = ZSData.nXDEnd_Index;
	
			float fkkkmax = ZSData.fkk_Max;
			float fkkkmin = ZSData.fkk_Min;

			ZSData = g_vecZhongshu[n];

			if(bChongDie)
			{
				ZSData.nOut_Index = nIndex;
				ZSData.fOut_Price = fval;
				ZSData.nXDEnd_Index = xdtmp;//�ѽ�β��
				ZSData.fkk_Max = max(fkkkmax, ZSData.fkk_Max);//�����ص�
				ZSData.fkk_Min = min(fkkkmin, ZSData.fkk_Min);//
			}
		}

		delete[] g_xd_l;

		//�����
		if(nCount >= 2)
		{

			//return TRUE;
			//��ʼ�ж�б��1,�����ж�
			//if(pairdata[0].fXieLv < pairdata[1].fXieLv)
			//{
			//	OutputDebugStringA("[chs] �뿪 ZhongShuAnalu_BeiLi");
			//	return TRUE;
			//}

			//�����ֻ���������࣬
			//����d1��ǰһ�ʱ��������µģ�������б��
			//d2����ֻ��4�ʣ�����
			if(nCount == 1)
			{
				//d1��ǰһ�ʱ��������µ�, d2Ҳ�����µ�
				//if( /*   g_xd_l[pairdata[0].d1.nXDStart_Before_Index].Bi_Direction == DOWN 
				//	&& g_xd_l[pairdata[0].d2.nXDStart_Before_Index].Bi_Direction == DOWN*/
				//	/*&& (pairdata[0].d2.nXDEnd_Index - pairdata[0].d2.nXDStart_Index) == 3*/)
				{

					int nXdIndex = pairdata[0].d1.nXDStart_Before_Index;

					if(g_xd_l[nXdIndex].Bi_Direction == DOWN)
					{
						return FALSE;
					}
					float fVDist_d1 = g_xd_l[pairdata[0].d1.nXDStart_Before_Index].PointHigh.fVal - g_xd_l[pairdata[0].d1.nXDStart_Before_Index].PointLow.fVal;
					float fVDist_d2 = pairdata[0].d1.fOut_Price - pairdata[0].d2.fIn_Price;
					if(fVDist_d1 > fVDist_d2)
					{
						return TRUE;
					}

					float fxlv = (g_xd_l[pairdata[0].d1.nXDStart_Before_Index].PointHigh.fVal - g_xd_l[pairdata[0].d1.nXDStart_Before_Index].PointLow.fVal)/(float)(g_xd_l[pairdata[0].d1.nXDStart_Before_Index].PointHigh.nIndex - g_xd_l[pairdata[0].d1.nXDStart_Before_Index].PointLow.nIndex);
					if(fxlv > pairdata[0].fXieLv)
					{
						return TRUE;
					}
				}
			}
			else
			{
				float fVDist_d1 = pairdata[1].d1.fOut_Price - pairdata[1].d2.fIn_Price;
				float fVDist_d2 = pairdata[0].d1.fOut_Price - pairdata[0].d2.fIn_Price;
				if(fVDist_d1 > fVDist_d2)
				{
					return TRUE;
				}
				//if(    g_xd_l[pairdata[0].d1.nXDStart_Before_Index].Bi_Direction == DOWN 
				//	&& g_xd_l[pairdata[0].d2.nXDStart_Before_Index].Bi_Direction == DOWN)
				{
					if(  pairdata[0].fXieLv < pairdata[1].fXieLv /*&& (pairdata[0].d2.nXDEnd_Index - pairdata[0].d2.nXDStart_Index) == 3 */)
					{
						return TRUE;
					}
				}
				
			}

		}
	}
	
	OutputDebugStringA("[chs] �뿪 ZhongShuAnalu_BeiLi");
	return FALSE;
}



//��������з�������������������࣬�����Ǳ���Ļ��ͷ���TRUE
BOOL ZhongShuAnalu_BeiLi()
{

	OutputDebugStringA("[chs] ZhongShuAnalu_BeiLi");
	std::vector<ZhongShuData> g_vecZhongshu;

	BOOL bFlag = FALSE;

	ZhongShuData ZSData;

	float fkk_max;
	float fkk_min;

	for (int n = 0; n < nSize_xd_l; n++)
	{
		if(g_xd_l[n].XianDuan_nprop == 1)
		{
			bFlag = TRUE;
			ZSData.fMax = g_xd_l[n].fMax;
			ZSData.fMin = g_xd_l[n].fMin;

			ZSData.nXDStart_Index = n;
			ZSData.nXDStart_Before_Index = n-1;

			fkk_max = g_xd_l[n].PointHigh.fVal;
			fkk_min = g_xd_l[n].PointLow.fVal;
			//��㣬������ߵĵ�
			if(g_xd_l[n].PointHigh.nIndex < g_xd_l[n].PointLow.nIndex)
			{
				ZSData.nIn_Index = g_xd_l[n].PointHigh.nIndex;
				ZSData.fIn_Price = g_xd_l[n].PointHigh.fVal;

			}
			else
			{
				ZSData.nIn_Index = g_xd_l[n].PointLow.nIndex;
				ZSData.fIn_Price = g_xd_l[n].PointLow.fVal;

			}
		}

		if(g_xd_l[n].XianDuan_nprop == 2)
		{
			bFlag = FALSE;
			fkk_max = max(fkk_max, g_xd_l[n].PointHigh.fVal);
			fkk_min = min(fkk_min, g_xd_l[n].PointLow.fVal);

			ZSData.fkk_Max = fkk_max;
			ZSData.fkk_Min = fkk_min;
	
			//
			ZSData.nXDEnd_Index = n;

			//��㣬������ߵĵ�
			if(g_xd_l[n].PointHigh.nIndex > g_xd_l[n].PointLow.nIndex)
			{
				ZSData.nOut_Index = g_xd_l[n].PointHigh.nIndex;
				ZSData.fOut_Price = g_xd_l[n].PointHigh.fVal;
			}
			else
			{
				ZSData.nOut_Index = g_xd_l[n].PointLow.nIndex;
				ZSData.fOut_Price = g_xd_l[n].PointLow.fVal;
			}

			g_vecZhongshu.push_back(ZSData);
		}

		if(bFlag == TRUE)
		{
			fkk_max = max(fkk_max, g_xd_l[n].PointHigh.fVal);
			fkk_min = min(fkk_min, g_xd_l[n].PointLow.fVal);
		}
	}

	//�����ռ�����Ժ󣬿�ʼ����
	int nSize = g_vecZhongshu.size();

	if(nSize > 2)
	{
		std::vector<Pair_Data>  pairdata;
		//�ҵ���һ�����࣬����������ϵģ�����û�������ص��Ļ���ֱ���˳�

		ZSData = g_vecZhongshu[nSize-1];

		int nCount = 0; //�������ж������ǲ��ص�
		for (int n = nSize-2; n >= 0; n--)
		{
			BOOL bChongDie = FALSE;
			if(g_vecZhongshu[n].fMin > ZSData.fMax)
			{
				if(g_vecZhongshu[n].fkk_Min < ZSData.fkk_Max)
				{
					if(nCount <= 1)
					{
						return FALSE;
					}
					bChongDie = TRUE;
				}
				else
				{
					Pair_Data pd;
					pd.d1 = g_vecZhongshu[n];
					pd.d2 = ZSData;
					pd.fXieLv = (pd.d1.fOut_Price - pd.d2.fIn_Price)/(pd.d2.nIn_Index-pd.d1.nOut_Index);
					pd.fChangDu = (pd.d1.fOut_Price - pd.d2.fIn_Price)*(pd.d1.fOut_Price - pd.d2.fIn_Price) + (pd.d2.nIn_Index-pd.d1.nOut_Index)*(pd.d2.nIn_Index-pd.d1.nOut_Index);

					if(ZSData.nXDEnd_Index - ZSData.nXDStart_Index >= 15 && pairdata.size() < 1)
					{
						return FALSE;
					}
					pairdata.push_back(pd);

					nCount++;
				}
				
				
			}
			else
			{
				//break;
				//�ж�һ���Ƿ����Ѿ������ཻ��������оͺ���,
				if ( ZSData.fMin > g_vecZhongshu[n].fMax &&   ZSData.fkk_Min >  g_vecZhongshu[n].fkk_Max )
				{
					break;
				}
				else
				{
					if(nCount <= 1)
					{
						return FALSE;
					}
					bChongDie = TRUE;
				}

			}

			int nIndex = ZSData.nOut_Index;
			float fval = ZSData.fOut_Price;
			int xdtmp = ZSData.nXDEnd_Index;
	
			float fkkkmax = ZSData.fkk_Max;
			float fkkkmin = ZSData.fkk_Min;

			ZSData = g_vecZhongshu[n];

			if(bChongDie)
			{
				ZSData.nOut_Index = nIndex;
				ZSData.fOut_Price = fval;
				ZSData.nXDEnd_Index = xdtmp;//�ѽ�β��
				ZSData.fkk_Max = max(fkkkmax, ZSData.fkk_Max);//�����ص�
				ZSData.fkk_Min = min(fkkkmin, ZSData.fkk_Min);//
			}
		}

		//delete[] g_xd_l;

		//�����
		if(nCount >= 2)
		{
			if(nCount == 1)
			{
				//d1��ǰһ�ʱ��������µ�, d2Ҳ�����µ�
				{
					int nXdIndex = pairdata[0].d1.nXDStart_Before_Index;

					if(g_xd_l[nXdIndex].Bi_Direction == DOWN)
					{
						delete[] g_xd_l;
						return FALSE;
					}

					//����������ҵ�ǰһ���������ߵ�
					float fVDist_d1 = g_xd_l[nXdIndex].PointHigh.fVal - g_xd_l[nXdIndex].PointLow.fVal;
					float fVDist_d2 = pairdata[0].d1.fOut_Price - pairdata[0].d2.fIn_Price;
					if(fVDist_d1 > fVDist_d2)
					{
						delete[] g_xd_l;
						return TRUE;
					}

					float fxlv = (g_xd_l[pairdata[0].d1.nXDStart_Before_Index].PointHigh.fVal - g_xd_l[pairdata[0].d1.nXDStart_Before_Index].PointLow.fVal)/(float)(g_xd_l[pairdata[0].d1.nXDStart_Before_Index].PointHigh.nIndex - g_xd_l[pairdata[0].d1.nXDStart_Before_Index].PointLow.nIndex);
					if(fxlv > pairdata[0].fXieLv)
					{
						delete[] g_xd_l;
						return TRUE;
					}
				}
			}
			else
			{
				////���жϳ���
				//if(pairdata[0].fChangDu > pairdata[1].fChangDu)
				//{
				//	return FALSE;
				//}
				//ȥ���������Ե�

				//���������ѹ�߶�Ҫ��, �������һ�������Ժ�ֻҪ��һ�ʳ�������һ������ĵ͵��ˣ����ų���
				int nLastXD = pairdata[0].d2.nXDEnd_Index+1;
				//for(; nLastXD < nSize_xd_l; nLastXD++) //��ʱȥ��
				//{
				//	if(g_xd_l[nLastXD].PointHigh.fVal > pairdata[0].d1.fkk_Min)
				//	{
				//		delete[] g_xd_l;
				//		return FALSE;
				//	}
				//}
	

				float fVDist_d1 = pairdata[1].d1.fOut_Price - pairdata[1].d2.fIn_Price;
				float fVDist_d2 = pairdata[0].d1.fOut_Price - pairdata[0].d2.fIn_Price;

				//if(pairdata[1].fPriceDistance < pairdata[0].fPriceDistance && fVDist_d1 < fVDist_d2)
				//{
				//	return FALSE;
				//}


				if(fVDist_d1 > fVDist_d2)
				{
					delete[] g_xd_l;
					return TRUE;
				}


				
				if(  pairdata[0].fXieLv < pairdata[1].fXieLv /*&& (pairdata[0].d2.nXDEnd_Index - pairdata[0].d2.nXDStart_Index) == 3 */)
				{
					delete[] g_xd_l;
					return TRUE;
				}
				
			}

		}
	}
	
	delete[] g_xd_l;
	OutputDebugStringA("[chs] �뿪 ZhongShuAnalu_BeiLi");
	return FALSE;
}




//��������з�����Ԥ����������
BOOL ZhongShuAnaly_YuCe()
{

	OutputDebugStringA("[chs] ZhongShuAnaly_YuCe");
	std::vector<ZhongShuData> g_vecZhongshu;

	BOOL bFlag = FALSE;

	ZhongShuData ZSData;

	float fkk_max;
	float fkk_min;

	for (int n = 0; n < nSize_xd_l; n++)
	{
		if(g_xd_l[n].XianDuan_nprop == 1)
		{
			bFlag = TRUE;
			ZSData.fMax = g_xd_l[n].fMax;
			ZSData.fMin = g_xd_l[n].fMin;

			ZSData.nXDStart_Index = n;
			ZSData.nXDStart_Before_Index = n-1;

			fkk_max = g_xd_l[n].PointHigh.fVal;
			fkk_min = g_xd_l[n].PointLow.fVal;
			//��㣬������ߵĵ�
			if(g_xd_l[n].PointHigh.nIndex < g_xd_l[n].PointLow.nIndex)
			{
				ZSData.nIn_Index = g_xd_l[n].PointHigh.nIndex;
				ZSData.fIn_Price = g_xd_l[n].PointHigh.fVal;

			}
			else
			{
				ZSData.nIn_Index = g_xd_l[n].PointLow.nIndex;
				ZSData.fIn_Price = g_xd_l[n].PointLow.fVal;

			}
		}

		if(g_xd_l[n].XianDuan_nprop == 2)
		{
			bFlag = FALSE;
			fkk_max = max(fkk_max, g_xd_l[n].PointHigh.fVal);
			fkk_min = min(fkk_min, g_xd_l[n].PointLow.fVal);

			ZSData.fkk_Max = fkk_max;
			ZSData.fkk_Min = fkk_min;

			//
			ZSData.nXDEnd_Index = n;

			//��㣬������ߵĵ�
			if(g_xd_l[n].PointHigh.nIndex > g_xd_l[n].PointLow.nIndex)
			{
				ZSData.nOut_Index = g_xd_l[n].PointHigh.nIndex;
				ZSData.fOut_Price = g_xd_l[n].PointHigh.fVal;
			}
			else
			{
				ZSData.nOut_Index = g_xd_l[n].PointLow.nIndex;
				ZSData.fOut_Price = g_xd_l[n].PointLow.fVal;
			}

			g_vecZhongshu.push_back(ZSData);
		}

		if(bFlag == TRUE)
		{
			fkk_max = max(fkk_max, g_xd_l[n].PointHigh.fVal);
			fkk_min = min(fkk_min, g_xd_l[n].PointLow.fVal);
		}
	}

	//�����ռ�����Ժ󣬿�ʼ����
	int nSize = g_vecZhongshu.size();

	if(nSize > 2)
	{
		std::vector<Pair_Data>  pairdata;
		//�ҵ���һ�����࣬����������ϵģ�����û�������ص��Ļ���ֱ���˳�

		ZSData = g_vecZhongshu[nSize-1];

		int nCount = 0; //�������ж������ǲ��ص�
		for (int n = nSize-2; n >= 0; n--)
		{
			BOOL bChongDie = FALSE;
			if(g_vecZhongshu[n].fMin > ZSData.fMax)
			{
				if(g_vecZhongshu[n].fkk_Min < ZSData.fkk_Max)
				{
					if(nCount == 0)
					{
						return FALSE;
					}
					bChongDie = TRUE;
				}
				else
				{
					Pair_Data pd;
					pd.d1 = g_vecZhongshu[n];
					pd.d2 = ZSData;
					pd.fXieLv = (pd.d1.fOut_Price - pd.d2.fIn_Price)/(pd.d2.nIn_Index-pd.d1.nOut_Index);
					pd.fChangDu = (pd.d1.fOut_Price - pd.d2.fIn_Price)*(pd.d1.fOut_Price - pd.d2.fIn_Price)*10000 + (pd.d2.nIn_Index-pd.d1.nOut_Index)*(pd.d2.nIn_Index-pd.d1.nOut_Index);

					if(ZSData.nXDEnd_Index - ZSData.nXDStart_Index >= 8 && pairdata.size() < 2)
					{
						return FALSE;
					}
					pairdata.push_back(pd);

					nCount++;
				}


			}
			else
			{
				//break;
				//�ж�һ���Ƿ����Ѿ������ཻ��������оͺ���,
				if ( ZSData.fMin > g_vecZhongshu[n].fMax &&   ZSData.fkk_Min >  g_vecZhongshu[n].fkk_Max )
				{
					break;
				}
				else
				{
					if(nCount == 0)
					{
						return FALSE;
					}
					bChongDie = TRUE;
				}

			}

			int nIndex = ZSData.nOut_Index;
			float fval = ZSData.fOut_Price;
			int xdtmp = ZSData.nXDEnd_Index;

			float fkkkmax = ZSData.fkk_Max;
			float fkkkmin = ZSData.fkk_Min;

			ZSData = g_vecZhongshu[n];

			if(bChongDie)
			{
				ZSData.nOut_Index = nIndex;
				ZSData.fOut_Price = fval;
				ZSData.nXDEnd_Index = xdtmp;//�ѽ�β��
				ZSData.fkk_Max = max(fkkkmax, ZSData.fkk_Max);//�����ص�
				ZSData.fkk_Min = min(fkkkmin, ZSData.fkk_Min);//
			}
		}

		//delete[] g_xd_l;

		//�����
		if(nCount)
		{

			//Ԥ���ʱ��Ҫ�ҵ��������࣬��������һ���������3���߶Σ����ҵ�һ���߶������£����ҵ�һ���߶εĳ��Ⱥ�ʱ��ȵ�һ��С
			//Ԥ�������Ҫ���̵�ĺ�
			int nIndexx = pairdata[0].d2.nXDEnd_Index;
			//for (int n = 0; n < nSize_xd_l; n++)
			//if(g_xd_l[n].XianDuan_nprop == 1)
			if(nIndexx < nSize_xd_l && (nSize_xd_l - nIndexx) >= 4)
			{
				if(g_xd_l[nIndexx+1].Bi_Direction == DOWN)
				{
					//pairdata[0].fPriceDistance;��һ���ļ�Ǯ
					//pairdata[0].fChangDu
					float fprice = g_xd_l[nIndexx+1].PointHigh.fVal - g_xd_l[nIndexx+1].PointLow.fVal;
					//int   nTimeCount = g_xd_l[nIndexx+1].
					float fVDist_d = pairdata[0].d1.fOut_Price - pairdata[0].d2.fIn_Price;
					if(fVDist_d > fprice)
					{
						{
							return TRUE;
							delete[] g_xd_l;
						}
						
					}
					
				}
			}	

			delete[] g_xd_l;
			return FALSE;


			if(nCount == 1)
			{
				//d1��ǰһ�ʱ��������µ�, d2Ҳ�����µ�
				{
					int nXdIndex = pairdata[0].d1.nXDStart_Before_Index;

					if(g_xd_l[nXdIndex].Bi_Direction == DOWN)
					{
						return FALSE;
					}

					//����������ҵ�ǰһ���������ߵ�
					float fVDist_d1 = g_xd_l[nXdIndex].PointHigh.fVal - g_xd_l[nXdIndex].PointLow.fVal;
					float fVDist_d2 = pairdata[0].d1.fOut_Price - pairdata[0].d2.fIn_Price;
					if(fVDist_d1 > fVDist_d2)
					{
						return TRUE;
					}

					float fxlv = (g_xd_l[pairdata[0].d1.nXDStart_Before_Index].PointHigh.fVal - g_xd_l[pairdata[0].d1.nXDStart_Before_Index].PointLow.fVal)/(float)(g_xd_l[pairdata[0].d1.nXDStart_Before_Index].PointHigh.nIndex - g_xd_l[pairdata[0].d1.nXDStart_Before_Index].PointLow.nIndex);
					if(fxlv > pairdata[0].fXieLv)
					{
						return TRUE;
					}
				}
			}
			else
			{

				float fVDist_d1 = pairdata[1].d1.fOut_Price - pairdata[1].d2.fIn_Price;
				float fVDist_d2 = pairdata[0].d1.fOut_Price - pairdata[0].d2.fIn_Price;
				if(fVDist_d1 > fVDist_d2)
				{
					return TRUE;
				}



				if(  pairdata[0].fXieLv < pairdata[1].fXieLv /*&& (pairdata[0].d2.nXDEnd_Index - pairdata[0].d2.nXDStart_Index) == 3 */)
				{
					return TRUE;
				}

			}

		}
	}

	OutputDebugStringA("[chs] �뿪 ZhongShuAnalu_BeiLi");
	return FALSE;
}



//��������з�����Ԥ����������
BOOL ZhongShuAnaly_TuPo()
{

	OutputDebugStringA("[chs] ZhongShuAnaly_TuPo");
	std::vector<ZhongShuData> g_vecZhongshu;

	BOOL bFlag = FALSE;

	ZhongShuData ZSData;

	float fkk_max;
	float fkk_min;

	for (int n = 0; n < nSize_xd_l; n++)
	{
		if(g_xd_l[n].XianDuan_nprop == 1)
		{
			bFlag = TRUE;
			ZSData.fMax = g_xd_l[n].fMax;
			ZSData.fMin = g_xd_l[n].fMin;

			ZSData.nXDStart_Index = n;
			ZSData.nXDStart_Before_Index = n-1;

			fkk_max = g_xd_l[n].PointHigh.fVal;
			fkk_min = g_xd_l[n].PointLow.fVal;
			//��㣬������ߵĵ�
			if(g_xd_l[n].PointHigh.nIndex < g_xd_l[n].PointLow.nIndex)
			{
				ZSData.nIn_Index = g_xd_l[n].PointHigh.nIndex;
				ZSData.fIn_Price = g_xd_l[n].PointHigh.fVal;

			}
			else
			{
				ZSData.nIn_Index = g_xd_l[n].PointLow.nIndex;
				ZSData.fIn_Price = g_xd_l[n].PointLow.fVal;

			}
		}

		if(g_xd_l[n].XianDuan_nprop == 2)
		{
			bFlag = FALSE;
			fkk_max = max(fkk_max, g_xd_l[n].PointHigh.fVal);
			fkk_min = min(fkk_min, g_xd_l[n].PointLow.fVal);

			ZSData.fkk_Max = fkk_max;
			ZSData.fkk_Min = fkk_min;

			//
			ZSData.nXDEnd_Index = n;

			//��㣬������ߵĵ�
			if(g_xd_l[n].PointHigh.nIndex > g_xd_l[n].PointLow.nIndex)
			{
				ZSData.nOut_Index = g_xd_l[n].PointHigh.nIndex;
				ZSData.fOut_Price = g_xd_l[n].PointHigh.fVal;
			}
			else
			{
				ZSData.nOut_Index = g_xd_l[n].PointLow.nIndex;
				ZSData.fOut_Price = g_xd_l[n].PointLow.fVal;
			}

			g_vecZhongshu.push_back(ZSData);
		}

		if(bFlag == TRUE)
		{
			fkk_max = max(fkk_max, g_xd_l[n].PointHigh.fVal);
			fkk_min = min(fkk_min, g_xd_l[n].PointLow.fVal);
		}
	}

	//�����ռ�����Ժ󣬿�ʼ����
	int nSize = g_vecZhongshu.size();

	if(nSize > 2)
	{
		std::vector<Pair_Data>  pairdata;
		//�ҵ���һ�����࣬����������ϵģ�����û�������ص��Ļ���ֱ���˳�

		ZSData = g_vecZhongshu[nSize-1];

		int nCount = 0; //�������ж������ǲ��ص�
		for (int n = nSize-2; n >= 0; n--)
		{
			BOOL bChongDie = FALSE;
			if(g_vecZhongshu[n].fMax < ZSData.fMin)
			{
				if(g_vecZhongshu[n].fkk_Max > ZSData.fkk_Min)
				{
					return FALSE;
					bChongDie = TRUE;
				}
				else
				{
					Pair_Data pd;
					pd.d1 = g_vecZhongshu[n];
					pd.d2 = ZSData;

					//pairdata.push_back(pd);
					
					if(pd.d1.nXDEnd_Index - pd.d1.nXDStart_Index >= 12)
					{
						return TRUE;
					}
					
					
					return FALSE;
					
				}
			}
			else
			{
				return FALSE;//ֱ�ӷ��������Ƚ��ϸ�
			}
		}

		
	}

	OutputDebugStringA("[chs] �뿪 ZhongShuAnalu_BeiLi");
	return FALSE;
}