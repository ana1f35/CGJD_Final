#include "AStar.h"

#include <algorithm>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <queue>
#include <limits>
#include <cmath>

Pathfinder::Pathfinder() : rng(std::random_device{}()) {}

void Pathfinder::resetGrid() {
    isResetting = true;
    
    openList.clear();
    isSearching = false;
    pathFound = false;
    noPath = false;

    bool validMapFound = false;
    int attempts = 0;
    const int MAX_ATTEMPTS = 10;

    const std::vector<std::string> mapFiles = getMapFiles();

    while (!validMapFound && attempts < MAX_ATTEMPTS && !mapFiles.empty()) {
        attempts++;
        std::uniform_int_distribution<int> fileDist(0, static_cast<int>(mapFiles.size()) - 1);
        const std::string& mapPath = mapFiles[fileDist(rng)];

        if (validateMapForPositions(mapPath) && loadSpecificMap(mapPath)) {
            validMapFound = true;
        }
    }

    if (!validMapFound) {
        generateRandomGrid();
    }

    configureStartAndGoal();
    isResetting = false;
}

void Pathfinder::step() {
    if (!isSearching || pathFound || noPath) return;

    if (openList.empty()) {
        noPath = true;
        isSearching = false;
        return;
    }

    int currentIdx = 0;
    for (size_t i = 1; i < openList.size(); ++i) {
        const Node& a = grid[openList[i].first][openList[i].second];
        const Node& b = grid[openList[currentIdx].first][openList[currentIdx].second];
        
        if (a.fCost < b.fCost || (a.fCost == b.fCost && a.hCost < b.hCost)) {
            currentIdx = i;
        }
    }

    int cx = openList[currentIdx].first;
    int cz = openList[currentIdx].second;

    openList[currentIdx] = openList.back();
    openList.pop_back();

    grid[cx][cz].state = CLOSED;

    if (cx == endX && cz == endZ) {
        pathFound = true;
        isSearching = false;
        reconstructPath();
        return;
    }

    const int dx[] = {-1, 1, 0, 0};
    const int dz[] = {0, 0, -1, 1};

    for (int i = 0; i < 4; i++) {
        int nx = cx + dx[i];
        int nz = cz + dz[i];

        if (inBounds(nx, nz)) {
            if (grid[nx][nz].type == WALL || grid[nx][nz].state == CLOSED) continue;

            int newCostToNeighbor = grid[cx][cz].gCost + 10;
            bool inOpenList = (grid[nx][nz].state == OPEN);

            if (newCostToNeighbor < grid[nx][nz].gCost || !inOpenList) {
                grid[nx][nz].gCost = newCostToNeighbor;
                // Otimização "Greedy" (tie-breaker): Multiplicar por 11 em vez de 10 reduz a exploração excessiva
                grid[nx][nz].hCost = (std::abs(nx - endX) + std::abs(nz - endZ)) * 11; 
                grid[nx][nz].fCost = grid[nx][nz].gCost + grid[nx][nz].hCost;
                
                grid[nx][nz].parentX = cx;
                grid[nx][nz].parentZ = cz;

                if (!inOpenList) {
                    grid[nx][nz].state = OPEN;
                    openList.push_back({nx, nz});
                }
            }
        }
    }
}

void Pathfinder::startInstantSearchAndAnimate() {
    isSearching = true;
    auto startTime = std::chrono::high_resolution_clock::now();

    while (!pathFound && !noPath && !openList.empty()) {
        step();
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    
    // Calcula a diferença num formato de ponto flutuante (double) em milissegundos
    std::chrono::duration<double, std::milli> ms_double = endTime - startTime;
    double duration = ms_double.count();

    std::cout << "-----------------------------------" << std::endl;
    if (pathFound) {
        std::cout << "[A*] Caminho encontrado em: " << duration << " ms!" << std::endl;
    } else {
        std::cout << "[A*] Mapa impossível. Tempo de deteção: " << duration << " ms!" << std::endl;
    }

    for (int x = 0; x < mapWidth; ++x) {
        for (int z = 0; z < mapHeight; ++z) {
            if (grid[x][z].state == PATH) {
                grid[x][z].isFinalPath = true;
            }
        }
    }

    openList.clear();
    pathFound = false;
    noPath = false;
    isSearching = true; 

    for (int x = 0; x < mapWidth; ++x) {
        for (int z = 0; z < mapHeight; ++z) {
            grid[x][z].gCost = 1000000;
            grid[x][z].hCost = 0;
            grid[x][z].fCost = 0;
            grid[x][z].state = UNVISITED;
        }
    }

    grid[startX][startZ].gCost = 0;
    grid[startX][startZ].hCost = (std::abs(startX - endX) + std::abs(startZ - endZ)) * 11;
    grid[startX][startZ].fCost = grid[startX][startZ].hCost;
    grid[startX][startZ].state = OPEN;
    openList.push_back({startX, startZ});
}

bool Pathfinder::inBounds(int x, int z) const {
    return x >= 0 && x < mapWidth && z >= 0 && z < mapHeight;
}

void Pathfinder::initializeGridCell(int x, int z) {
    grid[x][z] = {x, z, 1000000, 0, 0, x, z, EMPTY, UNVISITED, false};
}

void Pathfinder::reconstructPath() {
    int currX = endX, currZ = endZ;
    while (!(currX == startX && currZ == startZ)) {
        grid[currX][currZ].state = PATH;
        int px = grid[currX][currZ].parentX;
        int pz = grid[currX][currZ].parentZ;
        currX = px; currZ = pz;
    }
    grid[startX][startZ].state = PATH;
}

int Pathfinder::countWalkableNeighbors(int x, int z) const {
    const int dx[] = {-1, 1, 0, 0};
    const int dz[] = {0, 0, -1, 1};
    int count = 0;

    for (int i = 0; i < 4; ++i) {
        const int nx = x + dx[i];
        const int nz = z + dz[i];
        if (inBounds(nx, nz) && grid[nx][nz].type == EMPTY) {
            ++count;
        }
    }
    return count;
}

bool Pathfinder::chooseStartAndGoalFromMap() {
    std::vector<std::pair<int, int>> preferredStarts, fallbackStarts;

    for (int x = 0; x < mapWidth; ++x) {
        for (int z = 0; z < mapHeight; ++z) {
            if (grid[x][z].type != EMPTY) continue;

            const int neighborCount = countWalkableNeighbors(x, z);
            if (neighborCount >= 2) {
                preferredStarts.push_back({x, z});
            } else if (neighborCount >= 1) {
                fallbackStarts.push_back({x, z});
            }
        }
    }

    const auto* candidates = preferredStarts.empty() ? &fallbackStarts : &preferredStarts;
    if (candidates->empty()) return false;

    std::uniform_int_distribution<int> startDist(0, static_cast<int>(candidates->size()) - 1);
    std::pair<int, int> start = (*candidates)[startDist(rng)];

    std::queue<std::pair<int, int>> bfsQueue;
    std::vector<std::vector<int>> distance(mapWidth, std::vector<int>(mapHeight, -1));

    distance[start.first][start.second] = 0;
    bfsQueue.push(start);

    std::pair<int, int> goal = start;
    int bestDistance = 0;
    const int dx[] = {-1, 1, 0, 0}, dz[] = {0, 0, -1, 1};

    while (!bfsQueue.empty()) {
        auto current = bfsQueue.front();
        bfsQueue.pop();

        int currentDistance = distance[current.first][current.second];
        if (currentDistance > bestDistance) {
            bestDistance = currentDistance;
            goal = current;
        }

        for (int i = 0; i < 4; ++i) {
            int nx = current.first + dx[i];
            int nz = current.second + dz[i];
            if (inBounds(nx, nz) && grid[nx][nz].type == EMPTY && distance[nx][nz] == -1) {
                distance[nx][nz] = currentDistance + 1;
                bfsQueue.push({nx, nz});
            }
        }
    }

    startX = start.first; startZ = start.second;
    endX = goal.first; endZ = goal.second;
    return true;
}

void Pathfinder::configureStartAndGoal() {
    if (!chooseStartAndGoalFromMap()) {
        startX = 1; startZ = 1;
        endX = std::max(0, mapWidth - 2);
        endZ = std::max(0, mapHeight - 2);
    }

    if (inBounds(startX, startZ)) grid[startX][startZ].type = EMPTY;
    if (inBounds(endX, endZ)) grid[endX][endZ].type = EMPTY;

    grid[startX][startZ].gCost = 0;
    grid[startX][startZ].hCost = (std::abs(startX - endX) + std::abs(startZ - endZ)) * 11;
    grid[startX][startZ].fCost = grid[startX][startZ].hCost;
    grid[startX][startZ].state = OPEN;

    openList.push_back({startX, startZ});
}

void Pathfinder::generateRandomGrid() {
    grid.assign(FALLBACK_GRID_SIZE, std::vector<Node>(FALLBACK_GRID_SIZE));
    mapWidth = FALLBACK_GRID_SIZE;
    mapHeight = FALLBACK_GRID_SIZE;

    for (int x = 0; x < mapWidth; ++x) {
        for (int z = 0; z < mapHeight; ++z) {
            initializeGridCell(x, z);
            grid[x][z].type = (std::uniform_int_distribution<int>(0, 99)(rng) < 22) ? WALL : EMPTY;
        }
    }
}

bool Pathfinder::isEligibleMapFile(const std::string& mapPath) const {
    std::ifstream file(mapPath);
    if (!file.is_open()) return false;

    std::string line;
    int width = 0, height = 0;

    if (!std::getline(file, line) || line.rfind("type ", 0) != 0) return false;
    if (!std::getline(file, line) || line.rfind("height ", 0) != 0) return false;
    height = std::stoi(line.substr(7));
    if (!std::getline(file, line) || line.rfind("width ", 0) != 0) return false;
    width = std::stoi(line.substr(6));

    return width > 0 && height > 0 && width <= 150 && height <= 150;
}

std::vector<std::string> Pathfinder::getMapFiles() const {
    std::vector<std::string> mapFiles;
    DIR* directory = opendir("map");
    if (!directory) return mapFiles;

    while (dirent* entry = readdir(directory)) {
        std::string fileName = entry->d_name;
        if (fileName.size() > 4 && fileName.substr(fileName.size() - 4) == ".map") {
            std::string mapPath = "map/" + fileName;
            if (isEligibleMapFile(mapPath)) {
                mapFiles.push_back(mapPath);
            }
        }
    }
    closedir(directory);
    return mapFiles;
}

bool Pathfinder::validateMapForPositions(const std::string& mapPath) const {
    std::ifstream file(mapPath);
    if (!file.is_open()) return false;

    std::string line;
    int width = 0, height = 0;

    if (!std::getline(file, line) || line.rfind("type ", 0) != 0) return false;
    if (!std::getline(file, line) || line.rfind("height ", 0) != 0) return false;
    height = std::stoi(line.substr(7));
    if (!std::getline(file, line) || line.rfind("width ", 0) != 0) return false;
    width = std::stoi(line.substr(6));
    if (!std::getline(file, line) || line != "map") return false;

    if (width <= 0 || height <= 0 || width > 150 || height > 150) return false;

    std::vector<std::string> rows;
    rows.reserve(height);
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        rows.push_back(line);
    }

    if (static_cast<int>(rows.size()) < height) return false;

    std::vector<std::pair<int, int>> candidateStarts;
    const int dx[] = {-1, 1, 0, 0};
    const int dz[] = {0, 0, -1, 1};

    for (int x = 0; x < width; ++x) {
        for (int z = 0; z < height; ++z) {
            if (x < static_cast<int>(rows[z].size()) && rows[z][x] == '.') {
                int neighbors = 0;
                for (int i = 0; i < 4; ++i) {
                    int nx = x + dx[i], nz = z + dz[i];
                    if (nx >= 0 && nx < width && nz >= 0 && nz < height && rows[nz][nx] == '.') {
                        neighbors++;
                    }
                }
                if (neighbors >= 2) candidateStarts.push_back({x, z});
            }
        }
    }
    return !candidateStarts.empty();
}

bool Pathfinder::loadSpecificMap(const std::string& mapPath) {
    std::ifstream file(mapPath);
    if (!file.is_open()) return false;

    std::string line;
    int width = 0, height = 0;

    if (!std::getline(file, line) || line.rfind("type ", 0) != 0) return false;
    if (!std::getline(file, line) || line.rfind("height ", 0) != 0) return false;
    height = std::stoi(line.substr(7));
    if (!std::getline(file, line) || line.rfind("width ", 0) != 0) return false;
    width = std::stoi(line.substr(6));
    if (!std::getline(file, line) || line != "map") return false;

    std::vector<std::string> rows;
    rows.reserve(height);
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        rows.push_back(line);
    }

    mapWidth = width;
    mapHeight = height;
    grid.assign(mapWidth, std::vector<Node>(mapHeight));

    for (int x = 0; x < mapWidth; ++x) {
        for (int z = 0; z < mapHeight; ++z) {
            initializeGridCell(x, z);
            const char cell = (x < static_cast<int>(rows[z].size())) ? rows[z][x] : 'T';
            grid[x][z].type = (cell == '.') ? EMPTY : WALL;
        }
    }
    return true;
}