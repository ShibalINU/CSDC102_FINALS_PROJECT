#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <cstdlib>

#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#define CLEAR "cls"
#define SLEEP(ms) Sleep(ms)
#else
#include <termios.h>
#include <unistd.h>
#define CLEAR "clear"
#define SLEEP(ms) usleep((ms) * 1000)
#endif

using namespace std;

// ─── ANSI COLOR CODES ───────────────────────────────────────────────────────
const string RESET = "\033[0m";
const string BOLD = "\033[1m";
const string DIM = "\033[2m";

// Foreground
const string FG_YELLOW = "\033[38;5;220m";
const string FG_GOLD = "\033[38;5;178m";
const string FG_ORANGE = "\033[38;5;208m";
const string FG_GREEN = "\033[38;5;82m";
const string FG_TEAL = "\033[38;5;51m";
const string FG_CYAN = "\033[38;5;87m";
const string FG_WHITE = "\033[38;5;255m";
const string FG_LGRAY = "\033[38;5;250m";
const string FG_DGRAY = "\033[38;5;240m";
const string FG_RED = "\033[38;5;203m";
const string FG_PINK = "\033[38;5;213m";
const string FG_LBLUE = "\033[38;5;117m";

// Background
const string BG_BLACK = "\033[48;5;232m";
const string BG_DBLUE = "\033[48;5;17m";

// ─── CURSOR CONTROL ─────────────────────────────────────────────────────────
void moveCursor(int row, int col)
{
    cout << "\033[" << row << ";" << col << "H";
}
void hideCursor() { cout << "\033[?25l"; }
void showCursor() { cout << "\033[?25h"; }
void clearScreen() { system(CLEAR); }

// ─── UTILITY ────────────────────────────────────────────────────────────────
void sleep_ms(int ms)
{
    this_thread::sleep_for(chrono::milliseconds(ms));
}

// ─── BORDER SWEEP ANIMATION ─────────────────────────────────────────────────
// Draws the outer border character by character for a "drawing in" effect
void animateBorderSweep()
{
    const int W = 70; // total width
    const int H = 24; // total height

    // Top border sweep left→right
    moveCursor(1, 1);
    cout << FG_TEAL << BOLD;
    for (int i = 0; i < W; i++)
    {
        cout << "═";
        cout.flush();
        sleep_ms(8);
    }

    // Right border top→bottom
    for (int r = 2; r <= H; r++)
    {
        moveCursor(r, W);
        cout << "║";
        cout.flush();
        sleep_ms(12);
    }

    // Bottom border right→left
    moveCursor(H + 1, W);
    for (int i = W; i >= 1; i--)
    {
        moveCursor(H + 1, i);
        cout << "═";
        cout.flush();
        sleep_ms(8);
    }

    // Left border bottom→top
    for (int r = H; r >= 2; r--)
    {
        moveCursor(r, 1);
        cout << "║";
        cout.flush();
        sleep_ms(12);
    }

    // Corners
    moveCursor(1, 1);
    cout << "╔";
    moveCursor(1, W);
    cout << "╗";
    moveCursor(H + 1, 1);
    cout << "╚";
    moveCursor(H + 1, W);
    cout << "╝";
    cout << RESET;
    cout.flush();
}

// ─── TITLE FADE-IN ──────────────────────────────────────────────────────────
void printTitle()
{
    // ASCII art lines for "ROAD"
    vector<string> road = {
        "            ____     ___       _      ____  ",
        "           |  _ \\   / _ \\     / \\    |  _ \\ ",
        "           | |_) | | | | |   / _ \\   | | | |",
        "           |  _ <  | |_| |  / ___ \\  | |_| |",
        "           |_| \\_\\  \\___/  /_/   \\_\\ |____/ "};

    // ASCII art lines for "CROSSING"  (C-R-O-S-S-I-N-G)
    vector<string> crossing = {
        "  _____   _____    ____   _____  _____  ___  _   _   _____ ",
        " / ____| |  __ \\  / __ \\ / ____|/ ____||_ _|| \\ | | / ____|",
        "| |      | |__) || |  | |\\___  \\\\___  \\ | | |  \\| || |  __ ",
        "| |____  |  _  / | |__| | ___) | ___) | | | | |\\  || |_|  | ",
        " \\_____| |_| \\_\\  \\____/ |_____/|_____/|___||_| \\_| \\_____|"};

    // Print "ROAD" in golden yellow, starting row 4
    for (int i = 0; i < (int)road.size(); i++)
    {
        moveCursor(4 + i, 5);
        if (i == 0 || i == 4)
            cout << FG_GOLD << BOLD;
        else if (i == 2)
            cout << FG_YELLOW << BOLD;
        else
            cout << FG_ORANGE << BOLD;
        cout << road[i] << RESET;
        cout.flush();
        sleep_ms(60);
    }

    // Print "CROSSING" in green, starting row 10
    for (int i = 0; i < (int)crossing.size(); i++)
    {
        moveCursor(10 + i, 3);
        if (i == 0 || i == 4)
            cout << FG_GREEN << BOLD;
        else if (i == 2)
            cout << FG_TEAL << BOLD;
        else
            cout << "\033[38;5;48m" << BOLD;
        cout << crossing[i] << RESET;
        cout.flush();
        sleep_ms(60);
    }

    // Subtitle
    moveCursor(16, 20);
    cout << FG_LGRAY << DIM << "~  C + +   T E R M I N A L   G A M E  ~" << RESET;
    cout.flush();
    sleep_ms(200);

    // Decorative divider
    moveCursor(17, 3);
    cout << FG_DGRAY;
    for (int i = 0; i < 64; i++)
        cout << "─";
    cout << RESET;
    cout.flush();
}

// ─── CHICKEN BOUNCE ANIMATION ───────────────────────────────────────────────
//   The chicken hops along the bottom of the title box
//   Frames: normal, mid-jump (raised), normal, squish
const vector<string> CHICKEN_FRAMES = {
    // Frame 0 – standing
    " (>'-')>",
    // Frame 1 – mid jump (rotated)
    "  ^('-')^",
    // Frame 2 – standing again
    " (>'-')>",
    // Frame 3 – squish land
    " (v'-')v"};
const vector<int> CHICKEN_ROWS = {0, -1, 0, 1}; // row offset per frame

void animateChicken(int passes = 2)
{
    const int BASE_ROW = 20;
    const int START_COL = 4;
    const int END_COL = 58;
    const int FRAME_COUNT = 4;

    for (int p = 0; p < passes; p++)
    {
        for (int col = START_COL; col <= END_COL; col += 2)
        {
            int frame = (col / 2) % FRAME_COUNT;
            int row = BASE_ROW + CHICKEN_ROWS[frame];

            // Erase previous
            moveCursor(BASE_ROW - 1, col - 2);
            cout << "       ";
            moveCursor(BASE_ROW, col - 2);
            cout << "       ";
            moveCursor(BASE_ROW + 1, col - 2);
            cout << "       ";

            // Draw chicken
            moveCursor(row, col);
            cout << FG_YELLOW << BOLD << CHICKEN_FRAMES[frame] << RESET;

            // Little dust puffs on landing frame
            if (frame == 3)
            {
                moveCursor(BASE_ROW + 1, col + 1);
                cout << FG_LGRAY << "* *" << RESET;
            }

            cout.flush();
            sleep_ms(55);
        }

        // Erase chicken at end of pass
        for (int r = BASE_ROW - 1; r <= BASE_ROW + 1; r++)
        {
            moveCursor(r, END_COL - 2);
            cout << "           ";
        }
        cout.flush();

        if (p < passes - 1)
            sleep_ms(120);
    }
}

// ─── BLINKING PROMPT ────────────────────────────────────────────────────────
void blinkingPrompt()
{
    const int ROW = 23;
    const int COL = 18;
    const string msg = "[ Press ENTER to Start ]";

    for (int blink = 0; blink < 6; blink++)
    {
        moveCursor(ROW, COL);
        if (blink % 2 == 0)
            cout << FG_CYAN << BOLD << msg << RESET;
        else
            cout << FG_DGRAY << DIM << msg << RESET;
        cout.flush();
        sleep_ms(420);
    }

    // Leave it on
    moveCursor(ROW, COL);
    cout << FG_CYAN << BOLD << msg << RESET;
    cout.flush();
}

// ─── CREDITS & HOW TO PLAY ──────────────────────────────────────────────────
void printInfo()
{
    // How-to-play panel (right side, rows 18-22)
    moveCursor(18, 4);
    cout << FG_WHITE << BOLD << "HOW TO PLAY" << RESET;
    moveCursor(19, 4);
    cout << FG_LGRAY << "Arrow Keys" << FG_DGRAY << " ── Move your chicken" << RESET;
    moveCursor(20, 4);
    cout << FG_RED << "#####" << FG_DGRAY << "  ── Dodge trucks!" << RESET;
    moveCursor(21, 4);
    cout << FG_LBLUE << "=====" << FG_DGRAY << "  ── Hop on logs" << RESET;
    moveCursor(22, 4);
    cout << FG_GREEN << "x5 crossings" << FG_DGRAY << " ── Win the game!" << RESET;

    // Credits
    moveCursor(18, 42);
    cout << FG_PINK << BOLD << "CSDC102 Project" << RESET;
    moveCursor(19, 42);
    cout << FG_DGRAY << "Language : " << FG_TEAL << "C++" << RESET;
    moveCursor(20, 42);
    cout << FG_DGRAY << "Engine   : " << FG_TEAL << "Terminal / ANSI" << RESET;
    moveCursor(21, 42);
    cout << FG_DGRAY << "Platform : " << FG_TEAL << "Windows / Linux" << RESET;

    cout.flush();
}

// ─── WAIT FOR ENTER ─────────────────────────────────────────────────────────
void waitForEnter()
{
    // Flush any leftover input
    cin.sync();
    while (cin.get() != '\n')
    {
    }
}

// ─── MAIN TITLE SCREEN ──────────────────────────────────────────────────────
void titleScreen()
{
    hideCursor();
    clearScreen();

    // 1. Sweep the border in
    animateBorderSweep();
    sleep_ms(100);

    // 2. Fade-in ASCII title
    printTitle();
    sleep_ms(150);

    // 3. Print info panel
    printInfo();
    sleep_ms(100);

    // 4. Chicken runs across once
    // animateChicken(1);
    // sleep_ms(150);

    // 5. Blink the prompt
    blinkingPrompt();

    // 6. Wait for ENTER
    moveCursor(25, 1); // move cursor out of the way
    showCursor();
    waitForEnter();

    // 7. "Press start" flash effect
    hideCursor();
    clearScreen();
    for (int f = 0; f < 3; f++)
    {
        moveCursor(12, 20);
        cout << FG_GREEN << BOLD << "*** GAME STARTING ***" << RESET;
        cout.flush();
        sleep_ms(220);
        moveCursor(12, 20);
        cout << "                     ";
        cout.flush();
        sleep_ms(160);
    }
    sleep_ms(300);
    clearScreen();
    showCursor();
}

// ─── MAIN ────────────────────────────────────────────────────────────────────
int main()
{
    titleScreen();
    cout << FG_GREEN << "\nGame starting — good luck!\n"
         << RESET;
    return 0;
}
