#include "Framework.h"

struct Dimension {
    int x;
    int y;

    Dimension() : x(0), y(0) {}
    Dimension(int w, int h) : x(w), y(h) {}
};

class MySprite {
public:
    Sprite* sprite;
    Dimension size;

    MySprite(const char* path) {
        sprite = createSprite(path);
        getSpriteSize(sprite, size.x, size.y);
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
    MySprite** sprites;
    int numSprites;
    Dimension position;
    Dimension windowSize;

    void Draw(MySprite* sprite) {
        sprite->Draw(position.x, position.y);
    }

public:
    Entity(MySprite** sprites, int numSprites, Dimension position, Dimension windowSize)
        : sprites(sprites), numSprites(numSprites), position(position), windowSize(windowSize) {}

    ~Entity() {
        for (int i = 0; i < numSprites; i++) {
            delete sprites[i];
        }
        delete[] sprites;
    }

    virtual void Update() = 0;

    Direction moveDirection = Direction::NONE;
};

class Player : public Entity {
    Direction lastUsedDirection = Direction::RIGHT;
    float velocity = 0;
    bool isJumping = true;
    int jumpStartTime = 0;

public:
    Player(MySprite** sprites, int numSprites, Dimension position, Dimension windowSize)
        : Entity(sprites, numSprites, position, windowSize) {}

    void Update() override {
        const float gravity = 0.0125;
        velocity += gravity;

        position.y += velocity;

        if (position.y + sprites[0]->size.y > windowSize.y) {
            position.y = windowSize.y - sprites[0]->size.y;
            velocity = 0;
            isJumping = false;
        }

        switch (moveDirection) {
        case Direction::RIGHT:
            position.x += 1;
            lastUsedDirection = Direction::RIGHT;
            Draw(sprites[0]);
            if (position.x > windowSize.x) {
                position.x = -sprites[0]->size.x;
            }
            break;
        case Direction::LEFT:
            position.x -= 1;
            lastUsedDirection = Direction::LEFT;
            Draw(sprites[1]);
            if (position.x < -sprites[1]->size.x) {
                position.x = windowSize.x;
            }
            break;
        case Direction::NONE:
            switch (lastUsedDirection) {
            case Direction::LEFT:
                Draw(sprites[1]);
                break;
            case Direction::RIGHT:
                Draw(sprites[0]);
                break;
            }
            break;
        }

        if (isJumping) {
            position.y -= 0.25;
        }
        else {
            Jump();
        }
    }

    void Jump() {
        // Start the jump
        isJumping = true;
        jumpStartTime = getTickCount();
        velocity = -1.5;
    }
};

class Enemy : public Entity {
    Direction lastUsedDirection = Direction::RIGHT;

public:
    Enemy(MySprite** sprites, int numSprites, Dimension position, Dimension windowSize)
        : Entity(sprites, numSprites, position, windowSize) {}

    void Update() override {
        position.x += 1;
        Draw(sprites[0]);
    }
};


class MyFramework : public Framework {
    Dimension windowSize;
    MySprite* backgroundSprite;
    Player* player;

    void PreInit(int& width, int& height, bool& fullscreen) override
    {
        width = windowSize.x;
        height = windowSize.y;
        fullscreen = false;
    }

    // return : true - ok, false - failed, application will exit
    bool Init() {
        backgroundSprite = new MySprite("data/bck@2x.png");
        MySprite** playerSprites = new MySprite * [2];
        player = new Player(new MySprite * [2] {new MySprite("data/lik-right@2x.png"), new MySprite("data/lik-left@2x.png")}, 2, Dimension(10, 10), windowSize);
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

        player->Update();

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
    MyFramework() : windowSize(800, 600) {}
};

int main(int argc, char *argv[])
{
	return run(new MyFramework);
}
