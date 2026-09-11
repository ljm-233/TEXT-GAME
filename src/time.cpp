#include <ctime>
#include <iostream>
#include "time_utils.h"

using namespace std;

void getime(){
    time_t now = time(nullptr);
    cout << ctime(&now);
}