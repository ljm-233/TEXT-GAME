#include <ctime>
#include <iostream>
#include <fstream>

using namespace std;

int main(){
  time_t time = time(nullptr);
  cout << ctime(&time);
  return 0
}
