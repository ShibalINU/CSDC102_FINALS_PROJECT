#include <iostream>
#include <string>
#include <cstdlib>
#include <ctime>

using namespace std;

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
    FINISH,
    ROAD,
    ROAD,
    ROAD,
    BUFFER,
    RIVER,
    RIVER,
    ROAD,
    ROAD,
    RIVER,
    BUFFER,
    ROAD,
    BUFFER,
    RIVER,
    RIVER,
    ROAD,
    ROAD,
    BUFFER,
    START
};


const int LANE_WIDTH = 40; // yung loob ng map to width nung mapa syug
const int Full_width = 42; // borders


// now lets make the loops or obstacles for thy maps
string generateBufferLane() {
    string lane = "|"; // for the border to guys para may istitik

    for (int i = 0; i < LANE_WIDTH; i++) { // to equal the dots for the specific width lang for the obstacles
        lane += '.'; // padagdag ng padagdag na dots
    }

    return lane; // result
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

        for (int j = 0; j < 4; j++){
            inner[start + j] = '=';
        }
    }

    return "|" + inner + "|";
}

Node* buildRoad(int Trucks, int Logs) {
    Node* head = nullptr; // this will point to the first node which is finish

    Node* tail = nullptr; // this points to the last node

    for (int row =0; row < Total_Rows; row++){
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
            newNode->data = "| |";
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
    //lets send the random number gen muna 
    srand(time(0));

    //print zonemap
    printZoneMap();

    cout << "Building road [Easy Mode]" << endl;
    Node* head = buildRoad(2, 2);
    printList(head);

    cout << "Fetch row 7 should be road ata or river: " << endl;
    Node* row7 = getLane(head, 7);
    if (row7 != nullptr) {
        cout << "Row 7 data: " << row7->data << endl;
    }

    cout << "Fetch Row 0 (should be finish): " << endl;
    Node* row0 = getLane(head, 0);
    if (row0 != nullptr) {
        cout << "Row 0 data: " << row0->data << endl;
    }

    cout << "\nFreeing linked lis.." << endl; // to confirm no dangling pointer
    freeList(head);

    if(head == nullptr) {
        cout << "freeList() success - head is now nullptr. YEHEYY" << endl;
    }

    return 0;


}