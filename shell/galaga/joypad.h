#pragma once

/*starts process & open kbd device*/
int joypad_run();

/*stops process & releases kbd device*/
int joypad_stop();

/*check for key pressed*/
int joypad_check(unsigned char key);
