#include <xinu.h>
#include <keyboard.h>

static int32 kpgetchar()
{
	int32 retval = kbd_state.buffer[(kbd_state.tail++)%KBD_BUFFER_SIZE];
	kbd_state.tail++;
	kbd_state.count--;
}

devcall	kbdgetc(struct	dentry	*devptr)
{	
	char	ch;			/* Character to read		*/
	int32	retval;			/* Return value			*/

	if(getpid()!=kbd_state.pr_guest)	// NOT OPEN
		return SYSERR;

	wait(kbd_state.sem_buffer);
	retval = kpgetchar();

	return (devcall)ch;
}