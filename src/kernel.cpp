
#include <common/types.h>
#include <gdt.h>
#include <memorymanagement.h>
#include <hardwarecommunication/interrupts.h>
#include <syscalls.h>
#include <hardwarecommunication/pci.h>
#include <drivers/driver.h>
#include <drivers/keyboard.h>
#include <drivers/mouse.h>
#include <drivers/vga.h>
#include <drivers/ata.h>
#include <gui/desktop.h>
#include <gui/window.h>
#include <multitasking.h>

#include <drivers/amd_am79c973.h>
#include <net/etherframe.h>
#include <net/arp.h>
#include <net/ipv4.h>
#include <net/icmp.h>
#include <net/udp.h>
#include <net/tcp.h>


// #define GRAPHICSMODE


using namespace myos;
using namespace myos::common;
using namespace myos::drivers;
using namespace myos::hardwarecommunication;
using namespace myos::gui;
using namespace myos::net;



void printf(char* str)
{
    static uint16_t* VideoMemory = (uint16_t*)0xb8000;

    static uint8_t x=0,y=0;

    for(int i = 0; str[i] != '\0'; ++i)
    {
        switch(str[i])
        {
            case '\n':
                x = 0;
                y++;
                break;
            default:
                VideoMemory[80*y+x] = (VideoMemory[80*y+x] & 0xFF00) | str[i];
                x++;
                break;
        }

        if(x >= 80)
        {
            x = 0;
            y++;
        }

        if(y >= 25)
        {
            for(y = 0; y < 25; y++)
                for(x = 0; x < 80; x++)
                    VideoMemory[80*y+x] = (VideoMemory[80*y+x] & 0xFF00) | ' ';
            x = 0;
            y = 0;
        }
    }
}

void printfHex(uint8_t key)
{
    char* foo = "00";
    char* hex = "0123456789ABCDEF";
    foo[0] = hex[(key >> 4) & 0xF];
    foo[1] = hex[key & 0xF];
    printf(foo);
}
void printfHex16(uint16_t key)
{
    printfHex((key >> 8) & 0xFF);
    printfHex( key & 0xFF);
}
void printfHex32(uint32_t key)
{
    printfHex((key >> 24) & 0xFF);
    printfHex((key >> 16) & 0xFF);
    printfHex((key >> 8) & 0xFF);
    printfHex( key & 0xFF);
}





class PrintfKeyboardEventHandler : public KeyboardEventHandler
{
private:
    float num1 = 0, num2 = 0;
    char operation = '\0';
    bool enteringNum2 = false;
    bool decimalMode = false;
    float decimalFactor = 0.1;
public:
    void ftoa(float num, char* str, int precision = 3)
    {
        int intPart = (int)num;  // Lấy phần nguyên
        float floatPart = num - intPart; // Lấy phần thập phân

        // Chuyển phần nguyên sang chuỗi
        int i = 0;
        if (intPart == 0)
            str[i++] = '0';
        else
        {
            if (intPart < 0)
            {
                str[i++] = '-';
                intPart = -intPart;
            }

            int temp = intPart, len = 0;
            while (temp) { len++; temp /= 10; }
            for (int j = len - 1; j >= 0; j--)
            {
                str[i + j] = (intPart % 10) + '0';
                intPart /= 10;
            }
            i += len;
        }

        // Thêm dấu chấm
        str[i++] = '.';

        // Xử lý phần thập phân
        for (int j = 0; j < precision; j++)
        {
            floatPart *= 10;
            int digit = (int)floatPart;
            str[i++] = '0' + digit;
            floatPart -= digit;
        }

        str[i] = '\0'; // Kết thúc chuỗi
    }
    void OnKeyDown(char c)
    {
        if (c >= '0' && c <= '9') // Nhập số
        {
            printf(&c);
            if (!enteringNum2)
            {
                if (decimalMode)
                {
                    num1 += (c - '0') * decimalFactor;
                    decimalFactor /= 10;
                }
                else
                {
                    num1 = num1 * 10 + (c - '0');
                }
            }
            else
            {
                if (decimalMode)
                {
                    num2 += (c - '0') * decimalFactor;
                    decimalFactor /= 10;
                }
                else
                {
                    num2 = num2 * 10 + (c - '0');
                }
            }
        }
        else if (c == '.' && !decimalMode) // Nhập dấu chấm
        {
            decimalMode = true;
            printf(".");
        }
        else if (c == '+' || c == '-' || c == '*' || c == '/') // Nhập phép toán
        {
            if (operation == '\0') // Chỉ nhận phép toán một lần
            {
                operation = c;
                enteringNum2 = true;
                decimalMode = false;
                decimalFactor = 0.1;
                printf(" ");
                printf(&c);
                printf(" ");
            }
        }
        else if (c == '\n') // Nhấn Enter để tính toán
        {
            float result = 0;
            bool valid = true;

            switch (operation)
            {
            case '+': result = num1 + num2; break;
            case '-': result = num1 - num2; break;
            case '*': result = num1 * num2; break;
            case '/': 
                if (num2 != 0) result = num1 / num2;
                else valid = false;
                break;
            default:
                return; // Nếu chưa có phép toán, không làm gì cả
            }

            printf("\nKet Qua: ");
            if (valid)
            {
                char resultStr[20];
                ftoa(result, resultStr, 3);
                printf(resultStr);
            }
            else
            {
                printf("Loi: Chia cho 0!");
            }

            // Reset để nhập phép toán mới
            num1 = num2 = 0;
            operation = '\0';
            enteringNum2 = false;
            decimalMode = false;
            decimalFactor = 0.1;
            printf("\nNhap phep tinh:\n");
        }
    }
};


typedef void (*constructor)();
extern "C" constructor start_ctors;
extern "C" constructor end_ctors;
extern "C" void callConstructors()
{
    for(constructor* i = &start_ctors; i != &end_ctors; i++)
        (*i)();
}



extern "C" void kernelMain(const void* multiboot_structure, uint32_t /*multiboot_magic*/)
{
    printf("Vui long nhap phep tinh(1+1 roi enter)\n");

    GlobalDescriptorTable gdt;
    
    TaskManager taskManager;
    InterruptManager interrupts(0x20, &gdt, &taskManager);
    PrintfKeyboardEventHandler kbhandler;
    KeyboardDriver keyboard(&interrupts, &kbhandler);
    
    interrupts.Activate();
    
    while(1)
    {
    }
}
