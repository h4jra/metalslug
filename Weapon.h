#pragma once
#include "header.h"

//  FLAME PROJECTILE  5 blocks, 2HP/sec
class FlameProjectile : public Projectile {
    Sprite spr;
    Texture tex;
    bool texLoaded;
    float distanceTraveled;
    float damageTimer;
    static const float MAX_RANGE;
public:
    FlameProjectile(float x, float y, float vx, float vy, bool fromPlayer): Projectile(x, y, vx, vy, 2, fromPlayer),texLoaded(false), distanceTraveled(0), damageTimer(0){
        texLoaded = tex.loadFromFile("25I-0832_25I-0711_Assets/flame.png");
        if (texLoaded) {
            spr.setTexture(tex);
            float tw = (float)tex.getSize().x;
            float th = (float)tex.getSize().y;
            spr.setScale(32.0f / tw, 32.0f / th);
            if (vx < 0) {
                spr.setScale(-32.0f / tw, 32.0f / th);
                spr.setOrigin(tw, 0);
            }
        }
        w = 32; h = 32;
    }
    virtual bool isFlame() const {
        return true;
    }
    virtual bool isBallistic() const {
        return false;
    }
    bool canDamageNow() {
        if (damageTimer <= 0) { 
            damageTimer = 1.0f;
            return true; 
        }
        return false;
    }
    void update(float dt) override {
        x += velX;
        y += velY;
        distanceTraveled += (velX > 0 ? velX : -velX);
        if (damageTimer > 0)
            
            damageTimer -= dt;
        if (distanceTraveled >= MAX_RANGE)
            active = false;
        if (x < -100 || x > LEVEL_W * CELL + 100)
            active = false;
    }
    void draw(RenderWindow& win, float cx, float cy) override {
        spr.setPosition(x - cx, y - cy);
        win.draw(spr);
    }
};
const float FlameProjectile::MAX_RANGE = 5.0f * 64.0f;
//LASER  0.15s 
class LaserBeam {
    float x, y;       // world muzzle position when fired
    bool facingRight;
    float lifetime;
    bool active;
    Sprite  spr;
    Texture tex;
    bool texLoaded;
public:
    LaserBeam() : x(0), y(0), facingRight(true),lifetime(0), active(false), texLoaded(false){
        texLoaded = tex.loadFromFile("25I-0832_25I-0711_Assets/energyBeam.png");
        if (texLoaded)
            spr.setTexture(tex);
    }
    void spawn(float muzzleX, float muzzleY, bool fr) {
        x = muzzleX;
        y = muzzleY;
        facingRight = fr;
        lifetime = 0.15f; 
        active = true;
    }
    bool isActive() const { 
        return active;
    }
    void update(float dt) {
        if (!active)
            return;
        lifetime -= dt;
        if (lifetime <= 0) 
            active = false;
    }
    void draw(RenderWindow& win, float camX, float camY, float screenW) {
        if (!active)
            return;
        float beamX, beamW; //beam runs horizontally at muzzle Y
        if (facingRight) {
            beamX = x - camX;
            beamW = screenW - beamX;
        }
        else { 
            beamX = 0; 
        beamW = x - camX;
        }
        if (beamW <= 0)
            return;
        float beamH = 8.0f;
        float beamScreenY = y - camY;
        if (texLoaded) {
            float tw = (float)tex.getSize().x;
            float th = (float)tex.getSize().y;
            if (facingRight) {
                spr.setOrigin(0, 0); 
                spr.setScale(beamW / tw, beamH / th);
            }
            else {
                spr.setOrigin(tw, 0);
                spr.setScale(-beamW / tw, beamH / th); 
            }
            spr.setPosition(beamX, beamScreenY);
            win.draw(spr);
        }
        else {
            RectangleShape r;
            r.setSize({ beamW, beamH });
            r.setPosition(beamX, beamScreenY);
            r.setFillColor(Color(0, 255, 80, 220));
            win.draw(r);
        }
    }
};

//  FIRE POOL left by Eri's fire bomb on impact
//  10 seconds | radius 192px | 2HP/sec
class FirePool {
    float cx, cy, radius, lifetime, damageTimer;
    bool active;
public:
    FirePool(float x, float y): cx(x), cy(y), radius(192.0f),lifetime(10.0f), damageTimer(1.0f), active(true) {
    }

    bool  isActive()  const {
        return active; 
    }
    float getCX()const { 
        return cx;
    }
    float getCY() const { 
        return cy; 
    }
    float getRadius() const { 
        return radius;
    }

    bool canDamageNow(float dt) {
        damageTimer -= dt;
        if (damageTimer <= 0) {
            damageTimer = 1.0f;
            return true;
        }
        return false;
    }

    bool enemyInside(float ex, float ey, float ew, float eh) const {
        float ecx = ex + ew / 2, ecy = ey + eh / 2;
        float dx = ecx - cx, dy = ecy - cy;
        return (dx * dx + dy * dy) <= (radius * radius);
    }

    void update(float dt) { 
        lifetime -= dt; 
        if (lifetime <= 0)
            active = false;
    }

    void draw(RenderWindow& win, float camX, float camY) {
        if (!active) 
            return;
        RectangleShape pool;
        pool.setSize({ radius * 2, radius * 0.5f });
        pool.setPosition(cx - radius - camX, cy - radius * 0.25f - camY);
        pool.setFillColor(Color(255, 100, 0, 100));
        win.draw(pool);
    }
};

//  EXPLOSION
class Explosion {
    float   x, y, lifetime;
    bool  active, texLoaded;
    Sprite  spr;
    Texture tex;
public:
    Explosion(float x, float y): x(x), y(y), lifetime(0.4f), active(true), texLoaded(false){
        texLoaded = tex.loadFromFile("25I-0832_25I-0711_Assets/explosion.png");
        if (texLoaded)
            spr.setScale(128.0f / tex.getSize().x, 128.0f / tex.getSize().y);
        if (texLoaded) 
            spr.setTexture(tex);
    }

    bool isActive() const { 
        return active;
    }
    void update(float dt) {
        lifetime -= dt; 
        if (lifetime <= 0)
            active = false; }

    void draw(RenderWindow& win, float cx, float cy) {
        if (!active) 
            return;
        if (texLoaded) { 
            spr.setPosition(x - 64 - cx, y - 64 - cy);
            win.draw(spr);
        }
        else {
            RectangleShape r;
            r.setSize({ 96.f, 96.f });
            r.setPosition(x - 48 - cx, y - 48 - cy);
            r.setFillColor(Color(255, 140, 0, 200));
            win.draw(r);
        }
    }
};
class ProjectileWeapon : public Weapon {
protected:
    Projectile** projectiles;//this shared with da enjity manag
    int* projectileCount;
    int maxProjectiles;
    // gun sprite drawn on player
    Sprite  gunSprite;
    Texture gunTex;
    bool gunTexLoaded;
    float gunW, gunH;//pixel size of the gun
    //where the gun origin sits relative to
    float handOffX;    
    float handOffY; 
    //muzzle offset where shots spawn 
    float muzzleOffX;   
    float muzzleOffY;
    void loadGunSprite(const char* path,float gw, float gh,float hx, float hy,float mx, float my){
        gunTexLoaded = gunTex.loadFromFile(path);
        if (gunTexLoaded)
            gunSprite.setTexture(gunTex);
        gunW = gw; gunH = gh;
        handOffX = hx; handOffY = hy;
        muzzleOffX = mx; muzzleOffY = my;
    }
public:
    ProjectileWeapon(int ammo, float fireRate, int damage, bool infinite = false): Weapon(ammo, fireRate, damage, infinite),
projectiles(nullptr), projectileCount(nullptr), maxProjectiles(0),gunTexLoaded(false), gunW(48), gunH(24),handOffX(46), handOffY(42), muzzleOffX(94), muzzleOffY(50) {
    }
  void setProjectilePool(Projectile** pool, int* count, int max) {
        projectiles = pool; projectileCount = count; maxProjectiles = max;
    }
    bool poolReady() const {
        return projectiles && projectileCount && *projectileCount < maxProjectiles;
    }
    //world X of muzzle tip given player position nd current facing
    float getMuzzleX(float playerX, bool fr) const {
        return fr ? playerX + muzzleOffX: playerX + (90.0f - muzzleOffX);
    }
    float getMuzzleY(float playerY) const { 
        return playerY + muzzleOffY;
    }
    void drawGun(RenderWindow& win, float playerX, float playerY,bool facingRight, float camX, float camY){
        if (!gunTexLoaded)
            return;
        float tw = (float)gunTex.getSize().x;
        float th = (float)gunTex.getSize().y;
        if (facingRight) {
            gunSprite.setOrigin(0, 0);
            gunSprite.setScale(gunW / tw, gunH / th);
            gunSprite.setPosition((playerX - camX) + handOffX,(playerY - camY) + handOffY);
        }
        else {
            gunSprite.setOrigin(tw, 0);
            gunSprite.setScale(-(gunW / tw), gunH / th);
            gunSprite.setPosition((playerX - camX) + 90.0f - handOffX,(playerY - camY) + handOffY);
        }
        win.draw(gunSprite);
    }
    virtual bool isLaser() const {
        return false;
    }
    virtual bool isFireBomb() const {
        return false;
    }
    virtual bool isBallistic()const { 
        return false;
    }
    virtual bool isFlame()const { 
        return false;
    }
};
//PISTOL GUN SPRITE 
// as like the psitol cannot i ehr from projec wepaon and player iwll own one for 0 draw
struct PistolGunSprite {
    Sprite spr;
    Texture tex;
    bool loaded;
    //all offsets for a 90x110 player sprite
    static const float HAND_X;// 46
    static const float HAND_Y;// 44
    static const float GUN_W; // 44
    static const float GUN_H;// 22
    static const float MUZZLE_X;// 90 
    static const float MUZZLE_Y;// 55  
    PistolGunSprite() : loaded(false) {
        loaded = tex.loadFromFile("25I-0832_25I-0711_Assets/pistol.png");
        if (loaded) 
            spr.setTexture(tex);
    }
    // world muzzle position
    float getMuzzleX(float px, bool fr) const {
        return fr ? px + MUZZLE_X : px + (90.0f - MUZZLE_X);
    }
    float getMuzzleY(float py) const {
        return py + MUZZLE_Y;
    }
    void draw(RenderWindow& win, float px, float py,bool facingRight, float camX, float camY){
        if (!loaded)
            return;
        float tw = (float)tex.getSize().x, th = (float)tex.getSize().y;
        if (facingRight) {
            spr.setOrigin(0, 0);
            spr.setScale(GUN_W / tw, GUN_H / th);
            spr.setPosition((px - camX) + HAND_X, (py - camY) + HAND_Y);
        }
        else {
            spr.setOrigin(tw, 0);
            spr.setScale(-(GUN_W / tw), GUN_H / th);
            spr.setPosition((px - camX) + 90.0f - HAND_X, (py - camY) + HAND_Y);
        }
        win.draw(spr);
    }
};
const float PistolGunSprite::HAND_X = 46.0f;
const float PistolGunSprite::HAND_Y = 44.0f;
const float PistolGunSprite::GUN_W = 44.0f;
const float PistolGunSprite::GUN_H = 22.0f;
const float PistolGunSprite::MUZZLE_X = 90.0f;
const float PistolGunSprite::MUZZLE_Y = 55.0f;
//  HEAVY MACHINE GUN slot 1  8 shots/sec  3 HP
class HeavyMachineGun : public ProjectileWeapon {
public:
    HeavyMachineGun() : ProjectileWeapon(100, 8.0f, 3, false) {// loadGunSprite(path, gunW, gunH, handX, handY, muzzleX, muzzleY)
        loadGunSprite("25I-0832_25I-0711_Assets/hmg.png", 64, 24, 46, 42, 110, 50);
    }
    void fire(float x, float y, bool facingRight) override {
        if (!canFire() || !poolReady()) 
            return;
        float vx = facingRight ? 14.0f : -14.0f;
        //passed by tryFire()
        projectiles[(*projectileCount)++] =new StraightProjectile(x, y, vx, 0, damage, true, "25I-0832_25I-0711_Assets/hmgshot.png");
        ammo--;
        fireTimer = 1.0f / fireRate;
    }
};

//  ROCKET slot 2 BallisticProjectile, low grav 2s reload 5 HP blast radius 192px
class RocketLauncher : public ProjectileWeapon {
public:
    RocketLauncher() : ProjectileWeapon(10, 0.5f, 5, false) {
loadGunSprite("25I-0832_25I-0711_Assets/launcher.png", 72, 28, 44, 40, 116, 48);
    }
    bool isBallistic() const override {
        return true;
    }
    void fire(float x, float y, bool facingRight) override {
        if (!canFire() || !poolReady()) 
            return;
        float vx = facingRight ? 10.0f : -10.0f;
        projectiles[(*projectileCount)++] =new BallisticProjectile(x, y, vx, 0, damage, true,0.05f, "25I-0832_25I-0711_Assets/rocket.png");
        ammo--;
        fireTimer = 1.0f / fireRate;
    }
};

//  FLAME FlameProjectile 12/sec 2HP/sec instant mummy kill
class FlameShot : public ProjectileWeapon {
public:
    FlameShot() : ProjectileWeapon(200, 12.0f, 2, false) {
        loadGunSprite("25I-0832_25I-0711_Assets/flameshot.png", 64, 28, 44, 38, 108, 46);
    }

    void fire(float x, float y, bool facingRight) override {
        if (!canFire() || !poolReady()) 
            return;
        float vx = facingRight ? 6.0f : -6.0f;
        //slight vertical spread so it looks like a stream
        float vy = -0.5f + (float)(rand() % 10) * 0.1f;
        projectiles[(*projectileCount)++] =new FlameProjectile(x, y, vx, vy, true);
        ammo--;
        fireTimer = 1.0f / fireRate;
    }
};

//LASER GUN  slot 4 
class LaserGun : public ProjectileWeapon {
public:
    bool laserFiredThisFrame;
    float lastMuzzleX, lastMuzzleY; 

    LaserGun() : ProjectileWeapon(5, 1.0f, 999, false),laserFiredThisFrame(false), lastMuzzleX(0), lastMuzzleY(0){
        loadGunSprite("25I-0832_25I-0711_Assets/laser.png", 56, 22, 46, 42, 102, 50);
    }
    bool isLaser() const override { 
        return true; 
    }
    void fire(float x, float y, bool facingRight) override {
        if (!canFire())
            return;
        laserFiredThisFrame = true;
        lastMuzzleX = x;// already muzzle coords from tryFire()
        lastMuzzleY = y;
        ammo--;
        fireTimer = 1.0f / fireRate;
    }
    void update(float dt) override {
        Weapon::update(dt);
        laserFiredThisFrame = false;  // reset each frame after main reads it
    }
};
//HAND GRENADE WEAP
class HandGrenadeWeapon : public ProjectileWeapon {
public:
    HandGrenadeWeapon() : ProjectileWeapon(-1, 1.0f, 5, true) {
loadGunSprite("25I-0832_25I-0711_Assets/grenade.png", 28, 28, 50, 36, 64, 42);
    }
    bool isBallistic() const override { 
        return true;
    }
    void fire(float x, float y, bool facingRight) override {
        if (!canFire() || !poolReady())
            return;
        float vx = facingRight ? 8.0f : -8.0f;
        projectiles[(*projectileCount)++] =new StraightProjectile(x, y, vx, 0, damage, true , "25I-0832_25I-0711_Assets/grenade.png");
        fireTimer = 1.0f / fireRate;
    }
};

// FIRE BOMB 
class FireBombGrenadeWeapon : public ProjectileWeapon {
public:
    bool fireBombSpawned;
    FireBombGrenadeWeapon(): ProjectileWeapon(-1, 1.0f, 5, true), fireBombSpawned(false){
        loadGunSprite("25I-0832_25I-0711_Assets/fire_bomb.png", 28, 28, 50, 36, 64, 42);
    }
    bool isBallistic() const override {
        return true;
    }
    bool isFireBomb()  const override {
        return true; 
    }
    void fire(float x, float y, bool facingRight) override {
        if (!canFire() || !poolReady()) 
            return;
        float vx = facingRight ? 8.0f : -8.0f;
        projectiles[(*projectileCount)++] =new BallisticProjectile(x, y, vx, -12.0f, damage, true,0.3f, "25I-0832_25I-0711_Assets/fire_bomb.png");
        fireBombSpawned = true;
        fireTimer = 1.0f / fireRate;
    }
    void update(float dt) override {
        Weapon::update(dt);
        fireBombSpawned = false;//so lik e it resets
    }
};