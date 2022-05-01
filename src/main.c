#include <stdbool.h>

#include "stm32f1xx.h"


// Настраиваем тактирование системы от внешнего кварца
// через PLL на максимально возможных частотах.
// Внешний кварц должен быть на 8МГц
// Возвращает:
//  0 - завершено успешно
//  1 - не запустился кварцевый генератор
//  2 - не запустился PLL
// Настройка делается на 72МГц
int ClockInit(void)
{
  __IO int StartUpCounter;
  
  ////////////////////////////////////////////////////////////
  //Запускаем кварцевый генератор
  ////////////////////////////////////////////////////////////
  
  RCC->CR |= (1<<RCC_CR_HSEON_Pos); //Запускаем генератор HSE
  
  //Ждем успешного запуска или окончания тайм-аута
  for(StartUpCounter=0; ; StartUpCounter++)
  {
    //Если успешно запустилось, то 
    //выходим из цикла
    if(RCC->CR & (1<<RCC_CR_HSERDY_Pos))
      break;
    
    //Если не запустилось, то
    //отключаем все, что включили
    //и возвращаем ошибку
    if(StartUpCounter > 0x1000)
    {
      RCC->CR &= ~(1<<RCC_CR_HSEON_Pos); //Останавливаем HSE
      return 1;
    }
  }
  
  ////////////////////////////////////////////////////////////
  //Настраиваем и запускаем PLL
  ////////////////////////////////////////////////////////////
  
  //Настраиваем PLL
  RCC->CFGR |= (0x07<<RCC_CFGR_PLLMULL_Pos) //PLL множитель равен 9
            | (0x01<<RCC_CFGR_PLLSRC_Pos); //Тактирование PLL от HSE
  
  
  RCC->CR |= (1<<RCC_CR_PLLON_Pos); //Запускаем PLL
  
  //Ждем успешного запуска или окончания тайм-аута
  for(StartUpCounter=0; ; StartUpCounter++)
  {
    //Если успешно запустилось, то 
    //выходим из цикла
    if(RCC->CR & (1<<RCC_CR_PLLRDY_Pos))
      break;
    
    //Если по каким-то причинам не запустился PLL, то
    //отключаем все, что включили
    //и возвращаем ошибку
    if(StartUpCounter > 0x1000)
    {
      RCC->CR &= ~(1<<RCC_CR_HSEON_Pos); //Останавливаем HSE
      RCC->CR &= ~(1<<RCC_CR_PLLON_Pos); //Останавливаем PLL
      return 2;
    }
  }
  
  ////////////////////////////////////////////////////////////
  //Настраиваем FLASH и делители
  ////////////////////////////////////////////////////////////
  
  //Устанавливаем 2 цикла ожидания для Flash
  //так как частота ядра у нас будет 48 MHz < SYSCLK <= 72 MHz
  FLASH->ACR |= (0x02<<FLASH_ACR_LATENCY_Pos); 
  
  //Делители
  RCC->CFGR |= (0x00<<RCC_CFGR_PPRE2_Pos) //Делитель шины APB2 отключен
            | (0x04<<RCC_CFGR_PPRE1_Pos) //Делитель нишы APB1 равен 2
            | (0x00<<RCC_CFGR_HPRE_Pos); //Делитель AHB отключен
  
  
  RCC->CFGR |= (0x02<<RCC_CFGR_SW_Pos); //Переключаемся на работу от PLL
  
  //Ждем, пока переключимся
  while((RCC->CFGR & RCC_CFGR_SWS_Msk) != (0x02<<RCC_CFGR_SWS_Pos))
  {
  }
  
  //После того, как переключились на
  //внешний источник такирования
  //отключаем внутренний RC-генератор
  //для экономии энергии
  RCC->CR &= ~(1<<RCC_CR_HSION_Pos);
  
  //Настройка и переклбючение сисемы
  //на внешний кварцевый генератор
  //и PLL запершилось успехом.
  //Выходим
  return 0;
}


void msDelay(int ms)
{
   while (ms-- > 0) {
      volatile int x=500;
      while (x-- > 0)
         __asm("nop");
   }
}


//Alternates blue and green LEDs quickly
int main(void)
{
    ClockInit();

    // Включаем тактирование порта GPIOB
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;

    // Для начала сброс конфигурации порта B12 в ноль
    GPIOB->CRH &= ~(GPIO_CRH_MODE12 | GPIO_CRH_CNF12);

    // Конфигурация пина PB12
    // MODE: выход с максимальной частотой 50 МГц
    // CNF: режим push-pull
    GPIOB->CRH |= (0b11 << GPIO_CRH_MODE12_Pos) | (0x00 << GPIO_CRH_CNF12_Pos);


    while (true) {
        // msDelay(500);

        // Медленное переключение
        // 1мкс+1мкс = 2мкс - один период - на обычной частоте (неизвестно какой, которая стандартная)
        // GPIOB->ODR ^= (1<<LED1);

        // Более быстрое переключение
        // 250нс+250нс = 500нс - один период - на обычной частоте (неизвестно какой, которая стандартная)
        // 30нс+30нс = 60нс - один период - на частоте 72МГц
        GPIOB->BSRR = (1<<LED1); // Hi
        GPIOB->BRR = (1<<LED1); // Low
    }

    return 0;
}
