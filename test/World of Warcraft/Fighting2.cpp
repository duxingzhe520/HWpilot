#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include <algorithm>
#include <sstream>
using namespace std;

enum WeaponType { SWORD = 0, BOMB = 1, ARROW = 2 };
enum WarriorType { DRAGON = 0, NINJA = 1, ICEMAN = 2, LION = 3, WOLF = 4 };
enum Team { RED = 0, BLUE = 1, NON = 2 };
enum Flag { REDFLAG = 0, BLUEFLAG = 1, EMPTY = 2 };
enum FightingResult { SUCCESS = 0, FAILURE = 1, TIE = 2 };

int init_hp[5];
int init_attack[5];
string w_names[5] = {"dragon", "ninja", "iceman", "lion", "wolf"};
string t_names[2] = {"red", "blue"};

int spawn_order[2][5] = {
    {ICEMAN, LION, WOLF, NINJA, DRAGON},  // Red
    {LION, DRAGON, NINJA, ICEMAN, WOLF}   // Blue
};

string get_time_str(int minutes) {
    char buf[10];
    snprintf(buf, sizeof(buf), "%03d:%02d", minutes / 60, minutes % 60);
    return string(buf);
}

class Weapon {
public:
    WeaponType type;
    int uses;  // -1 表示无限可用
    int power;

    Weapon(WeaponType t, int init_power = 0) : type(t) {
        if (type == SWORD) {
            uses = -1;
            power = init_power;
        } else if (type == BOMB) {
            uses = 1;
        } else if (type == ARROW) {
            uses = 3;
        }
    }
};

// 武器的战斗排序
bool combat_cmp(const Weapon& a, const Weapon& b) {
    if (a.type != b.type) {
        return a.type < b.type;
    }
    if (a.type == ARROW) {
        return a.uses < b.uses;
    }
    return false;
}

// 武器的缴获/抢夺排序
bool loot_cmp(const Weapon& a, const Weapon& b) {
    if (a.type != b.type) {
        return a.type < b.type;
    }
    if (a.type == ARROW) {
        return a.uses > b.uses;
    }
    return false;
}

class Warrior {
public:
    int id;
    int hp;
    int attack;
    WarriorType type;
    Team team;
    int city;
    int loyalty;
    vector<Weapon> weapons;
    int combat_idx;
    int steps;
    double morale;
    bool shoot_victory;

    Warrior(int _id, WarriorType _type, Team _team, int _loyalty = 0, double _morale = 0)
        : id(_id), type(_type), team(_team), loyalty(_loyalty), combat_idx(0), steps(0), morale(_morale), shoot_victory(false) {
        hp = init_hp[type];
        attack = init_attack[type];
        if (team == RED) {
            city = 0;
        } else {
            city = -1;  // 表示为最后一个城市
        }
    }

    void add_weapon(WeaponType w_type) {
        Weapon weapon(w_type, attack * 2 / 10);
        if (weapon.type == SWORD && weapon.power == 0) {
            return;
        }
        weapons.push_back(weapon);
    }

    void sort_weapons_combat() {
        sort(weapons.begin(), weapons.end(), combat_cmp);
    }

    void sort_weapons_loot() {
        sort(weapons.begin(), weapons.end(), loot_cmp);
    }

    void remove_broken_weapons() {
        vector<Weapon> temp;  // 只收集最后uses != 0的武器
        for (auto& w : weapons) {
            if (w.uses != 0) {
                temp.push_back(w);
            }
        }
        weapons = temp;
    }

    void loot(Warrior* enemy) {
        enemy->sort_weapons_loot();
        for (auto& w : enemy->weapons) {
            weapons.push_back(w);
        }
    }

    bool has_usable_weapon() {
        for (auto& w : weapons) {
            if (w.uses != 0) {
                return true;
            }
        }
        return false;
    }

    bool has_bomb() {
        for (auto& w : weapons) {
            if (w.type == BOMB && w.uses != 0) {
                return true;
            }
        }
        return false;
    }

    bool has_weapon_type(WeaponType type) {
        for (auto& w : weapons) {
            if (w.type == type && w.uses != 0 && !(w.type == SWORD && w.power <= 0)) {
                return true;
            }
        }
        return false;
    }

    void remove_bomb() {
        for (auto it = weapons.begin(); it != weapons.end(); ++it) {
            if (it->type == BOMB && it->uses != 0) {
                weapons.erase(it);
                return;
            }
        }
    }

    // 如果该武士拥有sword，则返回其sword的攻击力值；否则返回0
    int sword_power() {
        for (auto& w : weapons) {
            if (w.type == SWORD && w.uses != 0) {
                return w.power;
            }
        }
        return 0;
    }

    void update_sword_power(int new_power) {
        for (auto it = weapons.begin(); it != weapons.end();) {
            auto& w = *it;
            if (w.type == SWORD && w.uses != 0) {
                w.power = new_power;
                if (new_power <= 0) {
                    it = weapons.erase(it);
                } else {
                    ++it;
                }
            } else {
                ++it;
            }
        }
    }

    void change_morale(int time_min, FightingResult result) {
        if (type != DRAGON) {
            return;
        }

        if (result == SUCCESS) {
            morale += 0.2;
        } else if (result == FAILURE) {
            return;
        } else if (result == TIE) {
            morale -= 0.2;
        } else {
            cerr << "What are you fucking calling the shit function?!\n";
        }

        if (morale > 0.8) {
            if (team == BLUE) {
                cout << get_time_str(time_min) << " blue dragon " << id << " yelled in city " << city << "\n";
            } else if (team == RED) {
                cout << get_time_str(time_min) << " red dragon " << id << " yelled in city " << city << "\n";
            } else {
                cerr << "wocaonimalegebidenizaixiachuanshenmcanshu!!!!\n";
            }
        }
    }
};

class Headquarter {
public:
    Team team;
    int elements;  // 所剩的生命元数量
    // int spawn_idx;
    int warrior_count;  // 武士降生id
    bool stopped;       // 是否停止降生武士
    int enemy_num;      // 已经到达本方司令部的敌人数量

    Headquarter(Team t, int M) : team(t), elements(M), warrior_count(0), stopped(false), enemy_num(0) {}

    Warrior* spawn(int time_min) {
        // if (stopped) {
        //     return nullptr;
        // }

        WarriorType w_type = (WarriorType)spawn_order[team][warrior_count % 5];
        if (elements >= init_hp[w_type]) {
            elements -= init_hp[w_type];
            warrior_count++;
            // spawn_idx++;

            int loy = (w_type == LION) ? elements : 0;
            double mor = (w_type == DRAGON) ? (double)elements / (double)init_hp[DRAGON] : 0.0;
            Warrior* w = new Warrior(warrior_count, w_type, team, loy, mor);

            if (w_type == DRAGON || w_type == ICEMAN) {
                w->add_weapon((WeaponType)(w->id % 3));
            } else if (w_type == NINJA) {
                w->add_weapon((WeaponType)(w->id % 3));
                w->add_weapon((WeaponType)((w->id + 1) % 3));
            }

            cout << get_time_str(time_min) << " " << t_names[team] << " " << w_names[w_type] << " " << w->id << " born\n";
            if (w_type == LION) {
                cout << "Its loyalty is " << w->loyalty << "\n";
            } else if (w_type == DRAGON) {
                cout << "Its morale is " << fixed << setprecision(2) << w->morale << "\n";
            }
            return w;
        } else {
            // stopped = true;
            return nullptr;
        }
    }
};

class City {
public:
    Warrior* red;
    Warrior* blue;
    Flag flag;
    int city_elements;
    pair<Team, Team> last_winner;
    City() : red(nullptr), blue(nullptr), flag(EMPTY), city_elements(0), last_winner(make_pair(NON, NON)) {}

    Team first_attacker(int city_id) {
        if (flag == BLUEFLAG || (city_id % 2 == 0 && flag == EMPTY)) {
            return BLUE;
        } else if (flag == REDFLAG || (city_id % 2 == 1 && flag == EMPTY)) {
            return RED;
        }
        return NON;
    }

    void update_winner_and_flag(int time_min, int city_id, Team new_winner) {
        if (new_winner == NON) {
            last_winner = make_pair(NON, NON);
            return;
        }
        last_winner.first = last_winner.second;
        last_winner.second = new_winner;
        if (last_winner.first == BLUE && last_winner.second == BLUE && flag != BLUEFLAG) {
            cout << get_time_str(time_min) << " blue flag raised in city " << city_id << "\n";
            flag = BLUEFLAG;
        } else if (last_winner.first == RED && last_winner.second == RED && flag != REDFLAG) {
            cout << get_time_str(time_min) << " red flag raised in city " << city_id << "\n";
            flag = REDFLAG;
        }
    }
};

class Simulator {
    int M, N, R, K, T;
    Headquarter* red_hq;
    Headquarter* blue_hq;
    vector<City> cities;
    bool game_over;

public:
    Simulator(int m, int n, int r, int k, int t) : M(m), N(n), R(r), K(k), T(t), game_over(false) {
        red_hq = new Headquarter(RED, M);
        blue_hq = new Headquarter(BLUE, M);
        cities.resize(N + 2);
    }

    ~Simulator() {
        delete red_hq;
        delete blue_hq;
        for (auto& c : cities) {
            if (c.red)
                delete c.red;
            if (c.blue)
                delete c.blue;
        }
    }

    void run() {
        for (int min = 0; min <= T; ++min) {
            int step = min % 60;
            if (step == 0)
                do_spawn(min);
            else if (step == 5)
                do_runaway(min);
            else if (step == 10)
                do_march(min);
            else if (step == 20)
                do_add_city_elements(min);
            else if (step == 30)
                do_earn_city_elements(min);
            else if (step == 35)
                do_shoot_arrow(min);
            else if (step == 38)
                do_bomb(min);
            else if (step == 40)
                do_battle(min);
            else if (step == 50)
                do_hp_report(min);
            else if (step == 55)
                do_weapon_report(min);
            if (game_over)
                break;
        }
    }

private:
    // 如果预测会被敌人杀死或攻击该敌人时自己死亡，返回该敌人的指针；否则返回空指针
    Warrior* evaluate_self_died(Warrior* w) {
        if (!w || w->city <= 0 || w->city >= N + 1 || w->hp <= 0 || !w->has_bomb()) {
            return nullptr;
        }
        Warrior* defender = (w->team == RED) ? cities[w->city].blue : cities[w->city].red;
        if (!defender || !w) {
            return nullptr;
        }
        if (defender->hp <= 0) {
            return nullptr;
        }

        Team first = cities[w->city].first_attacker(w->city);
        if (first == w->team) {
            int defender_after_attack = defender->hp - w->attack - w->sword_power();
            if (defender_after_attack <= 0 || defender->type == NINJA) {
                return nullptr;
            }
            int self_after_back = w->hp - defender->attack / 2 - defender->sword_power();
            return self_after_back <= 0 ? defender : nullptr;
        } else if (first == ((w->team == RED) ? BLUE : RED)) {
            int self_after_attack = w->hp - defender->attack - defender->sword_power();
            return self_after_attack <= 0 ? defender : nullptr;
        } else {
            cerr << "What the fuck?!\n";
            return nullptr;
        }
    }

    void do_bomb(int time_min) {
        for (int i = 1; i <= N; ++i) {
            if (cities[i].red && cities[i].blue && cities[i].red->has_bomb()) {
                Warrior* enemy = evaluate_self_died(cities[i].red);
                if (enemy) {
                    cout << get_time_str(time_min) << " red " << w_names[cities[i].red->type] << " " << cities[i].red->id
                         << " used a bomb and killed blue " << w_names[enemy->type] << " " << enemy->id << "\n";
                    delete enemy;
                    cities[i].blue = nullptr;
                    delete cities[i].red;
                    cities[i].red = nullptr;
                }
            }
            if (cities[i].red && cities[i].blue && cities[i].blue->has_bomb()) {
                Warrior* enemy = evaluate_self_died(cities[i].blue);
                if (enemy) {
                    cout << get_time_str(time_min) << " blue " << w_names[cities[i].blue->type] << " " << cities[i].blue->id
                         << " used a bomb and killed red " << w_names[enemy->type] << " " << enemy->id << "\n";
                    delete enemy;
                    cities[i].red = nullptr;
                    delete cities[i].blue;
                    cities[i].blue = nullptr;
                }
            }
        }
    }

    void do_shoot_arrow(int time_min) {
        for (int i = 0; i <= N + 1; ++i) {
            if (i <= N - 1 && cities[i].red && cities[i + 1].blue) {
                Warrior* w_red = cities[i].red;
                Warrior* w_blue = cities[i + 1].blue;
                for (Weapon& weapon : w_red->weapons) {
                    if (weapon.type == ARROW && weapon.uses != 0) {
                        weapon.uses -= 1;
                        w_blue->hp -= R;
                        if (w_blue->hp <= 0) {
                            cout << get_time_str(time_min) << " red " << w_names[w_red->type] << " " << w_red->id
                                 << " shot and killed blue " << w_names[w_blue->type] << " " << w_blue->id << "\n";
                            if (cities[i + 1].red) {
                                cities[i + 1].red->shoot_victory = true;
                            }
                        } else {
                            cout << get_time_str(time_min) << " red " << w_names[w_red->type] << " " << w_red->id << " shot\n";
                        }
                    }
                }
                w_red->remove_broken_weapons();
            }
            if (i >= 2 && cities[i].blue && cities[i - 1].red) {
                Warrior* w_blue = cities[i].blue;
                Warrior* w_red = cities[i - 1].red;
                for (Weapon& weapon : w_blue->weapons) {
                    if (weapon.type == ARROW && weapon.uses != 0) {
                        weapon.uses -= 1;
                        w_red->hp -= R;

                        if (w_red->hp <= 0) {
                            cout << get_time_str(time_min) << " blue " << w_names[w_blue->type] << " " << w_blue->id
                                 << " shot and killed red " << w_names[w_red->type] << " " << w_red->id << "\n";
                            if (cities[i - 1].blue) {
                                cities[i - 1].blue->shoot_victory = true;
                            }
                        } else {
                            cout << get_time_str(time_min) << " blue " << w_names[w_blue->type] << " " << w_blue->id << " shot\n";
                        }
                    }
                }
                w_blue->remove_broken_weapons();
            }
        }
    }

    void do_earn_city_elements(int time_min) {
        for (int i = 1; i <= N; ++i) {
            if (cities[i].city_elements == 0) {
                continue;
            }
            if (cities[i].red && cities[i].red->hp > 0 && (!cities[i].blue || cities[i].blue->hp <= 0)) {
                Warrior* w_red = cities[i].red;
                cout << get_time_str(time_min) << " red " << w_names[w_red->type] << " " << w_red->id << " earned "
                     << cities[i].city_elements << " elements for his headquarter\n";
                red_hq->elements += cities[i].city_elements;
                cities[i].city_elements = 0;
            } else if ((!cities[i].red || cities[i].red->hp <= 0) && cities[i].blue && cities[i].blue->hp > 0) {
                Warrior* w_blue = cities[i].blue;
                cout << get_time_str(time_min) << " blue " << w_names[w_blue->type] << " " << w_blue->id << " earned "
                     << cities[i].city_elements << " elements for his headquarter\n";
                blue_hq->elements += cities[i].city_elements;
                cities[i].city_elements = 0;
            }
        }
    }

    void do_add_city_elements(int time_min) {
        (void)time_min;
        for (int i = 1; i <= N; ++i) {
            cities[i].city_elements += 10;
        }
    }

    void do_spawn(int time_min) {
        Warrior* rw = red_hq->spawn(time_min);
        if (rw) {
            rw->city = 0;
            cities[0].red = rw;
        }
        Warrior* bw = blue_hq->spawn(time_min);
        if (bw) {
            bw->city = N + 1;
            cities[N + 1].blue = bw;
        }
    }

    void do_runaway(int time_min) {
        for (int i = 0; i <= N + 1; ++i) {
            if (cities[i].red && cities[i].red->type == LION && cities[i].red->loyalty <= 0 && i != N + 1) {
                cout << get_time_str(time_min) << " red lion " << cities[i].red->id << " ran away\n";
                delete cities[i].red;
                cities[i].red = nullptr;
            }
            if (cities[i].blue && cities[i].blue->type == LION && cities[i].blue->loyalty <= 0 && i != 0) {
                cout << get_time_str(time_min) << " blue lion " << cities[i].blue->id << " ran away\n";
                delete cities[i].blue;
                cities[i].blue = nullptr;
            }
        }
    }

    void do_march(int time_min) {
        vector<Warrior*> next_red(N + 2, nullptr);
        vector<Warrior*> next_blue(N + 2, nullptr);
        Warrior* new_blue_in_red_hq = cities[1].blue;
        Warrior* new_red_in_blue_hq = cities[N].red;

        next_blue[0] = cities[0].blue;
        next_red[N + 1] = cities[N + 1].red;

        for (int i = 0; i <= N; ++i) {
            if (cities[i].red) {
                next_red[i + 1] = cities[i].red;
            }
        }

        for (int i = 1; i <= N + 1; ++i) {
            if (cities[i].blue) {
                next_blue[i - 1] = cities[i].blue;
            }
        }

        for (int i = 0; i <= N + 1; ++i) {
            if (next_red[i]) {
                Warrior* w = next_red[i];
                w->city = i;
                if (!(w->team == RED && i == N + 1 && cities[N + 1].red == w)) {
                    w->steps++;
                }
                if (w->type == ICEMAN && w->steps > 0 && w->steps % 2 == 0) {
                    w->hp -= 9;
                    if (w->hp <= 0) {
                        w->hp = 1;
                    }
                    w->attack += 20;
                }
            }
            if (next_blue[i]) {
                Warrior* w = next_blue[i];
                w->city = i;
                if (!(w->team == BLUE && i == 0 && cities[0].blue == w)) {
                    w->steps++;
                }
                if (w->type == ICEMAN && w->steps > 0 && w->steps % 2 == 0) {
                    w->hp -= 9;
                    if (w->hp <= 0) {
                        w->hp = 1;
                    }
                    w->attack += 20;
                }
            }

            if (i == 0) {
                if (next_blue[0] && next_blue[0] == new_blue_in_red_hq) {
                    cout << get_time_str(time_min) << " blue " << w_names[next_blue[0]->type] << " " << next_blue[0]->id
                         << " reached red headquarter with " << next_blue[0]->hp << " elements and force " << next_blue[0]->attack << "\n";
                    red_hq->enemy_num += 1;
                    if (red_hq->enemy_num >= 2) {
                        cout << get_time_str(time_min) << " red headquarter was taken\n";
                        game_over = true;
                    }
                }
            } else if (i == N + 1) {
                if (next_red[N + 1] && next_red[N + 1] == new_red_in_blue_hq) {
                    cout << get_time_str(time_min) << " red " << w_names[next_red[N + 1]->type] << " " << next_red[N + 1]->id
                         << " reached blue headquarter with " << next_red[N + 1]->hp << " elements and force " << next_red[N + 1]->attack
                         << "\n";
                    blue_hq->enemy_num += 1;
                    if (blue_hq->enemy_num >= 2) {
                        cout << get_time_str(time_min) << " blue headquarter was taken\n";
                        game_over = true;
                    }
                }
            } else {
                if (next_red[i]) {
                    cout << get_time_str(time_min) << " red " << w_names[next_red[i]->type] << " " << next_red[i]->id << " marched to city "
                         << i << " with " << next_red[i]->hp << " elements and force " << next_red[i]->attack << "\n";
                }
                if (next_blue[i]) {
                    cout << get_time_str(time_min) << " blue " << w_names[next_blue[i]->type] << " " << next_blue[i]->id
                         << " marched to city " << i << " with " << next_blue[i]->hp << " elements and force " << next_blue[i]->attack
                         << "\n";
                }
            }
        }

        for (int i = 0; i <= N + 1; ++i) {
            cities[i].red = next_red[i];
            cities[i].blue = next_blue[i];
        }
    }

    // void do_wolf_steal(int time_min) {
    //     for (int i = 1; i <= N; ++i) {
    //         if (cities[i].red && cities[i].blue) {
    //             Warrior* r = cities[i].red;
    //             Warrior* b = cities[i].blue;
    //             if (r->type == WOLF && b->type != WOLF) {
    //                 wolf_steal(r, b, time_min, i);
    //             }else if (b->type == WOLF && r->type != WOLF) {
    //                 wolf_steal(b, r, time_min, i);
    //             }
    //         }
    //     }
    // }

    void wolf_steal(Warrior* wolf, Warrior* enemy, int time_min, int city_id) {
        (void)time_min;
        (void)city_id;
        for (auto weapon : enemy->weapons) {
            if (!wolf->has_weapon_type(weapon.type)) {
                wolf->weapons.push_back(weapon);
            }
        }
    }

    FightingResult attack_on_ones_own(Warrior* attacker, Warrior* defender, int time_min, ostream& out) {
        if (!attacker || !defender || attacker->hp <= 0 || defender->hp <= 0) {
            cerr << "nibunengrangxiaohaizilaidazhanga, sbdx!\n";
            return TIE;
        }
        int sword_power = attacker->sword_power();
        int total_power = attacker->attack + sword_power;
        if (sword_power > 0) {
            attacker->update_sword_power(sword_power * 8 / 10);
        }

        if (attacker->team == RED) {
            out << get_time_str(time_min) << " red " << w_names[attacker->type] << " " << attacker->id << " attacked blue "
                << w_names[defender->type] << " " << defender->id << " in city " << attacker->city << " with " << attacker->hp
                << " elements and force " << attacker->attack << "\n";
        } else if (attacker->team == BLUE) {
            out << get_time_str(time_min) << " blue " << w_names[attacker->type] << " " << attacker->id << " attacked red "
                << w_names[defender->type] << " " << defender->id << " in city " << attacker->city << " with " << attacker->hp
                << " elements and force " << attacker->attack << "\n";
        }

        defender->hp -= total_power;
        if (defender->hp <= 0) {
            if (defender->team == RED) {
                out << get_time_str(time_min) << " red " << w_names[defender->type] << " " << defender->id << " was killed in city "
                    << defender->city << "\n";
            } else if (defender->team == BLUE) {
                out << get_time_str(time_min) << " blue " << w_names[defender->type] << " " << defender->id << " was killed in city "
                    << defender->city << "\n";
            }
            return SUCCESS;
        }
        return TIE;
    }

    FightingResult attack_back(Warrior* attacker, Warrior* defender, int time_min, ostream& out) {
        if (!attacker || !defender || attacker->hp <= 0 || defender->hp <= 0) {
            cerr << "nibunengrangxiaohaizilaidazhanga, sbdx!\n";
        }
        int sword_power = attacker->sword_power();
        int total_power = attacker->attack / 2 + sword_power;
        if (sword_power > 0) {
            attacker->update_sword_power(sword_power * 8 / 10);
        }

        if (attacker->team == RED) {
            out << get_time_str(time_min) << " red " << w_names[attacker->type] << " " << attacker->id << " fought back against blue "
                << w_names[defender->type] << " " << defender->id << " in city " << attacker->city << "\n";
        } else if (attacker->team == BLUE) {
            out << get_time_str(time_min) << " blue " << w_names[attacker->type] << " " << attacker->id << " fought back against red "
                << w_names[defender->type] << " " << defender->id << " in city " << attacker->city << "\n";
        }

        defender->hp -= total_power;
        if (defender->hp <= 0) {
            if (defender->team == RED) {
                out << get_time_str(time_min) << " red " << w_names[defender->type] << " " << defender->id << " was killed in city "
                    << defender->city << "\n";
            } else if (defender->team == BLUE) {
                out << get_time_str(time_min) << " blue " << w_names[defender->type] << " " << defender->id << " was killed in city "
                    << defender->city << "\n";
            }
            return SUCCESS;
        }
        return TIE;
    }

    void clear_battle_field(const vector<Warrior*> winner, int time_min) {
        // 先发生命元奖励！
        for (int i = N; i >= 1; --i) {
            if (winner[i] && winner[i]->team == RED && red_hq->elements >= 8) {
                red_hq->elements -= 8;
                cities[i].red->hp += 8;
            }
        }
        for (int i = 1; i <= N; ++i) {
            if (winner[i] && winner[i]->team == BLUE && blue_hq->elements >= 8) {
                blue_hq->elements -= 8;
                cities[i].blue->hp += 8;
            }
        }

        (void)time_min;
    }

    void do_battle(int time_min) {
        vector<Warrior*> winner(N + 1, nullptr);
        vector<string> city_logs(N + 1);
        for (int i = 1; i <= N; ++i) {
            ostringstream log;
            Warrior* red = cities[i].red;
            Warrior* blue = cities[i].blue;
            if (!red || !blue) {
                if (red) {
                    red->shoot_victory = false;
                }
                if (blue) {
                    blue->shoot_victory = false;
                }
                continue;
            }

            int red_before = red->hp;
            int blue_before = blue->hp;
            bool real_battle = red->hp > 0 && blue->hp > 0;
            bool red_killed_enemy = false;
            bool blue_killed_enemy = false;
            Team first = cities[i].first_attacker(i);
            Warrior* active_attacker = (first == RED) ? red : blue;

            if (red->hp <= 0 || blue->hp <= 0) {
                if (red->hp > 0 && blue->hp <= 0) {
                    red_killed_enemy = true;
                    red->shoot_victory = false;
                    winner[i] = cities[i].red;
                } else if (blue->hp > 0 && red->hp <= 0) {
                    blue_killed_enemy = true;
                    blue->shoot_victory = false;
                    winner[i] = cities[i].blue;
                }
                red->shoot_victory = false;
                blue->shoot_victory = false;
                if (winner[i] && winner[i]->type == WOLF) {
                    Warrior* dead_enemy = (winner[i]->team == RED) ? blue : red;
                    wolf_steal(winner[i], dead_enemy, time_min, i);
                }
                if (active_attacker && active_attacker->type == DRAGON && winner[i] == active_attacker) {
                    active_attacker->morale += 0.2;
                    if (active_attacker->hp > 0 && active_attacker->morale > 0.8) {
                        log << get_time_str(time_min) << " " << t_names[active_attacker->team] << " dragon " << active_attacker->id
                            << " yelled in city " << i << "\n";
                    }
                }
                city_logs[i] = log.str();
                continue;
            }

            Warrior* attacker = (first == RED) ? red : blue;
            Warrior* defender = (first == RED) ? blue : red;

            FightingResult attack_first_result = attack_on_ones_own(attacker, defender, time_min, log);
            if (attack_first_result == SUCCESS) {
                winner[i] = attacker;
                if (attacker->team == RED) {
                    red_killed_enemy = true;
                } else {
                    blue_killed_enemy = true;
                }
                if (defender->type == LION) {
                    attacker->hp += (defender->team == RED) ? red_before : blue_before;
                }
                if (attacker->type == WOLF) {
                    wolf_steal(attacker, defender, time_min, i);
                }
            } else if (attack_first_result == TIE) {
                if (defender->type != NINJA) {
                    FightingResult attack_back_result = attack_back(defender, attacker, time_min, log);
                    if (attack_back_result == SUCCESS) {
                        winner[i] = defender;
                        if (defender->team == RED) {
                            red_killed_enemy = true;
                        } else {
                            blue_killed_enemy = true;
                        }
                        if (attacker->type == LION) {
                            defender->hp += (attacker->team == RED) ? red_before : blue_before;
                        }
                        if (defender->type == WOLF) {
                            wolf_steal(defender, attacker, time_min, i);
                        }
                    }
                }
            } else {
                cerr << "nizenmbazijidasile? What the fuck?!\n";
            }

            if (real_battle) {
                if (red->type == LION && !red_killed_enemy && red->hp > 0) {
                    red->loyalty -= K;
                }
                if (blue->type == LION && !blue_killed_enemy && blue->hp > 0) {
                    blue->loyalty -= K;
                }
            }

            if (!winner[i]) {
                cities[i].update_winner_and_flag(time_min, i, NON);
            }

            if (attacker->type == DRAGON && attacker->hp > 0) {
                if (winner[i] == attacker) {
                    attacker->morale += 0.2;
                } else {
                    attacker->morale -= 0.2;
                }
                if (attacker->morale > 0.8) {
                    log << get_time_str(time_min) << " " << t_names[attacker->team] << " dragon " << attacker->id << " yelled in city " << i
                        << "\n";
                }
            }
            city_logs[i] = log.str();
        }
        clear_battle_field(winner, time_min);

        for (int i = 1; i <= N; ++i) {
            cout << city_logs[i];
            if (winner[i] && winner[i]->team == RED) {
                red_hq->elements += cities[i].city_elements;
                cout << get_time_str(time_min) << " red " << w_names[winner[i]->type] << " " << winner[i]->id << " earned "
                     << cities[i].city_elements << " elements for his headquarter\n";
                cities[i].city_elements = 0;
                cities[i].update_winner_and_flag(time_min, i, RED);
            } else if (winner[i] && winner[i]->team == BLUE) {
                blue_hq->elements += cities[i].city_elements;
                cout << get_time_str(time_min) << " blue " << w_names[winner[i]->type] << " " << winner[i]->id << " earned "
                     << cities[i].city_elements << " elements for his headquarter\n";
                cities[i].city_elements = 0;
                cities[i].update_winner_and_flag(time_min, i, BLUE);
            }
        }

        for (int i = 1; i <= N; ++i) {
            if (cities[i].red && cities[i].red->hp <= 0) {
                delete cities[i].red;
                cities[i].red = nullptr;
            }
            if (cities[i].blue && cities[i].blue->hp <= 0) {
                delete cities[i].blue;
                cities[i].blue = nullptr;
            }
        }
    }

    // void do_battle(int time_min) {
    //     for (int i = 1; i <= N; ++i) {
    //         if (cities[i].red && cities[i].blue) {
    //             Warrior* r = cities[i].red;
    //             Warrior* b = cities[i].blue;
    //             Warrior* attacker = (i % 2 == 1) ? r : b;
    //             Warrior* defender = (i % 2 == 1) ? b : r;

    //             r->sort_weapons_combat();
    //             b->sort_weapons_combat();
    //             r->combat_idx = 0;
    //             b->combat_idx = 0;

    //             int no_change_count = 0;
    //             while (true) {
    //                 bool changed_a = attack_on_ones_own(attacker, defender);
    //                 if (attacker->hp <= 0 || defender->hp <= 0) {
    //                     break;
    //                 }

    //                 bool changed_d = attack_on_ones_own(defender, attacker);
    //                 if (attacker->hp <= 0 || defender->hp <= 0) {
    //                     break;
    //                 }

    //                 if (!changed_a && !changed_d) {
    //                     no_change_count++;
    //                     if (no_change_count > 10) {
    //                         break;
    //                     }
    //                 } else {
    //                     no_change_count = 0;
    //                 }
    //             }

    //             r->remove_broken_weapons();
    //             b->remove_broken_weapons();

    //             if (r->hp > 0 && b->hp <= 0) {
    //                 cout << get_time_str(time_min) << " red " << w_names[r->type] << " " << r->id << " killed blue " << w_names[b->type]
    //                 << " " << b->id << " in city " << i << " remaining " << r->hp << " elements\n"; if (r->type == DRAGON) cout <<
    //                 get_time_str(time_min) << " red dragon " << r->id << " yelled in city " << i << "\n"; r->loot(b); delete b;
    //                 cities[i].blue = nullptr;
    //             } else if (b->hp > 0 && r->hp <= 0) {
    //                 cout << get_time_str(time_min) << " blue " << w_names[b->type] << " " << b->id << " killed red " << w_names[r->type]
    //                 << " " << r->id << " in city " << i << " remaining " << b->hp << " elements\n"; if (b->type == DRAGON) {
    //                     cout << get_time_str(time_min) << " blue dragon " << b->id << " yelled in city " << i << "\n";
    //                 }
    //                 b->loot(r);
    //                 delete r;
    //                 cities[i].red = nullptr;
    //             } else if (r->hp <= 0 && b->hp <= 0) {
    //                 cout << get_time_str(time_min) << " both red " << w_names[r->type] << " " << r->id << " and blue " <<
    //                 w_names[b->type] << " " << b->id << " died in city " << i << "\n"; delete r; cities[i].red = nullptr; delete b;
    //                 cities[i].blue = nullptr;
    //             } else {
    //                 cout << get_time_str(time_min) << " both red " << w_names[r->type] << " " << r->id << " and blue " <<
    //                 w_names[b->type] << " " << b->id << " were alive in city " << i << "\n"; if (r->type == DRAGON) {
    //                     cout << get_time_str(time_min) << " red dragon " << r->id << " yelled in city " << i << "\n";
    //                 }
    //                 if (b->type == DRAGON) {
    //                     cout << get_time_str(time_min) << " blue dragon " << b->id << " yelled in city " << i << "\n";
    //                 }
    //             }
    //         }
    //     }
    // }

    void do_hp_report(int time_min) {
        cout << get_time_str(time_min) << " " << red_hq->elements << " elements in red headquarter\n";
        cout << get_time_str(time_min) << " " << blue_hq->elements << " elements in blue headquarter\n";
    }

    void do_weapon_report(int time_min) {
        for (int i = 0; i <= N + 1; ++i) {
            if (cities[i].red) {
                print_warrior_weapons(time_min, cities[i].red);
            }
        }
        for (int i = 0; i <= N + 1; ++i) {
            if (cities[i].blue) {
                print_warrior_weapons(time_min, cities[i].blue);
            }
        }
    }

    void print_warrior_weapons(int time_min, Warrior* w) {
        vector<string> weapons(3, "");
        vector<int> print_out_idx;
        for (auto& weapon : w->weapons) {
            if (weapon.type == ARROW && weapon.uses > 0) {
                weapons[0] += "arrow(";
                weapons[0] += to_string(weapon.uses);
                weapons[0] += ")";
                print_out_idx.push_back(0);
            }
            if (weapon.type == BOMB && weapon.uses > 0) {
                weapons[1] = "bomb";
                print_out_idx.push_back(1);
            }
            if (weapon.type == SWORD && weapon.power > 0) {
                weapons[2] = "sword(";
                weapons[2] += to_string(weapon.power);
                weapons[2] += ")";
                print_out_idx.push_back(2);
            }
        }

        if (w->team == BLUE) {
            cout << get_time_str(time_min) << " blue " << w_names[w->type] << " " << w->id << " has ";
        } else if (w->team == RED) {
            cout << get_time_str(time_min) << " red " << w_names[w->type] << " " << w->id << " has ";
        }

        if (print_out_idx.empty()) {
            cout << "no weapon\n";
            return;
        }
        bool first_weapon = true;
        for (int i = 0; i < 3; ++i) {
            if (weapons[i].empty()) {
                continue;
            }
            if (!first_weapon) {
                cout << ",";
            }
            cout << weapons[i];
            first_weapon = false;
        }
        cout << "\n";
    }
};

int main() {
    int t;
    if (!(cin >> t)) {
        return 0;
    }
    for (int i = 1; i <= t; ++i) {
        int M, N, R, K, T;
        cin >> M >> N >> R >> K >> T;
        for (int j = 0; j < 5; ++j) {
            cin >> init_hp[j];
        }
        for (int j = 0; j < 5; ++j) {
            cin >> init_attack[j];
        }

        cout << "Case " << i << ":" << endl;
        Simulator sim(M, N, R, K, T);
        sim.run();
    }
    return 0;
}
