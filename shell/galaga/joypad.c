#include <xinu.h>
#include <keyboard.h>

static unsigned long long keys_states;
static pid32 pid_joypad;

static int joypad_handler()
{
	open(KEYBOARD, NULL, NULL);
	control(KEYBOARD, KBD_CLEAR, NULL, NULL);
	
	while(recvclr()==OK) 
	{
		unsigned char c = getc(KEYBOARD);
	    if(c < 0x80)
	    	keys_states |= (1ULL << c);
	    else 
	    	keys_states &= ~(1ULL << (c-0x80));
	}
    close(KEYBOARD);
}

int joypad_run()
{
	keys_states = 0ULL;
    pid_joypad = create(joypad_handler, 1024, 20, "Joypad", 0);
    resume(pid_joypad);
    return OK;
}

int joypad_stop()
{
	send(pid_joypad, -1);
    return OK;
}

int joypad_check(unsigned char key)
{
	return (keys_states >> key) & 1ULL;
}

