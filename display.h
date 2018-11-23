//功能:用LCD1602显示 xingxiangrong
//编译环境: KEIL UVISION2
//单片机晶振：12M  单片机型号AT89S52
//单片机晶振: 无特殊要求
//作者：兴向荣电子元件店
//日期：2013.07.02
//:专利产品，严禁用于商业用途；

#include "common.h"


//**************函数声明***************************************
void    WriteDataLCM		(uchar WDLCM);//LCD模块写数据
void    WriteCommandLCM	(uchar WCLCM, int BuysC); //LCD模块写指令
uchar   ReadStatusLCM(void);//读LCD模块的忙标
void    DisplayOneChar(uchar X,uchar Y,uchar ASCII);//在第X+1行的第Y+1位置显示一个字符
void    LCMInit(void);//LCD初始
void    delayms(uint ms);//1MS基准延时程序
void    DisplayListChar(uchar X,uchar Y,uchar delayms, __code const uchar *DData);
void ClearLine(uchar row);
void ClearScreen();
