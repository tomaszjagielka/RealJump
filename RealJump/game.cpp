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
public:
    Sprite* sprite;
    Dimension size;

    MySprite(const char* path) {
        sprite = createSprite(path);
        int w, h;
        getSpriteSize(sprite, w, h);
        size = Dimension(w, h);
    }

    void Draw(int x, int y) {
        drawSprite(sprite, x, y);
    }

    ~MySprite() {
        if (sprite) {
            destroySprite(sprite);
        }
    }
};

enum class Direction {
    NONE,
    LEFT,
    RIGHT
};

class Entity {
protected:
    int numSprites;

    void Draw(MySprite* sprite) {
        sprite->Draw(position.x, position.y);
    }

public:
    MySprite** sprites;
    Dimension position;

    Entity(MySprite** sprites, int numSprites, Dimension position)
        : sprites(sprites), numSprites(numSprites), position(position) {}

    ~Entity() {
        for (int i = 0; i < numSprites; i++) {
            delete sprites[i];
        }
        delete[] sprites;
    }

    //virtual void Update(Dimension windowSize, std::list<Entity*> objects) = 0;
};

enum ObjectType {
    DEFAULT,
    PLATFORM_NORMAL,
    PLATFORM_BOOST,
    JETPACK
};

class Object : public Entity {
public:
    ObjectType objectType;
    Object(MySprite** sprites, int numSprites, Dimension position, ObjectType objectType)
        : Entity(sprites, numSprites, position), objectType(objectType) {}

    //void Update(Dimension windowSize, std::list<Entity*> objects) {

    //}
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
        case ObjectType::PLATFORM_NORMAL:
            velocity -= 3;
			break;
        case ObjectType::PLATFORM_BOOST:
            velocity -= 6;
            break;
        case ObjectType::JETPACK:
            velocity -= 55;
            isVulnerable = false;
            break;
        }

        isFalling = false;
    }

    bool collidesWithPlatform(const Entity* platform) {
        if (position.y + sprites[0]->size.y > platform->position.y && // Player's feet are below the top of the platform.
            position.y < platform->position.y && // Player's head is above the platform.
            position.x + sprites[0]->size.x > platform->position.x && // Player is to the right of the platform.
            position.x < platform->position.x + platform->sprites[0]->size.x) // Player is to the left of the platform.
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

    Player(MySprite** sprites, int numSprites, Dimension position)
        : Entity(sprites, numSprites, position) {}

    void Update(Dimension windowSize, std::list<Entity*>& objects, std::list<Entity*>& enemies) {
        // FLAGS
        const float gravity = 0.0125;
        float lastYPosition = position.y;

        velocity += gravity;
        position.y += velocity;

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

            std::cout << "Lives left: " << lives << std::endl;
        }

        gameOver = (lives < 1);

        isFalling = lastYPosition < position.y;

        int yPositionDelta = position.y - lastYPosition;
        if (yPositionDelta > 0)
            distance += yPositionDelta;

        // COLLISIONS
        bool collidedWithEnemy = false;
        if (isVulnerable) {
            auto it = enemies.begin();

            while (it != enemies.end()) {
                Collision collision = collidesWithObject(*it);

                if (collision == Collision::OTHER) {
                    collidedWithEnemy = true;
                    Dimension lowestPlatform = Dimension(0, 0);

                    for (Entity* object : objects) {
                        if (object->position.y > lowestPlatform.y) {
                            lowestPlatform = object->position;
                        }
                    }

                    position.x = lowestPlatform.x + this->sprites[0]->size.x / 4;
                    position.y = lowestPlatform.y - this->sprites[0]->size.y - 200;

                    lives--;
                    std::cout << "Lives left: " << lives << std::endl;

                    velocity = 0;

                    break;
                }
                else if (collision == Collision::TOP) {
                    // Remove enemy
                    it = enemies.erase(it);
                }
                else {
                    ++it;
                }
            }
        }

        bool collidedWithPlatform = false;
        if (isFalling) {
            isVulnerable = true;

            for (const auto& object : objects) {
                if (collidesWithPlatform(object)) {
                    velocity = 0;
                    position.y -= 0.25;

                    if (!onPlatform) {
                        platfromCount++;
                        onPlatform = true;
                    }

                    collidedWithPlatform = true;

                    Object* obj = (Object*)object;
                    Jump(obj);

                    // TODO:
                    // Delete the Jetpack object properly.
                    if (obj->objectType == ObjectType::JETPACK)
                        obj->sprites[0] = new MySprite("data/transparent.png");
                }
            }
        }

        if (!collidedWithPlatform && !collidedWithEnemy) {
            onPlatform = false;
        }

        // MOVEMENT
        switch (moveDirection) {
        case Direction::RIGHT:
            position.x += 1;
            lastMoveDirection = Direction::RIGHT;
            Draw(sprites[0]);
            if (position.x > windowSize.x) {
                position.x = -sprites[0]->size.x;
            }
            break;
        case Direction::LEFT:
            position.x -= 1;
            lastMoveDirection = Direction::LEFT;
            Draw(sprites[1]);
            if (position.x < -sprites[1]->size.x) {
                position.x = windowSize.x;
            }
            break;
        case Direction::NONE:
            switch (lastMoveDirection) {
            case Direction::LEFT:
                Draw(sprites[1]);
                break;
            case Direction::RIGHT:
                Draw(sprites[0]);
                break;
            }
            break;
        }
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
                it = enemies.erase(it);
                // TODO:
                // Delete the Projectile object properly.
                speed = 0;
                sprites[0] = new MySprite("data/transparent.png");
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
    int mx, my, mxrelative, myrelative;

    void PreInit(int& width, int& height, bool& fullscreen) override
    {
        width = windowSize.x;
        height = windowSize.y;
        fullscreen = false;
    }

    void InitPlatforms() {
        objects.push_back(new Object(new MySprite * [1] {new MySprite("data/game-tiles-green-platform-clipped@2x.png")},1, Dimension(player->position.x - player->sprites[0]->size.x / 4, player->position.y + player->sprites[0]->size.y), ObjectType::PLATFORM_NORMAL));
        int platformCount = 10;

        for (int i = 0; i < platformCount; i++) {
            int maxX = windowSize.x - objects.front()->sprites[0]->size.x;
            int minX = 0;
            int randomX = rand() % (maxX - minX + 1) + minX;

            int maxY = 50;
            int minY = 0;
            int randomY = rand() % (maxY - minY + 1) + minY;

            randomY = 100 * i + randomY;
            randomX = std::min(randomX, int(windowSize.x));

            Dimension randomDimension(randomX, randomY);
            objects.push_back(
                new Object(new MySprite * [1] {new MySprite("data/game-tiles-green-platform-clipped@2x.png")}, 1, randomDimension, ObjectType::PLATFORM_NORMAL));
        }
    }

    // return : true - ok, false - failed, application will exit
    bool Init() {
        srand(time(0));
        backgroundSprite = new MySprite("data/bck@2x.png");
        player = new Player(
            new MySprite * [2] {new MySprite("data/lik-right-clipped@2x.png"), new MySprite("data/lik-left-clipped@2x.png")},
            2,
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
            Entity* object = *it;

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
                object->sprites[0]->Draw(object->position.x, object->position.y);
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

            if (rand() % 100 < 90) {
                objects.push_back(new Object(new MySprite * [1] {new MySprite("data/game-tiles-green-platform-clipped@2x.png")}, 1, randomDimension, ObjectType::PLATFORM_NORMAL));
            }
            else {
                objects.push_back(new Object(new MySprite * [1] {new MySprite("data/game-tiles-blue-platform-clipped@2x.png")}, 1, randomDimension, ObjectType::PLATFORM_BOOST));
            }

            if (rand() % 100 < 10) {
                enemies.push_back(new Enemy(new MySprite * [1] {new MySprite("data/enemy0-clipped@2x.png")}, 1, Dimension(randomDimension.x - 30, randomDimension.y - 60)));
            }
            else if (rand() % 100 < 2) {
                objects.push_back(new Object(new MySprite * [1] {new MySprite("data/game-tiles-jetpack-clipped@2x.png")}, 1, Dimension(randomDimension.x + 35, randomDimension.y - 70), ObjectType::JETPACK));
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
        mx = x;
        my = y;
        mxrelative = xrelative;
        yrelative = yrelative;
    }

    void onMouseButtonClick(FRMouseButton button, bool isReleased) {
        if (!isReleased) {
            projectiles.push_back(new Projectile(new MySprite * [1] {new MySprite("data/projectile-tiles0-clipped@2x.png")},
                1,
                Dimension(player->position.x + player->sprites[0]->size.x / 4, player->position.y),
                Dimension(mx, my)));
        }
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