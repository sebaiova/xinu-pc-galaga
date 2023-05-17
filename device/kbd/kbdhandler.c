 /* source: https://forum.osdev.org/viewtopic.php?t=10247 */

/* kbdhandler.c - kbdhandler */

#include <xinu.h>
#include <keyboard.h>

unsigned char tecla_actual;

unsigned char get_scancode()
{
    unsigned char inputdata;
    inputdata = inportb(KEYBOARD_DATAREG);
    return inputdata;
}

/*------------------------------------------------------------------------
 *  kbdhandler  -  Handle an interrupt for the keyboard device
 *------------------------------------------------------------------------
 */
void kbdhandler(void)
{
	unsigned char scancode; 
	scancode = get_scancode();

	if(kbd_state.count<KBD_BUFFER_SIZE) 
	{
		kbd_state.buffer[kbd_state.head%KBD_BUFFER_SIZE] = scancode;
		kbd_state.head++;
		kbd_state.count++;
		signal(kbd_state.sem_buffer);
	}
}

