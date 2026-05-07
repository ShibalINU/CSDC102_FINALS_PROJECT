#include <windows.h>
#include <mmsystem.h>
#include <iostream>
#pragma comment(lib, "winmm.lib")

using namespace std;

int main()
{

    cout << "========= CHAYKIN RUDE KRUSIIG =========" << endl;
    cout << "Playing music...\n";
    PlaySound("songs/Lifetime.wav", NULL, SND_FILENAME | SND_ASYNC | SND_LOOP);
    cout << "Press Enter to Stop..\n";
    cin.get();
    cout << "Stopping music...\n";
    cout << "suf mga pogi\n";
    cout << "supot";
    cout << "miss u";
    cout << "kailan ka nagpatuli?: "; 
    cout << "i love sachi";
    cout << "sachi is the best";
    cout << "masarap si henry";
    cout << "sp pogi!";

    PlaySound(NULL, NULL, 0);

    return 0;
}
