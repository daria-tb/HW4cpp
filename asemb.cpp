#include <iostream>
using namespace std;

int main() {
    // Частина 1: if/else
    int var_a = 10; // або інше значення для перевірки
    int ebx = 0;

    if (var_a == 10) {
        ebx += 1;
    } else {
        ebx -= 1;
    }

    cout << "Після if/else: ebx = " << ebx << endl;

    // Частина 2: Цикл
    int ecx = 10;

    while (ecx != 0) {
        ecx--;
    }

    cout << "Після циклу: ecx = " << ecx << endl;

    return 0;
}
