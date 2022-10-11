#include <stdbool.h>

#include "stm32f1xx.h"

#include "initDevice.h"


// Прототипы используемых функций
void delayMs(int ms);
void delayCycles(uint32_t cycles);
void mainLoop();
void blink();
void setDebugLed(int n, bool on);

// Содержимое памяти
#define START_MEM_ADDR 0x8000 // Начальный адрес эмуляции ПЗУ в ПЭВМ Микроша
#define MEM_LEN 5

uint8_t mem[MEM_LEN]={0x55, 0x00, 0xFF, 0x01, 0x20};


// Чтение адреса с адресной шины Микроши
// Описывается первой чтобы небыло необходимости прописывать прототип,
// т. к. функция должна обязательно инлайниться
__attribute__((always_inline, section(".ramfunc")))
uint16_t readAddressBus()
{
    // Для ускорения адрес вначале считается 32-х битным чтобы проще работать
    // с 32-х битным регистром PA, и только в конце он один раз преобразуется в 16 бит
    uint32_t addr;

    // Установка на мультиплексоре сегмента адреса 00
    GPIOB->BSRR = GPIO_BSRR_BR3_Msk | GPIO_BSRR_BR4_Msk;

    // Считывается и запоминается значение ножек сегмента 0
    addr = ((GPIOA->IDR & 0x0F00) >> 8);


    // Установка на мультиплексоре сегмента адреса 01
    GPIOB->BSRR = GPIO_BSRR_BS3_Msk | GPIO_BSRR_BR4_Msk;

    // Считывается и запоминается значение ножек сегмента 1
    addr = addr | ((GPIOA->IDR & 0x0F00) >> 4);

    // Временно считывается 8 бит адреса

    // Установка на мультиплексоре сегмента адреса 10
    // GPIOB->BSRR = GPIO_BSRR_BR3_Msk | GPIO_BSRR_BS4_Msk;

    // Считывается и запоминается значение ножек сегмента 2
    // addr = addr | (GPIOA->IDR & 0x0F00);


    // Установка на мультиплексоре сегмента адреса 11
    // GPIOB->BSRR = GPIO_BSRR_BS3_Msk | GPIO_BSRR_BS4_Msk;

    // Считывается и запоминается значение ножек сегмента 3
    // addr = addr | ((GPIOA->IDR & 0x0F00) << 4);

    // Так как временно читается только 8 бит адреса,
    // к адресу добавляется базовый адрес
    return ((uint16_t) addr)+START_MEM_ADDR;
}


int main(void)
{
    // Начальные инициализации оборудования STM32 для работы с шинами Микроши
    clockInit();
    portClockInit();
    disableJtag();
    disableGlobalInterrupt();
    addressBusInit();
    dataBusInit();
    systemPinsInit();
    debugLedInit();
    
    // Задержка, чтобы Микроша успела нормально включиться
    delayMs(500);

    // mainLoop();
    blink();

    // setDebugLed(0, 1);
    // setDebugLed(1, 0);

    return 0;
}


__attribute__((noinline, section(".ramfunc")))
void mainLoop()
{
    // Флаг слежения за активностью шины данных
    // Если false - шина данных неактивна, и находится в высокоимпендансном состоянии
    // Если true - шина данных активна, на пинах выставлены какие-то данные
    bool dataBusActive=false;

    uint16_t addr=0;

    while (true) 
    {
        // Проверка системных сигналов /32К и /RD
        // Если они неактивны, сигнал EZ на шинном формирователе должны быть 1
        // чтобы не влиять на ШД и никаких действий происходить не должно
        // uint32_t workAddrDiapason = GPIOB->IDR & (GPIO_IDR_IDR6_Msk | GPIO_IDR_IDR7_Msk);

        // Если пришел сигнал /32К значит надо высчитывать адрес
        if( (GPIOB->IDR & (GPIO_IDR_IDR6_Msk )) == 0 )
        {
            addr=readAddressBus();
        }   

        
        // Если оба сигнала /32К и /RD физически не установлены в 0
        if( (GPIOB->IDR & (GPIO_IDR_IDR6_Msk | GPIO_IDR_IDR7_Msk)) != 0) 
        {
            // Надо проследить чтобы шинный формирователь был закрыт
            // чтобы никак не влиять на ШД компьютера и больше ничего не делать
            if(dataBusActive==true)
            {
                GPIOB->BSRR = (1<<GPIO_BSRR_BS0_Pos); // EZ=1 (передача выключена)

                dataBusActive=false;
            }

            continue;
        }
        else // Иначе /32К и /RD активны (оба в физ. нуле)
        {
            uint8_t byte=0x00; // Значение байта по-умолчанию

            // Если адрес в диапазоне эмуляции ПЗУ,
            // на шине данных выставляется нужный байт
            if( addr>=START_MEM_ADDR && addr<(START_MEM_ADDR+MEM_LEN) )
            {
                // Байт, который будет выдан на ШД
                byte=mem[addr-START_MEM_ADDR];
            }

            // Установка байта на ШД (в биты 8-15 порта B)
            GPIOB->BSRR = 0xFF000000 | (((uint16_t) byte) << 8);

            // Проталкивание байта сквозь шинный преобразователь
            // Если ШД неактивна
            if(dataBusActive==false)
            {
                // ШД должна стать активной
                GPIOB->BSRR = (1<<GPIO_BSRR_BR0_Pos); // EZ=0 (передача включается)
                
                dataBusActive=true;
            }                
                
        }

    }
}

 
// Функция не используется, оставлена для примера
// __attribute__((noinline, section(".ramfunc")))
// void msDelay(int ms)
// {
//    while (ms-- > 0) {
//       volatile int x=500;
//       while (x-- > 0)
//          __asm("nop");
//    }
// }


// Функция задержки в микросекундах, размещается в ОЗУ
__attribute__((noinline, section(".ramfunc")))
void delayMs(int ms)
{
  uint32_t cycles = ms * F_CPU / 9 / 1000;

  __asm volatile (
    "1: subs %[cycles], %[cycles], #1 \n"
    "   bne 1b \n"
    : [cycles] "+r"(cycles)
  );
}


// Функция задержки в машинных тактах, размещается в ОЗУ
__attribute__((noinline, section(".ramfunc")))
void delayCycles(uint32_t cycles)
{
  cycles /= 4;

  __asm volatile (
    "1: subs %[cycles], %[cycles], #1 \n"
    "   nop \n"
    "   bne 1b \n"
    : [cycles] "+l"(cycles)
  );
}


__attribute__((noinline, section(".ramfunc")))
void blink()
{
    while (true) 
    {
        delayMs(500);
        setDebugLed(0,0); // C13
        setDebugLed(1,1); // A0
        
        // Проверка B3, B4, A15
        GPIOB->BSRR = GPIO_BSRR_BR3_Msk | GPIO_BSRR_BR4_Msk;
        GPIOA->BSRR = GPIO_BSRR_BR15_Msk;

        delayMs(500);
        setDebugLed(0,1); // C13
        setDebugLed(1,0); // A0
        
        // Проверка B3, B4, A15
        GPIOB->BSRR = GPIO_BSRR_BS3_Msk | GPIO_BSRR_BS4_Msk;
        GPIOA->BSRR = GPIO_BSRR_BS15_Msk;
    } 
}


// Выставление отладочных светодиодов
__attribute__((always_inline, section(".ramfunc")))
void setDebugLed(int n, bool on)
{
    // Всего 3 светодиода
    // 0 - C13
    // 1 - A0
    // 2 - A2 (пока не сделан)
    
    // C13, подключен на (+), поэтому 0 -светится, 1 - выключен
    if(n==0)
    {
        if(on)
        {
            GPIOC->BSRR = (1<<GPIO_BSRR_BR13_Pos);
        }
        else
        {
            GPIOC->BSRR = (1<<GPIO_BSRR_BS13_Pos);
        }

    }

    // A0, подключен по-обычному
    if(n==1)
    {
        if(on)
        {
            GPIOA->BSRR = (1<<GPIO_BSRR_BS0_Pos); 
        }
        else
        {
            GPIOA->BSRR = (1<<GPIO_BSRR_BR0_Pos); 
        }
    }

    return;
}
