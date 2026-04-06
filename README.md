# Sistema Hidropônico Automatizado 🌿

Esse é um sistema de cultivo hidropônico automatizado, desenvolvido com microcontroladores ESP, pensado para manter as plantas saudáveis com o mínimo de intervenção manual.

<div align="center">
  <img src="https://github.com/user-attachments/assets/4fba2d22-24c4-47ba-ba71-e3f3364ca140" width="400" alt="Dashboard de Irrigação">
</div>


## 📌 Visão Geral

Esse projeto nasceu de uma necessidade simples: não depender de ficar controlando manualmente irrigação e circulação de água o tempo todo.

A ideia foi criar um sistema que:

- Funcione sozinho

- Seja confiável

- Continue operando mesmo sem internet

- E que possa ser expandido com sensores e automações mais avançadas

Hoje ele já consegue manter o fluxo de água e a lógica de funcionamento da hidroponia funcionando de forma estável.

## 🎯 Objetivo

Desenvolver um sistema hidropônico que seja:

- Simples de usar

- Automatizado

- Independente de cloud

- Expansível

- Aplicável tanto em casa quanto em projetos maiores

## 🌱 O que o sistema faz

- Controla a bomba de água

- Automatiza os ciclos de circulação

- Mantém o fluxo constante da solução nutritiva

- Pode ser integrado com sistemas como Home Assistant

- Permite controle manual e automático

Mesmo sem sensores ainda, ele já resolve o principal problema: manter a circulação correta da água, que é o coração da hidroponia.

## 🧠 Como funciona

O funcionamento é bem direto:

- A bomba é acionada em ciclos programados

- A água circula pelo sistema levando os nutrientes

- O controle é feito pelo ESP (ESP8266 ou ESP32)

- Toda a lógica roda localmente

- (Opcional) Pode integrar com MQTT e Home Assistant

## ⚙️ Arquitetura
- Modo local:

      Usuário

      ↓

      Interface / Lógica interna

      ↓

      ESP

      ↓

      Relé

      ↓

      Bomba de água

      ↓

      Sistema hidropônico

- Com automação (Home Assistant):

      Home Assistant

      ↓

      MQTT

      ↓

      ESP

      ↓

      Relé

      ↓

      Bomba de água

      ↓

      Sistema hidropônico

## 🔧 Funcionalidades
### Automação da bomba

- Liga e desliga automaticamente

- Pode funcionar por tempo ou ciclo

- Mantém o fluxo necessário para as plantas

### Controle manual

- Possibilidade de ligar/desligar quando quiser

- Útil para manutenção ou testes

### Integração com automação

- Compatível com MQTT

- Pode ser integrado ao Home Assistant

- Permite controle remoto e criação de automações

### Funcionamento local

- Não depende de internet

- Continua funcionando mesmo offline

- Mais confiável no dia a dia

## 🔌 Hardware utilizado

- ESP8266 ou ESP32

- Módulo relé

- Bomba submersa

- Fonte de alimentação

- Estrutura hidropônica (tubos, reservatório, etc.)

<div align="center">
  <img src="https://github.com/user-attachments/assets/fd3fda51-18b6-4586-a3cc-3f929406d089" width="400" alt="Dashboard de Irrigação">
</div>

## 🔄 Lógica do sistema

O sistema segue uma lógica simples e eficiente:

- Ativa a bomba em intervalos definidos

- Mantém a circulação da solução nutritiva

- Evita que as raízes fiquem sem água

- Garante funcionamento contínuo

## 🛡️ Confiabilidade

- Funciona sem internet

- Reinicia mantendo a lógica

- Baixo risco de falha

- Estrutura simples e robusta

## ⚠️ Limitações atuais

- Ainda não possui sensores (pH, EC, temperatura, etc.)

- Controle baseado apenas em tempo

- A parte física ainda está em fase de instalação completa

## 📈 Próximos passos

Adicionar sensores de:

- Umidade

- Temperatura

- pH

- EC

- Automação mais inteligente (baseada em dados)

- Controle remoto de longa distância (LoRa)

- Dashboard mais completo (SmartControl)

## 🌍 Aplicações

- Cultivo em casa

- Pequenas produções

- Estufas

- Projetos experimentais

## 👨‍💻 Autor

Marcos Gabriel Ferreira Miranda

Desenvolvedor IoT | Automação Residencial e Agrícola | 

Irrigação, Bombas e Controle Remoto | Fundador da SmartControl

Belo Horizonte - MG
