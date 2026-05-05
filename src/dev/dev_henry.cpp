#include <iostream>
using namespace std;

void titleScreen()
{

    string reset = "\033[0m";
    string yellow = "\033[1;33m";
    string cyan = "\033[1;36m";
    string white = "\033[1;37m";
    string gray = "\033[0;37m";
    string green = "\033[1;32m";
    string magenta = "\033[1;35m";

    system("cls");

    cout << yellow;
    cout << "=========================== TITLE SCREEN ===========================\n";
    cout << reset;

    cout << cyan;
    cout << "====================================================================\n";
    cout << yellow;
    cout << "|                                                                  |\n";
    cout << "|                _____    ____       _      _____                  |\n";
    cout << "|               |  __ \\  / __ \\     / \\    |  __ \\                 |\n";
    cout << "|               | |__) || |  | |   / _ \\   | |  | |                |\n";
    cout << "|               |  _  / | |  | |  / ___ \\  | |  | |                |\n";
    cout << "|               | | \\ \\ | |__| | / /   \\ \\ | |__| |                |\n";
    cout << "|               |_|  \\_\\ \\____/ /_/     \\_\\|_____/                 |\n";
    cout << "|                                                                  |\n";
    cout << green;
    cout << "|   _____  _____    ____    _____   _____  _____  _   _   _____    |\n";
    cout << "|  / ____||  __ \\  / __ \\  / ____| / ____||_   _|| \\ | | / ____|   |\n";
    cout << "| | |     | |__) || |  | || (___  | (___    | |  |  \\| || |  __    |\n";
    cout << "| | |     |  _  / | |  | | \\___ \\  \\___ \\   | |  | . ` || | |_ |   |\n";
    cout << "| | |____ | | \\ \\ | |__| | ____) | ____) | _| |_ | |\\  || |__| |   |\n";
    cout << "|  \\_____||_|  \\_\\ \\____/ |_____/ |_____/ |_____||_| \\_| \\_____|   |\n";
    cout << "|                                                                  |\n";
    cout << yellow;
    cout << "|                     C + +   T E R M I N A L                      |\n";
    cout << "|                                                                  |\n";
    cout << cyan;
    cout << "====================================================================\n";
    cout << reset;

    cout << magenta;
    cout << "Written for CSDC102 | Language: C++\n\n";
    cout << reset;

    cout << white;
    cout << "HOW TO PLAY:\n";
    cout << reset;

    cout << "- Move with Arrow Keys\n";
    cout << "- Dodge trucks (#####) in the ROAD ZONE\n";
    cout << "- Hop on logs (====) in the RIVER ZONE\n";
    cout << "- Reach the finish line 5 times to win!\n\n";

    cout << yellow;
    cout << "Press ENTER to start...";
    cout << reset;

    cin.ignore();
    cin.get();
}

int main()
{
    titleScreen();
    cout << "\nGame starting...\n";
    return 0;
}