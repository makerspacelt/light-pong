#include "mem.h"
#include "c_types.h"
#include "user_interface.h"
#include "ets_sys.h"
#include "driver/uart.h"
#include "osapi.h"
#include "espconn.h"
#include "ws2812_i2s.h"
#include "gpio.h"
#include "game.h"
#include "network.h"

// Leds frame buffer
uint8_t frameBuffer[LEDS*3];

int led = 0;
game_mode gameMode = START;
game_mode lastGameMode = START;
uint8_t isSafe = 0;
uint8_t strobe = 0;
uint8_t scoreRepeat = 0;
uint8_t activeStrip = 0;
uint8_t clearStep = 0;

Player player1 = {1, 1, NULL, false, 0};
Player player2 = {2, 1, NULL, false, 0};

signed char dir = 1;

Player *getPlayerByConnection(struct espconn *connection)
{
    if (player1.connection == connection) {
        return &player1;
    } else {
        return &player2;
    }
}

Player *getPlayer(uint8_t nr)
{
    if(nr == 1) {
        return &player1;
    } else {
        return &player2;
    }
}

// Prepare player start zone and game variables
void ICACHE_FLASH_ATTR prepareGame()
{
    isSafe = 0;

    int i;
    for (i = 0; i < LEDS; i++ ) {
        frameBuffer[i*3] = 0x00; 
        frameBuffer[i*3+1] = 0x00;
        frameBuffer[i*3+2] = 0x00;
        if (dir == -1 && i >= LEDS - SAFEZONE) {
            led = LEDS - 1;
            frameBuffer[i*3] = 0x50;
            frameBuffer[i*3+1] = 0xFF;
        } else if (dir == 1 && i < SAFEZONE) {
            led = 0;
             frameBuffer[i*3+2] = 0x80;
            
        }
    }

    ws2812_push(frameBuffer, sizeof(frameBuffer));
    gameMode = START;
}

void ICACHE_FLASH_ATTR allStrips()
{
    gpio_output_set(BIT12, 0, BIT12, 0);
    gpio_output_set(BIT13, 0, BIT13, 0);
    gpio_output_set(BIT14, 0, BIT14, 0);
}

void ICACHE_FLASH_ATTR switchStrip()
{
    lastGameMode = gameMode;
    gameMode = CLEAR;

    int i;
    for (i = 0; i < LEDS; i++ ) {
        frameBuffer[i*3] = 0x00; 
        frameBuffer[i*3+1] = 0x00;
        frameBuffer[i*3+2] = 0x00;
    }

    allStrips();
    ws2812_push(frameBuffer, sizeof(frameBuffer));

    uint8_t temp = activeStrip;
    while (temp == activeStrip) {
        activeStrip = ( os_random() % 3 ) + 1;
    }
    os_printf("Random Strip: %d", activeStrip);
}



// System task which will monitor players input
void ICACHE_FLASH_ATTR inputMonitor(os_event_t *events)
{
    #ifdef EMULATE_P1
        player1.button = GPIO_INPUT_GET(0);
    #endif

    if (gameMode == START) {
        if ((dir == 1 && !player1.button) || dir == -1 && !player2.button) {
            gameMode = MOVING;
            switchStrip();
            os_printf("Arming frame TIMER");
            os_timer_arm(&frameTimer, SPEED, 1);
        }
    }else if(gameMode == MOVING) {
        if (dir == 1) {
            if (!player2.button) {
                if (isSafe) {
                    led = LEDS - 1;
                    dir = -1;
                    switchStrip();
                    callSound(SOUND_PONG);
                } else {
                    os_timer_disarm(&frameTimer);

                    gameMode = SCORE_PHASE1;
                    player1.score++;
                    os_printf("SCORE %d VS %d\n", player1.score, player2.score);
                    callSound(SOUND_SCORE);

                    allStrips();
                    os_timer_arm(&scoreTimer, 5, 1); 
                }
            }
        } else {
            if (!player1.button) {
                if (isSafe) {
                    led = 0;
                    dir = 1;
                    switchStrip();
                    callSound(SOUND_PONG);
                } else {
                    os_timer_disarm(&frameTimer);

                    gameMode = SCORE_PHASE1;
                    player2.score++;
                    os_printf("SCORE %d VS %d\n", player1.score, player2.score);
                    callSound(SOUND_SCORE);

                    allStrips();
                    os_timer_arm(&scoreTimer, 5, 1);
                }
            }
        }
    }else if (gameMode == WIN) {
        if (player1.button == 0 && player2.button == 0) {
            os_timer_disarm(&winTimer);
            player1.button = player2.button = 1;
            player1.score = player2.score = 0;

            callSound(SOUND_MUSIC);
            strobe = 0;
            scoreRepeat = 0;
            prepareGame();           
        }
    }

    system_os_post(0, 0, 0 );
}

// A timer callback used to display fancy score lines
void ICACHE_FLASH_ATTR scoreTimerCallback(void *arg)
{
    int i;   
    for (i = 0; i < LEDS; i++) {
        frameBuffer[i*3] = 0x00;
        frameBuffer[i*3+1] = 0x00;
        frameBuffer[i*3+2] = 0x00;
        if (i < player1.score * SCORE_LEDS) {
            uint8 blue = strobe * 0x80 / 0xFF;
            frameBuffer[i*3+2] = strobe; // Player1 score
        } else if (i < LEDS - MAX_SCORE * SCORE_LEDS && i >= MAX_SCORE * SCORE_LEDS) {
            frameBuffer[i*3+1] = strobe; // Center line
        } else if (i >= LEDS - player2.score * SCORE_LEDS) {
            frameBuffer[i*3+1] = strobe; // Player2 score

            uint8_t green = strobe * 0x50 / 0xFF; 
            frameBuffer[i*3] = green;
        }
    }
    ws2812_push(frameBuffer, sizeof(frameBuffer));

    if (gameMode == SCORE_PHASE1) {
        strobe += 10;
        if (strobe >= 250) {
            gameMode = SCORE_PHASE2;
        }
    } else {
        strobe -= 10;
        if(strobe <= 10) {
            gameMode = SCORE_PHASE1;
            scoreRepeat++;
        }
    }

    if (scoreRepeat >= 3) {
        os_timer_disarm(&scoreTimer);

        scoreRepeat = 0;
        strobe = 0;
        dir = 0 - dir;

        if (player1.score == MAX_SCORE || player2.score == MAX_SCORE) {
            gameMode = WIN;
            os_timer_arm(&winTimer, 40, 1);
        } else {
            prepareGame();
        }
    }
}

void ICACHE_FLASH_ATTR winTimerCallback(void *arg)
{
    int i;   
    for (i = 0; i < LEDS; i++) {
        frameBuffer[i*3] = 0x00;
        frameBuffer[i*3+1] = 0x00;
        frameBuffer[i*3+2] = 0x00;

        if (player1.score == MAX_SCORE) {
            uint8_t blue = strobe * 0x80 / 0xFF;
            frameBuffer[i*3+2] = blue;
        } else {
            uint8_t green = strobe * 0x50 / 0xFF;
            frameBuffer[i*3] = green;
            frameBuffer[i*3+1] = strobe;
        }
    }
    ws2812_push(frameBuffer, sizeof(frameBuffer));

    if (scoreRepeat == 0) {
        strobe += 10;
        if (strobe >= 250) {
            scoreRepeat = 1;
        }
    } else {
        strobe -= 10;
        if (strobe <= 10) {
            scoreRepeat = 0;
        }
    }
}

// A timer callback used to display each frame of moving light in-play
void ICACHE_FLASH_ATTR frameTimerCallback(void *arg)
{
    if (gameMode == CLEAR) {
        clearStep++;
        if (clearStep > 5) {
            gameMode = CLEAR2;
            clearStep = 0;
        }

        return;
    }

    if (gameMode == CLEAR2) {
        gameMode = lastGameMode;

        gpio_output_set(0, BIT12, BIT12, 0);
        gpio_output_set(0, BIT13, BIT13, 0);
        gpio_output_set(0, BIT14, BIT14, 0);

        switch (activeStrip) {
            case 1:
                gpio_output_set(BIT12, 0, BIT12, 0);
                break;
            case 2:
                gpio_output_set(BIT13, 0, BIT13, 0);
                break;
            case 3:
                gpio_output_set(BIT14, 0, BIT14, 0);
                break;
        }

        return;
    }

    if (gameMode == LAST_FRAME) {
        gameMode = SCORE_PHASE1;
        os_timer_disarm(&frameTimer);
        os_timer_arm(&scoreTimer, 5, 1);
        allStrips();

        if (dir == 1) {
            led = 0;
        } else {
            led = LEDS - 1;
        }

        return;
    }

    if ((dir == -1 && led < SAFEZONE) || (dir == 1 && led >= LEDS - SAFEZONE)) {
        isSafe = 1;
    } else {
        isSafe = 0;
    }

    int i = 0;
    for (i = 0; i < LEDS; i++)
    {
        if (i <= led + TRAIL && i >= led - TRAIL) {
            frameBuffer[i*3] = 0xff; 
            frameBuffer[i*3+1] = 0xff;
            frameBuffer[i*3+2] = 0xff;
        } else {
            frameBuffer[i*3] = 0; 
            frameBuffer[i*3+1] = 0;
            frameBuffer[i*3+2] = 0;

            if (dir == -1 && led < SAFEZONE && i < SAFEZONE) {
                frameBuffer[i*3+2] = 0x80;
            } else if (dir == 1 && led >= LEDS - SAFEZONE && i >= LEDS - SAFEZONE) {
                frameBuffer[i*3+1] = 0xFF;
                frameBuffer[i*3] = 0x50;
            }
        }
    }

    if (dir == 1 && led == LEDS - 1) {
        player1.score++;
        os_printf("SCORE %d VS %d\n", player1.score, player2.score);
        gameMode = LAST_FRAME;
        callSound(SOUND_SCORE);

    } else if (dir == -1 && led == 0) {
        player2.score++;
        os_printf("SCORE %d VS %d\n", player1.score, player2.score);
        gameMode = LAST_FRAME;
        callSound(SOUND_SCORE);
    }

    led += dir;
    ws2812_push(frameBuffer, sizeof(frameBuffer));
}

void ICACHE_FLASH_ATTR callSound(uint8_t event)
{
    uint8_t data[1];

    switch (event) {
        case SOUND_SCORE:
            if (player1.score + player2.score == 1) {
                data[0] = SOUND_SCORE_FIRST;
            } else if(player1.score == MAX_SCORE || player2.score == MAX_SCORE) {
                data[0] = SOUND_VICTORY;
            } else {
                data[0] = SOUND_SCORE;
            }
            break;
        default:
            data[0] = event;
            break;
    }

     sint8 status = espconn_send(soundManager, data, 1);
     if (status == 0) {
         soundSending = true;
     }
     os_printf("Sound manager: %d\n", status);
}
