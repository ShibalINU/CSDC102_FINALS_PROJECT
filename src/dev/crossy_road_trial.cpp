/*
 * ============================================================
 *  CSDC102 | Intermediate Programming
 *  Final Project: Road Crossing Challenge (Crossy Road - Terminal)
 * ============================================================
 *
 *  HOW TO COMPILE & RUN:
 *    Linux/macOS : g++ -o crossy_road crossy_road.cpp && ./crossy_road
 *    Windows     : g++ -o crossy_road crossy_road.cpp && crossy_road.exe
 *
 *  CONTROLS:
 *    Arrow Keys  — Move the player (UP / DOWN / LEFT / RIGHT)
 *    Q           — Quit at any time
 *
 *  PLATFORM NOTES:
 *    - Arrow-key reading uses platform-specific code.
 *    - On Windows  : _kbhit() / _getch() from <conio.h>
 *    - On Linux/Mac: raw-mode terminal via termios
 *    - Sleep uses   Windows Sleep(ms) or POSIX usleep(us)
 *
 *  PERFORMANCE NOTES (v2):
 *    - Render target: 33 ms per frame (~30 FPS)
 *    - Obstacle movement is decoupled from render rate via a
 *      separate tick counter so speed feels consistent.
 *    - Screen is redrawn using ANSI cursor-home (\033[H) instead
 *      of system("clear") to eliminate flicker.
 * ============================================================
 */

// ─── Standard Headers ────────────────────────────────────────────────────────
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <chrono> // high_resolution_clock for frame timing

// ─── Platform-Specific Headers ───────────────────────────────────────────────
#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#define SLEEP_MS(ms) Sleep(ms)
// On Windows we use ANSI via the virtual terminal flag set in main()
#define CURSOR_HOME "\033[H"
#define HIDE_CURSOR "\033[?25l"
#define SHOW_CURSOR "\033[?25h"
#define CLEAR_FULL "\033[2J\033[H"
#else
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#define SLEEP_MS(ms) usleep((ms) * 1000)
#define CURSOR_HOME "\033[H"
#define HIDE_CURSOR "\033[?25l"
#define SHOW_CURSOR "\033[?25h"
#define CLEAR_FULL "\033[2J\033[H"
#endif

using namespace std;
using namespace std::chrono;

// ─── Constants ───────────────────────────────────────────────────────────────
const int FIELD_WIDTH = 40;
const int TOTAL_WIDTH = 42;
const int TOTAL_ROWS = 20;
const int LIVES_START = 3;
const int WIN_CROSSINGS = 5;
const string LB_FILE = "leaderboard.txt";

// Render period: 33 ms ≈ 30 FPS
const int FRAME_MS = 33;

// Zone identifiers
const int ZONE_FINISH = 0;
const int ZONE_ROAD = 1;
const int ZONE_BUFFER = 2;
const int ZONE_RIVER = 3;
const int ZONE_START = 4;

const int zoneMap[TOTAL_ROWS] = {
    ZONE_FINISH, // 0
    ZONE_ROAD,   // 1
    ZONE_ROAD,   // 2
    ZONE_ROAD,   // 3
    ZONE_ROAD,   // 4
    ZONE_ROAD,   // 5
    ZONE_BUFFER, // 6
    ZONE_RIVER,  // 7
    ZONE_RIVER,  // 8
    ZONE_BUFFER, // 9
    ZONE_ROAD,   // 10
    ZONE_ROAD,   // 11
    ZONE_ROAD,   // 12
    ZONE_ROAD,   // 13
    ZONE_ROAD,   // 14
    ZONE_BUFFER, // 15
    ZONE_RIVER,  // 16
    ZONE_RIVER,  // 17
    ZONE_BUFFER, // 18
    ZONE_START   // 19
};

// ─── Linked-List Node ─────────────────────────────────────────────────────────
struct Node
{
    string data;
    Node *next;
};
typedef Node *NodePtr;

// ─── Leaderboard Entry ───────────────────────────────────────────────────────
struct LeaderboardEntry
{
    string name;
    int score;
};

// ═══════════════════════════════════════════════════════════════════════════════
//  SECTION A — Platform Input Helpers
// ═══════════════════════════════════════════════════════════════════════════════

#ifndef _WIN32
static struct termios orig_termios;

void enableRawMode()
{
    tcgetattr(STDIN_FILENO, &orig_termios);
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ICANON | ECHO);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void disableRawMode()
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

bool kbhit()
{
    int ch = getchar();
    if (ch != EOF)
    {
        ungetc(ch, stdin);
        return true;
    }
    return false;
}

int getch() { return getchar(); }
#endif

// ═══════════════════════════════════════════════════════════════════════════════
//  SECTION B — Lane String Helpers
// ═══════════════════════════════════════════════════════════════════════════════

void shiftLeft(string &lane)
{
    char first = lane[1];
    for (int i = 1; i < FIELD_WIDTH; i++)
        lane[i] = lane[i + 1];
    lane[FIELD_WIDTH] = first;
}

void shiftRight(string &lane)
{
    char last = lane[FIELD_WIDTH];
    for (int i = FIELD_WIDTH; i > 1; i--)
        lane[i] = lane[i - 1];
    lane[1] = last;
}

string generateRoadLane(int laneNum, int numTrucks)
{
    string lane(TOTAL_WIDTH, '.');
    lane[0] = '|';
    lane[41] = '|';
    int spacing = FIELD_WIDTH / (numTrucks + 1);
    for (int t = 0; t < numTrucks; t++)
    {
        int pos = 1 + spacing * (t + 1) - 2 + (rand() % 5);
        if (pos < 1)
            pos = 1;
        if (pos + 5 > FIELD_WIDTH)
            pos = FIELD_WIDTH - 4;
        for (int k = 0; k < 5 && (pos + k) <= FIELD_WIDTH; k++)
            lane[pos + k] = '#';
    }
    return lane;
}

string generateRiverLane(int laneNum, int numLogs)
{
    string lane(TOTAL_WIDTH, '~');
    lane[0] = '|';
    lane[41] = '|';
    int spacing = FIELD_WIDTH / (numLogs + 1);
    for (int l = 0; l < numLogs; l++)
    {
        int pos = 1 + spacing * (l + 1) - 2 + (rand() % 5);
        if (pos < 1)
            pos = 1;
        if (pos + 4 > FIELD_WIDTH)
            pos = FIELD_WIDTH - 3;
        for (int k = 0; k < 4 && (pos + k) <= FIELD_WIDTH; k++)
            lane[pos + k] = '=';
    }
    return lane;
}

string generateBufferLane()
{
    string lane(TOTAL_WIDTH, '.');
    lane[0] = '|';
    lane[41] = '|';
    return lane;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  SECTION C — Linked-List Operations
// ═══════════════════════════════════════════════════════════════════════════════

NodePtr getLane(NodePtr head, int index)
{
    NodePtr cur = head;
    for (int i = 0; i < index && cur != nullptr; i++)
        cur = cur->next;
    return cur;
}

NodePtr buildRoad(int numTrucks, int numLogs)
{
    NodePtr head = nullptr, tail = nullptr;
    int roadCount = 0, riverCount = 0;

    for (int row = 0; row < TOTAL_ROWS; row++)
    {
        NodePtr newNode = new Node();
        newNode->next = nullptr;

        switch (zoneMap[row])
        {
        case ZONE_FINISH:
            newNode->data = "|========================================|";
            break;
        case ZONE_ROAD:
            newNode->data = generateRoadLane(++roadCount, numTrucks);
            break;
        case ZONE_RIVER:
            newNode->data = generateRiverLane(++riverCount, numLogs);
            break;
        case ZONE_BUFFER:
            newNode->data = generateBufferLane();
            break;
        case ZONE_START:
        default:
            newNode->data = "|                                        |";
            break;
        }

        if (!head)
        {
            head = tail = newNode;
        }
        else
        {
            tail->next = newNode;
            tail = newNode;
        }
    }
    return head;
}

/*
 * shiftObstacles — now takes a per-lane direction array so each lane
 * can have its own speed multiplier in the future.  For now behaviour
 * is identical to v1 (alternating L/R), but the function is called
 * only when the obstacle tick fires, not every render frame.
 */
void shiftObstacles(NodePtr head)
{
    NodePtr cur = head;
    int roadIdx = 0, riverIdx = 0;

    for (int row = 0; row < TOTAL_ROWS && cur != nullptr; row++)
    {
        if (zoneMap[row] == ZONE_ROAD)
        {
            roadIdx++;
            (roadIdx % 2 == 1) ? shiftRight(cur->data) : shiftLeft(cur->data);
        }
        else if (zoneMap[row] == ZONE_RIVER)
        {
            riverIdx++;
            (riverIdx % 2 == 1) ? shiftRight(cur->data) : shiftLeft(cur->data);
        }
        cur = cur->next;
    }
}

void freeList(NodePtr &head)
{
    while (head)
    {
        NodePtr t = head;
        head = head->next;
        delete t;
    }
    head = nullptr;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  SECTION D — Display  (flicker-free via ANSI cursor-home)
// ═══════════════════════════════════════════════════════════════════════════════

/*
 * displayRoad — builds the entire frame into a single string buffer and
 * writes it in one cout call.  Combined with CURSOR_HOME (moves cursor
 * to top-left without erasing) this replaces every character in-place,
 * producing a smooth update instead of a flash.
 *
 * A trailing space-pad on every line ensures any leftover characters
 * from a longer previous frame are overwritten.
 */
void displayRoad(NodePtr head,
                 int playerX, int playerY,
                 const string &playerName,
                 int lives, int crossings)
{
    // Build entire frame in one string to minimise I/O syscalls
    string frame;
    frame.reserve(2048);

    frame += CURSOR_HOME; // jump to top-left, no erase flash

    frame += "---------- Road Crossing Challenge ----------\n";
    frame += "Player: ";
    frame += playerName;
    frame += " | Lives: ";
    frame += to_string(lives);
    frame += " | Crossings: ";
    frame += to_string(crossings);
    frame += "   \n"; // trailing spaces clear old wider numbers

    NodePtr cur = head;
    for (int row = 0; row < TOTAL_ROWS && cur != nullptr; row++)
    {
        if (row == playerY)
        {
            char saved = cur->data[playerX];
            cur->data[playerX] = 'P';
            frame += cur->data;
            cur->data[playerX] = saved;
        }
        else
        {
            frame += cur->data;
        }
        frame += '\n';
        cur = cur->next;
    }

    frame += "(Arrow keys: move | Q: quit)   \n";

    cout << frame;
    cout.flush();
}

// ═══════════════════════════════════════════════════════════════════════════════
//  SECTION E — Collision & Movement Logic
// ═══════════════════════════════════════════════════════════════════════════════

bool checkCollision(NodePtr head, int playerX, int playerY)
{
    if (zoneMap[playerY] != ZONE_ROAD)
        return false;
    NodePtr lane = getLane(head, playerY);
    return lane && lane->data[playerX] == '#';
}

bool checkDrowned(NodePtr head, int playerX, int playerY)
{
    if (zoneMap[playerY] != ZONE_RIVER)
        return false;
    NodePtr lane = getLane(head, playerY);
    return lane && lane->data[playerX] == '~';
}

void updatePlayerWithLog(NodePtr head, int &playerX, int playerY)
{
    if (zoneMap[playerY] != ZONE_RIVER)
        return;
    int riverIdx = 0;
    for (int r = 0; r <= playerY; r++)
        if (zoneMap[r] == ZONE_RIVER)
            riverIdx++;
    (riverIdx % 2 == 1) ? playerX++ : playerX--;
    if (playerX < 1)
        playerX = 1;
    if (playerX > FIELD_WIDTH)
        playerX = FIELD_WIDTH;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  SECTION F — Keyboard Input
// ═══════════════════════════════════════════════════════════════════════════════

/*
 * keyboardInput — now drains ALL pending key events each frame so that
 * fast typists don't build up an input queue that makes the player feel
 * laggy.  Only the LAST direction key read takes effect.
 */
void keyboardInput(int &playerX, int &playerY, bool &quitFlag)
{
#ifdef _WIN32
    while (_kbhit())
    {
        int ch = _getch();
        if (ch == 0 || ch == 0xE0)
        {
            ch = _getch();
            switch (ch)
            {
            case 72:
                if (playerY > 0)
                    playerY--;
                break;
            case 80:
                if (playerY < TOTAL_ROWS - 1)
                    playerY++;
                break;
            case 75:
                if (playerX > 1)
                    playerX--;
                break;
            case 77:
                if (playerX < FIELD_WIDTH)
                    playerX++;
                break;
            }
        }
        else if (ch == 'q' || ch == 'Q')
        {
            quitFlag = true;
        }
    }
#else
    while (kbhit())
    {
        int ch = getch();
        if (ch == 27)
        {
            int ch2 = getch();
            if (ch2 == '[')
            {
                int ch3 = getch();
                switch (ch3)
                {
                case 'A':
                    if (playerY > 0)
                        playerY--;
                    break;
                case 'B':
                    if (playerY < TOTAL_ROWS - 1)
                        playerY++;
                    break;
                case 'D':
                    if (playerX > 1)
                        playerX--;
                    break;
                case 'C':
                    if (playerX < FIELD_WIDTH)
                        playerX++;
                    break;
                }
            }
        }
        else if (ch == 'q' || ch == 'Q')
        {
            quitFlag = true;
        }
    }
#endif
}

// ═══════════════════════════════════════════════════════════════════════════════
//  SECTION G — Leaderboard (File I/O)
// ═══════════════════════════════════════════════════════════════════════════════

void saveScore(const string &name, int score)
{
    ofstream file(LB_FILE, ios::app);
    if (file.is_open())
    {
        file << name << " " << score << "\n";
    }
}

void showLeaderboard()
{
    vector<LeaderboardEntry> entries;
    ifstream file(LB_FILE);
    if (file.is_open())
    {
        string name;
        int score;
        while (file >> name >> score)
            entries.push_back({name, score});
    }
    sort(entries.begin(), entries.end(),
         [](const LeaderboardEntry &a, const LeaderboardEntry &b)
         { return a.score > b.score; });

    cout << "\n===== LEADERBOARD (Top 5) =====\n";
    int limit = min((int)entries.size(), 5);
    for (int i = 0; i < limit; i++)
        cout << " " << (i + 1) << ". " << left << setw(20)
             << entries[i].name << " — " << entries[i].score << " crossing(s)\n";
    if (entries.empty())
        cout << " (No entries yet)\n";
    cout << "================================\n";
}

// ═══════════════════════════════════════════════════════════════════════════════
//  SECTION H — Title Screen, Player Setup, Difficulty
// ═══════════════════════════════════════════════════════════════════════════════

void showTitleScreen()
{
    cout << CLEAR_FULL;
    cout << R"(
  ============================================
   ____                 _    ____               _
  |  _ \ ___   __ _  __| |  / ___|_ __ ___  ___(_)_ __   __ _
  | |_) / _ \ / _` |/ _` | | |   | '__/ _ \/ __| | '_ \ / _` |
  |  _ < (_) | (_| | (_| | | |___| | | (_) \__ \ | | | | (_| |
  |_| \_\___/ \__,_|\__,_|  \____|_|  \___/|___/_|_| |_|\__, |
                              C h a l l e n g e          |___/
  ============================================
   A terminal Crossy Road game  |  CSDC102 C++
  ============================================

  HOW TO PLAY:
    Arrow Keys : UP / DOWN / LEFT / RIGHT
    Q          : Quit

    Dodge trucks  (#####) in the ROAD zones.
    Ride logs     (====)  in the RIVER zones.
    Rest on dots  (....)  — Buffer lanes are safe!
    Reach the TOP 5 times to WIN!

  Press ENTER to continue...
)";
    cin.ignore(1000, '\n');
    cin.get();
}

string getPlayerName()
{
    string name;
    cout << "\n Enter your name: ";
    cin >> name;
    cin.ignore(1000, '\n');
    return name;
}

/*
 * chooseDifficulty
 *
 * gameSpeed is now the obstacle tick interval in milliseconds — how often
 * obstacles actually move, independent of the 33 ms render frame.
 *
 *   Easy : obstacles shift every 180 ms  (~5.5 steps/sec)
 *   Hard : obstacles shift every  90 ms  (~11  steps/sec)
 *
 * The render loop always runs at ~30 FPS regardless of difficulty.
 */
void chooseDifficulty(int &obstacleTickMs, int &numTrucks, int &numLogs)
{
    cout << "\n SELECT DIFFICULTY\n";
    cout << "  1. Easy — obstacles move every 180 ms, 2 trucks/logs per lane\n";
    cout << "  2. Hard — obstacles move every  90 ms, 3 trucks/logs per lane\n";
    cout << "  Enter choice (1 or 2): ";

    int choice;
    cin >> choice;
    cin.ignore(1000, '\n');

    if (choice == 2)
    {
        obstacleTickMs = 90;
        numTrucks = 3;
        numLogs = 3;
    }
    else
    {
        obstacleTickMs = 180;
        numTrucks = 2;
        numLogs = 2;
    }
}

void gameStatus(int lives, int crossings, const string &playerName)
{
    if (lives <= 0)
        cout << "\n Game Over, " << playerName << "! You were hit too many times.\n";
    else if (crossings >= WIN_CROSSINGS)
        cout << "\n Congratulations, " << playerName
             << "! You crossed " << WIN_CROSSINGS << " times and WON!\n";
}

// ═══════════════════════════════════════════════════════════════════════════════
//  SECTION I — Main Game Loop  (decoupled render vs obstacle tick)
// ═══════════════════════════════════════════════════════════════════════════════

/*
 * runGame
 *
 * Timing model (v2):
 *   - Frame budget : FRAME_MS (33 ms) — measured with high_resolution_clock.
 *   - Obstacle tick: obstacleTickMs — accumulated; obstacles shift only when
 *                    the accumulated elapsed time exceeds the tick interval.
 *   - Log drift    : synced to obstacle ticks so the player drifts exactly
 *                    once per obstacle step (unchanged behaviour, no extra drift).
 *
 * This means on a fast machine the loop fires at a capped 30 FPS, and
 * obstacle speed is governed purely by obstacleTickMs, not frame rate.
 */
void runGame(const string &playerName,
             int obstacleTickMs, int numTrucks, int numLogs)
{
    srand((unsigned)time(nullptr));

    int lives = LIVES_START;
    int crossings = 0;
    int playerX = 20;
    int playerY = TOTAL_ROWS - 1;
    bool quitFlag = false;

    NodePtr head = buildRoad(numTrucks, numLogs);

#ifndef _WIN32
    enableRawMode();
#endif

    cout << HIDE_CURSOR; // hide blinking cursor during gameplay

    // Timing state
    auto lastFrame = high_resolution_clock::now();
    int obstacleAccMs = 0; // accumulated ms since last obstacle step

    // ── Main Game Loop ────────────────────────────────────────────────────────
    while (lives > 0 && crossings < WIN_CROSSINGS && !quitFlag)
    {
        auto now = high_resolution_clock::now();
        int elapsed = (int)duration_cast<milliseconds>(now - lastFrame).count();
        lastFrame = now;

        // ── Obstacle tick ─────────────────────────────────────────────────
        obstacleAccMs += elapsed;
        bool obstacleMoved = false;
        while (obstacleAccMs >= obstacleTickMs)
        {
            obstacleAccMs -= obstacleTickMs;
            shiftObstacles(head);
            obstacleMoved = true;
        }

        // ── Drift player with log (once per obstacle tick, same cadence) ──
        if (obstacleMoved)
            updatePlayerWithLog(head, playerX, playerY);

        // ── Read all pending keys ─────────────────────────────────────────
        keyboardInput(playerX, playerY, quitFlag);

        // ── Win condition ─────────────────────────────────────────────────
        if (playerY == 0)
        {
            crossings++;
            if (crossings < WIN_CROSSINGS)
            {
                freeList(head);
                head = buildRoad(numTrucks, numLogs);
                playerX = 20;
                playerY = TOTAL_ROWS - 1;
            }
        }
        // ── Truck collision ───────────────────────────────────────────────
        else if (checkCollision(head, playerX, playerY))
        {
            lives--;
            playerX = 20;
            playerY = TOTAL_ROWS - 1;
        }
        // ── Drowning ──────────────────────────────────────────────────────
        else if (checkDrowned(head, playerX, playerY))
        {
            lives--;
            playerX = 20;
            playerY = TOTAL_ROWS - 1;
        }

        // ── Render ────────────────────────────────────────────────────────
        displayRoad(head, playerX, playerY, playerName, lives, crossings);

        // ── Frame cap: sleep remaining budget ─────────────────────────────
        auto renderDone = high_resolution_clock::now();
        int renderMs = (int)duration_cast<milliseconds>(renderDone - lastFrame).count();
        int sleepMs = FRAME_MS - renderMs - elapsed;
        if (sleepMs > 0)
            SLEEP_MS(sleepMs);
    }

    cout << SHOW_CURSOR; // restore cursor

#ifndef _WIN32
    disableRawMode();
#endif

    // ── End-of-Game ──────────────────────────────────────────────────────────
    gameStatus(lives, crossings, playerName);
    saveScore(playerName, crossings);
    showLeaderboard();
    freeList(head);
}

// ═══════════════════════════════════════════════════════════════════════════════
//  SECTION J — Entry Point
// ═══════════════════════════════════════════════════════════════════════════════

int main()
{
#ifdef _WIN32
    // Enable ANSI escape sequences on Windows 10+ console
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    SetConsoleMode(hOut, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#endif

    showTitleScreen();

    string playerName = getPlayerName();

    char playAgain = 'y';
    while (playAgain == 'y' || playAgain == 'Y')
    {
        int obstacleTickMs, numTrucks, numLogs;
        chooseDifficulty(obstacleTickMs, numTrucks, numLogs);
        runGame(playerName, obstacleTickMs, numTrucks, numLogs);

        cout << "\n Play again? (y/n): ";
        cin >> playAgain;
        cin.ignore(1000, '\n');
    }

    cout << "\n Thanks for playing, " << playerName << "! See you next time.\n\n";
    return 0;
}