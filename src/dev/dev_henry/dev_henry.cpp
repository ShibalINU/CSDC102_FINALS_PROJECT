#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <algorithm>

#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

#ifdef _WIN32
#include <conio.h>
#include <fcntl.h>
#include <io.h>
#define CLEAR "cls"
void sleep_ms(int ms) { Sleep(ms); }
void initConsole()
{
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(hOut, &mode);
    SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    _setmode(_fileno(stdout), _O_BINARY);
}
#else
#include <unistd.h>
#define CLEAR "clear"
void sleep_ms(int ms) { usleep(ms * 1000); }
void initConsole() {}
#endif

using namespace std;

// ─── ANSI COLOR CODES ───────────────────────────────────────────────────────
const string RESET = "\033[0m";
const string BOLD = "\033[1m";
const string DIM = "\033[2m";
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
const string FG_BROWN = "\033[38;5;94m";

// Background flash colors for damage indicator
const string BG_RED = "\033[48;5;160m";
const string BG_ORANGE = "\033[48;5;202m";
const string BG_TEAL = "\033[48;5;30m";
const string BG_GREEN = "\033[48;5;22m";

// ─── CURSOR CONTROL ─────────────────────────────────────────────────────────
void moveCursor(int row, int col) { cout << "\033[" << row << ";" << col << "H"; }
void hideCursor() { cout << "\033[?25l"; }
void showCursor() { cout << "\033[?25h"; }

void clearScreen()
{
    cout << "\033[H\033[J";
    cout.flush();
}

// ============================================================================
//  DOUBLE-BUFFER RENDERER
// ============================================================================
struct DoubleBuffer
{
    vector<string> front;
    vector<string> back;
    string pending;

    void init(int rows)
    {
        front.assign(rows, "");
        back.assign(rows, "");
        pending.reserve(1 << 16);
    }

    void beginFrame()
    {
        for (auto &s : back)
            s.clear();
        pending.clear();
    }

    void writeLine(int row, const string &s)
    {
        if (row >= 0 && row < (int)back.size())
            back[row] += s;
    }

    void endFrame()
    {
        for (int r = 0; r < (int)back.size(); r++)
        {
            if (back[r] != front[r])
            {
                char movbuf[24];
                int n = snprintf(movbuf, sizeof(movbuf), "\033[%d;1H", r + 1);
                pending.append(movbuf, n);
                pending += back[r];
                pending += "\033[K";
                front[r] = back[r];
            }
        }
        if (!pending.empty())
        {
            fwrite(pending.data(), 1, pending.size(), stdout);
            fflush(stdout);
        }
    }

    void invalidate()
    {
        for (auto &s : front)
            s.clear();
    }
};

static DoubleBuffer g_buf;
static const int BUF_ROWS = 35; // extra rows for damage UI

// ============================================================================
//  SOUND EFFECTS
// ============================================================================
// Non-blocking one-shot SFX — SND_NOSTOP prevents killing the BGM channel.
// winmm supports two channels: looping BGM + one-shot SFX.
void playSFX(const char *file)
{
    PlaySound(file, NULL, SND_FILENAME | SND_ASYNC | SND_NOSTOP);
}

// ============================================================================
//  DAMAGE FLASH STATE
// ============================================================================
static bool g_flashActive = false;
static int g_flashFrames = 0;
static string g_flashMsg = "";
static string g_flashFg = FG_WHITE;
static string g_flashBg = BG_RED;
static string g_flashIcon = "";
const int FLASH_DURATION = 12; // ~400ms at 33ms/frame

// Hearts display — track previous lives to animate heart loss
static int g_prevLives = 3;

void triggerFlash(const string &msg, const string &fg, const string &bg,
                  const string &icon, const char *sfxFile)
{
    g_flashMsg = msg;
    g_flashFg = fg;
    g_flashBg = bg;
    g_flashIcon = icon;
    g_flashFrames = FLASH_DURATION;
    g_flashActive = true;
    if (sfxFile && sfxFile[0] != '\0')
        playSFX(sfxFile);
}

// ============================================================================
//  MAP CONFIGURATION
// ============================================================================
const int LANE_WIDTH = 40;
const int Total_Rows = 20;

enum ZoneType
{
    FINISH,
    ROAD,
    RIVER,
    BUFFER,
    START
};

ZoneType zoneMap[Total_Rows] = {
    FINISH, ROAD, ROAD, ROAD, ROAD, ROAD,
    BUFFER, RIVER, RIVER,
    BUFFER, ROAD, ROAD, ROAD, ROAD, ROAD,
    BUFFER, RIVER, RIVER,
    BUFFER, START};

// ============================================================================
//  DIFFICULTY SETTINGS
// ============================================================================
struct DifficultySettings
{
    string name;
    int trucks;
    int logs;
    int renderMs;
    int obstacleEvery;
    int winsNeeded;
    int gatorCount;
    bool extraGatorRoll;
};

DifficultySettings DIFFICULTIES[3] = {
    {"Easy", 2, 3, 33, 5, 3, 0, false},
    {"Hard", 2, 3, 33, 3, 5, 0, false},
    {"Extreme", 3, 3, 33, 2, 7, 0, true}};

DifficultySettings g_diff = DIFFICULTIES[0];

// ============================================================================
//  LEADERBOARD
// ============================================================================
struct LeaderboardEntry
{
    string name;
    int score;
    string difficulty;
    string result;
};

vector<LeaderboardEntry> g_leaderboard;
const string LEADERBOARD_FILE = "leaderboard.txt";

void loadLeaderboard()
{
    g_leaderboard.clear();
    ifstream f(LEADERBOARD_FILE);
    if (!f.is_open())
        return;
    string name, diff, result;
    int score;
    while (f >> name >> score >> diff >> result)
    {
        for (char &c : name)
            if (c == '_')
                c = ' ';
        g_leaderboard.push_back({name, score, diff, result});
    }
    f.close();
}

void saveLeaderboard()
{
    ofstream f(LEADERBOARD_FILE);
    for (auto &e : g_leaderboard)
    {
        string safeName = e.name;
        for (char &c : safeName)
            if (c == ' ')
                c = '_';
        f << safeName << " " << e.score << " " << e.difficulty << " " << e.result << "\n";
    }
    f.close();
}

void addLeaderboardEntry(const string &name, int score,
                         const string &diff, const string &result)
{
    g_leaderboard.push_back({name, score, diff, result});
    sort(g_leaderboard.begin(), g_leaderboard.end(),
         [](const LeaderboardEntry &a, const LeaderboardEntry &b)
         { return a.score > b.score; });
    if (g_leaderboard.size() > 10)
        g_leaderboard.resize(10);
    saveLeaderboard();
}

void displayLeaderboard()
{
    clearScreen();
    cout << FG_GOLD << BOLD;
    cout << "\n  ╔══════════════════════════════════════════════════════╗\n";
    cout << "  ║               🏆  LEADERBOARD  🏆                   ║\n";
    cout << "  ╚══════════════════════════════════════════════════════╝\n\n"
         << RESET;

    if (g_leaderboard.empty())
    {
        cout << FG_DGRAY << "  No entries yet. Be the first!\n"
             << RESET;
    }
    else
    {
        cout << FG_LGRAY << BOLD;
        cout << "  RANK  NAME               SCORE  DIFF        RESULT\n";
        cout << "  " << string(54, '-') << "\n"
             << RESET;
        for (int i = 0; i < (int)g_leaderboard.size(); i++)
        {
            auto &e = g_leaderboard[i];
            if (i == 0)
                cout << FG_GOLD;
            else if (i == 1)
                cout << FG_LGRAY;
            else if (i == 2)
                cout << FG_ORANGE;
            else
                cout << FG_DGRAY;
            cout << "  #" + to_string(i + 1) + "   ";
            string nm = e.name;
            if ((int)nm.size() > 18)
                nm = nm.substr(0, 18);
            cout << nm + string(19 - (int)nm.size(), ' ');
            cout << to_string(e.score) + string(7 - (int)to_string(e.score).size(), ' ');
            string df = e.difficulty;
            if ((int)df.size() > 11)
                df = df.substr(0, 11);
            cout << df + string(12 - (int)df.size(), ' ');
            if (e.result == "WIN")
                cout << FG_GREEN << "WIN" << RESET;
            else
                cout << FG_RED << e.result << RESET;
            cout << "\n";
        }
    }
    cout << "\n"
         << FG_CYAN << "  Press ENTER to continue..." << RESET << "\n";
    cin.sync();
    cin.get();
}

// ============================================================================
//  LINKED LIST
// ============================================================================
struct Node
{
    string data;
    Node *next;
};

// ============================================================================
//  LANE GENERATION
// ============================================================================
string generateBufferLane()
{
    return "|" + string(LANE_WIDTH, '.') + "|";
}

string generateRoadLane(int laneNum)
{
    int trucks = (laneNum % 2 == 1)
                     ? g_diff.trucks + (rand() % 2)
                     : g_diff.trucks;
    const int TRUCK_SIZE = 5, MIN_GAP = 2;
    while (trucks > 1 && trucks * TRUCK_SIZE + (trucks - 1) * MIN_GAP > LANE_WIDTH)
        trucks--;
    string inner(LANE_WIDTH, '.');
    int spacing = LANE_WIDTH / (trucks + 1), prevEnd = 0;
    for (int t = 0; t < trucks; t++)
    {
        int base = spacing * (t + 1) - (TRUCK_SIZE / 2) + (rand() % 5) - 2;
        int minPos = prevEnd + MIN_GAP;
        if (base < minPos)
            base = minPos;
        int maxPos = LANE_WIDTH - TRUCK_SIZE;
        int trucksLeft = trucks - t - 1;
        if (trucksLeft > 0)
            maxPos = LANE_WIDTH - TRUCK_SIZE - trucksLeft * (TRUCK_SIZE + MIN_GAP);
        if (maxPos < minPos)
            maxPos = minPos;
        if (base > maxPos)
            base = maxPos;
        if (base < 0)
            base = 0;
        for (int j = 0; j < TRUCK_SIZE && base + j < LANE_WIDTH; j++)
            inner[base + j] = '#';
        prevEnd = base + TRUCK_SIZE;
    }
    return "|" + inner + "|";
}

string generateRiverLane(int laneNum)
{
    int totalObjs = (laneNum % 2 == 1)
                        ? g_diff.logs + (rand() % 2)
                        : g_diff.logs;
    const int OBS_SIZE = 4;
    int maxFit = LANE_WIDTH / (OBS_SIZE + 1);
    if (totalObjs > maxFit)
        totalObjs = maxFit;
    if (totalObjs < 1)
        totalObjs = 1;

    vector<bool> isGator(totalObjs, false);
    if (g_diff.extraGatorRoll)
    {
        int safeSlots = min(g_diff.logs, totalObjs);
        int gatorSlots = totalObjs - safeSlots;
        if (gatorSlots > 0)
        {
            int gatorCount = 1 + rand() % 2;
            if (gatorCount > gatorSlots)
                gatorCount = gatorSlots;
            vector<int> idx(totalObjs);
            for (int i = 0; i < totalObjs; i++)
                idx[i] = i;
            for (int i = totalObjs - 1; i > 0; i--)
            {
                int j = rand() % (i + 1);
                swap(idx[i], idx[j]);
            }
            for (int i = 0; i < gatorCount; i++)
                isGator[idx[i]] = true;
        }
    }

    const int MIN_GAP = 2;
    string inner(LANE_WIDTH, '~');
    int spacing = LANE_WIDTH / (totalObjs + 1), prevEnd = 0;
    for (int i = 0; i < totalObjs; i++)
    {
        int base = spacing * (i + 1) - 2 + (rand() % 5) - 1;
        int minPos = prevEnd + MIN_GAP;
        if (base < minPos)
            base = minPos;
        int objsLeft = totalObjs - i - 1;
        int maxPos = LANE_WIDTH - OBS_SIZE - objsLeft * (OBS_SIZE + MIN_GAP);
        if (maxPos < minPos)
            maxPos = minPos;
        if (base > maxPos)
            base = maxPos;
        if (base < 0)
            base = 0;
        char ch = isGator[i] ? 'A' : '=';
        for (int j = 0; j < OBS_SIZE && base + j < LANE_WIDTH; j++)
            inner[base + j] = ch;
        prevEnd = base + OBS_SIZE;
    }
    return "|" + inner + "|";
}

// ============================================================================
//  SHIFT LEFT / RIGHT
// ============================================================================
void shiftLeft(string &lane)
{
    char first = lane[1];
    for (int i = 1; i < LANE_WIDTH; i++)
        lane[i] = lane[i + 1];
    lane[LANE_WIDTH] = first;
}

void shiftRight(string &lane)
{
    char last = lane[LANE_WIDTH];
    for (int i = LANE_WIDTH; i > 1; i--)
        lane[i] = lane[i - 1];
    lane[1] = last;
}

void shiftObstacles(Node *head)
{
    Node *current = head;
    int roadIndex = 0, riverIndex = 0;
    for (int row = 0; row < Total_Rows && current != nullptr; row++)
    {
        if (zoneMap[row] == ROAD)
        {
            roadIndex++;
            if (roadIndex % 2 == 1)
                shiftRight(current->data);
            else
                shiftLeft(current->data);
        }
        else if (zoneMap[row] == RIVER)
        {
            riverIndex++;
            if (riverIndex % 2 == 1)
                shiftRight(current->data);
            else
                shiftLeft(current->data);
        }
        current = current->next;
    }
}

// ============================================================================
//  LIST UTILITIES
// ============================================================================
Node *getLane(Node *head, int index)
{
    Node *current = head;
    for (int i = 0; i < index; i++)
    {
        if (current == nullptr)
            return nullptr;
        current = current->next;
    }
    return current;
}

Node *buildRoad()
{
    Node *head = nullptr, *tail = nullptr;
    int roadIndex = 0, riverIndex = 0;
    for (int row = 0; row < Total_Rows; row++)
    {
        Node *newNode = new Node();
        newNode->next = nullptr;
        if (row == 0)
            newNode->data = "|========================================|";
        else if (zoneMap[row] == ROAD)
        {
            roadIndex++;
            newNode->data = generateRoadLane(roadIndex);
        }
        else if (zoneMap[row] == RIVER)
        {
            riverIndex++;
            newNode->data = generateRiverLane(riverIndex);
        }
        else if (zoneMap[row] == BUFFER)
            newNode->data = generateBufferLane();
        else if (zoneMap[row] == START)
            newNode->data = "|                                        |";
        if (!head)
            head = tail = newNode;
        else
        {
            tail->next = newNode;
            tail = newNode;
        }
    }
    return head;
}

void freeList(Node *&head)
{
    Node *cur = head;
    while (cur)
    {
        Node *nxt = cur->next;
        delete cur;
        cur = nxt;
    }
    head = nullptr;
}

// ============================================================================
//  KEYBOARD INPUT
// ============================================================================
char keyboardInput()
{
    char lastDir = 0;
    while (_kbhit())
    {
        int key = _getch();
        if (key == 27)
            return 'Q';
        char mapped = 0;
        if (key == 224)
        {
            if (!_kbhit())
                break;
            key = _getch();
            switch (key)
            {
            case 72:
                mapped = 'U';
                break;
            case 80:
                mapped = 'D';
                break;
            case 75:
                mapped = 'L';
                break;
            case 77:
                mapped = 'R';
                break;
            }
        }
        else
        {
            switch (key)
            {
            case 'w':
            case 'W':
                mapped = 'U';
                break;
            case 's':
            case 'S':
                mapped = 'D';
                break;
            case 'a':
            case 'A':
                mapped = 'L';
                break;
            case 'd':
            case 'D':
                mapped = 'R';
                break;
            case 'q':
            case 'Q':
                mapped = 'Q';
                break;
            }
        }
        if (mapped == 'Q')
            return 'Q';
        if (mapped)
            lastDir = mapped;
    }
    return lastDir;
}

// ============================================================================
//  COLLISION DETECTION
// ============================================================================
bool checkCollision(Node *head, int playerX, int playerY)
{
    if (zoneMap[playerY] != ROAD)
        return false;
    Node *lane = getLane(head, playerY);
    if (!lane)
        return false;
    return lane->data[playerX] == '#';
}

bool checkDrowned(Node *head, int playerX, int playerY)
{
    if (zoneMap[playerY] != RIVER)
        return false;
    Node *lane = getLane(head, playerY);
    if (!lane)
        return false;
    return lane->data[playerX] == '~';
}

bool checkAlligator(Node *head, int playerX, int playerY)
{
    if (zoneMap[playerY] != RIVER)
        return false;
    Node *lane = getLane(head, playerY);
    if (!lane)
        return false;
    return lane->data[playerX] == 'A';
}

bool checkBorderOut(int playerX)
{
    return (playerX <= 0 || playerX > LANE_WIDTH);
}

// ============================================================================
//  LOG MOVEMENT
// ============================================================================
void updatePlayerWithLog(Node *head, int &playerX, int playerY)
{
    if (zoneMap[playerY] != RIVER)
        return;
    Node *lane = getLane(head, playerY);
    if (!lane)
        return;
    if (lane->data[playerX] != '=')
        return;
    int riverIndex = 0;
    for (int r = 0; r <= playerY; r++)
        if (zoneMap[r] == RIVER)
            riverIndex++;
    bool movesRight = (riverIndex % 2 == 1);
    if (movesRight)
    {
        playerX++;
        if (playerX > LANE_WIDTH + 1)
            playerX = LANE_WIDTH + 1;
    }
    else
    {
        playerX--;
        if (playerX < 0)
            playerX = 0;
    }
}

// ============================================================================
//  BUFFERED RENDER MAP
// ============================================================================
void renderMap(Node *head, int playerX, int playerY)
{
    // If flash is active, tint border chars on map edge rows red
    bool flashOn = g_flashActive && (g_flashFrames % 2 == 0);

    Node *cur = head;
    int row = 0;
    while (cur)
    {
        const string &temp = cur->data;
        string lineOut;
        lineOut.reserve(temp.size() * 14);

        // Left border — flash it red on damage frames
        if (flashOn && row > 0 && row < Total_Rows - 1)
            lineOut += g_flashBg + g_flashFg + temp[0] + RESET;
        else
            lineOut += temp[0];

        for (int i = 1; i < (int)temp.size() - 1; i++)
        {
            if (row == playerY && i == playerX)
            {
                // Player flickers on damage
                if (g_flashActive && g_flashFrames % 3 == 0)
                    lineOut += g_flashBg + FG_WHITE + BOLD + "P" + RESET;
                else
                    lineOut += FG_YELLOW + BOLD + "P" + RESET;
            }
            else if (temp[i] == '#')
            {
                lineOut += FG_RED + temp[i] + RESET;
            }
            else if (temp[i] == '=')
            {
                lineOut += FG_BROWN + temp[i] + RESET;
            }
            else if (temp[i] == 'A')
            {
                lineOut += FG_GREEN + BOLD + temp[i] + RESET;
            }
            else if (temp[i] == '~')
            {
                lineOut += FG_TEAL + temp[i] + RESET;
            }
            else if (row == 0)
            {
                lineOut += FG_GOLD + temp[i] + RESET;
            }
            else
            {
                lineOut += temp[i];
            }
        }

        // Right border
        if (flashOn && row > 0 && row < Total_Rows - 1)
            lineOut += g_flashBg + g_flashFg + temp[temp.size() - 1] + RESET;
        else
            lineOut += temp[temp.size() - 1];

        g_buf.writeLine(row, lineOut);
        cur = cur->next;
        row++;
    }
}

// ============================================================================
//  DAMAGE FLASH OVERLAY  (written into buffer rows below the map)
// ============================================================================
void renderDamageFlash()
{
    // Row Total_Rows is the separator/flash row
    int r = Total_Rows;

    if (!g_flashActive)
    {
        // Clear the flash row when inactive
        g_buf.writeLine(r, FG_DGRAY + "  " + string(44, '-') + RESET);
        g_flashFrames = 0;
        return;
    }

    // Pulsing alert bar — alternates solid and dim every other frame
    bool bright = (g_flashFrames % 2 == 0);
    string bar;
    bar += bright ? (g_flashBg + g_flashFg + BOLD) : (g_flashFg + DIM);

    // Build the full-width alert bar (44 chars to match map width)
    string content = "  " + g_flashIcon + "  " + g_flashMsg + "  ";
    // Pad to 44
    while ((int)content.size() < 44)
        content += " ";
    if ((int)content.size() > 44)
        content = content.substr(0, 44);

    bar += content + RESET;
    g_buf.writeLine(r, bar);

    g_flashFrames--;
    if (g_flashFrames <= 0)
        g_flashActive = false;
}

// ============================================================================
//  HUD  — hearts with animated lost-heart indicator
// ============================================================================
void gameStatus(const string &playerName, int wins, int lives, int winsNeeded)
{
    int r = Total_Rows + 1; // row after the flash bar

    // ── Hearts row ──────────────────────────────────────────────────────────
    string heartsRow;
    heartsRow += "  " + FG_CYAN + BOLD + playerName.substr(0, 12) + RESET;
    heartsRow += FG_DGRAY + "  |  " + RESET;
    heartsRow += FG_YELLOW + "Crossings: " + FG_WHITE + BOLD + to_string(wins) + "/" + to_string(winsNeeded) + RESET;
    heartsRow += FG_DGRAY + "  |  " + RESET;
    heartsRow += FG_RED + "Lives: ";

    // Full hearts
    for (int i = 0; i < lives; i++)
        heartsRow += FG_RED + BOLD + "\xe2\x99\xa5 "; // ♥

    // Lost hearts — show broken heart (grey) for lost ones, flash them red
    int maxLives = 3;
    for (int i = lives; i < maxLives; i++)
    {
        if (g_flashActive)
            heartsRow += FG_RED + DIM + "\xe2\x99\xa5 "; // dim red = just lost
        else
            heartsRow += FG_DGRAY + DIM + "\xe2\x99\xa5 "; // grey = already lost
    }
    heartsRow += RESET;
    g_buf.writeLine(r++, heartsRow);

    // ── Controls row ────────────────────────────────────────────────────────
    string row2;
    row2 += "  " + FG_DGRAY;
    row2 += "[" + FG_WHITE + "\xe2\x86\x91\xe2\x86\x93\xe2\x86\x90\xe2\x86\x92/WASD" + FG_DGRAY + " Move]  ";
    row2 += "[" + FG_WHITE + "ESC/Q" + FG_DGRAY + " Quit]  ";
    row2 += FG_PINK + "Diff: " + FG_WHITE + g_diff.name + RESET;
    g_buf.writeLine(r++, row2);

    g_buf.writeLine(r++, FG_DGRAY + "  " + string(44, '-') + RESET);

    // ── Legend row ──────────────────────────────────────────────────────────
    string legend;
    legend += "  ";
    legend += FG_RED + "# Truck  " + RESET;
    legend += FG_BROWN + "= Log    " + RESET;
    legend += FG_TEAL + "~ River  " + RESET;
    legend += FG_GREEN + "A Gator" + RESET;
    g_buf.writeLine(r++, legend);
}

// ============================================================================
//  TITLE SCREEN
// ============================================================================
void animateBorderSweep()
{
    const int W = 70, H = 24;
    cout << FG_TEAL << BOLD;
    moveCursor(1, 1);
    for (int i = 0; i < W; i++)
    {
        cout << '=';
        cout.flush();
        sleep_ms(8);
    }
    for (int r = 2; r <= H; r++)
    {
        moveCursor(r, W);
        cout << '|';
        cout.flush();
        sleep_ms(12);
    }
    for (int i = W; i >= 1; i--)
    {
        moveCursor(H + 1, i);
        cout << '=';
        cout.flush();
        sleep_ms(8);
    }
    for (int r = H; r >= 2; r--)
    {
        moveCursor(r, 1);
        cout << '|';
        cout.flush();
        sleep_ms(12);
    }
    moveCursor(1, 1);
    cout << '+';
    moveCursor(1, W);
    cout << '+';
    moveCursor(H + 1, 1);
    cout << '+';
    moveCursor(H + 1, W);
    cout << '+';
    cout << RESET;
    cout.flush();
}

void printTitle()
{
    vector<string> road = {
        "            ____     ___       _      ____  ",
        "           |  _ \\   / _ \\     / \\    |  _ \\ ",
        "           | |_) | | | | |   / _ \\   | | | |",
        "           |  _ <  | |_| |  / ___ \\  | |_| |",
        "           |_| \\_\\  \\___/  /_/   \\_\\ |____/ "};
    vector<string> crossing = {
        "     _____   _____    ____   _____  _____  ___  _   _   _____ ",
        "    / ____| |  __ \\  / __ \\ / ____|/ ____||_ _|| \\ | | / ____|",
        "   | |      | |__) || |  | |\\___  \\\\___  \\ | | |  \\| || |  __",
        "   | |____  |  _  / | |__| | ___) | ___) | | | | |\\  || |_|  | ",
        "    \\_____| |_| \\_\\  \\____/ |_____/|_____/|___||_| \\_| \\_____|"};
    string rc[] = {FG_GOLD, FG_ORANGE, FG_YELLOW, FG_ORANGE, FG_GOLD};
    string cc[] = {FG_GREEN, "\033[38;5;48m", FG_TEAL, "\033[38;5;48m", FG_GREEN};
    for (int i = 0; i < 5; i++)
    {
        moveCursor(4 + i, 5);
        cout << BOLD << rc[i] << road[i] << RESET;
        cout.flush();
        sleep_ms(60);
    }
    for (int i = 0; i < 5; i++)
    {
        moveCursor(10 + i, 3);
        cout << BOLD << cc[i] << crossing[i] << RESET;
        cout.flush();
        sleep_ms(60);
    }
    moveCursor(16, 17);
    cout << FG_LGRAY << DIM << "~  C + +   T E R M I N A L   G A M E  ~" << RESET;
    cout.flush();
    sleep_ms(200);
    moveCursor(17, 3);
    cout << FG_DGRAY;
    for (int i = 0; i < 66; i++)
        cout << '-';
    cout << RESET;
    cout.flush();
}

void animateChicken(int passes)
{
    const int BASE_ROW = 27, START_COL = 2, END_COL = 66;
    const string frames[4] = {" (>'-')>", " ^('-')^", " (>'-')>", " (v'-')v"};
    const int ro[4] = {0, -1, 0, 1};
    moveCursor(BASE_ROW + 2, START_COL);
    cout << FG_DGRAY;
    for (int i = 0; i < (END_COL - START_COL + 10); i++)
        cout << '~';
    cout << RESET;
    cout.flush();
    for (int p = 0; p < passes; p++)
    {
        for (int col = START_COL; col <= END_COL; col += 2)
        {
            int frame = (col / 2) % 4, row = BASE_ROW + ro[frame];
            for (int r = BASE_ROW - 1; r <= BASE_ROW + 1; r++)
            {
                moveCursor(r, col - 2);
                cout << "          ";
            }
            moveCursor(row, col);
            cout << FG_YELLOW << BOLD << frames[frame] << RESET;
            if (frame == 3)
            {
                moveCursor(BASE_ROW + 1, col + 1);
                cout << FG_LGRAY << "* *" << RESET;
            }
            cout.flush();
            sleep_ms(55);
        }
        for (int r = BASE_ROW - 1; r <= BASE_ROW + 1; r++)
        {
            moveCursor(r, END_COL);
            cout << "            ";
        }
        cout.flush();
        if (p < passes - 1)
            sleep_ms(120);
    }
    for (int r = BASE_ROW - 1; r <= BASE_ROW + 2; r++)
    {
        moveCursor(r, START_COL);
        cout << "                                                                    ";
    }
    cout.flush();
}

void blinkingPrompt(int row, int col, const string &msg)
{
    for (int b = 0; b < 6; b++)
    {
        moveCursor(row, col);
        if (b % 2 == 0)
            cout << FG_CYAN << BOLD << msg << RESET;
        else
            cout << FG_DGRAY << DIM << msg << RESET;
        cout.flush();
        sleep_ms(420);
    }
    moveCursor(row, col);
    cout << FG_CYAN << BOLD << msg << RESET;
    cout.flush();
}

// ============================================================================
//  DIFFICULTY SCREEN
// ============================================================================
int selectDifficulty()
{
    clearScreen();
    cout << "\n\n";
    cout << FG_GOLD << BOLD;
    cout << "  ╔══════════════════════════════════╗\n";
    cout << "  ║       SELECT DIFFICULTY          ║\n";
    cout << "  ╚══════════════════════════════════╝\n\n"
         << RESET;
    cout << FG_GREEN << BOLD << "  [1] Easy    " << RESET << FG_DGRAY << "- 2 trucks · no gators · slow   · 3 wins\n\n"
         << RESET;
    cout << FG_RED << BOLD << "  [2] Hard    " << RESET << FG_DGRAY << "- 3 trucks · no gators · medium · 5 wins\n\n"
         << RESET;
    cout << FG_DGRAY << "  ─────────────────────────────────────\n"
         << RESET;
    cout << FG_PINK << BOLD << "  Ian Lastimosa\n"
         << RESET;
    cout << FG_ORANGE << BOLD << "  [3] Extreme " << RESET << FG_DGRAY << "- 4 trucks · alligators! · fast · 7 wins\n\n"
         << RESET;
    cout << FG_DGRAY << "  ─────────────────────────────────────\n\n"
         << RESET;
    cout << FG_CYAN << "  Choice [1/2/3]: " << FG_WHITE;
    char c = '1';
    cin >> c;
    cin.ignore(1000, '\n');
    int choice = (c == '3') ? 2 : (c == '2') ? 1
                                             : 0;
    g_diff = DIFFICULTIES[choice];
    return choice;
}

// ============================================================================
//  MAIN MENU
// ============================================================================
string mainMenu()
{
    hideCursor();
    clearScreen();
    animateBorderSweep();
    sleep_ms(100);
    printTitle();
    sleep_ms(150);

    moveCursor(18, 4);
    cout << FG_WHITE << BOLD << "HOW TO PLAY" << RESET;
    moveCursor(19, 4);
    cout << FG_LGRAY << "Arrow/WASD" << FG_DGRAY << " -- Move player" << RESET;
    moveCursor(20, 4);
    cout << FG_RED << "#####" << FG_DGRAY << "      -- Dodge trucks!" << RESET;
    moveCursor(21, 4);
    cout << FG_BROWN << "=====" << FG_DGRAY << "      -- Hop on logs" << RESET;
    moveCursor(22, 4);
    cout << FG_GREEN << "AAAA" << FG_DGRAY << "       -- Avoid alligators" << RESET;
    moveCursor(23, 4);
    cout << FG_GOLD << "x wins" << FG_DGRAY << "     -- Complete crossings!" << RESET;
    moveCursor(18, 44);
    cout << FG_PINK << BOLD << "CSDC102 Project" << RESET;
    moveCursor(19, 44);
    cout << FG_DGRAY << "Language : " << FG_TEAL << "C++" << RESET;
    moveCursor(20, 44);
    cout << FG_DGRAY << "Engine   : " << FG_TEAL << "Terminal" << RESET;
    moveCursor(21, 44);
    cout << FG_DGRAY << "Compiler : " << FG_TEAL << "g++ 6.3.0" << RESET;
    cout.flush();

    animateChicken(1);
    sleep_ms(150);
    blinkingPrompt(23, 18, "[ Press ENTER to Start ]");
    moveCursor(25, 1);
    showCursor();
    cin.sync();
    cin.get();

    hideCursor();
    clearScreen();
    for (int f = 0; f < 3; f++)
    {
        moveCursor(12, 24);
        cout << FG_GREEN << BOLD << "*** ROAD CROSSING ***" << RESET;
        cout.flush();
        sleep_ms(220);
        moveCursor(12, 24);
        cout << "                     ";
        cout.flush();
        sleep_ms(160);
    }
    sleep_ms(200);
    clearScreen();
    showCursor();

    clearScreen();
    cout << "\n\n";
    cout << FG_GOLD << BOLD;
    cout << "  ╔══════════════════════════════════╗\n";
    cout << "  ║       ENTER YOUR NAME            ║\n";
    cout << "  ╚══════════════════════════════════╝\n\n"
         << RESET;
    cout << FG_LGRAY << "  Your name will be saved to the leaderboard.\n\n"
         << RESET;
    cout << FG_CYAN << "  Name: " << FG_WHITE;
    string playerName;
    getline(cin, playerName);
    while (!playerName.empty() && playerName.front() == ' ')
        playerName.erase(playerName.begin());
    while (!playerName.empty() && playerName.back() == ' ')
        playerName.pop_back();
    if (playerName.empty())
        playerName = "Anonymous";

    selectDifficulty();
    clearScreen();
    cout << "\n\n  " << FG_GREEN << BOLD << "Difficulty: " << g_diff.name << RESET << "\n";
    cout << "  " << FG_CYAN << "Good luck, " << playerName << "!\n"
         << RESET;
    sleep_ms(1200);
    clearScreen();
    return playerName;
}

// ============================================================================
//  GAME OVER / WIN SCREEN
// ============================================================================
void gameOverScreen(const string &playerName, int wins, const string &cause)
{
    clearScreen();
    cout << "\n\n";
    if (cause == "WIN")
    {
        cout << FG_GOLD << BOLD;
        cout << "  ╔══════════════════════════════════╗\n";
        cout << "  ║        \xf0\x9f\x8e\x89  YOU WIN!  \xf0\x9f\x8e\x89          ║\n";
        cout << "  ╚══════════════════════════════════╝\n\n"
             << RESET;
        cout << FG_GREEN << "  " << playerName << " crossed " << wins << " times!\n\n"
             << RESET;
    }
    else
    {
        cout << FG_RED << BOLD;
        cout << "  ╔══════════════════════════════════╗\n";
        cout << "  ║         \xf0\x9f\x92\x80  GAME OVER  \xf0\x9f\x92\x80        ║\n";
        cout << "  ╚══════════════════════════════════╝\n\n"
             << RESET;
        cout << FG_RED << "  Cause: " << FG_WHITE << BOLD << cause << RESET << "\n";
        cout << FG_LGRAY << "  " << playerName << " completed " << wins << " crossing(s).\n\n"
             << RESET;
    }
    cout << FG_DGRAY << "  Difficulty: " << FG_WHITE << g_diff.name << RESET << "\n\n";
    addLeaderboardEntry(playerName, wins, g_diff.name, (cause == "WIN") ? "WIN" : cause);
    sleep_ms(1500);
}

// ============================================================================
//  MAIN
// ============================================================================
int main()
{
    initConsole();
    srand(static_cast<unsigned>(time(nullptr)));
    loadLeaderboard();

    g_buf.init(BUF_ROWS);

    string playerName = mainMenu();

    bool keepPlaying = true;
    while (keepPlaying)
    {
        Node *head = buildRoad();

        int playerX = LANE_WIDTH / 2;
        int playerY = Total_Rows - 1;
        int wins = 0;
        int lives = 3;
        g_prevLives = 3;
        string deathCause = "";

        // Reset flash state at game start
        g_flashActive = false;
        g_flashFrames = 0;

        PlaySound("Songs/slimeyfox.wav", NULL, SND_FILENAME | SND_ASYNC | SND_LOOP);

        clearScreen();
        g_buf.invalidate();

        int tickCounter = 0;
        bool running = true;

        LARGE_INTEGER freq, frameStart, frameEnd;
        QueryPerformanceFrequency(&freq);

        // Respawn lambda (defined once, used in collision blocks)
        auto respawn = [&]()
        {
            freeList(head);
            head = buildRoad();
            playerX = LANE_WIDTH / 2;
            playerY = Total_Rows - 1;
            tickCounter = 0;
            clearScreen();
            g_buf.invalidate();
        };

        while (running)
        {
            QueryPerformanceCounter(&frameStart);

            // ── 1. Input ────────────────────────────────────────────────────
            char input = keyboardInput();
            if (input == 'Q')
            {
                deathCause = "QUIT";
                running = false;
                break;
            }

            int newX = playerX, newY = playerY;
            bool moved = false;
            if (input == 'U' && playerY > 0)
            {
                newY--;
                moved = true;
            }
            else if (input == 'D' && playerY < Total_Rows - 1)
            {
                newY++;
                moved = true;
            }
            else if (input == 'L')
            {
                newX--;
                moved = true;
            }
            else if (input == 'R')
            {
                newX++;
                moved = true;
            }

            // Jump sound on any movement (soft, non-blocking)
            if (moved)
                playSFX("Songs/jump.wav");

            playerX = newX;
            playerY = newY;

            // ── 2. Log carry + obstacle shift ───────────────────────────────
            tickCounter++;
            bool obstacleThisTick = (tickCounter % g_diff.obstacleEvery == 0);
            if (obstacleThisTick)
            {
                updatePlayerWithLog(head, playerX, playerY);
                shiftObstacles(head);
            }

            // ── 3. Win detection ─────────────────────────────────────────────
            if (playerY == 0)
            {
                wins++;
                playSFX("Songs/jump.wav"); // victory hop
                if (wins >= g_diff.winsNeeded)
                {
                    deathCause = "WIN";
                    running = false;
                    break;
                }
                freeList(head);
                head = buildRoad();
                playerX = LANE_WIDTH / 2;
                playerY = Total_Rows - 1;
                tickCounter = 0;
                g_flashActive = false;
                clearScreen();
                g_buf.invalidate();
                cout << FG_GREEN << BOLD
                     << "\n  CROSSING! " << wins << "/" << g_diff.winsNeeded
                     << " -- Keep going!\n"
                     << RESET;
                cout.flush();
                sleep_ms(700);
                clearScreen();
                g_buf.invalidate();
                continue;
            }

            // ── 4. Render ────────────────────────────────────────────────────
            g_buf.beginFrame();
            renderMap(head, playerX, playerY);
            renderDamageFlash(); // flash bar row + tinted borders
            gameStatus(playerName, wins, lives, g_diff.winsNeeded);
            g_buf.endFrame();

            // ── 5. Collisions ─────────────────────────────────────────────────
            if (checkBorderOut(playerX))
            {
                lives--;
                triggerFlash("OUT OF BOUNDS!", FG_WHITE, BG_ORANGE,
                             "\xe2\x9a\xa0", "Songs/hit.wav"); // ⚠
                if (lives <= 0)
                {
                    deathCause = "OUT OF BOUNDS";
                    running = false;
                }
                else
                {
                    // Let the flash render for a bit before respawning
                    for (int f = 0; f < 6 && g_flashActive; f++)
                    {
                        g_buf.beginFrame();
                        renderMap(head, playerX, playerY);
                        renderDamageFlash();
                        gameStatus(playerName, wins, lives, g_diff.winsNeeded);
                        g_buf.endFrame();
                        sleep_ms(g_diff.renderMs);
                    }
                    respawn();
                }
                continue;
            }

            if (checkCollision(head, playerX, playerY))
            {
                lives--;
                triggerFlash("SQUISHED BY TRUCK!", FG_WHITE, BG_RED,
                             "\xf0\x9f\x9a\x9b", "Songs/hit.wav"); // 🚛
                if (lives <= 0)
                {
                    deathCause = "TRUCK";
                    running = false;
                }
                else
                {
                    for (int f = 0; f < 6 && g_flashActive; f++)
                    {
                        g_buf.beginFrame();
                        renderMap(head, playerX, playerY);
                        renderDamageFlash();
                        gameStatus(playerName, wins, lives, g_diff.winsNeeded);
                        g_buf.endFrame();
                        sleep_ms(g_diff.renderMs);
                    }
                    respawn();
                }
                continue;
            }

            if (checkAlligator(head, playerX, playerY))
            {
                lives--;
                triggerFlash("CHOMPED BY GATOR!", FG_WHITE, BG_GREEN,
                             "\xf0\x9f\x90\x8a", "Songs/chomp.wav"); // 🐊
                if (lives <= 0)
                {
                    deathCause = "ALLIGATOR";
                    running = false;
                }
                else
                {
                    for (int f = 0; f < 6 && g_flashActive; f++)
                    {
                        g_buf.beginFrame();
                        renderMap(head, playerX, playerY);
                        renderDamageFlash();
                        gameStatus(playerName, wins, lives, g_diff.winsNeeded);
                        g_buf.endFrame();
                        sleep_ms(g_diff.renderMs);
                    }
                    respawn();
                }
                continue;
            }

            if (checkDrowned(head, playerX, playerY))
            {
                lives--;
                triggerFlash("DROWNED!", FG_WHITE, BG_TEAL,
                             "\xf0\x9f\x92\xa7", "Songs/splash.wav"); // 💧
                if (lives <= 0)
                {
                    deathCause = "DROWNED";
                    running = false;
                }
                else
                {
                    for (int f = 0; f < 6 && g_flashActive; f++)
                    {
                        g_buf.beginFrame();
                        renderMap(head, playerX, playerY);
                        renderDamageFlash();
                        gameStatus(playerName, wins, lives, g_diff.winsNeeded);
                        g_buf.endFrame();
                        sleep_ms(g_diff.renderMs);
                    }
                    respawn();
                }
                continue;
            }

            // ── 6. Frame-rate cap ─────────────────────────────────────────────
            QueryPerformanceCounter(&frameEnd);
            long long elapsed = (frameEnd.QuadPart - frameStart.QuadPart) * 1000 / freq.QuadPart;
            long long remaining = (long long)g_diff.renderMs - elapsed;
            if (remaining > 1)
                Sleep((DWORD)remaining);
        }

        freeList(head);
        PlaySound(NULL, NULL, 0);

        gameOverScreen(playerName, wins, deathCause);
        displayLeaderboard();

        clearScreen();
        cout << "\n\n  " << FG_CYAN << BOLD << "Play again? [Y/N]: " << FG_WHITE;
        char again;
        cin >> again;
        cin.ignore(1000, '\n');

        if (again == 'y' || again == 'Y')
        {
            selectDifficulty();
            clearScreen();
            cout << "\n\n  " << FG_GREEN << BOLD << "Difficulty: " << g_diff.name << RESET << "\n";
            cout << "  " << FG_CYAN << "Good luck, " << playerName << "!\n"
                 << RESET;
            sleep_ms(1200);
            clearScreen();
            g_buf.invalidate();
            g_flashActive = false;
            PlaySound("Songs/slimeyfox.wav", NULL, SND_FILENAME | SND_ASYNC | SND_LOOP);
        }
        else
        {
            keepPlaying = false;
        }
    }

    clearScreen();
    cout << FG_GOLD << BOLD << "\n  Thanks for playing Road Crossing!\n\n"
         << RESET;
    return 0;
}