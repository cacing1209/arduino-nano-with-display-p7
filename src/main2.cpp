
#include <avr/pgmspace.h>
#include <SPI.h>
#include "HUB08SPI.h"
#include "TimerOne.h"
#include "Buffer.h"

#define WIDTH 64
#define HEIGHT 16
#define BUTTON_PIN A0
#define BUZZER_PIN 3

HUB08SPI display;
uint8_t displaybuf[WIDTH * HEIGHT / 8];
Buffer buff(displaybuf, WIDTH, HEIGHT);

#include "ronnAnimation.h"

enum GameState
{
    IDLE,
    RUNNING,
    STOPPED,
    NONE
};
GameState state = IDLE;

// Timing
unsigned long startMillis = 0;
unsigned long elapsedMillis = 0;
unsigned long stoppedAtMillis = 0;

int lastButtonReading = HIGH;
int buttonState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 25;

bool isWin = false;
bool stoppedDisplayed = false;

const unsigned long AUTO_RESET_MS = 60000UL;

void refreshDisplay();
void drawTime(unsigned long ms);
void drawResult(bool &stopshow_rslt);
void startGame();
void stopGame();
void resetGame();
byte handleButtonPress();
void button();
void refreshDisplay()
{
    button();
    display.scan();
}

void drawTime(unsigned long ms)
{
    unsigned long target = 10000UL;
    unsigned long tolerance = 5UL;
    if (state == STOPPED)
    {
        buff.clear();
        stopGame();
    }
    ronn.setFont(B_7SEGMENT);
    int s = ms / 1000;
    int cs = (ms % 1000) / 10;
    Serial.print("second ");
    Serial.print(s);
    Serial.print(" cs ");
    Serial.println(cs);
    char buf[6];
    sprintf(buf, "%02d:%02d", s, cs);

    int textWidth = strlen(buf) * 8;
    int x = ((WIDTH - textWidth) / 2) - 3;
    int y = 0;
    ronn.printText(buf, x, y);
}
void drawResult_0(bool &c)
{
    if (!c)
        return;
    buff.clear();
    const char *msg = isWin ? "WIN" : "LOSE";
    ronn.setFont(B_STD);
    int charWidth = 6;
    int textWidth = strlen(msg) * charWidth;

    int x = ((WIDTH - textWidth) / 2) - 7;
    int y = 1;
    ronn.printText(msg, x, y);
    c = false;
}

void drawResult_1(bool &c)
{
    if (!c)
        return;
    const char *msg = isWin ? "WIN" : "LOSE";
    ronn.setFont(B_STD);
    int charWidth = 6;
    int textWidth = strlen(msg) * charWidth;
    int x = ((WIDTH - textWidth) / 2) - 7;
    int y = 1;
    const size_t BUF_BYTES = WIDTH * HEIGHT / 8;
    static uint8_t tmp[WIDTH * HEIGHT / 8];
    buff.clear();
    ronn.printText(msg, x, y);
    memcpy(tmp, displaybuf, BUF_BYTES);
    for (size_t i = 0; i < BUF_BYTES; ++i)
        displaybuf[i] = 0xFF;
    for (size_t i = 0; i < BUF_BYTES; ++i)
        displaybuf[i] &= ~tmp[i];
    c = false;
}

void drawResult(bool &stopshow_rslt)
{
    static unsigned long flip = 0;
    unsigned long interval = 1000UL;
    static byte count = 0;
    static bool print_c = false;
    if (count % 2 == 1)
        drawResult_0(print_c);
    else
        drawResult_1(print_c);
    if (millis() - flip > interval)
    {
        print_c = true;
        count++;
        flip = millis();
    }
    if (count > 12)
    {
        count = 0;
        stopshow_rslt = true;
    }
}
void startGame()
{
    startMillis = millis();
    elapsedMillis = 0;
    state = RUNNING;
}

byte count_ls = 0;
bool beep = false;

void buzzer_rt(bool iswin)
{
    static unsigned long flfp = 0;

    if (iswin)
    {
        if (millis() - flfp >= 250UL)
        {
            beep = !beep;
            digitalWrite(BUZZER_PIN, beep ? HIGH : LOW);
            flfp = millis();
        }
    }
    else
    {
        if (count_ls >= 6)
            return;

        if (millis() - flfp >= 1000UL)
        {
            beep = !beep;
            digitalWrite(BUZZER_PIN, beep ? HIGH : LOW);
            flfp = millis();
            count_ls++;
        }
    }
}

void resetGame()
{
    digitalWrite(BUZZER_PIN, LOW);
    count_ls = 0;
    beep = false;

    state = IDLE;
    startMillis = 0;
    elapsedMillis = 0;
    stoppedAtMillis = 0;
    isWin = false;
    buff.clear();
    buff.clear();
    ronn.setFont(B_7SEGMENT);
    drawTime(0);
}
void stopGame()
{
    digitalWrite(BUZZER_PIN, LOW);
    elapsedMillis = millis() - startMillis;
    state = STOPPED;
    stoppedAtMillis = millis();
    stoppedDisplayed = false;

    unsigned long target = 10000UL;
    unsigned long tolerance = 5UL;

    if (elapsedMillis >= target - tolerance && elapsedMillis <= target + tolerance)
    {
        elapsedMillis = target;
        Serial.print("MENANG: ");
        Serial.println(elapsedMillis);
    }

    if (elapsedMillis == target)
        isWin = true;
    else
        isWin = false;
}

byte handleButtonPress()
{
    if (state == IDLE || state == NONE)
    {
        buff.clear();
        startGame();
        return 0x01;
    }
    else if (state == RUNNING)
    {
        stopGame();
        return 0x02;
    }
    else if (state == STOPPED)
    {
        resetGame();
        return 0x03;
    }
}
void button()
{
    int reading = digitalRead(BUTTON_PIN);
    static unsigned long time_ringetone = 0;
    static bool pressed = false;
    static byte role = 0;
    if (reading != lastButtonReading)
        lastDebounceTime = millis();

    if ((millis() - lastDebounceTime) > debounceDelay)
    {
        if (reading != buttonState)
        {
            buttonState = reading;
            if (buttonState == LOW)
            {
                pressed = true;
                role = handleButtonPress();
                digitalWrite(BUZZER_PIN, HIGH);
            }
        }
    }

    if (pressed)
    {
        unsigned long dur = (role == 0x02) ? 2000UL : 200UL;
        if (millis() - time_ringetone >= dur)
        {
            digitalWrite(BUZZER_PIN, LOW);
            pressed = false;
            role = 0;
        }
    }
    else
    {
        time_ringetone = millis();
    }

    lastButtonReading = reading;
}

void setup()
{
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(BUZZER_PIN, OUTPUT);
    attachInterrupt(BUTTON_PIN, button, FALLING);
    display.begin(displaybuf, WIDTH, HEIGHT);
    display.setBrightness(100);
    Timer1.initialize(1000);
    Timer1.attachInterrupt(refreshDisplay);
    buff.clear();
    digitalWrite(BUZZER_PIN, HIGH);
    delay(150);
    digitalWrite(BUZZER_PIN, LOW);
    resetGame();
    Serial.begin(9600);
}

void loop()
{
    static unsigned long report = 0;
    static unsigned long ls = 0;
    static bool show_rslt = false;
    if (state == RUNNING)
    {
        unsigned long now = millis();
        unsigned long newElapsed = now - startMillis;

        if (newElapsed / 10 != elapsedMillis / 10)
        {
            elapsedMillis = newElapsed;
            drawTime(elapsedMillis);
        }
        ls = now;
        show_rslt = true;
    }

    else if (state == IDLE)
    {
        drawTime(0);
        state = NONE;
    }
    else if (state == STOPPED)
    {
        if (!stoppedDisplayed)
        {
            if (millis() - ls < 5000UL)
            {
                if (show_rslt)
                {
                    buff.clear();
                    show_rslt = false;
                    drawTime(elapsedMillis);
                }
            }
            else
            {
                drawResult(stoppedDisplayed);
            }
            buzzer_rt(isWin);
        }

        if ((millis() - stoppedAtMillis) >= AUTO_RESET_MS)
            resetGame();
    }
    button();
}
