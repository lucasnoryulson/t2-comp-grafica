# Makefile para Windows

PROG = ProgOpenGL.exe

#FONTES = Ponto.cpp Poligono.cpp Temporizador.cpp ListaDeCoresRGB.cpp SimuladorDeCidade-VersaoBasica2025.cpp

FONTES = SimuladorDeCidade-VersaoBasica2025.cpp Ponto.cpp ImageClass.cpp Texture.cpp Temporizador.cpp ListaDeCoresRGB.cpp SOIL/SOIL.cpp SOIL/image_DXT.cpp SOIL/image_helper.cpp SOIL/stb_image_aug.cpp

OBJETOS = $(FONTES:.cpp=.cpp)

CFLAGS = -g -Iinclude\GL  -Wno-write-strings -Wno-narrowing # -Wall -g  # Todas as warnings, infos de debug
#LDFLAGS = -Llib\GL -lfreeglut -lopengl32 -lglu32 -lm
LDFLAGS = -Llib -lfreeglut -lopengl32 -lglu32 -lm
CC = g++

$(PROG): $(OBJETOS)
	g++ $(CFLAGS) $(OBJETOS) -o $@ $(LDFLAGS)

clean:
	rm $(OBJETOS) $(PROG)
