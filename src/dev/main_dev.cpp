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
 * ============================================================
 */

// ─── Standard Headers ────────────────────────────────────────────────────────
#include <iostream>  // cout, cin
#include <fstream>   // ifstream, ofstream  (leaderboard file I/O)
#include <string>    // string
#include <vector>    // vector              (leaderboard sorting)
#include <algorithm> // sort
#include <cstdlib>   // rand, srand
#include <ctime>     // time
#include <iomanip>   // setw

// ─── Platform-Specific Headers ───────────────────────────────────────────────
#ifdef _WIN32
#include <conio.h>   // _kbhit(), _getch()
#include <windows.h> // Sleep(), system("cls")
#define CLEAR_SCREEN system("cls")
#define SLEEP_MS(ms) Sleep(ms)
#else
#include <termios.h> // struct termios, tcgetattr, tcsetattr
#include <unistd.h>  // usleep, STDIN_FILENO
#include <fcntl.h>   // fcntl, F_SETFL, O_NONBLOCK
#define CLEAR_SCREEN system("clear")
#define SLEEP_MS(ms) usleep((ms) * 1000)
#endif

using namespace std;

// ─── Constants ───────────────────────────────────────────────────────────────
const int FIELD_WIDTH = 40;               // playable columns (between the '|' borders)
const int TOTAL_WIDTH = 42;               // full string width including two '|' borders
const int TOTAL_ROWS = 20;                // rows 0-19  (0 = finish line, 19 = player start)
const int LIVES_START = 3;                // player begins with 3 lives
const int WIN_CROSSINGS = 5;              // need 5 successful crossings to win
const string LB_FILE = "leaderboard.txt"; // leaderboard filename

// Zone-type identifiers stored in the zoneMap array
const int ZONE_FINISH = 0;
const int ZONE_ROAD = 1;
const int ZONE_BUFFER = 2;
const int ZONE_RIVER = 3;
const int ZONE_START = 4;

/*
 * zoneMap[row] — tells us what kind of zone each row belongs to.
 * This lets collision/movement logic query the row type without
 * re-parsing the lane string.
 *
 * Row layout (spec Section 3.2):
 *   0          → FINISH LINE
 *   1–5        → ROAD  (trucks move alternating L/R)
 *   6          → BUFFER
 *   7–8        → RIVER (logs move alternating R/L)
 *   9          → BUFFER
 *   10–14      → ROAD
 *   15         → BUFFER
 *   16–17      → RIVER
 *   18         → BUFFER
 *   19         → START (empty)
 */
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

// ─── Linked-List Node (one node = one lane row) ───────────────────────────────
/*
 * struct Node
 *   data : the 42-character string representing a single lane, e.g.
 *          "| #####...#####......#####.......... |"
 *   next : pointer to the node below this one (lower row number → higher on screen)
 *
 * The HEAD of the list is Row 0 (finish line).
 * The TAIL is Row 19 (player start).
 */
struct Node
{
    string data;
    Node *next;
};
typedef Node *NodePtr;

// ─── Leaderboard Entry ───────────────────────────────────────────────────────
/*
 * struct LeaderboardEntry
 *   name  : player's name as entered at the title screen
 *   score : number of successful crossings achieved that session
 */
struct LeaderboardEntry
{
    string name;
    int score;
};

// ═══════════════════════════════════════════════════════════════════════════════
//  SECTION A — Platform Input Helpers
// ═══════════════════════════════════════════════════════════════════════════════

#ifndef _WIN32
/*
 * enableRawMode() / disableRawMode()  [Linux/macOS only]
 *
 * Switches the terminal into "raw" mode so that key presses are
 * delivered immediately without waiting for Enter, and so that
 * arrow-key escape sequences can be read byte-by-byte.
 *
 * The original terminal settings are saved in 'orig' so they can
 * be restored on exit via disableRawMode().
 */
static struct termios orig_termios;

void enableRawMode()
{
    tcgetattr(STDIN_FILENO, &orig_termios);
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ICANON | ECHO); // no line-buffering, no echo
    raw.c_cc[VMIN] = 0;              // non-blocking read
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void disableRawMode()
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

/*
 * kbhit()  [Linux/macOS only]
 *
 * Returns true if at least one byte is waiting in stdin.
 * Mirrors the Windows _kbhit() behaviour.
 */
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

/*
 * getch()  [Linux/macOS only]
 *
 * Reads a single byte from stdin without waiting for Enter.
 * Mirrors the Windows _getch() behaviour.
 */
int getch()
{
    return getchar();
}
#endif

// ═══════════════════════════════════════════════════════════════════════════════
//  SECTION B — Lane String Helpers
// ═══════════════════════════════════════════════════════════════════════════════

/*
 * shiftLeft(lane)
 *
 * Scrolls the inner 40 characters of 'lane' one position to the left.
 * The leftmost inner character wraps around to the rightmost position.
 *
 * Example:  "| ABC... |"  →  "| BC...A |"
 *
 * Only positions 1–40 are touched; border characters at 0 and 41 stay fixed.
 * This simulates obstacles moving left (eastbound lanes).
 */
void shiftLeft(string &lane)
{
    char first = lane[1]; // save the leftmost playfield char
    for (int i = 1; i < FIELD_WIDTH; i++)
        lane[i] = lane[i + 1]; // slide everything one step left
    lane[FIELD_WIDTH] = first; // wrap first char to the right end
}

/*
 * shiftRight(lane)
 *
 * Scrolls the inner 40 characters of 'lane' one position to the right.
 * The rightmost inner character wraps around to the leftmost position.
 *
 * Example:  "| ABC... |"  →  "| .ABC.. |"  (last char comes to front)
 *
 * Simulates obstacles moving right (westbound lanes).
 */
void shiftRight(string &lane)
{
    char last = lane[FIELD_WIDTH]; // save the rightmost playfield char
    for (int i = FIELD_WIDTH; i > 1; i--)
        lane[i] = lane[i - 1]; // slide everything one step right
    lane[1] = last;            // wrap last char to the left end
}

/*
 * generateRoadLane(laneNum, numTrucks)
 *
 * Builds a 42-character road lane string with 'numTrucks' trucks placed
 * at random positions.  Each truck is exactly 5 '#' characters.
 * Empty spaces are filled with '.'.
 *
 * laneNum   — used to place trucks so they don't overlap (spacing logic)
 * numTrucks — 2 (Easy) or 3 (Hard)
 *
 * Returns: e.g. "| #####...#####..........#####.......... |"
 */
string generateRoadLane(int laneNum, int numTrucks)
{
    string lane(TOTAL_WIDTH, '.'); // fill inner area with dots
    lane[0] = '|';
    lane[41] = '|';

    int spacing = FIELD_WIDTH / (numTrucks + 1); // even initial spacing

    for (int t = 0; t < numTrucks; t++)
    {
        // Base position spread evenly, then add random jitter ±3
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

/*
 * generateRiverLane(laneNum, numLogs)
 *
 * Builds a 42-character river lane string.
 * Background is '~' (water = deadly).
 * 'numLogs' logs of 4 '=' characters are placed at random positions.
 *
 * laneNum — kept for future use / symmetry with generateRoadLane
 * numLogs — 2 (Easy) or 3 (Hard)
 *
 * Returns: e.g. "| ~~~~====~~~~~~~~~~~~~~~~====~~~~~~ |"
 */
string generateRiverLane(int laneNum, int numLogs)
{
    string lane(TOTAL_WIDTH, '~'); // fill inner area with water
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

/*
 * generateBufferLane()
 *
 * Returns a safe lane full of dots — no trucks, no water.
 * Players can rest here indefinitely.
 *
 * Returns: "|........................................|"
 */
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

/*
 * getLane(head, index)
 *
 * Traverses the singly-linked list from head and returns a pointer to
 * the node at position 'index' (0-based, where 0 = finish line / head).
 *
 * Since the list is singly-linked there is no random access — we must
 * walk the list every time we want a specific row.
 *
 * Returns: NodePtr to the desired node, or nullptr if out of range.
 */
NodePtr getLane(NodePtr head, int index)
{
    NodePtr cur = head;
    for (int i = 0; i < index && cur != nullptr; i++)
        cur = cur->next;
    return cur;
}

/*
 * buildRoad(numTrucks, numLogs)
 *
 * Allocates a fresh 20-node linked list representing the entire game board.
 * The head node is Row 0 (finish line) and the tail is Row 19 (player start).
 *
 * Each node's 'data' field is initialised according to its zoneMap type:
 *   ZONE_FINISH → finish line string
 *   ZONE_ROAD   → road lane with random trucks
 *   ZONE_RIVER  → river lane with random logs
 *   ZONE_BUFFER → all-dot safe lane
 *   ZONE_START  → empty start row
 *
 * numTrucks / numLogs are set by the difficulty selection.
 *
 * Returns: NodePtr pointing to the head (finish line) node.
 */
NodePtr buildRoad(int numTrucks, int numLogs)
{
    NodePtr head = nullptr;
    NodePtr tail = nullptr;

    int roadCount = 0;
    int riverCount = 0;

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

        if (head == nullptr)
        {
            head = newNode;
            tail = newNode;
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
 * shiftObstacles(head, moveCounter)
 *
 * Called once per game-loop tick to advance all moving obstacles one step.
 *
 * Road lanes: odd-index road lanes shift RIGHT, even-index LEFT.
 * River lanes: odd-index river lanes shift RIGHT, even-index LEFT.
 * Buffer & finish lanes are skipped entirely (no obstacles).
 *
 * moveCounter — passed in but currently unused; kept for future difficulty
 *               scaling where every Nth tick triggers a shift.
 */
void shiftObstacles(NodePtr head, int moveCounter)
{
    NodePtr cur = head;
    int roadIdx = 0;
    int riverIdx = 0;

    for (int row = 0; row < TOTAL_ROWS && cur != nullptr; row++)
    {
        if (zoneMap[row] == ZONE_ROAD)
        {
            roadIdx++;
            if (roadIdx % 2 == 1)
                shiftRight(cur->data);
            else
                shiftLeft(cur->data);
        }
        else if (zoneMap[row] == ZONE_RIVER)
        {
            riverIdx++;
            if (riverIdx % 2 == 1)
                shiftRight(cur->data);
            else
                shiftLeft(cur->data);
        }
        cur = cur->next;
    }
}

/*
 * freeList(head)
 *
 * Walks the linked list from head to tail and deletes every node.
 * Sets head to nullptr after deletion to prevent dangling pointers.
 *
 * Must be called before a new game starts or before the program exits
 * to avoid memory leaks.
 */
void freeList(NodePtr &head)
{
    while (head != nullptr)
    {
        NodePtr temp = head;
        head = head->next;
        delete temp;
    }
    head = nullptr; // guard against accidental use after free
}

// ═══════════════════════════════════════════════════════════════════════════════
//  SECTION D — Display
// ═══════════════════════════════════════════════════════════════════════════════

/*
 * displayRoad(head, playerX, playerY, playerName, lives, crossings)
 *
 * Clears the screen and re-draws the entire game board from scratch each frame.
 *
 * Algorithm:
 *   1. Print the title banner.
 *   2. Walk the linked list node by node (row by row).
 *   3. At the player's row (playerY), temporarily replace the character at
 *      column playerX with 'P', print the row, then restore the original char.
 *      (We restore it so the underlying lane data is never corrupted.)
 *   4. Print the status line at the bottom.
 *
 * playerX is a 1-based column index into the lane string (positions 1–40).
 * playerY is the row index (0–19).
 */
void displayRoad(NodePtr head,
                 int playerX, int playerY,
                 const string &playerName,
                 int lives, int crossings)
{
    CLEAR_SCREEN;

    // ── Title banner ──────────────────────────────────────────────────────────
    cout << "---------- Road Crossing Challenge ----------\n";
    cout << "Player: " << playerName
         << " | Lives: " << lives
         << " | Crossings: " << crossings << "\n";

    // ── Walk the list, overlaying 'P' at the player's position ───────────────
    NodePtr cur = head;
    for (int row = 0; row < TOTAL_ROWS && cur != nullptr; row++)
    {
        if (row == playerY)
        {
            // Make a local copy so we can restore it after printing
            char saved = cur->data[playerX];
            cur->data[playerX] = 'P';
            cout << cur->data << "\n";
            cur->data[playerX] = saved;
        }
        else
        {
            cout << cur->data << "\n";
        }
        cur = cur->next;
    }

    // ── Bottom status line ────────────────────────────────────────────────────
    cout << "Lives: " << lives << " | Crossings: " << crossings << "\n";
    cout << "(Arrow keys: move | Q: quit)\n";
}

// ═══════════════════════════════════════════════════════════════════════════════
//  SECTION E — Collision & Movement Logic
// ═══════════════════════════════════════════════════════════════════════════════

/*
 * checkCollision(head, playerX, playerY)
 *
 * Returns true if the player is currently occupying a '#' (truck) cell
 * in a ROAD zone lane.
 *
 * Called every frame AFTER shiftObstacles() so positions are always current.
 */
bool checkCollision(NodePtr head, int playerX, int playerY)
{
    if (zoneMap[playerY] != ZONE_ROAD)
        return false;
    NodePtr lane = getLane(head, playerY);
    if (lane == nullptr)
        return false;
    return lane->data[playerX] == '#';
}

/*
 * checkDrowned(head, playerX, playerY)
 *
 * Returns true if the player is in a RIVER zone and standing on '~' (water).
 * Standing on '=' (log) is safe and returns false.
 *
 * River rule: '=' = SAFE, '~' = DEADLY.
 */
bool checkDrowned(NodePtr head, int playerX, int playerY)
{
    if (zoneMap[playerY] != ZONE_RIVER)
        return false;
    NodePtr lane = getLane(head, playerY);
    if (lane == nullptr)
        return false;
    return lane->data[playerX] == '~';
}

/*
 * updatePlayerWithLog(head, playerX, playerY)
 *
 * If the player is standing on a river lane, they must drift with the log
 * BEFORE keyboard input is processed (so the player stays on the log naturally).
 *
 * Determines drift direction by the river lane index (odd = right, even = left),
 * mirroring the shiftObstacles() direction for that lane.
 *
 * Clamps playerX to [1, FIELD_WIDTH] so the player cannot drift off-screen.
 * If the player drifts off the edge they are still in water — checkDrowned()
 * will handle the life penalty.
 */
void updatePlayerWithLog(NodePtr head, int &playerX, int playerY)
{
    if (zoneMap[playerY] != ZONE_RIVER)
        return;

    // Count which river lane index this is (1-based)
    int riverIdx = 0;
    for (int r = 0; r <= playerY; r++)
        if (zoneMap[r] == ZONE_RIVER)
            riverIdx++;

    if (riverIdx % 2 == 1)
        playerX++; // odd river lanes drift RIGHT
    else
        playerX--; // even river lanes drift LEFT

    // Clamp so player stays within border columns
    if (playerX < 1)
        playerX = 1;
    if (playerX > FIELD_WIDTH)
        playerX = FIELD_WIDTH;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  SECTION F — Keyboard Input
// ═══════════════════════════════════════════════════════════════════════════════

/*
 * keyboardInput(playerX, playerY, quitFlag)
 *
 * Non-blocking keyboard check.  If a key is waiting, reads it and updates
 * the player position accordingly.
 *
 * Arrow-key encoding:
 *   Windows  : Up=72, Down=80, Left=75, Right=77  (after 0 or 0xE0 prefix)
 *   Linux/Mac: ESC [ A/B/C/D escape sequences
 *
 * Movement bounds are clamped so the player cannot walk through border walls
 * or above/below the play area.
 *
 * Sets quitFlag = true if the player presses 'Q' or 'q'.
 */
void keyboardInput(int &playerX, int &playerY, bool &quitFlag)
{
#ifdef _WIN32
    if (!_kbhit())
        return;

    int ch = _getch();
    if (ch == 0 || ch == 0xE0)
    { // arrow-key prefix on Windows
        ch = _getch();
        switch (ch)
        {
        case 72:
            if (playerY > 0)
                playerY--;
            break; // UP
        case 80:
            if (playerY < TOTAL_ROWS - 1)
                playerY++;
            break; // DOWN
        case 75:
            if (playerX > 1)
                playerX--;
            break; // LEFT
        case 77:
            if (playerX < FIELD_WIDTH)
                playerX++;
            break; // RIGHT
        }
    }
    else if (ch == 'q' || ch == 'Q')
    {
        quitFlag = true;
    }
#else
    if (!kbhit())
        return;

    int ch = getch();
    if (ch == 27)
    { // ESC → start of arrow sequence
        int ch2 = getch();
        if (ch2 == '[')
        {
            int ch3 = getch();
            switch (ch3)
            {
            case 'A':
                if (playerY > 0)
                    playerY--;
                break; // UP
            case 'B':
                if (playerY < TOTAL_ROWS - 1)
                    playerY++;
                break; // DOWN
            case 'D':
                if (playerX > 1)
                    playerX--;
                break; // LEFT
            case 'C':
                if (playerX < FIELD_WIDTH)
                    playerX++;
                break; // RIGHT
            }
        }
    }
    else if (ch == 'q' || ch == 'Q')
    {
        quitFlag = true;
    }
#endif
}

// ═══════════════════════════════════════════════════════════════════════════════
//  SECTION G — Leaderboard (File I/O)
// ═══════════════════════════════════════════════════════════════════════════════

/*
 * saveScore(name, score)
 *
 * Appends one line to leaderboard.txt in the format:
 *   "<name> <score>\n"
 *
 * Uses ofstream with ios::app so previous entries are never overwritten.
 * If the file does not exist it is created automatically.
 */
void saveScore(const string &name, int score)
{
    ofstream file(LB_FILE, ios::app);
    if (file.is_open())
    {
        file << name << " " << score << "\n";
        file.close();
    }
}

/*
 * showLeaderboard()
 *
 * Reads every entry from leaderboard.txt into a vector of LeaderboardEntry,
 * sorts the vector by score descending, then prints the top 5.
 *
 * Comparison function: higher score first; ties are kept in file order.
 */
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
        file.close();
    }

    // Sort descending by score
    sort(entries.begin(), entries.end(),
         [](const LeaderboardEntry &a, const LeaderboardEntry &b)
         {
             return a.score > b.score;
         });

    cout << "\n===== LEADERBOARD (Top 5) =====\n";
    int limit = min((int)entries.size(), 5);
    for (int i = 0; i < limit; i++)
    {
        cout << " " << (i + 1) << ". "
             << left << setw(20) << entries[i].name
             << " — " << entries[i].score << " crossing(s)\n";
    }
    if (entries.empty())
        cout << " (No entries yet)\n";
    cout << "================================\n";
}

// ═══════════════════════════════════════════════════════════════════════════════
//  SECTION H — Title Screen, Player Setup, Difficulty
// ═══════════════════════════════════════════════════════════════════════════════

/*
 * showTitleScreen()
 *
 * Clears the screen and displays the ASCII art banner plus controls guide.
 * Waits for the player to press ENTER before continuing.
 */
void showTitleScreen()
{
    CLEAR_SCREEN;
    cout << R"(
  ============================================================
   ____                 _    ____               _
  |  _ \ ___   __ _  __| |  / ___|_ __ ___  ___(_)_ __   __ _
  | |_) / _ \ / _` |/ _` | | |   | '__/ _ \/ __| | '_ \ / _` |
  |  _ < (_) | (_| | (_| | | |___| | | (_) \__ \ | | | | (_| |
  |_| \_\___/ \__,_|\__,_|  \____|_|  \___/|___/_|_| |_|\__, |
                              C h a l l e n g e          |___/
  ============================================================
   A terminal Crossy Road game  |  CSDC102 C++
  ============================================================

  HOW TO PLAY:
    Arrow Keys : UP / DOWN / LEFT / RIGHT
    Q          : Quit

    Dodge trucks  (#####) in the ROAD zones.
    Ride logs     (====)  in the RIVER zones.
    Rest on dots  (....)  — Buffer lanes are safe!
    Reach the TOP 5 times to WIN!

  Press ENTER to continue...
)";
    // Flush any leftover input, then wait for Enter
    cin.ignore(1000, '\n');
    cin.get();
}

/*
 * getPlayerName()
 *
 * Prompts the player to type their name and returns it as a string.
 * Trims to a single word (no spaces) for clean leaderboard formatting.
 */
string getPlayerName()
{
    string name;
    cout << "\n Enter your name: ";
    cin >> name;
    cin.ignore(1000, '\n'); // clear remainder of the line
    return name;
}

/*
 * chooseDifficulty(gameSpeed, numTrucks, numLogs)
 *
 * Presents Easy / Hard options and sets three output parameters by reference:
 *   gameSpeed — milliseconds to sleep per game-loop tick
 *   numTrucks — trucks per road lane (2 = Easy, 3 = Hard)
 *   numLogs   — logs per river lane (2 = Easy, 3 = Hard)
 */
void chooseDifficulty(int &gameSpeed, int &numTrucks, int &numLogs)
{
    cout << "\n SELECT DIFFICULTY\n";
    cout << "  1. Easy — speed: 180ms, 2 trucks/logs per lane\n";
    cout << "  2. Hard — speed: 100ms, 3 trucks/logs per lane\n";
    cout << "  Enter choice (1 or 2): ";

    int choice;
    cin >> choice;
    cin.ignore(1000, '\n');

    if (choice == 2)
    {
        gameSpeed = 100;
        numTrucks = 3;
        numLogs = 3;
    }
    else
    {
        gameSpeed = 180;
        numTrucks = 2;
        numLogs = 2;
    }
}

/*
 * gameStatus(lives, crossings, playerName)
 *
 * Prints a brief status update.  Called after a life is lost or a crossing
 * is completed.  (Most live status is in the displayRoad header line.)
 */
void gameStatus(int lives, int crossings, const string &playerName)
{
    if (lives <= 0)
    {
        cout << "\n Game Over, " << playerName
             << "! You were hit too many times.\n";
    }
    else if (crossings >= WIN_CROSSINGS)
    {
        cout << "\n Congratulations, " << playerName
             << "! You crossed " << WIN_CROSSINGS << " times and WON!\n";
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
//  SECTION I — Main Game Loop
// ═══════════════════════════════════════════════════════════════════════════════

/*
 * runGame(playerName, gameSpeed, numTrucks, numLogs)
 *
 * Orchestrates one full game session:
 *   1. Build the linked list road.
 *   2. Enter the main loop:
 *        a. Drift player with log (if on river).
 *        b. Read keyboard input.
 *        c. Shift obstacles.
 *        d. Check win (reached Row 0).
 *        e. Check collision (truck).
 *        f. Check drowning (water).
 *        g. Handle life loss / respawn.
 *        h. Render the board.
 *        i. Sleep to control frame rate.
 *   3. When the loop ends (win, lose, or quit), save the score, show
 *      the leaderboard, and free the linked list.
 *
 * Parameters:
 *   playerName — displayed on-screen and saved to leaderboard
 *   gameSpeed  — ms sleep per tick (controls difficulty speed)
 *   numTrucks  — trucks per road lane
 *   numLogs    — logs per river lane
 */
void runGame(const string &playerName,
             int gameSpeed, int numTrucks, int numLogs)
{
    srand((unsigned)time(nullptr)); // seed RNG once per game session

    int lives = LIVES_START;
    int crossings = 0;
    int playerX = 20;             // centre column (1-based)
    int playerY = TOTAL_ROWS - 1; // Row 19 — start at bottom
    int moveCounter = 0;
    bool quitFlag = false;

    NodePtr head = buildRoad(numTrucks, numLogs);

#ifndef _WIN32
    enableRawMode(); // switch terminal to raw (non-blocking) mode
#endif

    // ── Main Game Loop ────────────────────────────────────────────────────────
    while (lives > 0 && crossings < WIN_CROSSINGS && !quitFlag)
    {

        // 1. Carry player along with current log (before input, so player moves
        //    naturally with the log before they can react)
        updatePlayerWithLog(head, playerX, playerY);

        // 2. Read keyboard — non-blocking, updates playerX / playerY
        keyboardInput(playerX, playerY, quitFlag);

        // 3. Advance all obstacles by one step
        moveCounter++;
        shiftObstacles(head, moveCounter);

        // 4. Check win condition: player reached the finish line (row 0)
        if (playerY == 0)
        {
            crossings++;
            if (crossings < WIN_CROSSINGS)
            {
                // Rebuild the road for the next crossing and reset player
                freeList(head);
                head = buildRoad(numTrucks, numLogs);
                playerX = 20;
                playerY = TOTAL_ROWS - 1;
            }
        }

        // 5. Check truck collision
        else if (checkCollision(head, playerX, playerY))
        {
            lives--;
            playerX = 20;
            playerY = TOTAL_ROWS - 1; // respawn at start
        }

        // 6. Check drowning in river
        else if (checkDrowned(head, playerX, playerY))
        {
            lives--;
            playerX = 20;
            playerY = TOTAL_ROWS - 1; // respawn at start
        }

        // 7. Render
        displayRoad(head, playerX, playerY, playerName, lives, crossings);

        // 8. Throttle frame rate
        SLEEP_MS(gameSpeed);
    }

#ifndef _WIN32
    disableRawMode(); // restore normal terminal mode
#endif

    // ── End-of-Game ──────────────────────────────────────────────────────────
    gameStatus(lives, crossings, playerName);
    saveScore(playerName, crossings);
    showLeaderboard();
    freeList(head); // release all nodes — no memory leaks
}

// ═══════════════════════════════════════════════════════════════════════════════
//  SECTION J — Entry Point
// ═══════════════════════════════════════════════════════════════════════════════

/*
 * main()
 *
 * Top-level driver.  Runs:
 *   1. Title screen
 *   2. Player name entry
 *   3. Difficulty selection
 *   4. Game loop  (via runGame)
 *   5. Play-again prompt
 *
 * The play-again loop lets the player start a fresh game without restarting
 * the executable.  Each iteration rebuilds the linked list and resets state.
 */
int main()
{
    showTitleScreen();

    string playerName = getPlayerName();

    char playAgain = 'y';
    while (playAgain == 'y' || playAgain == 'Y')
    {
        int gameSpeed, numTrucks, numLogs;
        chooseDifficulty(gameSpeed, numTrucks, numLogs);

        runGame(playerName, gameSpeed, numTrucks, numLogs);

        cout << "\n Play again? (y/n): ";
        cin >> playAgain;
        cin.ignore(1000, '\n');
    }

    cout << "\n Thanks for playing, " << playerName << "! See you next time.\n\n";
    return 0;
}
