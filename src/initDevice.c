#include <stdbool.h>
#include "stm32f1xx.h"

#include "initDevice.h"


// Настройка тактирования системы от внешнего кварца
// через PLL на максимально возможных частотах.
// Внешний кварц должен быть на 8МГц
// Возвращает:
//  0 - завершено успешно
//  1 - не запустился кварцевый генератор
//  2 - не запустился PLL
// Настройка делается на 72МГц
int clockInit(void)
{
  __IO int startUpCounter;
  
  ////////////////////////////////////////////////////////////
  //Запускаем кварцевый генератор
  ////////////////////////////////////////////////////////////
  
  RCC->CR |= (1<<RCC_CR_HSEON_Pos); //Запускаем генератор HSE
  
  //Ждем успешного запуска или окончания тайм-аута
  for(startUpCounter=0; ; startUpCounter++)
  {
    //Если успешно запустилось, то 
    //выходим из цикла
    if(RCC->CR & (1<<RCC_CR_HSERDY_Pos))
      break;
    
    //Если не запустилось, то
    //отключаем все, что включили
    //и возвращаем ошибку
    if(startUpCounter > 0x1000)
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
  for(startUpCounter=0; ; startUpCounter++)
  {
    //Если успешно запустилось, то 
    //выходим из цикла
    if(RCC->CR & (1<<RCC_CR_PLLRDY_Pos))
      break;
    
    //Если по каким-то причинам не запустился PLL, то
    //отключаем все, что включили
    //и возвращаем ошибку
    if(startUpCounter > 0x1000)
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


// Включение тактирования портов
void portClockInit(void)
{
    // Тактирование портов GPIOA и GPIOB
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
}


// Настройка пинов адресной шины для получения адреса с Микроши
void addressBusInit(void)
{
    // Захват с 16-битной шины адреса происходит через 6 пинов
    // через мультиплексор на двух микросхемах К533КП2
    // Выбор сегмента адреса: A0 - PA8, A1 - PA9
    // Адресная линия: A0 - PB3, A1 - PB4, A2 - PB6, A3 - PB7


    // Для начала сброс конфигурации всех портов в ноль
    GPIOA->CRH &= ~(GPIO_CRH_MODE8 | GPIO_CRH_CNF8);
    GPIOA->CRH &= ~(GPIO_CRH_MODE9 | GPIO_CRH_CNF9);

    GPIOB->CRL &= ~(GPIO_CRL_MODE3 | GPIO_CRL_CNF3);
    GPIOB->CRL &= ~(GPIO_CRL_MODE4 | GPIO_CRL_CNF4);
    GPIOB->CRL &= ~(GPIO_CRL_MODE6 | GPIO_CRL_CNF6);
    GPIOB->CRL &= ~(GPIO_CRL_MODE7 | GPIO_CRL_CNF7);


    uint32_t mode;
    uint32_t cnf;

    // Выбор банка адреса всегда работает как выход
    mode=0b11; // Режим выхода, с максимальной частотой 50 МГц
    cnf=0b00;  // Режим push-pull
    GPIOA->CRH |= (mode << GPIO_CRH_MODE8_Pos) | (cnf << GPIO_CRH_CNF8_Pos);
    GPIOA->CRH |= (mode << GPIO_CRH_MODE9_Pos) | (cnf << GPIO_CRH_CNF9_Pos);


    // Адресные линии всегда работают как вход
    mode=0b00; // Режим входа
    cnf=0b01;  // Режим плавающего входа, подтяжки нет
    GPIOB->CRL |= (mode << GPIO_CRL_MODE3_Pos) | (cnf << GPIO_CRL_CNF3_Pos);
    GPIOB->CRL |= (mode << GPIO_CRL_MODE4_Pos) | (cnf << GPIO_CRL_CNF4_Pos);
    GPIOB->CRL |= (mode << GPIO_CRL_MODE6_Pos) | (cnf << GPIO_CRL_CNF6_Pos);
    GPIOB->CRL |= (mode << GPIO_CRL_MODE7_Pos) | (cnf << GPIO_CRL_CNF7_Pos);
}


// Первичная настройка шины данных
void dataBusInit(void)
{
    // Шина данных находится на пинах PB8-PB15

    // Для начала сброс конфигурации всех 8 пинов в ноль
    GPIOB->CRH &= ~(GPIO_CRH_MODE8  | GPIO_CRH_CNF8);
    GPIOB->CRH &= ~(GPIO_CRH_MODE9  | GPIO_CRH_CNF9);
    GPIOB->CRH &= ~(GPIO_CRH_MODE10 | GPIO_CRH_CNF10);
    GPIOB->CRH &= ~(GPIO_CRH_MODE11 | GPIO_CRH_CNF11);
    GPIOB->CRH &= ~(GPIO_CRH_MODE12 | GPIO_CRH_CNF12);
    GPIOB->CRH &= ~(GPIO_CRH_MODE13 | GPIO_CRH_CNF13);
    GPIOB->CRH &= ~(GPIO_CRH_MODE14 | GPIO_CRH_CNF14);
    GPIOB->CRH &= ~(GPIO_CRH_MODE15 | GPIO_CRH_CNF15);

    // Шина данных по-умолчанию и большую часть времени должна находиться
    // в режиме входа чтобы никак не влиять на ШД компьютера

    const uint32_t mode=0b00; // Режим входа
    const uint32_t cnf=0b10;  // Вход с подтягиванием
    GPIOB->CRH |= (mode << GPIO_CRH_MODE8_Pos)  | (cnf << GPIO_CRH_CNF8_Pos);
    GPIOB->CRH |= (mode << GPIO_CRH_MODE9_Pos)  | (cnf << GPIO_CRH_CNF9_Pos);
    GPIOB->CRH |= (mode << GPIO_CRH_MODE10_Pos) | (cnf << GPIO_CRH_CNF10_Pos);
    GPIOB->CRH |= (mode << GPIO_CRH_MODE11_Pos) | (cnf << GPIO_CRH_CNF11_Pos);
    GPIOB->CRH |= (mode << GPIO_CRH_MODE12_Pos) | (cnf << GPIO_CRH_CNF12_Pos);
    GPIOB->CRH |= (mode << GPIO_CRH_MODE13_Pos) | (cnf << GPIO_CRH_CNF13_Pos);
    GPIOB->CRH |= (mode << GPIO_CRH_MODE14_Pos) | (cnf << GPIO_CRH_CNF14_Pos);
    GPIOB->CRH |= (mode << GPIO_CRH_MODE15_Pos) | (cnf << GPIO_CRH_CNF15_Pos);

    // Направление подтяжки - подтягивание к общему проводу
    GPIOB->BRR = GPIO_ODR_ODR8_Msk |
                 GPIO_ODR_ODR9_Msk |
                 GPIO_ODR_ODR10_Msk |
                 GPIO_ODR_ODR11_Msk |
                 GPIO_ODR_ODR12_Msk |
                 GPIO_ODR_ODR13_Msk |
                 GPIO_ODR_ODR14_Msk |
                 GPIO_ODR_ODR15_Msk;
}


// Настройка пинов получения системных сигналов с Микроши
void systemPinsInit(void)
{
    // Пины захвата системных сигналов Микроши
    // /32К - PA10
    // /RD  - PA11

    // Для начала сброс конфигурации обеих пинов в ноль
    GPIOA->CRH &= ~(GPIO_CRH_MODE10 | GPIO_CRH_CNF10);
    GPIOA->CRH &= ~(GPIO_CRH_MODE11 | GPIO_CRH_CNF11);

    // Пины работают на чтение как обычные входы
    const uint32_t mode=0b00; // Режим входа
    const uint32_t cnf=0b10;  // 10 - вход с подтягиванием, 01 - Плавающий вход, подтяжки нет
    GPIOA->CRH |= (mode << GPIO_CRH_MODE10_Pos)  | (cnf << GPIO_CRH_CNF10_Pos);
    GPIOA->CRH |= (mode << GPIO_CRH_MODE11_Pos)  | (cnf << GPIO_CRH_CNF11_Pos);

    // Установка подтяжки к общему проводу (Pull-Down)
    GPIOA->BSRR = (1<<GPIO_BSRR_BR10_Pos);
    GPIOA->BSRR = (1<<GPIO_BSRR_BR11_Pos);
}


// Отладочный светодиод на A0
void debugLedInit(void)
{
    // Для начала сброс конфигурации пина в ноль
    GPIOA->CRL &= ~(GPIO_CRL_MODE0 | GPIO_CRL_CNF0);

    const uint32_t mode=0b11; // Режим выхода, с максимальной частотой 50 МГц
    const uint32_t cnf=0b00;  // 00 - Двухтактный выход (Output push-pull)
    GPIOA->CRL |= (mode << GPIO_CRL_MODE0_Pos)  | (cnf << GPIO_CRL_CNF0_Pos);
}
   
