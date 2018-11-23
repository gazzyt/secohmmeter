//功能:用LCD1602显示 xingxiangrong
//编译环境: KEIL UVISION2
//单片机晶振：12M  单片机型号AT89S52
//单片机晶振: 无特殊要求
//作者：兴向荣电子元件店
//日期：2013.07.02
//:专利产品，严禁用于商业用途；

#include <reg52.h>
#include <intrins.h>

#define uchar unsigned char
#define uint  unsigned int


//==============LCD1602接口连接方法=====================
/*-----------------------------------------------------
       |DB0-----P0.0 | DB4-----P0.4 | RW-------P2.3    |
       |DB1-----P0.1 | DB5-----P0.5 | RS-------P2.4    |
       |DB2-----P0.2 | DB6-----P0.6 | E--------P2.2    |
       |DB3-----P0.3 | DB7-----P0.7 | 
    ---------------------------------------------------*/
//================================================*/              
#define LCM_Data     P0    //LCD1602数据接口
#define Busy         0x80   //用于检测LCM状态字中的Busy标识
sbit    LCM_RW     = P2^3;  //读写控制输入端，LCD1602的第五脚
sbit    LCM_RS     = P2^4;  //寄存器选择输入端，LCD1602的第四脚
sbit    LCM_E      = P2^2;  //使能信号输入端,LCD1602的第6脚

sbit    int0_int=   P3^2;
sbit    l_button =  P2^0;//电感测量控制开关；
sbit    c_button=P2^1;//电容测量控制开关；
sbit    f_button=P2^6;//频率测试控制开关；
sbit    min_elect_c_button=P2^7;//小电容测试控制开关
sbit    max_elect_c_button=P3^6;//小电容测试控制开关
sbit    fangdian_button=P2^5;//电解电容测量时放电控制脚

//**************函数声明***************************************
void    WriteDataLCM		(uchar WDLCM);//LCD模块写数据
void    WriteCommandLCM	(uchar WCLCM,BuysC); //LCD模块写指令
uchar   ReadStatusLCM(void);//读LCD模块的忙标
void    DisplayOneChar(uchar X,uchar Y,uchar ASCII);//在第X+1行的第Y+1位置显示一个字符
void    LCMInit(void);//LCD初始
void    delayms(uint ms);//1MS基准延时程序
void    DisplayListChar(uchar X,uchar Y,uchar delayms, uchar code *DData);
void   judge_xianshi(void);//显示处理程序
void   lx_display();
void   cx_display();
void   fx_display();
void   init_t0();

void    strive_f1();//求取F1
void    strive_f2();//求取电感的大小
void    strive_cx();//求取小电容（无极性电容）的大小
void    strive_fx();//测试外边频率
void    strive_min_c();//小电容测试
void    strive_max_c();//大电容测试
uchar t0_crycle=0;
uchar f_crycle;
uchar  flag1;//
uchar display_flag;
uint  f1,temp,f2;
long ryz;
//***********************主程序******************************
main()   
{   
   fangdian_button=1;
   LCMInit();
   init_t0();
   strive_f1();//求取F1
   DisplayListChar(0,0,0, "F/L/C Tester");
   while(1)
   {
     strive_f2();//求取F1
     strive_cx();
     strive_fx(); 
     strive_min_c();
     strive_max_c();
     judge_xianshi();
   }
}
void   judge_xianshi()
{
    lx_display();
    cx_display();
    fx_display();
}
void    strive_max_c()//大电解电容测试
{
   max_elect_c_button=1;//置为1，准备判断小电容测量开关的状态，上电时这个开关比较弹起
   if(max_elect_c_button==0)
   {  
        f_crycle=0;
        fangdian_button=0;
        delayms(250);
        fangdian_button=1;
        TMOD=0x10;//设定T0以工作方式1定时
        TH1=0;
        TL1=0;
        EA=1;
        ET1=1;//允许定时器0中断
        TR1=1;
        int0_int=1;
        while(int0_int==1);
        TR1=0;
        ryz=0;
        ryz= f_crycle*50000;
        ryz+=TH1*256+TL1;
        DisplayListChar(1,0,0, "Cx=");
        DisplayOneChar(1,3,  ryz/1000000%10+0x30);
        DisplayOneChar(1,4, ryz/100000%10+0x30);
        DisplayOneChar(1,5, ryz/10000%10+0x30);
        DisplayOneChar(1,6, ryz/1000%10+0x30);
        DisplayOneChar(1,7, ryz/100%10+0x30);
        DisplayOneChar(1,8,'.');
        DisplayOneChar(1,9, ryz/10%10+0x30);
        DisplayOneChar(1,10,ryz%10+0x30);
        DisplayListChar(1,11,0, "UF  ");
   }   
}
void    strive_min_c()//小电容测试
{
   min_elect_c_button=1;//置为1，准备判断小电容测量开关的状态，上电时这个开关比较弹起
   if(min_elect_c_button==0)
   {  
        f_crycle=0;
        fangdian_button=0;
        delayms(250);
        fangdian_button=1;
        TMOD=0x10;//设定T0以工作方式1定时
        TH1=0;
        TL1=0;
        EA=1;
	    ET1=1;//允许定时器0中断 
        TR1=1;
        display_flag=4;//显示标志，为4为测试小电解电容
        int0_int=1;
        while(int0_int==1);
        TR1=0;
        ryz=0;
        ryz+=50000*f_crycle;
        ryz+=TH1*256+TL1;
        ryz/=20;
        DisplayListChar(1,0,0, "Cx=");
        DisplayOneChar(1,3,  ryz/1000000%10+0x30);
        DisplayOneChar(1,4, ryz/100000%10+0x30);
        DisplayOneChar(1,5, ryz/10000%10+0x30);
        DisplayOneChar(1,6, ryz/1000%10+0x30);
        DisplayOneChar(1,7, ryz/100%10+0x30);
        DisplayOneChar(1,8,'.');
        DisplayOneChar(1,9, ryz/10%10+0x30);
        DisplayOneChar(1,10,ryz%10+0x30);
        DisplayListChar(1,11,0, "UF  ");
        display_flag=4;//显示标志，为4为测试小电解电容
   }   
}
void timer1() interrupt 3
{
	TH1=(65536-50000)/256;//定时50毫秒
	TL1=(65536-50000)%256;
    f_crycle++;
}
//求取小电容（无极性电容）的大小
void    strive_fx()
{
   uchar i;
   f_button=1;//置为1，准备判断小电容测量开关的状态，上电时这个开关比较弹起
   if(f_button==0)
   {
      if(display_flag!=3)init_t0();
      display_flag=3;//显示标志，为3为测试频率
      TR0=1;
      TR1=1;
      ryz=0;
      for(i=0;i<20;i++)
      {
        f_crycle=0;
        while(f_crycle<1);
        f_crycle=0;
        ryz+=temp;
      }
      TR0=0;
      TR1=0;
    }
}
void timer0() interrupt 1
{
	TH0=(65536-50000)/256;//定时50毫秒
	TL0=(65536-50000)%256;
      f_crycle++;
	  t0_crycle=0;
	  TR0=0;//关闭定时0
      temp=TH1*256+TL1;//
      TH1=0;
      TL1=0;
      TR0=1;
}
void fx_display()
{
   if(display_flag==3)
   {
     DisplayListChar(1,0,0, "Fx=");
     DisplayOneChar(1,3,  ryz/1000000%10+0x30);
     DisplayOneChar(1,4, ryz/100000%10+0x30);
     DisplayOneChar(1,5, ryz/10000%10+0x30);
     DisplayOneChar(1,6, ryz/1000%10+0x30);
     DisplayOneChar(1,7, ryz/100%10+0x30);
     DisplayOneChar(1,8, ryz/10%10+0x30);
     DisplayOneChar(1,9,ryz%10+0x30);
     DisplayListChar(1,10,0, "HZ    ");
   }
}
void cx_display()
{
   if(display_flag==2)
   {
     DisplayListChar(1,0,0, "Cx=");
     DisplayOneChar(1,3, ryz/1000000%10+0x30);
     DisplayOneChar(1,4, ryz/100000%10+0x30);
     DisplayOneChar(1,5, ryz/10000%10+0x30);
     DisplayOneChar(1,6, ryz/1000%10+0x30);
     DisplayOneChar(1,7, ryz/100%10+0x30);
     DisplayOneChar(1,8, ryz/10%10+0x30);
     DisplayOneChar(1,9, ryz%10+0x30);
     DisplayListChar(1,10,0, "pF    ");
   }
}
void   lx_display()
{
     if(display_flag==1)
     {
       DisplayListChar(1,0,0, "Lx=");
       DisplayOneChar(1,3,  ryz/10000000%10+0x30);
       DisplayOneChar(1,4,  ryz/1000000%10+0x30);
       DisplayOneChar(1,5, ryz/100000%10+0x30);
       DisplayOneChar(1,6, ryz/10000%10+0x30);
       DisplayOneChar(1,7, ryz/1000%10+0x30);
       DisplayOneChar(1,8, ryz/100%10+0x30);
       DisplayOneChar(1,9, ryz/10%10+0x30);
       DisplayOneChar(1,10,'.');
       DisplayOneChar(1,11,ryz%10+0x30);
       DisplayListChar(1,12,0, "uH  ");
     }
}
//********************************************************************************************
void init_t0()
{
    TMOD=0x51;//设定T0以工作方式1定时50毫秒,T1为计数器，工作方式1
	TH0=(65536-50000)/256;
	TL0=(65536-50000)%256;
	EA=1;//开总中断
	ET0=1;//允许定时器0中断
	t0_crycle=0;//定时器中断次数计数单元
    TH1=0;
    TL1=0;
}
//求取小电容（无极性电容）的大小
void    strive_cx()
{
     
   c_button=1;//置为1，准备判断小电容测量开关的状态，上电时这个开关比较弹起
   if(c_button==0)
   {
      if(display_flag!=2)init_t0();
      display_flag=2;//显示标志，为1为测试小电容
      TR0=1;
      TR1=1;
      f_crycle=0;
      while(f_crycle<=10);
      f_crycle=0;
      TR0=0;
      TR1=0;
      f2=temp;//
      if(f2>f1)f2=f1;
      if(f2<65)f2=65;
      if(f2!=0)
      {
        ryz=((unsigned long) f1)*((unsigned long )f1);
        ryz/=f2;
        ryz*=2000;//换算为PF
        ryz/=f2;
        ryz-=2000;
      }   
   }
}
//测电感
void   strive_f2()
{
   l_button=1;//置为1，准备判断电感测量开关的状态，上电时这个开关比较弹起
   if(l_button==0)
   {
      if(display_flag!=1)init_t0();
      display_flag=1;//显示标志，为1为测试电感
      TR0=1;
      TR1=1;
      f_crycle=0;
      while(f_crycle<=15);
      f_crycle=0;
      TR0=0;
      TR1=0;
      f2=temp;//
      if(f2>f1)f2=f1;
      if(f2<65)f2=65;
      if(f2!=0)
      {
        ryz=((unsigned long) f1)*((unsigned long )f1);
        ryz/=f2;
        ryz*=1000;//换算为UH
        ryz/=f2;
        ryz-=1000;
      }   
   }
}
//******************************************************
//上电的时候求取F1
void   strive_f1()
{
   uchar i;
   for(i=0;i<5;i++)
   {
      l_button=1;//置为1，准备判断电感测量开关的状态，上电时这个开关比较弹起
      while(l_button==0);
      TR0=1;
      TR1=1;
      while(f_crycle<=10);
      f_crycle=0;
      TR0=0;
      TR1=0;
      f1=temp;
   }
}
/*====================================================================  
  按指定位置显示一串字符:第 X 行,第 y列
  注意:字符串不能长于16个字符
======================================================================*/
void DisplayListChar(uchar X,uchar Y,uchar ms, uchar code *DData)
{
 unsigned char ListLength;

 ListLength = 0;

 X &= 0x1;
 Y &= 0xF; //限制X不能大于15，Y不能大于1
 while (DData[ListLength]!='\0') //若到达字串尾则退出
  { 
     if (Y <= 0xF) //X坐标应小于0xF
     {
        DisplayOneChar(X, Y, DData[ListLength]); //显示单个字符
        ListLength++;
        Y++;
	    delayms(ms);//延时显示字符串
     }
     else
	    break;//跳出循环体 
  }
}
/*======================================================================
 LCM初始化
======================================================================*/
void LCMInit(void) 
{
 LCM_Data = 0;
 WriteCommandLCM(0x38,0); //三次显示模式设置，不检测忙信号
 delayms(5);
 WriteCommandLCM(0x38,0);
 delayms(5);
 WriteCommandLCM(0x38,0);
 delayms(5);
 WriteCommandLCM(0x38,1); //显示模式设置,开始要求每次检测忙信号
 WriteCommandLCM(0x08,1); //关闭显示
 WriteCommandLCM(0x01,1); //显示清屏
 WriteCommandLCM(0x06,1); // 显示光标移动设置
 WriteCommandLCM(0x0C,1); // 显示开及光标设置
 delayms(100);
}
//==============================LCD1602显示子程序================================================
// 写数据函数: E =高脉冲 RS=1 RW=0
//======================================================================*/
void WriteDataLCM(uchar WDLCM)
{
 ReadStatusLCM(); //检测忙
 LCM_Data = WDLCM;
 LCM_RS = 1;
 LCM_RW = 0;
 LCM_E = 0; //若晶振速度太高可以在这后加小的延时
 LCM_E = 0; //延时
 LCM_E = 1;
}
/*====================================================================
  写指令函数: E=高脉冲 RS=0 RW=0
======================================================================*/
void WriteCommandLCM(uchar WCLCM,BuysC) //BuysC为0时忽略忙检测
{
 if (BuysC) ReadStatusLCM(); //根据需要检测忙
 LCM_Data = WCLCM;
 LCM_RS = 0;
 LCM_RW = 0;
 LCM_E = 0;
 LCM_E = 0;
 LCM_E = 1;
}
/*====================================================================
  正常读写操作之前必须检测LCD控制器状态:E=1 RS=0 RW=1;
  DB7: 0 LCD控制器空闲，1 LCD控制器忙。
  读状态
======================================================================*/
uchar ReadStatusLCM(void)
{
 LCM_Data = 0xFF;
 LCM_RS = 0;
 LCM_RW = 1;
 LCM_E = 0;
 LCM_E = 0;
 LCM_E = 1;
 while (LCM_Data & Busy); //检测忙信号  
 return(LCM_Data);
}
/*======================================================================
功 能:     在1602 指定位置显示一个字符:第一行位置0~15,第二行16~31
说 明:     第 X 行,第 y 列  注意:字符串不能长于16个字符
======================================================================*/
void DisplayOneChar( uchar X, uchar Y, uchar ASCII)
{
  X &= 0x1;
  Y &= 0xF; //限制Y不能大于15，X不能大于1
  if (X) Y |= 0x40; //当要显示第二行时地址码+0x40;
  Y |= 0x80; // 算出指令码
  WriteCommandLCM(Y, 0); //这里不检测忙信号，发送地址码
  WriteDataLCM(ASCII);
}
/*====================================================================
  设定延时时间:x*1ms
====================================================================*/
void delayms(uint Ms)
{
  uint i,TempCyc;
  _nop_();
  for(i=0;i<Ms;i++)
  {
    TempCyc =70;
    while(TempCyc--);
  }
}
