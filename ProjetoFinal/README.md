# Funcionamento
Este programa simula um ambiente que possui um controle de iluminação automático/manual baseado na incidência luminosa presente. Assim como também, observa a temperatura e demonstra quando a mesma se encontra em extremos para casos de temperaturas extremas.
O controle do programa é feito através de 8 componentes, separados em 3 como modo de entrada de dados e 5 como modo de saída.

# Componentes de entrada:
1. Joystick:
    1. Componente Vertical:
        Utilizado para simular a detecção da luminosidade ambiente que seria realizada idealmente por um sensor de luminosidade.
    2. Componente Horizontal:
        Utilizado para simular a detecção da temperatura do ambiente que seria realizada idealmente por um sensor de temperatura.
    3. Botão:
        Utilizado como um controle manual para a luz artificial do ambiente, quando a mesma se encontra em modo manual.
2. Botão A:
    Utilizado para controlar o modo de luminosidade, alternando entre Luz Branca, Luz Neutra ou Luz Fria.
3. Botão B:
    Utilizado para alternar o estado automático/manual da simulação.

# Componentes de saída:
1. Display:
    Utilizado para mostrar informações essenciais para controle do ambiente, como:
    1. Modo -> Visualização se a iluminação está no modo automático ou manual.
    2. Tipo de luminosidade -> Visualização se a luz do ambiente está definida como Luz Branca, Luz Neutra ou Luz Fria.
    3. Temperatura -> visualização da temperatura atual do ambiente.
    4. Luminosidade:
        1. Luminosidade Ambiente -> Visualização da luminosidade natural do ambiente.
        2. Luminosidade Artifical -> Visulização da luminosidade artificial fornecida para o ambiente.
2. LED RGB:
    1. Verde -> Indica a proximidade da temperatura com uma temperatura agradável (Aproximadamente 26°).
    2. Azul -> Indica a proximidade da temperatura com uma temperatura mais baixa (Apromadamente 16°).
    3. Vermelho -> Indica a proximidade da temperatura com uma temperatura mais alta (Aproximadamente 34°).
3. Matriz de LEDs:
    Mostrará a iluminação artificial do ambiente, baseando-se na iluminação ambiente e no modo de luminosidade escolhido.
    No modo manual, sua iluminação é fixa em 75%.
4. Buzzers (Buzzer A e Buzzer B):
    Utilizados para sinalizar o início da execução do programa.

# Instruções de uso
1. Após o sinal sonoro, o programa estará rodando e funcionando.
2. Controle de Luminosidade: Botão A, Botão B, Joystick, Matriz de LEDs:
    1. Utilize o Botão A para alternar entre os três modos de luminosidade.
    2. Utilize o Botão B para alternar entre iluminação automática e manual.
    3. Utilize o Componente Vertical para controlar a iluminação do ambiente, quando o modo automático estiver ligado.
        > A iluminação artificial será manipulada conforme a leitura da iluminação ambiente, buscando compensar a iluminação do ambiente.
    4. Utiliza o Botão do Joystick para controlar a iluminação artifical no modo manual.
3. Controle de Temperatura: Joystick e LED RGB:
    1. Utilize o Componente Horizontal para controlar a temperatura do ambiente.
