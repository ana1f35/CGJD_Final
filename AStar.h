#pragma once

#include <vector>
#include <string>
#include <random>
#include <chrono>

/**
 * Representa os possíveis tipos de terreno numa célula.
 */
enum CellType { EMPTY, WALL };

/**
 * Representa o estado atual da célula durante a execução do algoritmo A*.
 */
enum CellState { UNVISITED, OPEN, CLOSED, PATH };

/**
 * Estrutura de um nó da grelha (célula).
 */
struct Node {
    int x, z;
    int gCost, hCost, fCost;
    int parentX, parentZ;
    CellType type;
    CellState state;
    bool isFinalPath; 
};

/**
 * Classe responsável por gerir a grelha e a lógica de pathfinding (A*).
 */
class Pathfinder {
public:
    std::vector<std::vector<Node>> grid;
    
    int mapWidth = 0;
    int mapHeight = 0;
    int startX = 1, startZ = 1;
    int endX = 0, endZ = 0;

    bool isSearching = false;
    bool pathFound = false;
    bool noPath = false; 
    bool isResetting = false;

    Pathfinder();

    void resetGrid();
    void step();
    void startInstantSearchAndAnimate();

private:
    std::vector<std::pair<int, int>> openList;
    std::mt19937 rng;
    const int FALLBACK_GRID_SIZE = 25;

    bool inBounds(int x, int z) const;
    void initializeGridCell(int x, int z);
    void reconstructPath();
    int countWalkableNeighbors(int x, int z) const;
    bool chooseStartAndGoalFromMap();
    void configureStartAndGoal();
    void generateRandomGrid();
    bool isEligibleMapFile(const std::string& mapPath) const;
    std::vector<std::string> getMapFiles() const;
    bool validateMapForPositions(const std::string& mapPath) const;
    bool loadSpecificMap(const std::string& mapPath);
};