#pragma once
#include "header.h"
#include "Weapon.h"
//da constants
const float BASE_SPEED = 4.0f;
const float BASE_FIRE_RATE = 0.25f;//s between shots
const int BASE_GRENADES = 10;
const float BASE_MELEE_DMG = 2.0f;
const float BASE_BULLET_DMG = 3.0f;
const int BASE_LIVES = 2;
struct DamageState {//da damage states
    static const int NORMAL=0;
    static const int INJURED=1;
    static const int CRITICAL=2;
    static const int DEAD=3;
};
struct Inventory {//invetory which is separat for evry char
    int activeWeapon;//o for pist , 1 hmg , 2 rl, 4fs , 4 is las
    int weaponAmmo[5];
    int grenades;
    bool hasWeapon[5];
    Inventory() {
activeWeapon = 0;
for (int i = 0; i < 5; i++) {
    weaponAmmo[i] = 0;
    hasWeapon[i] = false;
}
hasWeapon[0] = true;//so like always have pistol
weaponAmmo[0] = -1;// infinite
grenades = BASE_GRENADES;
}
};
//Abstract 
class PlayerSoldier {
    //private:
    //Weapon* currentWeapon;
protected:
    //positions
    float x, y;
    float velX, velY;
    bool  onGround;
    bool  facingRight;
    bool inWater = false;
    int lives;//stats
    int damageState;
    float damageTimer;//1s / sta
    bool  alive;
    //the transf states
    bool  isUndead;
    bool  isMummy;
    float stateTimer;//10s
    //da special stuff
    bool  specialActive;
    float specialTimer;
    SpriteAnimator anim;//sprite
    Inventory inventory;

    float fireCooldown;
    float meleeCooldown;
//weap slots
    Pistol* pistol;  
    ProjectileWeapon* weapons[5];   
    ProjectileWeapon* grenadeWeapon;//Hand or Firebom
    PistolGunSprite   pistolGun;//gun sprite for pistol slot
    int activeGunSlot;
public:
    PlayerSoldier() : x(200.f), y(100.f), velX(0), velY(0), onGround(false), facingRight(true), lives(BASE_LIVES), damageState(DamageState::NORMAL),
damageTimer(0), alive(true), isUndead(false), isMummy(false), stateTimer(0), specialActive(false), specialTimer(0),
fireCooldown(0), meleeCooldown(0), activeGunSlot(0) {
        pistol = new Pistol();
        for (int i = 0; i < 5; i++)
            weapons[i] = nullptr;
        grenadeWeapon = new HandGrenadeWeapon();
    }
    virtual ~PlayerSoldier() {
        delete pistol;
        for (int i = 1; i < 5; i++)
            delete weapons[i];
        delete grenadeWeapon;
    }
    //tehsee da pure viryals nd evry char will overridee
    virtual const char* getName()=0;
    virtual float getSpeedMult() const=0;
    virtual float getFireRateMult() const = 0;
    virtual float getMeleeDmgMult() const = 0;
    virtual int getStartingGrenades() const = 0;
    virtual float getSpecialDuration() const = 0;
    virtual void activateSpecial() = 0;
    virtual void loadSprites() = 0;
    float getX() const {//da accessors
        return x;
    }
    float getY() const {
        return y;
    }
    float getVelX() const {
        return velX;
    }
    float getVelY() const {
        return velY;
    }
    bool isOnGround() const {
        return onGround;
    }
    bool isAlive() const {
        return alive;
    }
    bool isFacingRight() const {
        return facingRight;
    }
    bool isInWater() const {
        return inWater;
    }
    void setInWater(bool val) { 
        inWater = val;
    }
    int getLives() const { 
        return lives;
    }
    int getDamageState() const {
        return damageState;
    }
    bool  getIsUndead() const { 
        return isUndead;
    }
    bool getIsMummy() const { 
        return isMummy;
    }
    bool isSpecialActive() const {
        return specialActive;
    }
    Inventory& getInventory() { 
        return inventory;
    }
    ProjectileWeapon* getGrenadeWeapon() {
        return grenadeWeapon;
    }

    //ts returns the ProjectileWeapon* for current slot nd nullptr for pis
    ProjectileWeapon* getActiveProjectileWeapon() {
        int slot = inventory.activeWeapon;
        if (slot == 0) 
            return nullptr;
        return weapons[slot];
    }
    //geom so like matches mains
    float getW() const {
        return 70.f;
    }
    float getH() const {
        return 100.f;
    }
    //helpers for da resolve colliso in main
    void setPos(float px, float py) {
        x = px; y = py;
    }
    void setVelX(float vx) {
        velX = vx;
    }
    void setVelY(float vy) { 
        velY = vy;
    }
    void applyGravity(float grav) {
        velY += grav; 
    }
    void move() {
        x += velX;
        y += velY;
    }
    void jump() {
        if (onGround) { 
            velY = -15.0f; 
            onGround = false;
        }
    }
    void fireWeapon() { 
        tryFire();
    }
    //projectile pool into all weapon
    void setProjectilePool(Projectile** pool, int* count, int max) {
        pistol->setProjectilePool(pool, count, max);
        for (int i = 1; i < 5; i++)
            if (weapons[i])
                weapons[i]->setProjectilePool(pool, count, max);
        if (grenadeWeapon) 
            grenadeWeapon->setProjectilePool(pool, count, max);
    }
    //switch weap
    void switchWeapon(int slot) {
        if (slot < 0 || slot > 4)
            return;
        if (slot > 0 && !inventory.hasWeapon[slot])
            return;
        inventory.activeWeapon = slot;
        activeGunSlot = slot;
    }
    //pick up a weapon from da crate
    void pickUpWeapon(int slot, int ammoAmt,Projectile** pool, int* count, int max){
        if (slot < 1 || slot > 4) 
            return;
        inventory.hasWeapon[slot] = true;
        inventory.weaponAmmo[slot] = ammoAmt;
        delete weapons[slot];
        if (slot == 1)
            weapons[slot] = new HeavyMachineGun();
        else if (slot == 2) 
            weapons[slot] = new RocketLauncher();
        else if (slot == 3)
            weapons[slot] = new FlameShot();
        else if (slot == 4) 
            weapons[slot] = new LaserGun();
        weapons[slot]->setProjectilePool(pool, count, max);
    }
    //all weapons for devmode
    void giveAllWeapons(Projectile** pool, int* count, int max) {
        for (int i = 1; i < 5; i++) {
            inventory.hasWeapon[i] = true;
            inventory.weaponAmmo[i] = 100;
            delete weapons[i];
            if (i == 1)
                weapons[i] = new HeavyMachineGun();
            else if (i == 2) 
                weapons[i] = new RocketLauncher();
            else if (i == 3)
                weapons[i] = new FlameShot();
            else if (i == 4)
                weapons[i] = new LaserGun();
            weapons[i]->setProjectilePool(pool, count, max);
        }
    }
    void setPosition(float px, float py) {
        x = px;
        y = py;
    }
    void setVelocity(float vx, float vy) {
        velX = vx;
        velY = vy;
    }
    void setOnGround(bool g) {
        onGround = g;
    }
    void setFacingRight(bool fr){
        facingRight = fr;
    }
    //damage
    void takeDamage(){
        if (damageState == DamageState::DEAD)
            return;
        if (damageState < DamageState::CRITICAL){
            damageState++;
            damageTimer = 1.0f;
        }
        else
            die();
    }
    void die(){
        lives--;
        if (lives <= 0)
            alive = false;
        else{
            damageState = DamageState::NORMAL;//  //reset damage state nd keep inventory
            damageTimer = 0;
        }
    }
    //tran infec
    void infectUndead(){
        if (isMummy)
            return;
        isUndead = true;
        stateTimer = 10.0f;
    }
    void infectMummy(){
        if (isUndead)
            return;
        isMummy = true;
        stateTimer = 10.0f;
        inventory.activeWeapon = 0; //mummy only knife weapon nd effectively just melee
    }
    //effective speed for states
    float getEffectiveSpeed() const{
        float spd = BASE_SPEED * getSpeedMult();
        if (isUndead)
            spd *= 0.5f;//undead so 50% slow
        return spd;
    }
    //effec fire rate like if low num so fast
    float getEffectiveFireRate() const{
        float fr = BASE_FIRE_RATE / getFireRateMult();
        if (specialActive)
            fr = adjustFireRateForSpecial(fr);
        return fr;
    }
    //subclasses gonna override to apply special power to fire rate
    virtual float adjustFireRateForSpecial(float fr) const {
        return fr;
    }

    //update  timers nd animation
    void update(float dt){
        if (!alive)
            return;
        if (damageState > DamageState::NORMAL && damageState < DamageState::DEAD){
            damageTimer -= dt;//dam state timer
            if (damageTimer <= 0)
                damageState = DamageState::NORMAL;
        }
        //transformation timers
        if (isUndead){
            stateTimer -= dt;
            if (stateTimer <= 0) { 
                isUndead = false;
                stateTimer = 0;
            }
        }
        if (isMummy){
            stateTimer -= dt;
            if (stateTimer <= 0) {
                isMummy = false;
                stateTimer = 0;
            }
        }
        // special timer
        if (specialActive){
            specialTimer -= dt;
            if (specialTimer <= 0) { 
                specialActive = false;
                specialTimer = 0;
            }
        }
        // cooldowns
        if (fireCooldown > 0)
            fireCooldown -= dt;
        if (meleeCooldown > 0) 
            meleeCooldown -= dt;
        //updatin weapon fire timers
        pistol->update(dt);
        for (int i = 1; i < 5; i++)
            if (weapons[i])
                weapons[i]->update(dt);
        if (grenadeWeapon)
            grenadeWeapon->update(dt);
        //animation
        if (velX != 0) {
            anim.playing = true;
            anim.update(dt);
        }
        else {
            anim.currentFrame = 0;
            anim.animTimer = 0.f;
            anim.sprite.setTextureRect(IntRect(0, 0, anim.frameWidth, anim.frameHeight));
        }
    }
    void draw(RenderWindow& window, float camX, float camY){
        if (!alive)
            return;
        float sx = 70.0f / (float)anim.frameWidth;
        float sy = 100.0f / (float)anim.frameHeight;
        //checkin if in water zone by position
        bool swimming = (x >= 136.0f * 64.0f && x <= 203.0f * 64.0f && y >= 25.0f * 64.0f);
        if (swimming) {
            // flipin sprite to horizontal by making width tall and height thin
            float swx = 100.0f / (float)anim.frameWidth;//for width
            float swy = 40.0f / (float)anim.frameHeight;//thin height
            if (facingRight) {
                anim.sprite.setScale(swx, swy);
                anim.setPosition(x - camX, y - camY);
            }
            else {
                anim.sprite.setScale(-swx, swy);
                anim.setPosition((x - camX) + 90.0f, y - camY);
            }
        }
        else {
            if (facingRight) {
                anim.sprite.setScale(sx, sy);
                anim.setPosition(x - camX, y - camY);
            }
            else {
                anim.sprite.setScale(-sx, sy);
                anim.setPosition((x - camX) + 90.0f, y - camY);
            }
        }
        anim.draw(window);
    }
    //gun sprite layered on player hand
    void drawGun(RenderWindow& win, float camX, float camY){
        if (!alive || isMummy)
            return;
        int slot = activeGunSlot;
        if (slot == 0)
            pistolGun.draw(win, x, y, facingRight, camX, camY);
        else if (weapons[slot])
            weapons[slot]->drawGun(win, x, y, facingRight, camX, camY);
    }
    void applyFacingToSprite(){
        float sx = 70.0f / (float)anim.frameWidth;
        float sy = 100.0f / (float)anim.frameHeight;
        anim.sprite.setScale(sx, sy);
    }

    //bullet spawns at muzzle tip of the gun
    bool tryFire(){
        if (isMummy) 
            return false;
        int slot = inventory.activeWeapon;
        //fall back to pistol when active weapo empt
        if (slot > 0 && weapons[slot] && weapons[slot]->isEmpty()) {
            inventory.activeWeapon = 0;
            activeGunSlot = 0;
            slot = 0;
        }
        if (slot == 0) {
            // pistol muzzle comes from PistolGunSprite constants
            float mx = pistolGun.getMuzzleX(x, facingRight);
            float my = pistolGun.getMuzzleY(y);
            pistol->fire(mx, my, facingRight);
        }
        else {
            if (!weapons[slot])
                return false;
            float mx = weapons[slot]->getMuzzleX(x, facingRight);
            float my = weapons[slot]->getMuzzleY(y);
            weapons[slot]->fire(mx, my, facingRight);
        }
        return true;
    }
    bool tryFireAtMouse(float mouseWorldX, float mouseWorldY){
        if (isMummy)
            return false;
        int slot = inventory.activeWeapon;
        if (slot > 0 && weapons[slot] && weapons[slot]->isEmpty()) {
            inventory.activeWeapon = 0;
            activeGunSlot = 0;
            slot = 0;
        }
        //facing from mouse side first
        facingRight = (mouseWorldX >= x + 35.0f); //ts half player width
        //for muzzle with correct facing
        float mx, my;
        if (slot == 0) {
            mx = pistolGun.getMuzzleX(x, facingRight);
            my = pistolGun.getMuzzleY(y);
        }
        else {
            if (!weapons[slot])
                return false;
            mx = weapons[slot]->getMuzzleX(x, facingRight);
            my = weapons[slot]->getMuzzleY(y);
        }
        float dx = mouseWorldX - mx;
        float dy = mouseWorldY - my;
    
        if (dy > 0) dy = 0;//no downward just 0 sy 90
        float len = sqrt(dx * dx + dy * dy);
        if (len < 1.0f) {
            dx = facingRight ? 1.0f : -1.0f;
            dy = 0.0f;
            len = 1.0f;
        }
        //perslot bullet speeds pistol 12,HMG 14, rocket 10, flame6, laser0
        float speeds[5] = { 12.0f, 14.0f, 10.0f, 6.0f, 0.0f };
        float spd = speeds[slot];
        float vx = (dx / len) * spd;
        float vy = (dy / len) * spd;
        if (slot == 0)
            pistol->fireDirected(mx, my, vx, vy);
        else
            weapons[slot]->fireDirected(mx, my, vx, vy);
        return true;
    }
    bool tryMelee(){//attemp mele
        if (meleeCooldown > 0) 
            return false;
        meleeCooldown = 0.5f;//0.5s
        return true;
    }
    bool tryThrowGrenade(){
        if (inventory.grenades <= 0)
            return false;
        inventory.grenades--;
        return true;
    }
    //activate special 
    void triggerSpecial(){
        if (specialActive) 
            return;
        specialActive = true;
        specialTimer = getSpecialDuration();
        activateSpecial();
    }
    //subclasses to get the animator for loading
    SpriteAnimator& getAnimator() { 
        return anim;
    }
};
//MARCO 25% higher fire rate, melee pierces shields, 2 less grenades, no slug bonus and his sepcial fire both directions 10s
class Marco : public PlayerSoldier{
public:
    bool firingBothDirections;//special
    Marco() : firingBothDirections(false){
        inventory.grenades = BASE_GRENADES - 2;   // 2 less grenades
    }
    const char* getName() { 
        return "Marco";
    }
    float getSpeedMult() const { 
        return 1.0f; 
    }
    float getFireRateMult() const {
        return 1.25f;
    }
    float getMeleeDmgMult() const { 
        return 1.0f;
    }
    int getStartingGrenades() const {
        return BASE_GRENADES - 2;
    }
    float getSpecialDuration()  const { 
        return 10.0f;
    }
    void activateSpecial(){
        firingBothDirections = true;
    }
    //special ends we  reset
    void update(float dt){
        PlayerSoldier::update(dt);
        if (!specialActive)
            firingBothDirections = false;
    }
    void loadSprites(){
        anim.load("25I-0832_25I-0711_Assets/MarcoRun.png", 6, 350, 510);
        anim.setSpeed(0.3f);
        float desiredW = 70.f, desiredH = 100.f;
        anim.setScale(desiredW, desiredH);
    }
};
//TARMA 25% higher vehicle fire rate, 20% vehicle durability,undying in vehicle 20% slower on foot, 20% less HP
//immunity 20s
class Tarma : public PlayerSoldier{
public:
    bool immunityActive;
    Tarma() : immunityActive(false){
        inventory.grenades = BASE_GRENADES;
    }
    const char* getName() {
        return "Tarma";
    }
    float getSpeedMult() const{
        return 0.8f;
    }
    float getFireRateMult() const{
        return 1.0f; 
    }
    float getMeleeDmgMult() const{
        return 1.0f;
    }
    int getStartingGrenades() const{ 
        return BASE_GRENADES;
    }
    float getSpecialDuration()  const{ 
        return 20.0f;
    }
    //Tarma on foot so effectively less HP nd i just did it by takeDamage skipping INJURED and going straight to CRITICAL
    void takeDamageTarma(){
        if (immunityActive) 
            return;   
        if (damageState == DamageState::NORMAL){
            damageState = DamageState::CRITICAL;
            damageTimer = 1.0f;
        }
        else
            die(); 
    }
    void activateSpecial(){
        immunityActive = true;
    }
    void update(float dt){
        PlayerSoldier::update(dt);
        if (!specialActive) 
            immunityActive = false;
    }
    void loadSprites(){
        anim.load("25I-0832_25I-0711_Assets/Tarma.png", 6, 350, 510);
        anim.setSpeed(0.3f);
        float desiredW = 70.f, desiredH = 100.f;
        anim.setScale(desiredW, desiredH);
    }
};
//ERI double grenades, 50% bigger blast radius no melee, 20% lower fire ratethrow 2 grenades for 1, for 10s
class Eri : public PlayerSoldier{
public:
    bool doubleGrenadeActive;
    Eri() : doubleGrenadeActive(false){
        inventory.grenades = BASE_GRENADES * 2;//double grenades// Eri uses FireBombGrenade
        delete grenadeWeapon;
        grenadeWeapon = new FireBombGrenadeWeapon();
    }
    const char* getName() { 
        return "Eri";
    }
    float getSpeedMult() const { 
        return 1.0f;
    }
    float getFireRateMult() const {
        return 0.8f;
    }
    float getMeleeDmgMult() const { 
        return 0.0f;
    }
    int getStartingGrenades() const { 
        return BASE_GRENADES * 2; 
    }
    float getSpecialDuration()  const { 
        return 10.0f;
    }
    bool tryMelee() {  //cannot melee
        return false;
    }
    //throws 2 grenades for  1 when special is active
    bool tryThrowGrenade(){
        if (inventory.grenades <= 0)
            return false;
        inventory.grenades--;
        return true;
    }
    void activateSpecial(){
        doubleGrenadeActive = true;
    }
    void update(float dt){
        PlayerSoldier::update(dt);
        if (!specialActive)
            doubleGrenadeActive = false;
    }
    void loadSprites(){
        anim.load("25I-0832_25I-0711_Assets/eriRun.png", 6, 350, 510);
        anim.setSpeed(0.3f);
        float desiredW = 70.f, desiredH = 100.f;
        anim.setScale(desiredW, desiredH);
    }
};

//FIO 50% more ammo on pickup, 10% higher fire rate 25% less melee damage, 2 less grenades
//fire rate 2 for 10s
class Fio : public PlayerSoldier{
public:
    Fio(){
        inventory.grenades = BASE_GRENADES - 2;
    }
    const char* getName() {
        return "Fio"; 
    }
    float getSpeedMult() const { 
        return 1.0f;
    }
    float getFireRateMult()const {
        return 1.1f;
    }
    float getMeleeDmgMult()const {
        return 0.75f;
    }
    int getStartingGrenades() const {
        return BASE_GRENADES - 2;
    }
    float getSpecialDuration()  const {
        return 10.0f;
    }
    //fire rate x2 during special
    float adjustFireRateForSpecial(float fr) const{
        return fr * 0.5f;//halve the cooldown so like double fire rate
    }
    void activateSpecial() {} //effect handled via adjustFireRateForSpecial
    void loadSprites(){
        anim.load("25I-0832_25I-0711_Assets/fioRun.png", 6, 350, 510);
        anim.setSpeed(0.3f);
        float desiredW = 70.f, desiredH = 100.f;
        anim.setScale(desiredW, desiredH);
    }
};
//ROSTER manages 4 characters, Z-key switching, per character state
class CharacterRoster{
private:
    static const int MAX_CHARS = 4;
    PlayerSoldier* chars[MAX_CHARS];
    int activeIndex;
    // Z key
    float switchCooldown;
public:
    CharacterRoster() : activeIndex(0), switchCooldown(0){
        chars[0] = new Marco();
        chars[1] = new Tarma();
        chars[2] = new Eri();
        chars[3] = new Fio();
        for (int i = 0; i < MAX_CHARS; i++)
            chars[i]->loadSprites();
        // all start at same position; set in init
    }
    ~CharacterRoster(){
        for (int i = 0; i < MAX_CHARS; i++)
            delete chars[i];
    }
    //initialise all characters
    void spawnAll(float px, float py){
        for (int i = 0; i < MAX_CHARS; i++)
            chars[i]->setPosition(px, py);
    }
    //Z ke
    bool switchToNext(){
        if (switchCooldown > 0)
            return false;
        switchCooldown = 0.3f;
        int attempts = 0;
        int next = (activeIndex + 1) % MAX_CHARS;
        while (attempts < MAX_CHARS){
            if (chars[next]->isAlive()){//so carry position over to new character
                float cx = chars[activeIndex]->getX();
                float cy = chars[activeIndex]->getY();
                float vx = chars[activeIndex]->getVelX();
                float vy = chars[activeIndex]->getVelY();
                bool og = chars[activeIndex]->isOnGround();
                activeIndex = next;
                chars[activeIndex]->setPosition(cx, cy);
                chars[activeIndex]->setVelocity(vx, vy);
                chars[activeIndex]->setOnGround(og);
                return true;
            }
            next = (next + 1) % MAX_CHARS;
            attempts++;
        }
        return false;//all dead
    }
    //getters
    PlayerSoldier* getActive() { 
        return chars[activeIndex];
    }
    int getActiveIndex()  const { 
        return activeIndex;
    }
    PlayerSoldier* getChar(int i) {
        return (i >= 0 && i < MAX_CHARS) ? chars[i] : nullptr;
    }
    int getCount() const { 
        return MAX_CHARS;
    }
    //check for  if any character is still alive
    bool anyAlive() const{
        for (int i = 0; i < MAX_CHARS; i++)
            if (chars[i]->isAlive())
                return true;
        return false;
    }
    //update switch
    void update(float dt){
        if (switchCooldown > 0) switchCooldown -= dt;
    }
    //draw HUD indicator names + lives at top of screen
    void drawHUD(RenderWindow& window, Font& font){
        static const char* names[MAX_CHARS] = { "Marco", "Tarma", "Eri", "Fio" };
        static const Color colors[MAX_CHARS] = {Color(255,200,100), Color(100,200,255),Color(255,150,200), 
Color(150,255,150) };

        for (int i = 0; i < MAX_CHARS; i++){
            Text t;
            t.setFont(font);
            t.setCharacterSize(18);
            //build display string: name + hearts for lives
            //using simple ASCII since we can't use string easily
            char buf[64] = {};
            const char* nm = names[i];
            int len = 0;
            while (nm[len]) { 
                buf[len] = nm[len];
                len++;
            }
            //append lives as [L:x]
            buf[len++] = ' '; buf[len++] = '[';
            buf[len++] = 'L'; buf[len++] = ':';
            int lv = chars[i]->getLives();
            buf[len++] = (char)('0' + (lv > 9 ? 9 : lv));
            buf[len++] = ']';
            buf[len] = '\0';
            t.setString(buf);
            // highlight active character
            if (i == activeIndex){
                t.setFillColor(Color::White);
                t.setStyle(Text::Bold);
                //draw a background rectangle to make it pop
                RectangleShape bg;
                bg.setSize({ 120.f, 24.f });
                bg.setFillColor(Color(0, 0, 0, 150));
                bg.setPosition(10.f + i * 130.f, 8.f);
                window.draw(bg);
            }
            else if (!chars[i]->isAlive()){
                t.setFillColor(Color(80, 80, 80));   // grey = dead
            }
            else
                t.setFillColor(colors[i]);
            t.setPosition(14.f + i * 130.f, 10.f);
            window.draw(t);
        }
        //grenade count for active character
        Text gText;
        gText.setFont(font);
        gText.setCharacterSize(18);
        gText.setFillColor(Color::Yellow);
        char gbuf[32] ="Grenades: ";
        int gi = 10;
        int gr = getActive()->getInventory().grenades;
        if (gr >= 10) { gbuf[gi++] = (char)('0' + gr / 10); }
        gbuf[gi++] = (char)('0' + gr % 10);
        gbuf[gi] = '\0';
        gText.setString(gbuf);
        gText.setPosition(14.f, 36.f);
        window.draw(gText);
        //da damage state red overlay
        int ds = getActive()->getDamageState();
        if (ds > DamageState::NORMAL){
            RectangleShape redVignette;
            redVignette.setSize({ 1600.f, 900.f });
            Uint8 alpha = (ds == DamageState::INJURED) ? 60 : 100;
            redVignette.setFillColor(Color(200, 0, 0, alpha));
            window.draw(redVignette);
        }
    }
};