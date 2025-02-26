#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/i2c.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/adc.h"
#include "hardware/uart.h"
#include "hardware/pwm.h"
#include "inc/ssd1306.h"
#include "inc/font.h"
#include "pio_matrix.pio.h"

// Valores bases utilizados em funções de configuração e intervalo
#define VR_MAX 4095         // Valor máximo dos valores dos eixos
#define VR_MIN 0            // Valor mínimo dos valores dos eixos
#define PERIODO 4000        // Valor do Periodo Máximo (WRAP)
#define PWM_DIVISOR 32.0    // Valor do Dividor do PWM (clkdiv)

// Correlação de Pinos e Componentes da Placa
#define VRX_PIN 26      // Direcional Joystick no eixo X
#define VRY_PIN 27      // Direcional Joystick no eixo Y
#define BUTTON_A 5      // Botão A
#define BUTTON_B 6      // Botão B
#define BUTTON_JOY 22   // Botão Joystick
#define LED_G 11        // LED Verde
#define LED_R 13        // LED Vermelho
#define LED_B 12        // LED Azul
#define I2C_SDA 14      // Conexão do display (data)
#define I2C_SCL 15      // Conexão do display (clock)
#define NUM_PIXELS 25   // Quantidade de Pixels Matriz
#define MATRIX 7        // Matriz de Leds
#define BUZZER_A 21     // Buzzer A
#define BUZZER_B 10     // Buzzer B

// Configuração para I2C
#define I2C_PORT i2c1 
#define ADDRESS 0x3C    // Endereço

// Variáveis //
// Variáveis lógicas de controle
bool automatic = true;          // Luminosidade automática
bool manual_switch = false;     // Controle de Luminosidade Manual

// Variáveis numérica de controle
static volatile uint8_t mode = 0;       // Modo de Luminosidade
static volatile uint32_t last_time = 0; // Variável de tempo

ssd1306_t ssd;                          // Inicializa a estrutura do display

// Cabeçalhos de Funções
// Configuração do Display
static void ssd_setup();
// Função para criar informações dos LEDs da Matriz de LEDs
uint32_t matriz_rgb(double r, double g, double b);
// Função para enviar iluminação para a Matriz de LEDs
void iluminar(PIO pio, uint sm, uint16_t level);
// Função para tocar sons no Buzzer
void tocar_som(uint32_t frequencia, uint32_t duracao);
// Função para retornar a frequência das notas
uint32_t notas(char *note, uint32_t octave);
// Configurar o PWM
static void pwm_setup(uint pwm_pin, uint16_t level);
// Configurações Gerais
static void setup(PIO pio, uint sm);
// Função para interrupção
static void gpio_irq_handler(uint gpio, uint32_t events);
// Função para imprimir informações no Display
static void display_message(uint16_t level_v, uint16_t level_h);
// Função para configurar LED RGB
static void LED(uint16_t level_h);

// Função Principal
int main() {
    // Inicializando o programa
    stdio_init_all();
    PIO pio = pio0;
    uint sm = pio_claim_unused_sm(pio, true);
    setup(pio, sm);
    ssd_setup();
    adc_init();

    // Alerta Sonoro para sinalizar que o programa foi configurado
    tocar_som(notas("Bb", 3), 250);
    tocar_som(notas("A", 3), 250);

    // Definindo Interrupções
    gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);      // Botão A
    gpio_set_irq_enabled_with_callback(BUTTON_B, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);      // Botão B
    gpio_set_irq_enabled_with_callback(BUTTON_JOY, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);    // Botão Joystick

    while (true) {
        adc_select_input(0); 
        uint16_t vrv_value = adc_read();        // Lendo valor Vertical do Joystick
        adc_select_input(1); 
        uint16_t vrh_value = adc_read();        // Lendo valor Horizontal do Joystick
        display_message(vrv_value, vrh_value);  // Mostrar mensagens no Display
        LED(vrh_value);                         // Atualizar o LED
        iluminar(pio, sm, vrv_value);           // Iluminação da Matriz de LED
        sleep_ms(150);                          // Delay para reduzir consumo do processador   
    }
}

// Funções
static void ssd_setup() {
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, ADDRESS, I2C_PORT);    // Inicializando o Display
    ssd1306_config(&ssd);                                           // Configurando o Display
    ssd1306_send_data(&ssd);                                        // Enviando informação para o Display
    ssd1306_fill(&ssd, false);                                      // Limpando o display
    ssd1306_send_data(&ssd);                                        // Enviando informação para o Display
}
uint32_t matriz_rgb(double r, double g, double b) {
    unsigned char R, G, B;
    R = r * 255;
    G = g * 255;
    B = b * 255;
    return (G << 24) | (R << 16) | (B << 8);
}
void iluminar(PIO pio, uint sm, uint16_t level) {
    float amount_r, amount_g, amount_b, control = 0; // Valores e Variável de controle
    switch (mode) { // Se baseando no modo de iluminação.
    case 0: // Se for Luz Branca:
        amount_r = amount_g = amount_b = 0.03;
        break;
    case 1: // Se for Luz Neutra:
        amount_b = 0.02;
        amount_g = 0.0282;
        amount_r = 0.03;
        break;
    case 2: // Se for Luz Fria:
        amount_b = 0.03;
        amount_g = 0.0264;
        amount_r = 0.0234;
        break;
    default: // Default por segurança
        break;
    }
    if(automatic) { // Se estiver no modo automático:
        control = (1 - (float)level/(float)VR_MAX) * 1; // A variável de controle se baseia no valor da luminosidade ambiente.
    } else if (manual_switch) { // Se não, se o controle manual estiver ativado:
        control = 0.75; // A variavél de controle será 0.75 ou 75%
    }
        for (int i = 0; i < NUM_PIXELS; i++) {
            double r = amount_r * control;  // Intensidade de cada pixel vermelho no frame
            double g = amount_g * control;  // Intensidade de cada pixel verde no frame
            double b = amount_b * control;  // Intensidade de cada pixel azul no frame

            uint32_t color = matriz_rgb(r, g, b);   // Intensidade variável para o azul e verde
            pio_sm_put_blocking(pio, sm, color);    // Enviando para a Matriz de LEDs
        }
}
void tocar_som(uint32_t frequencia, uint32_t duracao) {

    uint32_t periodo = 1000000 / frequencia + 1;    // Calculando o período
    uint32_t meio_periodo = periodo / 2;            // Calculando o meio-período

    uint32_t cycles = (frequencia * duracao) / 1000;    // Calcula o número total de ciclos necessários

    // Gera a onda quadrada alternando o estado do pino
    for (uint32_t i = 0; i < cycles; i++) {
        gpio_put(BUZZER_A, 1);      // Liga Buzzer A
        sleep_us(meio_periodo/3);   // Espera 1/3 do meio-período
        gpio_put(BUZZER_B, 1);      // Liga Buzzer B
        sleep_us(2*meio_periodo/3); // Aguarda 2/3 do meio-período
        gpio_put(BUZZER_A, 0);      // Desliga Buzzer A
        sleep_us(2*meio_periodo/3); // Aguarda 2/3 do meio período
        gpio_put(BUZZER_B, 0);      // Desliga Buzzer B
        sleep_us(meio_periodo/3);   // Aguarda 1/3 do meio-período
    }
}
uint32_t notas(char *note, uint32_t octave) {
    uint32_t NOTES[12][7] = {   {16, 33, 65,  130, 261, 523, 1046}, //C
                                {17, 35, 69,  138, 277, 554, 1109}, //C#Db
                                {18, 38, 73,  155, 294, 587, 1175}, //D
                                {19, 39, 78,  155, 311, 622, 1244}, //D#Eb
                                {21, 41, 82,  165, 329, 659, 1318}, //E
                                {22, 44, 87,  175, 349, 698, 1397}, //F
                                {23, 46, 92,  185, 370, 740, 1480}, //F#Gb
                                {24, 49, 98,  196, 392, 784, 1568}, //G
                                {26, 52, 104, 208, 415, 831, 1661}, //G#Ab
                                {27, 55, 110, 220, 440, 880, 1760}, //A
                                {29, 58, 116, 233, 466, 932, 1865}, //A#Bb
                                {31, 62, 123, 247, 493, 988, 1975}};//B
                        
    char *notes_name[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};       // Nome das notas
    char *notes_name_alt[12] = {"C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B"};   // Nome alternativo das notas

    if (octave < 7 && octave >= 0) {                                    // Se está dentro dos octavos permitidos
        for (int i = 0; i < 12; i++) {                                  // Procurando nos cabeçalhos   
            if(note == notes_name[i] || note == notes_name_alt[i]) {    // Se achar
                return NOTES[i][octave];                                // Retorna a frequência
    }   }   }

    return 177; // Retorna 177
}
static void pwm_setup(uint pwm_pin, uint16_t level) {
    gpio_set_function(pwm_pin, GPIO_FUNC_PWM);      // Inicializando como PWM
    uint slice = pwm_gpio_to_slice_num(pwm_pin);    // Obtendo o Slice
    pwm_set_clkdiv(slice, PWM_DIVISOR);             // Definindo o divisor do clock
    pwm_set_wrap(slice, PERIODO);                   // Definindo o valor do WRAP
    pwm_set_gpio_level(pwm_pin, level);             // Colocando o nível inicial no pino
    pwm_set_enabled(slice, true);                   // Ligando o Slice como PWM
}
static void setup(PIO pio, uint sm) {

    // Configurando as conexões I2C
    i2c_init(I2C_PORT, 400 * 1000);             // Inicializando I2C
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);  // Configurando a função do SDA
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);  // Configurando a função do SCL
    gpio_pull_up(I2C_SDA);                      // Pull up the data line
    gpio_pull_up(I2C_SCL);                      // Pull up the clock line

    // Inicializando Joystick
    adc_gpio_init(VRX_PIN);             // Direcional vertical do Joystick
    adc_gpio_init(VRY_PIN);             // Direcional horizontal do Joystick
    gpio_init(BUTTON_JOY);              // Botão do Joystick
    gpio_set_dir(BUTTON_JOY, GPIO_IN);  // Modo de Entrada
    gpio_pull_up(BUTTON_JOY);           // Pull-up

    // Inicializando Botões
    gpio_init(BUTTON_A);                // Botão A
    gpio_set_dir(BUTTON_A, GPIO_IN);    // Modo de Entrada
    gpio_pull_up(BUTTON_A);             // Pull-up

    gpio_init(BUTTON_B);                // Botão B
    gpio_set_dir(BUTTON_B, GPIO_IN);    // Modo de Entrada
    gpio_pull_up(BUTTON_B);             // Pull-up

    // Inicializando Buzzers
    gpio_init(BUZZER_A);                // Buzzer A
    gpio_set_dir(BUZZER_A, GPIO_OUT);   // Modo de Saída
    gpio_init(BUZZER_B);                // Buzzer B
    gpio_set_dir(BUZZER_B, GPIO_OUT);   // Modo de Saída

    // Inicializando LEDs
    gpio_init(LED_G);               // LED Verde
    gpio_set_dir(LED_G, GPIO_OUT);  // Modo de Saída
    gpio_put(LED_G, false);         // Garantir que esteja desligado
    gpio_init(LED_B);               // LED Azul
    gpio_set_dir(LED_B, GPIO_OUT);  // Modo de Saída
    gpio_put(LED_B, false);         // Garantir que esteja desligado
    gpio_init(LED_R);               // LED Vermelho
    gpio_set_dir(LED_R, GPIO_OUT);  // Modo de Saída
    gpio_put(LED_R, false);         // Garantir que esteja desligado    

    // Configurando a Matriz de LEDs
    bool set_clock = set_sys_clock_khz(125000, false);
    uint offset = pio_add_program(pio, &pio_matrix_program);
    pio_matrix_program_init(pio, sm, offset, MATRIX);

    pwm_setup(LED_R, 0);    // Inicializando o LED Vermelho com nível inicial 0
    pwm_setup(LED_B, 0);    // Inicializando o LED Azul com nível inicial 0
    pwm_setup(LED_G, 0);    // Inicializando o LED Verde com nível inicial 0
}
static void gpio_irq_handler(uint gpio, uint32_t events) {
    uint32_t current_time = to_us_since_boot(get_absolute_time()); 
    // Verifica se passou tempo suficiente desde o último evento
    if (current_time - last_time > 200000) {                // 200 ms de debouncing
        last_time = current_time;                           // Atualiza o valor
        switch (gpio) {                                     // Verificar qual Botão foi pressionado
            case BUTTON_A:                                      // Se for o Botão A
                mode = (mode + 1) % 3;                          // Muda o modo de iluminação
                break;
            case BUTTON_B:                                      // Se for o Botão B
                automatic = !automatic;                         // Alterna a automatização
                manual_switch = false;                          // Desliga o controle manual por segurança
                break;
            case BUTTON_JOY:                                    // Se for o Botão do Joystick
                manual_switch = !automatic && !manual_switch;   // Alterna o estado do controle manual, se o estado automatico estiver desligado
                break;
            default:
                break;
        }
    }
}
static void display_message(uint16_t level_v, uint16_t level_h) {
    ssd1306_fill(&ssd, false);                              // Limpando o Display
    if(automatic) {                                         // Se no modo automático
        ssd1306_draw_string(&ssd, "Modo: Auto", 0, 4);      // Modo = Auto
    } else {                                                // Se não
        ssd1306_draw_string(&ssd, "Modo: Manual", 0, 4);    // Modo = Manual
    }
    switch (mode) {                                             // Verifica o modo de iluminação
        case 0:                                                 // Se for 0
            ssd1306_draw_string(&ssd, "Luz: Branca", 0, 12);    // Luz = Branca
            break;
        case 1:                                                 // Se for 1
            ssd1306_draw_string(&ssd, "Luz: Neutra", 0, 12);    // Luz = Neutra
            break;
        case 2:                                                 // Se for 2
            ssd1306_draw_string(&ssd, "Luz: Fria", 0, 12);      // Luz = Fria
            break;
        default:                                                // Default por segurança
            break;
    }

    char temp[10];                                                              // String de Temperatura
    sprintf(temp, "Temp: %d*", (uint16_t)(((50.0)/(float)(VR_MAX))*level_h));   // Colocando o valor na String
    ssd1306_draw_string(&ssd, temp, 0, 20);                                     // Enviando pro Display

    char ambiente[10], luzes[10];                                                   // Strings de Luminosidade
    sprintf(ambiente, "Amb: %.0f%%", (((float)level_v/(float)VR_MAX)*100));         // Luminosidade Ambiente (Leitura do Joystick)
    if(automatic) {                                                                 // Se for modo automático
        sprintf(luzes, "Art: %.0f%%", (1.0 - ((float)level_v/(float)VR_MAX))*100);  // Luminosidade Artificial (Correlacionada à ambiente)
    } else if (manual_switch) {                                                      // Se não, se o controle manual estiver ativado
        sprintf(luzes, "Art: %.0f%%", 75.0);                                        // Luminosidade Artificial = 75%
    } else {                                                                        // Se não
        sprintf(luzes, "Art: %.0f%%", 0.0);                                         // Luminosidade Artifical = 0%
    }

    ssd1306_draw_string(&ssd, "Luminosidade", 0, 28);   // Enviando Luminosidade pro Display
    ssd1306_draw_string(&ssd, ambiente, 0, 36);         // Ambiente
    ssd1306_draw_string(&ssd, luzes, 0, 44);            // Artificial
    ssd1306_send_data(&ssd);                            // Comunicação com o Display
}
static void LED(uint16_t level_h) {
    uint16_t level_r = 0;   // Nível Vermelho
    uint16_t level_g = 0;   // Nível Verde
    uint16_t level_b = 0;   // Nível Azul

    if (level_h <= VR_MAX/3) {   
        level_b = -1.5 * level_h + 2024;    // Cálculo do Nível do LED Azul
    } if (level_h >= (2*VR_MAX)/3) {
        level_r = 1.465 * level_h - 4000;   // Cálculo do Nível do LED Vermelho
    } if (level_h <= 2048) {
        level_g = 2048 - level_h;
    } else {
        level_g = level_h - 2048;
    }
    level_g = -0.488 * level_g + 1000;      // Cálculo do Nível do LED Verde

    pwm_set_gpio_level(LED_R, level_r);     // Enviando o Nível pro LED Vermelho
    pwm_set_gpio_level(LED_G, level_g);     // Enviando o Nível pro LED Verde
    pwm_set_gpio_level(LED_B, level_b);     // Enviando o Nível pro LED Azul
}