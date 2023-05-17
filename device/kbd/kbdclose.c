/* kbdclose.c  -  kbdclose */

#include <xinu.h>
#include <keyboard.h>

/*------------------------------------------------------------------------
 * kbdclose  -  Close the keyboard device
 *------------------------------------------------------------------------
 */
devcall	kbdclose (
	  struct dentry	*devptr		/* Entry in device switch table	*/
	)
{
	if(kbd_state.pr_guest!=currpid)
		return SYSERR;
	signal(kbd_state.sem_device);
	kbd_state.pr_guest = -1;
	return OK;
}
