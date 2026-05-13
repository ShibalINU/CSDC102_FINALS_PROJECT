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
#define CLEAR "cls"
void sleep_ms(int ms) { Sleep(ms); }
void initConsole() {
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(hOut, &mode);
    SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
}
#else
#include <unistd.h>
#define CLEAR "clear"
void sleep_ms(int ms) { usleep(ms * 1000); }
void initConsole() {}
#endif

using namespace std;

// ─── ANSI COLOR CODES ───────────────────────────────────────────────────────
const string RESET     = "\033[0m";
const string BOLD      = "\033[1m";
const string DIM       = "\033[2m";
const string FG_YELLOW = "\033[38;5;220m";
const string FG_GOLD   = "\033[38;5;178m";
const string FG_ORANGE = "\033[38;5;208m";
const string FG_GREEN  = "\033[38;5;82m";
const string FG_TEAL   = "\033[38;5;51m";
const string FG_CYAN   = "\033[38;5;87m";
const string FG_WHITE  = "\033[38;5;255m";
const string FG_LGRAY  = "\033[38;5;250m";
const string FG_DGRAY  = "\033[38;5;240m";
const string FG_RED    = "\033[38;5;203m";
const string FG_PINK   = "\033[38;5;213m";
const string FG_LBLUE  = "\033[38;5;117m";
const string FG_BROWN = "\033[38;5;94m";

// ─── CURSOR CONTROL ─────────────────────────────────────────────────────────
void moveCursor(int row, int col) { cout << "\033[" << row << ";" << col << "H"; }
void hideCursor()  { cout << "\033[?25l"; }
void showCursor()  { cout << "\033[?25h"; }
void clearScreen() { system(CLEAR); }

// ============================================================================
//  MAP CONFIGURATION
// ============================================================================
const int LANE_WIDTH = 40;
const int Total_Rows = 20;

enum ZoneType { FINISH, ROAD, RIVER, BUFFER, START };

ZoneType zoneMap[Total_Rows] = {
    FINISH,  // 0
    ROAD,    // 1
    ROAD,    // 2
    ROAD,    // 3
    ROAD,    // 4
    ROAD,    // 5
    BUFFER,  // 6
    RIVER,   // 7
    RIVER,   // 8
    BUFFER,  // 9
    ROAD,    // 10
    ROAD,    // 11
    ROAD,    // 12
    ROAD,    // 13
    ROAD,    // 14
    BUFFER,  // 15
    RIVER,   // 16
    RIVER,   // 17
    BUFFER,  // 18
    START    // 19
};

// ============================================================================
//  DIFFICULTY SETTINGS
// ============================================================================
struct DifficultySettings {
    string name;
    int    trucks;          // base trucks per road lane
    int    logs;            // guaranteed safe logs per river lane (always placed first)
    int    renderMs;        // screen refresh delay — kept LOW for responsive feel
    int    obstacleEvery;   // shift obstacles once every N render frames (higher = slower)
    int    winsNeeded;      // crossings needed to win
    // ── Gator rules ──────────────────────────────────────────────────────────
    // Easy:    gatorCount = 0, extraGatorRoll = false  → zero gators ever
    // Hard:    gatorCount = 1, extraGatorRoll = false  → exactly 1 gator per lane
    // Extreme: gatorCount = 2, extraGatorRoll = true   → 2 guaranteed + random extras
    int    gatorCount;      // guaranteed gators placed per river lane
    bool   extraGatorRoll;  // each remaining log has a 1-in-3 chance to also be a gator
};

//     name         trucks    logs   renderMs  obsEvery  wins  gators  extraRoll
DifficultySettings DIFFICULTIES[3] = {
    { "Easy",          2,       3,      80,      5,      3,      0,    false },
    { "Hard",          2,       3,      80,      3,      5,      2,    false },
    { "Extreme",       3,       3,      80,      2,      7,      3,    true  }
};

// renderMs  = 80ms always  → screen feels snappy / input-responsive on all difficulties
// obstacleEvery = 6/4/2    → obstacles shift every 480ms / 320ms / 160ms respectively

DifficultySettings g_diff = DIFFICULTIES[0];

// ============================================================================
//  LEADERBOARD
// ============================================================================
struct LeaderboardEntry {
    string name;
    int    score;
    string difficulty;
    string result;
};

vector<LeaderboardEntry> g_leaderboard;
const string LEADERBOARD_FILE = "leaderboard.txt";

void loadLeaderboard() {
    g_leaderboard.clear();
    ifstream f(LEADERBOARD_FILE);
    if (!f.is_open()) return;
    string name, diff, result;
    int score;
    while (f >> name >> score >> diff >> result) {
        for (char& c : name) if (c == '_') c = ' ';
        g_leaderboard.push_back({ name, score, diff, result });
    }
    f.close();
}

void saveLeaderboard() {
    ofstream f(LEADERBOARD_FILE);
    for (auto& e : g_leaderboard) {
        string safeName = e.name;
        for (char& c : safeName) if (c == ' ') c = '_';
        f << safeName << " " << e.score << " " << e.difficulty << " " << e.result << "\n";
    }
    f.close();
}

void addLeaderboardEntry(const string& name, int score,
                         const string& diff, const string& result) {
    g_leaderboard.push_back({ name, score, diff, result });
    sort(g_leaderboard.begin(), g_leaderboard.end(),
         [](const LeaderboardEntry& a, const LeaderboardEntry& b){
             return a.score > b.score;
         });
    if (g_leaderboard.size() > 10) g_leaderboard.resize(10);
    saveLeaderboard();
}

void displayLeaderboard() {
    clearScreen();
    cout << FG_GOLD << BOLD;
    cout << "\n  ╔══════════════════════════════════════════════════════╗\n";
    cout << "  ║               🏆  LEADERBOARD  🏆                   ║\n";
    cout << "  ╚══════════════════════════════════════════════════════╝\n\n" << RESET;

    if (g_leaderboard.empty()) {
        cout << FG_DGRAY << "  No entries yet. Be the first!\n" << RESET;
    } else {
        cout << FG_LGRAY << BOLD;
        cout << "  RANK  NAME               SCORE  DIFF        RESULT\n";
        cout << "  " << string(54, '-') << "\n" << RESET;

        for (int i = 0; i < (int)g_leaderboard.size(); i++) {
            auto& e = g_leaderboard[i];
            if      (i == 0) cout << FG_GOLD;
            else if (i == 1) cout << FG_LGRAY;
            else if (i == 2) cout << FG_ORANGE;
            else             cout << FG_DGRAY;

            cout << "  #" + to_string(i + 1) + "   ";

            string nm = e.name;
            if ((int)nm.size() > 18) nm = nm.substr(0, 18);
            cout << nm + string(19 - (int)nm.size(), ' ');
            cout << to_string(e.score) + string(7 - (int)to_string(e.score).size(), ' ');

            string df = e.difficulty;
            if ((int)df.size() > 11) df = df.substr(0, 11);
            cout << df + string(12 - (int)df.size(), ' ');

            if (e.result == "WIN") cout << FG_GREEN << "WIN" << RESET;
            else                   cout << FG_RED   << e.result << RESET;
            cout << "\n";
        }
    }
    cout << "\n" << FG_CYAN << "  Press ENTER to continue..." << RESET << "\n";
    cin.sync();
    cin.get();
}

// ============================================================================
//  LINKED LIST
// ============================================================================
struct Node {
    string data;
    Node*  next;
};

// ============================================================================
//  LANE GENERATION
// ============================================================================
string generateBufferLane() {
    return "|" + string(LANE_WIDTH, '.') + "|";
}

// ─── generateRoadLane ────────────────────────────────────────────────────────
string generateRoadLane(int laneNum) {
    int trucks = (laneNum % 2 == 1)
        ? g_diff.trucks + (rand() % 2)   // odd  → trucks or trucks+1
        : g_diff.trucks;                 // even → always base count

    const int TRUCK_SIZE = 5;

    // Safety clamp: ensure trucks physically fit with 1-cell gaps between them
    int maxFit = LANE_WIDTH / (TRUCK_SIZE + 1);
    if (trucks > maxFit) trucks = maxFit;

    string inner(LANE_WIDTH, '.');

    // Section-based placement — divide lane into equal bands, one truck per band.
    // This guarantees trucks never overlap regardless of random position within band.
    int sectionSize = LANE_WIDTH / trucks;

    for (int i = 0; i < trucks; i++) {
        // Band start/end in 0-indexed inner[] coords
        int bandStart = i * sectionSize;
        int bandEnd   = (i + 1) * sectionSize - 1;

        // Truck must fit fully inside the band; right-edge is bandEnd - (TRUCK_SIZE-1)
        int maxStart = bandEnd - (TRUCK_SIZE - 1);
        if (maxStart < bandStart) maxStart = bandStart;  // clamp, should not happen

        int pos = bandStart + rand() % (maxStart - bandStart + 1);

        // Stamp truck — pos is already 0-indexed in inner[]
        for (int j = 0; j < TRUCK_SIZE && pos + j < LANE_WIDTH; j++)
            inner[pos + j] = '#';
    }

    return "|" + inner + "|";
}

// ─── generateRiverLane ───────────────────────────────────────────────────────
// FIX SUMMARY:
//   Old code used secStart = i*sectionSize + 1 and secEnd = (i+1)*sectionSize - OBS_SIZE - 1.
//   The +1 and -OBS_SIZE-1 margins ate into each section, sometimes making secEnd < 0
//   or secEnd < secStart, collapsing the random range to 0 and causing rand()%0 (UB)
//   or stamping objects out-of-bounds (silently skipped by the compiler, leaving pure ~).
//
//   Fix: use clean 0-indexed band math identical to generateRoadLane.
//   Band covers [bandStart .. bandEnd]. Object must fit fully, so maxStart = bandEnd-(OBS_SIZE-1).
//   Clamp ensures maxStart >= bandStart, so rand() range is always >= 1.
//
// GATOR RULES (per difficulty):
//   Easy    → gatorCount=0: all objects are logs ('='), no 'A' ever
//   Hard    → gatorCount=1: exactly one randomly chosen object per lane is a gator
//   Extreme → gatorCount=2 + extraGatorRoll: 2 guaranteed gators, and each remaining
//             log slot has a 1-in-3 extra chance to also become a gator
string generateRiverLane(int laneNum) {

    // Total river objects this lane (logs + possible gators combined)
    int totalObjs = (laneNum % 2 == 1)
        ? g_diff.logs + (rand() % 2)   // odd  → logs or logs+1
        : g_diff.logs;                 // even → base count

    const int OBS_SIZE = 4;

    // Safety clamp: objects must fit with 1-cell gaps
    int maxFit = LANE_WIDTH / (OBS_SIZE + 1);
    if (totalObjs > maxFit) totalObjs = maxFit;
    if (totalObjs < 1)      totalObjs = 1;   // always at least 1 object

    // ── Build gator index set ────────────────────────────────────────────────
    // Decide which object slots (0..totalObjs-1) will be gators.
    // Easy: none. Hard: pick gatorCount random unique indices.
    // Extreme: pick gatorCount + random extras.
    vector<bool> isGator(totalObjs, false);

    if (g_diff.gatorCount > 0) {
        // Shuffle indices and mark the first gatorCount as gators
        vector<int> indices(totalObjs);
        for (int i = 0; i < totalObjs; i++) indices[i] = i;

        // Fisher-Yates shuffle on the indices vector
        for (int i = totalObjs - 1; i > 0; i--) {
            int j = rand() % (i + 1);
            int tmp = indices[i]; indices[i] = indices[j]; indices[j] = tmp;
        }

        int guaranteed = min(g_diff.gatorCount, totalObjs);
        for (int i = 0; i < guaranteed; i++)
            isGator[indices[i]] = true;

        // Extra random gator rolls for Extreme
        if (g_diff.extraGatorRoll) {
            for (int i = 0; i < totalObjs; i++) {
                if (!isGator[i] && rand() % 3 == 0)
                    isGator[i] = true;
            }
        }
    }
    // Easy: isGator stays all-false — no gators at all

    // ── Place objects using clean band math ──────────────────────────────────
    string inner(LANE_WIDTH, '~');
    int sectionSize = LANE_WIDTH / totalObjs;

    for (int i = 0; i < totalObjs; i++) {
        int bandStart = i * sectionSize;
        int bandEnd   = (i == totalObjs - 1)
                        ? LANE_WIDTH - 1              // last band gets any remainder
                        : (i + 1) * sectionSize - 1;

        // Object must fit fully within the band
        int maxStart = bandEnd - (OBS_SIZE - 1);
        if (maxStart < bandStart) maxStart = bandStart;  // should not happen after clamp

        int start = bandStart + rand() % (maxStart - bandStart + 1);

        char ch = isGator[i] ? 'A' : '=';
        for (int j = 0; j < OBS_SIZE && start + j < LANE_WIDTH; j++)
            inner[start + j] = ch;
    }

    return "|" + inner + "|";
}

// ============================================================================
//  SHIFT LEFT / RIGHT
// ============================================================================
void shiftLeft(string& lane) {
    char bg = '.';
    for (int i = 1; i <= LANE_WIDTH; i++) {
        if (lane[i] == '.' || lane[i] == '~') { bg = lane[i]; break; }
    }

    struct Obs { int start, len; char ch; };
    vector<Obs> obstacles;

    int i = 0;
    while (i < LANE_WIDTH) {
        char c = lane[i + 1];
        if (c != bg) {
            Obs o = { i, 0, c };
            while (i < LANE_WIDTH && lane[i + 1] == o.ch) { o.len++; i++; }
            obstacles.push_back(o);
        } else { i++; }
    }

    // Merge split obstacle that wraps the edge
    if (obstacles.size() >= 2) {
        Obs& F = obstacles.front();
        Obs& L = obstacles.back();
        if (F.ch == L.ch && F.start == 0 && L.start + L.len == LANE_WIDTH) {
            Obs merged = { L.start, L.len + F.len, L.ch };
            obstacles.erase(obstacles.begin());
            obstacles.pop_back();
            obstacles.push_back(merged);
        }
    }

    string inner(LANE_WIDTH, bg);
    for (auto& o : obstacles) {
        int newStart = (o.start - 1 + LANE_WIDTH) % LANE_WIDTH;
        if (newStart + o.len > LANE_WIDTH) {
            // Obstacle is crossing the edge — keep size fixed (no random resize)
            // to prevent objects from shrinking/growing unexpectedly mid-game
        }
        for (int j = 0; j < o.len; j++)
            inner[(newStart + j) % LANE_WIDTH] = o.ch;
    }
    lane = "|" + inner + "|";
}

void shiftRight(string& lane) {
    char bg = '.';
    for (int i = 1; i <= LANE_WIDTH; i++) {
        if (lane[i] == '.' || lane[i] == '~') { bg = lane[i]; break; }
    }

    struct Obs { int start, len; char ch; };
    vector<Obs> obstacles;

    int i = 0;
    while (i < LANE_WIDTH) {
        char c = lane[i + 1];
        if (c != bg) {
            Obs o = { i, 0, c };
            while (i < LANE_WIDTH && lane[i + 1] == o.ch) { o.len++; i++; }
            obstacles.push_back(o);
        } else { i++; }
    }

    if (obstacles.size() >= 2) {
        Obs& F = obstacles.front();
        Obs& L = obstacles.back();
        if (F.ch == L.ch && F.start == 0 && L.start + L.len == LANE_WIDTH) {
            Obs merged = { L.start, L.len + F.len, L.ch };
            obstacles.erase(obstacles.begin());
            obstacles.pop_back();
            obstacles.push_back(merged);
        }
    }

    string inner(LANE_WIDTH, bg);
    for (auto& o : obstacles) {
        int newStart = (o.start + 1) % LANE_WIDTH;
        for (int j = 0; j < o.len; j++)
            inner[(newStart + j) % LANE_WIDTH] = o.ch;
    }
    lane = "|" + inner + "|";
}

void shiftObstacles(Node* head) {
    Node* current  = head;
    int roadIndex  = 0;
    int riverIndex = 0;

    for (int row = 0; row < Total_Rows && current != nullptr; row++) {
        if (zoneMap[row] == ROAD) {
            roadIndex++;
            if (roadIndex % 2 == 1) shiftRight(current->data);
            else                    shiftLeft(current->data);
        }
        else if (zoneMap[row] == RIVER) {
            riverIndex++;
            if (riverIndex % 2 == 1) shiftRight(current->data);
            else                     shiftLeft(current->data);
        }
        current = current->next;
    }
}

// ============================================================================
//  LIST UTILITIES
// ============================================================================
Node* getLane(Node* head, int index) {
    Node* current = head;
    for (int i = 0; i < index; i++) {
        if (current == nullptr) return nullptr;
        current = current->next;
    }
    return current;
}

Node* buildRoad() {
    Node* head     = nullptr;
    Node* tail     = nullptr;
    int roadIndex  = 0;
    int riverIndex = 0;

    for (int row = 0; row < Total_Rows; row++) {
        Node* newNode = new Node();
        newNode->next = nullptr;

        if      (row == 0)                   newNode->data = "|========================================|";
        else if (zoneMap[row] == ROAD)      { roadIndex++;  newNode->data = generateRoadLane(roadIndex);  }
        else if (zoneMap[row] == RIVER)     { riverIndex++; newNode->data = generateRiverLane(riverIndex); }
        else if (zoneMap[row] == BUFFER)    newNode->data = generateBufferLane();
        else if (zoneMap[row] == START)     newNode->data = "|                                        |";

        if (!head) { head = tail = newNode; }
        else       { tail->next = newNode; tail = newNode; }
    }
    return head;
}

void freeList(Node*& head) {
    Node* cur = head;
    while (cur) { Node* nxt = cur->next; delete cur; cur = nxt; }
    head = nullptr;
}

// ============================================================================
//  KEYBOARD INPUT
// ============================================================================
char keyboardInput() {
    if (!_kbhit()) return 0;
    int key = _getch();
    if (key == 27) return 'Q';
    if (key == 224) {
        key = _getch();
        switch (key) {
            case 72: return 'U';
            case 80: return 'D';
            case 75: return 'L';
            case 77: return 'R';
        }
    }
    switch (key) {
        case 'w': case 'W': return 'U';
        case 's': case 'S': return 'D';
        case 'a': case 'A': return 'L';
        case 'd': case 'D': return 'R';
        case 'q': case 'Q': return 'Q';
    }
    return 0;
}

// ============================================================================
//  COLLISION DETECTION
// ============================================================================
bool checkCollision(Node* head, int playerX, int playerY) {
    if (zoneMap[playerY] != ROAD) return false;
    Node* lane = getLane(head, playerY);
    if (!lane) return false;
    return lane->data[playerX] == '#';
}

bool checkDrowned(Node* head, int playerX, int playerY) {
    if (zoneMap[playerY] != RIVER) return false;
    Node* lane = getLane(head, playerY);
    if (!lane) return false;
    return lane->data[playerX] == '~';
}

bool checkAlligator(Node* head, int playerX, int playerY) {
    if (zoneMap[playerY] != RIVER) return false;
    Node* lane = getLane(head, playerY);
    if (!lane) return false;
    return lane->data[playerX] == 'A';
}

// ============================================================================
//  LOG MOVEMENT
// ============================================================================
void updatePlayerWithLog(Node* head, int& playerX, int playerY) {
    if (zoneMap[playerY] != RIVER) return;
    Node* lane = getLane(head, playerY);
    if (!lane) return;
    if (lane->data[playerX] != '=') return;

    int riverIndex = 0;
    for (int r = 0; r <= playerY; r++)
        if (zoneMap[r] == RIVER) riverIndex++;

    bool movesRight = (riverIndex % 2 == 1);
    if (movesRight) { if (++playerX > LANE_WIDTH) playerX = 1; }
    else            { if (--playerX < 1) playerX = LANE_WIDTH; }
}

// ============================================================================
//  HUD
// ============================================================================
void gameStatus(const string& playerName, int wins, int lives, int winsNeeded) {
    cout << "\n";
    cout << FG_DGRAY << "  " << string(42, '-') << "\n" << RESET;
    cout << "  " << FG_CYAN << BOLD << playerName << RESET;
    cout << FG_DGRAY << "  |  " << RESET;
    cout << FG_YELLOW << "Crossings: " << FG_WHITE << BOLD << wins << "/" << winsNeeded << RESET;
    cout << FG_DGRAY  << "  |  " << RESET;
    cout << FG_RED    << "Lives: ";
    for (int i = 0; i < lives; i++) cout << FG_RED << BOLD << "♥ ";
    cout << RESET;
    cout << "\n  " << FG_DGRAY;
    cout << "[" << FG_WHITE << "↑↓←→/WASD" << FG_DGRAY << " Move]  ";
    cout << "[" << FG_WHITE << "ESC/Q"      << FG_DGRAY << " Quit]  ";
    cout << FG_PINK << "Diff: " << FG_WHITE << g_diff.name << RESET << "\n";
}

// ============================================================================
//  TITLE SCREEN
// ============================================================================
void animateBorderSweep() {
    const int W = 70, H = 24;
    cout << FG_TEAL << BOLD;
    moveCursor(1,1);  for(int i=0;i<W;i++){cout<<'=';cout.flush();sleep_ms(8);}
    for(int r=2;r<=H;r++){moveCursor(r,W);cout<<'|';cout.flush();sleep_ms(12);}
    for(int i=W;i>=1;i--){moveCursor(H+1,i);cout<<'=';cout.flush();sleep_ms(8);}
    for(int r=H;r>=2;r--){moveCursor(r,1);cout<<'|';cout.flush();sleep_ms(12);}
    moveCursor(1,1);cout<<'+'; moveCursor(1,W);cout<<'+';
    moveCursor(H+1,1);cout<<'+'; moveCursor(H+1,W);cout<<'+';
    cout<<RESET; cout.flush();
}

void printTitle() {
    vector<string> road = {
        "            ____     ___       _      ____  ",
        "           |  _ \\   / _ \\     / \\    |  _ \\ ",
        "           | |_) | | | | |   / _ \\   | | | |",
        "           |  _ <  | |_| |  / ___ \\  | |_| |",
        "           |_| \\_\\  \\___/  /_/   \\_\\ |____/ "
    };
    vector<string> crossing = {
        "     _____   _____    ____   _____  _____  ___  _   _   _____ ",
        "    / ____| |  __ \\  / __ \\ / ____|/ ____||_ _|| \\ | | / ____|",
        "   | |      | |__) || |  | |\\___  \\\\___  \\ | | |  \\| || |  __",
        "   | |____  |  _  / | |__| | ___) | ___) | | | | |\\  || |_|  | ",
        "    \\_____| |_| \\_\\  \\____/ |_____/|_____/|___||_| \\_| \\_____|"
    };
    string rc[] = {FG_GOLD,FG_ORANGE,FG_YELLOW,FG_ORANGE,FG_GOLD};
    string cc[] = {FG_GREEN,"\033[38;5;48m",FG_TEAL,"\033[38;5;48m",FG_GREEN};
    for(int i=0;i<5;i++){moveCursor(4+i,5);cout<<BOLD<<rc[i]<<road[i]<<RESET;cout.flush();sleep_ms(60);}
    for(int i=0;i<5;i++){moveCursor(10+i,3);cout<<BOLD<<cc[i]<<crossing[i]<<RESET;cout.flush();sleep_ms(60);}
    moveCursor(16,17);cout<<FG_LGRAY<<DIM<<"~  C + +   T E R M I N A L   G A M E  ~"<<RESET;cout.flush();sleep_ms(200);
    moveCursor(17,3);cout<<FG_DGRAY;for(int i=0;i<66;i++)cout<<'-';cout<<RESET;cout.flush();
}

void animateChicken(int passes) {
    const int BASE_ROW=27, START_COL=2, END_COL=66;
    const string frames[4]={" (>'-')>"," ^('-')^"," (>'-')>"," (v'-')v"};
    const int ro[4]={0,-1,0,1};
    moveCursor(BASE_ROW+2,START_COL);cout<<FG_DGRAY;
    for(int i=0;i<(END_COL-START_COL+10);i++)cout<<'~';
    cout<<RESET;cout.flush();
    for(int p=0;p<passes;p++){
        for(int col=START_COL;col<=END_COL;col+=2){
            int frame=(col/2)%4, row=BASE_ROW+ro[frame];
            for(int r=BASE_ROW-1;r<=BASE_ROW+1;r++){moveCursor(r,col-2);cout<<"          ";}
            moveCursor(row,col);cout<<FG_YELLOW<<BOLD<<frames[frame]<<RESET;
            if(frame==3){moveCursor(BASE_ROW+1,col+1);cout<<FG_LGRAY<<"* *"<<RESET;}
            cout.flush();sleep_ms(55);
        }
        for(int r=BASE_ROW-1;r<=BASE_ROW+1;r++){moveCursor(r,END_COL);cout<<"            ";}
        cout.flush();if(p<passes-1)sleep_ms(120);
    }
    for(int r=BASE_ROW-1;r<=BASE_ROW+2;r++){
        moveCursor(r,START_COL);
        cout<<"                                                                    ";
    }
    cout.flush();
}

void blinkingPrompt(int row, int col, const string& msg) {
    for(int b=0;b<6;b++){
        moveCursor(row,col);
        if(b%2==0) cout<<FG_CYAN<<BOLD<<msg<<RESET;
        else       cout<<FG_DGRAY<<DIM<<msg<<RESET;
        cout.flush();sleep_ms(420);
    }
    moveCursor(row,col);cout<<FG_CYAN<<BOLD<<msg<<RESET;cout.flush();
}

// ============================================================================
//  MAIN MENU
// ============================================================================
string mainMenu() {
    hideCursor();
    clearScreen();
    animateBorderSweep(); sleep_ms(100);
    printTitle();         sleep_ms(150);

    moveCursor(18, 4);  cout<<FG_WHITE<<BOLD<<"HOW TO PLAY"<<RESET;
    moveCursor(19, 4);  cout<<FG_LGRAY<<"Arrow/WASD"<<FG_DGRAY<<" -- Move player"<<RESET;
    moveCursor(20, 4);  cout<<FG_RED<<"#####"<<FG_DGRAY<<"      -- Dodge trucks!"<<RESET;
    moveCursor(21, 4);  cout<<FG_BROWN<<"====="<<FG_DGRAY<<"      -- Hop on logs"<<RESET;
    moveCursor(22, 4);  cout<<FG_GREEN<<"AAAA"<<FG_DGRAY<<"       -- Avoid alligators"<<RESET;
    moveCursor(23, 4);  cout<<FG_GOLD<<"x wins"<<FG_DGRAY<<"     -- Complete crossings!"<<RESET;
    moveCursor(18, 44); cout<<FG_PINK<<BOLD<<"CSDC102 Project"<<RESET;
    moveCursor(19, 44); cout<<FG_DGRAY<<"Language : "<<FG_TEAL<<"C++"<<RESET;
    moveCursor(20, 44); cout<<FG_DGRAY<<"Engine   : "<<FG_TEAL<<"Terminal"<<RESET;
    moveCursor(21, 44); cout<<FG_DGRAY<<"Compiler : "<<FG_TEAL<<"g++ 6.3.0"<<RESET;
    cout.flush();

    animateChicken(1); sleep_ms(150);
    blinkingPrompt(23, 18, "[ Press ENTER to Start ]");
    moveCursor(25, 1); showCursor();
    cin.sync(); cin.get();

    hideCursor(); clearScreen();
    for(int f=0;f<3;f++){
        moveCursor(12,24);cout<<FG_GREEN<<BOLD<<"*** ROAD CROSSING ***"<<RESET;cout.flush();sleep_ms(220);
        moveCursor(12,24);cout<<"                     ";cout.flush();sleep_ms(160);
    }
    sleep_ms(200); clearScreen(); showCursor();

    // ── Name entry ────────────────────────────────────────────────────────────
    clearScreen();
    cout << "\n\n";
    cout << FG_GOLD << BOLD;
    cout << "  ╔══════════════════════════════════╗\n";
    cout << "  ║       ENTER YOUR NAME            ║\n";
    cout << "  ╚══════════════════════════════════╝\n\n" << RESET;
    cout << FG_LGRAY << "  Your name will be saved to the leaderboard.\n\n" << RESET;
    cout << FG_CYAN  << "  Name: " << FG_WHITE;

    string playerName;
    getline(cin, playerName);
    while (!playerName.empty() && playerName.front() == ' ') playerName.erase(playerName.begin());
    while (!playerName.empty() && playerName.back()  == ' ') playerName.pop_back();
    if (playerName.empty()) playerName = "Anonymous";

    // ── Difficulty selection ──────────────────────────────────────────────────
    clearScreen();
    cout << "\n\n";
    cout << FG_GOLD << BOLD;
    cout << "  ╔══════════════════════════════════╗\n";
    cout << "  ║       SELECT DIFFICULTY          ║\n";
    cout << "  ╚══════════════════════════════════╝\n\n" << RESET;

    cout << FG_GREEN  << BOLD << "  [1] Easy    " << RESET
         << FG_DGRAY  << "- 2 trucks · no gators · slow · 3 wins\n\n"  << RESET;

    cout << FG_RED    << BOLD << "  [2] Hard    " << RESET
         << FG_DGRAY  << "- 3 trucks · gators appear · medium · 5 wins\n\n" << RESET;

    cout << FG_DGRAY  << "  ─────────────────────────────────────\n" << RESET;
    cout << FG_PINK   << BOLD << "  Ian Lastimosa\n" << RESET;
    cout << FG_ORANGE << BOLD << "  [3] (Extreme) " << RESET
         << FG_DGRAY  << "- 4 trucks · LOTS of gators · fast · 7 wins\n\n" << RESET;
    cout << FG_DGRAY  << "  ─────────────────────────────────────\n\n" << RESET;

    cout << FG_CYAN << "  Choice [1/2/3]: " << FG_WHITE;
    char c = '1';
    cin >> c;
    cin.ignore(1000, '\n');

    int choice = (c == '3') ? 2 : (c == '2') ? 1 : 0;
    g_diff = DIFFICULTIES[choice];

    clearScreen();
    cout << "\n\n  " << FG_GREEN << BOLD << "Difficulty: " << g_diff.name << RESET << "\n";
    cout << "  " << FG_CYAN << "Good luck, " << playerName << "!\n" << RESET;
    sleep_ms(1200);
    clearScreen();

    return playerName;
}

// ============================================================================
//  GAME OVER / WIN SCREEN
// ============================================================================
void gameOverScreen(const string& playerName, int wins, const string& cause) {
    clearScreen();
    cout << "\n\n";
    if (cause == "WIN") {
        cout << FG_GOLD << BOLD;
        cout << "  ╔══════════════════════════════════╗\n";
        cout << "  ║        🎉  YOU WIN!  🎉          ║\n";
        cout << "  ╚══════════════════════════════════╝\n\n" << RESET;
        cout << FG_GREEN << "  " << playerName << " crossed " << wins << " times!\n\n" << RESET;
    } else {
        cout << FG_RED << BOLD;
        cout << "  ╔══════════════════════════════════╗\n";
        cout << "  ║         💀  GAME OVER  💀        ║\n";
        cout << "  ╚══════════════════════════════════╝\n\n" << RESET;
        cout << FG_RED   << "  Cause: " << FG_WHITE << BOLD << cause << RESET << "\n";
        cout << FG_LGRAY << "  " << playerName << " completed " << wins << " crossing(s).\n\n" << RESET;
    }
    cout << FG_DGRAY << "  Difficulty: " << FG_WHITE << g_diff.name << RESET << "\n\n";
    addLeaderboardEntry(playerName, wins, g_diff.name, (cause == "WIN") ? "WIN" : cause);
    sleep_ms(1500);
}

// ============================================================================
//  RENDER MAP
// ============================================================================
void renderMap(Node* head, int playerX, int playerY) {
    Node* cur = head;
    int row = 0;
    while (cur) {
        string temp = cur->data;
        for (int i = 0; i < (int)temp.size(); i++) {
            if (row == playerY && i == playerX) {
                cout << FG_YELLOW << BOLD << 'P' << RESET;
            } else if (temp[i] == '#') {
                cout << FG_RED   << temp[i] << RESET;
            } else if (temp[i] == '=') {
                cout << FG_BROWN << temp[i] << RESET;
            } else if (temp[i] == 'A') {
                cout << FG_GREEN << BOLD << temp[i] << RESET;
            } else if (temp[i] == '~') {
                cout << FG_TEAL  << temp[i] << RESET;
            } else if (row == 0) {
                cout << FG_GOLD  << temp[i] << RESET;
            } else {
                cout << temp[i];
            }
        }
        cout << '\n';
        cur = cur->next;
        row++;
    }
}

// ============================================================================
//  MAIN
// ============================================================================
int main() {
    initConsole();
    srand(static_cast<unsigned>(time(nullptr)));
    loadLeaderboard();

    while (true) {

        string playerName = mainMenu();
        Node* head = buildRoad();

        int playerX   = LANE_WIDTH / 2;
        int playerY   = Total_Rows - 1;
        int wins      = 0;
        int lives     = 3;
        string deathCause = "";

        PlaySound("Songs/slimeyfox.wav", NULL, SND_FILENAME | SND_ASYNC | SND_LOOP);

        // ── Tick counter for decoupled obstacle speed ─────────────────────────
        // Every render frame = renderMs (always 80ms, fast for input feel).
        // Obstacles only shift once every obstacleEvery frames:
        //   Easy    → every 6 frames = every 480ms (slow)
        //   Hard    → every 4 frames = every 320ms (medium)
        //   Extreme → every 2 frames = every 160ms (fast)
        int tickCounter = 0;

        bool running = true;
        while (running) {

            system(CLEAR);

            // 1. Input — always polled every frame for responsiveness
            char input = keyboardInput();
            if (input == 'Q') { deathCause = "QUIT"; running = false; break; }

            if      (input == 'U' && playerY > 0)              playerY--;
            else if (input == 'D' && playerY < Total_Rows - 1) playerY++;
            else if (input == 'L' && playerX > 1)              playerX--;
            else if (input == 'R' && playerX < LANE_WIDTH)     playerX++;

            // 2. Log carry — only on obstacle ticks so player moves at obstacle speed
            tickCounter++;
            bool obstacleThisTick = (tickCounter % g_diff.obstacleEvery == 0);

            if (obstacleThisTick) {
                updatePlayerWithLog(head, playerX, playerY);
                shiftObstacles(head);
            }

            // 3. Win detection
            if (playerY == 0) {
                wins++;
                if (wins >= g_diff.winsNeeded) {
                    deathCause = "WIN";
                    running = false;
                    break;
                }
                freeList(head);
                head = buildRoad();
                playerX = LANE_WIDTH / 2;
                playerY = Total_Rows - 1;
                tickCounter = 0;
                clearScreen();
                cout << FG_GREEN << BOLD
                     << "\n  CROSSING! " << wins << "/" << g_diff.winsNeeded
                     << " -- Keep going!\n" << RESET;
                sleep_ms(700);
                continue;
            }

            // 4. Render
            renderMap(head, playerX, playerY);
            gameStatus(playerName, wins, lives, g_diff.winsNeeded);

            // 5. Collisions
            auto respawn = [&]() {
                freeList(head);
                head = buildRoad();
                playerX = LANE_WIDTH / 2;
                playerY = Total_Rows - 1;
                tickCounter = 0;
            };

            if (checkCollision(head, playerX, playerY)) {
                lives--;
                if (lives <= 0) { deathCause = "TRUCK"; running = false; }
                else {
                    clearScreen();
                    cout << FG_RED << BOLD << "\n  SQUISHED! Lives left: " << lives << "\n" << RESET;
                    sleep_ms(800); respawn();
                }
                continue;
            }
            if (checkAlligator(head, playerX, playerY)) {
                lives--;
                if (lives <= 0) { deathCause = "ALLIGATOR"; running = false; }
                else {
                    clearScreen();
                    cout << FG_GREEN << BOLD << "\n  EATEN! Lives left: " << lives << "\n" << RESET;
                    sleep_ms(800); respawn();
                }
                continue;
            }
            if (checkDrowned(head, playerX, playerY)) {
                lives--;
                if (lives <= 0) { deathCause = "DROWNED"; running = false; }
                else {
                    clearScreen();
                    cout << FG_TEAL << BOLD << "\n  DROWNED! Lives left: " << lives << "\n" << RESET;
                    sleep_ms(800); respawn();
                }
                continue;
            }

            sleep_ms(g_diff.renderMs);  // fixed 80ms — fast screen, slow obstacles
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
        if (again != 'y' && again != 'Y') break;
    }

    clearScreen();
    cout << FG_GOLD << BOLD << "\n  Thanks for playing Road Crossing!\n\n" << RESET;
    return 0;
}