#include <stdbool.h>

#include "stm32f1xx.h"

#include "initDevice.h"


// Прототипы используемых функций
uint16_t readAddressBus(); // Чтение адреса с ША Микроши
void delayMs(int ms);
void delayCycles(uint32_t cycles);
void mainLoop();
void blink();
void setDebugLed(int n, bool on);

// Содержимое памяти
#define START_MEM_ADDR 0x8000
#define MEM_LEN 5
uint8_t mem[MEM_LEN]={0x55, 0x00, 0xFF, 0x01, 0x20};


int main(void)
{
    // Начальные инициализации оборудования STM32 для работы с шинами Микроши
    clockInit();
    portClockInit();
    disableGlobalInterrupt();
    addressBusInit();
    dataBusInit();
    systemPinsInit();
    debugLedInit();

    // Задержка, чтобы Микроша успела нормально включиться
    delayMs(500);

    mainLoop();
    // blink();

    // setDebugLed(0, 1);
    // setDebugLed(1, 0);

    return 0;
}

__attribute__((noinline, section(".ramfunc")))
void blink()
{
    while (true) 
    {
        delayMs(500);
        GPIOA->BSRR = (1<<GPIO_BSRR_BS0_Pos);  // Hi A0
        GPIOC->BSRR = (1<<GPIO_BSRR_BS13_Pos); // Hi C13

        delayMs(500);
        GPIOA->BRR = (1<<GPIO_BSRR_BS0_Pos);  // Low A0
        GPIOC->BRR = (1<<GPIO_BSRR_BS13_Pos); // Low C13
    } 
}

__attribute__((noinline, section(".ramfunc")))
void mainLoop()
{
    // Флаг слежения за активностью шины данных
    // Если false - шина данных неактивна, и находится в высокоимпендансном состоянии
    // Если true - шина данных активна, на пинах выставлены какие-то данные
    bool dataBusActive=false;

    int status=0;

    while (true) 
    {
        // Проверка системных сигналов /32К и /RD
        // Если они неактивны, сигнал EZ на шинном формирователе должны быть 1
        // чтобы не влиять на ШД и никаких действий происходить не должно
        uint32_t workAddrDiapason = GPIOA->IDR & (GPIO_IDR_IDR10_Msk | GPIO_IDR_IDR11_Msk);
        
        // Если оба сигнала /32К и /RD физически не установлены в 0
        if(workAddrDiapason!=0) 
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
            // Нужно получить текущий адрес с ША
            // uint16_t addr=readAddressBus();

            // Если адрес в диапазоне эмуляции ПЗУ,
            // на шине данных выставляется нужный байт
            // if( addr>=START_MEM_ADDR && addr<(START_MEM_ADDR+MEM_LEN) )
            // if( true )
            {
                // Байт, который будет выдан на ШД
                uint8_t byte=0x77; // mem[addr-START_MEM_ADDR];

                // Текущие состояния всех пинов порта B
                uint16_t allPins=GPIOB->IDR;

                // Остаются состояния не-data пинов, а data-пины обнуляются
                allPins = allPins & 0x0000FFFF;

                // Значение байта данных смещается в область data-пинов (в биты 8-15)
                uint16_t dataPins=((uint16_t)byte) << 8;

                // Биты данных подмешиваются в значения всех пинов
                allPins = allPins | dataPins;

                // Биты данных выставляются на порту
                GPIOB->ODR=allPins;

                // Проталкивание байта сквозь шинный преобразователь
                // Если ШД неактивна
                if(dataBusActive==false)
                {
                    // ШД должна стать активной
                    GPIOB->BSRR = (1<<GPIO_BSRR_BR0_Pos); // EZ=0 (передача включается)
                    
                    dataBusActive=true;

                    status++;

                    // setDebugLed(0, 1);
                }                
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


// Чтение адреса с адресной шины Микроши
__attribute__((noinline, section(".ramfunc")))
uint16_t readAddressBus()
{
    uint16_t addrOctet0 = 0;
    uint16_t addrOctet1 = 0;
    uint16_t addrOctet2 = 0;
    uint16_t addrOctet3 = 0;

    uint32_t currentPins;

    // Установка на мультиплексоре сегмента адреса 00
    GPIOA->BSRR = 0 | (GPIO_BSRR_BR8_Msk | GPIO_BSRR_BR9_Msk );

    // Считывается и запоминается значение ножек сегмента 0
    currentPins = GPIOB->IDR;
    addrOctet0 = (uint16_t) (
                 (  (currentPins & GPIO_IDR_IDR3_Msk) >> GPIO_IDR_IDR3_Pos      ) + 
                 ( ((currentPins & GPIO_IDR_IDR4_Msk) >> GPIO_IDR_IDR4_Pos) << 1) +
                 ( ((currentPins & GPIO_IDR_IDR6_Msk) >> GPIO_IDR_IDR6_Pos) << 2) +
                 ( ((currentPins & GPIO_IDR_IDR7_Msk) >> GPIO_IDR_IDR7_Pos) << 3) );


    // Установка на мультиплексоре сегмента адреса 01
    GPIOA->BSRR = 0 | (GPIO_BSRR_BS8_Msk | GPIO_BSRR_BR9_Msk );

    // Считывается и запоминается значение ножек сегмента 1
    currentPins = GPIOB->IDR;
    addrOctet1 = (uint16_t) (
                 (  (currentPins & GPIO_IDR_IDR3_Msk) >> GPIO_IDR_IDR3_Pos      ) + 
                 ( ((currentPins & GPIO_IDR_IDR4_Msk) >> GPIO_IDR_IDR4_Pos) << 1) +
                 ( ((currentPins & GPIO_IDR_IDR6_Msk) >> GPIO_IDR_IDR6_Pos) << 2) +
                 ( ((currentPins & GPIO_IDR_IDR7_Msk) >> GPIO_IDR_IDR7_Pos) << 3) );


    // Установка на мультиплексоре сегмента адреса 10
    GPIOA->BSRR = 0 | (GPIO_BSRR_BR8_Msk | GPIO_BSRR_BS9_Msk );

    // Считывается и запоминается значение ножек сегмента 2
    currentPins = GPIOB->IDR;
    addrOctet2 = (uint16_t) (
                 (  (currentPins & GPIO_IDR_IDR3_Msk) >> GPIO_IDR_IDR3_Pos      ) + 
                 ( ((currentPins & GPIO_IDR_IDR4_Msk) >> GPIO_IDR_IDR4_Pos) << 1) +
                 ( ((currentPins & GPIO_IDR_IDR6_Msk) >> GPIO_IDR_IDR6_Pos) << 2) +
                 ( ((currentPins & GPIO_IDR_IDR7_Msk) >> GPIO_IDR_IDR7_Pos) << 3) );


    // Установка на мультиплексоре сегмента адреса 11
    GPIOA->BSRR = 0 | (GPIO_BSRR_BS8_Msk | GPIO_BSRR_BS9_Msk );

    // Считывается и запоминается значение ножек сегмента 3
    currentPins = GPIOB->IDR;
    addrOctet3 = (uint16_t) (
                 (  (currentPins & GPIO_IDR_IDR3_Msk) >> GPIO_IDR_IDR3_Pos      ) + 
                 ( ((currentPins & GPIO_IDR_IDR4_Msk) >> GPIO_IDR_IDR4_Pos) << 1) +
                 ( ((currentPins & GPIO_IDR_IDR6_Msk) >> GPIO_IDR_IDR6_Pos) << 2) +
                 ( ((currentPins & GPIO_IDR_IDR7_Msk) >> GPIO_IDR_IDR7_Pos) << 3) );

    return addrOctet0 + (addrOctet1 << 4) + (addrOctet2 << 8) + (addrOctet3 << 12);
}


// Чтение адреса с адресной шины Микроши
__attribute__((noinline, section(".ramfunc")))
void setDebugLed(int n, bool on)
{
    // Всего 3 светодиода
    // 0 - C13
    // 1 - A0
    // 2 - A2 (пока не сделан)
    
    // C13
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
