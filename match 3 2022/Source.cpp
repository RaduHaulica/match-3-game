#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>

// some utility moved to top for convenience
sf::Vector2f lerp(sf::Vector2f A, sf::Vector2f B, float t)
{
    return (1 - t) * A + t * B;
}

float norm(sf::Vector2f v)
{
    return std::sqrt(v.x * v.x + v.y * v.y);
}

sf::Vector2f rotateVector(sf::Vector2f vector, float angle)
{
    float alpha = std::atan2(vector.y, vector.x);
    return { norm(vector) * std::cos(alpha + angle), norm(vector) * std::sin(alpha + angle) };
}

//======================================================================================
//              .: GAME CONFIG AND DATA :.
//======================================================================================

struct Config
{
    float gameWidth{ 800 };
    float gameHeight{ 600 };
    float tileWidth{ 75 };
    float swapDuration{ 0.2f };
    float minx = 75.0f;
    float miny = 75.0f;
    float gridWidth = 7.0f;
    float gridHeight = 7.0f;
    float powerUpBomb = 10.0f;

    int tileTypes = 7;

    bool logging = false;
};
Config config;

class Textures
{
public:
    sf::Texture* blueTexture;
    sf::Texture* greenTexture;
    sf::Texture* redTexture;
    sf::Texture* wildcardTexture;
    sf::Texture* yellowTexture;
    sf::Texture* purpleTexture;
    sf::Texture* bombTexture;

    sf::Texture* backgroundTexture;
    sf::Texture* scoreTexture;

    sf::Texture* selectorTexture;

    void loadTextures()
    {
        this->blueTexture = new sf::Texture();
        (*this->blueTexture).loadFromFile("./assets/graphics/element_blue_polygon.png");
        this->bombTexture = new sf::Texture();
        (*this->bombTexture).loadFromFile("./assets/graphics/bomb.png");
        this->greenTexture = new sf::Texture();
        (*this->greenTexture).loadFromFile("./assets/graphics/element_green_polygon.png");
        this->redTexture = new sf::Texture();
        (*this->redTexture).loadFromFile("./assets/graphics/element_red_polygon.png");
        this->wildcardTexture = new sf::Texture();
        (*this->wildcardTexture).loadFromFile("./assets/graphics/element_grey_diamond.png");
        this->yellowTexture = new sf::Texture();
        (*this->yellowTexture).loadFromFile("./assets/graphics/element_yellow_polygon.png");
        this->purpleTexture = new sf::Texture();
        (*this->purpleTexture).loadFromFile("./assets/graphics/element_purple_polygon.png");
        this->wildcardTexture = new sf::Texture();
        (*this->wildcardTexture).loadFromFile("./assets/graphics/element_grey_polygon.png");

        this->backgroundTexture = new sf::Texture();
        (*this->backgroundTexture).loadFromFile("./assets/graphics/bg.png");
        this->scoreTexture = new sf::Texture();
        (*this->scoreTexture).loadFromFile("./assets/graphics/score.png");

        this->selectorTexture = new sf::Texture();
        (*this->selectorTexture).loadFromFile("./assets/graphics/selectorA.png");
    }
};
Textures textures;

class Fonts
{
public:
    sf::Font* defaultFont;

    void loadFonts()
    {
        this->defaultFont = new sf::Font();
        this->defaultFont->loadFromFile("./assets/fonts/Roboto-Bold.ttf");
    }
};
Fonts fontsLibrary;

class GameAssets
{
public:
    sf::Sprite backgroundSprite;
    sf::Sprite scoreSprite;

    void loadSprites()
    {
        this->backgroundSprite.setTexture(*textures.backgroundTexture);
        this->backgroundSprite.setScale(config.tileWidth * config.gridWidth / this->backgroundSprite.getTexture()->getSize().x, config.tileWidth * (config.gridHeight + 1) / this->backgroundSprite.getTexture()->getSize().y);
        this->backgroundSprite.setPosition({ config.minx - config.tileWidth / 2, config.miny - config.tileWidth / 2 });

        this->scoreSprite.setTexture(*textures.scoreTexture);
        this->scoreSprite.setScale(config.tileWidth * 2 / this->scoreSprite.getTexture()->getSize().x, config.tileWidth / this->scoreSprite.getTexture()->getSize().y);
        this->scoreSprite.setPosition({ config.minx + config.tileWidth * config.gridWidth - config.tileWidth / 2, config.miny - config.tileWidth / 2 });
    }
};
GameAssets gameAssets;

class Event
{
public:
    enum class EventType
    {
        EventMatch,
        EventSound
    };

    Event(Event::EventType type, int payload):
        type{ type },
        payload{ payload }
    {

    }

    EventType type;
    int payload;
};

class SoundLibrary
{
public:
    std::vector<sf::SoundBuffer*> soundBuffers;
    std::vector<sf::Sound*> sounds;
    enum SoundMapper
    {
        SOUND_MATCH
    };

    void loadSounds()
    {
        sf::SoundBuffer* s = new sf::SoundBuffer();
        s->loadFromFile("./assets/sounds/impactBell_heavy_000.ogg");
        soundBuffers.push_back(s);

        sf::Sound* sound = new sf::Sound(*s);
        sounds.push_back(sound);

    }

    void play(Event* e)
    {
        sounds[e->payload]->play();
    }
};
SoundLibrary soundLibrary;

//==============================================================================================
//                                   .: CLASS - TILE :.
//==============================================================================================

std::vector<std::string> tileTypeToColor = { "RED", "GREEN", "BLUE", "YELLOW", "PURPLE", "WILDCARD", "BOMB" };

class Tile: public sf::Drawable
{
public:
    enum class TileType
    {
        RED = 0,
        GREEN = 1,
        BLUE = 2,
        YELLOW = 3,
        PURPLE = 4,
        WILDCARD = 5,
        BOMB = 6
    };

    sf::Texture* tileTexture;
    TileType type;
    sf::Sprite tileSprite;
    sf::Vector2f position;
    bool selected;
    sf::Sprite selectorSprite;
    bool moving;
    float currentStep;
    float totalDuration;
    sf::Vector2f origin, destination;
    bool dead;

    Tile()
    {

    }

    Tile(TileType type, sf::Vector2f position, sf::Vector2f sizeInGameWorld)
    {
        this->type = type;
        this->position = position;
        this->selected = false;
        this->moving = false;
        this->currentStep = 0.0f;
        this->totalDuration = 0.0f;
        this->dead = false;

        this->tileSprite.setTexture(*this->getTextureForTile(type));
        this->tileSprite.setOrigin(this->tileSprite.getTexture()->getSize().x / 2, this->tileSprite.getTexture()->getSize().y / 2);
        this->tileSprite.setScale({ sizeInGameWorld.x / this->tileSprite.getTexture()->getSize().x , sizeInGameWorld.y / this->tileSprite.getTexture()->getSize().y});
        this->tileSprite.setPosition(this->position);

        this->selectorSprite.setTexture(*textures.selectorTexture);
        this->selectorSprite.setOrigin(this->selectorSprite.getTexture()->getSize().x / 2, this->selectorSprite.getTexture()->getSize().y / 2);
        this->selectorSprite.setScale({ sizeInGameWorld.x / this->selectorSprite.getTexture()->getSize().x , sizeInGameWorld.y / this->selectorSprite.getTexture()->getSize().y });
        this->selectorSprite.setPosition(this->position);
    }

    void update(float dt)
    {
		if (this->moving)
		{
			this->currentStep += dt;
			if (this->currentStep > this->totalDuration)
			{
				this->currentStep = 0.0f;
                this->totalDuration = 0.0f;
				this->moving = false;
                //std::cout << "Stopped moving" << std::endl;
			}
			else
			{
				this->position = lerp(this->origin, this->destination, this->currentStep / this->totalDuration);
				this->tileSprite.setPosition(this->position);
				this->selectorSprite.setPosition(this->position);
			}
		}
        else
        {
			//std::cout << "Tile not moving" << std::endl;
        }
    }

    void move(sf::Vector2f pos, float animationDuration)
    {
        this->origin = this->position;
        this->destination = pos;
        this->moving = true;
        this->currentStep = 0.0f;
        this->totalDuration = animationDuration;
        //std::cout << "Started moving" << std::endl;
    }

    void undoMove()
    {
        this->destination = this->origin;
        this->origin = this->position;
        this->moving = true;
        this->currentStep = 0.0f;
    }

    void draw(sf::RenderTarget& target, sf::RenderStates states) const
    {
        //if (this->type == Tile::TileType(0)) return;

        target.draw(this->tileSprite);
        if (this->selected)
        {
            target.draw(this->selectorSprite);
        }
    }

    sf::Texture* getTextureForTile(Tile::TileType type)
    {
        switch (type)
        {
        case (Tile::TileType::BLUE):
            return textures.blueTexture;
        case (Tile::TileType::GREEN):
            return textures.greenTexture;
        case (Tile::TileType::RED):
            return textures.redTexture;
        case (Tile::TileType::WILDCARD):
            return textures.wildcardTexture;
        case (Tile::TileType::YELLOW):
            return textures.yellowTexture;
        case (Tile::TileType::BOMB):
            return textures.bombTexture;
        case (Tile::TileType::PURPLE):
            return textures.purpleTexture;
        //    return nullptr;
        default:
            ;
        }
    }

    void select()
    {
        this->selected = true;
    }

    void deselect()
    {
        this->selected = false;
    }

    bool isSelected()
    {
        return this->selected;
    }

    bool operator==(Tile t)
    {
        return this->type == Tile::TileType::WILDCARD || t.type == Tile::TileType::WILDCARD || this->type == t.type;
    }

    bool isEmpty()
    {
        return this->type == Tile::TileType(0);
    }

    void markForDeath()
    {
        //this->selected = true;
        this->dead = true;
    }

    bool isDead()
    {
        return this->dead;
    }
};

//============================================================================================
//                    .: OBSERVERS & EVENTS :.
//============================================================================================


class Scoreboard
{
public:
    int score{ 0 };

    void add(int score)
    {
        this->score += score;
        std::cout << "Current score: " << this->score << std::endl;
    }
};
Scoreboard scoreboard;

class MatchEvent: public Event
{
public:
    MatchEvent(Event::EventType type, int payload):
        Event(type, payload)
    {
    }
};

class Observer
{
public:
    virtual void onNotify(Event* event) = 0;
};

class Subject
{
public:
    std::vector<Observer*> observers[10];
    int numObservers{ 0 };

    void addObserver(Observer* obs)
    {
        this->observers->push_back(obs);
        this->numObservers++;
    }

    void notify(Event* event)
    {
        for (int i = 0; i < this->numObservers; i++)
        {
            (*this->observers)[i]->onNotify(event);
        }
    }
};
Subject eventWatcher;

class MatchObserver : public Observer
{
public:
    Scoreboard* scoreboard;
    MatchObserver(Scoreboard& scoreboard)
    {
        this->scoreboard = &scoreboard;
    }

    virtual void onNotify(Event* event)
    {
        if (event->type == Event::EventType::EventMatch)
        {
            std::cout << "Score event notification" << std::endl;
            this->scoreboard->add(event->payload);

        }
    }
};

class SoundObserver : public Observer
{
public:
    virtual void onNotify(Event* event)
    {
        if (event->type == Event::EventType::EventMatch)
        {
			std::cout << "Sound event notification" << std::endl;
            event->payload = SoundLibrary::SoundMapper::SOUND_MATCH;
			soundLibrary.play(event);
        }
    }
};

//====================================================================================
//                            .: PARTICLE SYSTEM :.
//====================================================================================

/*
* a--b  texture polygon utility for textured particles
* |  |
* d--c
*/
class Quad
{
public:
    sf::Vector2f a, b, c, d;
    Quad() :
        a{ sf::Vector2f({0.0f,0.0f}) },
        b{ sf::Vector2f({0.0f,0.0f}) },
        c{ sf::Vector2f({0.0f, 0.0f}) },
        d{ sf::Vector2f({0.0f, 0.0f}) }
    {
    }

    Quad(sf::Vector2f a, sf::Vector2f b, sf::Vector2f c, sf::Vector2f d) :
        a{ a },
        b{ b },
        c{ c },
        d{ d }
    {};

    Quad operator=(Quad other)
    {
        this->a = other.a;
        this->b = other.b;
        this->c = other.c;
        this->d = other.d;
        return *this;
    }

    void setCoords(sf::Vector2f a, sf::Vector2f b, sf::Vector2f c, sf::Vector2f d)
    {
        this->a = a;
        this->b = b;
        this->c = c;
        this->d = d;
    };

};

struct ParticleProperties
{
    sf::Vector2f position;
    sf::Vector2f velocity;
    sf::Vector2f acceleration;
    sf::Color color;
    float lifetime;
    Quad textureCoords;
    sf::Vector2f size;
    float startingAlpha;
    float endAlpha;
};

class IUpdatable
{
public:
    void virtual update(float dt) = 0;
};

class IDrawable
{
public:
    void virtual draw(sf::VertexArray& va) = 0;
};

class Entity : public IUpdatable
{
public:
    sf::Vector2f position;
    sf::Vector2f velocity;
    sf::Vector2f acceleration;

    Entity(sf::Vector2f position, sf::Vector2f velocity, sf::Vector2f acceleration) :
        position{ position },
        velocity{ velocity },
        acceleration{ acceleration }
    {};

    virtual void update(float dt) override
    {
        this->velocity += this->acceleration * dt;
        this->position += this->velocity * dt;
    };
};

// ==========
// Particles
// ==========

class Particle : public Entity, public IDrawable
{
public:
    float lifetime;
    float timeElapsed;
    sf::Color color;
    Quad textureCoords;
    bool dead;
    sf::Vector2f size;
    float startingAlpha;
    float endAlpha;

    Particle(ParticleProperties props) :
        Entity(props.position, props.velocity, props.acceleration),
        lifetime{ props.lifetime },
        color{ props.color },
        textureCoords{ props.textureCoords },
        size{ props.size },
        startingAlpha{ props.startingAlpha },
        endAlpha{ props.endAlpha }
    {
        this->dead = false;
        this->timeElapsed = 0.0f;
    }

    void update(float dt)
    {
        this->Entity::update(dt);
        this->timeElapsed += dt;
        if (this->timeElapsed > this->lifetime)
        {
            this->dead = true;
        }
        this->color.a = startingAlpha + (startingAlpha - endAlpha > 0 ? -1 : 1) * (abs(startingAlpha - endAlpha)) * (this->timeElapsed / this->lifetime);
    }

    void draw(sf::VertexArray& va)
    {
        sf::Vertex v;

        v.color = this->color;
        v.position = this->position;
        v.texCoords = this->textureCoords.a;
        va.append(v);

        v.color = this->color;
        v.position = this->position + sf::Vector2f({ this->size.x,0 });
        v.texCoords = this->textureCoords.b;
        va.append(v);

        v.color = this->color;
        v.position = this->position + sf::Vector2f({ this->size.x, -this->size.y });
        v.texCoords = this->textureCoords.c;
        va.append(v);

        v.color = this->color;
        v.position = this->position + sf::Vector2f({ 0, -this->size.y });
        v.texCoords = this->textureCoords.d;
        va.append(v);
    }
};

class PixelFaderParticle : public Particle
{
public:
    PixelFaderParticle(ParticleProperties props) :
        Particle(props)
    {}

    void update(float dt)
    {
        this->Particle::update(dt);
        this->color.a = 255 - (this->timeElapsed / this->lifetime) * 255;
    }
};

// ==========
// Emitters
// ==========

class ParticleSystem;

class IEmitter : public IUpdatable
{
public:
    void virtual init(ParticleSystem& ps) = 0;
    void virtual update(float dt) = 0;
    void virtual createParticle(std::vector<Particle*>& v) = 0;

};

class BaseEmitter : public IEmitter
{
public:
    ParticleSystem* parentPS;
    bool initialized;
    sf::Vector2f position;
    sf::Vector2f acceleration;
    sf::Vector2f velocity;
    float lifetime;
    sf::Color color;
    Quad textureCoords;
    sf::Vector2f size;
    float startingAlpha;
    float endAlpha;

    BaseEmitter(ParticleProperties props) :
        position{ props.position },
        velocity{ props.velocity },
        acceleration{ props.acceleration },
        lifetime{ props.lifetime },
        color{ props.color },
        textureCoords{ props.textureCoords },
        size{ props.size },
        startingAlpha{ props.startingAlpha },
        endAlpha{ props.endAlpha }
    {
        this->initialized = false;
        parentPS = nullptr;
    }

    void init(ParticleSystem& ps)
    {
        this->parentPS = &ps;
        this->initialized = true;
    }

    void createParticle(std::vector<Particle*>& v)
    {
        if (this->initialized)
        {
            ParticleProperties props;
            props.position = this->position;
            props.velocity = this->velocity;
            props.acceleration = this->acceleration;
            props.color = this->color;
            props.lifetime = this->lifetime;
            props.textureCoords = this->textureCoords;
            props.size = this->size;
            props.startingAlpha = this->startingAlpha;
            props.endAlpha = this->endAlpha;
            v.push_back(new Particle(props));
        }
    }

    void update(float dt)
    {
    }

    void updateFromParent(sf::Vector2f position)
    {
        this->position = position;
    }
};

class ExplosionEmitter : public BaseEmitter
{
public:
    int particlesNumber;
    ExplosionEmitter(ParticleProperties props, int particlesNumber) :
        BaseEmitter(props),
        particlesNumber{ particlesNumber }
    {}

    void createParticle(std::vector<Particle*>& v)
    {
        for (int i = 0; i < particlesNumber; i++)
        {
            sf::Vector2f randomSpeed({ float(std::rand()) / RAND_MAX * 200, 0 });
            randomSpeed = rotateVector(randomSpeed, i * (360.0f / this->particlesNumber));

            ParticleProperties props;
            props.position = this->position;
            props.velocity = randomSpeed;
            props.acceleration = { 0, 0 };
            props.color = this->color;
            props.lifetime = this->lifetime;
            props.textureCoords = this->textureCoords;
            props.size = this->size;
            props.startingAlpha = this->startingAlpha;
            props.endAlpha = this->endAlpha;
            v.push_back(new Particle(props));
        }
    }
};

// ==========
// ParticleSystem
// ==========
class ParticleSystem : public sf::Drawable, public Entity
{
public:
    sf::Texture* texture;
    BaseEmitter* emitter;
    float timeAccumulator;
    float particleRate;
    float particleLifetime;
    bool active;
    bool firedParticles;
    std::vector<Particle*> particles;
    sf::VertexArray particlesVA;
    sf::VertexArray triangle;
    ParticleSystem(ParticleProperties props, BaseEmitter* emitter, float particleRate, sf::Texture* texture) :
        Entity(props.position, props.velocity, props.acceleration),
        particleLifetime{ props.lifetime },
        emitter{ emitter },
        particleRate{ particleRate },
        timeAccumulator{ 0.0f },
        texture{ texture },
        active{ true },
        firedParticles{ false }
    {
        this->particlesVA.setPrimitiveType(sf::PrimitiveType::Quads);
        this->triangle.setPrimitiveType(sf::PrimitiveType::Triangles);
    }

    ~ParticleSystem()
    {
        //delete this->emitter;
    }

    void update(float dt)
    {
        this->Entity::update(dt);
        this->emitter->updateFromParent(this->position);
        for (int i = 0; i < this->particles.size(); i++)
        {
            this->particles[i]->update(dt);
            if (this->particles[i]->dead)
            {
                delete this->particles[i];
                this->particles.erase(this->particles.begin() + i);
                i--;
            }
        }

        this->timeAccumulator += dt;
        if (!this->firedParticles)
        {
            this->firedParticles = true;
            this->emitter->createParticle(this->particles);
        }

        if (this->firedParticles && this->particles.size() == 0)
        {
            this->markForDeath();
        }

        this->triangle.clear();
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space))
        {
            this->triangle.append({ { this->position.x, this->position.y }, sf::Color::Yellow });
            this->triangle.append({ { this->position.x + 5, this->position.y - 7 }, sf::Color::Yellow });
            this->triangle.append({ { this->position.x - 5, this->position.y - 7 }, sf::Color::Yellow });
        }

        this->particlesVA.clear();
        for (int i = 0; i < this->particles.size(); i++)
        {
            this->particles[i]->draw(this->particlesVA);
        }
    }

    virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const override
    {
        target.draw(this->triangle);
        target.draw(this->particlesVA, this->texture);
    }
    
    void markForDeath()
    {
        this->active = false;
    }

    bool isDead()
    {
        return !this->active;
    }
};

//====================================================================================
//                           .: UTILITY FUNCTIONS :.
//====================================================================================

sf::VertexArray createVertexArray(std::vector<sf::Vector2f> v, sf::Color color)
{
    sf::VertexArray va;
    va.setPrimitiveType(sf::PrimitiveType::LinesStrip);
    for (int i = 0; i < v.size(); i++)
    {
        va.append(sf::Vertex(v[i], color));
    }
    return va;
}

sf::Vector2f normalize(sf::Vector2f v)
{
    return v / std::sqrt(v.x * v.x + v.y * v.y);
}

float distanceBetweenPoints(sf::Vector2f A, sf::Vector2f B)
{
    return std::sqrt((B.x - A.x) * (B.x - A.x) + (B.y - A.y) * (B.y - A.y));
}

void fillNewGrid(std::vector<Tile>& grid, Config& config)
{
    grid.clear();
    for (int j = 0; j < config.gridHeight; j++)
    {
        for (int i = 0; i < config.gridWidth; i++)
        {
            //std::cout << "Working on [" << j + 1 << ", " << i + 1 << "] index: "<<i + j * config.gridWidth<<std::endl;
            std::vector<int> possibleTypes;
            for (int k = 0; k < config.tileTypes; k++) possibleTypes.push_back(k);
            possibleTypes.erase(possibleTypes.end() - 1); // remove bomb tile
            possibleTypes.erase(possibleTypes.end() - 1); // remove wildcard tile

            if (i > 1)
            {
                if (grid[i - 1 + j * config.gridWidth] == grid[i - 2 + j * config.gridWidth])
                {
                    for (int k = 0; k < possibleTypes.size(); k++)
                    {
                        if (grid[i - 1 + j * config.gridWidth].type == Tile::TileType(possibleTypes[k]))
                        {
                            //std::cout << "Eliminated tile: " << tileTypeToColor[possibleTypes[k]] << std::endl;
                            possibleTypes.erase(possibleTypes.begin() + k);
                            break;
                        }
                    }
                }
            }

            if (j > 1)
            {
                if (grid[i + (j - 1) * config.gridWidth] == grid[i + (j - 2) * config.gridWidth])
                {
                    for (int k = 0; k < possibleTypes.size(); k++)
                    {
                        if (grid[i + (j - 1) * config.gridWidth].type == Tile::TileType(possibleTypes[k]))
                        {
                            //std::cout << "Eliminated tile: " << tileTypeToColor[possibleTypes[k]] << std::endl;
                            possibleTypes.erase(possibleTypes.begin() + k);
                            break;
                        }
                    }
                }
            }

            //std::cout << "possible : " << possibleTypes.size() << std::endl;
            int selector = rand() % possibleTypes.size();
            Tile::TileType t = Tile::TileType(possibleTypes[selector]);
            Tile tile = Tile(t, sf::Vector2f({ config.minx + config.tileWidth * i, config.miny + config.tileWidth * j }), { config.tileWidth, config.tileWidth });
            grid.push_back(tile);
            //std::cout << " type: " << tileTypeToColor[(int)grid[i + j * config.gridWidth].type] << std::endl;
        }
    }
}

bool matchPossible(std::vector<Tile>& grid, Config& config)
{
    float minx{ config.minx };
    float miny{ config.miny };
    float maxx{ config.minx + config.tileWidth * config.gridWidth };
    float maxy{ config.miny + config.tileWidth * config.gridHeight };

    // checking grid positions is inefficient without an actual grid
    for (int i = 0; i < grid.size(); i++)
    {
        for (int j = 0; j < grid.size(); j++)
        {
            // 4 tests to check 12 possible matches
            // check possible matches annex

            // grid[i+1][j+1] == grid[i][j] => 1 4 8 9 - 4 further tests
            if (
                i != j
                && grid[i] == grid[j]
                && std::fabs(grid[i].position.x + config.tileWidth - grid[j].position.x) <= 1.0f
                && std::fabs(grid[i].position.y + config.tileWidth - grid[j].position.y) <= 1.0f
                )
            {
                for (int k = 0; k < grid.size(); k++)
                {
                    if (
                        k != i
                        && grid[i] == grid[k]
                        )
                    {
                        if ( // 1
                            std::fabs(grid[i].position.x - config.tileWidth - grid[k].position.x) <= 1.0f
                            && std::fabs(grid[i].position.y + config.tileWidth - grid[k].position.y) <= 1.0f
                            )
                        {
                            return true;
                        }

                        if ( // 4
                            std::fabs(grid[i].position.x - config.tileWidth - grid[k].position.x) <= 1.0f
                            && std::fabs(grid[i].position.y - grid[k].position.y) <= 1.0f
                            )
                        {
                            return true;
                        }

                        if ( // 8
                            std::fabs(grid[i].position.x + config.tileWidth - grid[k].position.x) <= 1.0f
                            && std::fabs(grid[i].position.y - config.tileWidth - grid[k].position.y) <= 1.0f
                            )
                        {
                            return true;
                        }

                        if ( // 9
                            std::fabs(grid[i].position.x - grid[k].position.x) <= 1.0f
                            && std::fabs(grid[i].position.y - config.tileWidth - grid[k].position.y) <= 1.0f
                            )
                        {
                            return true;
                        }
                    }
                }
            }

            // grid[i+1][j-1] == grid[i][j] => 2 3 7 19 - 4 further tests
            if (
                i != j
                && grid[i] == grid[j]
                && std::fabs(grid[i].position.x - config.tileWidth - grid[j].position.x) <= 1.0f
                && std::fabs(grid[i].position.y + config.tileWidth - grid[j].position.y) <= 1.0f
                )
            {
                for (int k = 0; k < grid.size(); k++)
                {
                    if (i != k && grid[i] == grid[k])
                    {
                        if ( // 2
                            std::fabs(grid[i].position.x - 2 * config.tileWidth - grid[k].position.x) <= 1.0f
                            && std::fabs(grid[i].position.y - grid[k].position.y) <= 1.0f
                            )
                        {
                            return true;
                        }

                        if ( // 3
                            std::fabs(grid[i].position.x - 2 * config.tileWidth - grid[k].position.x) <= 1.0f
                            && std::fabs(grid[i].position.y + config.tileWidth - grid[k].position.y) <= 1.0f
                            )
                        {
                            return true;
                        }

                        if ( // 7
                            std::fabs(grid[i].position.x - config.tileWidth - grid[k].position.x) <= 1.0f
                            && std::fabs(grid[i].position.y - config.tileWidth - grid[k].position.y) <= 1.0f
                            )
                        {
                            return true;
                        }

                        if ( // 10
                            std::fabs(grid[i].position.x - grid[k].position.x) <= 1.0f
                            && std::fabs(grid[i].position.y - config.tileWidth - grid[k].position.y) <= 1.0f
                            )
                        {
                            return true;
                        }
                    }
                }
            }

            // grid[i][j+1] == grid[i][j] => 5 6 - 2 further tests
            if (
                i != j
                && grid[i] == grid[j]
                && std::fabs(grid[i].position.x + config.tileWidth - grid[j].position.x) <= 1.0f
                && std::fabs(grid[i].position.y - grid[j].position.y) <= 1.0f
                )
            {
                for (int k = 0; k < grid.size(); k++)
                {
                    if (i != k && grid[i] == grid[k])
                    {
                        if ( // 5
                            std::fabs(grid[i].position.x + 3 * config.tileWidth - grid[k].position.x) <= 1.0f
                            && std::fabs(grid[i].position.y - grid[k].position.y) <= 1.0f
                            )
                        {
                            return true;
                        }

                        if ( // 6
                            std::fabs(grid[i].position.x - 2 * config.tileWidth - grid[k].position.x) <= 1.0f
                            && std::fabs(grid[i].position.y - grid[k].position.y) <= 1.0f
                            )
                        {
                            return true;
                        }
                    }
                }
            }

            // grid[i+1][j] == grid[i][j] => 11 12 - 2 further tests
            if (
                i != j
                && grid[i] == grid[j]
                && std::fabs(grid[i].position.x - grid[j].position.x) <= 1.0f
                && std::fabs(grid[i].position.y + config.tileWidth - grid[j].position.y) <= 1.0f
                )
            {
                for (int k = 0; k < grid.size(); k++)
                {
                    if (i != k && grid[i] == grid[k])
                    {
                        if ( // 11
                            std::fabs(grid[i].position.x - grid[k].position.x) <= 1.0f
                            && std::fabs(grid[i].position.y - 2 * config.tileWidth - grid[k].position.y) <= 1.0f
                            )
                        {
                            return true;
                        }

                        if ( // 12
                            std::fabs(grid[i].position.x - grid[k].position.x) <= 1.0f
                            && std::fabs(grid[i].position.y + 3 * config.tileWidth - grid[k].position.y) <= 1.0f
                            )
                        {
                            return true;
                        }
                    }
                }
            }
        }
    }
    return false;
}

//==========================================================================
//                     .: MAIN :.
//============================================================================


int main()
{
    sf::RenderWindow window(sf::VideoMode(config.gameWidth, config.gameHeight), "SFML works!");

    // =========================
    // pre game initialization
    // =========================

    std::srand(std::time(nullptr));
    textures.loadTextures();
    soundLibrary.loadSounds();
    gameAssets.loadSprites();
    fontsLibrary.loadFonts();

    eventWatcher.addObserver(new MatchObserver(scoreboard));
    eventWatcher.addObserver(new SoundObserver());

    sf::Vector2i mousePos;
    sf::Vector2f mousePosWorld;

    sf::Clock frameClock;
    float dt;

    std::vector<Tile> grid; // 10 x 10
    Tile cornerCheck = Tile(Tile::TileType::RED, sf::Vector2f({ config.minx - config.tileWidth, config.miny - config.tileWidth}), { config.tileWidth, config.tileWidth });
    int selectedTileIndex{ -1 };
    int swappedFromTileIndex{ -1 };
    int swappedToTileIndex{ -1 };
    float lockInput{ 0.0f };
    float coyoteTime{ 0.0f };
    bool stuffMoving{ false };
    bool swapMatchCheck{ false };
    bool collapseNeeded{ false };
    bool gridResetRequired{ false };
    int matchedTileCount{ 0 };
    int powerUpTracker{ 0 };
    bool bombActive{ false };

    float createdTiles{ 0.0f };
    float createdWildcardTiles{ 0.0f };

    sf::Text scoreText;
    scoreText.setFont(*fontsLibrary.defaultFont);
    scoreText.setCharacterSize(24);
    scoreText.setFillColor(sf::Color::Black);
    scoreText.setStyle(sf::Text::Bold);
    scoreText.setPosition(gameAssets.scoreSprite.getPosition());

    sf::Text helpText;
    helpText.setFont(*fontsLibrary.defaultFont);
	helpText.setCharacterSize(12);
	helpText.setFillColor(sf::Color::White);
	//helpText.setStyle(sf::Text::Bold);
    helpText.setPosition({ 575, 525 });
    helpText.setString("Click to match tiles\nGrey tile is wildcard\nBomb tile will destroy\nall adjacent tiles");

    std::vector<ParticleSystem*> explosions;

    // ======================
    // -= initialization =-
    // ======================
    fillNewGrid(grid, config);

    // ======================
    // -= game is starting =-
    // ======================
    
    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();
            if (event.type == sf::Event::KeyPressed)
            {
                if (event.key.code == sf::Keyboard::Escape)
                {
                    window.close();
                }
            }
        }

        sf::Vector2i mousePos = sf::Mouse::getPosition(window);
        sf::Vector2f mousePosWorld = window.mapPixelToCoords(mousePos);

        dt = frameClock.restart().asSeconds();

        // meat and potatoes

		matchedTileCount = 0;

        // process input

        if (coyoteTime > 0)
        {
            coyoteTime -= dt;
        }
        else
        {
            coyoteTime = 0.0f;
        }

        if (lockInput > 0)
        {
            lockInput -= dt;
        }
        else
        {
            lockInput = 0.0f;
            // left mouse
            if (sf::Mouse::isButtonPressed(sf::Mouse::Left))
            {
                lockInput = config.swapDuration;
                for (int i = 0; i < grid.size(); i++)
                {
                    if (grid[i].tileSprite.getGlobalBounds().contains(mousePosWorld))
                    {
                        if (selectedTileIndex < 0)
                        {
                            grid[i].select();
                            selectedTileIndex = i;
                            lockInput = config.swapDuration;
                            std::cout << "new selection: " << tileTypeToColor[(int)grid[i].type] << std::endl;
                        }
                        else
                        {
                            if (
                                (grid[selectedTileIndex].tileSprite.getGlobalBounds().contains(grid[i].position + sf::Vector2f({ -config.tileWidth, 0 }))) // selected is to left of clicked
                                || (grid[selectedTileIndex].tileSprite.getGlobalBounds().contains(grid[i].position + sf::Vector2f({ config.tileWidth, 0 }))) // selected is to right of clicked
                                || (grid[selectedTileIndex].tileSprite.getGlobalBounds().contains(grid[i].position + sf::Vector2f({ 0, -config.tileWidth }))) // selected is above clicked
                                || (grid[selectedTileIndex].tileSprite.getGlobalBounds().contains(grid[i].position + sf::Vector2f({ 0, config.tileWidth }))) // selected is below clicked
                                )
                            {
                                grid[i].move(grid[selectedTileIndex].position, config.swapDuration);
                                grid[i].deselect();
                                grid[selectedTileIndex].move(grid[i].position, config.swapDuration);
                                grid[selectedTileIndex].deselect();
                                swappedFromTileIndex = selectedTileIndex;
                                swappedToTileIndex = i;
                                swapMatchCheck = true;
                                selectedTileIndex = -1;
                                lockInput = config.swapDuration;
                                std::cout << "swapped" << std::endl;
                                if (grid[swappedFromTileIndex].type == Tile::TileType::BOMB)
                                {
                                    bombActive = true;
                                }
                            }
                            else
                            {
                                grid[selectedTileIndex].deselect();
                                grid[i].select();
                                selectedTileIndex = i;
                                lockInput = config.swapDuration;
                                std::cout << "changed selection: "<< tileTypeToColor[(int)grid[i].type] << std::endl;
                            }
                        }
                        break;
                    }
                }
            }

            // right mouse
            if (sf::Mouse::isButtonPressed(sf::Mouse::Right))
            {
                for (int i = 0; i < grid.size(); i++)
                {
                    if (grid[i].tileSprite.getGlobalBounds().contains(mousePosWorld))
                    {
                        lockInput = config.swapDuration;
                        std::cout << "Tile query: " << tileTypeToColor[(int)grid[i].type] 
                            << " line: " << (int)(grid[i].position.y - config.miny + config.tileWidth/2) / (int)config.tileWidth 
                            << " column: " << (int)(grid[i].position.x - config.minx + config.tileWidth/2) / (int)config.tileWidth 
                            << " position: " << grid[i].position.x << ", " << grid[i].position.y 
                            << std::endl;
                    }
                }
            }

            // space
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space))
            {
                lockInput = 0.2f;
                std::cout << "Match possible? " << matchPossible(grid, config) << std::endl;
            }

            // ESC
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Escape))
            {
                break;
            }
        }

        stuffMoving = false;
        for (int i = 0; i < grid.size(); i++)
        {
            if (grid[i].moving == true)
            {
                stuffMoving = true;
                //std::cout << "Moving index: " << i << std::endl;
            }
        }
        //std::cout << "Stuffmoving: " << stuffMoving << std::endl;

        // update
        for (int i = 0; i < grid.size(); i++)
        {
            grid[i].update(dt);
        }
        for (int i = 0; i < explosions.size(); i++)
        {
            explosions[i]->update(dt);
        }

        // match 3
        if (!stuffMoving || coyoteTime > 0)
        {
			float minimumOffset = config.minx + 1.5 * config.tileWidth;

            if (!bombActive)
            {

				for (int i = 0; i < grid.size(); i++)
				{
					int leftOne = i;
					int leftTwo = i;
					int topOne = i;
					int topTwo = i;

					for (int j = 0; j < grid.size(); j++)
					{
						if (grid[i].position.x >= minimumOffset)
						{
							if (i != j && ((int)(grid[i].position.y - config.miny + config.tileWidth / 2) / (int)config.tileWidth == ((int)(grid[j].position.y - config.miny + config.tileWidth / 2) / (int)config.tileWidth)) && grid[i] == grid[j])
							{
								if (grid[j].tileSprite.getGlobalBounds().contains(grid[i].position - sf::Vector2f({ config.tileWidth, 0 })))
								{
									leftOne = j;
								}
								if (grid[j].tileSprite.getGlobalBounds().contains(grid[i].position - sf::Vector2f({ 2 * config.tileWidth, 0 })))
								{
									leftTwo = j;
								}
							}
						}

						if (grid[i].position.y >= minimumOffset)
						{
							if (i != j && ((int)(grid[i].position.x - config.minx + config.tileWidth / 2) / (int)config.tileWidth == (int)(grid[j].position.x - config.minx + config.tileWidth / 2) / (int)(config.tileWidth)) && grid[i] == grid[j])
							{
								if (grid[j].tileSprite.getGlobalBounds().contains(grid[i].position - sf::Vector2f({ 0, config.tileWidth })))
								{
									topOne = j;
								}
								if (grid[j].tileSprite.getGlobalBounds().contains(grid[i].position - sf::Vector2f({ 0, 2 * config.tileWidth })))
								{
									topTwo = j;
								}
							}
						}

					}

					if (leftOne != i && leftTwo != i && grid[leftOne] == grid[leftTwo])
					{
						grid[i].markForDeath();
						grid[leftOne].markForDeath();
						grid[leftTwo].markForDeath();
						swapMatchCheck = false;
						coyoteTime = 1.0f;
						powerUpTracker++;
						if (config.logging) std::cout << "Match horizontal " << tileTypeToColor[(int)grid[i].type] << std::endl;
					}

					if (topOne != i && topTwo != i && grid[topOne] == grid[topTwo])
					{
						grid[i].markForDeath();
						grid[topOne].markForDeath();
						grid[topTwo].markForDeath();
						swapMatchCheck = false;
						coyoteTime = 1.0f;
						powerUpTracker++;
						if (config.logging) std::cout << "Match vertical " << tileTypeToColor[(int)grid[i].type] << std::endl;
					}
				}
            }
            else
            {
                std::cout << "Bomb time!" << std::endl;
                bombActive = false;
                swapMatchCheck = false;
                int bombIndex;
                for (int i = 0; i < grid.size(); i++)
                {
                    if (grid[i].type == Tile::TileType::BOMB)
                        bombIndex = i;
                }
                grid[bombIndex].markForDeath();
                for (int i = 0; i < grid.size(); i++)
                {
                    if (i != bombIndex)
                    {
                        if (grid[i].tileSprite.getGlobalBounds().contains(grid[bombIndex].position + sf::Vector2f({ -config.tileWidth, -config.tileWidth }))) 
                        {
                            grid[i].markForDeath();
                            continue;
                        }
                        if (grid[i].tileSprite.getGlobalBounds().contains(grid[bombIndex].position + sf::Vector2f({ 0, -config.tileWidth }))) 
                        {
                            grid[i].markForDeath();
                            continue;
                        }
                        if (grid[i].tileSprite.getGlobalBounds().contains(grid[bombIndex].position + sf::Vector2f({ config.tileWidth, -config.tileWidth }))) 
                        {
                            grid[i].markForDeath();
                            continue;
                        }
                        
                        if (grid[i].tileSprite.getGlobalBounds().contains(grid[bombIndex].position + sf::Vector2f({ -config.tileWidth, 0 }))) 
                        {
                            grid[i].markForDeath();
                            continue;
                        }
                        // own position
                        if (grid[i].tileSprite.getGlobalBounds().contains(grid[bombIndex].position + sf::Vector2f({ config.tileWidth, 0 }))) 
                        {
                            grid[i].markForDeath();
                            continue;
                        }
                        
                        if (grid[i].tileSprite.getGlobalBounds().contains(grid[bombIndex].position + sf::Vector2f({ -config.tileWidth, config.tileWidth }))) 
                        {
                            grid[i].markForDeath();
                            continue;
                        }
                        if (grid[i].tileSprite.getGlobalBounds().contains(grid[bombIndex].position + sf::Vector2f({ 0, config.tileWidth }))) 
                        {
                            grid[i].markForDeath();
                            continue;
                        }
                        if (grid[i].tileSprite.getGlobalBounds().contains(grid[bombIndex].position + sf::Vector2f({ config.tileWidth, config.tileWidth }))) 
                        {
                            grid[i].markForDeath();
                            continue;
                        }


                    }
                }
            }

			// cleanup
			for (int i = 0; i < grid.size(); i++)
			{
				if (grid[i].isDead())
				{

                    //create explosion
                    ParticleProperties props;
                    props.position = { grid[i].position };
                    props.velocity = { 0, 0 };
                    props.acceleration = { 0, 0 };
                    props.lifetime = 0.5f;
                    props.color = sf::Color::Yellow;
                    props.textureCoords.a = { 0, 0 };
                    props.textureCoords.b = { 47, 0 };
                    props.textureCoords.c = { 47, 47 };
                    props.textureCoords.d = { 0, 47 };
                    props.size = { 2, 2 };
                    props.startingAlpha = 256;
                    props.endAlpha = 0;

                    BaseEmitter* explosionEmitter = new ExplosionEmitter(props, 100);
                    ParticleSystem* psExplosion = new ParticleSystem(props, explosionEmitter, 1.0f, textures.redTexture);
                    psExplosion->emitter->init(*psExplosion);
                    explosions.push_back(psExplosion);

                    // game cleanup
					grid.erase(grid.begin() + i);
                    matchedTileCount++;
					i--;
					collapseNeeded = true;
				}
			}

            for (int i = 0; i < explosions.size(); i++)
            {
                if (explosions[i]->isDead())
                {
                    explosions.erase(explosions.begin() + i);
                    i--;
                }
            }

            if (matchedTileCount >= 3)
            {
                if (gridResetRequired)
                {
                    gridResetRequired = false;
                }
                else
                {
                    std::cout << "Score to be added: " << matchedTileCount - 2 << std::endl;
                    eventWatcher.notify(new Event(Event::EventType::EventMatch, matchedTileCount - 2));
                }
            }

			// collapse
			if (collapseNeeded && coyoteTime <= 0.0f)
			{
				std::cout << "Collapse required" << std::endl;
                collapseNeeded = false;
				for (int i = 0; i < config.gridWidth; i++)
				{
					int needed{ 0 };

					for (int j = config.gridHeight - 1; j >= 0; j--)
					{
						bool found{ false };

						for (int k = 0; k < grid.size(); k++)
						{
							if (grid[k].tileSprite.getGlobalBounds().contains(sf::Vector2f(config.minx + i * config.tileWidth, config.miny + j * config.tileWidth)))
							{
								found = true;
								grid[k].move(sf::Vector2f({ config.minx + i * config.tileWidth, config.miny + (j + needed) * config.tileWidth }), config.swapDuration);
							}
						}

						if (!found)
						{
							needed++;
						}
					}

                    // generate tiles required to fill grid
					if (needed > 0) std::cout << "Needed tiles: " << needed << std::endl;
					for (int l = 1; l <= needed; l++)
					{
                        std::vector<int> possibleTypes;
                        for (int k = 0; k < config.tileTypes; k++) possibleTypes.push_back(k);
                        possibleTypes.pop_back(); // remove bomb tile
                        possibleTypes.pop_back(); // remove wildcard tile
                        int selector = rand() % possibleTypes.size();
                        createdTiles+=1.0f;
                        if ((rand() % 100) >= 95)
                        {
                            selector = (int)Tile::TileType::WILDCARD;
                            createdWildcardTiles+=1.0f;
                        }
                        if (powerUpTracker >= config.powerUpBomb)
                        {
                            selector = (int)Tile::TileType::BOMB;
                            powerUpTracker = 0;
                        }
                        //std::cout << "Wildcard tiles: " << createdWildcardTiles<<"/"<< createdTiles << "%" << std::endl;
						Tile::TileType t = Tile::TileType(selector);
						Tile tile(t, sf::Vector2f({ config.minx + i * config.tileWidth, config.miny - l * config.tileWidth }), sf::Vector2f({ config.tileWidth, config.tileWidth }));
						tile.move(sf::Vector2f({ config.minx + i * config.tileWidth, config.miny + (needed - l) * config.tileWidth }), (1 + 2*i/config.gridWidth + (float)l/needed) * config.swapDuration);
						grid.push_back(tile);
					}
				}
			}

            if (!matchPossible(grid, config))
            {
                //fillNewGrid(grid, config);
                for (int i = 0; i < grid.size(); i++)
                {
                    grid[i].markForDeath();
                    gridResetRequired = true;
                }
            }

            // reverse move if no match
            if (swapMatchCheck && lockInput == 0.0f)
            {
                grid[swappedFromTileIndex].move(grid[swappedToTileIndex].position, config.swapDuration);
                grid[swappedToTileIndex].move(grid[swappedFromTileIndex].position, config.swapDuration);
                lockInput += config.swapDuration;
                swapMatchCheck = false;
            }
        }

        // drawing
        scoreText.setString(std::to_string(scoreboard.score));

        window.clear();
        //window.draw(cornerCheck);
        window.draw(gameAssets.backgroundSprite);
        window.draw(gameAssets.scoreSprite);
        for (int i = 0; i < grid.size(); i++)
        {
            window.draw(grid[i]);
        }
        for (int i = 0; i < explosions.size(); i++)
        {
            window.draw(*explosions[i]);
        }

        window.draw(scoreText);
        window.draw(helpText);
        window.display();
    }

    return 0;
}