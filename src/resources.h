/*
	resources.h for Atomulator.
*/
#include "sidtypes.h"

#define IDM_FILE_RESET      40000
#define IDM_FILE_EXIT       40001
#define IDM_SETTINGS        40002
#define IDM_ABOUT           40003
#define IDM_OVERSCAN        40004
#define IDM_TAPE_LOAD       40010
#define IDM_TAPE_EJECT      40011
#define IDM_TAPE_REW        40012
#define IDM_TAPE_CAT        40013
#define IDM_TAPES_NORMAL    40015
#define IDM_TAPES_FAST      40016
#define IDM_DISC_LOAD_0     40020
#define IDM_DISC_LOAD_1     40021
#define IDM_DISC_EJECT_0    40022
#define IDM_DISC_EJECT_1    40023
#define IDM_DISC_NEW_0      40024
#define IDM_DISC_NEW_1      40025
#define IDM_DISC_WPROT_0    40026
#define IDM_DISC_WPROT_1    40027
#define IDM_DISC_WPROT_D    40028
#define IDM_DISC_AUTOBOOT   40029
#define IDM_HARD_BBC        40031
#define IDM_SOUND_ATOM      40040
#define IDM_SOUND_TAPE      40041
#define IDM_SOUND_DDNOISE   40042
#define IDM_VID_SNOW        40050
#define IDM_VID_FULLSCREEN  40051
#define IDM_VID_COLOUR      40052
#define IDM_VID_PALNOTNTSC  40053
#define IDM_KEY_REDEFINE    40060
#define IDM_KEY_DEFAULT     40061

// SP3 JOYSTICK SUPPORT

#define IDM_JOYSTICK        40070 // Joystick support

// END SP3

// SP10 KEYBOARDJOYSTICK SUPPORT

#define IDM_KEYJOYSTICK     40071 // Keyboardjoystick support

// END SP10

#define IDM_DDV_33          40080
#define IDM_DDV_66          40081
#define IDM_DDV_100         40082
#define IDM_DDT_525         40085
#define IDM_DDT_35          40086
#define IDM_MISC_DEBUG      40090
#define IDM_MISC_DEBONBRK	40091
#define IDM_MISC_BREAK      40092
#define IDM_MISC_SCRSHOT    40093

#define IDM_RAMROM_ENABLE   40094
#define IDM_RAMROM_DISKROM	40095

#define IDM_SOUND_ATOMSID   42000
#define IDM_WAVE_SID        42001
#define IDM_SID_INTERP      42002
#define IDM_SID_RESAMP      42003

#define IDM_SID_TYPE        43000

#define IDM_GDOS2015_ENABLE	43100
#define IDM_GDOS_BANK		43200

#define IDM_MAIN_RAM		43300
#define IDM_VIDEO_RAM		43400

#define IDM_SPD_10          41080
#define IDM_SPD_25          41081
#define IDM_SPD_50          41082
#define IDM_SPD_75          41083
#define IDM_SPD_100         41084
#define IDM_SPD_150         41085
#define IDM_SPD_170         41086
#define IDM_SPD_200         41087
#define IDM_SPD_300         41088
#define IDM_SPD_400         41089
#define IDM_SPD_500         41090

#define Button1 			1000
#define ListBox1 			40900

#define IDM_RAMROM_FAST		IDM_SPD_170

#define IDM_FILE_MENU		80001
#define IDM_TAPE_MENU		80002
#define IDM_TAPESPEED_MENU	80003
#define IDM_DISC_MENU		80004
#define IDM_ROMBANK_MENU	80005
#define IDM_RESID_MENU		80006
#define IDM_SIDMODEL_MENU	80007
#define IDM_SIDMETHOD_MENU	80008
#define IDM_SETTINGS_MENU	80009
#define IDM_VIDEO_MENU		80010
#define IDM_HARDWARE_MENU	80011
#define IDM_RAMROM_MENU		80012
#define IDM_RAM_MENU		80013
#define IDM_MAINRAM_MENU	80014
#define IDM_VIDEORAM_MENU	80015
#define IDM_FLOPPY_MENU		80016
#define IDM_DDVOL_MENU		80017
#define IDM_SOUND_MENU		80018
#define IDM_KEYBOARD_MENU	80019
#define IDM_JOYSTICK_MENU	80020
#define IDM_MISC_MENU		80021
#define IDM_SPEED_MENU		80022

#define IDM_SHOWSPEED		50000

#define IDM_RAM_1K			50001
#define IDM_RAM_1K_PLUS_5K	50002
#define IDM_RAM_6K_PLUS_3K	50003
#define IDM_RAM_6K_PLUS_19K	50004
#define IDM_RAM_6K_PLUS_22K	50005	
#define IDM_RAM_6K_PLUS_23K	50006

#define IDM_VIDEORAM_1K		50007
#define IDM_VIDEORAM_2K		50008
#define IDM_VIDEORAM_3K		50009
#define IDM_VIDEORAM_4K		50010
#define IDM_VIDEORAM_5K		50011
#define IDM_VIDEORAM_6K		50012
#define IDM_VIDEORAM_7K		50013
#define IDM_VIDEORAM_8K		50014

#define IDM_ROMBANK_00		50015
#define IDM_ROMBANK_01		50016
#define IDM_ROMBANK_02		50017
#define IDM_ROMBANK_03		50018
#define IDM_ROMBANK_04		50019
#define IDM_ROMBANK_05		50020
#define IDM_ROMBANK_06		50021
#define IDM_ROMBANK_07		50022
#define IDM_ROMBANK_08		50023
#define IDM_ROMBANK_09		50024
#define IDM_ROMBANK_10		50025
#define IDM_ROMBANK_11		50026
#define IDM_ROMBANK_12		50027
#define IDM_ROMBANK_13		50028
#define IDM_ROMBANK_14		50029
#define IDM_ROMBANK_15		50030

#define IDM_SID_6581			50031
#define IDM_SID_8580			50032
#define IDM_SID_8580DB			50033
#define IDM_SID_6581R4			50034
#define IDM_SID_6581R34885		50035
#define IDM_SID_6581R30486S		50036
#define IDM_SID_6581R33984		50037
#define IDM_SID_6581R4AR3789	50038
#define IDM_SID_6581R34485		50039
#define IDM_SID_6581R41986S		50040
#define IDM_SID_8085R53691		50041
#define IDM_SID_8580R53691DB	50042
#define IDM_SID_8580R51489		50043
#define IDM_SID_8580R51489DB	50044
