#ifndef __TCALC_FUNC_SETS
#define __TCALC_FUNC_SETS


void TestPlugin3(int DataLen,float* High,float* Low);
void TestPlugin4(int DataLen,float* High,float* Low);
void TestPlugin5(int DataLen,float* High,float* Low);
BOOL ZhongShuAnalu_BeiLi();	//这个可以是1分钟也可以是5fen 
BOOL ZhongShuAnalu_BeiLi_5Min();//添加一个专门5分钟的数据扫描，

BOOL ZhongShuAnaly_YuCe();
BOOL ZhongShuAnaly_ThreeBuy();

#endif