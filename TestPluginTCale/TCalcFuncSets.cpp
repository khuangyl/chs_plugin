#include "stdafx.h"
#include "TCalcFuncSets.h"
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <time.h>

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
	int   nProp;		//标志，顶分或者底分标志 1 顶，-1 底
	int   nMegre;		//合并标志 1表示被合并，0表示没有
	float MegreHigh;	//合并后高点
	float MegreLow;	    //合并后低点
	int   nDirector;    //方向,和前一个比，1 向上，0，没有方向， -1 向下
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
	int nIndex; //笔的的点的数据的索引
	float fVal;	//如果是高就取最大值，如果是低点就取最小值
	int nprop;  //之前笔的高低点

	int MegerIndex;//这个时候是合并的，指向新的数据索引
}Bi_Point;

typedef struct _Bi_Line
{
	Bi_Point PointLow; //笔的低点
	Bi_Point PointHigh;//笔的高点
	KDirection Bi_Direction;//笔的方向

	int XianDuan_nprop;//这个代表了线段的高低点 
	int nMeger;

	int nMegerx2;

	_Bi_Line()
	{
		nMegerx2 = 0;
	}

}Bi_Line;

typedef struct _Xianduan_Line
{
	Bi_Point PointLow; //笔的低点
	Bi_Point PointHigh;//笔的高点
	KDirection Bi_Direction;//笔的方向

	int XianDuan_nprop;//中枢的标志，1代表起点，2代表
	int nMeger;

	float fMax;
	float fMin;

}Xianduan_Line;

Bi_Line *g_Bl;
int      g_nBlSize;

int g_nSize = 0;
KLine* g_tgKs = NULL;

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

//这个是经过包含处理后的判断，
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

/******************************************************
	生成的dll及相关依赖dll
	请拷贝到通达信安装目录的T0002/dlls/下面
	,再在公式管理器进行绑定
*******************************************************/
void TestPlugin1(int DataLen,float* pfOUT,float* High,float* Low, float* Close)
{
	//for(int i=0;i<DataLen;i++)
	//{
	//	pfOUT[i] = High[i];
	//}
	KDirection direction= NODIRECTION; //1:up, -1:down, 0:no drection。
	KLine* ks = new KLine[DataLen];
	KLine up_k; //向上临时K线
	KLine down_k; //向下临时k线
	KLine top_k=ks[0]; //最近的顶分型
	KLine bottom_k=ks[0]; //最近的底分型
	KLine tmp_k; //初始临时k线
	int down_k_valid=0; //向下有效k线数
	int up_k_valid=0; //向上有效k线数
	KDirection up_flag = NODIRECTION; //上涨标志

	//init ks lines by using the import datas
	for(int i=0;i<DataLen;i++){
		ks[i].index = i;
		ks[i].high = High[i];
		ks[i].low = Low[i];
		ks[i].prop = 0;
		ZeroMemory(&ks[i].Ext, sizeof(klineExtern));//先清0
	};
	
	//对ks数组数据进行分型处理
	tmp_k = ks[0];
	
	for(int i=1;i<DataLen;i++)
	{
		KLine curr_k=ks[i];
		KLine last=ks[i-1];
		/*
		从第二根K线起，需要和第一根K线相比，给出一个初始方向。
		假如两根K线为全包含，则仍未有方向，tmp_k为包含的K线。
		*/
		direction = (KDirection)last.Ext.nDirector;
		tmp_k = last;
		if(tmp_k.Ext.nMegre)
		{
			//如果有合并，就把合并的高低点给tmp_k
			tmp_k.high = tmp_k.Ext.MegreHigh;
			tmp_k.low = tmp_k.Ext.MegreLow;
		}

		switch(direction)
		{
			case NODIRECTION:
			{
				int include_flag = isIncluded(tmp_k, curr_k);
				if(include_flag == 0)//不包含
				{
					//如果上涨，则把tmp_k设置为底分型；否则，则设置为顶分型
					if (isUp(tmp_k,curr_k))
					{
						ks[i].Ext.nDirector = (int)UP;
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
			
				//是否包含,tmp_k是上一根最新的，如果有包含也是最新的
				if (isIncluded(tmp_k, curr_k)!=0)
				{
					
					//若包含则取向上包含,同时赋值
					up_k = kMerge(tmp_k, curr_k, direction);

					ks[i].high = up_k.high;
					ks[i].low = up_k.low;

					ks[i].Ext.MegreHigh = up_k.high;
					ks[i].Ext.MegreLow = up_k.low;
					ks[i].Ext.nDirector = (int)UP; //方向是向上包含
					ks[i].Ext.nMegre = 1; //包含标识
					ks[i-1].Ext.nMegre = -1; //-1代表后面会忽略掉
					
				}
				else
				{//没有包含的情况
					up_flag = isUp(tmp_k, curr_k);
					if(up_flag==UP)
					{
						ks[i].Ext.nDirector = (int)UP;
					}
					else if(up_flag == DOWN)
					{
						ks[i].Ext.nDirector = (int)DOWN;
					};
				};
				break;
			}
			case DOWN: 
			{			
				//是否包含,tmp_k是上一根最新的，如果有包含也是最新的
				if (isIncluded(tmp_k, curr_k)!=0)
				{

					//若包含则取向上包含,同时赋值
					down_k = kMerge(tmp_k, curr_k, direction);

					ks[i].high = down_k.high;
					ks[i].low = down_k.low;

					ks[i].Ext.MegreHigh = down_k.high;
					ks[i].Ext.MegreLow = down_k.low;
					ks[i].Ext.nDirector = (int)DOWN; //方向是向上包含
					ks[i].Ext.nMegre = 1; //包含标识
					ks[i-1].Ext.nMegre = -1; //-1代表后面会忽略掉

				}
				else
				{//没有包含的情况
					up_flag = isUp(tmp_k, curr_k);
					if(up_flag==UP)
					{
						ks[i].Ext.nDirector = (int)UP;
					}
					else if(up_flag == DOWN)
					{
						ks[i].Ext.nDirector = (int)DOWN;
					};
				};
				break;
			};
			default:
				break;
		}
	};


	for(int i=0;i<DataLen;i++)
	{
		pfOUT[i] = ks[i].high;
	}

	delete []ks;
}

void TestPlugin2(int DataLen,float* pfOUT,float* High,float* Low, float* Close)
{

	KDirection direction= NODIRECTION; //1:up, -1:down, 0:no drection。
	KLine* ks = new KLine[DataLen];
	KLine up_k; //向上临时K线
	KLine down_k; //向下临时k线
	KLine top_k=ks[0]; //最近的顶分型
	KLine bottom_k=ks[0]; //最近的底分型
	KLine tmp_k; //初始临时k线
	int down_k_valid=0; //向下有效k线数
	int up_k_valid=0; //向上有效k线数
	KDirection up_flag = NODIRECTION; //上涨标志

	//init ks lines by using the import datas
	for(int i=0;i<DataLen;i++){
		ks[i].index = i;
		ks[i].high = High[i];
		ks[i].low = Low[i];
		ks[i].prop = 0;
		ZeroMemory(&ks[i].Ext, sizeof(klineExtern));//先清0
	};
	
	//对ks数组数据进行分型处理
	tmp_k = ks[0];
	
	for(int i=1;i<DataLen;i++)
	{
		KLine curr_k=ks[i];
		KLine last=ks[i-1];

		//if(curr_k.high >= 10.399 && 
		//	curr_k.high < 10.401 &&
		//	curr_k.low >= 10.209   &&
		//	curr_k.low < 10.211)
		//{
		//	if(IsDebuggerPresent() == TRUE)
		//	{
		//		__asm int 3
		//	}
		//}

		/*
		从第二根K线起，需要和第一根K线相比，给出一个初始方向。
		假如两根K线为全包含，则仍未有方向，tmp_k为包含的K线。
		*/
		direction = (KDirection)last.Ext.nDirector;
		tmp_k = last;
		if(tmp_k.Ext.nMegre)
		{
			//如果有合并，就把合并的高低点给tmp_k
			tmp_k.high = tmp_k.Ext.MegreHigh;
			tmp_k.low = tmp_k.Ext.MegreLow;
		}

		switch(direction)
		{
			case NODIRECTION:
			{
				int include_flag = isIncluded(tmp_k, curr_k);
				if(include_flag == 0)//不包含
				{
					//如果上涨，则把tmp_k设置为底分型；否则，则设置为顶分型
					if (isUp(tmp_k,curr_k))
					{
						ks[i].Ext.nDirector = (int)UP;
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
			
				//是否包含,tmp_k是上一根最新的，如果有包含也是最新的
				if (isIncluded(tmp_k, curr_k)!=0)
				{
					//若包含则取向上包含,同时赋值
					up_k = kMerge(tmp_k, curr_k, direction);

					ks[i].high = up_k.high;
					ks[i].low = up_k.low;

					ks[i].Ext.MegreHigh = up_k.high;
					ks[i].Ext.MegreLow = up_k.low;
					ks[i].Ext.nDirector = (int)UP; //方向是向上包含
					ks[i].Ext.nMegre = 1; //包含标识
					ks[i-1].Ext.nMegre = -1; //-1代表后面会忽略掉
					
				}
				else
				{//没有包含的情况
					up_flag = isUp(tmp_k, curr_k);
					if(up_flag==UP)
					{
						ks[i].Ext.nDirector = (int)UP;
					}
					else if(up_flag == DOWN)
					{
						ks[i].Ext.nDirector = (int)DOWN;
					};
				};
				break;
			}
			case DOWN: 
			{			
				//是否包含,tmp_k是上一根最新的，如果有包含也是最新的
				if (isIncluded(tmp_k, curr_k)!=0)
				{

					//若包含则取向上包含,同时赋值
					down_k = kMerge(tmp_k, curr_k, direction);

					ks[i].high = down_k.high;
					ks[i].low = down_k.low;

					ks[i].Ext.MegreHigh = down_k.high;
					ks[i].Ext.MegreLow = down_k.low;
					ks[i].Ext.nDirector = (int)DOWN; //方向是向上包含
					ks[i].Ext.nMegre = 1; //包含标识
					ks[i-1].Ext.nMegre = -1; //-1代表后面会忽略掉

				}
				else
				{//没有包含的情况
					up_flag = isUp(tmp_k, curr_k);
					if(up_flag==UP)
					{
						ks[i].Ext.nDirector = (int)UP;
					}
					else if(up_flag == DOWN)
					{
						ks[i].Ext.nDirector = (int)DOWN;
					};
				};
				break;
			};
			default:
				break;
		}
	};


	for(int i=0;i<DataLen;i++)
	{
		pfOUT[i] = ks[i].low;
	}

	delete []ks;
}

//判断缺口，一笔内如果有缺口就处理该缺口, 处理向上一笔中缺口
//假设上升过程中，如果有一个大的向下的缺口，并且返回
int Handle_One_Pen_QueKou_Up(KLine* ks, int nlow, int nhigh)
{
	
	for(int n = nlow+2; n < nhigh; n++)
	{
		if( ks[n-1].low >= ks[nlow].high  &&  ks[n].high < ks[nlow].low)
		{
			//已经是一个大缺口
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
			//已经是一个大缺口
			ks[n-1].prop = -1;
			ks[n].prop = 1;
			return TRUE;
		}
	}
	return 0;
}

//首先进行2个分型进行判断，有时候分型不ok也不能算一笔
BOOL Is_FengXing_Ok(KLine* ks, int nlow, int nhigh, KDirection Direction)
{
	if(Direction == UP)
	{
		int kl1 = 0; //底分型的高一点
		int kh1 = 0; //顶分型的低一点


		int kl_before = 0; //向上笔，底分型的前一个k线
		int kh_after = 0; //向上笔，顶分型的后一个k线

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

		//1，顶分型的最高点比kl1的最高点要高
		//2, 底分型的最低点比kh1的最低点低
		if(ks[nhigh].high > ks[kl1].high  && ks[nlow].low < ks[kh1].low)
		{
			return TRUE;
		}


	}
	else
	{
		//方向是向下的
		int kl1 = 0; //底分型的高一点
		int kh1 = 0; //顶分型的低一点

		int kh_before = 0; //向下笔，顶分型的前一个k线
		int kl_after = 0; //向下笔，底分型的后一个k线


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

		//1，顶分型的最高点比kl1的最高点要高
		//2, 底分型的最低点比kh1的最低点低
		if(ks[nlow].high > ks[kl1].high  && ks[nhigh].low < ks[kh1].low)
		{
			return TRUE;
		}
	}

	return FALSE;
}

//**********************************************新笔*******************************************************/
//特殊处理函数：如果是分型的一笔破坏了顶底之间的关系这个时候需要单独来处理
void SpecHandleUp(KLine* ks, int nlow, int nhigh)
{
	if(nhigh - nlow < 20)
	{
		//注意，这个是一个脑门值，也就是这超过了20个k线才考虑
		return ;
	}

	int kl1 = 0; //底分型的高一点
	int kh1 = 0; //顶分型的低一点

	//for(int n = nlow+1; n < nhigh; n++)
	//{
	//	if(ks[n].Ext.nMegre != -1)
	//	{
	//		kl1 = n;
	//		break;
	//	}
	//}

	kl1 = nlow+1;

	for(int n = nhigh-1; n > nlow; n--)
	{
		if(ks[n].Ext.nMegre != -1)
		{
			kh1 = n;
			break;
		}
	}

	//如果出现kl1大于kh1的时候做特殊处理
	if(ks[kl1].high >= ks[kh1].high)
	{
		//1,先找出最低点

	}
}



//首先进行2个分型进行判断，有时候分型不ok也不能算一笔
BOOL Is_FengXing_Ok_NewOpen_ZuiGaoZuiDi(KLine* ks, int nlow, int nhigh, KDirection Direction)
{
	if(Direction == UP)
	{
		int kl1 = 0; //底分型的高一点
		int kh1 = 0; //顶分型的低一点

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
		//方向是向下的
		int kl1 = 0; //底分型的高一点
		int kh1 = 0; //顶分型的低一点


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


//首先进行2个分型进行判断，有时候分型不ok也不能算一笔
BOOL Is_FengXing_Ok_NewOpen(KLine* ks, int nlow, int nhigh, KDirection Direction)
{
	if(Direction == UP)
	{
		int kl1 = 0; //底分型的高一点
		int kh1 = 0; //顶分型的低一点

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

		//1，顶分型的最高点比kl1的最高点要高
		//2, 底分型的最低点比kh1的最低点低
		if(ks[nhigh].high > ks[kl1].high  && ks[nlow].low < ks[kh1].low)
		{
			return TRUE;
		}


	}
	else
	{
		//方向是向下的
		int kl1 = 0; //底分型的高一点
		int kh1 = 0; //顶分型的低一点


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

		//1，顶分型的最高点比kl1的最高点要高
		//2, 底分型的最低点比kh1的最低点低
		if(ks[nlow].high > ks[kl1].high  && ks[nhigh].low < ks[kh1].low)
		{
			return TRUE;
		}
	}

	return FALSE;
}

//判断是否是符合一笔条件,是否是向上的一笔
//新笔，只需要判断两个分型就ok，就是
BOOL bIsOne_Open_Up_NewOpen(KLine* ks, int nlow, int nhigh)
{
	if(nhigh - nlow < 4)
	{
		return FALSE;
	}


	//先进行分型判断
	if(Is_FengXing_Ok_NewOpen(ks,  nlow,  nhigh, UP) == FALSE)
	{

		//补锅：如果有一根k线太过分的话，就要这个k线见鬼去吧，
		//做法如下：如果找到一个最低点和这个最新的顶点能形成一个向下一笔和向上的一笔的话可以了
		

		return FALSE;
	}			



	int kl1 = 0; //底分型的高一点
	int kh1 = 0; //顶分型的低一点

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


	if(isUp(ks[kl1], ks[kh1]) == UP)
	{
		//顺便在这里做一下缺口的处理，如果ks[nhigh+1]和ks[nhigh]有个大缺口，即已经构成了一笔，的情况下
		if(ks[nhigh+1].high < ks[nlow].low)
		{
			ks[nhigh+1].prop = -1;
		}
		return TRUE;
	}
	

	return FALSE;
}

//判断是否是符合一笔条件,是否是向下的一笔
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


	int kl1 = 0; //底分型的高一点
	int kh1 = 0; //顶分型的低一点

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

	if(isUp(ks[kh1], ks[kl1]) == DOWN)
	{
		//顺便在这里做一下缺口的处理，如果ks[nhigh+1]和ks[nhigh]有个大缺口，即已经构成了一笔，的情况下
		if(ks[nhigh+1].low > ks[nlow].high)
		{
			ks[nhigh+1].prop = 1;
		}
		return TRUE;
	}
	

	return FALSE;
}

//***********************************************新笔end*********************************************************/


//判断是否是符合一笔条件,是否是向上的一笔
BOOL bIsOne_Open_Up(KLine* ks, int nlow, int nhigh)
{
	//先进行分型判断
	if(Is_FengXing_Ok(ks,  nlow,  nhigh, UP) == FALSE)
	{
		return FALSE;
	}			

	//这里再加一个条件，如果中间只有4根线（不包括顶底分型），不破新高或者新低都不能算
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

	int kl1 = 0; //底分型的高一点
	int kh1 = 0; //顶分型的低一点

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
				//判断完分型后再做缺口判断,主要判断nlow, nhigh, nhigh+1之间的关系
				if(ks[nhigh].low >= ks[nlow].high && ks[nhigh+1].high <= ks[nlow].low)
				{
					ks[nhigh+1].prop = -1;//直接说明是底分型了
					return TRUE;
				}

			}
		}
	}

	if(1)
	{
		//这里属于放宽条件，在两点间只要是5根k线自由排列都行
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
								//判断完分型后再做缺口判断,主要判断nlow, nhigh, nhigh+1之间的关系
								if(ks[nhigh].low >= ks[nlow].high && ks[nhigh+1].high <= ks[nlow].low)
								{
									ks[nhigh+1].prop = -1;//直接说明是底分型了
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

//判断是否是符合一笔条件,是否是向下的一笔
BOOL bIsOne_Open_Down(KLine* ks, int nlow, int nhigh)
{

	if(Is_FengXing_Ok(ks,  nlow,  nhigh, DOWN) == FALSE)
	{
		return FALSE;
	}	

	//这里再加一个条件，如果中间只有4根线（不包括顶底分型），不破新高或者新低都不能算
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

	int kl1 = 0; //底分型的高一点
	int kh1 = 0; //顶分型的低一点

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
				//判断完分型后再做缺口判断主要判断nlow, nhigh, nhigh+1之间的关系
				if(ks[nhigh].high <= ks[nlow].low && ks[nhigh+1].low >= ks[nlow].high)
				{
					ks[nhigh+1].prop = 1;//直接说明是顶分型了
					return TRUE;
				}

			}
		}
	}


	if(1)
	{
		//这里属于放宽条件，在两点间只要是5根k线自由排列都行
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
								//判断完分型后再做缺口判断主要判断nlow, nhigh, nhigh+1之间的关系
								if(ks[nhigh].high <= ks[nlow].low && ks[nhigh+1].low >= ks[nlow].high)
								{
									ks[nhigh+1].prop = 1;//直接说明是顶分型了
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

void Handle_Right_DFX_Diyu_Left_DFX(KLine* ks, int nlowDFX, int nhighDFX, int *Outi)
{
	//由于已经清理掉nlowDFX这个底分型的标志位，往前找上一顶分型
	int n = nlowDFX;
	while(n)
	{
		n--;
		if(n <= 0)
		{
			break;
		}

		if(ks[n].prop == 1)
		{
			int TopIndex = n;
			for(int k = n+1; k < nhighDFX; k++)
			{
				//从上个顶分型找起，找到先在这个底分型，找到最高点，然后把最高点替换之前的顶分型，再把顶分型的后面的k先索引返回去再重新搜索
				if(ks[k].high >= ks[TopIndex].high && ks[k].Ext.nDirector != -1)
				{
					TopIndex = k;
				}
			}

			if(TopIndex != n)
			{
				ks[n].prop = 0;
				ks[TopIndex].prop = 1;
				//ks[TopIndex].Ext.nMegre = 0;
				*Outi = TopIndex+2;
			}
			else
			{
				ks[nhighDFX].prop = -1;
				ks[nhighDFX].Ext.nProp = -1;
			}
			return;

		}
		else if(ks[n].prop == -1)
		{
			return;
		}
	}
}

void Handle_Right_TFX_Dayu_Left_TFX(KLine* ks, int nlowDFX, int nhighDFX, int *Outi)
{
	//由于已经清理掉nlowTFX这个顶分型的标志位，往前找上一底分型
	int n = nlowDFX;
	while(n)
	{
		n--;
		if(n <= 0)
		{
			break;
		}

		if(ks[n].prop == -1)
		{
			int TopIndex = n;
			for(int k = n+1; k < nhighDFX; k++)
			{
				//从上个顶分型找起，找到先在这个底分型，找到最高点，然后把最高点替换之前的顶分型，再把顶分型的后面的k先索引返回去再重新搜索
				if(ks[k].low <= ks[TopIndex].low && ks[k].Ext.nDirector != -1)
				{
					TopIndex = k;
				}
			}

			if(TopIndex != n)
			{
				ks[n].prop = 0;
				ks[TopIndex].prop = -1;
				//ks[TopIndex].Ext.nMegre = 0;
				*Outi = TopIndex+2;
			}
			else
			{
				ks[nhighDFX].prop = 1;
				ks[nhighDFX].Ext.nProp = 1;
			}
			return;
		}
		else if(ks[n].prop == 1)
		{
			return;
		}
	}
}

//如果是返回值是TRUE就说明已经是缺口，并且已经处理过了，否则就返回false

//找上一个分型，然后根据传进来的方向，找到这个方向是不是有5个同方向的来确定分型
int Handle_FenXing(KLine* ks, int DataLen, int i, KDirection Direction, int *Outi)
{
	int nValid = 0;
	int n = i;
	if(Direction == UP)//顶分型
	{
		//因为是向上的方向，先倒遍历，找到上一个底分型，然后再判断是不是足够有5个连续同向上的k线
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

				if(ks[n].prop == 1)//顶分型
				{
					//顶分型标志位, 不符合现在找的目标，两个顶分型做对比
					if(k1.high >= k2.high)
					{
						//左边的比右边的高,直接返回
						return 0;
					}
					else
					{
						//左边的没有右边的高，清掉左边的
						ks[n].prop = 0;
						ks[n].Ext.nProp = 0;

						ks[i].prop = 1;      //暂时先去掉
						ks[i].Ext.nProp = 1; //暂时先去掉

						//这里也要做一次分型缺口的判断，就是如果缺口类型在分型的时候，就要判断
						//顺便在这里做一下缺口的处理，如果ks[nhigh+1]和ks[nhigh]有个大缺口，即已经构成了一笔，的情况下
						//(这个代码在bIsOne_Open_Up_NewOpen就有的)
						//先找到底分型
						for (int kkk = n; kkk > 0; kkk--)
						{
							if(ks[kkk].prop == -1)
							{
								if(ks[i+1].high < ks[kkk].low)
								{
									ks[i+1].prop = -1;
								}
								break;
							}
						}
						return 0;
					}
				}
				else //底分型 
				{
					//找到底分型以后，先判断，必须是向上方向
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
							//判断玩如果不是一笔的话，还有进行处理
						}
					}
					else
					{
						//如果方向都不算是向上，放弃，啥都不干
						return 0;
					}

				}

				return 0;
			}
		}



		//遍历完以后如果还没有找到任何一个分型, 直接标记顶分型，然后返回
		ks[i].prop = 1;
		ks[i].Ext.nProp = 1;
		return 0;
	}
	else if(Direction == DOWN)
	{
		//因为是向下的方向，先倒遍历，找到上一个顶分型，然后再判断是不是足够有5个连续同向下的k线
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

				if(ks[n].prop == -1)//底分型
				{
					//底分型标志位, 不符合现在找的目标，两个底分型做对比
					if(k1.low <= k2.low)
					{
						//左边的比右边的低,直接返回
						return 0;
					}
					else
					{
						//左边的没有右边的低，清掉左边的
						ks[n].prop = 0;
						ks[n].Ext.nProp = 0;

						ks[i].prop = -1;
						ks[i].Ext.nProp = -1;


						//这里也要做一次分型缺口的判断，就是如果缺口类型在分型的时候，就要判断
						//顺便在这里做一下缺口的处理，如果ks[nhigh+1]和ks[nhigh]有个大缺口，即已经构成了一笔，的情况下
						//(这个代码在bIsOne_Open_Up_NewOpen就有的)
						//先找到底分型
						for (int kkk = n; kkk > 0; kkk--)
						{
							if(ks[kkk].prop == 1)
							{
								if(ks[i+1].low > ks[kkk].high)
								{
									ks[i+1].prop = 1;
								}
								break;
							}
						}
						return 0;
					}
				}
				else //顶分型
				{
					//找到底分型以后，先判断，必须是向上方向
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
							//判断玩如果不是一笔的话，还有进行处理
							//需要再次判断这次的底和上次的底比较，如果找到了，
							//Handle_No_Pen_Down(ks, i);
						}
					}
					else
					{
						//如果方向都不算是向上，放弃，啥都不干
						return 0;
					}

				}
				return 0;
			}
		}



		//遍历完以后如果还没有找到任何一个分型, 直接标记顶分型，然后返回
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

void TestPlugin3(int DataLen,float* Out,float* High,float* Low, float* TIME/*float* Close*/)
{
	OutputDebugStringA("[chs] TestPlugin3 ");
	KDirection direction= NODIRECTION; //1:up, -1:down, 0:no drection。
	KLine* ks = new KLine[DataLen];
	KLine up_k; //向上临时K线
	KLine down_k; //向下临时k线
	KLine top_k=ks[0]; //最近的顶分型
	KLine bottom_k=ks[0]; //最近的底分型
	KLine tmp_k; //初始临时k线
	int down_k_valid=0; //向下有效k线数
	int up_k_valid=0; //向上有效k线数
	KDirection up_flag = NODIRECTION; //上涨标志

	//init ks lines by using the import datas
	for(int i=0;i<DataLen;i++){
		ks[i].index = i;
		ks[i].high = High[i];
		ks[i].low = Low[i];
		ks[i].prop = 0;
		ZeroMemory(&ks[i].Ext, sizeof(klineExtern));//先清0
	};

	//对ks数组数据进行分型处理
	tmp_k = ks[0];
	
	for(int i=1;i<DataLen;i++)
	{
		KLine curr_k=ks[i];
		KLine last=ks[i-1];

		if(curr_k.high >= 28.329 && 
			curr_k.high < 28.331 &&
			curr_k.low >= 27.469   &&
			curr_k.low <  27.471)
		{

		if(ks[i+1].high >= 28.239 && 
			ks[i+1].high < 28.241 &&
			ks[i+1].low >= 27.929   &&
			ks[i+1].low <  27.931)
			{
				if(IsDebuggerPresent() == TRUE)
				{
					__asm int 3
				}
			}
			
		}

		/*
		从第二根K线起，需要和第一根K线相比，给出一个初始方向。
		假如两根K线为全包含，则仍未有方向，tmp_k为包含的K线。
		*/
		direction = (KDirection)last.Ext.nDirector;
		tmp_k = last;
		if(tmp_k.Ext.nMegre)
		{
			//如果有合并，就把合并的高低点给tmp_k
			tmp_k.high = tmp_k.Ext.MegreHigh;
			tmp_k.low = tmp_k.Ext.MegreLow;
		}

		switch(direction)
		{
			case NODIRECTION:
			{
				int include_flag = isIncluded(tmp_k, curr_k);
				if(include_flag > 0)//左包含，这里是否应该是 include_flag != 0 也就是不管左右包含都是同样处理
				{
					//tmp_k = curr_k;刚刚开始的时候，有包含，但是不知道怎么处理好
				}
				else if(include_flag == 0)//不包含
				{
					//如果上涨，则把tmp_k设置为底分型；否则，则设置为顶分型
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
			
				//是否包含,tmp_k是上一根最新的，如果有包含也是最新的
				//这里要判断左右包含，如果是左包含才算，有包含不算包含
				if (isIncluded(tmp_k, curr_k)!=0)
				{
					
					//若包含则取向上包含,同时赋值
					up_k = kMerge(tmp_k, curr_k, direction);

					ks[i].high = up_k.high;
					ks[i].low = up_k.low;

					ks[i].Ext.MegreHigh = up_k.high;
					ks[i].Ext.MegreLow = up_k.low;
					ks[i].Ext.nDirector = (int)UP; //方向是向上包含
					ks[i].Ext.nMegre = 1; //包含标识
					ks[i-1].Ext.nMegre = -1; //-1代表后面会忽略掉
					if(ks[i-1].prop)
					{
						ks[i].prop = ks[i-1].prop;
						 ks[i-1].prop = 0;
					}
				}
				else
				{//没有包含的情况
				
					//判断是否上涨
					up_flag = isUp(tmp_k, curr_k);
					if(up_flag==UP)
					{
						
						//继续上涨则把当前K线置换为上涨临时K线
						//如果方向相同,只把方向标识位改变，其他的就不用去管
						ks[i].Ext.nDirector = (int)UP;
					}
					else if(up_flag == DOWN)
					{
						ks[i].Ext.nDirector = (int)DOWN;
						//不继续上涨
						//判断顶分型是否成立
						Handle_FenXing(ks, DataLen, i-1, UP, &i);
					};
				};
				break;
			}
			case DOWN: 
			{			
				//是否包含,tmp_k是上一根最新的，如果有包含也是最新的
				if (isIncluded(tmp_k, curr_k)!=0)
				{

					//若包含则取向上包含,同时赋值
					down_k = kMerge(tmp_k, curr_k, direction);

					ks[i].high = down_k.high;
					ks[i].low = down_k.low;

					ks[i].Ext.MegreHigh = down_k.high;
					ks[i].Ext.MegreLow = down_k.low;
					ks[i].Ext.nDirector = (int)DOWN; //方向是向上包含
					ks[i].Ext.nMegre = 1; //包含标识
					ks[i-1].Ext.nMegre = -1; //-1代表后面会忽略掉
					if(ks[i-1].prop)
					{
						ks[i].prop = ks[i-1].prop;
						ks[i-1].prop = 0;
					}

				}
				else
				{//没有包含的情况

					//判断是否继续下跌
					up_flag = isUp(tmp_k, curr_k);
					if(up_flag==DOWN)
					{

						//如果方向相同,只把方向标识位改变，其他的就不用去管
						curr_k.Ext.nDirector = (int)DOWN;
						ks[i].Ext.nDirector = (int)DOWN;
					}
					else if(up_flag == UP)
					{
						ks[i].Ext.nDirector = (int)UP;
						//不继续下跌
						//判断顶分型是否成立
						Handle_FenXing(ks, DataLen, i-1, DOWN, &i);
					};
				};

				break;
			};
			default:
				break;
		}
	};


	//在最后做一次缺口的处理
	//Handle_QueKou(ks, DataLen);


	for(int i=0;i<DataLen;i++)
	{
		Out[i] = ks[i].prop;
		if(ks[i].prop)
		{
			char szbuff[128] = {0};
			sprintf(szbuff, "[chs] index=%d, prop=%.2f, high=%.2f, low=%.2f", ks[i].index, ks[i].prop, ks[i].high, ks[i].low);

			//OutputDebugStringA(szbuff);
		}
	};

	//k线数据拷贝
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

	OutputDebugStringA("[chs] TestPlugin3 ends");
}


//-----------------------------------------------------线段的处理函数---------------------------------------------------
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
		return -1;//左包含
	}
	else if((kleft.PointHigh.fVal <= kright.PointHigh.fVal) && (kleft.PointLow.fVal >= kright.PointLow.fVal))
	{
		return 1;//右包含
	}
	else
	{
		return 0;//不包含
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


//参数1是已经经过处理的序列特征
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



//特征序列合并，把结果传出来
int Te_Zheng_XuLie_Meger(Bi_Line *Bl, int nStartk, int nLen, Bi_Line *BlxOut)
{
	//BlxOut = new Bi_Line[nLen];
	memcpy(BlxOut, Bl, sizeof(Bi_Line)*nLen);

	//向上的线段，
	if(BlxOut[nStartk].Bi_Direction == UP)
	{
		//首先特征序列进行包含，先找到
		for (int k = 0; nStartk+k*2+3 < nLen; k++)
		{
			int nRet = isIncluded_XianDuan(BlxOut[nStartk+k*2+1],  BlxOut[nStartk+k*2+3]);
			if( nRet== -1)//只有左包含才处理
			{
				//不管左右包含都是取上
				Bi_Line temp =  kMerge_XianDuan(BlxOut[nStartk+k*2+1], BlxOut[nStartk+k*2+3], UP);
				BlxOut[nStartk+k*2+1].nMeger = -1;
				BlxOut[nStartk+k*2+3].nMeger = 1;


				//if(BlxOut[nStartk+k*2+3].PointHigh.fVal != temp.PointHigh.fVal)
				{
					//说明比前一个高点低
					BlxOut[nStartk+k*2+3].PointHigh.MegerIndex = BlxOut[nStartk+k*2+3].PointHigh.nIndex;//保存原来的点，
					BlxOut[nStartk+k*2+3].PointHigh.nIndex = BlxOut[nStartk+k*2+1].PointHigh.nIndex;//把新店赋值给它				 
				}

				BlxOut[nStartk+k*2+3].PointHigh.fVal = temp.PointHigh.fVal;
				BlxOut[nStartk+k*2+3].PointLow.fVal = temp.PointLow.fVal;
			}
			else if(nRet == 1)
			{
				//右包含
				BlxOut[nStartk+k*2+1].nMegerx2 = 1;
			}

		}
	}
	else
	{
		//首先特征序列进行包含，先找到
		for (int k = 0; nStartk+k*2+3 < nLen; k++)
		{
			int nRet = isIncluded_XianDuan(BlxOut[nStartk+k*2+1],  BlxOut[nStartk+k*2+3]);
			if(nRet == -1)
			{
				//不管左右包含都是取上
				Bi_Line temp =  kMerge_XianDuan(BlxOut[nStartk+k*2+1], BlxOut[nStartk+k*2+3], DOWN);
				BlxOut[nStartk+k*2+1].nMeger = -1;
				BlxOut[nStartk+k*2+3].nMeger = 1;

				//if(BlxOut[nStartk+k*2+3].PointLow.fVal != temp.PointLow.fVal)
				{
					//说明比前一个高点低
					BlxOut[nStartk+k*2+3].PointLow.MegerIndex = BlxOut[nStartk+k*2+3].PointLow.nIndex;//保存原来的点，
					BlxOut[nStartk+k*2+3].PointLow.nIndex = BlxOut[nStartk+k*2+1].PointLow.nIndex;//把新店赋值给它
				}

				BlxOut[nStartk+k*2+3].PointHigh.fVal = temp.PointHigh.fVal;
				BlxOut[nStartk+k*2+3].PointLow.fVal = temp.PointLow.fVal;
			}
			else if(nRet == 1)
			{
				//右包含
				BlxOut[nStartk+k*2+1].nMegerx2 = 1;
			}

		}
	}

	return 0;
}

//获取到破坏一笔(拐点)前一个笔(最高，或者最低，因为存在伪线段的存在，暂时就用找破坏笔的前一个最高或者最低点来判断)
//nStart是开始的位置 用来判断是第一种破坏还是第二种破
int Get_GuaiDian_Before_Bi(Bi_Line *Bl, int k, KDirection nDirect, int nStart)
{
	//k是拐点点，由这个点往前推算，找到一个标志位XianDuan_nprop,说明是
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


	//方向是向下
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
					//说明比前一个高点低
					BlxOut[nStartk+k*2+3].PointHigh.MegerIndex = BlxOut[nStartk+k*2+3].PointHigh.nIndex;//保存原来的点，
					BlxOut[nStartk+k*2+3].PointHigh.nIndex = BlxOut[nStartk+k*2+1].PointHigh.nIndex;//把新店赋值给它				 
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
					//说明比前一个高点低
					BlxOut[nStartk+k*2+3].PointLow.MegerIndex = BlxOut[nStartk+k*2+3].PointLow.nIndex;//保存原来的点，
					BlxOut[nStartk+k*2+3].PointLow.nIndex = BlxOut[nStartk+k*2+1].PointLow.nIndex;//把新店赋值给它
				}

				BlxOut[nStartk+k*2+3].PointHigh.fVal = temp.PointHigh.fVal;
				BlxOut[nStartk+k*2+3].PointLow.fVal = temp.PointLow.fVal;
			}
		}
		else
		{
			//不包含，只改变方向
			Directionxx = isUp_XianDuan(Temp,  BlxOut[nStartk+k*2+3]);
		}

		Temp = BlxOut[nStartk+k*2+3];

	}

	return 0;
}

//从该点起找分型,返回分型的位置
int Is_XianDuan_FenXing(Bi_Line *Bl, int nStartk, int nLen, KDirection nDirect)
{
	Bi_Line *BlxOut = new Bi_Line[nLen];;
	Te_Zheng_XuLie_Meger_For_FengXing(Bl, nStartk, nLen, BlxOut);

	KDirection nDirectxx = NODIRECTION;

	if(DOWN == nDirect)//方向向下，找出顶分型
	{
		if(BlxOut[nStartk].Bi_Direction != DOWN)
		{
			//先判断一下吧避免出现啥问题
			delete BlxOut;
			return 0;
		}

		int nCount = 0;
		Bi_Line  Temp = BlxOut[nStartk +1];
		for (int k = 1; nStartk + 2*k+1 < nLen; k++)
		{
			//找到特征序列的分型
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
		//if(DOWN == nDirect)//方向向上，找出底分型
		{
			if(BlxOut[nStartk].Bi_Direction != UP)
			{
				//先判断一下吧避免出现啥问题
				delete BlxOut;
				return 0;
			}

			int nCount = 0;
			Bi_Line  Temp = BlxOut[nStartk +1];
			for (int k = 1; nStartk + 2*k+1 < nLen; k++)
			{
				//找到特征序列的分型
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


//获取到没有合并的一笔
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

//传入的Bl[0]是线段的开始点，而且这个点是一个真实点
//一定要确定下一个线段经存在的情况下才能返回
//所以先找到拐点，然后确认拐点后面确实能形成线段
//破坏的那一笔很重要，
int Lookup_Next_XianDuan(Bi_Line *Bl, int nStartk, int nLen)
{
	BOOL bFlagGuaiDian = FALSE;
	//画起点
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
				//先找到一个特征序列的一个拐点，让判断是是不是满足条件1或者2
				if(BlxOut[nStartk+2*k+3].PointHigh.fVal < Temp.PointHigh.fVal)
				{
					bFlagGuaiDian = TRUE;

					//DrawGuaiDian(BlxOut,  nStartk,  k);//画拐点的笔

					//满足拐点的，说明已经出现了第一个或者第二个条件了
					//判断是第一个情况或者第二个情况
					//2*k+1的低点 < 2*k-1的高点,就是第一种情况，出现，这个时候不要找特征序列，而是判断后面的是不是能够形成一个线段
					//这里应该不要考虑2*k+1, 2*k-1的Meger特性了吧，还真需要考虑2*k-1的特性
					//int np = Get2k_dec_1(BlxOut, nStartk+2*k+1);
					int np = Get_GuaiDian_Before_Bi(BlxOut, nStartk+2*k+1, UP, nStartk);

					//DrawGuaiDian_Before_bi(BlxOut,  nStartk, np);//画拐点前的一笔
					//还有找到真正的拐点笔
					int nreal_gd = Get_GuaiDian_Real(BlxOut, nStartk+2*k+1, UP, nStartk);//拐点前的一笔

					if(np)
					{
						if(BlxOut[nreal_gd].PointLow.fVal <= BlxOut[np].PointHigh.fVal)
						{
							int nRet = 0;

							//画出线段之前---要先判断是否有meger,因为是左包含原则，所以要找出和nStartk+2*k+1 meger的最前面的一个笔
							//然后这个笔就是该线段最后一笔
							if(BlxOut[nStartk+2*k+1].nMeger != 0)
							{
								//确实证实了是meger
								int n_no_meger_bi = Get_No_Meger_Bi(BlxOut, nStartk+2*k+1);
								nRet = n_no_meger_bi;


								//还要处理包含的问题，就是处理Bl的问题
								Bl[nStartk+2*k+1].PointHigh.nIndex = Bl[n_no_meger_bi].PointHigh.nIndex;
								Bl[nStartk+2*k+1].PointHigh.fVal = Bl[n_no_meger_bi].PointHigh.fVal;

								//还有处理显示的数据
								//vecData[nStartk+2*k+1].pt1.x = vecData[n_no_meger_bi].pt1.x;
								//vecData[nStartk+2*k+1].pt1.y = vecData[n_no_meger_bi].pt1.y;

							}	
							else
							{
								nRet = nStartk+2*k+1;
							}


							//Draw_QueDing_Xianduan_First_UP(BlxOut, nStartk, nRet);//画第一情况的笔

							//确定是第一种情况的出现了，这个时候假设2*k+1的高点为线段的起点，找到向下的线段，再一次找到拐点就算是一个线段的完成				
							Bl[nStartk+2*k+1].XianDuan_nprop = 1;
							delete BlxOut;
							return nStartk+2*k+1;

						}
						else
						{
							//这个是第二种情况下，也就是一笔并没有破坏了。
							//假设2*k+1为起点，如果找到底分型的特征序列，就算是完成了一个线段的开始
							//Is_XianDuan_FenXing这个函数返回底分型最低的位置的k
							int npos = Is_XianDuan_FenXing(BlxOut, nStartk+2*k+1, nLen, DOWN);
							if(npos)
							{
								//Draw_Second_FengXing(BlxOut, npos);//画出第二种情况的分型

								//找到底分型了，然后看看这个底分型和拐点之间是否已经是一个线段
								//判断方法，拐点间到npos如果有笔高过拐点，那说这个拐点不是一个线段的点，需要用用高一点的点来继续
								int j = k+1;
								//for (int j = nStartk+2*k+1; j <= npos; j++)
								BOOL bFlagRet = FALSE;
								for (; nStartk+2*j < npos; j++)
								{
									//如果有点低于某个笔的点低于拐点的前一个点，也能构成
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
									//已经是确定形成了

									if(BlxOut[nStartk+2*k+1].nMeger)
									{
										while(1)
										{
											k--;

											if(BlxOut[nStartk+2*k+1].nMeger != -1)
											{
												k++;
												//Draw_QueDing_Xianduan_Second_UP(BlxOut,  nStartk,  k);//画第二种线段的形成

												//也就是能形成线段
												Bl[nStartk+2*k+1].XianDuan_nprop = 1;
												delete BlxOut;
												return nStartk+2*k+1;
											}
										}
									}
									else
									{
										//Draw_QueDing_Xianduan_Second_UP(BlxOut,  nStartk,  k);//画第二种线段的形成

										//也就是能形成线段
										Bl[nStartk+2*k+1].XianDuan_nprop = 1;
										delete BlxOut;
										return nStartk+2*k+1;
									}


								}
								else
								{
									//::MessageBoxA(m_hWnd, "找到分型也不能形成线段", NULL, MB_OK);
									Temp = BlxOut[nStartk+2*k+3];
								}

							}
							else
							{
								//::MessageBoxA(m_hWnd, "找不到第二种情况的分型", NULL, MB_OK);
								//到这里有可能说明是快结束了，应该要用结束的情况来分析
								//因为是向下的方向，所以直接找最低的笔，在看看是否能够成为一个
								//到这里已经有拐点出现了，说明拐点有可能形成一个线段，
								//有两种情况，
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
									//已经形成线段线段
									int  nRet = 0;
									if(BlxOut[nStartk+2*k+1].nMeger != 0)
									{
										//确实证实了是meger
										int n_no_meger_bi = Get_No_Meger_Bi(BlxOut, nStartk+2*k+1);
										nRet = n_no_meger_bi;

										//还要处理包含的问题，就是处理Bl的问题
										Bl[nStartk+2*k+1].PointHigh.nIndex = Bl[n_no_meger_bi].PointHigh.nIndex;
										Bl[nStartk+2*k+1].PointHigh.fVal = Bl[n_no_meger_bi].PointHigh.fVal;

										//还有处理显示的数据
										//vecData[nStartk+2*k+1].pt1.x = vecData[n_no_meger_bi].pt1.x;
										//vecData[nStartk+2*k+1].pt1.y = vecData[n_no_meger_bi].pt1.y;

									}
									else
									{
										nRet = nStartk+2*k+1;
									}

									//Draw_QueDing_Xianduan_First_UP(BlxOut, nStartk, nRet);//画第一情况的笔

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

		// 没有找到拐点，也就是一条直线，
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
					//Draw_QueDing_Xianduan_First_UP(Bl, nStartk, nStartk+n);//画第一情况的笔
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
					//Draw_QueDing_Xianduan_First_UP(Bl, nStartk, nStartk+n);//画第一情况的笔
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
				//先找到一个特征序列的一个拐点，让判断是是不是满足条件1或者2
				if(BlxOut[nStartk+2*k+3].PointLow.fVal > Temp.PointLow.fVal)
				{

					bFlagGuaiDian = TRUE;
					//DrawGuaiDian(BlxOut,  nStartk,  k);//画拐点的笔

					//满足拐点的，说明已经出现了第一个或者第二个条件了
					//判断是第一个情况或者第二个情况
					//2*k+1的低点 < 2*k-1的高点,就是第一种情况，出现，这个时候不要找特征序列，而是判断后面的是不是能够形成一个线段
					//这里应该不要考虑2*k+1, 2*k-1的Meger特性了吧，还真需要考虑2*k-1的特性
					//int np = Get2k_dec_1(BlxOut, nStartk+2*k+1);
					int np = Get_GuaiDian_Before_Bi(BlxOut, nStartk+2*k+1, DOWN, nStartk);

					//DrawGuaiDian_Before_bi(BlxOut,  nStartk, np);//画拐点前的一笔
					//还有找到真正的拐点笔
					int nreal_gd = Get_GuaiDian_Real(BlxOut, nStartk+2*k+1, DOWN, nStartk);//拐点前的一笔

					if(np)
					{
						if(BlxOut[nreal_gd].PointHigh.fVal >= BlxOut[np].PointLow.fVal)//是否满足第一种情况
						{
							int nRet = 0;

							//画出线段之前---要先判断是否有meger,因为是左包含原则，所以要找出和nStartk+2*k+1 meger的最前面的一个笔
							//然后这个笔就是该线段最后一笔
							if(BlxOut[nStartk+2*k+1].nMeger != 0)
							{
								//确实证实了是meger
								int n_no_meger_bi = Get_No_Meger_Bi(BlxOut, nStartk+2*k+1);
								nRet = n_no_meger_bi;


								//还要处理包含的问题，就是处理Bl的问题
								Bl[nStartk+2*k+1].PointLow.nIndex = Bl[n_no_meger_bi].PointLow.nIndex;
								Bl[nStartk+2*k+1].PointLow.fVal = Bl[n_no_meger_bi].PointLow.fVal;

								//还有处理显示的数据
								//vecData[nStartk+2*k+1].pt1.x = vecData[n_no_meger_bi].pt1.x;
								//vecData[nStartk+2*k+1].pt1.y = vecData[n_no_meger_bi].pt1.y;

							}	
							else
							{
								nRet = nStartk+2*k+1;
							}

							Bl[nStartk+2*k+1].XianDuan_nprop = -1;

							//Draw_QueDing_Xianduan_First_DOWN(BlxOut, nStartk, nRet);//画第一情况的笔

							//确定是第一种情况的出现了，这个时候假设2*k+1的高点为线段的起点，找到向下的线段，再一次找到拐点就算是一个线段的完成				
							delete BlxOut;
							return nStartk+2*k+1;

						}
						else
						{
							//这个是第二种情况下，也就是一笔并没有破坏了。
							//假设2*k+1为起点，如果找到底分型的特征序列，就算是完成了一个线段的开始
							//Is_XianDuan_FenXing这个函数返回底分型最低的位置的k
							int npos = Is_XianDuan_FenXing(BlxOut, nStartk+2*k+1, nLen, UP);
							if(npos)
							{
								//Draw_Second_FengXing(Bl, npos);//画出第二种情况的分型

								//找到底分型了，然后看看这个底分型和拐点之间是否已经是一个线段
								//判断方法，拐点间到npos如果有笔高过拐点，那说这个拐点不是一个线段的点，需要用用高一点的点来继续
								int j = k+1;
								//for (int j = nStartk+2*k+1; j <= npos; j++)
								BOOL bFlagRet = FALSE;
								for (; nStartk+2*j < npos; j++)
								{
									//如果有点低于某个笔的点低于拐点的前一个点，也能构成
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
									//已经确定能形成线段了，需要做的就是k是否有meger的标志，找到没有标志的新k，然后该k就是要找的
									if(BlxOut[nStartk+2*k+1].nMeger)
									{
										while(1)
										{
											k--;

											if(BlxOut[nStartk+2*k+1].nMeger != -1)
											{
												k++;
												//Draw_QueDing_Xianduan_Second_DWON(BlxOut,  nStartk,  k);//画第二种线段的形成
												//也就是能形成线段
												Bl[nStartk+2*k+1].XianDuan_nprop = -1;
												delete BlxOut;
												return nStartk+2*k+1;
											}
										}
									}
									else
									{
										//Draw_QueDing_Xianduan_Second_DWON(Bl,  nStartk,  k);//画第二种线段的形成
										//也就是能形成线段
										Bl[nStartk+2*k+1].XianDuan_nprop = -1;
										delete BlxOut;
										return nStartk+2*k+1;
									}

								}
								else
								{
									//::MessageBoxA(m_hWnd, "找到分型也不能形成线段", NULL, MB_OK);
									Temp = BlxOut[nStartk+2*k+3];
								}

							}
							else
							{
								//::MessageBoxA(m_hWnd, "找不到第二种情况的分型", NULL, MB_OK);
								//到这里有可能说明是快结束了，应该要用结束的情况来分析
								//因为是向下的方向，所以直接找最低的笔，在看看是否能够成为一个
								//到这里已经有拐点出现了，说明拐点有可能形成一个线段，
								//有两种情况，
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
									//已经形成线段线段
									int nRet = 0;
									if(BlxOut[nStartk+2*k+1].nMeger != 0)
									{
										//确实证实了是meger
										int n_no_meger_bi = Get_No_Meger_Bi(BlxOut, nStartk+2*k+1);
										nRet = n_no_meger_bi;


										//还要处理包含的问题，就是处理Bl的问题
										Bl[nStartk+2*k+1].PointLow.nIndex = Bl[n_no_meger_bi].PointLow.nIndex;
										Bl[nStartk+2*k+1].PointLow.fVal = Bl[n_no_meger_bi].PointLow.fVal;

										//还有处理显示的数据
										//vecData[nStartk+2*k+1].pt1.x = vecData[n_no_meger_bi].pt1.x;
										//vecData[nStartk+2*k+1].pt1.y = vecData[n_no_meger_bi].pt1.y;

									}	
									else
									{
										nRet = nStartk+2*k+1;
									}

									Bl[nStartk+2*k+1].XianDuan_nprop = -1;

									//Draw_QueDing_Xianduan_First_DOWN(BlxOut, nStartk, nRet);//画第一情况的笔


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

		// 没有找到拐点，也就是一条直线，
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
					//Draw_QueDing_Xianduan_First_DOWN(Bl, nStartk, nStartk+n);//画第一情况的笔
					Bl[nStartk+n].XianDuan_nprop = -1;
				}
			}
		}
		else
		{
			//假设找到拐点了，可以直接画在拐点那里为止
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
					//Draw_QueDing_Xianduan_First_DOWN(Bl, nStartk, nStartk+n);//画第一情况的笔
					Bl[nStartk+n].XianDuan_nprop = -1;
				}
			}
		}
	}

	delete BlxOut;
	return 0;
}

//线段分析函数
void AnalyXD(Bi_Line *Bl, int nLen)
{
	//CClientDC dc(this);
	//CPen pen1;
	//pen1.CreatePen(PS_SOLID,2,RGB(0,0,255));
	//CPen *oldPen=dc.SelectObject(&pen1);
	//首先找出第一个三笔重叠的笔，用来代表第一个线段的起始
	int i = 0;
	for( i=0; i<nLen-2; i++ )
	{
		if(Bl[i].Bi_Direction == UP)
		{
			if( (Bl[i].PointLow.fVal < Bl[i+1].PointLow.fVal) && (Bl[i+2].PointHigh.fVal > Bl[i+1].PointHigh.fVal) )
			{
				Bl[i].XianDuan_nprop = -1;

				//char szContent[128] = {0};
				//sprintf(szContent, "找到开始点:%d low=%.2f high=%.2f", i, Bl[i].PointLow.fVal, Bl[i].PointHigh.fVal);

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
				//sprintf(szContent, "找到开始点:%d low=%.2f high=%.2f", i, Bl[i].PointLow.fVal, Bl[i].PointHigh.fVal);

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
		k = Lookup_Next_XianDuan(Bl, k, nLen );
		if(k == 0)
		{
			break;
		}
	}


}


void TestPlugin4(int DataLen,float* Out,float* High,float* Low, float* Close)
{
	OutputDebugStringA("[chs] TestPlugin4 ");
	if(DataLen != g_nSize)
	{
		return;
	}

	//到这里说明已经处理完笔，进入线段的处理系列了，
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
					//这个是向上的一笔
					Bl[nblCount].Bi_Direction = UP;   //向上的一笔
					Bl[nblCount].PointHigh.nIndex = i;
					Bl[nblCount].PointHigh.fVal = g_tgKs[i].high;
					Bl[nblCount].PointHigh.nprop = g_tgKs[i].prop;//笔的高低点

					Bl[nblCount].PointLow.nIndex = klTmp.index;
					Bl[nblCount].PointLow.fVal = klTmp.low;
					Bl[nblCount].PointLow.nprop = klTmp.prop; //笔的高低点
	
					Bl[nblCount].XianDuan_nprop = 0;
				}
				else
				{
					//这个是向下的一笔
					Bl[nblCount].Bi_Direction = DOWN;   //向上的一笔
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

	for(int i=0;i<DataLen;i++)
	{
		Out[i] = g_tgKs[i].prop;
	};

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
	OutputDebugStringA("[chs] TestPlugin3 ends");
}


/************************************************************************************************************************/
/************************************************************************************************************************/
/******************************************************中枢**************************************************************/
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


	//先判断，如果前几个4个没有重叠就不算了
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
			bGuaiDian = TRUE;//有拐点
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
			//没有拐点
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


void TestPlugin5(int DataLen,float* Out,float* High,float* Low, float* Close) 
{
	OutputDebugStringA("[chs] TestPlugin5");
	//首先先把线段转成一个数据结构
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
	
	
	//这个函数主要是把中枢的起点输出去，也就是=1=st
	for (int n = 0; n < nSize_xd_l; n++)
	{
		if(g_xd_l[n].XianDuan_nprop == 1)
		{
			//起点，找最左边的的
			if(g_xd_l[n].PointHigh.nIndex < g_xd_l[n].PointLow.nIndex)
			{
				//xIndex1 = xd1.PointHigh.nIndex;
				Out[g_xd_l[n].PointHigh.nIndex] = 1;

				char szBuff[128] = {0};
				sprintf(szBuff, "[chs] 中枢开始1：%d, val=%.2f", g_xd_l[n].PointHigh.nIndex, g_xd_l[n].PointHigh.fVal);
				OutputDebugStringA(szBuff);

			}
			else
			{
				//xIndex1 = xd1.PointLow.nIndex;
				Out[g_xd_l[n].PointLow.nIndex] = 1;

				char szBuff[128] = {0};
				sprintf(szBuff, "[chs] 中枢开始1：%d, val=%.2f", g_xd_l[n].PointLow.nIndex, g_xd_l[n].PointLow.fVal);
				OutputDebugStringA(szBuff);
			}
		}
	}
	

	OutputDebugStringA("[chs] TestPlugin5 ends");
}

void TestPlugin6(int DataLen,float* Out,float* High,float* Low, float* Close) 
{
	OutputDebugStringA("[chs] TestPlugin6");
	//这个函数主要是把中枢的起点输出去，也就是=2=end
	for (int n = 0; n < nSize_xd_l; n++)
	{
		if(g_xd_l[n].XianDuan_nprop == 2)
		{
			//起点，找最又边的的
			if(g_xd_l[n].PointHigh.nIndex > g_xd_l[n].PointLow.nIndex)
			{
				//xIndex1 = xd1.PointHigh.nIndex;
				Out[g_xd_l[n].PointHigh.nIndex] = 2;

				char szBuff[128] = {0};
				sprintf(szBuff, "[chs] 中枢结束2：%d, val=%.2f", g_xd_l[n].PointHigh.nIndex, g_xd_l[n].PointHigh.fVal);
				OutputDebugStringA(szBuff);
			}
			else
			{
				//xIndex1 = xd1.PointLow.nIndex;
				Out[g_xd_l[n].PointLow.nIndex] = 2;
				char szBuff[128] = {0};
				sprintf(szBuff, "[chs] 中枢结束2：%d, val=%.2f", g_xd_l[n].PointLow.nIndex, g_xd_l[n].PointLow.fVal);
				OutputDebugStringA(szBuff);
			}
		}
	}

}


void TestPlugin7(int DataLen,float* Out,float* High,float* Low, float* Close) 
{
	OutputDebugStringA("[chs] TestPlugin7");
	//这个是中枢的最高点
	for (int n = 0; n < nSize_xd_l; n++)
	{
		if(g_xd_l[n].XianDuan_nprop == 1)
		{
			//起点，找最左边的的
			if(g_xd_l[n].PointHigh.nIndex < g_xd_l[n].PointLow.nIndex)
			{
				//xIndex1 = xd1.PointHigh.nIndex;
				Out[g_xd_l[n].PointHigh.nIndex] = g_xd_l[n].fMax;

				char szBuff[128] = {0};
				sprintf(szBuff, "[chs] 中枢开始最大值1：%d, fMax=%.2f", g_xd_l[n].PointHigh.nIndex, g_xd_l[n].fMax);
				OutputDebugStringA(szBuff);
			}
			else
			{
				//xIndex1 = xd1.PointLow.nIndex;
				Out[g_xd_l[n].PointLow.nIndex] = g_xd_l[n].fMax;

				char szBuff[128] = {0};
				sprintf(szBuff, "[chs] 中枢开始最大值1：%d, fMax=%.2f", g_xd_l[n].PointLow.nIndex, g_xd_l[n].fMax);
				OutputDebugStringA(szBuff);
			}
		}

		if(g_xd_l[n].XianDuan_nprop == 2)
		{
			//起点，找最左边的的
			if(g_xd_l[n].PointHigh.nIndex > g_xd_l[n].PointLow.nIndex)
			{
				Out[g_xd_l[n].PointHigh.nIndex] = g_xd_l[n].fMax;

				char szBuff[128] = {0};
				sprintf(szBuff, "[chs] 中枢结束最大值2：%d, fMax=%.2f", g_xd_l[n].PointHigh.nIndex, g_xd_l[n].fMax);
				OutputDebugStringA(szBuff);
			}
			else
			{
				//xIndex1 = xd1.PointLow.nIndex;
				Out[g_xd_l[n].PointLow.nIndex] = g_xd_l[n].fMax;

				char szBuff[128] = {0};
				sprintf(szBuff, "[chs] 中枢结束最大值2：%d, fMax=%.2f", g_xd_l[n].PointLow.nIndex, g_xd_l[n].fMax);
				OutputDebugStringA(szBuff);
			}
		}
	}

}


void TestPlugin8(int DataLen,float* Out,float* High,float* Low, float* Close) 
{
	OutputDebugStringA("[chs] TestPlugin8");
	//这个是中枢的最低点
	for (int n = 0; n < nSize_xd_l; n++)
	{
		if(g_xd_l[n].XianDuan_nprop == 1)
		{
			//起点，找最左边的的
			if(g_xd_l[n].PointHigh.nIndex < g_xd_l[n].PointLow.nIndex)
			{
				//xIndex1 = xd1.PointHigh.nIndex;
				Out[g_xd_l[n].PointHigh.nIndex] = g_xd_l[n].fMin;

				char szBuff[128] = {0};
				sprintf(szBuff, "[chs] 中枢开始最大值1：%d, fMax=%.2f", g_xd_l[n].PointHigh.nIndex, g_xd_l[n].fMin);
				OutputDebugStringA(szBuff);
			}
			else
			{
				//xIndex1 = xd1.PointLow.nIndex;
				Out[g_xd_l[n].PointLow.nIndex] = g_xd_l[n].fMin;

				char szBuff[128] = {0};
				sprintf(szBuff, "[chs] 中枢开始最大值1：%d, fMax=%.2f", g_xd_l[n].PointLow.nIndex, g_xd_l[n].fMin);
				OutputDebugStringA(szBuff);
			}
		}

		if(g_xd_l[n].XianDuan_nprop == 2)
		{
			//起点，找最左边的的
			if(g_xd_l[n].PointHigh.nIndex > g_xd_l[n].PointLow.nIndex)
			{
				Out[g_xd_l[n].PointHigh.nIndex] = g_xd_l[n].fMin;

				char szBuff[128] = {0};
				sprintf(szBuff, "[chs] 中枢结束最大值2：%d, fMax=%.2f", g_xd_l[n].PointHigh.nIndex, g_xd_l[n].fMin);
				OutputDebugStringA(szBuff);
			}
			else
			{
				//xIndex1 = xd1.PointLow.nIndex;
				Out[g_xd_l[n].PointLow.nIndex] = g_xd_l[n].fMin;

				char szBuff[128] = {0};
				sprintf(szBuff, "[chs] 中枢结束最大值2：%d, fMax=%.2f", g_xd_l[n].PointLow.nIndex, g_xd_l[n].fMin);
				OutputDebugStringA(szBuff);
			}
		}
	}

	delete[] g_xd_l;
}



//加载的函数
PluginTCalcFuncInfo g_CalcFuncSets[] = 
{
	{1,(pPluginFUNC)&TestPlugin1},
	{2,(pPluginFUNC)&TestPlugin2},
	{3,(pPluginFUNC)&TestPlugin3},
	{4,(pPluginFUNC)&TestPlugin4},

	{5,(pPluginFUNC)&TestPlugin5},
	{6,(pPluginFUNC)&TestPlugin6},
	{7,(pPluginFUNC)&TestPlugin7},
	{8,(pPluginFUNC)&TestPlugin8},
	{0,NULL},
};
BOOL RegisterTdxFunc(PluginTCalcFuncInfo** pFun)
{
	if(*pFun==NULL)
	{
		(*pFun)=g_CalcFuncSets;
		return TRUE;
	}
	return FALSE;

}