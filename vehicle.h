#pragma once
#include "header.h"
//BASE: Vehicle
// UML: Vehicle -> GroundVehicle, AerialVehicle, AquaticVehicle
class Vehicle {
protected:
    float x, y;
    float velX, velY;
    bool facingRight;
    bool active;//player usin or nah
    bool destroyed;
    int hp;
    int maxHP;
    SpriteAnimator anim;
    float durabilityMult;//modifier set by Tarma's buff
    float fireCooldown;
    float fireRate; //s between shots
    float dispW, dispH;  // set by each subclass in loadSprites()
public:
    Vehicle(int hp_, float fireRate_) : x(0), y(0), velX(0), velY(0), facingRight(true), active(false), destroyed(false),
hp(hp_), maxHP(hp_), durabilityMult(1.0f), fireCooldown(0), fireRate(fireRate_) {
    }
    virtual ~Vehicle() {}
    virtual void loadSprites() = 0;
    virtual void update(float dt, char levelMap[][LEVEL_W], int levelW, int levelH, int cellSize) = 0;
    virtual void draw(RenderWindow& window, float camX, float camY) = 0;
    virtual const char* getType() const = 0;
    //accesoors
    float getX() const {
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
    bool  isActive() const {
        return active;
    }
    bool isDestroyed() const {
        return destroyed;
    }
    bool isFacingRight() const {
        return facingRight;
    }
    int getHP() const {
        return hp;
    }
    void setPosition(float px, float py) {
        x = px; y = py;
    }
    void setActive(bool a) {
        active = a;
    }
    void setDurabilityMult(float m) {
        durabilityMult = m;
    }
    //damag

    bool takeDamage(int dmg) {
        if (destroyed)
            return false;
        hp -= dmg;
        if (hp <= 0) {
            hp = 0;
            destroyed = true;
            active = false;
            return true;//if vehicl got destroyed so true
        }
        return false;
    }
    void applyFacing() {
        float tw = (float)anim.texture.getSize().x;
        float th = (float)anim.texture.getSize().y;
        if (tw == 0 || th == 0)
            return;
        float sx = (float)anim.frameWidth;
        float sy = (float)anim.frameHeight;
        anim.sprite.setScale(facingRight ? (dispW / sx) : -(dispW / sx), dispH / sy);
    }
    bool canFire() {
        return fireCooldown <= 0.0f;
    }
    void resetFireCooldown() {
        fireCooldown = fireRate;
    }
};
// GROUND VEHICLE
class GroundVehicle : public Vehicle {
public:
    GroundVehicle(int hp, float fireRate) : Vehicle(hp, fireRate) {}
    virtual bool canFly()  const { 
        return false;
    }
    virtual bool canSwim() const {
        return false;
    }
};

// AERIAL VEHICLE 
class AerialVehicle : public Vehicle {
public:
    AerialVehicle(int hp, float fireRate) : Vehicle(hp, fireRate) {}
    virtual bool canFly()  const { 
        return true;
    }
    virtual bool canSwim() const { 
        return false;
    }
};
// AQUATIC VEHICLE
class AquaticVehicle : public Vehicle {
public:
    AquaticVehicle(int hp, float fireRate) : Vehicle(hp, fireRate) {}
    virtual bool canFly()  const {
        return false;
    }
    virtual bool canSwim() const { 
        return true;
    }
};

//Groud Vehicle metal slug so basially it can tilt traverse 1 block height and tarma ka 20% more also he can survive destruct
class MetalSlug : public GroundVehicle {
private:
    static const int w = 96;
    static const int h = 64;
    bool  onGround;
    float gravity;
    float jumpVel;
    bool canJump;
public:
    MetalSlug() : GroundVehicle(10, 3.0f), onGround(false), gravity(0.5f), jumpVel(-12.0f), canJump(false) {}//i added 10 hp so fast rate
    const char* getType() const {
        return "MetalSlug";
    }
    void loadSprites() {
        dispW = 96;
        dispH = 85; 
        anim.load("25I-0832_25I-0711_Assets/slugIdle.png", 1, 1759, 1530);
        anim.setSpeed(0.12f);
        applyFacing();
    }
    void update(float dt, char levelMap[][LEVEL_W], int levelW, int levelH, int cellSize) {
        if (destroyed || !active)
            return;
        velY += gravity;//gravity
        if (velY > 15.0f)
            velY = 15.0f;
        x += velX * dt * 60.0f;
        y += velY;
        int colL = (int)(x / cellSize);//coliisio  with tiles
        int colR = (int)((x + w - 1) / cellSize);
        int rowFeet = (int)((y + h) / cellSize);
        int rowHead = (int)(y / cellSize);
        colL = colL < 0 ? 0 : (colL >= levelW ? levelW - 1 : colL);
        colR = colR < 0 ? 0 : (colR >= levelW ? levelW - 1 : colR);
        rowFeet = rowFeet < 0 ? 0 : (rowFeet >= levelH ? levelH - 1 : rowFeet);
        rowHead = rowHead < 0 ? 0 : (rowHead >= levelH ? levelH - 1 : rowHead);
        char tl = levelMap[rowFeet][colL];
        char tr = levelMap[rowFeet][colR];
        bool solidFeet = (tl != 's' && tl != 'w' && tl != ' ') || (tr != 's' && tr != 'w' && tr != ' ');
        onGround = false;
        if (velY >= 0 && solidFeet) {
            y = (float)(rowFeet * cellSize) - h;
            velY = 0.0f;
            onGround = true;
            canJump = true;
        }
        char hl = levelMap[rowHead][colL];
        char hr = levelMap[rowHead][colR];
        bool solidHead = (hl != 's' && hl != 'w' && hl != ' ') ||
            (hr != 's' && hr != 'w' && hr != ' ');
        if (velY < 0 && solidHead) {
            y = (float)((rowHead + 1) * cellSize);
            velY = 0.0f;
        }
        //horizontal walls
        int rowTop = (int)(y / cellSize);
        int rowBottom = (int)((y + h - 1) / cellSize);
        rowTop = rowTop < 0 ? 0 : (rowTop >= levelH ? levelH - 1 : rowTop);
        rowBottom = rowBottom < 0 ? 0 : (rowBottom >= levelH ? levelH - 1 : rowBottom);

        if (velX > 0) {
            int colREdge = (int)((x + w) / cellSize);
            colREdge = colREdge < 0 ? 0 : (colREdge >= levelW ? levelW - 1 : colREdge);
            char rt = levelMap[rowTop][colREdge];
            char rb = levelMap[rowBottom][colREdge];
            if ((rt != 's' && rt != 'w' && rt != ' ') || (rb != 's' && rb != 'w' && rb != ' ')) {
                x = (float)(colREdge * cellSize) - w;
                velX = 0;
            }
        }
        else if (velX < 0) {
            int colLEdge = (int)(x / cellSize);
            colLEdge = colLEdge < 0 ? 0 : (colLEdge >= levelW ? levelW - 1 : colLEdge);
            char lt = levelMap[rowTop][colLEdge];
            char lb = levelMap[rowBottom][colLEdge];
            if ((lt != 's' && lt != 'w' && lt != ' ') || (lb != 's' && lb != 'w' && lb != ' ')) {
                x = (float)((colLEdge + 1) * cellSize);
                velX = 0;
            }
        }

        // bounds
        if (x < 0) x = 0;
        if (y < 0) { y = 0; velY = 0; }

        if (fireCooldown > 0)
            fireCooldown -= dt;
        applyFacing();
        anim.update(dt);
    }
    void handleInput(float speed, bool isTarma) {//this is gonna handle the input
        if (destroyed || !active)
            return;

        float spd = speed * (isTarma ? 1.0f : 1.0f); //Tarma speed same on vehicle
        if (isTarma)
            fireRate = 0.12f * 0.8f;//da 25% faster fire rate
        velX = 0;
        if (Keyboard::isKeyPressed(Keyboard::Right)) {
            velX = spd;
            facingRight = true;
        }
        if (Keyboard::isKeyPressed(Keyboard::Left)) {
            velX = -spd;
            facingRight = false;
        }
        if (Keyboard::isKeyPressed(Keyboard::Up) && canJump) {
            velY = jumpVel;
            canJump = false;
        }
    }

    void draw(RenderWindow& window, float camX, float camY) {
        if (destroyed)
            return;
        float drawY = y - (dispH - h);
        anim.setPosition(x - camX, drawY - camY);
        anim.draw(window);
    }
};
//Aerial Vehicle , slug flyer and this got no fravity fire rate also slow like half from slug's nd 4 missiles
class SlugFlyer : public AerialVehicle {
private:
    static const int flyW = 96;
    static const int flyH = 48;
    int   missiles;
    float missileCooldown;
public:
    SlugFlyer() :AerialVehicle(10, 3.0f), missiles(4), missileCooldown(0) {}//8 so slow
    const char* getType() const {
        return "SlugFlyer";
    }
    void loadSprites() {
        dispW = flyW; dispH = flyH;  
        anim.load("25I-0832_25I-0711_Assets/slugFlyer.png", 1, 1759, 1045);
        anim.setSpeed(0.15f);
        applyFacing();
    }

    //getetrs 
    int  getMissiles() const {
        return missiles;
    }
    bool canFireMissile()  const {
        return missiles > 0 && missileCooldown <= 0;
    }
    void fireMissile() {
        if (missiles > 0) {
            missiles--;
            missileCooldown = 1.0f;
        }
    }
    void fireMissileProjectile(Projectile** pool, int* count, int maxP) {
        if (!canFireMissile())
            return;
        float mx = x + (facingRight ? flyW : 0);
        float my = y + flyH / 2;
        if (*count >= maxP)
            return;
        pool[(*count)++] = new BallisticProjectile(mx, my, facingRight ? 10.0f : -10.0f, -8.0f, 5, true, 0.3f, "25I-0832_25I-0711_Assets/granade.png");
        fireMissile();
    }
    void handleInput(float speed, bool isTarma) {
        if (destroyed || !active)
            return;
        if (isTarma)
            fireRate = 0.24f * 0.8f;
        velX = 0; velY = 0;
        if (Keyboard::isKeyPressed(Keyboard::Right)) {
            velX = speed;
            facingRight = true;
        }
        if (Keyboard::isKeyPressed(Keyboard::Left)) {
            velX = -speed;
            facingRight = false;
        }
        if (Keyboard::isKeyPressed(Keyboard::Up))
            velY = -speed;
        if (Keyboard::isKeyPressed(Keyboard::Down))
            velY = speed;
    }

    void update(float dt, char levelMap[][LEVEL_W], int levelW, int levelH, int cellSize) {
        if (destroyed || !active)
            return;
        x += velX * dt * 60.0f;
        y += velY * dt * 60.0f;
        if (x < 0)
            x = 0;//no grav
        if (y < 0)
            y = 0;
        if (fireCooldown > 0)
            fireCooldown -= dt;
        if (missileCooldown > 0)
            missileCooldown -= dt;
        applyFacing();
        anim.update(dt);
    }

    void draw(RenderWindow& window, float camX, float camY) {
        if (destroyed)
            return;
        anim.setPosition(x - camX, y - camY);
        anim.draw(window);
    }
};

//AQuatic , slug marin so got underwater movement 3 missiles like vertical,hori and reverse 
class SlugMariner : public AquaticVehicle {
private:
    static const int marW = 112;
    static const int marH = 48;
    int missileAmmo[3];//3 each
    int selectedMissile; // 0,1,2
    float missileCooldown;
public:
    SlugMariner() :AquaticVehicle(10, 3.0f), selectedMissile(0), missileCooldown(0) {
        missileAmmo[0] = 3;
        missileAmmo[1] = 3;
        missileAmmo[2] = 3;
    }
    const char* getType() const {
        return "SlugMariner";
    }

    void loadSprites() {
        dispW = marW; dispH = marH;   // 112×48
        anim.load("25I-0832_25I-0711_Assets/slugMariner.png", 1, 1759, 1599);
        anim.setSpeed(0.2f);
        applyFacing();
    }

    int  getSelectedMissile() const {
        return selectedMissile;
    }
    int  getMissileAmmo(int type) const {
        return (type >= 0 && type < 3) ? missileAmmo[type] : 0;
    }
    bool canFireMissile() const {
        return missileAmmo[selectedMissile] > 0 && missileCooldown <= 0;
    }
    void cycleMissile() {
        selectedMissile = (selectedMissile + 1) % 3;
    }
    void fireMissile() {
        if (canFireMissile()) {
            missileAmmo[selectedMissile]--;
            missileCooldown = 1.0f;
        }
    }
    void fireMissileProjectile(Projectile** pool, int* count, int maxP) {
        if (!canFireMissile())
            return;
        float mx = x + (facingRight ? marW : 0);
        float my = y + marH / 2;
        if (*count >= maxP)
            return;
        if (selectedMissile == 0) { // horizontal
            pool[(*count)++] = new StraightProjectile(mx, my, facingRight ? 15.0f : -15.0f, 0, 5, true, "25I-0832_25I-0711_Assets/bullet.png");
        }
        else if (selectedMissile == 1) { // vertical 
            pool[(*count)++] = new StraightProjectile(mx, my, 0, -15.0f, 5, true, "25I-0832_25I-0711_Assets/bullet.png");
        }
        else { // reverse arc like ballistic upward toward surface
            pool[(*count)++] = new BallisticProjectile(mx, my, facingRight ? 8.0f : -8.0f, -10.0f, 5, true, 0.3f, "25I-0832_25I-0711_Assets/grenade.png");
        }
        fireMissile(); // decrement ammo + cooldown
    }

    void handleInput(float speed, bool isTarma) {
        if (destroyed || !active)
            return;
        if (isTarma)
            fireRate = 0.3f * 0.8f;
        velX = 0; velY = 0;
        if (Keyboard::isKeyPressed(Keyboard::Right)) {
            velX = speed;
            facingRight = true;
        }
        if (Keyboard::isKeyPressed(Keyboard::Left)) {
            velX = -speed;
            facingRight = false;
        }
        if (Keyboard::isKeyPressed(Keyboard::Up))
            velY = -speed;
        if (Keyboard::isKeyPressed(Keyboard::Down))
            velY = speed;
        static bool tabWasPressed = false;//TAB KEY CYCLES THE missile
        bool tabNow = Keyboard::isKeyPressed(Keyboard::Tab);
        if (tabNow && !tabWasPressed)
            cycleMissile();
        tabWasPressed = tabNow;
    }

    void update(float dt, char levelMap[][LEVEL_W], int levelW, int levelH, int cellSize) {
        if (destroyed || !active)
            return;
        x += velX * dt * 60.0f;
        y += velY * dt * 60.0f;
        velX *= 0.85f;//the water drag
        velY *= 0.85f;
        if (x < 0)
            x = 0;
        float waterTop = (float)25 * cellSize; // row 25 is where water starts
        if (y < waterTop)
            y = waterTop;
        //added a bottom clamp
        if (y > (LEVEL_H - 2) * cellSize)
            y = (float)((LEVEL_H - 2) * cellSize);
        if (fireCooldown > 0)
            fireCooldown -= dt;
        if (missileCooldown > 0)
            missileCooldown -= dt;
        applyFacing();
        anim.update(dt);
    }

    void draw(RenderWindow& window, float camX, float camY) {
        if (destroyed)
            return;
        anim.setPosition(x - camX, y - camY);
        anim.draw(window);
    }
};

//Ambig sluv the uml extends the 3 vehcilces it owns the 3 ty[es the act mode is based on tile udner vehic and the biome rules
class AmphibiousSlug : public Vehicle {
public:
    static const int groun = 0;//the mode consts
    static const int air = 1;
    static const int  water = 2;

private:
    static const int amphiW = 56;
    static const int amphiH = 32;
    int mode;
    bool onGround;
    float gravity;
    //old animation classes nd 3 separate animators for each visual state
    SpriteAnimator groundAnim;
    SpriteAnimator airAnim;
    SpriteAnimator waterAnim;
    int missiles;// shared pool for air missiles
    int missileAmmo[3];//mariner missiles
    int selectedMissile;
    float missileCooldown;
public:
    AmphibiousSlug() : Vehicle(12, 0.12f), mode(groun), onGround(false), gravity(0.5f), missiles(4), selectedMissile(0), missileCooldown(0) {
        missileAmmo[0] = 3;
        missileAmmo[1] = 3;
        missileAmmo[2] = 3;
    }
    const char* getType() const {
        return "AmphibiousSlug";
    }
    int getMode() const {
        return mode;
    }

    void loadSprites() {
        dispW = 96;
        dispH = 72;
        groundAnim.load("25I-0832_25I-0711_Assets/amphibous.png", 1, 1759, 948);
        groundAnim.setSpeed(0.12f);
        airAnim.load("25I-0832_25I-0711_Assets/amphibous.png", 1, 1759, 948);
        airAnim.setSpeed(0.15f);
        waterAnim.load("25I-0832_25I-0711_Assets/amphibous.png", 1, 1759, 948); 
        waterAnim.setSpeed(0.2f);
        anim = groundAnim;
        applyFacing();
    }
    bool canFireMissile() const {
        if (mode == air)
            return missiles > 0 && missileCooldown <= 0;
        if (mode == water)
            return missileAmmo[selectedMissile] > 0 && missileCooldown <= 0;
        return false;
    }
    void fireMissile() {
        if (!canFireMissile())
            return;
        if (mode == air)
            missiles--;
        if (mode == water)
            missileAmmo[selectedMissile]--;
        missileCooldown = 1.0f;
    }
    int getSelectedMissile() const {
        return selectedMissile;
    }
    void handleInput(float speed, bool isTarma) {
        if (destroyed || !active)
            return;
        if (isTarma)
            fireRate = 0.12f * 0.8f;
        velX = 0;
        if (mode != air && mode != water)
            velY = 0; //only reset Y for ground ndgravity handles the rest
        if (Keyboard::isKeyPressed(Keyboard::Right)) {
            velX = speed;
            facingRight = true;
        }
        if (Keyboard::isKeyPressed(Keyboard::Left)) {
            velX = -speed;
            facingRight = false;
        }
        if (mode == air || mode == water) {
            velY = 0;
            if (Keyboard::isKeyPressed(Keyboard::Up))
                velY = -speed;
            if (Keyboard::isKeyPressed(Keyboard::Down))
                velY = speed;
        }
        else if (mode == groun) {// jump
            if (Keyboard::isKeyPressed(Keyboard::Up) && onGround) {
                velY = -12.0f;
                onGround = false;
            }
        }
        //Tab cycles mariner missile in water mode
        static bool tabWas = false;
        bool tabNow = Keyboard::isKeyPressed(Keyboard::Tab);
        if (tabNow && !tabWas && mode == water)
            selectedMissile = (selectedMissile + 1) % 3;
        tabWas = tabNow;
    }

    void update(float dt, char levelMap[][LEVEL_W], int levelW, int levelH, int cellSize) {
        if (destroyed || !active)
            return;
        int centerCol = (int)((x + amphiW / 2) / cellSize);//detecting the tiles
        int centerRow = (int)((y + amphiH / 2) / cellSize);
        centerCol = centerCol < 0 ? 0 : (centerCol >= levelW ? levelW - 1 : centerCol);
        centerRow = centerRow < 0 ? 0 : (centerRow >= levelH ? levelH - 1 : centerRow);
        char centerTile = levelMap[centerRow][centerCol];
        if (centerTile == 'w') {
            mode = water;
            onGround = false;
        }
        else {
        
            int feetRow = (int)((y + amphiH + 2) / cellSize);
            feetRow = feetRow < 0 ? 0 : (feetRow >= levelH ? levelH - 1 : feetRow);
            int centerCol2 = (int)((x + amphiW / 2) / cellSize);
            centerCol2 = centerCol2 < 0 ? 0 : (centerCol2 >= levelW ? levelW - 1 : centerCol2);
            char feetTile = levelMap[feetRow][centerCol2];
            if (feetTile != 's' && feetTile != 'w' && feetTile != ' ')
                mode = groun;
            else
                mode = air;
        }

        if (mode == groun) {//the physic
            velY += gravity;
            if (velY > 15.0f)
                velY = 15.0f;
        }
        else if (mode == water) {
            velX *= 0.85f;
            velY *= 0.85f;
        }
        x += velX * dt * 60.0f;//air so no grav
        y += velY;
        if (mode == groun) {//tile ollis is only with ground mode
            int colL = (int)(x / cellSize);
            int colR = (int)((x + amphiW - 1) / cellSize);
            int rowFeet = (int)((y + amphiH) / cellSize);
            int rowHead = (int)(y / cellSize);
            colL = colL < 0 ? 0 : (colL >= levelW ? levelW - 1 : colL);
            colR = colR < 0 ? 0 : (colR >= levelW ? levelW - 1 : colR);
            rowFeet = rowFeet < 0 ? 0 : (rowFeet >= levelH ? levelH - 1 : rowFeet);
            rowHead = rowHead < 0 ? 0 : (rowHead >= levelH ? levelH - 1 : rowHead);
            char tl = levelMap[rowFeet][colL];
            char tr = levelMap[rowFeet][colR];
            bool solidFeet = (tl != 's' && tl != 'w' && tl != ' ') || (tr != 's' && tr != 'w' && tr != ' ');

            onGround = false;
            if (velY >= 0 && solidFeet) {
                y = (float)(rowFeet * cellSize) - amphiH;
                velY = 0;
                onGround = true;
            }
            char hl = levelMap[rowHead][colL];
            char hr = levelMap[rowHead][colR];
            bool solidHead = (hl != 's' && hl != 'w' && hl != ' ') || (hr != 's' && hr != 'w' && hr != ' ');
            if (velY < 0 && solidHead) {
                y = (float)((rowHead + 1) * cellSize); velY = 0;
            }

            int rowTop = (int)(y / cellSize);
            int rowBottom = (int)((y + amphiH - 1) / cellSize);
            rowTop = rowTop < 0 ? 0 : (rowTop >= levelH ? levelH - 1 : rowTop);
            rowBottom = rowBottom < 0 ? 0 : (rowBottom >= levelH ? levelH - 1 : rowBottom);
            if (velX > 0) {
                int colRE = (int)((x + amphiW) / cellSize);
                colRE = colRE < 0 ? 0 : (colRE >= levelW ? levelW - 1 : colRE);
                char rt = levelMap[rowTop][colRE];
                char rb = levelMap[rowBottom][colRE];
                if ((rt != 's' && rt != 'w' && rt != ' ') || (rb != 's' && rb != 'w' && rb != ' ')) {
                    x = (float)(colRE * cellSize) - amphiW; velX = 0;
                }
            }
            else if (velX < 0) {
                int colLE = (int)(x / cellSize);
                colLE = colLE < 0 ? 0 : (colLE >= levelW ? levelW - 1 : colLE);
                char lt = levelMap[rowTop][colLE];
                char lb = levelMap[rowBottom][colLE];
                if ((lt != 's' && lt != 'w' && lt != ' ') || (lb != 's' && lb != 'w' && lb != ' ')) {
                    x = (float)((colLE + 1) * cellSize); velX = 0;
                }
            }
        }

        if (x < 0)
            x = 0;
        if (y < 0) {
            y = 0;
            velY = 0;
        }

        if (fireCooldown > 0)
            fireCooldown -= dt;
        if (missileCooldown > 0)
            missileCooldown -= dt;

        applyFacing();
        if (mode == groun)
            groundAnim.update(dt);
        else if (mode == air)
            airAnim.update(dt);
        else
            waterAnim.update(dt);
    }

    void draw(RenderWindow& window, float camX, float camY) {
        if (destroyed) 
            return;
        if (!groundAnim.sprite.getTexture()) 
            return; // not loaded yet

        float sx = x - camX;
        float sy = y - camY;

        if (mode == groun) {
            groundAnim.setScale(facingRight ? (float)96 : (float)-96, (float)72);
            groundAnim.setPosition(facingRight ? sx : sx + 96, sy);
            groundAnim.draw(window);
        }
        else if (mode == air) {
            airAnim.setScale(facingRight ? (float)96 : (float)-96, (float)72);
            airAnim.setPosition(facingRight ? sx : sx + 96, sy);
            airAnim.draw(window);
        }
        else {
            waterAnim.setScale(facingRight ? (float)96 : (float)-96, (float)72);
            waterAnim.setPosition(facingRight ? sx : sx + 96, sy);
            waterAnim.draw(window);
        }
    }
};

// ENEMY VEHICLE BASE
class EnemyVehicle {
protected:
    float x, y;
    float velX;
    int hp;
    bool destroyed;
    bool  facingRight;
    SpriteAnimator anim;
    float fireTimer;
    float fireRate;

public:
    EnemyVehicle(float x_, float y_, int hp_, float fireRate_) : x(x_), y(y_), velX(0), hp(hp_), destroyed(false), facingRight(false), fireTimer(0), fireRate(fireRate_) {
    }
    virtual ~EnemyVehicle() {}
    virtual void loadSprites() = 0;
    virtual void update(float dt, float playerX, float playerY, Projectile** pool, int* count, int maxP, char levelMap[][LEVEL_W], int cellSize) = 0;
    virtual void draw(RenderWindow& win, float camX, float camY) = 0;
    bool takeDamage(int dmg) {
        if (destroyed)
            return false;
        hp -= dmg;
        if (hp <= 0) {
            hp = 0;
            destroyed = true;
            return true;
        }
        return false;
    }

    bool  isDestroyed() const {
        return destroyed;
    }
    float getX() const {
        return x;
    }
    float getY() const {
        return y;
    }
    int   getHP()  const {
        return hp;
    }
};

//BRADLEY  (enemy ground, 7 HP) Stationary missile launcher, moves horizontally on flat ground toward player then stops and fires ballistic missiles
class Bradley : public EnemyVehicle {
    static const int bW = 96;
    static const int bH = 80;
    bool  onGround;
    float gravity;

    float approachRange;//stops moving when this close to player
public:
    Bradley(float x_, float y_) : EnemyVehicle(x_, y_, 7, 3.0f), onGround(false), gravity(0.5f), approachRange(600.f) {
    }
    void loadSprites() override {
        anim.load("25I-0832_25I-0711_Assets/bradley.png", 1, 1759, 1532);
        anim.setScale((float)bW, (float)bH);
    }
    void update(float dt, float playerX, float playerY, Projectile** pool, int* count, int maxP, char levelMap[][LEVEL_W], int cellSize) override {
        if (destroyed)
            return;
        //gravity so it sits on ground
        velX = 0;
        float dist = playerX - (x + bW / 2);
        if (abs(dist) > approachRange) {
            velX = (dist > 0) ? -1.5f : 1.5f;//approach the player slowli
            facingRight = dist > 0;
        }
        x += velX;
        //gravity + ground snap
        y += gravity;
        int col = (int)((x + bW / 2) / cellSize);
        int rowF = (int)((y + bH) / cellSize);
        col = col < 0 ? 0 : (col >= LEVEL_W ? LEVEL_W - 1 : col);
        rowF = rowF < 0 ? 0 : (rowF >= LEVEL_H ? LEVEL_H - 1 : rowF);
        char tf = levelMap[rowF][col];
        if (tf != 's' && tf != 'w' && tf != ' ') {
            y = (float)(rowF * cellSize) - bH;
        }
        // fire ballistic missile toward player
        fireTimer -= dt;
        if (fireTimer <= 0) {
            fireTimer = fireRate;
            if (*count < maxP) {
                float mx = x + (facingRight ? bW : 0);
                float my = y;
                float dx = playerX - mx;
                float dy = playerY - my;
                float len = sqrt(dx * dx + dy * dy);
                if (len > 0) { dx /= len; dy /= len; }
                // ballistic arc: give upward initial velocity
                //da -10 is da steep arc
                pool[(*count)++] = new BallisticProjectile(mx, my, dx * 8.0f, -10.0f, 5, false, 0.25f, "25I-0832_25I-0711_Assets/missile.png");
            }
        }
        anim.update(dt);
    }

    void draw(RenderWindow& win, float camX, float camY) override {
        if (destroyed)
            return;
        float sx = x - camX;
        float sy = y - camY;
        float scx = anim.sprite.getScale().x;
        float scy = anim.sprite.getScale().y;
        float absx = scx < 0 ? -scx : scx;
        anim.sprite.setScale(facingRight ? absx : -absx, scy);
        anim.setPosition(sx, sy);
        anim.draw(win);
    }
};

// ENEMY SUB  (aquatic biome, 7 HP)Stays underwater nd fires rockets at player
class EnemySub : public EnemyVehicle {
    static const int sW = 128;
    static const int sH = 56;
    float waterTop; //y of water surface (row 25 * cellSize)
public:
    EnemySub(float x_, float y_) : EnemyVehicle(x_, y_, 7, 2.5f), waterTop(25 * CELL) {
    }
    void loadSprites() override {
        anim.load("25I-0832_25I-0711_Assets/enemySub.png", 1, 1759, 920);
        anim.setScale((float)sW, (float)sH);
    }
    void update(float dt, float playerX, float playerY, Projectile** pool, int* count, int maxP, char levelMap[][LEVEL_W], int cellSize) override {
        if (destroyed)
            return;

        // track player horizontally
        float dist = playerX - (x + sW / 2);
        facingRight = dist > 0;

        if (abs(dist) > 150.0f) {
            velX = (dist > 0) ? 1.5f : -1.5f;
            x += velX;
        }
        else {
            velX = 0;
        }

        // clamp to aquatic biome only (cols 136-203)
        float aquaticLeft = 136.0f * CELL;
        float aquaticRight = 203.0f * CELL;
        if (x < aquaticLeft)  x = aquaticLeft;
        if (x > aquaticRight) x = aquaticRight;

        // clamp to stay underwater
        if (y < waterTop + cellSize)
            y = waterTop + (float)cellSize;

        fireTimer -= dt;
        if (fireTimer <= 0) {
            fireTimer = fireRate;
            if (*count < maxP) {
                float mx = x + sW / 2;
                float my = y;
                float dx = playerX - mx;
                float dy = playerY - my;
                float len = sqrt(dx * dx + dy * dy);
                if (len > 0) { dx /= len; dy /= len; }
                pool[(*count)++] = new StraightProjectile(mx, my, dx * 10.0f, dy * 10.0f, 5, false, "25I-0832_25I-0711_Assets/missile.png");
            }
        }
        anim.update(dt);
    }
    void draw(RenderWindow& win, float camX, float camY) override {
        if (destroyed)
            return;
        float sx = x - camX;
        float sy = y - camY;
        float scx = anim.sprite.getScale().x;
        float scy = anim.sprite.getScale().y;
        float absx = scx < 0 ? -scx : scx;
        anim.sprite.setScale(facingRight ? absx : -absx, scy);
        anim.setPosition(sx, sy);
        anim.draw(win);
    }
};
//enem veh manag
class EnemyVehicleManager {
    static const int maxx = 10;
    EnemyVehicle* vehicles[maxx];
    int count;
public:
    EnemyVehicleManager() : count(0) {
        for (int i = 0; i < maxx; i++)
            vehicles[i] = nullptr;
    }
    ~EnemyVehicleManager() {
        for (int i = 0; i < count; i++)
            delete vehicles[i];
    }

    void add(EnemyVehicle* ev) {
        if (count < maxx)
            vehicles[count++] = ev;
    }
    void clearAll() {                    
        for (int i = 0; i < count; i++) {
            delete vehicles[i];
            vehicles[i] = nullptr;
        }
        count = 0;
    }
    void update(float dt, float playerX, float playerY, Projectile** pool, int* pCount, int maxP, char levelMap[][LEVEL_W], int cellSize) {
        for (int i = 0; i < count; i++) {
            if (vehicles[i] && !vehicles[i]->isDestroyed())
                vehicles[i]->update(dt, playerX, playerY, pool, pCount, maxP, levelMap, cellSize);
        }
    }

    //called after resolveEnemyProjectileHits to also damage player
    void checkPlayerCollision(PlayerSoldier* player) {
        for (int i = 0; i < count; i++) {
            if (!vehicles[i] || vehicles[i]->isDestroyed())
                continue;
            EnemyVehicle* ev = vehicles[i];
            bool hit = player->getX() < ev->getX() + 96 && player->getX() + player->getW() > ev->getX() && player->getY() < ev->getY() + 64 && player->getY() + player->getH() > ev->getY();
            if (hit)
                player->takeDamage();
        }
    }

    // player projectiles can destroy enemy vehicles
    void resolvePlayerHits(Projectile** pool, int pCount,
        Explosion** explosions, int& explCount, int maxExpl) {
        for (int i = 0; i < pCount; i++) {
            Projectile* p = pool[i];

            if (!p || !p->isActive() || !p->isFromPlayer())
                continue;

            for (int j = 0; j < count; j++) {
                EnemyVehicle* ev = vehicles[j];
                if (!ev || ev->isDestroyed())
                    continue;

                bool hit = p->getX() < ev->getX() + 96 && p->getX() + 8 > ev->getX() && p->getY() < ev->getY() + 64 && p->getY() + 8 > ev->getY();
                if (hit) {
                    bool destroyed = ev->takeDamage(p->getDamage());
                    p->setActive(false);
                    if (destroyed && explCount < maxExpl)
                        explosions[explCount++] =
                        new Explosion(ev->getX(), ev->getY());
                }
            }
        }
    }

    void draw(RenderWindow& win, float camX, float camY) {
        for (int i = 0; i < count; i++)
            if (vehicles[i] && !vehicles[i]->isDestroyed())
                vehicles[i]->draw(win, camX, camY);
    }
    int getCount() const {
        return count;
    }
    EnemyVehicle* get(int i) {
        return vehicles[i];
    }
};

//THe vehicle MAnager holds all the spawned player vheicles handles keys and tarma features
class VehicleManager {
private:
    static const int max = 4;
    Vehicle* vehicles[max];
    int count;
    Vehicle* piloted;//null if on foot
    bool tarmaImmune;// set by Tarma's powerup
    float tarmaImmuneTimer;
public:
    VehicleManager() : count(0), piloted(nullptr), tarmaImmune(false), tarmaImmuneTimer(0) {
        for (int i = 0; i < max; i++)
            vehicles[i] = nullptr;
    }
    ~VehicleManager() {
        for (int i = 0; i < count; i++)
            delete vehicles[i];
    }

    //spawnin vehicles at level start
    void spawnDefaultVehicles() {

        MetalSlug* slug = new MetalSlug();
        slug->loadSprites();
        slug->setPosition(10 * 64, 23 * 64);

        vehicles[count++] = slug;

        SlugFlyer* flyer = new SlugFlyer();
        flyer->loadSprites();
        flyer->setPosition(90 * 64, 7 * 64);
        vehicles[count++] = flyer;

        SlugMariner* mariner = new SlugMariner();
        mariner->loadSprites();
        mariner->setPosition(155 * 64, 27 * 64);
        vehicles[count++] = mariner;

        AmphibiousSlug* amphi = new AmphibiousSlug();
        amphi->loadSprites();
        amphi->setPosition(136 * 64, 23 * 64);
        vehicles[count++] = amphi;
    }
    //try to enter nearest vehicle F key  nd returns true if player boarded
    bool tryEnter(float playerX, float playerY, bool isTarma) {
        if (piloted)
            return false;  //already in a vehicle
        float bestDist = 400.0f;   //must be within 200px
        Vehicle* best = nullptr;
        for (int i = 0; i < count; i++) {
            if (vehicles[i]->isDestroyed() || vehicles[i]->isActive())
                continue;
            float dx = vehicles[i]->getX() - playerX;
            float dy = vehicles[i]->getY() - playerY;
            float dist = dx * dx + dy * dy;
            if (dist < bestDist * bestDist) {
                bestDist = dist;
                best = vehicles[i];
            }
        }
        if (best) {
            best->setActive(true);
            if (isTarma) best->setDurabilityMult(1.2f);  //Tarma +20% durability
            piloted = best;
            return true;
        }
        return false;
    }
    //exit vehicle F key
    //Returns true if vehicle was destroyed and Tarma survives
    bool tryExit(bool isTarma, bool vehicleJustDestroyed) {
        if (!piloted)
            return false;
        //if vehicle destroyed, Tarma survives so caller justchecks the return value and no kill
        bool tarmaTotemActivated = isTarma && vehicleJustDestroyed;
        piloted->setActive(false);
        piloted = nullptr;
        return tarmaTotemActivated;
    }
    //Tarma immunity for 20s
    void activateTarmaImmunity() {
        tarmaImmune = true;
        tarmaImmuneTimer = 20.0f;
    }
    bool isTarmaImmune() const {

        return tarmaImmune;
    }
    Vehicle* getPiloted()const {
        return piloted;
    }
    bool isInVehicle()const {
        return piloted != nullptr;
    }
    Vehicle* getVehicle(int i)const {
        return (i >= 0 && i < count) ? vehicles[i] : nullptr;
    }
    int getCount()const {
        return count;
    }
    //update all vehicles nd timers 
    void update(float dt, char levelMap[][LEVEL_W], int levelW, int levelH, int cellSize) {
        if (tarmaImmune) {
            tarmaImmuneTimer -= dt;
            if (tarmaImmuneTimer <= 0) tarmaImmune = false;
        }
        for (int i = 0; i < count; i++)
            vehicles[i]->update(dt, levelMap, levelW, levelH, cellSize);
    }

    void handleInput(float speed, bool isTarma) {
        if (!piloted || piloted->isDestroyed())
            return;
        const char* t = piloted->getType();
        if (strcmp(t, "MetalSlug") == 0)
            ((MetalSlug*)piloted)->handleInput(speed, isTarma);
        else if (strcmp(t, "SlugFlyer") == 0)
            ((SlugFlyer*)piloted)->handleInput(speed, isTarma);
        else if (strcmp(t, "SlugMariner") == 0)
            ((SlugMariner*)piloted)->handleInput(speed, isTarma);
        else if (strcmp(t, "AmphibiousSlug") == 0)
            ((AmphibiousSlug*)piloted)->handleInput(speed, isTarma);
    }

    //ts gonna spawns a straight bullet from the vehicle
    void tryFireVehicle(Projectile** pool, int* count, int maxP) {
        if (!piloted || piloted->isDestroyed() || !piloted->canFire())
            return;
        const char* t = piloted->getType();
        float vx = piloted->isFacingRight() ? 20.0f : -20.0f;
        float mx = piloted->getX() + (piloted->isFacingRight() ? 96.f : 0.f);
        float my = piloted->getY() + 32.f;
        if (*count >= maxP)
            return;
        pool[(*count)++] = new StraightProjectile(mx, my, vx, 0, 3, true, "25I-0832_25I-0711_Assets/bullet.png");
        piloted->resetFireCooldown();
    }

    // Missile fire 
    void tryFireMissile(Projectile** pool, int* count, int maxP) {
        if (!piloted || piloted->isDestroyed())
            return;
        const char* t = piloted->getType();
        if (strcmp(t, "SlugFlyer") == 0)
            ((SlugFlyer*)piloted)->fireMissileProjectile(pool, count, maxP);
        else if (strcmp(t, "SlugMariner") == 0)
            ((SlugMariner*)piloted)->fireMissileProjectile(pool, count, maxP);
        else if (strcmp(t, "AmphibiousSlug") == 0) {
            AmphibiousSlug* a = (AmphibiousSlug*)piloted;// reused mariner logic for water, flyer logic for air
            a->fireMissile(); // handles ammo/cooldown internally
            int mode = a->getMode();// spawn projectile based on mode
            float mx = a->getX() + (a->isFacingRight() ? 112.f : 0.f);
            float my = a->getY() + 32.f;
            if (*count >= maxP)
                return;
            if (mode == AmphibiousSlug::air)
                pool[(*count)++] = new BallisticProjectile(mx, my, a->isFacingRight() ? 10.f : -10.f, -8.f, 5, true, 0.3f, "25I-0832_25I-0711_Assets/grenade.png");
            else if (mode == AmphibiousSlug::water)
                pool[(*count)++] = new StraightProjectile(mx, my, 0, -15.f, 5, true, "25I-0832_25I-0711_Assets/bullet.png");
        }
    }

    void draw(RenderWindow& window, float camX, float camY) {
        for (int i = 0; i < count; i++)
            vehicles[i]->draw(window, camX, camY);
    }
    //draw HUD
    void drawVehicleHUD(RenderWindow& window, Font& font) {
        if (!piloted || piloted->isDestroyed())
            return;

        // HP bar background
        RectangleShape bgBar;
        bgBar.setSize({ 200.f, 14.f });
        bgBar.setFillColor(Color(60, 60, 60, 200));
        bgBar.setPosition(14.f, 62.f);
        window.draw(bgBar);
        // HP bar fill
        float ratio = (float)piloted->getHP() / 12.0f; // rough max
        RectangleShape hpBar;
        hpBar.setSize({ 200.f * ratio, 14.f });
        hpBar.setFillColor(Color(50, 200, 80));
        hpBar.setPosition(14.f, 62.f);
        window.draw(hpBar);
        //vehicle name text
        Text vt;
        vt.setFont(font);
        vt.setCharacterSize(15);
        vt.setFillColor(Color::Cyan);
        vt.setString(piloted->getType());
        vt.setPosition(14.f, 78.f);
        window.draw(vt);
        //missile info for flyer/mariner/amphi
        const char* tp = piloted->getType();
        bool showMissile = (tp[4] == 'F' || tp[4] == 'M' || tp[0] == 'A');
        if (showMissile) {
            Text mt;
            mt.setFont(font);
            mt.setCharacterSize(14);
            mt.setFillColor(Color::Yellow);
            mt.setString("Press Space: Missile");
            mt.setPosition(14.f, 96.f);
            window.draw(mt);
        }

        //Tarma immunity indicator
        if (tarmaImmune) {
            Text it;
            it.setFont(font);
            it.setCharacterSize(14);
            it.setFillColor(Color(255, 100, 255));
            it.setString("IMMUNE!");
            it.setPosition(14.f, 114.f);
            window.draw(it);
        }
    }
};