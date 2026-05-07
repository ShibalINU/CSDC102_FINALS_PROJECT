#include <iostream>
#include <time.h>
#include <conio.h>
#include <Windows.h>

using namespace std;

struct Node{
    string data;
    Node* next;
};
typedef Node* NodePtr;





// if lanenum is odd = move right, if even = move left
string generateRoadLane(string border, int laneNum, int moveCounter){ // Generate random symbol, either $ or ., in a random position between 0 and 40
    int pos1, pos2, random, n1 = 0, n2 = 1;
    char road;
    string truck;

    random = (rand() % (n2 - n1 + 1)) + n1;
    if(random == 0){
        randsym = "$";
    }
    else{
        randsym = "."; 
    }
    
    pos1 = 1 + rand() % 40;
    pos2 = 1 + rand() % 40; 
    while(pos1 == pos2){
        pos2 = 1 + rand() % 40;
    }

    border[pos1] = randsym;
    border[pos2] = randsym;

    return border;
}

int main(){

    string border(42, ' '); // To generate a string with 40 characters
    border[0] = '|', border[41] = '|';



    return 0;
}