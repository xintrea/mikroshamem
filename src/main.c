#include <stdbool.h>

#include "stm32f1xx.h"

#include "initDevice.h"


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
        
        // Сигналы инверсны, значит и оба бита при активности равны нулю, 
        // а остальные из-за маски нулевые
        if(workAddrDiapason!=0) // Сигналы /32К и /RD физически не установлены в 0
        {
            // Надо проследить чтобы ШД была в высокоимпендансном состоянии
            // и больше ничего не делать
            if(dataBusActive==true)
            {
                // Доработать, так как ИЛИ без предварительного обнуления
                // управляющего регистра не выставит нужный режим
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
        else // Иначе /32К и /RD активны (оба в физ. нуле), и ШД должна стать активной
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


// Функция не используется, оставлена для примера
void msDelay(int ms)
{
   while (ms-- > 0) {
      volatile int x=500;
      while (x-- > 0)
         __asm("nop");
   }
}

