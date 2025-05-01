#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <deque>
#include <vector>
#include <fstream>
#include <cstdlib>
#include <ctime>

const int CELL_SIZE = 20;
const int WIDTH = 800;
const int HEIGHT = 600;
const int COLS = WIDTH / CELL_SIZE;
const int ROWS = HEIGHT / CELL_SIZE;

enum Direction { UP, DOWN, LEFT, RIGHT };
enum GameState { START, PLAYING, GAME_OVER };

struct SnakeSegment {
    int x, y;
    SnakeSegment(int x, int y) : x(x), y(y) {}
};

class SnakeGame {
private:
    sf::RenderWindow window;
    std::deque<SnakeSegment> snake;
    std::vector<sf::Vector2i> barriers;
    sf::Vector2i food, bigFood;
    Direction dir;
    GameState gameState = START;
    sf::Clock clock;
    float speed = 0.5f;

    int score = 0;
    int highScore = 0;
    int level = 1;

    bool bigFoodActive = false;
    sf::Clock bigFoodClock;

    sf::Font font;
    sf::Music music;
    sf::SoundBuffer eatBuffer, deathBuffer, levelBuffer;
    sf::Sound eatSound, deathSound, levelSound;

    sf::Texture foodTexture, bigFoodTexture;
    sf::Sprite foodSprite, bigFoodSprite;

public:
    SnakeGame() : window(sf::VideoMode(WIDTH, HEIGHT), "Snake Game Ultimate Edition") {
        srand(static_cast<unsigned>(time(nullptr)));
        font.loadFromFile("font/Arial.ttf");

        eatBuffer.loadFromFile("sounds/EatFruit.wav");
        deathBuffer.loadFromFile("sounds/endgame.wav");
        levelBuffer.loadFromFile("sounds/levelup.wav");
        eatSound.setBuffer(eatBuffer);
        deathSound.setBuffer(deathBuffer);
        levelSound.setBuffer(levelBuffer);

        foodTexture.loadFromFile("pics/little_food.jpg");
        bigFoodTexture.loadFromFile("pics/big_food.jpg");
        foodSprite.setTexture(foodTexture);
        bigFoodSprite.setTexture(bigFoodTexture);

        loadHighScore();
        resetGame();
    }

    void loadHighScore() {
        std::ifstream in("hi.txt");
        if (in.is_open()) {
            in >> highScore;
            in.close();
        }
    }

    void saveHighScore() {
        std::ofstream out("hi.txt");
        if (out.is_open()) {
            out << highScore;
            out.close();
        }
    }

    void resetGame() {
        snake.clear();
        snake.push_back(SnakeSegment(COLS / 2, ROWS / 2));
        dir = RIGHT;
        score = 0;
        level = 1;
        speed = 0.15f;
        spawnFood();
        bigFoodActive = false;
        bigFoodClock.restart();
        createBarriers();
        clock.restart();
    }

    void createBarriers() {
        barriers.clear();
        for (int i = 5; i < 15; ++i) {
            barriers.push_back({i, 10});
            barriers.push_back({COLS - i, ROWS - 12});
        }
        for (int i = 8; i < 18; ++i) {
            barriers.push_back({20, i});
        }
    }

    void spawnFood() {
        do {
            food.x = rand() % COLS;
            food.y = rand() % ROWS;
        } while (isOnSnake(food) || isOnBarrier(food));
    }

    void spawnBigFood() {
        do {
            bigFood.x = rand() % COLS;
            bigFood.y = rand() % ROWS;
        } while (isOnSnake(bigFood) || isOnBarrier(bigFood));
        bigFoodActive = true;
        bigFoodClock.restart();
    }

    void run() {
        while (window.isOpen()) {
            handleEvents();
            if (gameState == PLAYING && clock.getElapsedTime().asSeconds() > speed) {
                update();
                clock.restart();
            }
            render();
        }
    }

    void handleEvents() {
        sf::Event e;
        while (window.pollEvent(e)) {
            if (e.type == sf::Event::Closed)
                window.close();

            if (e.type == sf::Event::KeyPressed) {
                if (gameState == START && e.key.code == sf::Keyboard::Enter) {
                    resetGame();
                    gameState = PLAYING;
                } else if (gameState == GAME_OVER && e.key.code == sf::Keyboard::R) {
                    gameState = START;
                }

                if (gameState == PLAYING) {
                    if ((e.key.code == sf::Keyboard::W || e.key.code == sf::Keyboard::Up) && dir != DOWN) dir = UP;
                    else if ((e.key.code == sf::Keyboard::S || e.key.code == sf::Keyboard::Down) && dir != UP) dir = DOWN;
                    else if ((e.key.code == sf::Keyboard::A || e.key.code == sf::Keyboard::Left) && dir != RIGHT) dir = LEFT;
                    else if ((e.key.code == sf::Keyboard::D || e.key.code == sf::Keyboard::Right) && dir != LEFT) dir = RIGHT;
                }
            }
        }
    }

    void update() {
        SnakeSegment newHead = snake.front();
        switch (dir) {
            case UP: newHead.y--; break;
            case DOWN: newHead.y++; break;
            case LEFT: newHead.x--; break;
            case RIGHT: newHead.x++; break;
        }
        if (checkCollision(newHead)) {
            gameState = GAME_OVER;
            deathSound.play();
            if (score > highScore) {
                highScore = score;
                saveHighScore();
            }
            return;
        }

        bool grow = false;
        if (newHead.x == food.x && newHead.y == food.y) {
            score += 10;
            grow = true;
            eatSound.play();
            spawnFood();
            if (score >= level * 100) {
                level++;
                increaseDifficulty();
                levelSound.play();
            }
            if (score % 50 == 0 && speed > 0.05f) speed -= 0.01f;
        } else if (bigFoodActive && newHead.x == bigFood.x && newHead.y == bigFood.y) {
            score += 20;
            grow = true;
            eatSound.play();
            bigFoodActive = false;
        }

        if (grow)
            snake.push_front(newHead);
        else {
            snake.push_front(newHead);
            snake.pop_back();
        }

        if (!bigFoodActive && bigFoodClock.getElapsedTime().asSeconds() > 10.f)
            spawnBigFood();
        if (bigFoodActive && bigFoodClock.getElapsedTime().asSeconds() > 5.f)
            bigFoodActive = false;
    }

    void increaseDifficulty() {
        switch (level) {
            case 2:
                barriers.push_back({COLS / 2, ROWS / 2});
                break;
            case 3:
                for (int i = 0; i < 5; ++i)
                    barriers.push_back({10 + i, ROWS / 2});
                break;
            case 4:
                for (int i = 5; i < 15; ++i)
                    barriers.push_back({i, i});
                break;
            case 5:
                for (int i = 10; i < 20; ++i)
                    barriers.push_back({i, ROWS / 2});
                break;
            case 6:
                for (int i = 0; i < COLS; i += 2)
                    barriers.push_back({i, 5});
                break;
            case 7:
                for (int i = 0; i < ROWS; i += 2)
                    barriers.push_back({5, i});
                break;
            case 8:
                for (int i = 10; i < 20; ++i) {
                    barriers.push_back({i, 8});
                    barriers.push_back({i, 9});
                }
                break;
            case 9:
                for (int i = 0; i < 10; ++i)
                    barriers.push_back({COLS / 2 + i, i});
                break;
            case 10:
                for (int i = 0; i < COLS; i += 3)
                    for (int j = 0; j < ROWS; j += 3)
                        barriers.push_back({i, j});
                break;
            default:
                break;
        }
    }
    
    bool checkCollision(const SnakeSegment& head) {
        if (head.x < 0 || head.y < 0 || head.x >= COLS || head.y >= ROWS)
            return true;
        if (isOnBarrier({head.x, head.y}))
            return true;
        for (auto& s : snake)
            if (s.x == head.x && s.y == head.y)
                return true;
        return false;
    }

    bool isOnSnake(const sf::Vector2i& pos) {
        for (auto& s : snake)
            if (s.x == pos.x && s.y == pos.y)
                return true;
        return false;
    }

    bool isOnBarrier(const sf::Vector2i& pos) {
        for (auto& b : barriers)
            if (b == pos)
                return true;
        return false;
    }

    void drawBackground() {
        sf::RectangleShape tile(sf::Vector2f(CELL_SIZE, CELL_SIZE));
        sf::Color lightGreen(170, 215, 81);
        sf::Color darkGreen(162, 209, 73);
        for (int y = 0; y < ROWS; ++y) {
            for (int x = 0; x < COLS; ++x) {
                tile.setPosition(x * CELL_SIZE, y * CELL_SIZE);
                tile.setFillColor((x + y) % 2 == 0 ? lightGreen : darkGreen);
                window.draw(tile);
            }
        }
    }

    void render() {
        window.clear();
        drawBackground();

        if (gameState == START) {
            drawText("SNAKE GAME", 50, sf::Color::Green, WIDTH / 2, HEIGHT / 3, true);
            drawText("Press Enter to Start", 24, sf::Color::White, WIDTH / 2, HEIGHT / 2, true);
            drawText("High Score: " + std::to_string(highScore), 20, sf::Color::Yellow, WIDTH / 2, HEIGHT / 1.5, true);
        } else if (gameState == GAME_OVER) {
            drawSnake();
            drawFood();
            if (bigFoodActive) drawBigFood();
            drawBarriers();
            drawText("Game Over!", 40, sf::Color::Red, WIDTH / 2, HEIGHT / 3, true);
            drawText("Score: " + std::to_string(score), 24, sf::Color::White, WIDTH / 2, HEIGHT / 2, true);
            drawText("Press R to return to main menu", 20, sf::Color::White, WIDTH / 2, HEIGHT / 1.5, true);
        } else {
            drawSnake();
            drawFood();
            if (bigFoodActive) drawBigFood();
            drawBarriers();
            drawText("Score: " + std::to_string(score), 20, sf::Color::Yellow, 10, 10);
            drawText("Level: " + std::to_string(level), 20, sf::Color::Green, WIDTH / 2 - 40, 10);
            drawText("High Score: " + std::to_string(highScore), 20, sf::Color::Cyan, WIDTH - 170, 10);
        }

        window.display();
    }

    void drawSnake() {
        sf::RectangleShape segment(sf::Vector2f(CELL_SIZE - 2, CELL_SIZE - 2));
        segment.setFillColor(sf::Color::Green);
        for (auto& s : snake) {
            segment.setPosition(s.x * CELL_SIZE, s.y * CELL_SIZE);
            window.draw(segment);
        }
    }

    void drawFood() {
        foodSprite.setPosition(food.x * CELL_SIZE, food.y * CELL_SIZE);
        window.draw(foodSprite);
    }

    void drawBigFood() {
        bigFoodSprite.setPosition(bigFood.x * CELL_SIZE, bigFood.y * CELL_SIZE);
        window.draw(bigFoodSprite);
    }

    void drawBarriers() {
        sf::RectangleShape wall(sf::Vector2f(CELL_SIZE - 2, CELL_SIZE - 2));
        wall.setFillColor(sf::Color(150, 150, 150));
        for (auto& b : barriers) {
            wall.setPosition(b.x * CELL_SIZE, b.y * CELL_SIZE);
            window.draw(wall);
        }
    }

    void drawText(const std::string& str, int size, sf::Color color, float x, float y, bool center = false) {
        sf::Text text(str, font, size);
        text.setFillColor(color);
        if (center) {
            sf::FloatRect rect = text.getLocalBounds();
            text.setOrigin(rect.width / 2, rect.height / 2);
        }
        text.setPosition(x, y);
        window.draw(text);
    }
};

int main() {
    SnakeGame game;
    game.run();
    return 0;
}