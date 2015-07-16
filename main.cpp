#include <iostream>
#include "gpi/gpi.h"
#include "phitron/phitron.h"

using namespace std;

int main() {
    std::mt19937 rand(1);
    gpi::Population(10, 1, 20, 1, 1, 8, 32, rand);
    phi::P3 a;
    cout << "Hello World!" << endl;
    return 0;
}

