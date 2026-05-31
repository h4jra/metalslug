#pragma once
#include <iostream>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
using namespace sf;
using namespace std;

// PERLIN NOISE  

class PerlinNoise {
private:
    int permutation[512];

    double fade(double t) {
        return t * t * t * (t * (t * 6 - 15) + 10);
    }
    double lerp(double t, double a, double b) {
        return a + t * (b - a);
    }
    double grad(int hash, double x, double y) {
        int h = hash & 15;
        double u = h < 8 ? x : y;
        double v = h < 4 ? y : (h == 12 || h == 14 ? x : y);
        return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
    }

public:
    PerlinNoise() {
        static const int p[256] = {
            151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,
            142,8,99,37,240,21,10,23,190,6,148,247,120,234,75,0,26,197,62,94,252,219,
            203,117,35,11,32,57,177,33,88,237,149,56,87,174,20,125,136,171,168,68,175,
            74,165,71,134,139,48,27,166,77,146,158,231,83,111,229,122,60,211,133,230,
            220,105,92,41,55,46,245,40,244,102,143,54,65,25,63,161,1,216,80,73,209,76,
            132,187,208,89,18,169,200,196,135,130,116,188,159,86,164,100,109,198,173,
            186,3,64,52,217,226,250,124,123,5,202,38,147,118,126,255,82,85,212,207,
            206,59,227,47,16,58,17,182,189,28,42,223,183,170,213,119,248,152,2,44,154,
            163,70,221,153,101,155,167,43,172,9,129,22,39,253,19,98,108,110,79,113,224,
            232,178,185,112,104,218,246,97,228,251,34,242,193,238,210,144,12,191,179,
            162,241,81,51,145,235,249,14,239,107,49,192,214,31,181,199,106,157,184,84,
            204,176,115,121,50,45,127,4,150,254,138,236,205,93,222,114,67,29,24,72,243,
            141,128,195,78,66,215,61,156,180
        };
        for (int i = 0; i < 256; ++i) {
            permutation[i] = p[i];
            permutation[i + 256] = p[i];
        }
    }

    double noise(double x, double y) {
        int xi = (int)floor(x) & 255;
        int yi = (int)floor(y) & 255;
        double xf = x - floor(x);
        double yf = y - floor(y);
        double u = fade(xf);
        double v = fade(yf);
        int aa = permutation[permutation[xi] + yi];
        int ab = permutation[permutation[xi] + yi + 1];
        int ba = permutation[permutation[xi + 1] + yi];
        int bb = permutation[permutation[xi + 1] + yi + 1];
        double x1 = lerp(u, grad(aa, xf, yf), grad(ba, xf - 1, yf));
        double x2 = lerp(u, grad(ab, xf, yf - 1), grad(bb, xf - 1, yf - 1));
        return lerp(v, x1, x2);
    }

    double fractalNoise(double x, double y, int octaves,
        double persistence, double lacunarity) {
        double value = 0.0, amplitude = 1.0;
        double frequency = 0.01, maxAmp = 0.0;
        for (int i = 0; i < octaves; ++i) {
            value += noise(x * frequency, y * frequency) * amplitude;
            maxAmp += amplitude;
            amplitude *= persistence;
            frequency *= lacunarity;
        }
        return value / maxAmp;
    }
};

// LEVEL PROFILE

class LevelProfile {
public:
    virtual ~LevelProfile() {}
    virtual double getFrequency() = 0;
    virtual double getAmplitude() = 0;
    virtual int    getOctaves() = 0;
    virtual double getPersistence() = 0;
    virtual double getLacunarity() = 0;
};

class AmplifiedProfile : public LevelProfile {
public:
    double getFrequency() { 
        return 0.005; }
    double getAmplitude() { 
        return 1.5; }
    int    getOctaves() {
        return 6; }
    double getPersistence() { 
        return 0.6; }
    double getLacunarity() {
        return 2.0; }
};

class FlatProfile : public LevelProfile {
public:
    double getFrequency() { 
        return 0.02; }
    double getAmplitude() { 
        return 0.3; }
    int    getOctaves() { 
        return 3; }
    double getPersistence() { 
        return 0.4; }
    double getLacunarity() { 
        return 2.0; }
};

class NormalProfile : public LevelProfile {
public:
    double getFrequency() {
        return 0.01; }
    double getAmplitude() { 
        return 1.0; }
    int    getOctaves() { 
        return 4; }
    double getPersistence() {
        return 0.5; }
    double getLacunarity() {
        return 2.0; }
};

inline LevelProfile* createProfile(const char* name) {
    if (name[0] == 'A') return new AmplifiedProfile();
    if (name[0] == 'F') return new FlatProfile();
    return new NormalProfile();
}

inline char getTileTypeFromHeight(double h) {
    if (h < -0.4) return 'w';
    if (h < 0.2)  return 'g';
    return 'm';
}

inline int getTerrainY(double h, int screenH, int cellSize) {
    double t = (h + 1.0) / 2.0;
    int maxRows = screenH / cellSize;
    return (int)(t * (maxRows - 8)) + 5;
}

// SPRITE ANIMATOR 

class SpriteAnimator {
public:
    Texture texture;
    Sprite  sprite;
    int   totalFrames, frameWidth, frameHeight, currentFrame;
    float animTimer, animSpeed;
    bool  playing, loop;

    SpriteAnimator()
        : totalFrames(1), frameWidth(0), frameHeight(0),
        currentFrame(0), animTimer(0.0f), animSpeed(0.1f),
        playing(false), loop(true) {
    }

    bool load(const char* path, int frames, int fW, int fH) {
        totalFrames = frames; frameWidth = fW; frameHeight = fH;
        if (!texture.loadFromFile(path)) return false;
        sprite.setTexture(texture);
        sprite.setTextureRect(IntRect(0, 0, fW, fH));
        playing = true;
        return true;
    }
    //this one is the overload version of above bool load
    bool load(const char* path, int frames, int fW, int fH, int startFrame) {
        totalFrames = frames; frameWidth = fW; frameHeight = fH;
        if (!texture.loadFromFile(path)) return false;
        sprite.setTexture(texture);
        currentFrame = startFrame;
        sprite.setTextureRect(IntRect(startFrame * fW, 0, fW, fH));
        playing = true;
        return true;
    }
    void setScale(float w, float h) {
        sprite.setScale(w / frameWidth, h / frameHeight);
    }
    void setPosition(float x, float y) { sprite.setPosition(x, y); }
    void setSpeed(float spf) { animSpeed = spf; }
    void play() { playing = true; }
    void pause() { playing = false; }
    void reset() { currentFrame = 0; animTimer = 0.0f; playing = true; }

    void update(float dt) {
        if (!playing) return;
        animTimer += dt;
        if (animTimer >= animSpeed) {
            animTimer = 0.0f;
            currentFrame++;
            if (currentFrame >= totalFrames) {
                if (loop) currentFrame = 0;
                else { currentFrame = totalFrames - 1; playing = false; }
            }
            sprite.setTextureRect(IntRect(
                currentFrame * frameWidth, 0, frameWidth, frameHeight));
        }
    }
    bool isFinished() const {
        return !loop && !playing;
    }
    void draw(RenderWindow& w) { w.draw(sprite); }

};

// CONSTANTS
const int LEVEL_W = 204;
const int LEVEL_H = 40;
const int CELL = 64;

const char T_SKY = 's';
const char T_GRASS = 'g';
const char T_DIRT = 'd';
const char T_WATER = 'w';
const char T_MOUNT = 'm';
const char T_ROCK = 'X';

// ENTITY (Abstract Root fo all the classes)
class Entity {
protected:
    float x, y, w, h;
    bool  active;
public:
    Entity(float x, float y, float w, float h)
        : x(x), y(y), w(w), h(h), active(true) {
    }

    virtual void update(float dt) = 0;
    virtual void draw(RenderWindow& window, float camX, float camY) = 0;

    float getX() const { 
        return x; }
    float getY() const { 
        
        return y; }
    float getW() const { 
        return w; }
    float getH() const { 
        return h; }
    bool  isActive() const { 
        return active; }
    void  setActive(bool a) { 
        active = a; }

    bool collidesWith(Entity* o) const {
        return x < o->x + o->w && x + w > o->x &&
            y < o->y + o->h && y + h > o->y;
    }
    virtual ~Entity() {}
};

// DAMAGABLE ENTITY (Abstract)
class DamagableEntity : public Entity {
protected:
    int hp, maxHp;
    bool dead;

    enum DamageState { NORMAL, INJURED, CRITICAL };
    DamageState damageState;
    float       damageTimer;

public:
    DamagableEntity(float x, float y, float w, float h, int hp)
        : Entity(x, y, w, h), hp(hp), maxHp(hp),
        dead(false), damageState(NORMAL), damageTimer(0) {
    }

    virtual void takeDamage(int amount) {
        if (damageState == NORMAL) { damageState = INJURED;  damageTimer = 1.0f; }
        else if (damageState == INJURED) { damageState = CRITICAL; damageTimer = 1.0f; }
        else { dead = true; active = false; }
        hp -= amount;
        if (hp <= 0) { dead = true; active = false; }
    }

    void updateDamageState(float dt) {
        if (damageState != NORMAL) {
            damageTimer -= dt;
            if (damageTimer <= 0) damageState = NORMAL;
        }
    }

    bool isDead() const { 
        return dead; }
    int  getHp() const { 
        return hp; }
    int  getDamageState() const {
        return damageState; }
};

// TRANSFORMATION STATE (State Pattern  for player)
class PlayerSoldier; // forward declare

class TransformationState {
public:
    virtual void applyEffect(PlayerSoldier* p, float dt) = 0;
    virtual bool isActive() = 0;
    virtual ~TransformationState() {}
};

class NormalTransform : public TransformationState {
public:
    void applyEffect(PlayerSoldier* p, float dt) override {}
    bool isActive() override {
        return false; }
};

class UndeadTransform : public TransformationState {
    float timer;
public:
    UndeadTransform() : timer(10.0f) {}
    void applyEffect(PlayerSoldier* p, float dt) override;
    bool isActive() override {
        return timer > 0; }
};

class MummyTransform : public TransformationState {
    float timer;
public:
    MummyTransform() : timer(10.0f) {}
    void applyEffect(PlayerSoldier* p, float dt) override;
    bool isActive() override {
        return timer > 0; }
};

// WEAPON (Abstract)
class Weapon {
protected:
    int   ammo;
    bool  infiniteAmmo;
    float fireRate;
    float fireTimer;
    int   damage;
public:
    Weapon(int ammo, float fireRate, int damage, bool infinite = false)
        : ammo(ammo), infiniteAmmo(infinite),
        fireRate(fireRate), fireTimer(0), damage(damage) {
    }

    virtual void fire(float x, float y, bool facingRight) = 0;
    virtual void fireDirected(float x, float y, float vx, float vy) {
        fire(x, y, vx >= 0);
    }
    virtual void update(float dt) {
        if (fireTimer > 0)
            fireTimer -= dt;
    }

    bool canFire()   const {
        
        return fireTimer <= 0 && (infiniteAmmo || ammo > 0); }
    int  getAmmo()   const { 
        return ammo; }
    int  getDamage() const { 
        return damage; }
    bool isEmpty()   const { 
        return !infiniteAmmo && ammo <= 0; }
    void addAmmo(int a) { ammo += a; }
    virtual ~Weapon() {}
};

// PROJECTILE (Abstract)
class Projectile : public Entity {
protected:
    float velX, velY;
    int   damage;
    bool  fromPlayer;
public:
    Projectile(float x, float y, float vx, float vy,
        int dmg, bool fromPlayer)
        : Entity(x, y, 10, 6), velX(vx), velY(vy),
        damage(dmg), fromPlayer(fromPlayer) {
    }

    bool isFromPlayer() const { 
        return fromPlayer; }
    int  getDamage()    const { 
        return damage; }

    virtual bool isFlame()     const {
        return false; }
    virtual bool isBallistic() const { 
        return false; }

    virtual ~Projectile() {}
};

class StraightProjectile : public Projectile {
    Sprite  spr;
    Texture tex;
    bool    texLoaded;
public:
    StraightProjectile(float x, float y, float vx, float vy,
        int dmg, bool fromPlayer,
        const char* texturePath = "25I-0832_25I-0711_Assets/bullet.png")
        : Projectile(x, y, vx, vy, dmg, fromPlayer),
        texLoaded(false)
    {
        texLoaded = tex.loadFromFile(texturePath);
        if (texLoaded) {
            spr.setTexture(tex);
            // scale sprite to a reasonable bullet size
            float tw = (float)tex.getSize().x;
            float th = (float)tex.getSize().y;
            spr.setScale(20.0f / tw, 10.0f / th);
            // flip horizontally if going left
            if (vx < 0) {
                spr.setScale(-20.0f / tw, 10.0f / th);

            }
        }
        // update the hitbox size to match
        w = 20; h = 10;
    }

    void update(float dt) override {
        x += velX;
        y += velY;
        // deactivate when off level bounds
        if (x < -100 || x > LEVEL_W * CELL + 100 || y < -200 || y > LEVEL_H * CELL + 100)
            active = false;
    }

    void draw(RenderWindow& win, float cx, float cy) override {
        if (velX >= 0)
            spr.setPosition(x - cx, y - cy);
        else
            spr.setPosition((x - cx) + 32.0f, y - cy);        win.draw(spr);
    }
};
class BallisticProjectile : public Projectile {
    float   grav;
    Sprite  spr;
    Texture tex;
    bool    texLoaded;
public:
    // texturePath 
    BallisticProjectile(float x, float y, float vx, float vy,
        int dmg, bool fromPlayer,
        float grav = 0.3f,
        const char* texturePath = "25I-0832_25I-0711_Assets/grenade.png")
        : Projectile(x, y, vx, vy, dmg, fromPlayer),
        grav(grav), texLoaded(false)
    {
        texLoaded = tex.loadFromFile(texturePath);
        if (texLoaded) {
            spr.setTexture(tex);
            float tw = (float)tex.getSize().x;
            float th = (float)tex.getSize().y;
            // grenade = 24x24 and th e rocket = 40x20
            spr.setScale(32.0f / tw, 32.0f / th);
        }
        w = 32; h = 32;
    }

    void update(float dt) override {
        x += velX;
        velY += grav;
        y += velY;
        // rotate sprite to follow arc direction
        // NEW - flip sprite based on horizontal direction
        if (texLoaded) {
            float tw = (float)tex.getSize().x;
            float th = (float)tex.getSize().y;
            if (velX < 0)
                spr.setScale(-32.0f / tw, 32.0f / th);
            else
                spr.setScale(32.0f / tw, 32.0f / th);
        }
        if (x < -100 || x > LEVEL_W * CELL + 100 ||
            y > LEVEL_H * CELL + 100) active = false;
    }

    void draw(RenderWindow& win, float cx, float cy) override {
        spr.setPosition(x - cx, y - cy);
        win.draw(spr);
    }
};

// PISTOL WEAPON (Concrete)
class Pistol : public Weapon {
public:
    Projectile** projectiles;
    int* projectileCount;
    int maxProjectiles;

    Pistol() :Weapon(-1, 4.0f, 3, true), projectiles(nullptr), projectileCount(nullptr), maxProjectiles(0) {
    }

    void setProjectilePool(Projectile** pool, int* count, int max) {
        projectiles = pool;
        projectileCount = count;
        maxProjectiles = max;
    }

    void fire(float x, float y, bool facingRight) override {
        if (!canFire()) return;
        if (projectiles && projectileCount &&
            *projectileCount < maxProjectiles) {
            float vx = facingRight ? 12.0f : -12.0f;
            projectiles[(*projectileCount)++] =
                new StraightProjectile(x, y, vx, 0, damage, true,
                    "25I-0832_25I-0711_Assets/bullet.png");
        }
        fireTimer = 1.0f / fireRate;
    }

    void fireDirected(float x, float y, float vx, float vy) override {
        if (!canFire())
            return;
        if (projectiles && projectileCount && *projectileCount < maxProjectiles)
            projectiles[(*projectileCount)++] = new StraightProjectile(x, y, vx, vy, damage, true, "25I-0832_25I-0711_Assets/bullet.png");
        fireTimer = 1.0f / fireRate;
    }

};

// SOLDIER (Abstract)
class Soldier : public DamagableEntity {
protected:
    Weapon* currentWeapon;
    int     grenades;
    float   speed;
    bool    facingRight;
public:
    float getSpeed() const { return speed; }

    Soldier(float x, float y, float w, float h,
        int hp, float speed, int grenades)
        : DamagableEntity(x, y, w, h, hp),
        currentWeapon(nullptr), grenades(grenades),
        speed(speed), facingRight(true) {
    }

    void    equipWeapon(Weapon* wep) { 
        delete currentWeapon; currentWeapon = wep; }
    Weapon* getWeapon()     const { 
        return currentWeapon; }
    int     getGrenades()   const { 
        return grenades; }
    bool    isFacingRight() const {
        return facingRight; }
    void    setFacingRight(bool f) {
        facingRight = f; }

    virtual void throwGrenade(float tx, float ty) = 0;
    virtual ~Soldier() { delete currentWeapon; }
};

// ENEMY AI STATES (State Pattern)
class Enemy; // forward declare

class EnemyAIState {
public:
    virtual void update(Enemy* e, float px, float py, float dt) = 0;
    virtual ~EnemyAIState() {}
};

class StationaryAI : public EnemyAIState {
public:
    void update(Enemy* e, float px, float py, float dt) override {
        // enemy does not move at all
        // just faces the player direction
    }
};

class ChaseAI : public EnemyAIState {
public:
    void update(Enemy* e, float px, float py, float dt) override;
};

//HORIZONTAL AI  moves left and right only
//Used by FlyingTara
class HorizontalAI : public EnemyAIState {
    float dir;
public:
    HorizontalAI() : dir(1.0f) {}
    void update(Enemy* e, float px, float py, float dt) override;
};

// ENEMY (Abstract)
class Enemy : public Soldier {
protected:
    EnemyAIState* aiState;
    int           scoreValue;
    float         detectionRange;
    bool          onScreen;    // true only when visible in camera
    float         shootTimer;  // countdown between shots

public:
    Enemy(float x, float y, float w, float h,
        int hp, float spd, int score, float range = 400)
        : Soldier(x, y, w, h, hp, spd, 0),
        aiState(new StationaryAI()),
        scoreValue(score), detectionRange(range),
        onScreen(false), shootTimer(1.5f),
        velY(0.0f) {  
    }

    // called every frame from main to tell enemy if it is visible
    void setOnScreen(bool val) {
        onScreen = val; }
    bool isOnScreen()    const { 
        return onScreen; }

    int   getScoreValue()     const {
        return scoreValue; }
    float getDetectionRange() const { 
        return detectionRange; }
    float getShootTimer()     const {
        return shootTimer; }

    void setAIState(EnemyAIState* s) { delete aiState; aiState = s; }

    void moveX(float dx) { 
        x += dx; }
    void moveY(float dy) {
        y += dy; }

    // shoots a straight bullet toward the player's position
    // only called from main when enemy isOnScreen
    void shootAt(float px, float py,
        Projectile** pool, int* count, int max) {
        shootTimer -= 0.016f; 
        if (shootTimer > 0) 
            return;
        shootTimer = 1.5f;    // reset an d shoots every 1.5 seconds
        if (*count >= max) 
            return;
        float vx = (px > x) ? 8.0f : -8.0f;
        setFacingRight(px > x);
        pool[(*count)++] = new StraightProjectile(
            x + w / 2, y + h / 2, vx, 0, 3, false, "25I-0832_25I-0711_Assets/bullet.png");   // gun soldier bullet
    }

    void update(float dt) override {
        updateDamageState(dt);
    }

    // AI only runs when enemy is on screen
    virtual void updateAI(float px, float py, float dt, float camX = 0, float camY = 0) {
        if (onScreen && aiState)
            aiState->update(this, px, py, dt);
    }

    void throwGrenade(float tx, float ty) override {}
    float velY = 0.0f;
    float getVelY() const { 
        return velY; }
    void  setVelY(float vy) {
        velY = vy; }
    void  setPos(float nx, float ny) {
        x = nx; 
        y = ny; }
    virtual void fireSpecial(float px, float py,
        Projectile** pool, int* count, int max) {
    }
    virtual bool isAerial() const {
        return false; }
    virtual bool isMelee()  const { 
        return false; }
    virtual bool isFlying() const { 
        return false; }

    virtual ~Enemy() { delete aiState; }
};


// ChaseAI from  walks directly toward player 
//we will reuse all these ai named functions in the self playing mode in game 
inline void ChaseAI::update(Enemy* e, float px, float py, float dt) {
    float dx = px - e->getX();
    if (abs(dx) < 40.0f) {
        e->setFacingRight(dx > 0);
        return;
    }
    float d = (dx > 0) ? 1.0f : -1.0f;
    e->setFacingRight(d > 0);
    e->moveX(d * e->getSpeed() * dt * 60);
}
// HorizontalAI moves left/right, bounces off level edges (FlyingTara)
inline void HorizontalAI::update(Enemy* e, float px, float py, float dt) {
    e->moveX(dir * e->getSpeed() * dt * 60);
    // bounce when hitting level boundaries
    if (e->getX() < 0 || e->getX() > LEVEL_W * CELL)
        dir *= -1.0f;
    e->setFacingRight(dir > 0);
}

class RebelSoldier : public Enemy {
    SpriteAnimator anim;
public:
    RebelSoldier(float x, float y)
        : Enemy(x, y, 60, 80, 2, 0.0f, 50) {
        if (!anim.load("25I-0832_25I-0711_Assets/Enemies/gun soldier.png", 8, 503, 481))
            cout << "ERROR loading: gun soldier.png" << endl;

        anim.setSpeed(0.2f);
    }

    void update(float dt) override {
        Enemy::update(dt);
        anim.update(dt);
    }
    void fireSpecial(float px, float py,
        Projectile** pool, int* count, int max) override {
        shootAt(px, py, pool, count, max);
    }
    void draw(RenderWindow& win, float cx, float cy) override {
        float sx = 60.0f / 503.0f;
        float sy = 80.0f / 481.0f;
        anim.sprite.setScale(facingRight ? -sx : sx, sy);
        if (facingRight)
            anim.setPosition(x - cx, y - cy);
        else
            anim.setPosition((x - cx) + 60.0f, y - cy);
        anim.draw(win);
    }
};

//stays put, shoots when on screen
class ShieldedSoldier : public Enemy {
    SpriteAnimator anim;
    bool aboveAttack;
public:
    ShieldedSoldier(float x, float y)
        : Enemy(x, y, 60, 80, 5, 0.0f, 75), aboveAttack(false) {
        if (!anim.load("25I-0832_25I-0711_Assets/Enemies/shielded soldier.png", 10, 463, 436))
            cout << "ERROR loading: shielded soldier.png" << endl;
        anim.setSpeed(0.12f);
    }
    void setAboveAttack(bool a) { aboveAttack = a; }
    void update(float dt) override {
        Enemy::update(dt);
        int startFrame = aboveAttack ? 7 : 0;
        int endFrame = aboveAttack ? 9 : 6;
        if (anim.currentFrame < startFrame || anim.currentFrame > endFrame)
            anim.currentFrame = startFrame;
        anim.animTimer += 0.016f;
        if (anim.animTimer >= anim.animSpeed) {
            anim.animTimer = 0.0f;
            anim.currentFrame++;
            if (anim.currentFrame > endFrame) anim.currentFrame = startFrame;
        }
        anim.sprite.setTextureRect(IntRect(anim.currentFrame * 450, 0, 450, 370));
    }
    void fireSpecial(float px, float py,
        Projectile** pool, int* count, int max) override {
        shootAt(px, py, pool, count, max);
    }
    void draw(RenderWindow& win, float cx, float cy) override {
        float sx = 60.0f / 463.0f;
        float sy = 80.0f / 436.0f;
        anim.sprite.setScale(facingRight ? -sx : sx, sy);
        if (facingRight)
            anim.setPosition(x - cx, y - cy);
        else
            anim.setPosition((x - cx) + 60.0f, y - cy);
        anim.draw(win);
    }
};

class BazookaSoldier : public Enemy {
    SpriteAnimator anim;
    float rocketTimer;
public:
    BazookaSoldier(float x, float y)
        : Enemy(x, y, 60, 80, 2, 0.0f, 100), rocketTimer(3.0f) {
        if (!anim.load("25I-0832_25I-0711_Assets/Enemies/rocket soldier.png", 5, 510, 543))
            cout << "ERROR loading: rocket soldier.png" << endl;
        anim.setSpeed(0.15f);
    }
    void fireRocket(float px, float py,
        Projectile** pool, int* count, int max) {
        rocketTimer -= 0.016f;
        if (rocketTimer > 0) return;
        rocketTimer = 3.0f;
        if (*count >= max) return;
        float dx = px - x;
        float dy = py - y;
        float dist = sqrt(dx * dx + dy * dy);
        float vx = (dx / dist) * 6.0f;
        float vy = -8.0f;
        setFacingRight(px > x);
        pool[(*count)++] = new BallisticProjectile(
            x + w / 2, y, vx, vy, 5, false, 0.25f,
            "25I-0832_25I-0711_Assets/rocket.png");   // rocket soldier rocket
    }
    void update(float dt) override {
        Enemy::update(dt);
        anim.update(dt);
    }
    void fireSpecial(float px, float py, Projectile** pool, int* count, int max) override {
        fireRocket(px, py, pool, count, max);
    }
    void draw(RenderWindow& win, float cx, float cy) override {
        float sx = 60.0f / 510.0f;
        float sy = 80.0f / 543.0f;
        anim.sprite.setScale(facingRight ? -sx : sx, sy);
        if (facingRight)
            anim.setPosition(x - cx, y - cy);
        else
            anim.setPosition((x - cx) + 60.0f, y - cy);
        anim.draw(win);
    }
};
class GrenadeSoldier : public Enemy {
    SpriteAnimator anim;
    float grenadeTimer;
public:
    GrenadeSoldier(float x, float y)
        : Enemy(x, y, 60, 80, 2, 0.0f, 100), grenadeTimer(2.5f) {
        if (!anim.load("25I-0832_25I-0711_Assets/Enemies/granade soldier.png", 4, 503, 466))
            cout << "ERROR loading: grenade soldier.png" << endl;
        anim.setSpeed(0.2f);
    }
    void throwGrenadeAt(float px, float py,
        Projectile** pool, int* count, int max) {
        grenadeTimer -= 0.016f;
        if (grenadeTimer > 0) return;
        grenadeTimer = 2.5f;
        if (*count >= max) return;
        float dx = px - x;
        float vx = (dx > 0) ? 5.0f : -5.0f;
        float vy = -10.0f;
        setFacingRight(px > x);
        pool[(*count)++] = new BallisticProjectile(
            x + w / 2, y, vx, vy, 20, false, 0.3f,
            "25I-0832_25I-0711_Assets/grenade.png");  // grenade soldier grenade
    }
    void fireSpecial(float px, float py, Projectile** pool, int* count, int max) override {
        throwGrenadeAt(px, py, pool, count, max);
    }
    void throwGrenade(float tx, float ty) override {}
    void update(float dt) override {
        Enemy::update(dt);
        anim.update(dt);
    }
    void draw(RenderWindow& win, float cx, float cy) override {
        float sx = 60.0f / 503.0f;
        float sy = 80.0f / 466.0f;
        anim.sprite.setScale(facingRight ? -sx : sx, sy);
        if (facingRight)
            anim.setPosition(x - cx, y - cy);
        else
            anim.setPosition((x - cx) + 60.0f, y - cy);
        anim.draw(win);
    }
};
class Zombie : public Enemy {
    SpriteAnimator anim;
public:
    Zombie(float x, float y)
        : Enemy(x, y, 60, 80, 5, 1.0f, 100) {
        if (!anim.load("25I-0832_25I-0711_Assets/Enemies/zombie.png", 9, 434, 473))
            cout << "ERROR loading: zombie.png" << endl;
        anim.setSpeed(0.12f);
        setAIState(new ChaseAI());
    }
    void update(float dt) override {
        Enemy::update(dt);
        anim.update(dt);
    }
    bool isMelee() const override { 
        return true; 
    }
    void draw(RenderWindow& win, float cx, float cy) override {
        float sx = 60.0f / 434.0f;
        float sy = 80.0f / 473.0f;
        anim.sprite.setScale(facingRight ? -sx : sx, sy);
        if (facingRight)
            anim.setPosition(x - cx, y - cy);
        else
            anim.setPosition((x - cx) + 60.0f, y - cy);
        anim.draw(win);
    }
};
class MummyWarrior : public Enemy {
    SpriteAnimator anim;
public:
    MummyWarrior(float x, float y)
        : Enemy(x, y, 60, 70, 5, 0.6f, 150) {
        if (!anim.load("25I-0832_25I-0711_Assets/Enemies/mummy.png", 9, 404, 574))
            cout << "ERROR loading: mummy.png" << endl;
        anim.setSpeed(0.14f);
        setAIState(new ChaseAI());
    }
    void update(float dt) override {
        Enemy::update(dt);
        anim.update(dt);
    }
    bool isMelee() const override { return true; }
    void draw(RenderWindow& win, float cx, float cy) override {
        float sx = 60.0f / 404.0f;
        float sy = 70.0f / 574.0f;
        anim.sprite.setScale(facingRight ? -sx : sx, sy);
        if (facingRight)
            anim.setPosition(x - cx, y - cy);
        else
            anim.setPosition((x - cx) + 60.0f, y - cy);
        anim.draw(win);
    }
};
// MARTIAN  2 phase enemy
//phase 1 flying pod, hovers above player, fires energy beam downward
//aand in phase 2 pod destroyed, drops to ground, shoots pistol
class Martian : public Enemy {

    //sprite sheets
    SpriteAnimator flyIdle;    
    SpriteAnimator groundIdle; 
    SpriteAnimator groundShot;
    bool beamJustFired;
    bool  beamActive;
    //phase
    enum MartianPhase { PHASE_FLYING, PHASE_GROUND };
    MartianPhase phase;

    // pod HP (separate from enemy HP used in phase 2) 
    int podHp;

    //horizontal movement (phase 1) 
    float moveDir;    // +1 = right, -1 = left
    float moveSpeed;

    // beam / shoot timers 
    float beamTimer;      
    float groundShootTimer;
    bool  isShooting;   
    float shootAnimTimer; 

    bool  landed;         
    float groundY;        

    float beamLifetime;
    float beamX, beamY, beamH; 

public:
    Martian(float x, float y)
        : Enemy(x, y, 80, 100, 3, 2.5f, 200), beamJustFired(false),
        phase(PHASE_FLYING),
        podHp(3),
        moveDir(1.0f),
        moveSpeed(2.5f),
        beamTimer(2.5f),
        groundShootTimer(1.8f),
        isShooting(false),
        shootAnimTimer(0.0f),
        landed(false),
        groundY(0.0f),
        beamActive(false),
        beamLifetime(0.0f),
        beamX(0), beamY(0), beamH(0)
    {
        if (!flyIdle.load("25I-0832_25I-0711_Assets/Enemies/martianPOD.png", 8, 587, 520))
            cout << "ERROR loading: martian_pod.png" << endl;
        flyIdle.setSpeed(0.08f);

        if (!groundIdle.load("25I-0832_25I-0711_Assets/Enemies/martian.png", 8, 678, 520))
            cout << "ERROR loading: martian_ground.png" << endl;
        groundIdle.setSpeed(0.12f);

        if (!groundShot.load("25I-0832_25I-0711_Assets/Enemies/martian.png", 3, 678, 520))
            cout << "ERROR loading: martian.png" << endl;
        groundShot.setSpeed(0.1f);
        groundShot.loop = false; // plays once per shot
    }
    bool isAerial() const override { 
        return true; }
    bool isFlying() const override { 
        return phase == PHASE_FLYING; }
    void fireSpecial(float px, float py, Projectile** pool, int* count, int max) override {
        fireBeam(pool, count, max);
        fireGroundBullet(pool, count, max);
    }
    // called from main just like other enemies
    void update(float dt) override {
        Enemy::update(dt); // damage flash timer

        if (phase == PHASE_FLYING) {
            flyIdle.update(dt);

            // beam lifetime countdown
            if (beamActive) {
                beamLifetime -= dt;
                if (beamLifetime <= 0) beamActive = false;
            }
        }
        else { // PHASE GROUND
            if (isShooting) {
                groundShot.update(dt);
                shootAnimTimer -= dt;
                if (shootAnimTimer <= 0) {
                    isShooting = false;
                    groundShot.reset();
                    groundShot.pause();
                }
            }
            else {
                groundIdle.update(dt);
            }
        }
    }

    // ai one iscalled from main when onScreen
    void updateAI(float px, float py, float dt, float camX = 0, float camY = 0) {
        if (phase == PHASE_FLYING) {
            // bounce within current screen width only
            float screenLeft = camX + 20.0f;
            float screenRight = camX + 1600.0f - w - 20.0f;

            x += moveDir * moveSpeed * dt * 60.0f;

            if (x <= screenLeft) {
                x = screenLeft; 
                moveDir = 1.0f; }
            if (x >= screenRight) { 
                x = screenRight;
                moveDir = -1.0f; }

            setFacingRight(moveDir > 0);

            // fire beam straight down when horizontally above player
            float centerX = x + w / 2.0f;
            float playerCX = px + 45.0f;
            if (fabsf(centerX - playerCX) < 80.0f) {
                beamTimer -= dt;
                if (beamTimer <= 0.0f) {
                    beamTimer = 2.5f;
                    beamActive = true;
                    beamJustFired = false;
                    beamLifetime = 0.35f;
                    beamX = centerX - 6.0f;
                    beamY = y + h;
                    beamH = py - beamY;
                    if (beamH < 40.0f) beamH = 40.0f;
                }
            }
        }
        else { // PHASE GROUND
            // walk toward player on ground
            float dir = (px > x) ? 1.0f : -1.0f;
            x += dir * moveSpeed * dt * 60.0f;
            setFacingRight(dir > 0);

            // stop walking when close enough to shoot
            float distToPlayer = fabsf(px - x);
            if (distToPlayer < 200.0f) {
                // close enough: stop and shoot
                groundShootTimer -= dt;
                if (groundShootTimer <= 0.0f) {
                    groundShootTimer = 1.8f;
                    isShooting = true;
                    shootAnimTimer = 0.3f;
                    groundShot.reset();
                    groundShot.play();
                }
            }
        }
    }

    // pod takes damage in phase 1  body takes damage in phase 2
    // called from main collision loop exactly like other enemies
    void takeDamage(int amount, char map[][LEVEL_W]) {
        if (phase == PHASE_FLYING) {
            podHp -= amount;
            if (podHp <= 0) {
                phase = PHASE_GROUND;
                landed = false;
                groundY = 0.0f;

                int col = (int)((x + w / 2.0f) / CELL);
                col = col < 0 ? 0 : (col >= LEVEL_W ? LEVEL_W - 1 : col);

                for (int row = (int)(y / CELL); row < LEVEL_H; row++) {
                    char tile = map[row][col];
                    if (tile != T_SKY && tile != T_WATER) {
                        groundY = (float)(row * CELL) - h;
                        landed = true;
                        break;
                    }
                }
                if (!landed) groundY = (float)(LEVEL_H - 2) * CELL - h;
                y = groundY;
                groundShootTimer = 1.8f;
                isShooting = false;
            }
        }
        else {
            Enemy::takeDamage(amount);
        }
    }
    // shoot bullet in ground phase  called from main
    void fireGroundBullet(Projectile** pool, int* count, int max) {
        if (phase != PHASE_GROUND) 
            return;
        if (!isShooting)           
            return;
        if (*count >= max)       
            return;
        // fire once per shoot cycle
        if (shootAnimTimer > 0.25f) { // fire at start of animation
            float vx = facingRight ? 9.0f : -9.0f;
pool[(*count)++] = new StraightProjectile(facingRight ? x + w : x,y + h * 0.4f,vx, 0, 3, false,"25I-0832_25I-0711_Assets/bullet.png");
        }
    }

    // returns true if beam is active AND player is inside it
    bool beamHitsPlayer(float px, float py, float pw, float ph) const {
        if (!beamActive || phase != PHASE_FLYING)
            return false;
        // beam is a vertical strip at beamX
        bool xOverlap = (px < beamX + 12.0f) && (px + pw > beamX);
        bool yOverlap = (py < beamY + beamH) && (py + ph > beamY);
        return xOverlap && yOverlap;
    }
    // fire energy beam downward 
    void fireBeam(Projectile** pool, int* count, int max) {
        if (phase != PHASE_FLYING) 
            return;
        if (!beamActive)        
            return;
        if (*count >= max)     
            return;
        if (!beamJustFired) {          // only fire ONCE per beam activation
            beamJustFired = true;
            pool[(*count)++] = new StraightProjectile(x + w / 2.0f,y + h,0, 12.0f,5, false,"25I-0832_25I-0711_Assets/energyBeam.png");
        }
    }

    // DRAW
    void draw(RenderWindow& win, float cx, float cy) override {
        if (phase == PHASE_FLYING) {
            float sx = 80.0f / 620.0f;
            float sy = 100.0f / 520.0f;
            flyIdle.sprite.setScale(facingRight ? -sx : sx, sy);
            if (facingRight)
                flyIdle.setPosition(x - cx, y - cy);
            else
                flyIdle.setPosition((x - cx) + 80.0f, y - cy);
            flyIdle.setPosition(x - cx, y - cy);
            flyIdle.draw(win);

            // draw energy beam
            if (beamActive) {
                RectangleShape beam(Vector2f(12.0f, beamH));
                beam.setFillColor(Color(0, 255, 220, 180));
                beam.setPosition(beamX - cx, beamY - cy);
                win.draw(beam);
            }
        }
        else { // PHASE GROUND
            float sx = 80.0f / 678.0f;
            float sy = 100.0f / 520.0f;
            if (isShooting) {
                groundShot.sprite.setScale(facingRight ? -sx : sx, sy);

                if (facingRight)
                    groundShot.setPosition(x - cx, y - cy);
                else
                    groundShot.setPosition((x - cx) + 80.0f, y - cy);            
                groundShot.setPosition(x - cx, y - cy);
                groundShot.draw(win);
            }
            else {
                groundIdle.sprite.setScale(facingRight ? -sx : sx, sy);

                if (facingRight)
                    groundIdle.setPosition(x - cx, y - cy);
                else
                    groundIdle.setPosition((x - cx) + 80.0f, y - cy);           
                groundIdle.setPosition(x - cx, y - cy);
                groundIdle.draw(win);
            }
        }
    }
};

//paratrouper  random infantry type descends from above
// parachute shows while descending, landing anim plays once
// then parachute disappears and soldier fights normally
class Paratrooper : public Enemy {

    SpriteAnimator soldierAnim;
    SpriteAnimator chuteDescend;
    SpriteAnimator chuteLand;    

    enum ParaState { DESCENDING, LANDING, LANDED };
    ParaState paraState;

    float descentSpeed;
    float groundY;
    bool  chuteGone;

    // 0=rebel  1=shielded  2=bazooka  3=grenade
    int soldierType;

    float findGroundY(char map[][LEVEL_W], float spawnX, float spawnY) {
        int col = (int)(spawnX / CELL);
        col = col < 0 ? 0 : (col >= LEVEL_W ? LEVEL_W - 1 : col);
        int startRow = (int)(spawnY / CELL);
        for (int row = startRow; row < LEVEL_H; row++) {
            char tile = map[row][col];
            if (tile != T_SKY && tile != T_WATER) {
                return (float)(row * CELL) - h;
            }
        }
        return (float)(LEVEL_H - 2) * CELL - h;
    }

public:
    // map passed in so we can find real ground Y automatically
    Paratrooper(float x, float startY, char map[][LEVEL_W], int type = -1)
        : Enemy(x, startY, 70, 90, 2, 2.0f, 75),
        paraState(DESCENDING),
        descentSpeed(120.0f),
        groundY(0.0f),
        chuteGone(false),
        soldierType(type)
    {
        // pick random type if not specified
        if (soldierType < 0 || soldierType > 3)
            soldierType = rand() % 4;

        const char* soldierPath;
        int frames, fw, fh;

        if (soldierType == 0) {
            soldierPath = "25I-0832_25I-0711_Assets/Enemies/gun soldier.png";
            frames = 8; fw = 503; fh = 481;
            soldierAnim.setSpeed(0.2f);
        }
        else if (soldierType == 1) {
            soldierPath = "25I-0832_25I-0711_Assets/Enemies/shielded soldier.png";
            frames = 10; fw = 463; fh = 436;
        }
        else if (soldierType == 2) {
            soldierPath = "25I-0832_25I-0711_Assets/Enemies/rocket soldier.png";
            frames = 5; fw = 510; fh = 543;
        }
        else {
            soldierPath = "25I-0832_25I-0711_Assets/Enemies/granade soldier.png";
            frames = 4; fw = 503; fh = 466;
        }

        if (!soldierAnim.load(soldierPath, frames, fw, fh))
            cout << "ERROR: soldier sprite not found: " << soldierPath << "\n";
        soldierAnim.setSpeed(0.12f);

        if (!chuteDescend.load("25I-0832_25I-0711_Assets/Enemies/parashoot.png", 4, 450, 632))
            cout << "ERROR: parachute.png not found\n";
        chuteDescend.setSpeed(0.12f);
        chuteDescend.loop = true;

        if (!chuteLand.load("25I-0832_25I-0711_Assets/Enemies/parashoot.png", 5, 450, 632, 4))
            cout << "ERROR: parachute.png not found\n";
        chuteLand.setSpeed(0.10f);
        chuteLand.loop = false;

        // find real ground Y from levelMap
        groundY = findGroundY(map, x, startY);
    }
    bool isAerial() const override { return true; }
    void fireSpecial(float px, float py, Projectile** pool, int* count, int max) override {
        if (isLanded()) shootAt(px, py, pool, count, max);
    }
    void update(float dt) override {
        Enemy::update(dt);
        soldierAnim.update(dt);

        if (paraState == DESCENDING)
            chuteDescend.update(dt);
        else if (paraState == LANDING) {
            chuteLand.update(dt);
            if (chuteLand.isFinished()) {
                chuteGone = true;
                paraState = LANDED;
            }
        }
    }

    virtual void updateAI(float px, float py, float dt,
        float camX = 0, float camY = 0) override {
        if (paraState == DESCENDING) {
            y += descentSpeed * dt;
            setFacingRight(px > x);

            if (y >= groundY) {
                y = groundY; // snap exactly to ground
                paraState = LANDING;
                chuteLand.reset();
                chuteLand.play();
            }
        }
        else {
            setFacingRight(px > x);
        }
    }

    bool isDescending() const { 
        return paraState == DESCENDING; }
    bool isLanded()     const { 
        return paraState == LANDED; }
    int  getType()      const { 
        return soldierType; }

    void draw(RenderWindow& win, float cx, float cy) override {
        float drawX = x - cx;
        float drawY = y - cy;

        float ssx = 70.0f / 330.0f;
        float ssy = 90.0f / 510.0f;
        float psx = 100.0f / 450.0f;
        float psy = 90.0f / 632.0f;

        // draw parachute above soldier
        if (!chuteGone) {
            if (paraState == DESCENDING) {
                chuteDescend.sprite.setScale(psx, psy);
                chuteDescend.setPosition(drawX - 15.0f, drawY - 90.0f);
                chuteDescend.draw(win);
            }
            else if (paraState == LANDING) {
                chuteLand.sprite.setScale(psx, psy);
                chuteLand.setPosition(drawX - 15.0f, drawY - 90.0f);
                chuteLand.draw(win);
            }
        }

        // soldier always drawn
        drawX = x - cx;
        drawY = y - cy;
        soldierAnim.sprite.setScale(facingRight ? ssx : -ssx, ssy);
        if (facingRight)
            soldierAnim.setPosition(drawX, drawY);
        else
            soldierAnim.setPosition(drawX + 70.0f, drawY);

        soldierAnim.draw(win);
    }
};

class FlyingTara : public Enemy {
    SpriteAnimator anim;
    float          missileTimer;
    float          displayW, displayH;

public:
    FlyingTara(float x, float y)
        : Enemy(x, y, 80, 60, 3, 2.0f, 150),
        missileTimer(2.0f), displayW(96.0f), displayH(72.0f)
    {
        if (!anim.load("25I-0832_25I-0711_Assets/tara.png", 1, 1759, 896))
            cout << "ERROR loading: tara.png" << endl;
        anim.setSpeed(0.12f);
    }

    void update(float dt) override {
        Enemy::update(dt);
        anim.update(dt);
    }
    bool isAerial() const override { 
        return true; }
    void fireSpecial(float px, float py, Projectile** pool, int* count, int max) override {
        fireMissile(px, py, pool, count, max);
    }
    void updateAI(float px, float py, float dt, float camX = 0, float camY = 0) override {
        if (!onScreen) 
            return;
        float dx = px - x;
        float dy = py - y;
        float dist = sqrtf(dx * dx + dy * dy);
        if (dist > 150.0f) {
            float speed = getSpeed() * dt * 60.0f;
            x += (dx / dist) * speed;
            y += (dy / dist) * speed;
        }
        setFacingRight(px > x);
        missileTimer -= dt;
    }

    void fireMissile(float px, float py,
        Projectile** pool, int* count, int max) {
        if (!onScreen)     
            return;
        if (missileTimer > 0)
            return;
        if (*count >= max)   
            return;
        missileTimer = 2.0f;
        float dx = px - x;
        float dy = py - y;
        float dist = sqrtf(dx * dx + dy * dy);
        if (dist == 0) return;
        float speed = 7.0f;
        float vx = (dx / dist) * speed;
        float vy = (dy / dist) * speed;
        pool[(*count)++] = new StraightProjectile(
            x + w / 2.0f, y + h / 2.0f,
            vx, vy, 3, false,
            "25I-0832_25I-0711_Assets/missile.png");
    }

    void draw(RenderWindow& win, float cx, float cy) override {
        if (!active)
            return;
        float sx = displayW / 1759.0f;
        float sy = displayH / 896.0f;
        anim.sprite.setScale(facingRight ? -sx : sx, sy);
        float drawX = facingRight ? (x - cx) : (x - cx) + displayW;
        anim.setPosition(drawX, y - cy);
        anim.draw(win);
    }
};
// COLLECTIBLE (Abstract)
class Collectible : public Entity {
protected:
    bool collected;
public:
    Collectible(float x, float y, float w, float h): Entity(x, y, w, h), collected(false) {
    }
    virtual void onCollect(PlayerSoldier* p) = 0;
    bool isCollected() const {
        return collected; }
};

// INTERACTABLE OBJECT (Abstract)
class InteractableObject : public Entity {
public:
    InteractableObject(float x, float y, float w, float h)
        : Entity(x, y, w, h) {
    }
    virtual void onInteract(PlayerSoldier* p) = 0;
};

class POWPrisoner : public InteractableObject {
    SpriteAnimator anim;
    bool released;
    bool falling;
    bool done;
    float fallSpeed;
    float doneTimer;   // small delay after anim finishes before disappearing
    Texture crateTexture;
    Sprite  crateSprite;
    bool    crateVisible;
public:
    POWPrisoner(float x, float y): InteractableObject(x, y, 60, 80),released(false), falling(false), done(false),fallSpeed(0.5f), doneTimer(0.4f)
    {
        if (!anim.load("25I-0832_25I-0711_Assets/prisoner.png", 4, 707, 586, 0))
            cout << "ERROR loading: prisoner.png" << endl;
        anim.setScale(60.0f, 80.0f);
        anim.loop = false;
        anim.pause();
        crateVisible = false;
        if (!crateTexture.loadFromFile("25I-0832_25I-0711_Assets/supplycrate.png"))
            cout << "ERROR loading: supplycrate.png" << endl;
        crateSprite.setTexture(crateTexture);
        crateSprite.setScale(60.0f / crateTexture.getSize().x,60.0f / crateTexture.getSize().y);
    }
    bool isCrateVisible() const {
        return crateVisible; }
    bool isReleased() const {
        return released; }
    bool isDone()     const {
        return done; }

    void onInteract(PlayerSoldier* p) override {
        if (released) 
            return;
        released = true;
        falling = true;

        anim.currentFrame = 1;                                      
        anim.totalFrames = 4;                                        
        anim.sprite.setTextureRect(IntRect(1 * 707, 0, 707, 586));   
        anim.loop = false;
        anim.setSpeed(0.18f);
        anim.play();
    }

    void update(float dt) override {
        if (!released) 
            return;

        anim.update(dt);

        if (falling) {
            y += fallSpeed;
            fallSpeed += 0.05f;
            if (fallSpeed > 3.0f) fallSpeed = 3.0f;
        }

        // disappear only after animation fully done
        if (anim.isFinished()) {
            doneTimer -= dt;
            if (doneTimer <= 0.0f) {
                done = true;
                crateVisible = true;   
            }
        }

        if (done) active = false;
    }
    void draw(RenderWindow& win, float cx, float cy) override {
        if (crateVisible) {
            crateSprite.setPosition(x - cx, y - cy);
            win.draw(crateSprite);
            return;
        }
        if (!active) 
            return;
        anim.setPosition(x - cx, y - cy);
        anim.draw(win);
    }
};
// Forward declarations for Weapon.h types used in EntityManager methods
class Explosion;
class FirePool;
class FlameProjectile;
class ProjectileWeapon;
class MummyWarrior;

// ENTITY MANAGER
class EntityManager {
    static const int MAX_E = 100;
    static const int MAX_P = 200;

    Enemy* enemies[MAX_E];
    Projectile* projectiles[MAX_P];
    int enemyCount;
    int projectileCount;

public:
    EntityManager() : enemyCount(0), projectileCount(0) {
        for (int i = 0; i < MAX_E; i++) 
            enemies[i] = nullptr;
        for (int i = 0; i < MAX_P; i++)
            projectiles[i] = nullptr;
    }

    void addEnemy(Enemy* e) {
        if (enemyCount < MAX_E) enemies[enemyCount++] = e;
    }
    void addProjectile(Projectile* p) {
        for (int i = 0; i < MAX_P; i++) {
            if (projectiles[i] == nullptr) {
                projectiles[i] = p;
                if (i >= projectileCount) projectileCount = i + 1;
                break;
            }
        }
    }

    int getEnemyCount()const { 
        return enemyCount; }
    int         getProjectileCount() const { 
        return projectileCount; }
    Enemy* getEnemy(int i) {
        return enemies[i]; }
    Projectile* getProjectile(int i) {
        return projectiles[i]; }

    // pool access for enemy shooting
    Projectile** getProjectilePool() { 
        return projectiles; }
    int* getProjectileCountPtr() { 
        return &projectileCount; }
    int          getMaxProjectiles() const {
        return MAX_P; }

    // only updates projectiles and enemy update is done manually in main
    void updateProjectiles(float dt) {
        for (int i = 0; i < projectileCount; i++)
            if (projectiles[i] && projectiles[i]->isActive())
                projectiles[i]->update(dt);
    }

    void resolvePlayerProjectileHits(Explosion** explosions, int& explCount, int maxExpl,
        FirePool** firePools, int& firePoolCount, int maxPools,
        ProjectileWeapon* grenadeWeapon,
        class ScoreManager* scores = nullptr,
        float camX = 0, float camY = 0,
        bool playerInAir = false);
    void resolveEnemyProjectileHits(PlayerSoldier* player);

    void drawAll(RenderWindow& win, float cx, float cy) {
        for (int i = 0; i < enemyCount; i++)
            if (enemies[i] && enemies[i]->isActive())
                enemies[i]->draw(win, cx, cy);
        for (int i = 0; i < projectileCount; i++)
            if (projectiles[i] && projectiles[i]->isActive())
                projectiles[i]->draw(win, cx, cy);
    }

    void cleanup() {
        // null incvtive
        for (int i = 0; i < enemyCount; i++) {
            if (enemies[i] && !enemies[i]->isActive()) {
                delete enemies[i];
                enemies[i] = nullptr;
            }
        }

        // null inactive
        for (int i = 0; i < projectileCount; i++) {
            if (projectiles[i] && !projectiles[i]->isActive()) {
                delete projectiles[i];
                projectiles[i] = nullptr;
            }
        }

    }

    ~EntityManager() {
        for (int i = 0; i < MAX_E; i++) {
            delete enemies[i];
            enemies[i] = nullptr;
        }
        for (int i = 0; i < MAX_P; i++) {
            delete projectiles[i];
            projectiles[i] = nullptr;
        }
    }
};

// GAME STATE (Abstract)
class GameState {
public:
    virtual void update(float dt) = 0;
    virtual void draw(RenderWindow& window) = 0;
    virtual void handleInput(Event& ev) = 0;
    virtual ~GameState() {}
};

// GAME STATE MANAGER
class GameStateManager {
    GameState* current;
public:
    GameStateManager() : current(nullptr) {}
    void setState(GameState* s) {
        delete current; 
        current = s; }
    void update(float dt) { if (current) 
        current->update(dt); }
    void draw(RenderWindow& w) { 
        if (current) 
            current->draw(w); }
    void handleInput(Event& ev) { if (current) 
        current->handleInput(ev); }
    ~GameStateManager() {
        delete current; }
};
// PLAY STATE (Concrete)
class PlayState : public GameState {
    EntityManager& entityManager;
    float dt;
public:
    PlayState(EntityManager& em) : entityManager(em), dt(0) {}

    void update(float dt) override {
        this->dt = dt;
        entityManager.updateProjectiles(dt);
    }
    void draw(RenderWindow& window) override {
        entityManager.drawAll(window, 0, 0);
    }
    void handleInput(Event& ev) override {}
};