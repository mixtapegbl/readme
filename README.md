# 🚀 Ecossistema Inteligente MIXTAPE

Este repositório armazena os sistemas de monitorização climática contínua e auditoria automatizada desenvolvidos pelo grupo **Mixtape**. Os projetos foram projetados para a proteção de infraestruturas de TI (Data Centers) e salas de servidores, atuando na prevenção de falhas causadas por superaquecimento, oxidação por humidade excessiva ou descargas eletrostáticas.

O grande diferencial tecnológico da arquitetura é a implementação de uma máquina de estados estável integrada a um **banco de dados circular baseado na memória não-volátil (EEPROM)** do microcontrolador, permitindo a retenção local e segura de históricos de falhas sem a necessidade de módulos externos de cartão SD.

---

## 🛠️ Modelos de Projeto (Hardware e Arquitetura)

O repositório está estruturado em duas versões estáveis, otimizadas para diferentes cenários de aplicação e níveis de precisão de sensores:

### 1. 📂 MIXTAPE v4.0 (Hardware de Bancada Atual)
Versão física validada em laboratório, focada em otimização de custo e blindagem de software contra ruídos elétricos na protoboard.
*   **Sensor de Telemetria:** DHT11 (Leitura digital de temperatura e humidade).
*   **Módulo de Tempo Real (RTC):** DS1302 físico operando via comunicação serial a 3 fios (RST, DAT, CLK).
*   **Blindagem de Software:** Implementa filtro de redundância contra ruídos de barramento para eliminar o travamento de calendário (bug do ano fantasma 2043) e algoritmo de tendência preditiva térmica e de humidade.
*   **Armazenamento:** Gravação enxuta baseada em blocos diretos de 3 bytes (Hora, Temperatura x10, Humidade).

### 2. 📂 M.E.C.A_4.0 -- PROJETO_FISICO (Versão de Alta Precisão I2C)
Versão projetada para ambientes industriais exigentes, unificando a comunicação de periféricos no mesmo barramento para economizar pinos e fios.
*   **Sensor de Telemetria:** DHT22 / AM2302 (Alta precisão decimal e maior faixa de leitura para humidade estável).
*   **Módulo de Tempo Real (RTC):** DS1307 ou equivalente, operando de forma nativa no **Barramento I2C** (SDA/SCL compartilhado com o driver do display LCD).
*   **Armazenamento:** Estrutura de log expandida que salva o Timestamp Unix completo (4 bytes), Temperatura x100 (2 bytes) e Humidade x100 (2 bytes), totalizando blocos estáveis de 8 bytes por ocorrência.

---

## 💻 Funcionalidades Principais do Firmware
*   **Máquina de Estados Finitos (FSM):** Divisão operacional transparente entre os modos `NORMAL` (Varredura de telemetria), `HISTORICO` (Auditoria física de dados na EEPROM) e `AJUSTAR` (Calibração manual do RTC via teclado).
*   **Auditoria Minutada Assíncrona:** Rotina baseada em `millis()` que avalia os índices e realiza a gravação apenas se houver violação dos limites seguros, poupando a vida útil de escrita da EEPROM.
*   **Aba Dupla Dinâmica:** Alternância de exibição no visor LCD 16x2 controlada por timeout para evitar travamentos e o efeito de cintilação visual (*flickering*).

---

## 📊 Limites Operacionais de Segurança
*   **Temperatura Mínima:** 20.0 °C  |  **Temperatura Máxima:** 30.0 °C
*   **Humidade Mínima:** 30.0 %  |  **Humidade Máxima:** 60.0 %

Ao ultrapassar qualquer um destes parâmetros, o display entra automaticamente em modo crítico, sinalizando o aviso de emergência e iniciando a retenção automática dos dados em disco.

---

## 🔗 Links do Projeto

*   **Demonstração em Vídeo (YouTube):** [Assista à Operação Física do Projeto](https://youtu.be/wLRaBeLDzy4?si=TLXCD_tmrHCUqJIL)
*   **Simulador de Hardware (Wokwi):** [Execute o Projeto M.E.C.A 4.0 Online](https://wokwi.com/projects/477727989879656449)

---

## 👥 Organização Corporativa
*   **Desenvolvedor Líder:** Gabriel Souza
*   **Grupo de Desenvolvimento:** Mixtape (ADS & Engenharia)
