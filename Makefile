CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
LDFLAGS = -lglfw -lGLEW -lGL -ldl

# Ficheiros
SOURCES = main.cpp AStar.cpp
EXECUTABLE = a_star_3d

# Includes
INCLUDES = -I/usr/include -I. -I/usr/local/include

all: $(EXECUTABLE)

# Compila diretamente todos os ficheiros .cpp para o executável final
$(EXECUTABLE): $(SOURCES)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SOURCES) -o $@ $(LDFLAGS)
	@echo "✓ Build concluído: $@"

run: all
	./$(EXECUTABLE)

clean:
	rm -f $(EXECUTABLE)
	@echo "✓ Limpeza concluída"

.PHONY: all run clean