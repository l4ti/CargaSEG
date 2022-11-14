/******************************************************************************************************************
* CargaSEG v1.0                                                                                                   *
*                                                                                                                 *
* The MIT License (MIT)                                                                                           *
* Copyright © 2022 Renato de Castro Ferreira (https://github.com/l4ti)                                            *
*                                                                                                                 *
* Permission is  hereby granted, free of charge,  to any person obtaining a copy of this  software and associated *
* documentation files (the “Software”), to deal in the Software without restriction, including without limitation *
* the rights to use,  copy, modify, merge,  publish, distribute, sublicense,  and/or sell copies of the Software, * 
* and to permit persons to whom the Software is furnished to do so, subject to the following conditions:          *
*                                                                                                                 *
* The above copyright  notice and this permission notice shall  be included in all copies or substantial portions *
* of the Software.                                                                                                *
*                                                                                                                 *
* THE SOFTWARE IS PROVIDED “AS IS”,  WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,  INCLUDING BUT NOT LIMITED *
* TO THE WARRANTIES OF MERCHANTABILITY,  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL *
* THE AUTHORS OR COPYRIGHT  HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,  WHETHER IN AN ACTION OF *
* CONTRACT,  TORT OR  OTHERWISE,  ARISING FROM,  OUT OF OR IN  CONNECTION WITH THE  SOFTWARE OR THE  USE OR OTHER *
* DEALINGS IN THE SOFTWARE.                                                                                       *
*                                                                                                                 *
*                                                                                                                 *
* Supported OS: Mbed v2                                                                                           *
* Supported HW: Kinetis FRDM-KL25Z                                                                                *
******************************************************************************************************************/

/*
 * BIBLIOTECAS
 */
#include "mbed.h" //Mbed v2
#include "rtos.h" //RTOS
#include "TextLCD.h" //http://mbed.org/users/simon/code/TextLCD/
#include "MMA8451Q.h" //http://mbed.org/users/emilmont/code/MMA8451Q/
#include "TSISensor.h" //http://mbed.org/users/emilmont/code/TSI/

#include <string> //C++ STD String LIB
#include <cstdio> //C++ STD File O/I LIB



/*
 * DEFINIÇÃO DOS PINOS
 */
// LED
PwmOut ledR(LED1);
PwmOut ledG(LED2);
PwmOut ledB(LED3);

// Motor de passo
DigitalOut passoP1(PTE23);
DigitalOut passoP2(PTE22);
DigitalOut passoP3(PTE21);
DigitalOut passoP4(PTE20);

// Sensor de temperatura
AnalogIn tempSensor(A0);

// Acelerometro
#define MMA8451Q_I2C_ADDRESS (0x1d<<1)
MMA8451Q acel(PTE25, PTE24, MMA8451Q_I2C_ADDRESS);

// Botão
InterruptIn botao(PTD7);

// Touch
TSISensor tsi;



/*
 * DECLARAÇÃO DE VARIÁVEIS
 */
// Habilita/Desabilita
bool ligado = true;   //Sistema ligado
bool enPasso = false; //Motor ligado

// Valores de sensores
float temperatura = -275.0;  //Temperatura
float angX = 200, angY = 200, angZ = 200; //Angulos

int tipoErro = 0; //Classfic. de erro
float valErro = -999; //Valor do sensor no erro

// Valores de usuário
int minTemp = 0, maxTemp = 20;
int maxAng = 10;
bool controlTemp = false; //Lig/Des refrigeração



/*
 * DECLARAÇÃO DAS FUNÇÕES
 */
// SISTEMA
void ShowDisplay(std::string texto = "default");
void GetTemperatura();
void GetAngulos();
void MotorPasso();
float TouchSwipe();
void Leds(float R = 1, float G = 1, float B = 1);

// USUARIO
void BoasVindas();
void Monitoramento();
void DesligaPlaca();
void InteracaoMonitoramento();



/*
 * DECLARAÇÃO DE FUNÇÕES DE THREAD
 */
void th1Function();
void th2Function();
void th3Function();



/*
 * FUNÇÃO PRINCIPAl
*/
int main() {
    /* INICIALIZAÇÃO DA PLACA */
    Thread::wait(25);
    Leds(0, 1, 1);
    ShowDisplay("Iniciando...");
    Thread::wait(50);
    // -------------------------------
    Leds(1, 0, 1);
    // Inicialização do Thread 1 - th1
    ShowDisplay("Iniciando...\nThread1");
    unsigned char *th1Stack;                          // Ponteiro para stack do th1
    th1Stack = new unsigned char[1024];               // Alocação na heap
    Thread th1Thread(osPriorityHigh, 1024, th1Stack); // Declaração do Thread
    th1Thread.start(th1Function);                     // Start do th1
    Thread::wait(50);
    // Inicialização do Thread 2 - th2
    ShowDisplay("Iniciando...\nThread2");
    unsigned char *th2Stack;                          // Ponteiro para stack do th2
    th2Stack = new unsigned char[1024];               // Alocação na heap
    Thread th2Thread(osPriorityHigh, 1024, th2Stack); // Declaração do Thread
    th2Thread.start(th2Function);                     // Start do th2
    Thread::wait(50);
    // -------------------------------
    Leds(1, 1, 0);
    // Espera pela inicialização dos sensores
    ShowDisplay("Iniciando...\nSens. Temp.");
    while (temperatura < -274.0 || temperatura > 80) { Thread::wait(1); };
    ShowDisplay("Iniciando...\nSens. Acel.");
    while (angX > 180 || angY > 180 || angZ > 180) { Thread::wait(1); };
    ShowDisplay("Iniciando...\nMotor Passo");
    enPasso = true;
    Thread::wait(200);
    enPasso = false;
    Leds();

    /* INTERAÇÃO DE BOAS VINDAS */
    BoasVindas();

    /* INICIALIZAÇÃO DO THREAD DE MONITORAMENTO */
    unsigned char *th3Stack;                                 // Ponteiro para stack do th3
    th3Stack = new unsigned char[1024];                      // Alocação na heap
    Thread th3Thread(osPriorityAboveNormal, 1024, th3Stack); // Declaração do Thread
    th3Thread.start(th3Function);                            // Start do th3

    /* INTERAÇÃO DE MONITORAMENTO */
    ShowDisplay("");
    Thread::wait(3000);
    botao.rise(&DesligaPlaca); // Habilita a interrupção do botão
    InteracaoMonitoramento();

    /* ENCERRAMENTO */
    // Espera o fim dos Threads para desligar
    ShowDisplay("   Desligando");
    Leds(0, 1, 1);
    Thread::wait(200);
    Leds(1, 0, 1);
    Thread::wait(200);
    Leds(1, 1, 0);
    Thread::wait(200);
    Leds();
    th1Thread.join();
    th2Thread.join();
    th3Thread.join();
    ShowDisplay("   Desligue a\n    energia.");
    Leds(1, 1, 0);
    Thread::wait(50);
    Leds();
    Thread::wait(50);
    Leds(1, 1, 0);
    Thread::wait(50);
    Leds();
}


/*****************
* OUTROS THREADS *
*****************/
// Thread1
void th1Function() {
    while (ligado) {
        MotorPasso();
    }
}

// Thread2
void th2Function() {
    while (ligado) {
        GetTemperatura();
        GetAngulos();
        Thread::wait(100);
    }
}

// Thread3
void th3Function() {
    while (ligado) {
        Monitoramento();
        Thread::wait(100);
    }
}



/*****************
* OUTRAS FUNÇÕES *
*****************/
// Função para passar para o display
void ShowDisplay(std::string texto) {
    TextLCD lcd(PTE0, PTE1, PTE2, PTE3, PTE4, PTE5);
    const char* c = texto.c_str(); // String -> Char
    lcd.cls();
    lcd.printf(c);
}

// Função de leiura da temperatura
void GetTemperatura() {
    const int correcao = 350000; // Corrige erro
    float temp = 0;
    for (int i = 0; i < 500; i++) {
        temp += tempSensor; // Soma p/ fazer a média
        Thread::wait(1);
    }
    temperatura = ((temp/500)/1024)*correcao;
}

// Função de leitura do acelerômetro
void GetAngulos() {
    angX = acel.getAccX() * 90.0; //Retorna eixo X em graus
    angY = acel.getAccY() * 90.0; //Retorna eixo Y em graus
    angZ = 90 - (acel.getAccZ() * 90.0); //Retorna eixo Z em graus
}

// Função do motor de passo
void MotorPasso() {
    const int delay = 3; 

    if ((controlTemp && temperatura > (maxTemp - 3)) || enPasso) {
        passoP1 = false;
        passoP2 = true;
        passoP3 = true;
        passoP4 = true;
        Thread::wait(delay);
        passoP1 = true;
        passoP2 = false;
        passoP3 = true;
        passoP4 = true;
        Thread::wait(delay);
        passoP1 = true;
        passoP2 = true;
        passoP3 = false;
        passoP4 = true;
        Thread::wait(delay);
        passoP1 = true;
        passoP2 = true;
        passoP3 = true;
        passoP4 = false;
        Thread::wait(delay);
    } else {
        passoP1 = false;
        passoP2 = false;
        passoP3 = false;
        passoP4 = false;
        Thread::wait(delay);
    }
}

// Função do touch
float TouchSwipe() {
    if (tsi.readDistance() > 25)
        return 1.0; // Região direita ++
    if (tsi.readDistance() < 15 && tsi.readDistance() > 1) 
        return -1.0; // Região esquerda --
    return 0; // Não pressionado
}

// Função de acender o LED RGB
void Leds(float R, float G, float B) {
    ledR = R;
    ledG = G;
    ledB = B;
}

// Função de boas vindas
void BoasVindas() {
    char floatChar1[6];
    char floatChar2[6];
    char floatChar3[6];


    ShowDisplay("    CargaSEG\n SW: v1.0/Mbed2");
    Thread::wait(2000);
    ShowDisplay("    CargaSEG\n HW: FRDM-KL25Z");
    Thread::wait(2000);

    sprintf(floatChar1, "%.2f", temperatura); // Float -> Char
    ShowDisplay(std::string() + "Temp: " + floatChar1 + " C");
    Thread::wait(3000);

    sprintf(floatChar1, "%.1f", angX);
    sprintf(floatChar2, "%.1f", angY);
    sprintf(floatChar3, "%.1f", angZ);
    ShowDisplay(std::string() + "X: " + floatChar1 + " Y: " + floatChar2 + "\nZ: " + floatChar3);
    Thread::wait(3000);
    
    ShowDisplay("  Aguarde para\n   ajuda ou");
    Thread::wait(2500);
    ShowDisplay(" aperte o botao\npara configurar.");
    {
        bool ajuda = false;
        for(int i = 0; i < 500 && !botao; i++) { // Espera interação ou exibe ajuda
            Thread::wait(10);
            if (i == 499) { ajuda = true; };
        }
        if (ajuda) {
            ShowDisplay(" Para setar os\n    valores");
            Thread::wait(2500);
            ShowDisplay(" coloque o dedo\n    no touch:");
            Thread::wait(2500);
            ShowDisplay("  lado esq. -\n  lado dir. +");
            Thread::wait(2500);
            ShowDisplay("    e entao\n aperte o botao");
            Thread::wait(2500);
            ShowDisplay(" para confirmar");
            Thread::wait(2500);
        }
    }
    
    ShowDisplay("");
    Thread::wait(1000);
    while (!botao) { // User seta temperatura máxima
        sprintf(floatChar1, "%d", maxTemp);
        ShowDisplay(std::string() + "Temp. Maxima:\n" + floatChar1 + " C");
        for (int i = 0; i < 5; i++) {
            if (TouchSwipe() != 0) {
                maxTemp += TouchSwipe(); // ++ ou -- conforme dedo no touch
                sprintf(floatChar1, "%d", maxTemp);
                ShowDisplay(std::string() + "Temp. Maxima:\n" + floatChar1 + " C");
            }
            if (botao)
                break; // Sai do loop antes do delay para melhorar exp do user
            Thread::wait(150);
        }

        ShowDisplay("Temp. Maxima:"); // Para valor piscar
        if (botao)
            break; // Sai do loop antes do delay para melhorar exp do user
        Thread::wait(200);
    }

    ShowDisplay("");
    Thread::wait(1000);
    minTemp = maxTemp - 1; // minTemp não pode ser > maxTemp
    while (!botao) { // User seta temepratura mínima
        sprintf(floatChar1, "%d", minTemp);
        ShowDisplay(std::string() + "Temp. Minima:\n" + floatChar1 + " C");

        for (int i = 0; i < 5; i++) {
            if (TouchSwipe() != 0 && (minTemp + TouchSwipe() < maxTemp)) {
                minTemp += TouchSwipe();
                sprintf(floatChar1, "%d", minTemp);
                ShowDisplay(std::string() + "Temp. Minima:\n" + floatChar1 + " C");
            }
            if (botao)
                break;
            Thread::wait(150);
        }

        ShowDisplay("Temp. Minima:");
        if (botao)
            break;
        Thread::wait(200);
    }

    ShowDisplay("");
    Thread::wait(1000);
    while (!botao) { // User seta ângulo máximo
        sprintf(floatChar1, "%d", maxAng);
        ShowDisplay(std::string() + "Ang. Maximo:\n" + floatChar1);

        for (int i = 0; i < 5; i++) {
            if (TouchSwipe() != 0 && (maxAng + TouchSwipe() >= 0)) {
                maxAng += TouchSwipe();
                sprintf(floatChar1, "%d", maxAng);
                ShowDisplay(std::string() + "Ang. Maximo:\n" + floatChar1);
            }
            if (botao)
                break;
            Thread::wait(150);
        }

        ShowDisplay("Ang. Maximo:");
        if (botao)
            break;
        Thread::wait(200);
    }

    ShowDisplay("");
    Thread::wait(1000);
    while (!botao) { // User define se precisa de refrigeração
        if (controlTemp) {
            ShowDisplay(std::string() + "Controle temp?\nSIM");
        } else {
            ShowDisplay(std::string() + "Controle temp?\nNAO");
        }

        for (int i = 0; i < 5; i++) {
            if (TouchSwipe() == 1) { // Touch lado direito
                controlTemp = true;
                ShowDisplay(std::string() + "Controle temp?\nSIM");
            } else if (TouchSwipe() == -1) { // Touch lado esquerdo
                controlTemp = false;
                ShowDisplay(std::string() + "Controle temp?\nNAO");
            }

            if (botao)
                break;
            Thread::wait(150);
        }

        ShowDisplay("Controle temp?");
        if (botao)
            break;
        Thread::wait(200);
    }

    Leds(1, 0.5, 1);
    ShowDisplay(" Config.  salva");
    Leds(1, 0.5, 1);
    Thread::wait(100);
    Leds();
    Thread::wait(100);
    Leds(1, 0.5, 1);
    Thread::wait(100);
    Leds();
    Thread::wait(2000);
    ShowDisplay(" Aperte o botao\n para  iniciar.");
    while (!botao) { Thread::wait(1); } // Aguarda pressionamento do botão
}

// Função de monitoramento
void Monitoramento() {
    if ((temperatura > maxTemp || temperatura < minTemp || angX > maxAng // Se ainda não houve erro
    || angY > maxAng || angZ > maxAng) && (tipoErro == 0)) { // e sensores estão acima do esperado
        if (temperatura > maxTemp) { // Temperatura máxima excedida
            tipoErro = 1;
            valErro = temperatura;
        } else if (temperatura < minTemp) { // Temperatura mínima excedida
            tipoErro = 2;
            valErro = temperatura;
        }
        if (angX > maxAng) { // Ângulo máximo excedido (eixo X)
            tipoErro = 3;
            valErro = angX;
        } else if (angY > maxAng) { // Ângulo máximo excedido (eixo Y)
            tipoErro = 4;
            valErro = angY;
        } else if (angZ > maxAng) { // Ângulo máximo excedido (eixo Z)
            tipoErro = 5;
            valErro = angZ;
        }
    }
}

// Função de desligar a placa
void DesligaPlaca() {
    ligado = false;
}

// Função de interação durante monitoramento
void InteracaoMonitoramento() {
    char floatChar1[6];

    while (ligado) {
        if (tipoErro == 0) { // Se não há erro
            sprintf(floatChar1, "%.2f", temperatura);
            ShowDisplay(std::string() + "Estado: OK\nTemp: " + floatChar1 + " C");
            Leds(1, 0.5, 1);
            Thread::wait(50);
            Leds();
            Thread::wait(1500);
            sprintf(floatChar1, "%.2f", angX);
            ShowDisplay(std::string() + "Estado: OK\nAngX: " + floatChar1);
            Thread::wait(1500);
            sprintf(floatChar1, "%.2f", angY);
            ShowDisplay(std::string() + "Estado: OK\nAngY: " + floatChar1);
            Thread::wait(1500);
            sprintf(floatChar1, "%.2f", angZ);
            ShowDisplay(std::string() + "Estado: OK\nAngZ: " + floatChar1);
            Thread::wait(1500);
            Leds(1, 0.5, 1);
            Thread::wait(50);
            Leds();
        } else {
            switch (tipoErro) { // Varia display de acordo com o erro
                case 1:
                    Leds(0.5);
                    ShowDisplay("Estado: FALHA\n Tmax. excedida");
                    Thread::wait(2000);
                    sprintf(floatChar1, "%.2f", valErro);
                    ShowDisplay(std::string() + "Estado: FALHA\n Tmax: " + floatChar1 + " C");
                    Leds();
                    Thread::wait(2000);
                    break;

                case 2:
                    Leds(0.5);
                    ShowDisplay("Estado: FALHA\n Tmin. excedida");
                    Thread::wait(2000);
                    sprintf(floatChar1, "%.2f", valErro);
                    ShowDisplay(std::string() + "Estado: FALHA\n Tmin: " + floatChar1 + " C");
                    Leds();
                    Thread::wait(2000);
                    break;

                case 3:
                    Leds(0.5);
                    ShowDisplay("Estado: FALHA\n AngX. excedido");
                    Thread::wait(2000);
                    sprintf(floatChar1, "%.2f", valErro);
                    ShowDisplay(std::string() + "Estado: FALHA\n AngX: " + floatChar1);
                    Leds();
                    Thread::wait(2000);
                    break;

                case 4:
                    Leds(0.5);
                    ShowDisplay("Estado: FALHA\n AngY. excedido");
                    Thread::wait(2000);
                    sprintf(floatChar1, "%.2f", valErro);
                    ShowDisplay(std::string() + "Estado: FALHA\n AngY: " + floatChar1);
                    Leds();
                    Thread::wait(2000);
                    break;

                case 5:
                    Leds(0.5);
                    ShowDisplay("Estado: FALHA\n AngZ. excedido");
                    Thread::wait(2000);
                    sprintf(floatChar1, "%.2f", valErro);
                    ShowDisplay(std::string() + "Estado: FALHA\nAngZ: " + floatChar1);
                    Leds();
                    Thread::wait(2000);
                    break;
            
                default:
                    ShowDisplay("Estado: FALHA\nDESCONHECIDO!"); // Nunca deve ocorrer
            }
        }
    }  
}
