 /* source: https://forum.osdev.org/viewtopic.php?t=10247 */

/* kbdhandler.c - kbdhandler */

#include <xinu.h>
#include <keyboard.h>

unsigned char tecla_actual;
unsigned long long keys_states = 0ULL;

unsigned char get_scancode()
{
    unsigned char inputdata;
    inputdata = inportb(KEYBOARD_DATAREG);
    return inputdata;
}

int check_key(unsigned char key)
{
	return (keys_states >> key) & 1ULL;
}

/*------------------------------------------------------------------------
 *  kbdhandler  -  Handle an interrupt for the keyboard device
 *------------------------------------------------------------------------
 */
void kbdhandler(void)
{
	char t[80];
	unsigned char scancode; 
	unsigned int shift_key = 0;
	int i = 10;

	scancode = get_scancode();

	if(scancode < 0x80)
		keys_states |= (1ULL << scancode);
	else 
		keys_states &= ~(1ULL << (scancode-0x80));

	tecla_actual = scancode;
	sprintf(t, "kbd: 0x%x     ", scancode);
	print_text_on_vga(10, 300, t);

	if(scancode == 0x2A) {
		shift_key = 1;//Shift key is pressed
	} else if(scancode & 0xAA) {
		shift_key = 0;//Shift Key is not pressed
	} else {          
		if (scancode & 0x80) {
			int shiftaltctrl = 1;//Put anything to see what special keys were pressed
		} else {  
		}     
	}
}

