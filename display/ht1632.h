#ifndef HT1632_H
#define	HT1632_H

#define HT1632_WIDTH 24
#define HT1632_HEIGHT 16
#define HT1632_ADDR_SPACE_SIZE (HT1632_WIDTH*HT1632_HEIGHT/4)


#define HT1632_CMD_SYSDIS 0b00000000  /* CMD= 0000-0000-x Turn off oscil */
#define HT1632_CMD_SYSEN  0b00000001  /* CMD= 0000-0001-x Enable system oscil */
#define HT1632_CMD_LEDOFF 0b00000010  /* CMD= 0000-0010-x LED duty cycle gen off */
#define HT1632_CMD_LEDON  0b00000011  /* CMD= 0000-0011-x LEDs ON */
#define HT1632_CMD_BLOFF  0b00001000  /* CMD= 0000-1000-x Blink ON */
#define HT1632_CMD_BLON   0b00001001  /* CMD= 0000-1001-x Blink Off */
#define HT1632_CMD_SLVMD  0b00010000  /* CMD= 0001-00xx-x Slave Mode */
#define HT1632_CMD_MSTMD  0b00010100  /* CMD= 0001-01xx-x Master Mode */
#define HT1632_CMD_RCCLK  0b00011000  /* CMD= 0001-10xx-x Use on-chip clock */
#define HT1632_CMD_EXTCLK 0b00011100  /* CMD= 0001-11xx-x Use external clock */
#define HT1632_CMD_COMS00 0b00100000  /* CMD= 0010-ABxx-x commons options */
#define HT1632_CMD_COMS01 0b00100100  /* CMD= 0010-ABxx-x commons options */
#define HT1632_CMD_COMS10 0b00101000  /* CMD= 0010-ABxx-x commons options */
#define HT1632_CMD_COMS11 0b00101100  /* CMD= 0010-ABxx-x commons options */
#define HT1632_CMD_PWM_T  0b10100000  /* CMD= 101x-PPPP-x PWM duty cycle - template*/
#define HT1632_CMD_PWM(lvl) (HT1632_CMD_PWM_T | (lvl-1))
  /* Produces the correct command from the given value of lvl. lvl = [0..15] */
#define HT1632_CMD_LEN    8  /* Commands are 8 bits long, excluding the trailing bit */
#define HT1632_ADDR_LEN   7  /* Addresses are 7 bits long */
#define HT1632_WORD_LEN   4  /* Words are 4 bits long */




#endif	/* HT1632_H */

