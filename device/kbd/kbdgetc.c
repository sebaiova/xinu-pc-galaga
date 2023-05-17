#include <xinu.h>
#include <keyboard.h>

devcall	kbdgetc(struct	dentry	*devptr)
{	
	int32 retval;
	if(getpid()!=kbd_state.pr_guest)	// NOT OPEN
		return SYSERR;

	wait(kbd_state.sem_buffer);

	retval = kbd_state.buffer[kbd_state.tail%KBD_BUFFER_SIZE];
	kbd_state.tail++;
	kbd_state.count--;

	return (devcall)(0xFF & retval);
}