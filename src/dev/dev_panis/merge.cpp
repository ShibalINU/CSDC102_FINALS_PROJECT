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
string generateRoadLane(int Trucks){
    string inner(LANE_WIDTH, '.'); // syempre si agihan dapat buffer man siya para maka agi kapadi

    //magamit kita srand syug sa truck para random si spawn niya 
    for(int i = 0; i < Trucks; i++) {

        // set ta kung gano kataba si truck tas i change ta ning -5 si width baga para ma change ning truck man ngaya
        int position = rand() % (LANE_WIDTH - 5); // random start

        for (int j = 0; j < 5; j++) {
            inner[position + j] = '#'; //watip emoji pwede
        }

    }

    return "|" + inner + "|"; 
    // si inner si obstacles;
}


// now is for the river and logs ~ this as water and this = as logs
// just gonna be the same to the truck method
string generateRiverLane(int Logs) {

    string inner(LANE_WIDTH, '~');

    for (int i = 0; i < Logs; i++) {

        int start = rand() % (LANE_WIDTH - 4);

        // 25% chance maging alligator
        bool isAlligator = (rand() % 4 == 0);

        for (int j = 0; j < 4; j++) {

            if (isAlligator) {
                inner[start + j] = 'A';
            }

            else {
                inner[start + j] = '=';
            }
        }
    }

    return "|" + inner + "|";
}

// ─── SHIFT LEFT ─────────────────────────────────────────────
void shiftLeft(string& lane) {

    char first = lane[1];

    for (int i = 1; i < LANE_WIDTH; i++) {
        lane[i] = lane[i + 1];
    }

    lane[LANE_WIDTH] = first;
}

// ─── SHIFT RIGHT ────────────────────────────────────────────
void shiftRight(string& lane) {

    char last = lane[LANE_WIDTH];

    for (int i = LANE_WIDTH; i > 1; i--) {
        lane[i] = lane[i - 1];
    }

    lane[1] = last;
}

Node* buildRoad(int Trucks, int Logs) {
    Node* head = nullptr; // this will point to the first node which is finish

    Node* tail = nullptr; // this points to the last node

    for (int row = 0; row < Total_Rows; row++){
        Node* newNode = new Node();
        newNode->next = nullptr;

        if (row == 0){

            newNode->data = "|========================================|";

        }

        else if (zoneMap[row] == ROAD){ // for the road duon sa strings to print the rows
            newNode->data = generateRoadLane(Trucks); // to print the truks in the string of ROAD 

        }

        else if (zoneMap[row] == RIVER) {
            newNode->data = generateRiverLane(Logs); // all the same
        }

        else if (zoneMap[row] == BUFFER) {
            newNode->data = generateBufferLane();
        }

        else if (zoneMap[row] == START) {
            newNode->data = "|                                        |";
        }

        if (head == nullptr) {
            head = newNode;
            tail = newNode;
        }

        else {
            //updates current tail
            tail->next = newNode;
            tail = newNode;
        }


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

void shiftObstacles(Node* head) {

    Node* current = head;

    int roadIndex = 0;
    int riverIndex = 0;

    for (int row = 0; row < Total_Rows && current != nullptr; row++) {

        // ROAD MOVEMENT
        if (zoneMap[row] == ROAD) {

            roadIndex++;

            // odd lanes → RIGHT
            if (roadIndex % 2 == 1) {
                shiftRight(current->data);
            }

            // even lanes → LEFT
            else {
                shiftLeft(current->data);
            }
        }

        // RIVER MOVEMENT
        else if (zoneMap[row] == RIVER) {

            riverIndex++;

            // odd river → RIGHT
            if (riverIndex % 2 == 1) {
                shiftRight(current->data);
            }

            // even river → LEFT
            else {
                shiftLeft(current->data);
            }
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

void updatePlayerWithLog(Node* head, int& playerX, int playerY) {

    if (zoneMap[playerY] != RIVER) {
        return;
    }

    int riverIndex = 0;

    for (int r = 0; r <= playerY; r++) {

        if (zoneMap[r] == RIVER) {
            riverIndex++;
        }
    }

    // odd river lanes move RIGHT
    if (riverIndex % 2 == 1) {
        playerX++;
    }

    // even river lanes move LEFT
    else {
        playerX--;
    }

    // clamp
    if (playerX < 1) {
        playerX = 1;
    }

    if (playerX > LANE_WIDTH) {
        playerX = LANE_WIDTH;
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

    PlaySound("songs/Lifetime.wav", NULL, SND_FILENAME | SND_ASYNC | SND_LOOP);

    initConsole();

    titleScreen();

    cout << FG_GREEN << "\nGame starting - good luck!\n" << RESET;

    srand(time(0));

    Node* head = buildRoad(2, 2);

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

            // draw player
            if (row == playerY) {
                temp[playerX] = 'P';
            }

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