#include "common.h"
#include "delay.h"

void delayms(unsigned int Ms)
{
  unsigned int i, TempCyc;
  _nop_();
  for (i = 0; i < Ms; i++)
  {
    TempCyc = 70;
    while (TempCyc--)
      ;
  }
}
