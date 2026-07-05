#include <iostream>
#include <string>
#include <map>
#include <cstring>

const int NUM_WARRIORS = 5;
void printStdTime(int n);

class HeadQuarter {
    public:
        HeadQuarter(int energy, int* energyConsume) {
            this->energy = energy;
            int minNum = 0x3f3f3f3f;
            for (int i = 0; i < 5; ++i) {
                this->energyConsume[i] = energyConsume[i];
                minNum = std::min(minNum, energyConsume[i]);
            }
            this->minEnergyNeed = minNum;
        }

        bool getHasStop() {
            return hasStop;
        }

        void nextItem(int time) {
            if (energy >= energyNeed(nextWarriorIndex)) {
                energy -= energyNeed(nextWarriorIndex);
                existWarrior[nextWarriorIndex] += 1;
                printItem(time, nextWarriorIndex, existWarrior[nextWarriorIndex]);
                nextWarriorIndex = updateIndex(nextWarriorIndex);
                warriorID += 1;
            } else if (energy >= getMinEnergyNeed()){
                nextWarriorIndex = updateIndex(nextWarriorIndex);
                nextItem(time);
            } else {
                hasStop = true;
                printStop(time);
            }
        }

    private:
        /** The key is the index of warriors.  */
        std::map<int, int> energyConsume;

        const std::string WARRIORS[NUM_WARRIORS] = {"dragon", "ninja", "iceman", "lion", "wolf"};

        int minEnergyNeed;

    protected:
        std::string name;
        int energy;
        int existWarrior[NUM_WARRIORS];
        bool hasStop;
        int warriorID = 1;
        int nextWarriorIndex;

        int getMinEnergyNeed() {
            return minEnergyNeed;
        }

        std::string warriorAt(int index) {
            if (index < 0 || index >= NUM_WARRIORS) {
                return "";
            }
            return WARRIORS[index];
        }

        int energyNeed(int index) {
            if (index < 0 || index >= NUM_WARRIORS) {
                return 0;
            }
            return energyConsume.at(index);
        }

        void printItem(int time, int warriorIndex, int warriorNum) {
            printStdTime(time);
            printf("%s %s %d born with strength %d,%d %s in %s headquarter\n",name.c_str(), warriorAt(warriorIndex).c_str(), warriorID, energyNeed(warriorIndex), existWarrior[warriorIndex], warriorAt(warriorIndex).c_str(), name.c_str());
        }

        virtual int updateIndex(int thisIndex) {
            return 0;
        }

        void printStop(int time) {
            printStdTime(time);
            printf("%s headquarter stops making warriors\n", name.c_str());
        }
};

class RedHeadQuarter : public HeadQuarter {
    public:
        RedHeadQuarter(int energy, int* energyConsume) : HeadQuarter(energy, energyConsume) {
            name = "red";
            nextWarriorIndex = 2;
            hasStop = false;
            memset(existWarrior, 0, sizeof(existWarrior));
        }

    private:     
        int updateIndex(int thisIndex) override {
            switch (thisIndex) {
                case 0:
                    return 2;
                case 1:
                    return 0;
                case 2:
                    return 3;
                case 3:
                    return 4;
                case 4:
                    return 1;
                default:
                    return 0;
            }
        }
};

class BlueHeadQuarter : public HeadQuarter {
    public:
        BlueHeadQuarter(int energy, int* energyConsume) : HeadQuarter(energy, energyConsume) {
            nextWarriorIndex = 3;
            name = "blue";
            hasStop = false;
            memset(existWarrior, 0, sizeof(existWarrior));
        }

    private:
        int updateIndex(int thisIndex) override {
            switch (thisIndex) {
                case 0:
                    return 1;
                case 1:
                    return 2;
                case 2:
                    return 4;
                case 3:
                    return 0;
                case 4:
                    return 3;
                default:
                    return 0;
            }
        }
};

void printStdTime(int n) {
    if (n < 0 || n > 100) {
        return;
    }
    if (n / 10 == 0) {
        std::cout << "00" << n << " ";
    } else if (n / 10 < 10) {
        std::cout << "0" << n << " ";
    } else {
        std::cout << n << " ";
    }
}

int main() {
    int caseNum = 0;
    std::cin >> caseNum;
    int cnt = 1;
    while (cnt <= caseNum) {
        std::cout << "Case:" << cnt << '\n';
        int energy = 0;
        int energyConsume[5];
        std::cin >> energy;
        for(int i = 0; i < 5; ++i) {
            std::cin >> energyConsume[i];
        }

        RedHeadQuarter r(energy, energyConsume);
        BlueHeadQuarter b(energy, energyConsume);
        int time = 0;
        while (!r.getHasStop() || !b.getHasStop()) {
            if (time > 100) {
                std::cout << "Too many function calling! ";
            }
            if (!r.getHasStop()) {
                r.nextItem(time);
            }
            if (!b.getHasStop()) {
                b.nextItem(time);
            }
            time += 1;
        }
        cnt++;
    }
    return 0;
}