#include "simulator.hpp"

int main(int argc, char** argv) {
    if (argc < 2) {
        cout << "Usage: ./sim <scheme_number>\n";
        cout << "Schemes:\n";
        cout << "   0 : Clustering\n";
        cout << "   1 : Switcher\n";
        cout << "   2 : Global Scheduling\n";
        cout << "   3 : Bonus\n";
        exit(0);
    }

    System system((SchedulerScheme)atoi(argv[1]));
    system.run();
    system.printStats();
}
