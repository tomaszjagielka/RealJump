#include <list>
#include <iostream>

#include "Framework.h"

struct Dimension {
    float x;
    float y;

    Dimension() : x(0), y(0) {}
    Dimension(float x, float y) : x(x), y(y) {}
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

class Object : public Entity {
public:
    Object(MySprite** sprites, int numSprites, Dimension position)
        : Entity(sprites, numSprites, position) {}

    void Update(Dimension windowSize, std::list<Entity*> objects) {

    }
};

class Player : public Entity {
    bool onPlatform = false;

    void Jump() {
        isJumping = true;
        velocity = -3;
    }

public:
    bool isJumping = true;
    Direction lastMoveDirection = Direction::RIGHT;
    Direction moveDirection = Direction::NONE;
    float velocity = 0;
    bool maxHeightCapped = false;
    int lifes = 5;
    bool gameOver = false;
    int distance = 0;
    int platfromCount = 0;

    Player(MySprite** sprites, int numSprites, Dimension position)
        : Entity(sprites, numSprites, position) {}

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

    void Update(Dimension windowSize, std::list<Entity*>& objects) {
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
        if (lifes > 0 && position.y > windowSize.y - this->sprites[0]->size.y / 2) {
            Dimension lowestPlatform = Dimension(0, 0);

            for (Entity* object : objects) {
                if (object->position.y > lowestPlatform.y) {
                    lowestPlatform = object->position;
                }
            }

            position.x = lowestPlatform.x + this->sprites[0]->size.x / 4;
            position.y = lowestPlatform.y - this->sprites[0]->size.y;
            lifes--;
        }

        gameOver = (lifes < 1);

        bool isFalling = lastYPosition < position.y;

        int yPositionDelta = position.y - lastYPosition;
        if (yPositionDelta > 0)
            distance += yPositionDelta;

        // COLLISIONS
        bool collidedWithPlatform = false;
        if (isFalling) {
            for (const auto& object : objects) {
                if (collidesWithPlatform(object) && isFalling) {
                    velocity = 0;
                    position.y -= 0.25;

                    if (!onPlatform) {
                        platfromCount++;
                        onPlatform = true;
                    }

                    collidedWithPlatform = true;

                    Jump();
                }
            }
        }

        if (!collidedWithPlatform) {
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
        this->isJumping = true;
        this->moveDirection = Direction::NONE;
        this->lastMoveDirection = Direction::RIGHT;
        this->maxHeightCapped = false;
        this->lifes = 5;
        this->gameOver = false;
        this->distance = 0;
        this->platfromCount = 0;
    }
};

class Enemy : public Entity {
    Direction lastUsedDirection = Direction::RIGHT;

public:
    Enemy(MySprite** sprites, int numSprites, Dimension position)
        : Entity(sprites, numSprites, position) {}

    void Update(Dimension windowSize) {
        position.x += 1;
        Draw(sprites[0]);
    }
};

class MyFramework : public Framework {
    Dimension windowSize;
    MySprite* backgroundSprite;
    Player* player;
    std::list<Entity*> objects;

    void PreInit(int& width, int& height, bool& fullscreen) override
    {
        width = windowSize.x;
        height = windowSize.y;
        fullscreen = false;
    }

    void InitPlatforms() {
        objects.push_back(new Object(new MySprite * [1] {new MySprite("data/game-tiles-green-platform@2x.png")},
            1,
            Dimension(player->position.x - player->sprites[0]->size.x / 4, player->position.y + player->sprites[0]->size.y)));

        while (objects.size() < 10) {
            int maxX = windowSize.x - objects.front()->sprites[0]->size.x;
            int minX = 0;
            int randomX = rand() % (maxX - minX + 1) + minX;

            int maxY = windowSize.y;
            int minY = 0;
            int randomY = rand() % (maxY - minY + 1) + minY;

            randomY = std::max(randomY, -500);
            randomX = std::min(randomX, int(windowSize.x));

            Dimension randomDimension(randomX, randomY);
            objects.push_back(new Object(new MySprite * [1] {new MySprite("data/game-tiles-green-platform@2x.png")}, 1, randomDimension));
        }
    }


    // return : true - ok, false - failed, application will exit
    bool Init() {
        backgroundSprite = new MySprite("data/bck@2x.png");
        player = new Player(
            new MySprite * [2] {new MySprite("data/lik-right-clipped@2x.png"), new MySprite("data/lik-left-clipped@2x.png")},
            2,
            Dimension(windowSize.x / 2, windowSize.y / 2));
        InitPlatforms();

        return true;
    }

    void Close() {
        delete backgroundSprite;
        delete player;
    }

    // return value: if true will exit the application
    bool Tick() {
        for (int width = 0; width < windowSize.x; width += backgroundSprite->size.x) {
            backgroundSprite->Draw(width, 0);
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

        player->Update(windowSize, objects);

        if (!objectExists) {
            int maxX = windowSize.x - objects.front()->sprites[0]->size.x;
            int minX = 0;
            int randomX = rand() % (maxX - minX + 1) + minX;

            int maxY = 0;
            int minY = -300;
            int randomY = rand() % (maxY - minY + 1) + minY;

            randomY = std::max(randomY, -500);
            randomX = std::min(randomX, int(windowSize.x));

            Dimension randomDimension(randomX, randomY);
            objects.push_back(new Object(new MySprite * [1] {new MySprite("data/game-tiles-green-platform@2x.png")},
                1,
                randomDimension));
        }

        if (player->gameOver) {
            std::cout << "Distance: " << player->distance << std::endl;
            std::cout << "Platform count: " << player->platfromCount << std::endl;
            player->Reset();
            for (auto& object : objects) {
                delete object;
            }
            objects.clear();
            InitPlatforms();
        }

        return false;
    }


    // param: xrel, yrel: The relative motion in the X/Y direction 
    // param: x, y : coordinate, relative to window
    void onMouseMove(int x, int y, int xrelative, int yrelative) { }

    void onMouseButtonClick(FRMouseButton button, bool isReleased) { }

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
    MyFramework() : windowSize(800, 1000) {}
};

int main(int argc, char *argv[])
{
	return run(new MyFramework);
}
