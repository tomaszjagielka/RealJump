#include <list>
#include <iostream>
#include <string>

#include "Framework.h"

struct Dimension {
    float x;
    float y;

    Dimension() : x(0), y(0) {}
    Dimension(float x, float y) : x(x), y(y) {}

    // Subtraction.
    Dimension operator-(const Dimension& other) const {
        return Dimension(x - other.x, y - other.y);
    }

    // In-place scalar division.
    Dimension& operator/=(float scalar) {
        x /= scalar;
        y /= scalar;
        return *this;
    }
};


class MySprite {
    std::string spritePath;

public:
    Sprite* sprite;
    Dimension size;

    MySprite(const char* path) {
        sprite = createSprite(path);
        int w, h;
        getSpriteSize(sprite, w, h);
        size = Dimension(w, h);
        spritePath = path;
    }

    // Used for object copying.
    //MySprite(const MySprite& other) {
    //    spritePath = other.spritePath;
    //    sprite = createSprite(other.spritePath);
    //    int w, h;
    //    getSpriteSize(sprite, w, h);
    //    size = Dimension(w, h);
    //}

    void Draw(int x, int y) {
        drawSprite(sprite, x, y);
    }

    ~MySprite() {
        std::cout << spritePath << " destroyed" << std::endl;
        destroySprite(sprite);
    }
};

enum class Direction {
    NONE,
    LEFT,
    RIGHT
};

class Entity {
protected:
    void Draw(MySprite* sprite) {
        if (sprite)
            sprite->Draw(position.x, position.y);
    }

public:
    MySprite** sprites;
    int numSprites;
    Dimension position;
    int drawnSpriteIndex = 0;

    Entity(MySprite** sprites, int numSprites, Dimension position)
        : sprites(sprites), numSprites(numSprites), position(position) {}

    ~Entity() {
        for (int i = 0; i < numSprites; i++) {
            if (sprites[i])
                delete sprites[i];
        }
        delete[] sprites;
    }

    //virtual void Update(Dimension windowSize, std::list<Entity*> objects) = 0;
};

enum ObjectType {
    DEFAULT,
    JUMP,
    JUMP_BOOST,
    JETPACK
};

class Object : public Entity {
public:
    ObjectType objectType;
    Object(MySprite** sprites, int numSprites, Dimension position, ObjectType objectType)
        : Entity(sprites, numSprites, position), objectType(objectType) {}

    void Update() {
        Draw(sprites[drawnSpriteIndex]);
    }
};

enum Collision {
    NONE,
    TOP,
    OTHER,
};

class Player : public Entity {
    bool onPlatform = false;
    bool isVulnerable = true;
    bool isFalling = false;

    void Jump(Object*& object) {
        switch (object->objectType) {
        case ObjectType::JUMP:
            velocity -= 3;
			break;
        case ObjectType::JUMP_BOOST:
            if (object->drawnSpriteIndex == 0)
                velocity -= 3;
            else
                velocity -= 6;
            break;
        case ObjectType::JETPACK:
            velocity -= 55;
            break;
        }

        // TODO:
        // Check if it takes effect.
        //isFalling = false;
    }

    void Jump() {
        velocity -= 3;

        // TODO:
        // Check if it takes effect.
        //isFalling = false;
    }

    bool collidesWithEntity(const Entity* entity) {
        if (position.y + sprites[0]->size.y > entity->position.y && // Player's feet are below the top of the entity.
            position.y < entity->position.y && // Player's head is above the entity.
            position.x + sprites[0]->size.x > entity->position.x && // Player is to the right of the entity.
            position.x < entity->position.x + entity->sprites[0]->size.x) // Player is to the left of the entity.
        {
            return true;
        }
        return false;
    }

    bool CollidesWithTopOf(Entity* other) const {
        if (position.y + sprites[0]->size.y < other->position.y) {
            return false; // This entity is above the other entity.
        }

        if (position.y + sprites[0]->size.y - 5 > other->position.y + other->sprites[0]->size.y) {
            return false; // This entity is not close enough to the top of the other entity.
        }

        if (position.x + sprites[0]->size.x < other->position.x) {
            return false; // This entity is to the left of the other entity.
        }

        if (position.x > other->position.x + other->sprites[0]->size.x) {
            return false; // This entity is to the right of the other entity.
        }

        return true;
    }

    Collision collidesWithObject(Entity* enemy) const {
        if (position.y + sprites[0]->size.y < enemy->position.y) {
            return Collision::NONE; // Player is above the enemy
        }

        if (position.y > enemy->position.y + enemy->sprites[0]->size.y) {
            return Collision::NONE; // Player is below the enemy
        }

        if (position.x + sprites[0]->size.x < enemy->position.x) {
            return Collision::NONE; // Player is to the left of the enemy
        }

        if (position.x > enemy->position.x + enemy->sprites[0]->size.x) {
            return Collision::NONE; // Player is to the right of the enemy
        }

        // Check if player is on top of enemy
        if (velocity > 0 && CollidesWithTopOf(enemy)) {
            return Collision::TOP; // Player is on top of enemy, but we've already removed it so no collision
        }

        return Collision::OTHER;
    }

public:
    Direction lastMoveDirection = Direction::RIGHT;
    Direction moveDirection = Direction::NONE;
    float velocity = 0;
    bool maxHeightCapped = false;
    int lives = 5;
    bool gameOver = false;
    int distance = 0;
    int platfromCount = 0;
    bool lastFalling = false;
    int jumpingTicks = 0;
    int shootingTicks = 0;

    Player(MySprite** sprites, int numSprites, Dimension position)
        : Entity(sprites, numSprites, position) {}

    void Update(Dimension windowSize, std::list<Entity*>& objects, std::list<Entity*>& enemies) {
        // FLAGS
        const float gravity = 0.0125;
        float lastYPosition = position.y;

        velocity += gravity;
        position.y += velocity;

        isFalling = velocity > 0;

        if (isFalling)
            isVulnerable = true;

        if (position.y < windowSize.y / 2 - this->sprites[0]->size.y / 2) {
            position.y = windowSize.y / 2 - this->sprites[0]->size.y / 2;
            maxHeightCapped = true;
        }
        else maxHeightCapped = false;

        // HANDLE LIFES
        if (lives > 0 && position.y > windowSize.y - this->sprites[0]->size.y / 2) {
            Dimension lowestPlatform = Dimension(0, 0);

            for (Entity* object : objects) {
                if (object->position.y > lowestPlatform.y) {
                    lowestPlatform = object->position;
                }
            }

            position.x = lowestPlatform.x + this->sprites[0]->size.x / 4;
            position.y = lowestPlatform.y - this->sprites[0]->size.y;
            lives--;

            velocity = -1;
            isVulnerable = false;

            std::cout << "Lives left: " << lives << std::endl;
        }

        gameOver = lives < 1;

        int yPositionDelta = position.y - lastYPosition;
        if (yPositionDelta > 0)
            distance += yPositionDelta;

        // COLLISIONS
        bool collidedWithEnemy = false;
        auto it = enemies.begin();

        while (it != enemies.end()) {
            Collision collision = collidesWithObject(*it);

            if (isVulnerable && collision == Collision::OTHER) {
                collidedWithEnemy = true;
                Dimension lowestPlatform = Dimension(0, 0);

                for (Entity* object : objects) {
                    if (object->position.y > lowestPlatform.y) {
                        lowestPlatform = object->position;
                    }
                }

                position.x = lowestPlatform.x + this->sprites[0]->size.x / 4;
                position.y = lowestPlatform.y - this->sprites[0]->size.y;

                lives--;
                std::cout << "Lives left: " << lives << std::endl;

                velocity = -1;
                isVulnerable = false;

                break;
            }
            else if (collision == Collision::TOP) {
                // Remove enemy
                it = enemies.erase(it);
                Jump();
            }
            else {
                ++it;
            }
        }

        bool collidedWithObject = false;
        if (isFalling) {
            for (const auto& object : objects) {
                if (collidesWithEntity(object)) {
                    velocity = 0;
                    position.y -= 0.25;

                    if (!onPlatform) {
                        platfromCount++;
                        onPlatform = true;
                    }


                    Object* obj = (Object*)object;

                    if (obj->objectType == ObjectType::JETPACK) {
                        // TODO:
                        // Delete the Jetpack object properly? Is it not a proper way?
                        obj->position.y = windowSize.y + 1;
                        isVulnerable = false;
                    }
                    else if (obj->objectType == ObjectType::JUMP_BOOST && obj->drawnSpriteIndex == 0) {
                        obj->position.y += obj->sprites[0]->size.y - obj->sprites[1]->size.y;
                        obj->drawnSpriteIndex = 1;
                    }

                    Jump(obj);

                    jumpingTicks = 150;
                    collidedWithObject = true;

                    break;
                }
            }
        }

        if (!collidedWithObject && !collidedWithEnemy) {
            onPlatform = false;
        }

        // MOVEMENT & SPRITES
        if (shootingTicks && jumpingTicks) {
            Draw(sprites[6]);
        }
        else if (shootingTicks) {
            Draw(sprites[5]);
        }

        switch (moveDirection) {
        case Direction::RIGHT:
            position.x += 1;
            lastMoveDirection = moveDirection;

            if (position.x > windowSize.x) {
                position.x = -sprites[0]->size.x;
            }

            if (jumpingTicks && shootingTicks)
                Draw(sprites[6]);
            else if (shootingTicks)
                Draw(sprites[5]);
            else if (jumpingTicks)
                Draw(sprites[2]);
            else
                Draw(sprites[0]);
            break;
        case Direction::LEFT:
            position.x -= 1;
            lastMoveDirection = moveDirection;

            if (position.x < -sprites[3]->size.x) {
                position.x = windowSize.x;
            }

            if (jumpingTicks && shootingTicks)
                Draw(sprites[6]);
            else if (shootingTicks)
                Draw(sprites[5]);
            else if (jumpingTicks)
                Draw(sprites[3]);
            else
                Draw(sprites[1]);
            break;
        case Direction::NONE:
            switch (lastMoveDirection) {
            case Direction::LEFT:
                if (jumpingTicks && shootingTicks)
                    Draw(sprites[6]);
                else if (shootingTicks)
                    Draw(sprites[5]);
                else if (jumpingTicks)
                    Draw(sprites[3]);
                else
                    Draw(sprites[1]);

                break;
            case Direction::RIGHT:
                if (jumpingTicks && shootingTicks)
                    Draw(sprites[6]);
                else if (shootingTicks)
                    Draw(sprites[5]);
                else if (jumpingTicks)
                    Draw(sprites[2]);
                else
                    Draw(sprites[0]);
                break;
            }
            break;
        }

        if (jumpingTicks > 0)
            jumpingTicks--;
        if (shootingTicks > 0)
            shootingTicks--;
    }

    void Reset() {
        this->position = position;
        this->velocity = 0;
        this->moveDirection = Direction::NONE;
        this->lastMoveDirection = Direction::RIGHT;
        this->maxHeightCapped = false;
        this->lives = 5;
        this->gameOver = false;
        this->distance = 0;
        this->platfromCount = 0;
        this->onPlatform = false;
        this->isVulnerable = true;
        this->jumpingTicks = 0;
        this->shootingTicks = 0;
    }
};

class Enemy : public Entity {
    Direction lastUsedDirection = Direction::RIGHT;

public:
    Enemy(MySprite** sprites, int numSprites, Dimension position)
        : Entity(sprites, numSprites, position) {}

    void Update(Dimension windowSize) {
        Draw(sprites[0]);
    }
};

class Projectile : public Entity {
    Direction lastUsedDirection = Direction::RIGHT;
    float speed = 3.f;
    Dimension direction;

    Collision collidesWithObject(Entity* object) const {
        if (position.y + sprites[0]->size.y < object->position.y) {
            return Collision::NONE; // Player is above the enemy.
        }

        if (position.y > object->position.y + object->sprites[0]->size.y) {
            return Collision::NONE; // Player is below the enemy.
        }

        if (position.x + sprites[0]->size.x < object->position.x) {
            return Collision::NONE; // Player is to the left of the enemy.
        }

        if (position.x > object->position.x + object->sprites[0]->size.x) {
            return Collision::NONE; // Player is to the right of the enemy.
        }

        return Collision::OTHER;
    }

public:
    Projectile(MySprite** sprites, int numSprites, Dimension position, Dimension cursorPosition)
        : Entity(sprites, numSprites, position)
    {
        // Calculate direction towards cursor.
        direction = cursorPosition - position;
        float length = sqrt(direction.x * direction.x + direction.y * direction.y);
        direction /= length; // Normalize direction vector.
    }

    void Update(Dimension windowSize, std::list<Entity*>& enemies) {
        // Move projectile in direction towards cursor.
        position.x += direction.x * speed;
        position.y += direction.y * speed;

        // Check if projectile has gone off the screen.
        if (position.x < 0) {
            position.x = windowSize.x;
        }
        else if (position.x > windowSize.x) {
            position.x = 0;
        }

        bool collidedWithEnemy = false;
        auto it = enemies.begin();

        while (it != enemies.end()) {
            Collision collision = collidesWithObject(*it);

            if (collision == Collision::OTHER) {
                Entity* ent = (Entity*)*it;
                // TODO:
                // Delete these objects properly? Is it not a proper way?
                ent->position.y = windowSize.y + 100;
                //it = enemies.erase(it);
                this->position.y = windowSize.y + 1;
            }
            else {
                ++it;
            }
        }

        Draw(sprites[0]);
    }
};

class MyFramework : public Framework {
    Dimension windowSize;
    MySprite* backgroundSprite;
    Player* player;
    std::list<Entity*> objects;
    std::list<Enemy*> enemies;
    std::list<Projectile*> projectiles;
    Dimension mousePosition;

    void PreInit(int& width, int& height, bool& fullscreen) override
    {
        width = windowSize.x;
        height = windowSize.y;
        fullscreen = false;
    }

    void InitPlatforms() {
        objects.push_back(new Object(new MySprite * [1] {new MySprite("data/game-tiles-green-platform-clipped@2x.png")}, 1, Dimension(player->position.x - player->sprites[0]->size.x / 4, player->position.y + player->sprites[0]->size.y), ObjectType::JUMP));
        int platformCount = 5;

        for (int i = 0; i < platformCount; i++) {
            int maxX = windowSize.x - objects.front()->sprites[0]->size.x;
            int minX = 0;
            int randomX = rand() % (maxX - minX + 1) + minX;

            int maxY = 50;
            int minY = 0;
            int randomY = rand() % (maxY - minY + 1) + minY;

            randomY = 200 * i + randomY;
            randomX = std::min(randomX, int(windowSize.x));

            Dimension randomDimension(randomX, randomY);
            objects.push_back(
                new Object(new MySprite * [1] {new MySprite("data/game-tiles-green-platform-clipped@2x.png")}, 1, randomDimension, ObjectType::JUMP));
        }
    }

    // return : true - ok, false - failed, application will exit
    bool Init() {
        srand(time(0));

        backgroundSprite = new MySprite("data/bck@2x.png");
        player = new Player(
            new MySprite * [7] {new MySprite("data/lik-right-clipped@2x.png"),
            new MySprite("data/lik-left-clipped@2x.png"),
            new MySprite("data/lik-right-odskok-clipped@2x.png"),
            new MySprite("data/lik-left-odskok-clipped@2x.png"),
            new MySprite("data/lik-right-odskok-clipped@2x.png"),
            new MySprite("data/lik-puca-clipped@2x.png"),
            new MySprite("data/lik-puca-odskok-clipped@2x.png")
            },
            3,
            Dimension(windowSize.x / 2, windowSize.y / 2));

        std::cout << "Lives left: " << player->lives << std::endl;

        InitPlatforms();

        return true;
    }

    void Close() {
        delete backgroundSprite;
        delete player;
    }

    // return value: if true will exit the application
    bool Tick() {
        for (int height = 0; height < windowSize.y; height += backgroundSprite->size.y) {
            for (int width = 0; width < windowSize.x; width += backgroundSprite->size.x) {
                backgroundSprite->Draw(width, height);
            }
        }

        bool objectExists = false;
        for (auto it = objects.begin(); it != objects.end(); ) {
            Object* object = (Object*)*it;

            if (object->position.y < 0) {
                objectExists = true;
            }

            if (player->velocity < 0 && player->maxHeightCapped) {
                object->position.y -= player->velocity;
            }

            if (object->position.y > windowSize.y) {
                delete object;
                it = objects.erase(it);
            }
            else {
                object->Update();
                ++it;
            }
        }

        for (auto it = enemies.begin(); it != enemies.end(); ) {
            Enemy* enemy = *it;

            if (player->velocity < 0 && player->maxHeightCapped) {
                enemy->position.y -= player->velocity;
            }

            if (enemy->position.y > windowSize.y) {
                delete enemy;
                it = enemies.erase(it);
            }
            else {
                enemy->Update(windowSize);
                ++it;
            }
        }

        for (auto it = projectiles.begin(); it != projectiles.end(); ) {
            Projectile* projectile = *it;

            if (player->velocity < 0 && player->maxHeightCapped) {
                projectile->position.y -= player->velocity;
            }

            if (projectile->position.y > windowSize.y || projectile->position.y < 0) {
                delete projectile;
                it = projectiles.erase(it);
            }
            else {
                projectile->Update(windowSize, reinterpret_cast<std::list<Entity*>&>(enemies));
                ++it;
            }
        }

        player->Update(windowSize, objects, reinterpret_cast<std::list<Entity*>&>(enemies));

        if (!objectExists) {
            int maxX = windowSize.x - objects.front()->sprites[0]->size.x;
            int minX = 0;
            int randomX = rand() % (maxX - minX + 1) + minX;

            int maxY = 0;
            int minY = -300;
            int randomY = rand() % (maxY - minY + 1) + minY;

            randomY = std::max(randomY, -450);
            randomX = std::min(randomX, int(windowSize.x));
            Dimension randomDimension(randomX, randomY);

            objects.push_back(new Object(new MySprite * [1] {new MySprite("data/game-tiles-green-platform-clipped@2x.png")}, 1, randomDimension, ObjectType::JUMP));

            int platformWidth = objects.back()->sprites[0]->size.x;
            int platformCenterX = randomX + platformWidth / 2;

            if (rand() % 100 < 15) {
                const int numOfEnemies = 12;
                std::string enemySpriteFilenames[numOfEnemies];

                for (int i = 0; i < numOfEnemies; i++) {
                    enemySpriteFilenames[i] = ("data/game-tiles-enemy" + std::to_string(i) + "-clipped@2x.png");
                }

                const char* enemySpriteFilename = enemySpriteFilenames[rand() % numOfEnemies].c_str();
                MySprite* enemySprite = new MySprite(enemySpriteFilename);

                int enemyX = platformCenterX - enemySprite->size.x / 2;
                int enemyY = randomY - enemySprite->size.y;
                Dimension enemyDimension(enemyX, enemyY);

                MySprite** spriteArray = new MySprite * [1] { enemySprite };
                Enemy* newEnemy = new Enemy(spriteArray, 1, enemyDimension);

                enemies.push_back(newEnemy);
            }
            else if (rand() % 100 < 10) {
                MySprite* untensionedSpringSprite = new MySprite("data/game-tiles-untensioned-spring-clipped@2x.png");
                MySprite* tensionedSpringSprite = new MySprite("data/game-tiles-tensioned-spring-clipped@2x.png");

                int springX = platformCenterX - untensionedSpringSprite->size.x / 2;
                int springY = randomY - untensionedSpringSprite->size.y;
                Dimension springDimension(springX, springY);

                MySprite** spriteArray = new MySprite * [2] { untensionedSpringSprite, tensionedSpringSprite };
                Object* newSpring = new Object(spriteArray, 2, springDimension, ObjectType::JUMP_BOOST);

                objects.push_back(newSpring);
            }
            else if (rand() % 100 < 5) {
                MySprite* jetpackSprite = new MySprite("data/game-tiles-jetpack-clipped@2x.png");

                int jetpackX = platformCenterX - jetpackSprite->size.x / 2;
                int jetpackY = randomY - jetpackSprite->size.y;
                Dimension jetpackDimension(jetpackX, jetpackY);

                MySprite** spriteArray = new MySprite * [1] { jetpackSprite };
                objects.push_back(new Object(spriteArray, 1, jetpackDimension, ObjectType::JETPACK));
            }
        }

        if (player->gameOver) {
            std::cout << "GAME OVER! SCORE:" << std::endl;
            std::cout << "Distance: " << player->distance << std::endl;
            std::cout << "Platform count: " << player->platfromCount << std::endl;

            player->Reset();

            for (auto& object : objects) {
                delete object;
            }
            for (auto& enemy : enemies) {
                delete enemy;
            }
            for (auto& projectile : projectiles) {
                delete projectile;
            }

            objects.clear();
            enemies.clear();
            projectiles.clear();

            InitPlatforms();
        }

        return false;
    }

    // param: xrel, yrel: The relative motion in the X/Y direction 
    // param: x, y : coordinate, relative to window
    void onMouseMove(int x, int y, int xrelative, int yrelative) {
        mousePosition = Dimension(x, y);
    }

    void onMouseButtonClick(FRMouseButton button, bool isReleased) {
        if (!isReleased) {
            projectiles.push_back(new Projectile(new MySprite * [1] {new MySprite("data/projectile-tiles0-clipped@2x.png")},
                1,
                Dimension(player->position.x + player->sprites[0]->size.x / 4, player->position.y),
                mousePosition));
        }

        player->shootingTicks = 150;
    }

    void onKeyPressed(FRKey k) {
        switch (k) {
        case FRKey::RIGHT:
            player->moveDirection = Direction::RIGHT;
            break;
        case FRKey::LEFT:
            player->moveDirection = Direction::LEFT;
            break;
        }
    }

    void onKeyReleased(FRKey k) {
        if (k == FRKey::RIGHT || k == FRKey::LEFT)
            player->moveDirection = Direction::NONE;
    }

    const char* GetTitle() {
        return "RealJump";
    }

public:
    MyFramework(int width, int height) : windowSize(width, height) {}
};

int main(int argc, char *argv[])
{
    int width = 800, height = 1000;

    if (argc < 2)
        std::cerr << "Usage: " << argv[0] << " <windowSize>\n";
    else {
        std::string windowSize(argv[1]);
        size_t xPos = windowSize.find('x');

        if (xPos == std::string::npos) {
            std::cerr << "Invalid window size format\n";
            return 1;
        }

        width = std::stoi(windowSize.substr(0, xPos));
        height = std::stoi(windowSize.substr(xPos + 1));
    }

	return run(new MyFramework(width, height));
}