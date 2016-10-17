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
int pauseCycle = 0;

game_mode gameMode = START;
game_mode lastGameMode = START;

uint8_t isSafe = 0;
uint8_t strobe = 0;
uint8_t wasSafe = 0;
uint8_t nextCaller = 0;
uint8_t pongEventSent = 0;

uint8_t guilty = 0;

int scoreRepeat = 0;
uint8_t activeStrip = 0;
uint8_t clearStep = 0;
uint8_t speed = SPEED_START;

Player player1 = {1, {1, 1, 1, 1, 1, 1, 1, 1, 1, 1}, 0};
Player player2 = {2, {1, 1, 1, 1, 1, 1, 1, 1, 1, 1}, 0};

signed char dir = 1;
signed char guiltyDir = 0;

Player *getPlayer(uint8_t nr)
{
    if(nr == 1) {
        return &player1;
    } else {
        return &player2;
    }
}

uint8_t ICACHE_FLASH_ATTR isAnyPressed(Player *player) {
    uint8_t i;
    for (i = 0; i < 10; i++) {
        if (player->buttons[i] == 0) {
            return i;
        }
    }
    
    return 0;
}

bool ICACHE_FLASH_ATTR isActivePressed(Player *player) {
    if (player->buttons[activeStrip] == 0) {
        return true;
    }
    
    return false;
}

uint8_t ICACHE_FLASH_ATTR isStartCondition()
{
    uint8_t i;
    for (i = 0; i < 10; i++) {
        if (player1.buttons[i] == 0 && player2.buttons[i] == 0) {
            return true;
        }
    }
    
    return 0;
}

void ICACHE_FLASH_ATTR clearButtons()
{
    uint8_t i;
    for (i = 0; i < 10; i++) {
        player1.buttons[i] = player2.buttons[i] = 1;
    }
}

// Prepare player start zone and game variables
void ICACHE_FLASH_ATTR prepareGame(game_mode mode)
{
    isSafe = 0;

    int i;
    for (i = 0; i < LEDS; i++ ) {
        frameBuffer[i*3] = 0x00; 
        frameBuffer[i*3+1] = 0x00;
        frameBuffer[i*3+2] = 0x00;
        if (dir == -1) {
            if (i >= LEDS - SAFEZONE) {
                led = LEDS - 1;
                frameBuffer[i*3] = 0x50;
                frameBuffer[i*3+1] = 0xFF;
            } else if (i == SAFEZONE - 1) {
                frameBuffer[i*3+2] = 0x80;
            }
        } else {
            if (i < SAFEZONE) {
                led = 0;
                frameBuffer[i*3+2] = 0x80;
            } else if (i == LEDS - SAFEZONE) {
                frameBuffer[i*3] = 0x50;
                frameBuffer[i*3+1] = 0xFF;
            }
        }
    }

    ws2812_push(frameBuffer, sizeof(frameBuffer));
    speed = SPEED_START;
    if (mode == PAUSE) {
        os_timer_arm(&pauseTimer, 12, 1);
    }
    gameMode = mode;
}

void ICACHE_FLASH_ATTR allStrips()
{
    gpio_output_set(BIT12, 0, BIT12, 0);
    gpio_output_set(BIT13, 0, BIT13, 0);
    gpio_output_set(BIT14, 0, BIT14, 0);
}

void clearStrips()
{
    if (gameMode == CLEAR || gameMode == CLEAR2) {
        return;
    }
    
    lastGameMode = gameMode;
    gameMode = CLEAR;

    int i;
    for (i = 0; i < LEDS; i++ ) {
        frameBuffer[i*3] = 0x00; 
        frameBuffer[i*3+1] = 0x00;
        frameBuffer[i*3+2] = 0x00;
        if (i == SAFEZONE - 1) {
            frameBuffer[i*3+2] = 0x80;
        } else if (i == (LEDS - SAFEZONE)) {
            frameBuffer[i*3] = 0x50;
            frameBuffer[i*3+1] = 0xFF;
        }
    }

    allStrips();
    ws2812_push(frameBuffer, sizeof(frameBuffer));
}

void ICACHE_FLASH_ATTR switchStrip()
{
    clearStrips();
    activeStrip = nextCaller;
    nextCaller = 0;
    
    os_printf("Strip selected: %d\n", activeStrip);
}

void ICACHE_FLASH_ATTR selectStrip(uint8_t strip)
{
    gpio_output_set(0, BIT12, BIT12, 0);
    gpio_output_set(0, BIT13, BIT13, 0);
    gpio_output_set(0, BIT14, BIT14, 0);

    switch (strip) {
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
}

void ICACHE_FLASH_ATTR incSpeed()
{
    os_timer_disarm(&frameTimer);
    
    if (speed > SPEED_MAX) {
        speed -= SPEED_INC;
    }
    
    clearStrips(); 
    os_timer_arm(&frameTimer, speed, 1);
}

void ICACHE_FLASH_ATTR pongEvent()
{
    sendEvent(SOUND_PONG, 0);
    pongEventSent = 1;
}

void ICACHE_FLASH_ATTR pong(Player *player)
{
    if (player->nr == 1) {
        led = 0;
        dir = 1;
    } else {
        led = LEDS - 1;
        dir = -1;          
    } 
    
    if (nextCaller == 0) {
        nextCaller = activeStrip;
    }
    
    switchStrip();
    if (!pongEventSent) {
        sendEvent(SOUND_PONG, 0);
    }
    
    pongEventSent = 0;
    incSpeed();
    wasSafe = 0;
}

uint8_t ICACHE_FLASH_ATTR getGultyStrip(uint8_t button)
{
    switch(button) {
        case 1:
        case 2:
            return 1;
        case 3:
        case 4:
            return 2;
        case 5:
        case 6:
            return 3;
        default:
            return 0;
    }
}


// System task which will monitor players input
void ICACHE_FLASH_ATTR inputMonitor(os_event_t *events)
{
    #ifdef EMULATE_P1
        player1.button = GPIO_INPUT_GET(0);
    #endif

    if (gameMode == START) {
        if ((dir == 1 && isAnyPressed(&player1)) || dir == -1 && isAnyPressed(&player2)) {
            Player *player;
            if (dir == 1) {
                player = &player1;
            } else {
                player = &player2;
            }
            gameMode = MOVING;
            nextCaller = isAnyPressed(player);
            
            switchStrip();
            clearButtons();
            
            os_timer_arm(&frameTimer, speed, 1);
        }
    }else if(gameMode == MOVING) {
        if (dir == 1) {
            uint8_t tempNext = isAnyPressed(&player2);
           
            if (tempNext) {
                if (isSafe) {
                    bool activePressed = isActivePressed(&player2);
                    clearButtons();
                    
                    if (activePressed){
                        if (nextCaller == 0) {
                            wasSafe = 1;
                            pongEvent();
                        } else {
                            pong(&player2);
                        }
                    } else if (nextCaller == 0) {
                        nextCaller = tempNext;
                        if (wasSafe == 1) {
                            pong(&player2);
                        }
                    }
                } else {
                    os_timer_disarm(&frameTimer);

                    gameMode = SCORE_PHASE1;
                    player1.score++;
                    os_printf("SCORE %d VS %d\n", player1.score, player2.score);
                    sendEvent(SOUND_SCORE, 1);
                    
                    guilty = getGultyStrip(tempNext);
                    guiltyDir = dir;
                    
                    clearStrips();
                    os_timer_arm(&scoreTimer, 2, 1); 
                }
                clearButtons();
            }
        } else {
            uint8_t tempNext = isAnyPressed(&player1);
           
            if (tempNext) {
                if (isSafe) {
                    bool activePressed = isActivePressed(&player1);
                    clearButtons();
                    
                    if (activePressed){
                        if (nextCaller == 0) {
                            wasSafe = 1;
                            pongEvent();
                        } else {
                            pong(&player1);
                        }
                    } else if (nextCaller == 0) {
                        nextCaller = tempNext;
                        if (wasSafe == 1) {
                            pong(&player1);
                        }
                    }                    
                } else {
                    os_timer_disarm(&frameTimer);

                    gameMode = SCORE_PHASE1;
                    player2.score++;

                    os_printf("SCORE %d VS %d\n", player1.score, player2.score);
                    sendEvent(SOUND_SCORE, 2);

                    guilty = getGultyStrip(tempNext);
                    guiltyDir = dir;
                    clearStrips();
                    
                    os_timer_arm(&scoreTimer, 2, 1);
                }
                clearButtons();
            }
        }
    }else if (gameMode == WIN) {
        if (isStartCondition()) {
            os_timer_disarm(&winTimer);
            
            clearButtons();
            player1.score = player2.score = 0;

            sendEvent(SOUND_START, 0);
            
            strobe = 0;
            scoreRepeat = 0;
            prepareGame(PAUSE);      
        }
    }

    system_os_post(0, 0, 0 );
}

// A timer callback used to display fancy score lines
void ICACHE_FLASH_ATTR scoreTimerCallback(void *arg)
{
    if (gameMode == CLEAR) {
        gameMode = CLEAR2;
        
        os_printf("CLEAR 1 Done\n");

        return;
    }

    if (gameMode == CLEAR2) {
        gameMode = lastGameMode;
        selectStrip(guilty);
        if (guiltyDir == 1) {
            led = 0;
        } else {
            led = LEDS - 1;
        }
        
        scoreRepeat = 0;
        os_printf("CLEAR DONE SHOULD ANIMATE, MODE: %d, DIR: %d\n", gameMode, guiltyDir);
    }
    
    int i;
    for (i = 0; i < LEDS; i++) {
        frameBuffer[i*3] = 0x00;
        frameBuffer[i*3+1] = 0x00;
        frameBuffer[i*3+2] = 0x00;
        
        if ((guiltyDir == 1 && i <= led) || (guiltyDir == -1 && i > led)) {
            frameBuffer[i*3+1] = 0xFF;
        }
    }
    ws2812_push(frameBuffer, sizeof(frameBuffer));
    scoreRepeat++;
    led += guiltyDir;
    
    if (scoreRepeat >= LEDS) {
        os_timer_disarm(&scoreTimer);

        scoreRepeat = 0;
        strobe = 0;
        dir = 0 - dir;

        if (player1.score == MAX_SCORE || player2.score == MAX_SCORE) {
            gameMode = WIN;
            speed = SPEED_START;
            os_timer_arm(&winTimer, 40, 1);
        } else {
            prepareGame(START);
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
        selectStrip(activeStrip);

        return;
    }

    if (gameMode == LAST_FRAME) {
        gameMode = SCORE_PHASE1;
        os_timer_disarm(&frameTimer);
        
        if (dir == 1) {
            led = 0;
        } else {
            led = LEDS - 1;
        }
        
        guilty = getGultyStrip(activeStrip);
        guiltyDir = dir;
                    
        clearStrips();
        os_timer_arm(&scoreTimer, 2, 1);

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

            if ((dir == -1 && led < SAFEZONE && i < SAFEZONE) || i == SAFEZONE - 1) {
                if (wasSafe) {
                    frameBuffer[i*3] = 0xFF;
                } else {
                    frameBuffer[i*3+2] = 0x80;
                }
            } else if ((dir == 1 && led >= LEDS - SAFEZONE && i >= LEDS - SAFEZONE) || i == LEDS - SAFEZONE) {
                if (wasSafe) {
                    frameBuffer[i*3] = 0xFF;
                } else {
                    frameBuffer[i*3+1] = 0xFF;
                    frameBuffer[i*3] = 0x50;
                }
            }
        }
    }

    if (dir == 1 && led == LEDS - 1) {
        if (wasSafe) {
            wasSafe = 0;

            pong(&player2);
        } else {
            player1.score++;
            os_printf("SCORE %d VS %d\n", player1.score, player2.score);
            gameMode = LAST_FRAME;
            sendEvent(SOUND_SCORE, 1);
        }

    } else if (dir == -1 && led == 0) {
        if (wasSafe) {
            wasSafe = 0;

            pong(&player1);
        } else {
            player2.score++;
            os_printf("SCORE %d VS %d\n", player1.score, player2.score);
            gameMode = LAST_FRAME;
            sendEvent(SOUND_SCORE, 2);
        }
    }

    led += dir;
    ws2812_push(frameBuffer, sizeof(frameBuffer));
}

void ICACHE_FLASH_ATTR pauseTimerCallback(void *arg)
{
    int i = 0;
    
    for (i = 0; i < LEDS; i++ ) {
        frameBuffer[i*3] = 0x00; 
        frameBuffer[i*3+1] = 255 - pauseCycle;
        frameBuffer[i*3+2] = 0x00;
    }
    ws2812_push(frameBuffer, sizeof(frameBuffer));
    pauseCycle++;
    
    if (pauseCycle >= PAUSE_COUNT) {
        pauseCycle = 0;
        os_timer_disarm (&pauseTimer);
        prepareGame(START);
    }
}

void ICACHE_FLASH_ATTR sendEvent(uint8_t event, uint8_t playerScored)
{
    uint8_t data[7] = {
        CMD_EVENT,
        0,
        player1.score,
        player2.score,
        MAX_SCORE,
        playerScored,
        124,
    };
    
    switch (event) {
        case SOUND_SCORE:
            if (player1.score + player2.score == 1) {
                data[1] = SOUND_SCORE_FIRST;
            } else if(player1.score == MAX_SCORE || player2.score == MAX_SCORE) {
                data[1] = SOUND_VICTORY;
            } else {
                data[1] = SOUND_SCORE;
            }
            break;
        default:
            data[1] = event;
            break;
    }
    
    sint8 status = espconn_send(master, data, 7);
    
    os_printf("Call event %d - status: %d\n", event, status);
}
