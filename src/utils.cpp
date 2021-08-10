#include <iostream>

using std::cout;

int randint(int min, int max) {
    return rand() % (max - min) + min;
}

void print() {
    cout<<'\n';
}

template<typename T, typename ...TAIL>
void print(const T &t, TAIL... tail) {
    cout << t << ' ';
    print(tail...);
}
