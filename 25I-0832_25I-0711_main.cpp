#include <iostream>
#include <fstream>
#include <cmath>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include "header.h"
#include "Weapon.h"
#include "PlayerSoldier.h"
#include "vehicle.h"
#include"scoreManager.h"
using namespace sf;
using namespace std;

//Entitymanager out of linee definitions as bodies live here so Weapon.h types are fully visible
void EntityManager::resolvePlayerProjectileHits(Explosion** explosions, int& explCount, int maxExpl,FirePool** firePools, int& firePoolCount, int maxPools,
ProjectileWeapon* grenadeWeapon,ScoreManager* scores,float camX, float camY,bool playerInAir)
{
    for (int i = 0; i < projectileCount; i++) {
        Projectile* proj = projectiles[i];
        if (!proj || !proj->isActive() || !proj->isFromPlayer())
            continue;

        for (int j = 0; j < enemyCount; j++) {
            Enemy* e = enemies[j];
            if (!e || !e->isActive()) continue;

            bool hit = proj->getX() < e->getX() + e->getW() &&
                proj->getX() + proj->getW() > e->getX() &&
                proj->getY() < e->getY() + e->getH() &&
                proj->getY() + proj->getH() > e->getY();
            if (!hit) continue;

            //FlameProjectile
            if (proj->isFlame()) {
                FlameProjectile* fp = (FlameProjectile*)proj;
                MummyWarrior* mw = dynamic_cast<MummyWarrior*>(e);
                if (mw) {
                    if (scores) {
                        float sx = e->getX() - camX;
                        float sy = e->getY() - camY;
                        scores->awardEnemyKill(
                            e->getScoreValue(), playerInAir, false, sx, sy);
                    }
                    e->setActive(false);

                    fp->setActive(false);
                }
                else if (fp->canDamageNow()) {
                    bool wasAlive = e->isActive();
                    e->takeDamage(2);
                    if (wasAlive && !e->isActive() && scores) {
                        float sx = e->getX() - camX;
                        float sy = e->getY() - camY;
                        scores->awardEnemyKill(
                            e->getScoreValue(), playerInAir, false, sx, sy);
                    }
                }
                continue;
            }

            //BallisticProjectile for grenade or rocket
            if (proj->isBallistic()) {
                if (explCount < maxExpl)
                    explosions[explCount++] =
                    new Explosion(proj->getX(), proj->getY());

                int blastKills = 0;
                float bsx = proj->getX() - camX;
                float bsy = proj->getY() - camY;

                for (int k = 0; k < enemyCount; k++) {
                    Enemy* be = enemies[k];
                    if (!be || !be->isActive()) continue;
                    float dx = (be->getX() + be->getW() / 2) - proj->getX();
                    float dy = (be->getY() + be->getH() / 2) - proj->getY();
                    if (dx * dx + dy * dy <= 192.0f * 192.0f) {
                        bool wasAlive = be->isActive();
                        be->takeDamage(5);
                        if (wasAlive && !be->isActive()) {
                            // Score each individual blast kill
                            if (scores)
                                scores->awardEnemyKill(
                                    be->getScoreValue(), false, false, bsx, bsy);
                            blastKills++;
                        }
                    }
                }

                //Multi kill or massacre BONUS on top of individual scores
                if (scores && blastKills >= 2)
                    scores->awardExplosionBonus(blastKills, bsx, bsy);

                if (grenadeWeapon && grenadeWeapon->isFireBomb()
                    && firePoolCount < maxPools)
                    firePools[firePoolCount++] =
                    new FirePool(proj->getX(), proj->getY());

                proj->setActive(false);
                continue;
            }

            //straightprojectile
            bool wasAlive = e->isActive();
            e->takeDamage(proj->getDamage());
            if (wasAlive && !e->isActive() && scores) {
                float sx = e->getX() - camX;
                float sy = e->getY() - camY;
                scores->awardEnemyKill(
                    e->getScoreValue(), playerInAir, false, sx, sy);
            }
            proj->setActive(false);
            break;
        }
    }
}


void EntityManager::resolveEnemyProjectileHits(PlayerSoldier* player)
{
    for (int i = 0; i < projectileCount; i++) {
        Projectile* proj = projectiles[i];
        if (!proj || !proj->isActive() || proj->isFromPlayer()) continue;

        bool hit = proj->getX() < player->getX() + player->getW() &&
            proj->getX() + proj->getW() > player->getX() &&
            proj->getY() < player->getY() + player->getH() &&
            proj->getY() + proj->getH() > player->getY();

        if (hit) { player->takeDamage(); proj->setActive(false); }
    }
}

//screen constants
const int screen_x = 1600;
const int screen_y = 900;
const int cell_size = 64;

const int AERIAL_END = 12;
const int PLAINS_END = 27;
const int AQUATIC_END = 38;

enum GameMode { MENU, SURVIVAL, CAMPAIGN };

char levelMap[LEVEL_H][LEVEL_W];
void generateLevel1();


//DEVELOPER MODE

class DeveloperMode {
    bool active;
    float shootTimer;
    float shootInterval;

public:
    DeveloperMode() : active(false), shootTimer(0), shootInterval(0.1f) {}
    void setActive(bool val) { active = val; }
    void toggle() { active = !active; }
    bool isActive() const { return active; }

    void handleMovement(PlayerSoldier* player) {
        float devSpeed = 15.0f;  //5x normal speed in thi s mode
        if (Keyboard::isKeyPressed(Keyboard::Up))
            player->setVelY(-devSpeed);
        else if (Keyboard::isKeyPressed(Keyboard::Down))
            player->setVelY(devSpeed);
        else
            player->setVelY(0);

    }

    void autoShoot(float dt, PlayerSoldier* player,
        Projectile** pool, int* pCount, int pMax) {
        shootTimer += dt;
        if (shootTimer < shootInterval) return;
        shootTimer = 0;

        float cx = player->getX() + player->getW() / 2;
        float cy = player->getY() + player->getH() / 2;
        float spd = 30.0f;
        float diagSpd = spd * 0.707f;
        int dmg = 99;

        float vxList[8] = { spd, -spd,  0,    0,    diagSpd, -diagSpd, -diagSpd,  diagSpd };
        float vyList[8] = { 0,   0,  -spd,  spd,  -diagSpd, -diagSpd,  diagSpd,  diagSpd };

        //always overwrite slots 0-7, no searching, no condition
        for (int i = 0; i < 8; i++) {
            delete pool[i];
            pool[i] = new StraightProjectile(
                cx, cy, vxList[i], vyList[i],
                dmg, true, "25I-0832_25I-0711_Assets/bullet.png");
        }

        if (*pCount < 8) *pCount = 8;
    }

    void removeBlockUnderPlayer(PlayerSoldier* player) {
        int col = (int)(player->getX() / cell_size);
        int row = (int)(player->getY() / cell_size);
        for (int r = row - 1; r <= row + 1; r++)
            for (int c = col - 1; c <= col + 2; c++)
                if (r >= 0 && r < LEVEL_H - 1 && c >= 0 && c < LEVEL_W)
                    if (levelMap[r][c] != T_ROCK)  // never remove indestructible bottom
                        levelMap[r][c] = T_SKY;
    }
};
// LEVEL BASE CLASS
class Level {
public:
    virtual void generateMap() = 0;
    virtual void spawnEnemies(EntityManager& em) = 0;
    virtual ~Level() {}
};

class Level1 : public Level {
public:
    void generateMap() override {
        generateLevel1();   // calls your existing function
    }
    void spawnEnemies(EntityManager& em) override {

        //INFANTRY: 2 batches each, spread across all 3 biomes
        em.addEnemy(new RebelSoldier(10 * cell_size, 1520.0f)); // plains batch 1
        em.addEnemy(new ShieldedSoldier(20 * cell_size, 1520.0f)); // plains batch 1
        em.addEnemy(new BazookaSoldier(30 * cell_size, 1520.0f)); // plains batch 1
        em.addEnemy(new GrenadeSoldier(45 * cell_size, 1520.0f)); // plains batch 1

        em.addEnemy(new RebelSoldier(90 * cell_size, 10.0f * cell_size)); // aerial batch 2
        em.addEnemy(new ShieldedSoldier(150 * cell_size, 23.0f * cell_size)); // aquatic batch 2
        em.addEnemy(new BazookaSoldier(110 * cell_size, 8.0f * cell_size)); // aerial batch 2
        em.addEnemy(new GrenadeSoldier(170 * cell_size, 23.0f * cell_size)); // aquatic batch 2

        //AERIAL andd ALIEN: 1 batch, aerial biome only
        em.addEnemy(new Paratrooper(75 * cell_size, 5.0f * cell_size, levelMap));
        em.addEnemy(new Paratrooper(115 * cell_size, 5.0f * cell_size, levelMap));
        em.addEnemy(new Martian(85 * cell_size, 12.0f * cell_size));
        em.addEnemy(new Martian(125 * cell_size, 12.0f * cell_size));

        //UNDEAD: 2 batches each, plains only 
        em.addEnemy(new Zombie(12 * cell_size, 24.0f * cell_size)); // batch 1
        em.addEnemy(new Zombie(35 * cell_size, 24.0f * cell_size)); // batch 1
        em.addEnemy(new Zombie(22 * cell_size, 24.0f * cell_size)); // batch 2
        em.addEnemy(new Zombie(55 * cell_size, 24.0f * cell_size)); // batch 2

        em.addEnemy(new MummyWarrior(18 * cell_size, 1530.0f)); // batch 1
        em.addEnemy(new MummyWarrior(40 * cell_size, 1530.0f)); // batch 1
        em.addEnemy(new MummyWarrior(28 * cell_size, 1530.0f)); // batch 2
        em.addEnemy(new MummyWarrior(62 * cell_size, 1530.0f)); // batch 2

        //ZOMBIES: 2 batches in aquatic biome
        em.addEnemy(new Zombie(145 * cell_size, 23.0f * cell_size)); // batch 1
        em.addEnemy(new Zombie(158 * cell_size, 23.0f * cell_size)); // batch 1
        em.addEnemy(new Zombie(172 * cell_size, 23.0f * cell_size)); // batch 2
        em.addEnemy(new Zombie(188 * cell_size, 23.0f * cell_size)); // batch 2

        //FLYING TARA: 2 batches across full levell
        em.addEnemy(new FlyingTara(30 * cell_size, 8.0f * cell_size)); // batch 1
        em.addEnemy(new FlyingTara(65 * cell_size, 8.0f * cell_size)); // batch 1
        em.addEnemy(new FlyingTara(120 * cell_size, 8.0f * cell_size)); // batch 2
        em.addEnemy(new FlyingTara(175 * cell_size, 8.0f * cell_size)); // batch 2
    }
};

class Level2 : public Level {
public:
    void generateMap() override {
        generateLevel1();   //same terrain, only spawns increase
    }
    void spawnEnemies(EntityManager& em) override {
        // INFANTRY  3 batches each, all 3 biomes
        // RebelSoldier  batch 1 plains, batch 2 aerial, batch 3 aquatic
        em.addEnemy(new RebelSoldier(10 * cell_size, 23.8f * cell_size));
        em.addEnemy(new RebelSoldier(30 * cell_size, 23.8f * cell_size));
        em.addEnemy(new RebelSoldier(80 * cell_size, 14.0f * cell_size));
        em.addEnemy(new RebelSoldier(110 * cell_size, 10.0f * cell_size));
        em.addEnemy(new RebelSoldier(150 * cell_size, 23.8f * cell_size));
        em.addEnemy(new RebelSoldier(175 * cell_size, 23.8f * cell_size));

        // ShieldedSoldier  batch 1 plains, batch 2 aerial, batch 3 aquatic
        em.addEnemy(new ShieldedSoldier(15 * cell_size, 23.0f * cell_size));
        em.addEnemy(new ShieldedSoldier(40 * cell_size, 23.0f * cell_size));
        em.addEnemy(new ShieldedSoldier(85 * cell_size, 12.0f * cell_size));
        em.addEnemy(new ShieldedSoldier(115 * cell_size, 10.0f * cell_size));
        em.addEnemy(new ShieldedSoldier(145 * cell_size, 23.0f * cell_size));
        em.addEnemy(new ShieldedSoldier(170 * cell_size, 23.0f * cell_size));

        // BazookaSoldier — batch 1 plains, batch 2 aerial, batch 3 aquatic
        em.addEnemy(new BazookaSoldier(20 * cell_size, 24.0f * cell_size));
        em.addEnemy(new BazookaSoldier(50 * cell_size, 23.8f * cell_size));
        em.addEnemy(new BazookaSoldier(90 * cell_size, 10.0f * cell_size));
        em.addEnemy(new BazookaSoldier(120 * cell_size, 8.0f * cell_size));
        em.addEnemy(new BazookaSoldier(155 * cell_size, 23.8f * cell_size));
        em.addEnemy(new BazookaSoldier(185 * cell_size, 23.8f * cell_size));

        // GrenadeSoldier batch 1 plains, batch 2 aerial, batch 3 aquatic
        em.addEnemy(new GrenadeSoldier(25 * cell_size, 22.8f * cell_size));
        em.addEnemy(new GrenadeSoldier(55 * cell_size, 23.8f * cell_size));
        em.addEnemy(new GrenadeSoldier(95 * cell_size, 10.0f * cell_size));
        em.addEnemy(new GrenadeSoldier(125 * cell_size, 8.0f * cell_size));
        em.addEnemy(new GrenadeSoldier(160 * cell_size, 23.8f * cell_size));
        em.addEnemy(new GrenadeSoldier(190 * cell_size, 23.8f * cell_size));

        // AERIAL and ALIEN  2 batches each, aerial biome only 
        //Paratrooper
        em.addEnemy(new Paratrooper(70 * cell_size, 8.0f * cell_size, levelMap));
        em.addEnemy(new Paratrooper(85 * cell_size, 8.0f * cell_size, levelMap));
        em.addEnemy(new Paratrooper(100 * cell_size, 8.0f * cell_size, levelMap));
        em.addEnemy(new Paratrooper(120 * cell_size, 8.0f * cell_size, levelMap));

        // Martian
        em.addEnemy(new Martian(72 * cell_size, 10.0f * cell_size));
        em.addEnemy(new Martian(95 * cell_size, 10.0f * cell_size));
        em.addEnemy(new Martian(110 * cell_size, 10.0f * cell_size));
        em.addEnemy(new Martian(130 * cell_size, 10.0f * cell_size));

        // UNDEAD  3 batches each, plains biome only 
        // Zombie
        em.addEnemy(new Zombie(8 * cell_size, 24.0f * cell_size));
        em.addEnemy(new Zombie(18 * cell_size, 24.0f * cell_size));
        em.addEnemy(new Zombie(28 * cell_size, 24.0f * cell_size));
        em.addEnemy(new Zombie(38 * cell_size, 24.0f * cell_size));
        em.addEnemy(new Zombie(48 * cell_size, 24.0f * cell_size));
        em.addEnemy(new Zombie(60 * cell_size, 24.0f * cell_size));

        // MummyWarrior
        em.addEnemy(new MummyWarrior(12 * cell_size, 1530.0f));
        em.addEnemy(new MummyWarrior(22 * cell_size, 1530.0f));
        em.addEnemy(new MummyWarrior(35 * cell_size, 1530.0f));
        em.addEnemy(new MummyWarrior(45 * cell_size, 1530.0f));
        em.addEnemy(new MummyWarrior(55 * cell_size, 1530.0f));
        em.addEnemy(new MummyWarrior(63 * cell_size, 1530.0f));

        // AQUATIC ZOMBIES  3 batches
        em.addEnemy(new Zombie(140 * cell_size, 23.0f * cell_size));
        em.addEnemy(new Zombie(150 * cell_size, 23.0f * cell_size));
        em.addEnemy(new Zombie(158 * cell_size, 23.0f * cell_size));
        em.addEnemy(new Zombie(167 * cell_size, 23.0f * cell_size));
        em.addEnemy(new Zombie(175 * cell_size, 23.0f * cell_size));
        em.addEnemy(new Zombie(185 * cell_size, 23.0f * cell_size));

        // FLYING TARA  3 batches across full level
        em.addEnemy(new FlyingTara(20 * cell_size, 8.0f * cell_size));
        em.addEnemy(new FlyingTara(45 * cell_size, 8.0f * cell_size));
        em.addEnemy(new FlyingTara(75 * cell_size, 8.0f * cell_size));
        em.addEnemy(new FlyingTara(105 * cell_size, 8.0f * cell_size));
        em.addEnemy(new FlyingTara(140 * cell_size, 8.0f * cell_size));
        em.addEnemy(new FlyingTara(180 * cell_size, 8.0f * cell_size));
    }
};
class Level3 : public Level {
public:
    void generateMap() override {
        generateLevel1();   //same terrain
    }
    void spawnEnemies(EntityManager& em) override {

        // INFANTRY  3 batches each, all 3 biomes
        // RebelSoldier
        em.addEnemy(new RebelSoldier(10 * cell_size, 23.8f * cell_size));  // plains
        em.addEnemy(new RebelSoldier(30 * cell_size, 23.8f * cell_size));
        em.addEnemy(new RebelSoldier(80 * cell_size, 14.0f * cell_size));  // aerial
        em.addEnemy(new RebelSoldier(105 * cell_size, 10.0f * cell_size));
        em.addEnemy(new RebelSoldier(150 * cell_size, 23.8f * cell_size)); // aquatic
        em.addEnemy(new RebelSoldier(180 * cell_size, 23.8f * cell_size));

        // ShieldedSoldier
        em.addEnemy(new ShieldedSoldier(15 * cell_size, 23.0f * cell_size));  // plains
        em.addEnemy(new ShieldedSoldier(40 * cell_size, 23.0f * cell_size));
        em.addEnemy(new ShieldedSoldier(85 * cell_size, 12.0f * cell_size));  // aerial
        em.addEnemy(new ShieldedSoldier(115 * cell_size, 10.0f * cell_size));
        em.addEnemy(new ShieldedSoldier(145 * cell_size, 23.0f * cell_size)); // aquatic
        em.addEnemy(new ShieldedSoldier(175 * cell_size, 23.0f * cell_size));

        // BazookaSoldier
        em.addEnemy(new BazookaSoldier(20 * cell_size, 24.0f * cell_size));   // plains
        em.addEnemy(new BazookaSoldier(50 * cell_size, 23.8f * cell_size));
        em.addEnemy(new BazookaSoldier(90 * cell_size, 10.0f * cell_size));   // aerial
        em.addEnemy(new BazookaSoldier(120 * cell_size, 8.0f * cell_size));
        em.addEnemy(new BazookaSoldier(155 * cell_size, 23.8f * cell_size));  // aquatic
        em.addEnemy(new BazookaSoldier(185 * cell_size, 23.8f * cell_size));

        // GrenadeSoldier
        em.addEnemy(new GrenadeSoldier(25 * cell_size, 22.8f * cell_size));   // plains
        em.addEnemy(new GrenadeSoldier(55 * cell_size, 23.8f * cell_size));
        em.addEnemy(new GrenadeSoldier(95 * cell_size, 10.0f * cell_size));   // aerial
        em.addEnemy(new GrenadeSoldier(125 * cell_size, 8.0f * cell_size));
        em.addEnemy(new GrenadeSoldier(160 * cell_size, 23.8f * cell_size));  // aquatic
        em.addEnemy(new GrenadeSoldier(190 * cell_size, 23.8f * cell_size));

        // AERIAL and  ALIEN — 3 batches each, aerial biome only 
        // Paratrooper
        em.addEnemy(new Paratrooper(70 * cell_size, 8.0f * cell_size, levelMap));
        em.addEnemy(new Paratrooper(82 * cell_size, 8.0f * cell_size, levelMap));
        em.addEnemy(new Paratrooper(95 * cell_size, 8.0f * cell_size, levelMap));
        em.addEnemy(new Paratrooper(108 * cell_size, 8.0f * cell_size, levelMap));
        em.addEnemy(new Paratrooper(120 * cell_size, 8.0f * cell_size, levelMap));
        em.addEnemy(new Paratrooper(132 * cell_size, 8.0f * cell_size, levelMap));

        // Martian
        em.addEnemy(new Martian(70 * cell_size, 10.0f * cell_size));
        em.addEnemy(new Martian(85 * cell_size, 10.0f * cell_size));
        em.addEnemy(new Martian(100 * cell_size, 10.0f * cell_size));
        em.addEnemy(new Martian(112 * cell_size, 10.0f * cell_size));
        em.addEnemy(new Martian(122 * cell_size, 10.0f * cell_size));
        em.addEnemy(new Martian(133 * cell_size, 10.0f * cell_size));

        // UNDEAD  4 batches each, plains biome only
        // Zombie
        em.addEnemy(new Zombie(5 * cell_size, 24.0f * cell_size));
        em.addEnemy(new Zombie(12 * cell_size, 24.0f * cell_size));
        em.addEnemy(new Zombie(22 * cell_size, 24.0f * cell_size));
        em.addEnemy(new Zombie(32 * cell_size, 24.0f * cell_size));
        em.addEnemy(new Zombie(42 * cell_size, 24.0f * cell_size));
        em.addEnemy(new Zombie(52 * cell_size, 24.0f * cell_size));
        em.addEnemy(new Zombie(60 * cell_size, 24.0f * cell_size));
        em.addEnemy(new Zombie(65 * cell_size, 24.0f * cell_size));

        // MummyWarrior
        em.addEnemy(new MummyWarrior(8 * cell_size, 1530.0f));
        em.addEnemy(new MummyWarrior(18 * cell_size, 1530.0f));
        em.addEnemy(new MummyWarrior(28 * cell_size, 1530.0f));
        em.addEnemy(new MummyWarrior(38 * cell_size, 1530.0f));
        em.addEnemy(new MummyWarrior(48 * cell_size, 1530.0f));
        em.addEnemy(new MummyWarrior(57 * cell_size, 1530.0f));
        em.addEnemy(new MummyWarrior(63 * cell_size, 1530.0f));
        em.addEnemy(new MummyWarrior(66 * cell_size, 1530.0f));

        // AQUATIC ZOMBIES 4 batches
        em.addEnemy(new Zombie(138 * cell_size, 23.0f * cell_size));
        em.addEnemy(new Zombie(145 * cell_size, 23.0f * cell_size));
        em.addEnemy(new Zombie(153 * cell_size, 23.0f * cell_size));
        em.addEnemy(new Zombie(160 * cell_size, 23.0f * cell_size));
        em.addEnemy(new Zombie(168 * cell_size, 23.0f * cell_size));
        em.addEnemy(new Zombie(175 * cell_size, 23.0f * cell_size));
        em.addEnemy(new Zombie(183 * cell_size, 23.0f * cell_size));
        em.addEnemy(new Zombie(190 * cell_size, 23.0f * cell_size));

        // FLYING TARA 3 batches across full level
        em.addEnemy(new FlyingTara(15 * cell_size, 8.0f * cell_size));
        em.addEnemy(new FlyingTara(40 * cell_size, 8.0f * cell_size));
        em.addEnemy(new FlyingTara(70 * cell_size, 8.0f * cell_size));
        em.addEnemy(new FlyingTara(100 * cell_size, 8.0f * cell_size));
        em.addEnemy(new FlyingTara(140 * cell_size, 8.0f * cell_size));
        em.addEnemy(new FlyingTara(175 * cell_size, 8.0f * cell_size));
    }
};
// LEVEL MANAGER
class LevelManager {
    Level* current;
    int    currentLevel;
public:
    LevelManager() : current(new Level1()), currentLevel(1) {}

    void nextLevel() {
        delete current;
        current = nullptr;
        currentLevel++;
        if (currentLevel == 2) current = new Level2();
        else if (currentLevel == 3) current = new Level3();
        else { current = new Level1(); currentLevel = 1; }
    }

    void generateMap() { 
        current->generateMap(); }
    void spawnEnemies(EntityManager& em) {
        current->spawnEnemies(em); }
    int  getCurrentLevel()   const {
        return currentLevel; }
    Level* getLevel()    const {
        return current; }
    void reset() { 
        delete current; 
        current = nullptr;
        current = new Level1();
        currentLevel = 1;

    }
    ~LevelManager() { 
        delete current; 
        current = nullptr;
    }
};
//level generation
void generateLevel1() {
    // Fill everything with sky
    for (int r = 0; r < LEVEL_H; r++)
        for (int c = 0; c < LEVEL_W; c++)
            levelMap[r][c] = T_SKY;

    // biome 1
    // Flat ground at row 25 with small hills only
    for (int c = 0; c < 68; c++) {
        levelMap[25][c] = T_GRASS;
        levelMap[26][c] = T_DIRT;
        levelMap[27][c] = T_DIRT;
    }

    // Small hill 1 centered at col 15
    for (int c = 8; c < 23; c++) {
        int h = 2 - abs(c - 15) / 4;
        if (h < 0) h = 0;
        for (int r = 20; r <= 25; r++) levelMap[r][c] = T_SKY;
        levelMap[25 - h][c] = T_GRASS;
        for (int r = 25 - h + 1; r <= 27; r++) levelMap[r][c] = T_DIRT;
    }

    // Small hill 2 centered at col 45
    for (int c = 38; c < 53; c++) {
        int h = 2 - abs(c - 45) / 4;
        if (h < 0) h = 0;
        for (int r = 20; r <= 25; r++) levelMap[r][c] = T_SKY;
        levelMap[25 - h][c] = T_GRASS;
        for (int r = 25 - h + 1; r <= 27; r++) levelMap[r][c] = T_DIRT;
    }

    // Floating platform in plains
    for (int c = 28; c < 36; c++) {
        levelMap[21][c] = T_GRASS;
        levelMap[22][c] = T_DIRT;
    }
    //biome 2

    for (int c = 68; c <= 104; c++) {
        int dist = abs(c - 86);
        int peakRow = 4 + dist;        //sharp diamond slope
        if (peakRow > 24) peakRow = 24;
        for (int r = peakRow; r <= 27; r++)
            levelMap[r][c] = T_MOUNT;
        for (int r = 0; r < peakRow; r++)
            levelMap[r][c] = T_SKY;
    }

    for (int c = 105; c <= 135; c++) {
        int dist = abs(c - 119);
        int peakRow = 4 + dist;
        if (peakRow > 24) peakRow = 24;
        for (int r = peakRow; r <= 27; r++)
            levelMap[r][c] = T_MOUNT;
        for (int r = 0; r < peakRow; r++)
            levelMap[r][c] = T_SKY;
    }
    //fill with water
    for (int c = 136; c < LEVEL_W; c++)
        for (int r = 25; r <= LEVEL_H - 2; r++)
            levelMap[r][c] = T_WATER;

    // Sky above water
    for (int c = 136; c < LEVEL_W; c++)
        for (int r = 0; r < 25; r++)
            levelMap[r][c] = T_SKY;

    //bridge 1
    for (int c = 147; c <= 153; c++)
        levelMap[24][c] = T_ROCK;
    for (int r = 25; r <= 30; r++)
        levelMap[r][150] = T_ROCK;

    // Bridge 2
    for (int c = 167; c <= 173; c++)
        levelMap[24][c] = T_ROCK;
    for (int r = 25; r <= 30; r++)
        levelMap[r][170] = T_ROCK;

    //Bridge 3
    for (int c = 187; c <= 193; c++)
        levelMap[24][c] = T_ROCK;
    for (int r = 25; r <= 30; r++)
        levelMap[r][190] = T_ROCK;
    //indestructible bottom row (it i s the all width)
    for (int c = 0; c < LEVEL_W; c++)
        levelMap[LEVEL_H - 1][c] = T_ROCK;
}


//collision helper
bool isSolid(char tile) {
    return tile != T_SKY && tile != T_WATER;
}

void resolveCollision(PlayerSoldier* player, float gravity, float& max_speed) {
    float px = player->getX();
    float py = player->getY();
    float pw = player->getW();
    float ph = player->getH();
    float vx = player->getVelX();
    float vy = player->getVelY();

    player->setOnGround(false);

    // VERTICAL(feet)
    int cL = max(0, min((int)(px / cell_size), LEVEL_W - 1));
    int cR = max(0, min((int)((px + pw - 1) / cell_size), LEVEL_W - 1));
    int rFeet = max(0, min((int)((py + ph) / cell_size), LEVEL_H - 1));

    if (isSolid(levelMap[rFeet][cL]) || isSolid(levelMap[rFeet][cR])) {
        player->setVelY(0);
        player->setPos(px, (float)(rFeet * cell_size) - ph);
        player->setOnGround(true);
        py = player->getY();
    }

    // VERTICAL(head)
    int rHead = max(0, min((int)(py / cell_size), LEVEL_H - 1));
    if (isSolid(levelMap[rHead][cL]) || isSolid(levelMap[rHead][cR])) {
        player->setVelY(0);
        player->setPos(px, (float)((rHead + 1) * cell_size));
        py = player->getY();
    }

    // HORIZONTAL
    int rTop = max(0, min((int)(py / cell_size), LEVEL_H - 1));
    int rBot = max(0, min((int)((py + ph - 1) / cell_size), LEVEL_H - 1));

    if (vx > 0) {
        int cRight = max(0, min((int)((px + pw) / cell_size), LEVEL_W - 1));
        if (isSolid(levelMap[rTop][cRight]) ||
            isSolid(levelMap[rBot][cRight])) {
            player->setVelX(0);
            player->setPos((float)(cRight * cell_size) - pw, py);
        }
    }
    else if (vx < 0) {
        int cLeft = max(0, min((int)(px / cell_size), LEVEL_W - 1));
        if (isSolid(levelMap[rTop][cLeft]) ||
            isSolid(levelMap[rBot][cLeft])) {
            player->setVelX(0);
            player->setPos((float)((cLeft + 1) * cell_size), py);
        }
    }

    // WATER CHECK
    int cc = max(0, min((int)((px + pw / 2) / cell_size), LEVEL_W - 1));
    int cr = max(0, min((int)((py + ph / 2) / cell_size), LEVEL_H - 1));
    player->setInWater(levelMap[cr][cc] == T_WATER);

    // bounds
    px = player->getX(); py = player->getY();
    if (px < 0) player->setPos(0, py);
    if (px + pw > LEVEL_W * cell_size)
        player->setPos(LEVEL_W * cell_size - pw, py);
    if (py < 0) { player->setPos(px, 0); player->setVelY(0); }
}
void resolveEnemyCollision(float& ex, float& ey, float ew, float eh,
    float& velY, bool& onGround) {
    onGround = false;
    int cL = max(0, min((int)(ex / cell_size), LEVEL_W - 1));
    int cR = max(0, min((int)((ex + ew - 1) / cell_size), LEVEL_W - 1));
    int rFeet = max(0, min((int)((ey + eh - 1) / cell_size), LEVEL_H - 1));
    if (isSolid(levelMap[rFeet][cL]) || isSolid(levelMap[rFeet][cR])) {
        velY = 0; ey = (float)(rFeet * cell_size) - eh; onGround = true;
    }
    int rHead = max(0, min((int)(ey / cell_size), LEVEL_H - 1));
    if (isSolid(levelMap[rHead][cL]) || isSolid(levelMap[rHead][cR])) {
        velY = 0; ey = (float)((rHead + 1) * cell_size);
    }
    int rTop = max(0, min((int)(ey / cell_size), LEVEL_H - 1));
    int rBot = max(0, min((int)((ey + eh - 1) / cell_size), LEVEL_H - 1));
    int cRight = max(0, min((int)((ex + ew) / cell_size), LEVEL_W - 1));
    if (isSolid(levelMap[rTop][cRight]) || isSolid(levelMap[rBot][cRight]))
        ex = (float)(cRight * cell_size) - ew;
    int cLeft = max(0, min((int)(ex / cell_size), LEVEL_W - 1));
    if (isSolid(levelMap[rTop][cLeft]) || isSolid(levelMap[rBot][cLeft]))
        ex = (float)((cLeft + 1) * cell_size);
    if (ex < 0) ex = 0;
    if (ey < 0) { ey = 0; velY = 0; }
}
void scaleTile(Sprite& s, Texture& t, int cell_size) {
    s.setTexture(t);
    s.setScale((float)cell_size / t.getSize().x,
        (float)cell_size / t.getSize().y);
}
const int ROCK_COUNT = 12;
//main
int main() {
    RenderWindow window(VideoMode(screen_x, screen_y), "Metal Slug", Style::Close);
    window.setFramerateLimit(60);

    Clock clock, animClock;
    DeveloperMode devMode;
    bool vHeld = false, hHeld = false;
    //textures
    Texture  grassTex, dirtTex, waterTex,
        mountainTex, indestructTex, menuBgTex, airTex;
    Texture bgTex;
    if (!bgTex.loadFromFile("25I-0832_25I-0711_Assets/background.png"))
        cout << "Warning: background.png not found!" << endl;
    bgTex.setRepeated(false);

    Texture rockSmallTex;
    if (!rockSmallTex.loadFromFile("25I-0832_25I-0711_Assets/Blocks/andesite 1.png"))
        cout << "rock small not found\n";
    Sprite rockSmallSp;
    rockSmallSp.setTexture(rockSmallTex);
    rockSmallSp.setScale(
        48.0f / rockSmallTex.getSize().x,
        48.0f / rockSmallTex.getSize().y);

    Sprite bgSprite;
    bgSprite.setTexture(bgTex);
    bgSprite.setScale(
        (float)screen_x / bgTex.getSize().x,
        (float)screen_y / bgTex.getSize().y);

    airTex.loadFromFile("25I-0832_25I-0711_Assets/Blocks/air.png");
    grassTex.loadFromFile("25I-0832_25I-0711_Assets/blocks/grass_block_side.png");
    dirtTex.loadFromFile("25I-0832_25I-0711_Assets/blocks/dirt.png");
    waterTex.loadFromFile("25I-0832_25I-0711_Assets/blocks/water.png");
    mountainTex.loadFromFile("25I-0832_25I-0711_Assets/blocks/stone.png");
    indestructTex.loadFromFile("25I-0832_25I-0711_Assets/blocks/andesite.png");
    menuBgTex.loadFromFile("25I-0832_25I-0711_Assets/menu_bg.png");

    //sprites
    Sprite  grassSp, dirtSp, waterSp,
        mountainSp, indestructSp, menuBgSp, airSp;
    scaleTile(airSp, airTex, cell_size);
    scaleTile(grassSp, grassTex, cell_size);
    scaleTile(dirtSp, dirtTex, cell_size);
    scaleTile(waterSp, waterTex, cell_size);
    scaleTile(mountainSp, mountainTex, cell_size);
    scaleTile(indestructSp, indestructTex, cell_size);

    menuBgSp.setTexture(menuBgTex);
    menuBgSp.setScale((float)screen_x / menuBgTex.getSize().x,
        (float)screen_y / menuBgTex.getSize().y);
    //fontt
    Font font;
    font.loadFromFile("arial.ttf");
    Text hintText;
    hintText.setFont(font);
    hintText.setString("1=Survival   2=Campaign   3=Leaderboard   ESC=Quit");
    hintText.setCharacterSize(28);
    hintText.setFillColor(Color::White);
    hintText.setPosition(screen_x / 2.0f - 200, screen_y - 50.0f);

    //perlin and profile
    PerlinNoise noise;
    LevelProfile* profile = createProfile("Normal");
    CharacterRoster roster;
    VehicleManager vehicles;
    EnemyVehicleManager enemyVehicles;

    // Enemy vehicles

    PlayerSoldier* player = roster.getActive();  // points to current active character

    generateLevel1();
    vehicles.spawnDefaultVehicles();
    //entity manager
    EntityManager entityManager;
    //score manader
    ScoreManager scoreManager;
    scoreManager.initLeaderboard((float)screen_x, (float)screen_y);
   
    float enemySpawnTimer = 3.0f;

    //WEAPON SYSTEM SETUP
    //projectile pool into every character's weapons
    {
        Projectile** pool = entityManager.getProjectilePool();
        int* pCount = entityManager.getProjectileCountPtr();
        int  pMax = entityManager.getMaxProjectiles();
        for (int i = 0; i < roster.getCount(); i++) {
            roster.getChar(i)->setProjectilePool(pool, pCount, pMax);
            roster.getChar(i)->giveAllWeapons(pool, pCount, pMax);
        }
    }

    //Laser beam
    LaserBeam laserBeam;

    //Fire pool array (Eri's fire bomb puddles)
    const int MAX_POOLS = 20;
    FirePool* firePools[MAX_POOLS];
    int firePoolCount = 0;
    for (int i = 0; i < MAX_POOLS; i++) firePools[i] = nullptr;

    //Explosion array (rocket/grenade impacts)
    const int MAX_EXPL = 20;
    Explosion* explosions[MAX_EXPL];
    int explCount = 0;
    for (int i = 0; i < MAX_EXPL; i++)
        explosions[i] = nullptr;

    // Plains biome stationary shooters spread across level
    LevelManager levelManager;
    int currentLevel = 1;
  
    //as they wil be usd in the next levels also 
    const int MAX_POW = 10;
    POWPrisoner* pows[MAX_POW] = { nullptr };
    int powCount = 0;
    levelManager.generateMap();
     levelManager.spawnEnemies(entityManager);
    //CAMERA
    float cameraX = 0, cameraY = (float)(15 * cell_size);

    //PHYSICS
    float gravity = 0.5f;
    float max_speed = 5.0f;

    GameMode currentMode = MENU;

    //MUSIC
    Music bgMusic;
    if (!bgMusic.openFromFile("background.ogg"))
        cout << "Warning: background.ogg not found!\n";
    bgMusic.setLoop(true);
    bgMusic.play();
    // rock positions in world coordinates (x, y)

    const int ROCK_COUNT = 20;
    float rockX[ROCK_COUNT] = {
        // mountain 1 peak 
        15 * cell_size + 10,
        17 * cell_size + 20,
        19 * cell_size + 5,
        21 * cell_size + 35,
        23 * cell_size + 15,
        // mountain 2 peak 
        62 * cell_size + 10,
        64 * cell_size + 30,
        66 * cell_size + 5,
        68 * cell_size + 40,
        70 * cell_size + 20,
        // mountain 3 peak
        112 * cell_size + 10,
        114 * cell_size + 25,
        116 * cell_size + 5,
        118 * cell_size + 35,
        120 * cell_size + 15,
        // mountain 4 peak 
        166 * cell_size + 10,
        168 * cell_size + 30,
        170 * cell_size + 5,
        172 * cell_size + 40,
        174 * cell_size + 20
    };
    float rockY[ROCK_COUNT] = {
        // mountain 1 
        4 * cell_size + 20,
        5 * cell_size + 10,
        4 * cell_size + 35,
        5 * cell_size + 5,
        4 * cell_size + 25,
        // mountain 2
        4 * cell_size + 15,
        5 * cell_size + 30,
        4 * cell_size + 5,
        5 * cell_size + 20,
        4 * cell_size + 40,
        // mountain 3
        4 * cell_size + 20,
        5 * cell_size + 10,
        4 * cell_size + 35,
        5 * cell_size + 5,
        4 * cell_size + 25,
        // mountain 4
        4 * cell_size + 15,
        5 * cell_size + 30,
        4 * cell_size + 5,
        5 * cell_size + 20,
        4 * cell_size + 40
    };
    //game loop
    Event ev;
    bool inWater = false;
    while (window.isOpen()) {
        float dt = animClock.restart().asSeconds();
        //mouse world position (used for aimed fire)
        float mouseWorldX = (float)Mouse::getPosition(window).x + cameraX;
        float mouseWorldY = (float)Mouse::getPosition(window).y + cameraY;
        while (window.pollEvent(ev)) {

            if (ev.type == Event::Closed) window.close();

            if (currentMode == MENU && ev.type == Event::KeyPressed) {
                if (ev.key.code == Keyboard::Num1 && !scoreManager.isLeaderboardOpen()) {
                    currentMode = SURVIVAL;
                    player->setPos(2 * cell_size, (float)(23 * cell_size));
                    player->setVelX(0); player->setVelY(0);
                    cameraX = 0;
                    cameraY = (float)(12 * cell_size);
                    clock.restart();

                    //reset and respawn POWs
                    for (int i = 0; i < MAX_POW; i++) {
                        delete pows[i];
                        pows[i] = nullptr;
                    }
                    powCount = 0;

                    if (currentLevel == 1) {
                        pows[powCount++] = new POWPrisoner(33 * cell_size, 23.0f * cell_size);
                        pows[powCount++] = new POWPrisoner(95 * cell_size, 6.0f * cell_size);
                        pows[powCount++] = new POWPrisoner(160 * cell_size, 23.0f * cell_size);
                    }
                    else if (currentLevel == 2) {
                        pows[powCount++] = new POWPrisoner(20 * cell_size, 23.0f * cell_size);
                        pows[powCount++] = new POWPrisoner(45 * cell_size, 23.0f * cell_size);
                        pows[powCount++] = new POWPrisoner(95 * cell_size, 6.0f * cell_size);
                        pows[powCount++] = new POWPrisoner(160 * cell_size, 23.0f * cell_size);
                    }
                    else if (currentLevel == 3) {
                        pows[powCount++] = new POWPrisoner(20 * cell_size, 23.0f * cell_size);
                        pows[powCount++] = new POWPrisoner(45 * cell_size, 23.0f * cell_size);
                        pows[powCount++] = new POWPrisoner(85 * cell_size, 6.0f * cell_size);
                        pows[powCount++] = new POWPrisoner(110 * cell_size, 6.0f * cell_size);
                        pows[powCount++] = new POWPrisoner(150 * cell_size, 23.0f * cell_size);
                        pows[powCount++] = new POWPrisoner(175 * cell_size, 23.0f * cell_size);
                    }

                    // reset and respawn enemy vehicles
                    enemyVehicles.clearAll();

                    if (currentLevel == 1) {
                        Bradley* b1 = new Bradley(80 * cell_size, 24 * cell_size);
                        b1->loadSprites();
                        enemyVehicles.add(b1);
                        EnemySub* sub1 = new EnemySub(180 * cell_size, 27 * cell_size);
                        sub1->loadSprites();
                        enemyVehicles.add(sub1);
                    }
                    else if (currentLevel == 2) {
                        Bradley* b1 = new Bradley(80 * cell_size, 24 * cell_size);
                        b1->loadSprites();
                        enemyVehicles.add(b1);
                        Bradley* b2 = new Bradley(140 * cell_size, 24 * cell_size);
                        b2->loadSprites();
                        enemyVehicles.add(b2);
                        EnemySub* sub1 = new EnemySub(180 * cell_size, 27 * cell_size);
                        sub1->loadSprites();
                        enemyVehicles.add(sub1);
                    }
                    else if (currentLevel == 3) {
                        Bradley* b1 = new Bradley(80 * cell_size, 24 * cell_size);
                        b1->loadSprites();
                        enemyVehicles.add(b1);
                        Bradley* b2 = new Bradley(140 * cell_size, 24 * cell_size);
                        b2->loadSprites();
                        enemyVehicles.add(b2);
                        EnemySub* sub1 = new EnemySub(165 * cell_size, 27 * cell_size);
                        sub1->loadSprites();
                        enemyVehicles.add(sub1);
                        EnemySub* sub2 = new EnemySub(185 * cell_size, 27 * cell_size);
                        sub2->loadSprites();
                        enemyVehicles.add(sub2);
                    }
                }
                if (ev.key.code == Keyboard::Num2 && !scoreManager.isLeaderboardOpen()) {
                    currentMode = CAMPAIGN;
                    player->setPos(800, 100);
                    player->setVelX(0); player->setVelY(0);
                    cameraX = 0;
                    cameraY = 0;
                    clock.restart();
                   

                }

                //da lb
                if (ev.key.code == Keyboard::Num3) {
                    scoreManager.toggleLeaderboard();
                }
            }
            if (ev.type == Event::KeyPressed && ev.key.code == Keyboard::V)
                vHeld = true;
            if (ev.type == Event::KeyPressed && ev.key.code == Keyboard::H)
                hHeld = true;
            if (ev.type == Event::KeyReleased && ev.key.code == Keyboard::V)
                vHeld = false;
            if (ev.type == Event::KeyReleased && ev.key.code == Keyboard::H)
                hHeld = false;

            // toggle dev mode when both held
            devMode.setActive(vHeld && hHeld);
            // Z KEY for the next character
            if (currentMode != MENU &&
                ev.type == Event::KeyPressed &&
                ev.key.code == Keyboard::Z) {
                roster.switchToNext();
                player = roster.getActive();
            }


            // JUMP
            if (currentMode == SURVIVAL &&
                ev.type == Event::KeyPressed &&
                ev.key.code == Keyboard::Up)
                player->jump();

            // WEAPON SWITCH 
            if (currentMode != MENU && ev.type == Event::KeyPressed) {
                if (ev.key.code == Keyboard::Q) 
                    player->switchWeapon(0); // pistol
                if (ev.key.code == Keyboard::E) 
                    player->switchWeapon(1); // HMG
                if (ev.key.code == Keyboard::R) 
                    player->switchWeapon(2); // Rocket
                if (ev.key.code == Keyboard::T) 
                    player->switchWeapon(3); // Flame
                if (ev.key.code == Keyboard::Y) 
                    player->switchWeapon(4); // Laser
            }
            // FIRE ( left mouse button for aimed fire) 
            if (currentMode != MENU && ev.type == Event::MouseButtonPressed &&ev.mouseButton.button == Mouse::Left) {
                if (vehicles.isInVehicle()) {
                    vehicles.tryFireVehicle(
                        entityManager.getProjectilePool(),
                        entityManager.getProjectileCountPtr(),
                        entityManager.getMaxProjectiles());
                }
                else {
                    int slot = player->getInventory().activeWeapon;
                    if (slot != 1 && slot != 3)  //HMG and flame are held not clicked
                        player->tryFireAtMouse(mouseWorldX, mouseWorldY);
                }
            }
            // FIRE  (Space bar still fires horizontally)
            if (currentMode != MENU &&ev.type == Event::KeyPressed &&ev.key.code == Keyboard::Space) {
                if (vehicles.isInVehicle()) {
                    vehicles.tryFireVehicle(
                        entityManager.getProjectilePool(),
                        entityManager.getProjectileCountPtr(),
                        entityManager.getMaxProjectiles());
                }
                else {
                    int slot = player->getInventory().activeWeapon;
                    if (slot != 1 && slot != 3)
                        player->tryFire();
                }
            }

            // MELEE (KNIFE) by  X key
            if (currentMode != MENU &&
                ev.type == Event::KeyPressed &&
                ev.key.code == Keyboard::X) {
                if (player->tryMelee()) {
                    float meleeRange = player->getW() + 20.0f;
                    for (int i = 0; i < entityManager.getEnemyCount(); i++) {
                        Enemy* e = entityManager.getEnemy(i);
                        if (!e || !e->isActive()) 
                            continue;
                        float dx = (e->getX() + e->getW() / 2.0f)
                            - (player->getX() + player->getW() / 2.0f);
                        float dy = (e->getY() + e->getH() / 2.0f)
                            - (player->getY() + player->getH() / 2.0f);
                        bool inRange = (dx * dx + dy * dy) <= meleeRange * meleeRange;
                        bool inFront = player->isFacingRight() ? dx > 0 : dx < 0;
                        if (inRange && inFront) {
                            bool wasAlive = e->isActive();
                            e->takeDamage((int)(BASE_MELEE_DMG * player->getMeleeDmgMult()));
                            if (wasAlive && !e->isActive()) {
                                float sx = e->getX() - cameraX;
                                float sy = e->getY() - cameraY;
                                scoreManager.awardEnemyKill(
                                    e->getScoreValue(),!player->isOnGround(),  // aerial bonus
                                    true,                   // melee = +50
                                    sx, sy);
                            }
                        }
                    }
                }
            }

            // GRENADE G key
            if (currentMode != MENU &&ev.type == Event::KeyPressed &&ev.key.code == Keyboard::G) {
                if (player->tryThrowGrenade()) {
                    ProjectileWeapon* gw = player->getGrenadeWeapon();
                    if (gw) {
                        float mx = gw->getMuzzleX(player->getX(), player->isFacingRight());
                        float my = gw->getMuzzleY(player->getY());
                        gw->fire(mx, my, player->isFacingRight());
                    }
                }
            }
            // ENTER / EXIT VEHICLE  F key
            if (currentMode != MENU &&
                ev.type == Event::KeyPressed &&
                ev.key.code == Keyboard::F) {
                if (vehicles.isInVehicle()) {
                    vehicles.tryExit(false, false);
                }
                else {
                    bool isTarma = (strcmp(player->getName(), "Tarma") == 0);
                    vehicles.tryEnter(player->getX(), player->getY(), isTarma);
                }
            }
            // ESC
            if (ev.type == Event::KeyPressed && ev.key.code == Keyboard::Escape) {
                if (currentMode != MENU) {
                    scoreManager.tryInsertScore("PLAYER");
                    scoreManager.reset();
                    currentMode = MENU;
                }
                else if (scoreManager.isLeaderboardOpen()) {
                    scoreManager.closeLeaderboard();//close overlay first
                }
                else {
                    window.close();
                }
            }
        }
        //menu
        if (currentMode == MENU) {
            float pulse = (sin(clock.getElapsedTime().asSeconds() * 3) + 1) / 2.0f;
            hintText.setFillColor(Color(255, 255, 255, (Uint8)(80 + pulse * 175)));
            window.clear(Color::Black);
            window.draw(menuBgSp);

            // update + draw leaderboard overlay if open
            scoreManager.update(dt);
            scoreManager.drawLeaderboardScreen(window);   // draws nothing when closed

            window.draw(hintText);
            window.display();
            continue;
        }





        // HMG (slot 1) and FlameShot (slot 3) fire while Space is held
        if (currentMode != MENU) {
            int slot = player->getInventory().activeWeapon;
            if (Mouse::isButtonPressed(Mouse::Left) &&
                (slot == 1 || slot == 3))
                player->tryFireAtMouse(mouseWorldX, mouseWorldY);
            // Space still works as horizontal fallback for held weapons
            if (Keyboard::isKeyPressed(Keyboard::Space) &&
                (slot == 1 || slot == 3))
                player->tryFire();
        }

        // Laser 
        if (currentMode != MENU) {
            ProjectileWeapon* aw = player->getActiveProjectileWeapon();
            if (aw && aw->isLaser()) {
                LaserGun* lg = (LaserGun*)aw;  // safe isLaser() guarantees type
                if (lg->laserFiredThisFrame) {
                    // spawn beam visual from muzzle position
                    laserBeam.spawn(lg->lastMuzzleX, lg->lastMuzzleY,
                        player->isFacingRight());
                    // instant kill all enemies in same Y band in beam direction
                    Projectile** pool = entityManager.getProjectilePool();
                    int* pCount = entityManager.getProjectileCountPtr();
                    int  pMax = entityManager.getMaxProjectiles();
                    for (int i = 0; i < entityManager.getEnemyCount(); i++) {
                        Enemy* e = entityManager.getEnemy(i);
                        if (!e || !e->isActive()) continue;
                        bool sameY = (e->getY() < lg->lastMuzzleY + 60) &&
                            (e->getY() + e->getH() > lg->lastMuzzleY - 20);
                        bool inPath = player->isFacingRight()
                            ? e->getX() > player->getX()
                            : e->getX() + e->getW() < player->getX();
                        if (sameY && inPath) {
                            bool wasAlive = e->isActive();
                            e->setActive(false);
                            if (wasAlive) {
                                float sx = e->getX() - cameraX;
                                float sy = e->getY() - cameraY;
                                scoreManager.awardEnemyKill(
                                    e->getScoreValue(),
                                    !player->isOnGround(), false, sx, sy);
                            }
                        }
                    }
                }
            }
        }
        //physice
        if (currentMode == SURVIVAL || currentMode == CAMPAIGN) {


            if (devMode.isActive()) {
                devMode.handleMovement(player);
                devMode.removeBlockUnderPlayer(player);
                devMode.autoShoot(dt, player,
                    entityManager.getProjectilePool(),
                    entityManager.getProjectileCountPtr(),
                    entityManager.getMaxProjectiles());
                player->move();
            }

            // V + H together the developer mode
            bool vKey = Keyboard::isKeyPressed(Keyboard::V);
            bool hKey = Keyboard::isKeyPressed(Keyboard::H);
            if (vKey && hKey) {
                float flySpeed = 4.0f;
                if (Keyboard::isKeyPressed(Keyboard::Up))
                    player->setPos(player->getX(), player->getY() - flySpeed);
                if (Keyboard::isKeyPressed(Keyboard::Down))
                    player->setPos(player->getX(), player->getY() + flySpeed);
                if (Keyboard::isKeyPressed(Keyboard::Right)) {
                    player->setPos(player->getX() + flySpeed, player->getY());
                    player->setFacingRight(true);
                }
                if (Keyboard::isKeyPressed(Keyboard::Left)) {
                    player->setPos(player->getX() - flySpeed, player->getY());
                    player->setFacingRight(false);
                }
                player->setVelY(0);   // no gravity while flying
                player->setVelX(0);
            }
            float moveSpeed = 3.0f;

            if (vehicles.isInVehicle()) {
                // vehicle controls motion —don't run player physics at all
                bool isTarma = (strcmp(player->getName(), "Tarma") == 0);
                vehicles.handleInput(moveSpeed, isTarma);
                Vehicle* v = vehicles.getPiloted();
                player->setPos(v->getX(), v->getY());
            }
            else {
                // normal on foot movement
                if (Keyboard::isKeyPressed(Keyboard::Right)) {
                    player->setVelX(moveSpeed);
                    player->setFacingRight(true);
                }
                else if (Keyboard::isKeyPressed(Keyboard::Left)) {
                    player->setVelX(-moveSpeed);
                    player->setFacingRight(false);
                }
                else {
                    player->setVelX(0);
                }
                ///detect water by aquatic biome position
                {
                    float waterLeft = 136.0f * cell_size;
                    float waterRight = 203.0f * cell_size;
                    float waterTop = 25.0f * cell_size;
                    bool inWaterZone = (player->getX() >= waterLeft &&
                        player->getX() <= waterRight &&
                        player->getY() + player->getH() >= waterTop);
                    player->setInWater(inWaterZone);
                }

                float grav = player->isInWater() ? 0.1f : 0.5f;
                max_speed = player->isInWater() ? 3.0f : 5.0f;
                if (player->isInWater()) {
                    // very slow sink
                    float waterVelY = player->getVelY();
                    waterVelY += 0.03f;                          // tiny gravity
                    if (waterVelY > 0.8f) waterVelY = 0.8f;     // max sink speed
                    if (Keyboard::isKeyPressed(Keyboard::Up))
                        waterVelY = -2.5f;                       // swim up
                    if (Keyboard::isKeyPressed(Keyboard::Down))
                        waterVelY = 2.5f;                        // swim down
                    player->setVelY(waterVelY);
                    player->setVelX(player->getVelX() * 0.85f); // slight drag
                }
                else {
                    player->applyGravity(grav);
                }
                player->move();
                if (!player->isInWater())
                    resolveCollision(player, grav, max_speed);
            }

            // update vehicle physics first so position is correct before camera
            vehicles.update(dt, levelMap, LEVEL_W, LEVEL_H, cell_size);
            if (vehicles.isInVehicle()) {
                Vehicle* v = vehicles.getPiloted();
                player->setPos(v->getX(), v->getY());
            }

            // CAMERA
            cameraX = player->getX() - screen_x / 2.0f;
            cameraY = player->getY() - screen_y / 2.0f;
            if (cameraX < 0) cameraX = 0;
            if (cameraX > LEVEL_W * cell_size - screen_x)
                cameraX = LEVEL_W * cell_size - screen_x;
            if (cameraY < 0) cameraY = 0;
            if (cameraY > LEVEL_H * cell_size - screen_y)
                cameraY = LEVEL_H * cell_size - screen_y;
            //LEVEL TRANSITION CHECK
            bool allDead = true;
            for (int i = 0; i < entityManager.getEnemyCount(); i++)
                if (entityManager.getEnemy(i) && entityManager.getEnemy(i)->isActive())
                {
                    allDead = false; break;
                }

            bool atEnd = player->getX() >= 200 * cell_size && player->getX() <= 204 * cell_size;

            if (allDead && atEnd) {
                bool noHits = (player->getLives() == BASE_LIVES);
                scoreManager.awardSurvivalClear(noHits);
                if (currentLevel == 1) {
                    currentLevel = 2;
                    levelManager.nextLevel();
                }
                else if (currentLevel == 2) {
                    currentLevel = 3;
                    levelManager.nextLevel();
                }
                else {
                    scoreManager.tryInsertScore("PLAYER");
                    scoreManager.reset();
                    currentMode = MENU;
                    currentLevel = 1;
                    levelManager.reset();
                }

                // reset entity manager
                entityManager.~EntityManager();
                new (&entityManager) EntityManager();

                // rewire weapons
                Projectile** pool = entityManager.getProjectilePool();
                int* pCount = entityManager.getProjectileCountPtr();
                int  pMax = entityManager.getMaxProjectiles();
                for (int i = 0; i < roster.getCount(); i++) {
                    roster.getChar(i)->setProjectilePool(pool, pCount, pMax);
                    roster.getChar(i)->giveAllWeapons(pool, pCount, pMax);
                }

                if (currentMode != MENU) {
                    levelManager.generateMap();
                    levelManager.spawnEnemies(entityManager);
                    player->setPos(2 * cell_size, 23.0f * cell_size);
                    player->setVelX(0); player->setVelY(0);
                    cameraX = 0; cameraY = 12.0f * cell_size;
                }
            }
        }
        else // CAMPAIGN
        {
            // movement
            float moveSpeed = 3.0f;
            if (Keyboard::isKeyPressed(Keyboard::Right)) {
                player->setVelX(moveSpeed);
                player->setFacingRight(true);
            }
            else if (Keyboard::isKeyPressed(Keyboard::Left)) {
                player->setVelX(-moveSpeed);
                player->setFacingRight(false);
            }
            else { player->setVelX(0); }

            // jump only when on ground
            if (Keyboard::isKeyPressed(Keyboard::Up) && player->isOnGround())
                player->jump();

            int col = (int)((player->getX() + player->getW() / 2) / cell_size);
            
            double h = noise.fractalNoise(
                col * profile->getFrequency(), 0.0,
                profile->getOctaves(),
                profile->getPersistence(),
                profile->getLacunarity());
            h *= profile->getAmplitude();
            float groundY = (float)(getTerrainY(h, screen_y, cell_size)
                * cell_size);
            if (player->getY() + player->getH() >= groundY) {
                player->setPos(player->getX(), groundY - player->getH());
                player->setVelY(0);
                player->setOnGround(true);
            }
            else {                          
                player->setOnGround(false); 
            }
            player->applyGravity(0.5f);
            player->move();
            cameraX = player->getX() - screen_x / 2.0f;
            if (cameraX < 0) cameraX = 0;
            cameraY = player->getY() - screen_y / 2.0f;  
            if (cameraY < 0) cameraY = 0;
        }

        scoreManager.setLevel(currentLevel);
        scoreManager.updateLives(player->getLives());
        scoreManager.update(dt);

        //UPDATE
        player->update(dt);
        // auto switch to next player when current dies
        if (!player->isAlive()) {
            if (roster.anyAlive()) {
                roster.switchToNext();
                player = roster.getActive();
                player->setPos(2 * cell_size, 23.0f * cell_size);
            }
        }
        PlayerSoldier* active = roster.getActive();
        float player_x = active->getX();
        float player_y = active->getY();
        // update projectiles always
        entityManager.updateProjectiles(dt);



        // get projectile pool pointers for enemy shooting
        Projectile** pool = entityManager.getProjectilePool();
        int* pCount = entityManager.getProjectileCountPtr();
        int   pMax = entityManager.getMaxProjectiles();
        //player
        active->setPosition(player_x, player_y);
        active->setVelocity(player->getVelX(), player->getVelY());
        active->setOnGround(player->isOnGround());

        roster.update(dt);
        active->update(dt);
        enemyVehicles.update(dt, player->getX(), player->getY(), pool, pCount, pMax, levelMap, cell_size);
        player = roster.getActive();  // re sync in case Z was pressed


        // update each enemy  only activate when on screen
        for (int i = 0; i < entityManager.getEnemyCount(); i++) {
            Enemy* e = entityManager.getEnemy(i);
            if (!e || !e->isActive()) continue;

            // check if this enemy is visible in the camera window
            bool onScreen = (e->getX() + e->getW() > cameraX) &&
                (e->getX() < cameraX + screen_x);
            e->setOnScreen(onScreen);

            // always update damage flash timer
            e->update(dt);

            if (onScreen) {
                e->updateAI(player->getX(), player->getY(), dt, cameraX, cameraY);

                if (!e->isMelee())
                    e->fireSpecial(player->getX(), player->getY(), pool, pCount, pMax);

                // martian beam damage — only unavoidable dynamic_cast
                Martian* martian = dynamic_cast<Martian*>(e);
                if (martian && martian->beamHitsPlayer(
                    player->getX(), player->getY(),
                    player->getW(), player->getH()))
                    player->takeDamage();
            }
        }
        // gravity and collision for ground enemies
        for (int i = 0; i < entityManager.getEnemyCount(); i++) {
            Enemy* e = entityManager.getEnemy(i);
            if (!e || !e->isActive()) continue;
            if (e->isAerial()) continue;
            if (e->isFlying()) continue;
            float ex = e->getX(), ey = e->getY();
            float ew = (float)e->getW(), eh = (float)e->getH();
            float vy = e->getVelY();
            vy += 0.5f;
            if (vy > 15.0f) vy = 15.0f;
            ey += vy;
            bool onGround = false;
            resolveEnemyCollision(ex, ey, ew, eh, vy, onGround);
            e->setPos(ex, ey);
            e->setVelY(vy);
        }
        // dynamic spawning for campaign mode
        if (currentMode == CAMPAIGN) {
            enemySpawnTimer -= dt;
            if (enemySpawnTimer <= 0) {
                enemySpawnTimer = 4.0f;
                int col = (int)(player->getX() / cell_size) + 15;
              
                float sx = col * (float)cell_size;
                
            }
            
        }

        entityManager.cleanup();

        for (int i = 0; i < powCount; i++) {
            if (!pows[i] || !pows[i]->isActive()) continue;
            pows[i]->update(dt);
            if (!pows[i]->isReleased() && Keyboard::isKeyPressed(Keyboard::P)) {
                float px = player->getX(), py = player->getY();
                float pw = player->getW(), ph = player->getH();
                float ex = pows[i]->getX(), ey = pows[i]->getY();
                float ew = pows[i]->getW(), eh = pows[i]->getH();
                bool hit = px < ex + ew && px + pw > ex &&
                    py < ey + eh && py + ph > ey;
                if (hit)
                    pows[i]->onInteract(player);
            }
        }

        entityManager.resolvePlayerProjectileHits(explosions, explCount, MAX_EXPL, firePools, firePoolCount, MAX_POOLS,
            player->getGrenadeWeapon(), &scoreManager, cameraX, cameraY, !player->isOnGround());

        entityManager.resolveEnemyProjectileHits(player);


        for (int i = 0; i < firePoolCount; i++) {
            if (!firePools[i] || !firePools[i]->isActive()) continue;
            firePools[i]->update(dt);
            if (firePools[i]->canDamageNow(dt))
                for (int j = 0; j < entityManager.getEnemyCount(); j++) {
                    Enemy* e = entityManager.getEnemy(j);
                    if (!e || !e->isActive()) continue;
                    if (firePools[i]->enemyInside(e->getX(), e->getY(),
                        e->getW(), e->getH()))
                        e->takeDamage(2);
                }
        }
        for (int i = 0; i < explCount; i++)
            if (explosions[i]) explosions[i]->update(dt);
        laserBeam.update(dt);

        //DRAWING

        window.clear(Color::Black);
        window.draw(bgSprite);

        //DRAW TILES
        if (currentMode == SURVIVAL) {
            int sc = max(0, (int)(cameraX / cell_size));
            int ec = min(LEVEL_W,
                sc + screen_x / cell_size + 2);
            for (int row = 0; row < LEVEL_H; row++)
                for (int col = sc; col < ec; col++) {
                    char tile = levelMap[row][col];
                    if (tile == T_SKY) {
                        float sx = (float)(col * cell_size) - cameraX;
                        float sy = (float)(row * cell_size) - cameraY;
                        airSp.setPosition(sx, sy);
                        window.draw(airSp);
                        continue;
                    }
                    float sx = (float)(col * cell_size) - cameraX;
                    float sy = (float)(row * cell_size) - cameraY;
                    Sprite* sp = nullptr;
                    switch (tile) {
                    case T_GRASS: sp = &grassSp;    
                        break;
                    case T_DIRT:  sp = &dirtSp;    
                        break;
                    case T_WATER: sp = &waterSp;   
                        break;
                    case T_MOUNT: sp = &mountainSp; 
                        break;
                    case T_ROCK:  sp = &indestructSp;
                        break;
                    default:      sp = &dirtSp;      
                        break;
                    }
                    sp->setPosition(sx, sy);
                    window.draw(*sp);
                }
            // draw small rock decorations on mountains
            for (int i = 0; i < ROCK_COUNT; i++) {
                float screenX = rockX[i] - cameraX;
                float screenY = rockY[i] - cameraY;
                // only draw if visible on screen
                if (screenX > -20 && screenX < screen_x &&
                    screenY > -20 && screenY < screen_y) {
                    rockSmallSp.setPosition(screenX, screenY);
                    window.draw(rockSmallSp);
                }
            }
        }
        else // CAMPAIGN
        {
            int sc = max(0, (int)(cameraX / cell_size) - 1);
            int ec = sc + screen_x / cell_size + 3;
            for (int col = sc; col < ec; col++) {
                double h = noise.fractalNoise(
                    col * profile->getFrequency(), 0.0,
                    profile->getOctaves(),
                    profile->getPersistence(),
                    profile->getLacunarity());
                h *= profile->getAmplitude();
                char tt = getTileTypeFromHeight(h);
                int  tr = getTerrainY(h, screen_y, cell_size);
                float sx = (float)(col * cell_size) - cameraX;
                Sprite* top = nullptr;
                switch (tt) {
                case 'g': top = &grassSp;    break;
                case 'w': top = &waterSp;    break;
                case 'm': top = &mountainSp; break;
                default:  top = &grassSp;    break;
                }
                top->setPosition(sx, (float)(tr * cell_size));
                window.draw(*top);
                int total = screen_y / cell_size + 2;
                for (int r = tr + 1; r < total; r++) {
                    dirtSp.setPosition(sx, (float)(r * cell_size));
                    window.draw(dirtSp);
                }
            }
        }

        // DRAW ENTITIES
        entityManager.drawAll(window, cameraX, cameraY);
        for (int i = 0; i < powCount; i++)
            if (pows[i] && (pows[i]->isActive() || pows[i]->isCrateVisible()))
                pows[i]->draw(window, cameraX, cameraY);

        vehicles.draw(window, cameraX, cameraY);
        enemyVehicles.draw(window, cameraX, cameraY);
        //DRAW FIRE POOLS+ EXPLOSIONS 
        for (int i = 0; i < firePoolCount; i++)
            if (firePools[i] && firePools[i]->isActive())
                firePools[i]->draw(window, cameraX, cameraY);
        for (int i = 0; i < explCount; i++)
            if (explosions[i] && explosions[i]->isActive())
                explosions[i]->draw(window, cameraX, cameraY);

        //DRAW PLAYER +GUN
        if (!vehicles.isInVehicle()) {
            active->draw(window, cameraX, cameraY);   // character sprite
            active->drawGun(window, cameraX, cameraY); // gun layered on hand
        }

        //DRAW LASER BEAM
        laserBeam.draw(window, cameraX, cameraY, (float)screen_x);
        //RED DAMAGE OVERLAY
        if (player->getDamageState() != 0) {
            RectangleShape overlay(
                Vector2f((float)screen_x, (float)screen_y));
            Uint8 alpha = (player->getDamageState() == 1) ? 60 : 120;
            overlay.setFillColor(Color(255, 0, 0, alpha));
            window.draw(overlay);
        }
        scoreManager.drawHUD(window);
        if (currentMode == MENU)
            scoreManager.drawLeaderboardScreen(window);

        // GAME OVER screen
        if ((currentMode == SURVIVAL || currentMode == CAMPAIGN)
            && !roster.anyAlive()) {
            // dark overlay
            Sprite overlay;
            Texture overlayTex;
            overlayTex.create(screen_x, screen_y);
            overlay.setTexture(overlayTex);
            overlay.setColor(Color(0, 0, 0, 180));
            window.draw(overlay);
            // GAME OVER screen
            if ((currentMode == SURVIVAL || currentMode == CAMPAIGN)
                && !roster.anyAlive()) {
                Text gameOverText;
                gameOverText.setFont(font);
                gameOverText.setString("GAME OVER - Press ESC");
                gameOverText.setCharacterSize(80);
                gameOverText.setFillColor(Color::Red);
                gameOverText.setPosition(
                    screen_x / 2.0f - 300.0f,
                    screen_y / 2.0f - 40.0f);
                window.draw(gameOverText);
            }

            Text restartText;
            restartText.setFont(font);
            restartText.setString("Press ESC to return to Menu");
            restartText.setCharacterSize(30);
            restartText.setFillColor(Color::White);
            restartText.setPosition(
                screen_x / 2.0f - 180.0f,
                screen_y / 2.0f + 60.0f);
            window.draw(restartText);
        }

        window.display();
    }

    // roster destructor deletes all characters  do not delete player separately
    for (int i = 0; i < MAX_POOLS; i++) delete firePools[i];
    for (int i = 0; i < MAX_EXPL; i++) delete explosions[i];
    for (int i = 0; i < powCount; i++)
        delete pows[i];

    delete profile;

    return 0;
}