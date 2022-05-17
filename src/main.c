#include <stdbool.h>

#include "stm32f1xx.h"

#include "initDevice.h"


// Прототипы используемых функций
uint16_t readAddressBus(); // Чтение адреса с ША Микроши
void delayMs(int ms);
void delayCycles(uint32_t cycles);


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
    debugLedInit();

    // Задержка, чтобы Микроша успела нормально включиться
    delayMs(1000);

    // Флаг слежения за активностью шины данных
    // Если false - шина данных неактивна, и находится в высокоимпендансном состоянии
    // Если true - шина данных активна, на пинах выставлены какие-то данные
    bool dataBusActive=false;

    while (true) 
    {
        // Проверка системных сигналов /32К и /RD
        // Если они неактивны, пины STM должна быть в режиме входа чтобы не влиять на ШД
        // и никаких действий происходить не должно
        uint32_t workAddrDiapason = GPIOA->IDR & (GPIO_IDR_IDR10_Msk | GPIO_IDR_IDR11_Msk);
        
        // Сигналы инверсны, значит и оба активных бита должны быть равны нулю, 
        // а остальные из-за маски нулевые
        if(workAddrDiapason!=0) // Если сигналы /32К и /RD физически не установлены в 0
        {
            // Надо проследить чтобы пины STM были в состоянии входа
            // чтобы они никак не влияли на ШД компьютера
            // и больше ничего не делать
            if(dataBusActive==true)
            {
                const uint32_t mode=0b00; // Режим входа
                const uint32_t cnf=0b10;  // Вход с подтягиванием

                // Текущие режимы всех ножек порта B
                uint32_t statusB=GPIOB->CRH;

                // Обнуление битов режима для ножек ШД
                statusB &= ~(GPIO_CRH_MODE8_Msk  | GPIO_CRH_CNF8_Msk |
                             GPIO_CRH_MODE9_Msk  | GPIO_CRH_CNF9_Msk |
                             GPIO_CRH_MODE10_Msk | GPIO_CRH_CNF10_Msk |
                             GPIO_CRH_MODE11_Msk | GPIO_CRH_CNF11_Msk |
                             GPIO_CRH_MODE12_Msk | GPIO_CRH_CNF12_Msk |
                             GPIO_CRH_MODE13_Msk | GPIO_CRH_CNF13_Msk |
                             GPIO_CRH_MODE14_Msk | GPIO_CRH_CNF14_Msk |
                             GPIO_CRH_MODE15_Msk | GPIO_CRH_CNF15_Msk );

                // Установка битов режима
                GPIOB->CRH = statusB | 
                             (mode << GPIO_CRH_MODE8_Pos)  | (cnf << GPIO_CRH_CNF8_Pos)  |
                             (mode << GPIO_CRH_MODE9_Pos)  | (cnf << GPIO_CRH_CNF9_Pos)  |
                             (mode << GPIO_CRH_MODE10_Pos) | (cnf << GPIO_CRH_CNF10_Pos) |
                             (mode << GPIO_CRH_MODE11_Pos) | (cnf << GPIO_CRH_CNF11_Pos) |
                             (mode << GPIO_CRH_MODE12_Pos) | (cnf << GPIO_CRH_CNF12_Pos) |
                             (mode << GPIO_CRH_MODE13_Pos) | (cnf << GPIO_CRH_CNF13_Pos) |
                             (mode << GPIO_CRH_MODE14_Pos) | (cnf << GPIO_CRH_CNF14_Pos) |
                             (mode << GPIO_CRH_MODE15_Pos) | (cnf << GPIO_CRH_CNF15_Pos);

                // Направление подтяжки - подтягивание к общему проводу
                GPIOB->BRR = GPIO_ODR_ODR8_Msk |
                             GPIO_ODR_ODR9_Msk |
                             GPIO_ODR_ODR10_Msk |
                             GPIO_ODR_ODR11_Msk |
                             GPIO_ODR_ODR12_Msk |
                             GPIO_ODR_ODR13_Msk |
                             GPIO_ODR_ODR14_Msk |
                             GPIO_ODR_ODR15_Msk;

                dataBusActive=false;

                // GPIOA->BRR = (1<<GPIO_BRR_BR0_Pos); // Светодиод выключается
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
                // Если ШД неактивна
                // if(dataBusActive==false)
                {
                    // ШД должна стать активной
                    const uint32_t mode=0b11; // Режим выхода с максимальной частотой 50 МГц
                    const uint32_t cnf=0b00;  // Двухтактный выход (Output push-pull)

                    // Текущие режимы всех ножек порта B
                    uint32_t statusB=GPIOB->CRH;

                    // Обнуление битов режима для ножек ШД
                    statusB &= ~(GPIO_CRH_MODE8_Msk  | GPIO_CRH_CNF8_Msk |
                                 GPIO_CRH_MODE9_Msk  | GPIO_CRH_CNF9_Msk |
                                 GPIO_CRH_MODE10_Msk | GPIO_CRH_CNF10_Msk |
                                 GPIO_CRH_MODE11_Msk | GPIO_CRH_CNF11_Msk |
                                 GPIO_CRH_MODE12_Msk | GPIO_CRH_CNF12_Msk |
                                 GPIO_CRH_MODE13_Msk | GPIO_CRH_CNF13_Msk |
                                 GPIO_CRH_MODE14_Msk | GPIO_CRH_CNF14_Msk |
                                 GPIO_CRH_MODE15_Msk | GPIO_CRH_CNF15_Msk );

                    // Установка битов режима
                    GPIOB->CRH = statusB | 
                                 (mode << GPIO_CRH_MODE8_Pos)  | (cnf << GPIO_CRH_CNF8_Pos)  |
                                 (mode << GPIO_CRH_MODE9_Pos)  | (cnf << GPIO_CRH_CNF9_Pos)  |
                                 (mode << GPIO_CRH_MODE10_Pos) | (cnf << GPIO_CRH_CNF10_Pos) |
                                 (mode << GPIO_CRH_MODE11_Pos) | (cnf << GPIO_CRH_CNF11_Pos) |
                                 (mode << GPIO_CRH_MODE12_Pos) | (cnf << GPIO_CRH_CNF12_Pos) |
                                 (mode << GPIO_CRH_MODE13_Pos) | (cnf << GPIO_CRH_CNF13_Pos) |
                                 (mode << GPIO_CRH_MODE14_Pos) | (cnf << GPIO_CRH_CNF14_Pos) |
                                 (mode << GPIO_CRH_MODE15_Pos) | (cnf << GPIO_CRH_CNF15_Pos);

                    // Настройка одного пина 15, использовалось при отладке
                    // uint32_t statusB=GPIOB->CRH;
                    // statusB &= ~(GPIO_CRH_MODE15_Msk | GPIO_CRH_CNF15_Msk); // Обнуление битов режима
                    // GPIOB->CRH = statusB | (mode << GPIO_CRH_MODE15_Pos) | (cnf << GPIO_CRH_CNF15_Pos); // Установка режима
                    
                    dataBusActive=true;
                }

                /*
                static bool blinkFlag=false;
                blinkFlag=!blinkFlag;
                if(blinkFlag)
                {
                    GPIOA->BSRR = (1<<GPIO_BSRR_BS0_Pos); // Светодиод включается
                    GPIOB->BSRR = (1<<GPIO_BSRR_BS15_Pos); // Пин ШД7 включается
                }                    
                else
                {
                    GPIOA->BSRR = (1<<GPIO_BSRR_BR0_Pos); // Светодиод выключается
                    GPIOB->BSRR = (1<<GPIO_BSRR_BR15_Pos); // Пин ШД7 вылючается
                }
                */


                GPIOA->BSRR = (1<<GPIO_BSRR_BS0_Pos); // Светодиод включается

                // Байт, который будет выдан на ШД
                uint8_t byte=0x00; // mem[addr-START_MEM_ADDR];

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
