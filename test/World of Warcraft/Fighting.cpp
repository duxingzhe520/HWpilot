#include <cstring>
#include <iostream>
#include <map>
#include <string>
#include <vector>

// =========================================================
// 1. Common.hpp 内容
// =========================================================
const int NUM_WARRIORS = 5;
const int NUM_WEAPONS = 3;
const std::string WARRIORS[NUM_WARRIORS] = {"dragon", "ninja", "iceman", "lion", "wolf"};
const std::string WEAPONS[NUM_WEAPONS] = {"sword", "bomb", "arrow"};
int numCities;

void printStdTime(int hour, int minute) {
    if (hour < 0 || hour > 100 || minute < 0 || minute >= 60) {
        return;
    }
    if (hour / 10 == 0) {
        if (minute / 10 > 0) {
            std::cout << "00" << hour << ":" << minute << " ";
        } else {
            std::cout << "00" << hour << ":" << "0" << minute << " ";
        }
    } else if (hour / 10 < 10) {
        if (minute / 10 > 0) {
            std::cout << "0" << hour << ":" << minute << " ";
        } else {
            std::cout << "0" << hour << ":" << "0" << minute << " ";
        }
    } else {
        if (minute / 10 > 0) {
            std::cout << hour << ":" << minute << " ";
        } else {
            std::cout << hour << ":" << "0" << minute << " ";
        }
    }
}

// =========================================================
// 2. 前置声明 (解决循环依赖)
// =========================================================
class HeadQuarter;
class Warrior;

// =========================================================
// 3. City.hpp 内容
// =========================================================
class City {
    int id;
    bool isRedHeadQuarter;
    bool isBlueHeadQuarter;
    Warrior* redWarrior;
    Warrior* blueWarrior;

public:
    City(int id) {
        this->id = id;
        isRedHeadQuarter = false;
        isBlueHeadQuarter = false;
    }

    void setRedQ() {
        isRedHeadQuarter = true;
    }

    void setBlueQ() {
        isBlueHeadQuarter = true;
    }

    void addWarrior(Warrior* warrior) {
        std::string camp = warrior->getCamp();
        if (camp == "red") {
            redWarrior = warrior;
        } else if (camp == "blue") {
            blueWarrior = warrior;
        } else {
            std::cerr << "No such headQuarter named " << camp << '\n';
        }
    }

    void moveWarrior(Warrior* warrior) {
        // TODO:
        std::string camp = warrior->getCamp();
        if (camp == "red") {
        } else if (camp == "blue") {
        } else {
            std::cerr << "No such headQuarter named " << camp << '\n';
        }
    }
};

// =========================================================
// 4. Warrior.hpp 内容 (保留类定义)
// =========================================================

class Weapon {
protected:
    int index;  // 这时候访问武器的名称只需要WEAPONS[index]即可
    int power;
    Warrior* master;

public:
    Weapon(int index, Warrior* master) {
        this->index = index;
        this->master = master;
    }
};

class Sword0 : public Weapon {
public:
    Sword0(int index, Warrior* master) : Weapon(index, master) {
        power = master->getAttackPower() / 5;
    }
};

class Bomb1 : public Weapon {  // 一旦使用就没了
    int selfHurt;
    bool hasUsed;

public:
    Bomb1(int index, Warrior* master) : Weapon(index, master) {
        power = master->getAttackPower() * 2 / 5;
        selfHurt = power / 2;
        hasUsed = false;
    }

    bool getHasUsed() {
        return hasUsed;
    }
};

class Arrow2 : public Weapon {  // 用两次就没了
    int useNum;

public:
    Arrow2(int index, Warrior* master) : Weapon(index, master) {
        power = master->getAttackPower() * 3 / 10;
        useNum = 2;
    }

    int getUseNum() {
        return useNum;
    }
};

class Warrior {
protected:
    HeadQuarter* headQ;
    int id;
    // std::string name;
    int index;
    int energy;
    int cityIndex;  // 访问所在的city只需要cities[index]便得到所在City的指针
    int moveIndex;  // 红方的moveIndex = +1，蓝方的moveIndex = -1
    std::vector<Weapon*> weapon;
    int attackPower;

public:
    Warrior(int id, HeadQuarter* headQ, int energyNeed, int powerHas) {
        this->id = id;
        this->headQ = headQ;
        this->energy = energyNeed;
        if (headQ->getName() == "red") {
            cityIndex = 0;
            moveIndex = 1;
        } else if (headQ->getName() == "blue") {
            cityIndex = numCities + 1;
            moveIndex = -1;
        }
        attackPower = powerHas;
    }

    // virtual void printBehaviour() { return; }

    // 声明保留在此，实现被移动到 HeadQuarter 定义之后以解决依赖
    virtual void reportBorn(int hour, int minute);

    int getAttackPower() {
        return attackPower;
    }

    std::string getCamp() {
        return headQ->getName();
    }

    void march() {}

    int getNameIndex() {
        return index;
    }

    virtual bool getRunAway() {
        return false;
    }

    virtual void runAway() {}
};

class Dragon0 : public Warrior {
    double morale;

public:
    Dragon0(int id, HeadQuarter* headQ, int remainEnergy, int energyNeed, int powerHas) : Warrior(id, headQ, energyNeed, powerHas) {
        // weaponIndex = id % 3;
        morale = (double)remainEnergy / (double)energyNeed;
        index = 0;
    }

    // void printBehaviour() override {
    //     // printf("It has a %s,and it's morale is %.2lf\n", WEAPONS[weaponIndex].c_str(), morale);
    // }
};

class Ninja1 : public Warrior {
protected:
    int otherWeaponIndex;

public:
    Ninja1(int id, HeadQuarter* headQ, int energyNeed, int powerHas) : Warrior(id, headQ, energyNeed, powerHas) {
        // weaponIndex = id % 3;
        otherWeaponIndex = (id + 1) % 3;
        index = 1;
    }

    // void printBehaviour() override {
    //     // printf("It has a %s and a %s\n", WEAPONS[weaponIndex].c_str(), WEAPONS[otherWeaponIndex].c_str());
    // }
};

class Iceman2 : public Warrior {
public:
    Iceman2(int id, HeadQuarter* headQ, int energyNeed, int powerHas) : Warrior(id, headQ, energyNeed, powerHas) {
        // weaponIndex = id % 3;
        index = 2;
    }

    // void printBehaviour() override { printf("It has a %s\n", WEAPONS[weaponIndex].c_str()); }
};

class Lion3 : public Warrior {
    static int loyaltyReduce;
    int loyalty;

public:
    Lion3(int id, HeadQuarter* headQ, int remainEnergy, int energyNeed, int powerHas) : Warrior(id, headQ, energyNeed, powerHas) {
        loyalty = remainEnergy;
        index = 3;
    }

    static void setLoyaltyReduce(int reduceValue) {
        loyaltyReduce = reduceValue;
    }

    void reportBorn(int hour, int minute) override {
        Warrior::reportBorn(hour, minute);
        printf("It's loyalty is %d\n", loyalty);
    }

    void runAway() override {}

    bool getRunAway() override {
        return loyalty <= 0;
    }

    // void printBehaviour() override { printf("It's loyalty is %d\n", loyalty); }
};

// C++ 语法要求：类内声明的 static 变量必须在类外进行定义
int Lion3::loyaltyReduce = 0;

class Wolf4 : public Warrior {
public:
    Wolf4(int id, HeadQuarter* headQ, int energyNeed, int powerHas) : Warrior(id, headQ, energyNeed, powerHas) {
        index = 4;
    }

    void robWeapon() {}
};

// =========================================================
// 5. HeadQuarter.hpp 内容
// =========================================================
class HeadQuarter {
public:
    HeadQuarter(int energy, int* energyConsume, int* attackPower) {
        this->energy = energy;
        int minNum = 0x3f3f3f3f;
        for (int i = 0; i < 5; ++i) {
            this->energyConsume[i] = energyConsume[i];
            this->attackPower[i] = attackPower[i];
        }
        warriors.push_back(NULL);  // 占位warriors[0]，这样warriors[warriorID]即可找到对应武士
    }

    void reportEnergy() {}

    bool getHasStopCreateWarrior() {
        return hasStopCreateWarrior;
    }

    void born(int hour, int minute) {
        if (energy < energyNeed(nextWarriorIndex) || hasStopCreateWarrior) {
            hasStopCreateWarrior = true;
            return;
        }
        switch (nextWarriorIndex) {
            case 0: {
                Warrior* dragon = new Dragon0(warriorID, this, energy - energyNeed(0), energyNeed(0), powerHas(0));
                dragon->reportBorn(hour, minute);
                warriors.push_back(dragon);
                break;
            }
            case 1: {
                Warrior* ninja = new Ninja1(warriorID, this, energyNeed(1), powerHas(1));
                ninja->reportBorn(hour, minute);
                warriors.push_back(ninja);
                break;
            }
            case 2: {
                Warrior* iceman = new Iceman2(warriorID, this, energyNeed(2), powerHas(2));
                iceman->reportBorn(hour, minute);
                warriors.push_back(iceman);
                break;
            }
            case 3: {
                Warrior* lion = new Lion3(warriorID, this, energy - energyNeed(3), energyNeed(3), powerHas(3));
                lion->reportBorn(hour, minute);
                warriors.push_back(lion);
                break;
            }
            case 4: {
                Warrior* wolf = new Wolf4(warriorID, this, energyNeed(4), powerHas(4));
                wolf->reportBorn(hour, minute);
                warriors.push_back(wolf);
                break;
            }
            default:
                std::cerr << "Warrior index exceed 5! " << '\n';
        }
        nextWarriorIndex = updateIndex(nextWarriorIndex);
        warriorID += 1;
        energy -= energyNeed(0);
    }

    std::string getName() {
        return name;
    }

    void lionRunAway(int hour, int minute) {
        for (auto warrior : warriors) {
            if (warrior->getNameIndex() == 3) {
                if (warrior->getRunAway()) {
                    warrior->runAway();
                }
            }
        }
    }

protected:
    /** The key is the index of warriors.  */
    std::map<int, int> energyConsume;
    std::map<int, int> attackPower;
    std::string name;
    int energy;
    std::vector<Warrior*> warriors;
    bool hasStopCreateWarrior;
    int warriorID = 1;
    int nextWarriorIndex;

    int energyNeed(int index) {
        if (index < 0 || index >= NUM_WARRIORS) {
            std::cerr << "Warrior's index must in {0, 1, 2, 3, 4}! " << '\n';
            return 0;
        }
        return energyConsume.at(index);
    }

    int powerHas(int index) {
        if (index < 0 || index >= NUM_WARRIORS) {
            std::cerr << "Warrior's index must in {0, 1, 2, 3, 4}" << '\n';
            return 0;
        }
        return attackPower.at(index);
    }

    virtual int updateIndex(int thisIndex) {
        return -1;
    }
};

class RedHeadQuarter : public HeadQuarter {
public:
    RedHeadQuarter(int energy, int* energyConsume, int* attackPower) : HeadQuarter(energy, energyConsume, attackPower) {
        name = "red";
        nextWarriorIndex = 2;
        hasStopCreateWarrior = false;
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
    BlueHeadQuarter(int energy, int* energyConsume, int* attackPower) : HeadQuarter(energy, energyConsume, attackPower) {
        nextWarriorIndex = 3;
        name = "blue";
        hasStopCreateWarrior = false;
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

// =========================================================
// 6. 延迟实现的成员函数（彻底解决循环依赖）
// =========================================================
void Warrior::reportBorn(int hour, int minute) {
    printStdTime(hour, minute);
    printf("%s %s %d born\n", headQ->getName().c_str(), WARRIORS[index].c_str(), id);
}

// =========================================================
// 7. main.cpp 内容
// =========================================================
std::vector<City*> cities;

int main() {
    int caseNum = 0;
    std::cin >> caseNum;
    int cnt = 1;
    while (cnt <= caseNum) {
        std::cout << "Case:" << cnt << '\n';
        cnt++;

        int energy, hour, minute, lionLoyaltyReduce;
        std::cin >> energy >> numCities >> lionLoyaltyReduce >> minute;
        Lion3::setLoyaltyReduce(lionLoyaltyReduce);
        hour = minute / 60;
        minute = minute % 60;

        int energyConsume[5], attackPower[5];
        for (int i = 0; i < 5; ++i) {
            std::cin >> energyConsume[i];
        }
        for (int i = 0; i < 5; ++i) {
            std::cin >> attackPower[i];
        }

        cities.clear();
        for (int i = 0; i < numCities + 2; ++i) {
            cities.push_back(new City(i));
        }
        cities[0]->setRedQ();
        cities[numCities + 1]->setBlueQ();

        RedHeadQuarter redQ(energy, energyConsume, attackPower);
        BlueHeadQuarter blueQ(energy, energyConsume, attackPower);

        for (int h = 0; h <= hour; ++h) {
            int m = 0;
            while (1) {
                if (m >= 60) {
                    break;
                }
                if (h == hour && m > minute) {
                    break;
                }
                switch (m) {
                    case 0:
                        redQ.born(hour, minute);
                        blueQ.born(hour, minute);
                        m = 5;
                        break;
                    case 5:
                        redQ.lionRunAway(hour, minute);
                        blueQ.lionRunAway(hour, minute);
                        m = 10;
                        break;
                    case 10:
                        m = 35;
                        break;
                    case 35:
                        m = 40;
                        break;
                    case 40:
                        m = 50;
                        break;
                    case 50:
                        m = 55;
                        break;
                    case 55:
                        m = 60;
                        break;
                    default:
                        std::cerr << "Time out of limit! " << '\n';
                }
            }
        }
        for (int i = 0; i < numCities + 2; ++i) {
            delete cities[i];
        }
    }
    return 0;
}