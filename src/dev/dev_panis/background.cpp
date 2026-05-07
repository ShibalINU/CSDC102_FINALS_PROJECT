#include <windows.h>
#include <mmsystem.h>
#include <iostream>
#pragma comment(lib, "winmm.lib")

using namespace std;

int main()
{

    cout << "========= CHAYKIN RUDE KRUSIIG =========" << endl;
    cout << "Playing music...\n";
    PlaySound("songs/Stardew.wav", NULL, SND_FILENAME | SND_ASYNC | SND_LOOP);
    cout << "Press Enter to Stop..\n";
    cin.get();
    cout << "Stopping music...\n";
    cout << "suf mga pogi\n";

    cout << "sp pogi!";

    PlaySound(NULL, NULL, 0);

    return 0;
}
