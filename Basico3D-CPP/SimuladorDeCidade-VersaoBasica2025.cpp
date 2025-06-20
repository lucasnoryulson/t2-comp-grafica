// **********************************************************************
// PUCRS/Escola Politecnica
// COMPUTACAO GRAFICA
//
// Programa basico para criar aplicacoes 3D em OpenGL
//
// Marcio Sarroglia Pinho
// pinho@pucrs.br
// **********************************************************************
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <iostream>
#include <cmath>
#include <ctime>
#include <cfloat>  // Para isnan()

using namespace std;
#define Gray 8  // ou outro valor livre e uma cor apropriada no array de cores


#ifdef WIN32
#include <windows.h>
#include <glut.h>
#else
#include <sys/time.h>
#endif

#ifdef __APPLE__
#include <GLUT/glut.h>
#endif

#ifdef __linux__
#include <GL/glut.h>
#endif

#include "Temporizador.h"
Temporizador T;
double AccumDeltaT=0;

#include "Ponto.h"
#include "ListaDeCoresRGB.h"
#include "Texture.h"

GLfloat AnguloDeVisao=90;
GLfloat AspectRatio, angulo=0;
GLfloat AlturaViewportDeMensagens = 0.2; // percentual em relacao � altura da tela


// Controle do modo de projecao
// 0: Projecao Paralela Ortografica; 1: Projecao Perspectiva
// A funcao "PosicUser" utiliza esta variavel. O valor dela eh alterado
// pela tecla 'p'

int ModoDeProjecao = 1;

// Controle do modo de projecao
// 0: Wireframe; 1: Faces preenchidas
// A funcao "Init" utiliza esta variavel. O valor dela eh alterado
// pela tecla 'w'
int ModoDeExibicao = 1;

double nFrames=0;
double TempoTotal=0;

// Qtd de ladrilhos do piso. Inicialzada na INIT
int QtdX;
int QtdZ;


// Representa o conteudo de uma celula do piso
class Elemento{
public:
    int tipo;
    int corDoObjeto;
    int corDoPiso;
    float altura;
    int textureID;
};

// codigos que definem o o tipo do elemento que esta em uma celula
#define VAZIO 0
#define PREDIO 10
#define RUA 20
#define COMBUSTIVEL 30
#define VEICULO 40

// Matriz que armazena informacoes sobre o que existe na cidade
Elemento Cidade[100][100];


Ponto Observador, Alvo, TerceiraPessoa, PosicaoVeiculo;

bool ComTextura = true;

// ===== VARIÁVEIS DE CONTROLE DO VEÍCULO =====
// Posição e direção do veículo
int DirecaoVeiculo = 0;  // 0 = norte, 1 = leste, 2 = sul, 3 = oeste
float VelocidadeVeiculo = 0.0f;  // Velocidade atual (0 = parado, 100 = máxima)
bool VeiculoEmMovimento = false;  // Se o veículo está se movendo
float VelocidadeMaxima = 8.0f;  // Velocidade em unidades/segundo

// Combustível
float CombustivelAtual = 100.0f;  // Combustível atual (0-100%)
float ConsumoCombustivel = 1.0f;  // Consumo por segundo

// Modo de visualização
bool ModoPrimeiraPessoa = false;  // true = primeira pessoa, false = terceira pessoa

// ===== DECLARAÇÕES DAS FUNÇÕES DE CONTROLE DO VEÍCULO =====
void AtualizarPosicaoVeiculo(float deltaTime);
void AlternarModoVisualizacao();
void DesenharVeiculo();
Ponto EncontrarPosicaoInicialValida();
void AtualizaCamera();
bool VerificarColisao(float x, float z);

// **********************************************************************
//
// **********************************************************************
void PosicionaEmTerceiraPessoa();
void lerMapa(const char* nomeArquivo);
void renderizarMapa();
void desenharPredio(float altura);
void desenharPlanoComTextura(int texturaID);
void desenharPlanoVerde();

void ImprimeCidade() {
    for (int i = 0; i < QtdZ; i++) {
        for (int j = 0; j < QtdX; j++) {
            cout << Cidade[i][j].corDoPiso << " ";
        }
        cout << endl;
    }
}

// **********************************************************************
// void InicializaCidade(int QtdX, int QtdZ)
// Esta funcao sera substituida por uma que le a matriz que representa
// a Cidade
// **********************************************************************




void InicializaCidade(int qtdX, int qtdZ)
{
    ifstream arquivo("cidade.txt");
    if (!arquivo)
    {
        cerr << "Erro ao abrir o arquivo cidade.txt\n";
        exit(1);
    }

    string linha;
    int z = 0;

    while (getline(arquivo, linha) && z < qtdZ)
    {
        stringstream ss(linha);
        string valor;
        int x = 0;

        while (getline(ss, valor, ',') && x < qtdX)
        {
            int tipo = stoi(valor);
            Cidade[z][x].tipo = tipo;

            switch (tipo)
            {
                case 0: // rua
                    Cidade[z][x].tipo = RUA;
                    Cidade[z][x].corDoPiso = Gray;
                    break;
                case 1: // grama / quadra
                    Cidade[z][x].tipo = VAZIO;
                    Cidade[z][x].corDoPiso = Green;
                    break;
                case 2: // rio
                    Cidade[z][x].tipo = VAZIO;
                    Cidade[z][x].corDoPiso = Blue;
                    break;
                case 3: // prédio vermelho
                    Cidade[z][x].tipo = PREDIO;
                    Cidade[z][x].corDoPiso = Green;
                    Cidade[z][x].corDoObjeto = Red;
                    Cidade[z][x].altura = 3.0;
                    break;
                case 4: // prédio amarelo
                    Cidade[z][x].tipo = PREDIO;
                    Cidade[z][x].corDoPiso = Green;
                    Cidade[z][x].corDoObjeto = Yellow;
                    Cidade[z][x].altura = 2.0;
                    break;
                case 5: // cápsula de combustível
                    Cidade[z][x].tipo = COMBUSTIVEL;
                    Cidade[z][x].corDoPiso = Gray;
                    break;
                default:
                    Cidade[z][x].tipo = VAZIO;
                    Cidade[z][x].corDoPiso = White;
                    break;
            }
            x++;
        }
        z++;
    }
    arquivo.close();

    // Adicionar algumas cápsulas de combustível em posições estratégicas
    // Cápsula 1: no centro da cidade
    if (Cidade[25][25].tipo == RUA)
    {
        Cidade[25][25].tipo = COMBUSTIVEL;
        cout << "Cápsula de combustível adicionada na posição (25, 25)" << endl;
    }
    
    // Cápsula 2: na parte superior
    if (Cidade[10][25].tipo == RUA)
    {
        Cidade[10][25].tipo = COMBUSTIVEL;
        cout << "Cápsula de combustível adicionada na posição (25, 10)" << endl;
    }
    
    // Cápsula 3: na parte inferior
    if (Cidade[40][25].tipo == RUA)
    {
        Cidade[40][25].tipo = COMBUSTIVEL;
        cout << "Cápsula de combustível adicionada na posição (25, 40)" << endl;
    }
    
    // Cápsula 4: na lateral esquerda
    if (Cidade[25][10].tipo == RUA)
    {
        Cidade[25][10].tipo = COMBUSTIVEL;
        cout << "Cápsula de combustível adicionada na posição (10, 25)" << endl;
    }
    
    // Cápsula 5: na lateral direita
    if (Cidade[25][40].tipo == RUA)
    {
        Cidade[25][40].tipo = COMBUSTIVEL;
        cout << "Cápsula de combustível adicionada na posição (40, 25)" << endl;
    }

    cout << "Mapa carregado com sucesso!" << endl;

    QtdX = qtdX;
    QtdZ = qtdZ;

    TerceiraPessoa = Ponto(QtdX / 2, 10, QtdZ * 1.1);
    PosicaoVeiculo = EncontrarPosicaoInicialValida();
    PosicionaEmTerceiraPessoa();
}



// **********************************************************************
// void PosicionaEmTerceiraPessoa()
//   Este metodo posiciona o obeservador fora do cenario, olhando para o
// centro do mapa
// As variaveis "TerceiraPessoa" e "PosicaoVeiculo" sao setadas na INIT
// **********************************************************************
void PosicionaEmTerceiraPessoa()
{
    Observador = TerceiraPessoa;   // Posicao do Observador
    Alvo = PosicaoVeiculo;         // Posicao do Alvo
    //Alvo.imprime("Posiciona - Veiculo:");
}

// **********************************************************************
//  void init(void)
//    Inicializa os parametros globais de OpenGL
// **********************************************************************
void init(void)
{
    glClearColor(0.0f, 0.0f, 1.0f, 1.0f); // Fundo de tela preto
   
    glClearDepth(1.0);
    glDepthFunc(GL_LESS);
    glShadeModel(GL_SMOOTH); // (GL_FLAT);
    glColorMaterial ( GL_FRONT, GL_AMBIENT_AND_DIFFUSE );
    
    glEnable(GL_DEPTH_TEST);
    //glEnable (GL_CULL_FACE);
    
    if (ModoDeExibicao) // Faces Preenchidas??
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    else
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    
    glEnable(GL_NORMALIZE);
    
    // Quantidade de retangulos do piso vai depender do
    // mapa que sera' lido da
    QtdX = 50;
    QtdZ = 50;
    
    InicializaCidade(QtdX, QtdZ);
    
    // Define a posicao do observador e do veiculo
    // com base no tamanho do mapa
    TerceiraPessoa = Ponto(QtdX/2, 10, QtdZ*1.1);
    PosicaoVeiculo = EncontrarPosicaoInicialValida();
    
    PosicionaEmTerceiraPessoa();
    glDisable(GL_TEXTURE_2D);
    
    AnguloDeVisao = 45;
    
    LoadTexture ("bricks.jpg"); // esta serah a textura 0
    LoadTexture ("Piso.jpg"); // esta serah a textura 1
    UseTexture (-1); // desabilita o uso de textura, inicialmente
    
}
// **********************************************************************
//
// **********************************************************************
void animate()
{
    double dt;
    dt = T.getDeltaT();
    AccumDeltaT += dt;
    TempoTotal += dt;
    nFrames++;

    // Atualizar posição do veículo
    AtualizarPosicaoVeiculo(dt);

    // Atualiza a camera
    AtualizaCamera();

    if (AccumDeltaT > 1.0/60) // Aumentar para 60 FPS para movimento mais suave
    {
        AccumDeltaT = 0;
        angulo+= 1;
        glutPostRedisplay();
    }
    if (TempoTotal > 5.0)
    {
        cout << "Tempo Acumulado: "  << TempoTotal << " segundos. " ;
        cout << "Nros de Frames sem desenho: " << nFrames << endl;
        cout << "FPS(sem desenho): " << nFrames/TempoTotal << endl;
        TempoTotal = 0;
        nFrames = 0;
    }
}

// **********************************************************************
//  Desenha um predio no meio de uma clula
// **********************************************************************
void DesenhaPredio(float altura)
{
    glPushMatrix();
       // glTranslatef(0, -1, 0);
        glScalef(0.2, altura, 0.2);
        glTranslatef(0, 1, 0);
        glutSolidCube(1);
    glPopMatrix();
    
}
// **********************************************************************
// void DesenhaLadrilhoTex(int idTextura)
// **********************************************************************
void DesenhaLadrilhoTEX(int idTextura)
{
    UseTexture(idTextura); // Habilita a textura id_textura
    glBegin ( GL_QUADS );
        glNormal3f(0,1,0);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(-0.5f,  0.0f, -0.5f);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(-0.5f,  0.0f,  0.5f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f( 0.5f,  0.0f,  0.5f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f( 0.5f,  0.0f, -0.5f);
    glEnd();
    glDisable(GL_TEXTURE_2D);

}
// **********************************************************************
// void DesenhaPoligonosComTextura()
// **********************************************************************
void DesenhaPoligonosComTextura()
{
    glPushMatrix();
    glTranslatef(QtdX*0.2,1, QtdZ*0.8);
    glRotatef(angulo,1,0,0);
    DesenhaLadrilhoTEX(0);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(QtdX*0.6,1,QtdZ*0.8);
    glRotatef(45,1,0,0);
    DesenhaLadrilhoTEX(1);
    glPopMatrix();

}

// **********************************************************************
// void DesenhaLadrilho(int corBorda, int corDentro)
// Desenha uma celula do piso.
// Eh possivel definir a cor da borda e do interior do piso
// O ladrilho tem largula 1, centro no (0,0,0) e esta sobre o plano XZ
// Para aqueles que forem usar texturas, esta funcao serah substituida
// pela funcao "DesenhaLadrilhoTex"
// **********************************************************************
void DesenhaLadrilho(int corBorda, int corDentro)
{
    defineCor(corDentro); // desenha QUAD preenchido
    glBegin ( GL_QUADS );
        glNormal3f(0,1,0);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(-0.5f,  0.0f, -0.5f);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(-0.5f,  0.0f,  0.5f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f( 0.5f,  0.0f,  0.5f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f( 0.5f,  0.0f, -0.5f);
    glEnd();
    
    defineCor(corBorda);
    glBegin ( GL_LINE_STRIP ); // desenha borda do ladrilho
        glNormal3f(0,1,0);
        glVertex3f(-0.5f,  0.01f, -0.5f);
        glVertex3f(-0.5f,  0.01f,  0.5f);
        glVertex3f( 0.5f,  0.01f,  0.5f);
        glVertex3f( 0.5f,  0.01f, -0.5f);
    glEnd();
}
// **********************************************************************
//
//
// **********************************************************************
void DesenhaCidade(int QtdX, int QtdZ)
{
    srand(100); // usa uma semente fixa para gerar sempre as mesma cores no piso
    glPushMatrix();

    for(int x=0; x<QtdX;x++)
    {
        glPushMatrix();
        for(int z=0; z<QtdZ;z++)
        {
            DesenhaLadrilho(Yellow, Cidade[z][x].corDoPiso);
            glTranslated(0, 0, 1);
            if (Cidade[z][x].tipo == PREDIO)
        {
            defineCor(Cidade[z][x].corDoObjeto);
            glPushMatrix();
            glTranslatef(0, 0, 0);
            DesenhaPredio(Cidade[z][x].altura);
            glPopMatrix();
        }
            else if (Cidade[z][x].tipo == COMBUSTIVEL)
            {
                // Desenhar cápsula de combustível
                defineCor(Yellow);
                glPushMatrix();
                glTranslatef(0, 0.3f, 0);
                glScalef(0.3f, 0.6f, 0.3f);
                glutSolidCube(1.0f);
                glPopMatrix();
            }
        }
        // Aqui, os predios devem ser desenhados
        
        
        glPopMatrix();
        glTranslated(1, 0, 0);
    }
    glPopMatrix();
}

// **********************************************************************
//  void DefineLuz(void)
// **********************************************************************
void DefineLuz(void)
{
  // Define cores para um objeto dourado
  GLfloat LuzAmbiente[]   = {0.4, 0.4, 0.4f } ;
  GLfloat LuzDifusa[]   = {0.7, 0.7, 0.7};
  GLfloat LuzEspecular[] = {0.9f, 0.9f, 0.9 };
    
  GLfloat PosicaoLuz0[]  = {Alvo.x, Alvo.y, Alvo.z };  // Posicao da Luz
  //GLfloat PosicaoLuz0[]  = {0.0f, 3.0f, 5.0f };  // Posicao da Luz
  GLfloat Especularidade[] = {1.0f, 1.0f, 1.0f};

   // ****************  Fonte de Luz 0

  glEnable ( GL_COLOR_MATERIAL );

   // Habilita o uso de iluminacao
  glEnable(GL_LIGHTING);

  // Ativa o uso da luz ambiente
  glLightModelfv(GL_LIGHT_MODEL_AMBIENT, LuzAmbiente);
    
  // Define os parametros da luz numero Zero
  glLightfv(GL_LIGHT0, GL_AMBIENT, LuzAmbiente);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, LuzDifusa  );
  glLightfv(GL_LIGHT0, GL_SPECULAR, LuzEspecular  );
  glLightfv(GL_LIGHT0, GL_POSITION, PosicaoLuz0 );
  glEnable(GL_LIGHT0);

  // Ativa o "Color Tracking"
  glEnable(GL_COLOR_MATERIAL);

  // Define a reflectancia do material
  glMaterialfv(GL_FRONT,GL_SPECULAR, Especularidade);

  // Define a concentracaoo do brilho.
  // Quanto maior o valor do Segundo parametro, mais
  // concentrado sera o brilho. (Valores vlidos: de 0 a 128)
  glMateriali(GL_FRONT,GL_SHININESS,51);

}

// **********************************************************************
//  void PosicUser()
// **********************************************************************
void PosicUser()
{

    // Define os parametros da projecao Perspectiva
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    // Define o volume de visualizacao sempre a partir da posicao do
    // observador
    if (ModoDeProjecao == 0)
        glOrtho(-10, 10, -10, 10, 0, 20); // Projecao paralela Orthografica
    else gluPerspective(AnguloDeVisao,AspectRatio,0.01,1500); // Projecao perspectiva

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    gluLookAt(Observador.x, Observador.y, Observador.z,   // Posicao do Observador
              Alvo.x, Alvo.y, Alvo.z,     // Posicao do Alvo
              0.0f,1.0f,0.0f); // UP


}
// **********************************************************************
//  void reshape( int w, int h )
//		trata o redimensionamento da janela OpenGL
//
// **********************************************************************
void reshape( int w, int h )
{

	// Evita divisao por zero, no caso de uma janela com largura 0.
	if(h == 0) h = 1;
    // Ajusta a relacao entre largura e altura para evitar distorcao na imagem.
    // Veja funcao "PosicUser".
    AspectRatio = (float)w / h; // recalcula o AspectRatio
    
	// Reset the coordinate system before modifying
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
    
	// Ajusta a viewport para ocupar toda a janela
    //glViewport(0, 0, w, h);
    
    // Seta a viewport para ocupar a parte superior da janela
    glViewport(0, h*AlturaViewportDeMensagens, w, h-h*AlturaViewportDeMensagens);
    
    gluPerspective(AnguloDeVisao,AspectRatio,0.01,1500); // Projecao perspectiva

}
// **********************************************************************
//
// **********************************************************************
void printString(string s, int posX, int posY, int cor)
{
    defineCor(cor);
    
    glRasterPos3i(posX, posY, 0); //define posicao na tela
    for (int i = 0; i < s.length(); i++)
    {
//GLUT_BITMAP_HELVETICA_10,
        glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, s[i]);
    }
    
}
// **********************************************************************
// Esta funcao permite que se escreva mensagens na tela
//
// **********************************************************************
void DesenhaEm2D()
{
    int ativarLuz = false;
    if (glIsEnabled(GL_LIGHTING))
    {
        glDisable(GL_LIGHTING);
        ativarLuz = true;
    }
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    
    // Salva o tamanho da janela
    int w = glutGet(GLUT_WINDOW_WIDTH);
    int h = glutGet(GLUT_WINDOW_HEIGHT);
    
    // Define a area a ser ocupada pela area OpenGL dentro da Janela
    glViewport(0, 0, w, h*AlturaViewportDeMensagens); // a janela de mensagens fica na parte de baixo da janela

    // Define os limites logicos da area OpenGL dentro da Janela
    glOrtho(0,10, 0,10, 0,1);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Desenha linha que Divide as áreas 2D e 3D
    defineCor(GreenCopper);
    glLineWidth(15);
    glBegin(GL_LINES);
        glVertex2f(0,10);
        glVertex2f(10,10);
    glEnd();
    
    // Informações do veículo
    printString("=== SIMULADOR DE CIDADE ===", 0, 9, White);
    
    // Status do veículo
    string statusVeiculo = VeiculoEmMovimento ? "MOVIMENTO" : "PARADO";
    string corStatus = VeiculoEmMovimento ? "Green" : "Red";
    printString("Status: " + statusVeiculo, 0, 8, VeiculoEmMovimento ? Green : Red);
    
    // Combustível
    string combustivel = "Combustível: " + to_string((int)CombustivelAtual) + "%";
    string corCombustivel = (CombustivelAtual > 20.0f) ? "Green" : "Red";
    printString(combustivel, 0, 7, CombustivelAtual > 20.0f ? Green : Red);
    
    // Velocidade
    string velocidade = "Velocidade: " + to_string((int)VelocidadeVeiculo) + " m/s";
    printString(velocidade, 0, 6, Yellow);
    
    // Posição
    string posicao = "Pos: (" + to_string((int)PosicaoVeiculo.x) + ", " + to_string((int)PosicaoVeiculo.z) + ")";
    printString(posicao, 0, 5, Cyan);
    
    // Direção
    string direcoes[] = {"Norte", "Leste", "Sul", "Oeste"};
    string direcao = "Direção: " + direcoes[DirecaoVeiculo];
    printString(direcao, 0, 4, Cyan);
    
    // Modo de visualização
    string modoVisao = ModoPrimeiraPessoa ? "1ª PESSOA" : "3ª PESSOA";
    printString("Modo: " + modoVisao, 0, 3, Magenta);
    
    // Controles
    printString("Controles:", 5, 8, White);
    printString("ESPACO: Andar/Parar", 5, 7, White);
    printString("SETAS: Virar", 5, 6, White);
    printString("V: Alternar Visão", 5, 5, White);
    printString("R: Reabastecer", 5, 4, White);
    printString("ESC: Sair", 5, 3, White);

    // Restaura os parametro que foram alterados
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glViewport(0, h*AlturaViewportDeMensagens, w, h-h*AlturaViewportDeMensagens);

    if (ativarLuz)
        glEnable(GL_LIGHTING);

}

// **********************************************************************
//  void display( void )
//
//
// **********************************************************************
void display( void )
{

	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	DefineLuz();

	PosicUser();
    glLineWidth(2);
	
	glMatrixMode(GL_MODELVIEW);

    DesenhaCidade(QtdX,QtdZ);
    
    // Desenhar o veículo
    DesenharVeiculo();
    
    glPushMatrix();
    DesenhaPoligonosComTextura();
    glPopMatrix();

    DesenhaEm2D();

	glutSwapBuffers();
}


// **********************************************************************
//  void keyboard ( unsigned char key, int x, int y )
//
//
// **********************************************************************
void keyboard ( unsigned char key, int x, int y )
{
	switch ( key ) 
	{
    case 27:        // Termina o programa qdo
      exit ( 0 );   // a tecla ESC for pressionada
      break;        
    case 'p':
        ModoDeProjecao = !ModoDeProjecao;
        glutPostRedisplay();
        break;
    case 'e':
        ModoDeExibicao = !ModoDeExibicao;
        init();
        glutPostRedisplay();
        break;
    case 't':
        ComTextura = !ComTextura;
        break;
    case ' ':       // Tecla ESPAÇO - andar/parar
        if (CombustivelAtual > 0.0f)
        {
            VeiculoEmMovimento = !VeiculoEmMovimento;
            if (VeiculoEmMovimento)
            {
                VelocidadeVeiculo = VelocidadeMaxima;
                cout << "Veículo iniciou movimento!" << endl;
            }
            else
            {
                VelocidadeVeiculo = 0.0f;
                cout << "Veículo parado!" << endl;
            }
        }
        else
        {
            cout << "Sem combustível! Reabasteça primeiro." << endl;
        }
        break;
    case 'v':       // Tecla V - alternar visualização
        AlternarModoVisualizacao();
        glutPostRedisplay();
        break;
    case 'r':       // Tecla R - reabastecer (para testes)
        CombustivelAtual = 100.0f;
        cout << "Combustível reabastecido manualmente!" << endl;
        break;
    default:
            cout << key;
    break;
  }
}

// **********************************************************************
//  void arrow_keys ( int a_keys, int x, int y )  
//
//
// **********************************************************************
void arrow_keys ( int a_keys, int x, int y )  
{
	switch ( a_keys ) 
	{
		case GLUT_KEY_UP:       // When Up Arrow Is Pressed...
			glutFullScreen ( ); // Go Into Full Screen Mode
			break;
	    case GLUT_KEY_DOWN:     // When Down Arrow Is Pressed...
			glutInitWindowSize  ( 700, 500 ); 
			break;
		case GLUT_KEY_LEFT:     // Seta esquerda - virar à esquerda
			DirecaoVeiculo = (DirecaoVeiculo - 1 + 4) % 4;  // Virar à esquerda (0->3, 1->0, 2->1, 3->2)
			
			// Verificar se a direção é válida
			if (DirecaoVeiculo < 0 || DirecaoVeiculo > 3)
			{
				cout << "ERRO: Direção do veículo inválida! Resetando..." << endl;
				DirecaoVeiculo = 0;
			}
			
			{
				string direcoes[] = {"Norte", "Leste", "Sul", "Oeste"};
				cout << "Direção: " << direcoes[DirecaoVeiculo] << endl;
			}
			break;
		case GLUT_KEY_RIGHT:    // Seta direita - virar à direita
			DirecaoVeiculo = (DirecaoVeiculo + 1) % 4;  // Virar à direita (0->1, 1->2, 2->3, 3->0)
			
			// Verificar se a direção é válida
			if (DirecaoVeiculo < 0 || DirecaoVeiculo > 3)
			{
				cout << "ERRO: Direção do veículo inválida! Resetando..." << endl;
				DirecaoVeiculo = 0;
			}
			
			{
				string direcoes[] = {"Norte", "Leste", "Sul", "Oeste"};
				cout << "Direção: " << direcoes[DirecaoVeiculo] << endl;
			}
			break;
		default:
			break;
	}
}

// **********************************************************************
//  void main ( int argc, char** argv )
// **********************************************************************
int  main ( int argc, char** argv )
{
    cout << "Simulador de Cidade" << endl;
    
    
    system("pwd");

    glutInit            ( &argc, argv );
    glutInitDisplayMode (GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGB );
    glutInitWindowPosition (0,0);

    // Define o tamanho inicial da janela grafica do programa
    glutInitWindowSize  ( 900, 700);
   
    // Cria a janela na tela, definindo o nome da
    // que aparecera na barra de titulo da janela.
    glutCreateWindow    ( "Simulador de Cidades" );

    // executa algumas inicializacoes
    init ();

    // Define que o tratador de evento para
    // o redesenho da tela. A funcao "display"
    // sera' chamada automaticamente quando
    // for necessario redesenhar a janela
    glutDisplayFunc ( display );

    // Define que o tratador de evento para
    // o invalidacao da tela. A funcao "display"
    // sera' chamada automaticamente sempre que a
    // maquina estiver ociosa (idle)
    glutIdleFunc(animate);

    // Define que o tratador de evento para
    // o redimensionamento da janela. A funcao "reshape"
    // sera chamada automaticamente quando
    // o usuario alterar o tamanho da janela
    glutReshapeFunc ( reshape );

    // Define que o tratador de evento para
    // as teclas. A funcao "keyboard"
    // sera chamada automaticamente sempre
    // o usuario pressionar uma tecla comum
    glutKeyboardFunc ( keyboard );

    // Define que o tratador de evento para
    // as teclas especiais(F1, F2,... ALT-A,
    // ALT-B, Teclas de Seta, ...).
    // A funcao "arrow_keys" sera chamada
    // automaticamente sempre o usuario
    // pressionar uma tecla especial
    glutSpecialFunc ( arrow_keys );

    // inicia o tratamento dos eventos
    glutMainLoop ( );

    return 0;
}

// ===== IMPLEMENTAÇÃO DAS FUNÇÕES DE CONTROLE DO VEÍCULO =====

void AtualizarPosicaoVeiculo(float deltaTime)
{
    if (!VeiculoEmMovimento || CombustivelAtual <= 0.0f)
        return;

    // Verificar se deltaTime é válido
    if (isnan(deltaTime) || deltaTime <= 0.0f)
        return;

    float distancia = VelocidadeVeiculo * deltaTime;
    
    // Calcular nova posição baseada na direção atual (4 direções fixas)
    float novaX = PosicaoVeiculo.x;
    float novaZ = PosicaoVeiculo.z;
    
    switch (DirecaoVeiculo)
    {
        case 0: // Norte (0 graus)
            novaZ -= distancia;
            break;
        case 1: // Leste (90 graus)
            novaX += distancia;
            break;
        case 2: // Sul (180 graus)
            novaZ += distancia;
            break;
        case 3: // Oeste (270 graus)
            novaX -= distancia;
            break;
        default:
            cout << "ERRO: Direção inválida!" << endl;
            return;
    }
    
    // Verificar se a nova posição é válida
    if (isnan(novaX) || isnan(novaZ))
    {
        cout << "ERRO: Nova posição inválida calculada!" << endl;
        return;
    }
    
    // ===== SISTEMA DE COLISÕES =====
    // Verificar se a nova posição está em uma rua válida
    if (VerificarColisao(novaX, novaZ))
    {
        // Se há colisão, o veículo continua se movendo em linha reta
        // mas não muda de posição (simula "deslizar" ao longo da borda)
        // O veículo mantém a direção atual e continua consumindo combustível
        
        // Debug: mostrar que está "deslizando" ao longo da borda
        static int colisaoCounter = 0;
        if (colisaoCounter++ % 300 == 0) // Print a cada 5 segundos
        {
            string direcoes[] = {"Norte", "Leste", "Sul", "Oeste"};
            cout << "Colisão detectada! Veículo deslizando ao longo da borda na direção: " 
                 << direcoes[DirecaoVeiculo] << endl;
        }
    }
    else
    {
        // Se não há colisão, move o veículo para a nova posição
        PosicaoVeiculo.x = novaX;
        PosicaoVeiculo.z = novaZ;
        
        // Debug: verificar nova posição
        static int debugCounter = 0;
        if (debugCounter++ % 300 == 0) // Print a cada 5 segundos (60 FPS * 5)
        {
            string direcoes[] = {"Norte", "Leste", "Sul", "Oeste"};
            cout << "Movimento válido - Veículo em movimento: " << VeiculoEmMovimento 
                 << ", Combustível: " << CombustivelAtual 
                 << ", Direção: " << direcoes[DirecaoVeiculo]
                 << ", Posição atual: (" << PosicaoVeiculo.x << ", " << PosicaoVeiculo.z << ")" << endl;
            cout << "Nova posição calculada: (" << novaX << ", " << novaZ << ")" << endl;
        }
    }
    
    // Consumir combustível (mesmo quando há colisão)
    CombustivelAtual -= ConsumoCombustivel * deltaTime;
    if (CombustivelAtual < 0.0f) CombustivelAtual = 0.0f;
    
    // Verificar se passou por uma cápsula de combustível
    int gridX = (int)PosicaoVeiculo.x;
    int gridZ = (int)PosicaoVeiculo.z;
    
    // Verificar se as coordenadas estão dentro dos limites do mapa
    if (gridX >= 0 && gridX < QtdX && gridZ >= 0 && gridZ < QtdZ)
    {
        if (Cidade[gridZ][gridX].tipo == COMBUSTIVEL)
        {
            CombustivelAtual = 100.0f;  // Reabastecer
            Cidade[gridZ][gridX].tipo = RUA;  // Remover cápsula e deixar como rua
            cout << "Combustível reabastecido!" << endl;
        }
    }
}

void AlternarModoVisualizacao()
{
    ModoPrimeiraPessoa = !ModoPrimeiraPessoa;
    
    // A atualização da câmera (1a ou 3a pessoa) acontecerá no animate()
    cout << "Modo de visualização alterado para: "
         << (ModoPrimeiraPessoa ? "Primeira Pessoa" : "Terceira Pessoa") << endl;
}

void DesenharVeiculo()
{
    glPushMatrix();
    glTranslatef(PosicaoVeiculo.x, 0.3f, PosicaoVeiculo.z);
    
    // Converter direção para ângulo em graus
    float anguloRotacao = 0.0f;
    switch (DirecaoVeiculo)
    {
        case 0: // Norte
            anguloRotacao = 0.0f;
            break;
        case 1: // Leste
            anguloRotacao = 90.0f;
            break;
        case 2: // Sul
            anguloRotacao = 180.0f;
            break;
        case 3: // Oeste
            anguloRotacao = 270.0f;
            break;
        default:
            anguloRotacao = 0.0f;
            break;
    }
    
    glRotatef(anguloRotacao, 0, 1, 0);
    
    // Desenhar o veículo como um paralelepípedo vermelho
    defineCor(Red);
    glScalef(0.4f, 0.2f, 0.8f); // Dimensões do veículo
    glutSolidCube(1.0f);
    
    glPopMatrix();
}

Ponto EncontrarPosicaoInicialValida()
{
    // Procurar uma posição inicial válida (uma rua)
    for(int z = 0; z < QtdZ; z++)
    {
        for(int x = 0; x < QtdX; x++)
        {
            if(Cidade[z][x].tipo == RUA)
            {
                cout << "Posição inicial encontrada: (" << x << ", " << z << ")" << endl;
                return Ponto(x + 0.5f, 0, z + 0.5f);
            }
        }
    }
    // Se não encontrar, retornar uma posição padrão
    cout << "Nenhuma posição inicial válida encontrada. Usando posição padrão." << endl;
    return Ponto(10.5f, 0, 10.5f);
}

void AtualizaCamera()
{
    // Verificar se as variáveis estão inicializadas corretamente
    if (ModoPrimeiraPessoa)
    {
        // Primeira pessoa: câmera na posição do veículo
        
        // Verificar se a posição do veículo é válida
        if (isnan(PosicaoVeiculo.x) || isnan(PosicaoVeiculo.z))
        {
            cout << "ERRO: Posição do veículo inválida!" << endl;
            return;
        }
        
        Observador = PosicaoVeiculo;
        Observador.y = 0.5f; // Altura dos "olhos" do motorista
        
        // Calcular alvo baseado na direção atual (4 direções fixas)
        float distanciaOlhar = 2.0f;
        switch (DirecaoVeiculo)
        {
            case 0: // Norte
                Alvo.x = PosicaoVeiculo.x;
                Alvo.z = PosicaoVeiculo.z - distanciaOlhar;
                break;
            case 1: // Leste
                Alvo.x = PosicaoVeiculo.x + distanciaOlhar;
                Alvo.z = PosicaoVeiculo.z;
                break;
            case 2: // Sul
                Alvo.x = PosicaoVeiculo.x;
                Alvo.z = PosicaoVeiculo.z + distanciaOlhar;
                break;
            case 3: // Oeste
                Alvo.x = PosicaoVeiculo.x - distanciaOlhar;
                Alvo.z = PosicaoVeiculo.z;
                break;
            default:
                Alvo.x = PosicaoVeiculo.x;
                Alvo.z = PosicaoVeiculo.z - distanciaOlhar;
                break;
        }
        Alvo.y = 0.5f;
        
        // Verificar se o alvo é válido
        if (isnan(Alvo.x) || isnan(Alvo.z))
        {
            cout << "ERRO: Alvo da câmera inválido!" << endl;
            Alvo = PosicaoVeiculo; // Fallback para posição do veículo
        }
    }
    else
    {
        // Terceira pessoa: câmera olha para o veículo
        // Verificar se a posição de terceira pessoa é válida
        if (isnan(TerceiraPessoa.x) || isnan(TerceiraPessoa.z))
        {
            cout << "ERRO: Posição de terceira pessoa inválida!" << endl;
            TerceiraPessoa = Ponto(25, 10, 55); // Posição padrão
        }
        
        Observador = TerceiraPessoa; // Garante que a camera está na posição de 3a pessoa
        Alvo = PosicaoVeiculo;
        
        // Verificar se o alvo é válido
        if (isnan(Alvo.x) || isnan(Alvo.z))
        {
            cout << "ERRO: Alvo da câmera inválido!" << endl;
            Alvo = Ponto(0, 0, 0); // Fallback para origem
        }
    }
}

bool VerificarColisao(float x, float z)
{
    int gridX = (int)x;
    int gridZ = (int)z;

    // Verificar se as coordenadas estão dentro dos limites do mapa
    if (gridX < 0 || gridX >= QtdX || gridZ < 0 || gridZ >= QtdZ)
    {
        return true; // Colisão se estiver fora dos limites
    }

    // Verificar se a posição é uma rua (tipo RUA = 20)
    if (Cidade[gridZ][gridX].tipo == RUA)
    {
        return false; // Não há colisão se for uma rua
    }
    else
    {
        return true; // Há colisão se não for uma rua
    }
}





