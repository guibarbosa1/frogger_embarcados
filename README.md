# Frogger Embarcados

Guilherme da Silva Barbosa e Gabriel Targas

Este repositório contém a implementação de uma versão arcade do clássico jogo Frogger (Atari), desenvolvida especificamente para a placa de desenvolvimento FPGA Zybo Z7-20. O projeto foi criado como parte das atividades da disciplina de Sistemas Embarcados da Universidade Federal de São Carlos (UFSCar).

## Arquitetura e Ferramentas Utilizadas

O desenvolvimento do projeto foi dividido em duas etapas principais, separando a camada de hardware e a camada de software:

* Hardware (Block Design): Utilizou-se o Vivado Design Suite 2017.4 para a criação do block design, mapeamento de periféricos e geração do hardware base para a placa Zybo Z7-20.
* Software (Lógica do Jogo): Utilizou-se o Xilinx SDK para o desenvolvimento de todo o código fonte em linguagem C.

## Estrutura do Repositório

O repositório está organizado da seguinte maneira:

* /Zybo-Z7-20-HDMI: Contém arquivos e dependências relacionadas à interface de vídeo HDMI.
* /vivado-boards: Submódulo contendo os arquivos de definição da placa (board files).
* /vivado_proj_hdmi: Diretório contendo o projeto principal do Vivado.
* build_hdmi.tcl: Script TCL para a reconstrução automatizada do projeto no Vivado.
* hdmi.xdc: Arquivo de constraints contendo o mapeamento dos pinos físicos da FPGA.

### Localização do Código Principal (Jogo)

Toda a lógica do jogo Frogger está concentrada em um único arquivo de código. O arquivo principal `main.c` está localizado dentro da pasta `src`, que por sua vez fica dentro do diretório de workspace do Xilinx SDK gerado após a exportação do hardware.

## Como Jogar

O objetivo do jogo segue as regras clássicas do Frogger: você deve guiar o sapo desde a parte inferior da tela até o topo, atravessando uma rodovia movimentada e um rio cheio de obstáculos, evitando colisões para não perder vidas.

Controles:
A movimentação do personagem é feita de forma nativa utilizando os próprios botões físicos (Push Buttons) presentes na placa Zybo Z7-20. 
* Botão 0: Move para baixo
* Botão 1: Move para a direita
* Botão 2: Move para a esquerda
* Botão 3: Move para cima

(Nota: A orientação exata dos botões pode variar conforme a configuração definida no `main.c`).

## Tutorial de Instalação e Execução

Para rodar o projeto na sua Zybo Z7-20, siga os passos abaixo:

### Passo 1: Preparação do Hardware (Vivado 2017.4)
1. Clone este repositório em sua máquina local.
2. Abra o Vivado 2017.4.
3. Você pode abrir o projeto de duas formas:
   * Navegando até a pasta `vivado_proj_hdmi` e abrindo o arquivo `.xpr`.
   * Ou utilizando a aba "Tcl Console" no Vivado, navegando até a raiz do repositório e executando o comando: `source build_hdmi.tcl`.
4. Com o projeto aberto, clique em "Generate Bitstream" na barra lateral esquerda e aguarde a conclusão da síntese e implementação.
5. Após a geração do bitstream, vá em `File -> Export -> Export Hardware`. Certifique-se de marcar a caixa "Include bitstream" e clique em OK.
6. Vá em `File -> Launch SDK` para abrir o Xilinx SDK.

### Passo 2: Compilação e Execução (Xilinx SDK)
1. Com o Xilinx SDK aberto, o workspace carregará as definições de hardware que você acabou de exportar.
2. Navegue até a pasta do projeto da aplicação no painel "Project Explorer". Abra a pasta `src`. É aqui que o arquivo `main.c` deve estar localizado.
3. O SDK compilará o projeto automaticamente por padrão. Caso não ocorra, vá em `Project -> Build All`.
4. Conecte a sua placa Zybo Z7-20 ao computador através da porta USB (PROG/UART) e ligue a chave de energia.
5. Conecte também o cabo HDMI da placa a um monitor.
6. No SDK, vá em `Xilinx Tools -> Program FPGA` e clique em "Program" para gravar o bitstream na placa.
7. Por fim, clique com o botão direito no projeto da aplicação (onde está o seu `main.c`), selecione `Run As -> 1 Launch on Hardware (System Debugger)`.

O jogo será carregado na memória da placa e a imagem deverá aparecer no monitor conectado via HDMI. Utilize os botões da placa para jogar.
