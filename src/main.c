#include <stdbool.h>

#include "stm32f1xx.h"

#include "initDevice.h"


// Прототипы используемых функций
uint16_t readAddressBus(); // Чтение адреса с ША Микроши

// Содержимое памяти
#define START_MEM_ADDR 0x8000
#define MEM_LEN 5
uint8_t mem[MEM_LEN]={0x55, 0x00, 0xFF, 0x01, 0x20};


int main(void)
{
    // Начальные инициализации оборудования STM32 для работы с шинами Микроши
    clockInit();
    portClockInit();
    addressBusInit();
    dataBusInit();
    systemPinsInit();

    // Флаг слежения за активностью шины данных
    // Если false - шина данных неактивна, и находится в высокоимпендансном состоянии
    // Если true - шина данных активна, на пинах выставлены какие-то данные
    bool dataBusActive=false;

    while (true) 
    {
        // Проверка системных сигналов /32К и /RD
        // Если они неактивны, ШД должна находиться в высокоимпендансном состоянии
        // и никаких действий происходить не должно
        uint32_t workAddrDiapason = GPIOA->IDR & (GPIO_IDR_IDR10_Msk | GPIO_IDR_IDR11_Msk);
        
        // Сигналы инверсны, значит и оба активных бита должны быть равны нулю, 
        // а остальные из-за маски нулевые
        if(workAddrDiapason!=0) // Сигналы /32К и /RD физически не установлены в 0
        {
            // Надо проследить чтобы ШД была в высокоимпедансном состоянии
            // и больше ничего не делать
            if(dataBusActive==true)
            {
                const uint32_t mode=0b11; // Режим выхода, с максимальной частотой 50 МГц
                const uint32_t cnf=0b01;  // Выход с открытым коллектором, высокоимпендансный режим
                GPIOB->CRH |= (mode << GPIO_CRH_MODE8_Pos)  | (cnf << GPIO_CRH_CNF8_Pos)  |
                              (mode << GPIO_CRH_MODE9_Pos)  | (cnf << GPIO_CRH_CNF9_Pos)  |
                              (mode << GPIO_CRH_MODE10_Pos) | (cnf << GPIO_CRH_CNF10_Pos) |
                              (mode << GPIO_CRH_MODE11_Pos) | (cnf << GPIO_CRH_CNF11_Pos) |
                              (mode << GPIO_CRH_MODE12_Pos) | (cnf << GPIO_CRH_CNF12_Pos) |
                              (mode << GPIO_CRH_MODE13_Pos) | (cnf << GPIO_CRH_CNF13_Pos) |
                              (mode << GPIO_CRH_MODE14_Pos) | (cnf << GPIO_CRH_CNF14_Pos) |
                              (mode << GPIO_CRH_MODE15_Pos) | (cnf << GPIO_CRH_CNF15_Pos);
                
                dataBusActive=false;
            }

            continue;
        }
        else // Иначе /32К и /RD активны (оба в физ. нуле)
        {
            // Нужно получить текущий адрес с ША
            uint16_t addr=readAddressBus();

            // Если адрес в диапазоне эмуляции ПЗУ
            if( addr>=START_MEM_ADDR && addr<(START_MEM_ADDR+MEM_LEN) )
            {
                // Байт, который будет выдан на ШД
                uint8_t byte=mem[addr-START_MEM_ADDR];

                // ШД должна стать активной
                const uint32_t mode=0b11; // Режим выхода, с максимальной частотой 50 МГц
                const uint32_t cnf=0b00;  // Двухтактный выход (Output push-pull)
                GPIOB->CRH |= (mode << GPIO_CRH_MODE8_Pos)  | (cnf << GPIO_CRH_CNF8_Pos)  |
                              (mode << GPIO_CRH_MODE9_Pos)  | (cnf << GPIO_CRH_CNF9_Pos)  |
                              (mode << GPIO_CRH_MODE10_Pos) | (cnf << GPIO_CRH_CNF10_Pos) |
                              (mode << GPIO_CRH_MODE11_Pos) | (cnf << GPIO_CRH_CNF11_Pos) |
                              (mode << GPIO_CRH_MODE12_Pos) | (cnf << GPIO_CRH_CNF12_Pos) |
                              (mode << GPIO_CRH_MODE13_Pos) | (cnf << GPIO_CRH_CNF13_Pos) |
                              (mode << GPIO_CRH_MODE14_Pos) | (cnf << GPIO_CRH_CNF14_Pos) |
                              (mode << GPIO_CRH_MODE15_Pos) | (cnf << GPIO_CRH_CNF15_Pos);
                
                dataBusActive=true;


                // На шине данных выставляется нужный байт

                // Текущие состояния всех пинов порта B
                uint16_t allPins=GPIOB->IDR;

                // Остаются состояния не-data пинов, а data-пины обнуляются
                allPins = allPins & 0x0000FFFF;

                // Значение байта данных смещается в область data-пинов (в биты 8-15)
                uint16_t dataPins=((uint16_t)byte) << 8;

                // Биты данных подмешиваются в значения всех пинов
                allPins = allPins | dataPins;

                GPIOB->ODR=allPins;
            }    
        }


        // Медленное переключение
        // 1мкс+1мкс = 2мкс - один период - на обычной частоте (неизвестно какой, которая стандартная)
        // GPIOB->ODR ^= (1<<LED1);

        // Более быстрое переключение
        // 250нс+250нс = 500нс - один период - на обычной частоте (неизвестно какой, которая стандартная)
        // 30нс+30нс = 60нс - один период - на частоте 72МГц
        // GPIOB->BSRR = (1<<LED1); // Hi
        // GPIOB->BRR = (1<<LED1); // Low
    }

    return 0;
}


// Функция не используется, оставлена для примера
void msDelay(int ms)
{
   while (ms-- > 0) {
      volatile int x=500;
      while (x-- > 0)
         __asm("nop");
   }
}


// Чтение адреса с адресной шины Микроши
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
