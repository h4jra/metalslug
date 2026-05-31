#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <fstream>
#include <string>
using namespace sf;
using namespace std;
//constants
const int REBEL_SOLDIER = 50;
const int SHIELDED_SOLDIER = 75;
const int BAZOOKA_SOLDIER = 100;
const int GRENADE_SOLDIER = 100;
const int PARATROOPER_BONUS = 25;
const int MUMMY_WARRIOR = 150;
const int ZOMBIE = 100;
const int MARTIAN = 200;
const int BOSS_NORMAL = 500;
const int BOSS_ULTIMATE = 1500;
const int MELEE_KILL = 50;
const int AERIAL_KILL = 100;
const int MULTI_KILL = 200;
const int MASSACRE_BASE = 300;
const int MASSACRE_EXTRA = 50;
const int SURVIVAL_CLEAR = 1000;
const int CAMPAIGN_CLEAR = 3000;
const int FLAWLESS_VICTORY = 5000;
//LEADERBOARD 
struct LeaderboardEntry {
    char name[16];
    int  score;
};
const int   MAX_LEADERBOARD = 10;
const char* const LEADERBOARD_FILE = "highscores.txt";
//FLOATING POPUP
struct ScorePopup {
    Text label;
    float baseY;
    float timer;
    bool  active;
    ScorePopup() : baseY(0), timer(0), active(false) {}
};

//SCREEN Loads leaderboard  Draws Text rows over the black board area of the image.
class LeaderboardScreen {
private:
    bool visible;
    float openTimer;
    Texture bgTex;
    Sprite bgSprite;
    bool bgLoaded;
    Texture coinTex;
    Sprite coinSprite;
    bool coinLoaded;
    Font font;
    bool fontLoaded;
    float boardX, boardY, boardW, boardH;
    float rowH;
    Text nameTexts[MAX_LEADERBOARD];
    Text scoreTexts[MAX_LEADERBOARD];
    Text emptyText;
    Text backHint;
    //helpers
    static string zeroPad(int v, int w = 7) {
        string s =to_string(v);
        while ((int)s.size() < w) s = "0" + s;
        return s;
    }
    bool tryLoadFont() {
        const char* paths[] = {"arial.ttf", "fonts/arial.ttf", "../arial.ttf","../../arial.ttf", "../../fonts/arial.ttf",
"C:/Windows/Fonts/arial.ttf","C:/Windows/Fonts/Arial.ttf", nullptr
        };
        for (int i = 0; paths[i]; i++)
            if (font.loadFromFile(paths[i]))
                return true;
        return false;
    }
public:
    LeaderboardScreen(): visible(false), openTimer(0),bgLoaded(false), coinLoaded(false), fontLoaded(false),
boardX(0), boardY(0), boardW(0), boardH(0), rowH(0){
    }
    void init(float screenW, float screenH) {
        fontLoaded = tryLoadFont();
//load background image
        bgLoaded = bgTex.loadFromFile("25I-0832_25I-0711_Assets/leaderboard.png");
        if (!bgLoaded)
            bgLoaded = bgTex.loadFromFile("leaderboard.png");
        if (bgLoaded) {
            float imgW = (float)bgTex.getSize().x;
            float imgH = (float)bgTex.getSize().y;
            float targetW = screenW * 0.50f;
            float scale = targetW / imgW;
            float drawnW = imgW * scale;
            float drawnH = imgH * scale;
            //centre on screen
            float originX = (screenW - drawnW) / 2.0f;
            float originY = (screenH - drawnH) / 2.0f;
            bgSprite.setTexture(bgTex);
            bgSprite.setScale(scale, scale);
            bgSprite.setPosition(originX, originY);
            boardX = originX + drawnW * 0.08f;
            boardY = originY + drawnH * 0.48f;
            boardW = drawnW * 0.84f;
            boardH = drawnH * 0.48f;
            rowH = boardH / (float)MAX_LEADERBOARD;
        }
        //coinLoaded = coinTex.loadFromFile("Sprites/coin.png");
        //if (!coinLoaded)
        //    coinLoaded = coinTex.loadFromFile("coin.png");
        //if (coinLoaded)
        //    coinSprite.setTexture(coinTex);

        //if (!fontLoaded)
        // return;
        //row text
        for (int i = 0; i < MAX_LEADERBOARD; i++) {
            float ry = boardY + i * rowH - rowH * 0.15f;
            nameTexts[i].setFont(font);
            nameTexts[i].setCharacterSize(17);
            nameTexts[i].setFillColor(Color::White);
            nameTexts[i].setString("---");
            nameTexts[i].setPosition(boardX + 60, ry);
            scoreTexts[i].setFont(font);
            scoreTexts[i].setCharacterSize(17);
            scoreTexts[i].setFillColor(Color(255, 220, 50));
            scoreTexts[i].setString("0000000");
            scoreTexts[i].setPosition(boardX + boardW - 180, ry);
        }
        emptyText.setFont(font);
        emptyText.setCharacterSize(20);
        emptyText.setFillColor(Color(200, 200, 200));
        emptyText.setString("No scores yet! Go earn some!");
        emptyText.setPosition(boardX + 20, boardY + boardH * 0.4f);
        backHint.setFont(font);
        backHint.setCharacterSize(15);
        backHint.setFillColor(Color(200, 200, 200));
        backHint.setString("[ 3 ] or [ ESC ] to return");
        backHint.setPosition(boardX + boardW * 0.25f, boardY + boardH + 6);
    }
    void refreshRows(LeaderboardEntry* entries, int count) {
        if (!fontLoaded)
            return;
        for (int i = 0; i < MAX_LEADERBOARD; i++) {
            if (i < count) {
                string rankStr = std::to_string(i + 1);
                rankStr += ".  ";
                rankStr += entries[i].name;
                nameTexts[i].setString(rankStr);
                scoreTexts[i].setString(zeroPad(entries[i].score));
                if (i == 0) 
                    nameTexts[i].setFillColor(Color(255, 215, 0));
                else if (i == 1)
                    nameTexts[i].setFillColor(Color(210, 210, 210));
                else if (i == 2) 
                    nameTexts[i].setFillColor(Color(205, 127, 50));
                else         
                    nameTexts[i].setFillColor(Color::White);
            }
            else {
                nameTexts[i].setString("");
                scoreTexts[i].setString("");
            }
        }
    }
    void open() { 
        visible = true;  
        openTimer = 0;
    }
    void close() {
        visible = false; 
        openTimer = 0;
    }
    bool isVisible() const { 
        return visible; 
    }
    void update(float dt) {
        if (!visible)
            return;
        openTimer += dt;
    }
    void draw(RenderWindow& window, LeaderboardEntry* entries, int count) {
        if (!visible)
            return;
        if (bgLoaded)
            window.draw(bgSprite);
        if (!fontLoaded)
            return;
        if (count == 0) {
            window.draw(emptyText);
        }
        else {
            for (int i = 0; i < count && i < MAX_LEADERBOARD; i++) {
                window.draw(nameTexts[i]);
                window.draw(scoreTexts[i]);
            }
        }
        window.draw(backHint);
    }
};
//SCORE MANAGER
class ScoreManager {
private:
    int currentScore;
    int highScore;
    LeaderboardEntry entries[MAX_LEADERBOARD];
    int entryCount;
    Font font;
    bool fontLoaded;
    Text scoreText;
    Text hiText;
    Text levelText;
    Text livesText;
    ScorePopup popup;
    int currentLevel;
    LeaderboardScreen lbScreen;
    static string pad(int v, int w = 7) {
        string s = std::to_string(v);
        while ((int)s.size() < w)
            s = "0" + s;
        return s;
    }
    bool tryLoadFont() {
        const char* paths[] = {"arial.ttf", "fonts/arial.ttf", "../../arial.ttf",
"../../fonts/arial.ttf", "../arial.ttf","C:/Windows/Fonts/arial.ttf","C:/Windows/Fonts/Arial.ttf", nullptr
        };
for (int i = 0; paths[i]; i++)
if (font.loadFromFile(paths[i])) 
return true;
        return false;
    }
    void refreshScore() {
        string s = "SCORE: "; 
        s += pad(currentScore);
        scoreText.setString(s);
       string h = "HI:    ";
       h += pad(highScore);
        hiText.setString(h);
    }
    void applyFont(Text& t, unsigned int sz, Color c, float x, float y) {
        t.setFont(font);
        t.setCharacterSize(sz);
        t.setFillColor(c);
        t.setPosition(x, y);
    }
    void showPopup(int value, float sx, float sy) {
        popup.baseY = sy;
        popup.timer = 1.4f;
        popup.active = true;
        std::string s = "+"; s += std::to_string(value);
        popup.label.setString(s);
        popup.label.setPosition(sx, sy);
        if (value >= 500)
            popup.label.setFillColor(Color(255, 215, 0));
        else if (value >= 200)
            popup.label.setFillColor(Color(255, 120, 0));
        else
            popup.label.setFillColor(Color(255, 255, 80));
    }
public:
    ScoreManager(): currentScore(0), highScore(0),entryCount(0), fontLoaded(false), currentLevel(1){
        for (int i = 0; i < MAX_LEADERBOARD; i++) {
            entries[i].name[0] = '\0';
            entries[i].score = 0;
        }
        fontLoaded = tryLoadFont();
        applyFont(scoreText, 26, Color::White, 14, 8);
        applyFont(hiText, 22, Color(190, 190, 190), 14, 40);
        applyFont(levelText, 26, Color(200, 220, 255), 700, 8);
        applyFont(livesText, 26, Color(100, 255, 100), 1370, 8);
        applyFont(popup.label, 30, Color::Yellow, 0, 0);
        refreshScore();
        levelText.setString("LVL 1");
        livesText.setString("LIVES: 2");
        loadLeaderboard();
    }
    // once after the RenderWindow is created
    void initLeaderboard(float screenW, float screenH) {
        lbScreen.init(screenW, screenH);
        lbScreen.refreshRows(entries, entryCount);
    }
    int  getCurrentScore() const { 
        return currentScore;
    }
    int  getHighScore() const {
        return highScore;
    }
    bool isFontLoaded() const {
        return fontLoaded;
    }
    void openLeaderboard() {
        lbScreen.open(); 
    }
    void closeLeaderboard() { 
        lbScreen.close();
    }
    bool isLeaderboardOpen() const {
        return lbScreen.isVisible();
    }
    void toggleLeaderboard() {
        if (lbScreen.isVisible())
            lbScreen.close();
        else            
            lbScreen.open();
    }
    void setLevel(int lv) {
        currentLevel = lv;
       string s = "LVL "; 
       s += to_string(lv);
        levelText.setString(s);
    }
    void updateLives(int lives) {
     string s = "LIVES: "; 
     s += to_string(lives);
        livesText.setString(s);
    }
    void addPoints(int pts, float screenX = -1, float screenY = -1) {
        currentScore += pts;
        if (currentScore > highScore) 
            highScore = currentScore;
        refreshScore();
        if (screenX >= 0) 
            showPopup(pts, screenX, screenY);
    }
    int awardEnemyKill(int baseScore,bool wasAerialKill, bool wasMeleeKill,float screenX, float screenY) {
        int total = baseScore;
        if (wasMeleeKill) 
            total += MELEE_KILL;
        if (wasAerialKill) 
            total += AERIAL_KILL;
        addPoints(total, screenX, screenY);
        return total;
    }
    void awardExplosionBonus(int killCount, float screenX, float screenY) {
        if (killCount < 2)
            return;
        int bonus = (killCount == 2)? MULTI_KILL: MASSACRE_BASE + (killCount - 3) * MASSACRE_EXTRA;
        addPoints(bonus, screenX, screenY);
    }
    void awardSurvivalClear(bool flawless) {
        addPoints(SURVIVAL_CLEAR);
        if (flawless)
            addPoints(FLAWLESS_VICTORY);
    }
    void awardCampaignClear(bool flawless) {
        addPoints(CAMPAIGN_CLEAR);
        if (flawless)
            addPoints(FLAWLESS_VICTORY);
    }
    void awardBossKill(bool isUltimateBoss, float screenX, float screenY) {
        addPoints(isUltimateBoss ? BOSS_ULTIMATE : BOSS_NORMAL, screenX, screenY);
    }
    void reset() {
        currentScore = 0;
        popup.active = false;
        refreshScore();
        setLevel(1);
    }
    void update(float dt) {
        if (popup.active) {
            popup.timer -= dt;
            if (popup.timer <= 0.0f)
                popup.active = false;
            
            else {
                float elapsed = 1.4f - popup.timer;
                float newY = popup.baseY - elapsed * 40.0f;
                popup.label.setPosition(popup.label.getPosition().x, newY);
                if (popup.timer < 0.4f) {
                    int alpha = (int)(popup.timer / 0.4f * 255.0f);
                    if (alpha < 0)  
                        alpha = 0;
                    if (alpha > 255)
                        alpha = 255;
                    Color c = popup.label.getFillColor();
                    c.a = (Uint8)alpha;
                    popup.label.setFillColor(c);
                }
            }
        }
        lbScreen.update(dt);
    }
    void drawHUD(RenderWindow& window) {
        if (!fontLoaded) 
            return;
        window.draw(scoreText);
        window.draw(hiText);
        window.draw(levelText);
        window.draw(livesText);
        if (popup.active)
            window.draw(popup.label);
    }

    void drawLeaderboardScreen(RenderWindow& window) {
        lbScreen.draw(window, entries, entryCount);
    }
    void drawLeaderboard(RenderWindow& window, float startX, float startY) {
        if (!fontLoaded)
            return;
        Text t;
        t.setFont(font);
        t.setCharacterSize(18);
        t.setFillColor(Color(160, 160, 160));
        t.setString("Press 3 for leaderboard");
        t.setPosition(startX, startY);
        window.draw(t);
    }
    void loadLeaderboard() {
        entryCount = 0;
        std::ifstream f(LEADERBOARD_FILE);
        if (!f.is_open()) 
            return;
        while (entryCount < MAX_LEADERBOARD) {
            LeaderboardEntry e;
            if (!(f >> e.name >> e.score)) 
                break;
            entries[entryCount++] = e;
        }
        lbScreen.refreshRows(entries, entryCount);
    }

    void saveLeaderboard() {
        ofstream f(LEADERBOARD_FILE);
        if (!f.is_open()) 
            return;
        for (int i = 0; i < entryCount; i++)
            f << entries[i].name << " " << entries[i].score << "\n";
    }
    bool tryInsertScore(const char* playerName) {
        if (currentScore == 0)
            return false;
        int pos = entryCount;
        for (int i = 0; i < entryCount; i++) {
            if (currentScore > entries[i].score) {
                pos = i;
                break;
            }
        }
        if (pos >= MAX_LEADERBOARD) 
            return false;
        int last = (entryCount < MAX_LEADERBOARD) ? entryCount : MAX_LEADERBOARD - 1;
        for (int i = last; i > pos; i--) 
            entries[i] = entries[i - 1];
        int n = 0;
        while (n < 15 && playerName[n] != '\0') {
            entries[pos].name[n] = playerName[n]; n++;
        }
        entries[pos].name[n] = '\0';
        entries[pos].score = currentScore;
        if (entryCount < MAX_LEADERBOARD) 
            entryCount++;
        saveLeaderboard();
        lbScreen.refreshRows(entries, entryCount);
        return true;
    }
    void drawScoreSummary(RenderWindow& window, float cx, float cy) {
        if (!fontLoaded) 
            return;
        Text header;
        header.setFont(font);
        header.setString("GAME OVER");
        header.setCharacterSize(40);
        header.setFillColor(Color(255, 60, 60));
        header.setPosition(cx - 100.0f, cy - 80.0f);
        window.draw(header);
        Text sc;
        sc.setFont(font);
        sc.setCharacterSize(28);
        sc.setFillColor(Color::White);
        std::string s = "SCORE: "; s += pad(currentScore);
        sc.setString(s);
        sc.setPosition(cx - 100.0f, cy - 20.0f);
        window.draw(sc);
        Text hi;
        hi.setFont(font);
        hi.setCharacterSize(24);
        hi.setFillColor(Color(200, 200, 200));
       string h = "BEST:  "; h += pad(highScore);
        hi.setString(h);
        hi.setPosition(cx - 100.0f, cy + 20.0f);
        window.draw(hi);
    }
};
struct ExplosionKillResult {
    int killCount;
    float centreScreenX;
    float centreScreenY;
};