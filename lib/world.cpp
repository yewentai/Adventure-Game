﻿#include "world.h"
#include <QDateTime>
#include <QImage>
#include <QRgb>
#include <QTimer>
#include <QtDebug>
#include <ctime>
#include <iostream>
#include <limits>
#include <random>
#include <sstream>

constexpr float maxEH{100.0f};

//class Tile
Tile::Tile(int xPosition, int yPosition, float tileWeight)
    : xPos{xPosition}, yPos{yPosition}, value{tileWeight}
{}

std::string Tile::serialize()
{
  std::stringstream res;
  res << xPos << "," << yPos << "," << value;
  return res.str();
}

//class Enemy
Enemy::Enemy(int xPosition, int yPosition, float strength)
    : Tile(xPosition, yPosition, strength)
{}

std::string Enemy::serialize()
{
  std::stringstream res;
  res << Tile::serialize() << "," << defeated;
  return res.str();
}

//class PEnemy
PEnemy::PEnemy(int xPosition, int yPosition, float strength)
    : Enemy(xPosition, yPosition, strength)
    , poisonLevel{strength}
{
  srand(time(nullptr));
}

bool PEnemy::poison()
{
  poisonLevel -= 10.0f;
  if (poisonLevel > 0.0f)
  {
    emit poisonLevelUpdated(poisonLevel);
    int t = rand() % 5;
    std::cout << "starting timer for " << t << " seconds"
              << " with poisonLevel = " << poisonLevel << std::endl;
    QTimer::singleShot(t * 1000, this, SLOT(poison()));
    return true;
  }
  else
  {
    poisonLevel = 0.0f;
    setDefeated(true);
    emit dead();
  }
  return false;
}

float PEnemy::getPoisonLevel() const
{
  return poisonLevel;
}

void PEnemy::setPoisonLevel(float value)
{
  poisonLevel = value;
}

std::string PEnemy::serialize()
{
  std::stringstream res;
  res << Enemy::serialize() << "," << poisonLevel;
  return res.str();
}

//class Protagonist
Protagonist::Protagonist()
    : Tile(0, 0, 0.0F)
    , health{maxEH}
    , energy{maxEH}
{}

std::string Protagonist::serialize()
{
  std::stringstream res;
  res << Tile::serialize() << "," << health << "," << energy;
  return res.str();
}

//class World
// methods throws a std::logic_error exception when file filename is not found
void World::createWorld(QString filename,
                        unsigned int nrOfEnemies,
                        unsigned int nrOfHealthpacks,
                        float pRatio)
{
  std::vector<bool> used;
  //read image in, every pixel greyvalue is value of tile, black pixels are considered as impassable, gets value infinity
  world.load(filename);
  if (world.isNull())
    throw std::logic_error(std::string("file " + filename.toStdString() + " not found"));
  rows = world.height();
  cols = world.width();
  tiles.reserve(rows * cols);
  used.reserve(rows * cols);
  for (int row = 0; row < rows; row++) {
    for (int col = 0; col < cols; col++) {
      float val = qGray(world.pixel(col, row)) / 255.0F;
      if (val > 0.0F)
        tiles.emplace_back(new Tile(col, row, val));
      else
        tiles.emplace_back(new Tile(col, row, std::numeric_limits<float>::infinity()));
    }
  }
  //generate enemies
  const unsigned int black{0xff000000};
  std::random_device r;

  // Choose a random distribution for x, y and strength
  std::default_random_engine e1(r());
  std::uniform_int_distribution<int> uniform_x(0, cols - 1);
  std::uniform_int_distribution<int> uniform_y(0, rows - 1);
  std::uniform_int_distribution<int> uniform_perc(0, 100);

  //use array used to check with positions are already occupied by E or HP
  enemies.reserve(nrOfEnemies);
  int PEnemyCounter = 0;
  while (enemies.size() < nrOfEnemies) {
    int xPos = uniform_x(e1);
    int yPos = uniform_y(e1);
    if (world.pixel(xPos, yPos) == black)
      continue;
    std::unique_ptr<Enemy> eptr;
    int dummy = static_cast<int>(1.0 / pRatio);
    if ((xPos + yPos) % dummy != 0)
    {
      eptr = std::make_unique<Enemy>(xPos, yPos, uniform_perc(e1));
    }
    else
    {
      eptr = std::make_unique<PEnemy>(xPos, yPos, uniform_perc(e1));
      PEnemyCounter++;
    }
    if (xPos != 0 || yPos != 0) {
      if (used[yPos * cols + xPos] == false)
      {
        enemies.push_back(std::move(eptr));
        used[yPos * cols + xPos] = true;
      }
    }
  }
  //be sure at least 1 PEnemy in the collection
  if (PEnemyCounter == 0 && nrOfEnemies > 0) {
    auto &en = enemies.at(nrOfEnemies - 1);
    int xPos = en->getXPos();
    int yPos = en->getYPos();
    float val = en->getValue();
    enemies.pop_back();
    auto eptr = std::make_unique<PEnemy>(xPos, yPos, val);
    enemies.push_back(std::move(eptr));
  }

  healthPacks.reserve(nrOfHealthpacks);
  while (healthPacks.size() < nrOfHealthpacks) {
    int xPos = uniform_x(e1);
    int yPos = uniform_y(e1);
    if (world.pixel(xPos, yPos) == black)
      continue;
    auto hptr = std::make_unique<Tile>(xPos, yPos, uniform_perc(e1));
    if (xPos != 0 || yPos != 0) {
      if (used[yPos * cols + xPos] == false) {
        healthPacks.push_back(std::move(hptr));
        used[yPos * cols + xPos] = true;
      }
    }
  }
}

std::vector<std::unique_ptr<Tile>> World::getTiles()
{
  if (world.isNull())
    throw "No tiles created yet";
  return std::move(tiles);
}

std::vector<std::unique_ptr<Enemy>> World::getEnemies()
{
  if (world.isNull())
    throw "No enemies created yet";
  return std::move(enemies);
}

std::vector<std::unique_ptr<Tile>> World::getHealthPacks()
{
  if (world.isNull())
    throw "No healthpacks created yet";
  return std::move(healthPacks);
}

std::unique_ptr<Protagonist> World::getProtagonist() const
{
  return std::make_unique<Protagonist>();
}
