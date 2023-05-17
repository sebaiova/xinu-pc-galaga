/* kbdopen.c  -  kbdopen */

#include <xinu.h>
#include <keyboard.h>

/*------------------------------------------------------------------------
 * kbdopen  -  Open the ps/2 keyboard device
 *------------------------------------------------------------------------
 */

devcall	kbdopen (
	 struct	dentry	*devptr,	/* Entry in device switch table	*/
	 char	*name,			/* Unused for a kbd */
	 char	*mode			/* Unused for a kbd */
	)
{
	wait(kbd_state.sem_device);
	kbd_state.pr_guest = getpid();
}
