#include <iostream>
#include <string>
#include <cstdlib>
#include <ctime>

#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

using namespace std;


#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>

#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#define CLEAR "cls"
void sleep_ms(int ms) { Sleep(ms); }

// Switches terminal to UTF-8 and enables ANSI color processing
void initConsole()
{
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

// ─── CURSOR CONTROL ─────────────────────────────────────────────────────────
void moveCursor(int row, int col)
{
    cout << "\033[" << row << ";" << col << "H";
}
void hideCursor() { cout << "\033[?25l"; }
void showCursor() { cout << "\033[?25h"; }
void clearScreen() { system(CLEAR); }

// ─── BORDER SWEEP ANIMATION ─────────────────────────────────────────────────
void animateBorderSweep()
{
    const int W = 70;
    const int H = 24;

    cout << FG_TEAL << BOLD;

    moveCursor(1, 1);
    for (int i = 0; i < W; i++)
    {
        cout << "=";
        cout.flush();
        sleep_ms(8);
    }

    for (int r = 2; r <= H; r++)
    {
        moveCursor(r, W);
        cout << "|";
        cout.flush();
        sleep_ms(12);
    }

    for (int i = W; i >= 1; i--)
    {
        moveCursor(H + 1, i);
        cout << "=";
        cout.flush();
        sleep_ms(8);
    }

    for (int r = H; r >= 2; r--)
    {
        moveCursor(r, 1);
        cout << "|";
        cout.flush();
        sleep_ms(12);
    }

    // Corners
    moveCursor(1, 1);
    cout << "+";
    moveCursor(1, W);
    cout << "+";
    moveCursor(H + 1, 1);
    cout << "+";
    moveCursor(H + 1, W);
    cout << "+";
    cout << RESET;
    cout.flush();
}

// ─── TITLE FADE-IN ──────────────────────────────────────────────────────────
void printTitle()
{
    // "ROAD"
    vector<string> road;
    road.push_back("            ____     ___       _      ____  ");
    road.push_back("           |  _ \\   / _ \\     / \\    |  _ \\ ");
    road.push_back("           | |_) | | | | |   / _ \\   | | | |");
    road.push_back("           |  _ <  | |_| |  / ___ \\  | |_| |");
    road.push_back("           |_| \\_\\  \\___/  /_/   \\_\\ |____/ ");

    // "CROSSING" — C-R-O-S-S-I-N-G verified
    vector<string> crossing;
    crossing.push_back("     _____   _____    ____   _____  _____  ___  _   _   _____ ");
    crossing.push_back("    / ____| |  __ \\  / __ \\ / ____|/ ____||_ _|| \\ | | / ____|");
    crossing.push_back("   | |      | |__) || |  | |\\___  \\\\___  \\ | | |  \\| || |  __");
    crossing.push_back("   | |____  |  _  / | |__| | ___) | ___) | | | | |\\  || |_|  | ");
    crossing.push_back("    \\_____| |_| \\_\\  \\____/ |_____/|_____/|___||_| \\_| \\_____|");

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
    moveCursor(16, 17);
    cout << FG_LGRAY << DIM << "~  C + +   T E R M I N A L   G A M E  ~" << RESET;
    cout.flush();
    sleep_ms(200);

    // Divider
    moveCursor(17, 3);
    cout << FG_DGRAY;
    for (int i = 0; i < 66; i++)
        cout << "-";
    cout << RESET;
    cout.flush();
}

// ─── CHICKEN BOUNCE ANIMATION ───────────────────────────────────────────────
// Runs BELOW the main box (box bottom is row 25) — never overlaps any text
void animateChicken(int passes)
{
    const int BASE_ROW = 27; // below box which ends at row 25
    const int START_COL = 2;
    const int END_COL = 66;
    const int FRAME_COUNT = 4;

    const string frames[4] = {
        " (>'-')>",  // standing
        "  ^('-')^", // mid-jump (one row up)
        " (>'-')>",  // standing
        " (v'-')v"   // squish land (one row down)
    };
    const int row_offset[4] = {0, -1, 0, 1};

    // Draw a ground line for the chicken to run on
    moveCursor(BASE_ROW + 2, START_COL);
    cout << FG_DGRAY;
    for (int i = 0; i < (END_COL - START_COL + 10); i++)
        cout << "~";
    cout << RESET;
    cout.flush();

    for (int p = 0; p < passes; p++)
    {
        for (int col = START_COL; col <= END_COL; col += 2)
        {
            int frame = (col / 2) % FRAME_COUNT;
            int row = BASE_ROW + row_offset[frame];

            // Erase previous chicken position (3 rows around base)
            moveCursor(BASE_ROW - 1, col - 2);
            cout << "          ";
            moveCursor(BASE_ROW, col - 2);
            cout << "          ";
            moveCursor(BASE_ROW + 1, col - 2);
            cout << "          ";

            // Draw chicken
            moveCursor(row, col);
            cout << FG_YELLOW << BOLD << frames[frame] << RESET;

            // Dust puff on landing
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
            moveCursor(r, END_COL);
            cout << "            ";
        }
        cout.flush();

        if (p < passes - 1)
            sleep_ms(120);
    }

    // Clean up all chicken rows + ground line after animation finishes
    for (int r = BASE_ROW - 1; r <= BASE_ROW + 2; r++)
    {
        moveCursor(r, START_COL);
        cout << "                                                                    ";
    }
    cout.flush();
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

    moveCursor(ROW, COL);
    cout << FG_CYAN << BOLD << msg << RESET;
    cout.flush();
}

// ─── INFO PANEL ─────────────────────────────────────────────────────────────
void printInfo()
{
    moveCursor(18, 4);
    cout << FG_WHITE << BOLD << "HOW TO PLAY" << RESET;
    moveCursor(19, 4);
    cout << FG_LGRAY << "Arrow Keys" << FG_DGRAY << " -- Move your chicken" << RESET;
    moveCursor(20, 4);
    cout << FG_RED << "#####" << FG_DGRAY << "  -- Dodge trucks!" << RESET;
    moveCursor(21, 4);
    cout << FG_LBLUE << "=====" << FG_DGRAY << "  -- Hop on logs" << RESET;
    moveCursor(22, 4);
    cout << FG_GREEN << "x5 wins" << FG_DGRAY << "     -- Win the game!" << RESET;

    moveCursor(18, 44);
    cout << FG_PINK << BOLD << "CSDC102 Project" << RESET;
    moveCursor(19, 44);
    cout << FG_DGRAY << "Language : " << FG_TEAL << "C++" << RESET;
    moveCursor(20, 44);
    cout << FG_DGRAY << "Engine   : " << FG_TEAL << "Terminal / ANSI" << RESET;
    moveCursor(21, 44);
    cout << FG_DGRAY << "Compiler : " << FG_TEAL << "g++ 6.3.0" << RESET;

    cout.flush();
}

// ─── WAIT FOR ENTER ─────────────────────────────────────────────────────────
void waitForEnter()
{
    cin.sync();
    while (cin.get() != '\n')
    {
    }
}

// ─── TITLE SCREEN ────────────────────────────────────────────────────────────
void titleScreen()
{
    hideCursor();
    clearScreen();

    animateBorderSweep();
    sleep_ms(100);

    printTitle();
    sleep_ms(150);

    printInfo();
    sleep_ms(100);

    animateChicken(1);
    sleep_ms(150);

    blinkingPrompt();

    moveCursor(25, 1);
    showCursor();
    waitForEnter();

    hideCursor();
    clearScreen();
    for (int f = 0; f < 3; f++)
    {
        moveCursor(12, 24);
        cout << FG_GREEN << BOLD << "*** GAME STARTING ***" << RESET;
        cout.flush();
        sleep_ms(220);
        moveCursor(12, 24);
        cout << "                     ";
        cout.flush();
        sleep_ms(160);
    }
    sleep_ms(300);
    clearScreen();
    showCursor();
}

// ─── MAIN ────────────────────────────────────────────────────────────────────

struct Node {
    string data; // Para sa mga lanes
    Node* next; // Pointer for the lanes 

};


enum ZoneType { // so we can use either stuct naman but with enum we wont need to put int or char pa it directly assigns it
    FINISH, //Phising layn gais
    ROAD, // agihan ni padi
    RIVER, // pwede mag swimming igdi
    BUFFER, // Dito ka magpahinga, sa tabi ko
    START // Syempre ano ganon
};

const int Total_Rows = 20; // pwede man dawa pira bente muna

ZoneType zoneMap[Total_Rows] = {  // Sabi sainyo eh parang struct lang
    FINISH, // 0
    ROAD, // 1
    ROAD, // 2
    ROAD, // 3
    ROAD, // 4
    ROAD, // 5
    BUFFER, // 6
    RIVER, // *7
    RIVER, // 7
    BUFFER, // 8
    ROAD, // 9
    ROAD, // 10
    ROAD, // 11
    ROAD, // 12
    ROAD, // 13
    BUFFER, // 14
    RIVER, // 15
    RIVER, // 16
    BUFFER, // 17
    START // 18
};


const int LANE_WIDTH = 40; // yung loob ng map to width nung mapa syug
const int Full_width = 42; // borders


// now lets make the loops or obstacles for thy maps
string generateBufferLane() {
    string lane = "|"; // for the border to guys para may istitik

    for (int i = 0; i < LANE_WIDTH; i++) { // to equal the dots for the specific width lang for the obstacles
        lane += '.'; // padagdag ng padagdag na dots
    }

    return lane + "|"; // result
}

// para sa road naman guys, may buff ako ni gom guys na may mga truck
string generateRoadLane(int laneNum) {

    // now (2 or 3) / 2
    int trucks;
    if (laneNum % 2 == 1)
        trucks = (rand() % 2 == 0) ? 2 : 3; // odd lane → 2 or 3
    else
        trucks = 2;  // even lane → always 2

    string inner(LANE_WIDTH, '.');
    const int TRUCK_SIZE = 5;
    int sectionSize = LANE_WIDTH / trucks;

    for (int i = 0; i < trucks; i++) {
        int secStart = i * sectionSize + 1;
        int secEnd   = (i + 1) * sectionSize - TRUCK_SIZE - 1;
        if (secEnd < secStart) secEnd = secStart;

        int position = secStart + rand() % (secEnd - secStart + 1);
        for (int j = 0; j < TRUCK_SIZE; j++)
            inner[position + j] = '#';
    }

    return "|" + inner + "|";
}


// now is for the river and logs ~ this as water and this = as logs
// just gonna be the same to the truck method

// RIVER LANE 
// laneNum decides odd (3 or 5 logs) or even (4 logs)  
// One guaranteed alligator per lane, section-based so logs never stack
string generateRiverLane(int laneNum) {

    int logs;
    if (laneNum % 2 == 1)
        logs = (rand() % 2 == 0) ? 3 : 5; // odd lane → 3 or 5
    else
        logs = 4;                           // even lane → 4

    string inner(LANE_WIDTH, '~');
    const int OBS_SIZE = 4;
    int sectionSize = LANE_WIDTH / logs;
    int alligatorIdx = rand() % logs;      // one alligator guaranteed

    for (int i = 0; i < logs; i++) {
        int secStart = i * sectionSize + 1;
        int secEnd   = (i + 1) * sectionSize - OBS_SIZE - 1;
        if (secEnd < secStart) secEnd = secStart;

        int start = secStart + rand() % (secEnd - secStart + 1);
        char ch = (i == alligatorIdx) ? 'A' : '=';

        for (int j = 0; j < OBS_SIZE; j++)
            inner[start + j] = ch;
    }

    return "|" + inner + "|";
}

// BUILD ROAD 
// Tracks roadIndex and riverIndex to pass the correct lane number
Node* buildRoad(ZoneType zoneMap[]) {
    Node* head     = nullptr;
    Node* tail     = nullptr;
    int roadIndex  = 0;
    int riverIndex = 0;

    for (int row = 0; row < Total_Rows; row++) {
        Node* newNode  = new Node();
        newNode->next  = nullptr;

        if (row == 0)
            newNode->data = "|========================================|";

        else if (zoneMap[row] == ROAD) {
            roadIndex++;
            newNode->data = generateRoadLane(roadIndex);   // pass lane number
        }
        else if (zoneMap[row] == RIVER) {
            riverIndex++;
            newNode->data = generateRiverLane(riverIndex); // pass lane number
        }
        else if (zoneMap[row] == BUFFER)
            newNode->data = generateBufferLane();

        else if (zoneMap[row] == START)
            newNode->data = "|                                        |";

        if (head == nullptr) { head = newNode; tail = newNode; }
        else                 { tail->next = newNode; tail = newNode; }
    }

    return head;
}

Node* getLane(Node* head, int index) {

    Node* current = head;

    // if we hit nullptr before reaching the index then it is out of range a
    for (int i = 0; i < index; i++){
        if (current == nullptr) {
            cout << "ERROR: getLane() - index " << index << " is out of range!" << endl;
            return nullptr;
        }

        current = current->next; // to move to next node

    }

    return current; // now current points to the index
}

//  SHIFT LEFT
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

        // Resize on wrap 
        // Wrap is detected when the obstacle's new position crosses the edge
        if (newStart + o.len > LANE_WIDTH) {
            int minSize, maxSize;// you can change the sizes of the obstacles going to the left
            if      (o.ch == '#') { minSize = 5; maxSize = 5; } // trucks
            else if (o.ch == '=') { minSize = 4; maxSize = 4; } // logs
            else if (o.ch == 'A') { minSize = 5; maxSize = 5; } // alligators
            else                  { minSize = 5; maxSize = 5; }

            int delta = (rand() % 3) - 1; // randomly -1, 0, or +1
            o.len = max(minSize, min(maxSize, o.len + delta));
        }
        

        for (int j = 0; j < o.len; j++)
            inner[(newStart + j) % LANE_WIDTH] = o.ch;
    }

    lane = "|" + inner + "|";
}

//  SHIFT RIGHT
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

        // ── Resize on wrap 
        if (newStart + o.len > LANE_WIDTH) {
            int minSize, maxSize;//you can change the sizes of the obstacles going to the right here
            if      (o.ch == '#') { minSize = 5; maxSize = 5; }
            else if (o.ch == '=') { minSize = 4; maxSize = 4; }
            else if (o.ch == 'A') { minSize = 4; maxSize = 4; }
            else                  { minSize = 1; maxSize = 5; }

            int delta = (rand() % 3) - 1;
            o.len = max(minSize, min(maxSize, o.len + delta));
        }
        // ===----------

        for (int j = 0; j < o.len; j++)
            inner[(newStart + j) % LANE_WIDTH] = o.ch;
    }

    lane = "|" + inner + "|";
}

//  SHIFT OBSTACLES ====------
void shiftObstacles(Node* head) {

    Node* current = head;
    int roadIndex = 0;
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

bool checkAlligator(Node* head, int playerX, int playerY) {

    if (zoneMap[playerY] != RIVER) {
        return false;
    }

    Node* lane = getLane(head, playerY);

    if (lane == nullptr) {
        return false;
    }

    return lane->data[playerX] == 'A';
}

void updatePlayerWithLog(Node* head, int& playerX, int playerY)// for henry and kat pala to lmao welcome guys
{
    if (zoneMap[playerY] != RIVER) return;

    int riverIndex = 0;

    for (int r = 0; r <= playerY; r++) {
        if (zoneMap[r] == RIVER) {
            riverIndex++;
        }
    }

    Node* lane = getLane(head, playerY);
    if (!lane) return;

    bool onLog = (lane->data[playerX] == '=');

    // ONLY move player if standing on log
    if (!onLog) return;

    // determine direction of this river lane
    bool movesRight = (riverIndex % 2 == 1);

    if (movesRight) {
        playerX++;
        if (playerX > LANE_WIDTH) {
            playerX = 1;   // wrap to left side
        }
    }
    else {
        playerX--;
        if (playerX < 1) {
            playerX = LANE_WIDTH; // wrap to right side
        }
    }
} 

// FREELIST pero ako hindi pa FREE

void freeList(Node*& head) {
    Node* current = head; // starts at the top of the sting the list

    while (current != nullptr) { //walks to every node and deletes

        Node* nextNode = current->next; // to save the pointer before ma delete sabi ni YT

        delete current;
        current = nextNode; // move to the saved node


    }

    head = nullptr;
}

void printZoneMap() { // dont mind this for only temporary printing
    for (int i = 0; i < Total_Rows; i++) {
        cout << "ROW " << i << ": ";

        switch (zoneMap[i]) {
            case FINISH: 
            cout << "FINISH";
            break;

            case ROAD: 
            cout << "ROAD";
            break;

            case RIVER: 
            cout << "RIVER";
            break;

            case BUFFER: 
            cout << "BUFFER";
            break;

            case START: 
            cout << "START";
            break;
        }
        cout << endl;
    }
    cout << "---------------------\n" << endl;

}

//ignore this i just wanna check how the link list print from head to tail
void printList(Node* head) {
    Node* current = head;
    int rowIndex = 0;

    while (current != nullptr) {
        cout << "ROW " << rowIndex << ": " << current->data << endl;
        current = current->next;
        rowIndex++;
    }

    cout << "---------------------------\n" << endl;
}


// now for the test
int main () {

    PlaySound("SC:/Users/Aljosh Mendoza/OneDrive/문서/GitHub/CSDC102_FINALS_PROJECT/src/dev_Mendoza_yoyo/Songs/slimeyfox.mp3", NULL, SND_FILENAME | SND_ASYNC | SND_LOOP);// doesn't wwork still finding out why

    initConsole();

    titleScreen();

    cout << FG_GREEN << "\nGame starting - good luck!\n" << RESET;

    srand(time(0));

    Node* head = buildRoad(zoneMap);

    int playerX = 20;
    int playerY = 18;

    while (true) {

        system(CLEAR);

        // ─── PLAYER INPUT ─────────────────────
        if (_kbhit()) {

            int key = _getch();

            // arrow keys return 224 first
            if (key == 224) {

                key = _getch();

                // UP
                if (key == 72 && playerY > 0) {
                    playerY--;
                }

                // DOWN
                else if (key == 80 && playerY < Total_Rows - 1) {
                    playerY++;
                }

                // LEFT
                else if (key == 75 && playerX > 1) {
                    playerX--;
                }

                // RIGHT
                else if (key == 77 && playerX < LANE_WIDTH) {
                    playerX++;
                }
            }
        }

        // move player with logs
        updatePlayerWithLog(head, playerX, playerY);

        // move obstacles
        shiftObstacles(head);

        // display map
        Node* current = head;
        int row = 0;

        while (current != nullptr) {

            string temp = current->data;

            // color alligators
        for (int i = 0; i < (int)temp.size(); i++) {
            if (row == playerY && i == playerX) {
                cout << FG_YELLOW << 'P' << RESET;
            }
            else if (temp[i] == 'A') {
                cout << FG_GREEN << 'A' << RESET;
            }
            else {
                cout << temp[i];
            }
        }
        
        cout << endl; 

        current = current -> next; 
        row++; 
    }
        // alligator collision
        if (checkAlligator(head, playerX, playerY)) {

            cout << FG_RED << "\nYOU GOT EATEN BY AN ALLIGATOR!\n" << RESET;

            break;
        }

        sleep_ms(200);
    }

    freeList(head);

    cout << "\nPress Enter to Stop..\n";
    cin.get();

    PlaySound(NULL, NULL, 0);

    return 0;
}