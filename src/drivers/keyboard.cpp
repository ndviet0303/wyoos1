
#include <drivers/keyboard.h>

using namespace myos::common;
using namespace myos::drivers;
using namespace myos::hardwarecommunication;


KeyboardEventHandler::KeyboardEventHandler()
{
}

void KeyboardEventHandler::OnKeyDown(char)
{
}

void KeyboardEventHandler::OnKeyUp(char)
{
}





KeyboardDriver::KeyboardDriver(InterruptManager* manager, KeyboardEventHandler *handler)
: InterruptHandler(manager, 0x21),
dataport(0x60),
commandport(0x64)
{
    this->handler = handler;
}

KeyboardDriver::~KeyboardDriver()
{
}

void printf(char*);
void printfHex(uint8_t);

void KeyboardDriver::Activate()
{
    while(commandport.Read() & 0x1)
        dataport.Read();
    commandport.Write(0xae); // activate interrupts
    commandport.Write(0x20); // command 0x20 = read controller command byte
    uint8_t status = (dataport.Read() | 1) & ~0x10;
    commandport.Write(0x60); // command 0x60 = set controller command byte
    dataport.Write(status);
    dataport.Write(0xf4);
}

uint32_t KeyboardDriver::HandleInterrupt(uint32_t esp)
{
    uint8_t key = dataport.Read();
    
    if(handler == 0)
        return esp;
    
    if(key < 0x80)
    {
        bool shiftPressed = false;
        switch(key)
        {
            case 0x4F:
            case 0x02: handler->OnKeyDown('1'); break;
            case 0x50:
            case 0x03: handler->OnKeyDown('2'); break;
            case 0x51:
            case 0x04: handler->OnKeyDown('3'); break;
            case 0x4B:
            case 0x05: handler->OnKeyDown('4'); break;
            case 0x4C:
            case 0x06: handler->OnKeyDown('5'); break;
            case 0x4D:
            case 0x07: handler->OnKeyDown('6'); break;
            case 0x47:
            case 0x08: handler->OnKeyDown('7'); break;
            case 0x48:
            case 0x09: handler->OnKeyDown('8'); break;
            case 0x49:
            case 0x0A: handler->OnKeyDown('9'); break;
            case 0x52:
            case 0x0B: handler->OnKeyDown('0'); break;
            case 0x34:
            case 0x53: handler->OnKeyDown('.'); break;
            case 0x4A:
            case 0x0C: handler->OnKeyDown('-'); break;
            case 0x0D: handler->OnKeyDown(shiftPressed ? '+' : '='); break;
            case 0x4E: handler->OnKeyDown('+'); break;
            case 0x35: handler->OnKeyDown('/'); break;
            case 0x37: handler->OnKeyDown('*'); break;
            case 0x1C: handler->OnKeyDown('\n'); break;
            case 0x39: handler->OnKeyDown(' '); break;

            default:
            {
                printf("KEYBOARD 0x");
                printfHex(key);
                break;
            }
        }
    }
    return esp;
}
