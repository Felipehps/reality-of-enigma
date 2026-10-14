# Reality of Enigma — entrega funcional Unreal

Verificado em 14/10/2026. A versão Windows independente foi compilada, empacotada e executada. Use ABRIR-JOGO-UNREAL.cmd; não é necessário abrir o editor para jogar. O jogo permanece em desenvolvimento.

## Sistemas migrados

Campanha com 70 encontros e sete regiões, prólogo, fichas, equipe, formação, invocação, níveis, estrelas, caminhos, reconstrução, tickets, recompensas offline, salvamento, treino com três ondas, combate por turnos individuais e arenas de teste. AKUROTH mantém duas fases, transformação aos 30% de vida e dez habilidades. A arena cinematográfica anterior permanece separada da campanha.

A Unreal controla apresentação, entrada e áudio. O núcleo original em C# foi preservado e compilado em uma DLL nativa Win64 que roda dentro do processo da Unreal. O jogador não depende do Godot nem de um runtime .NET separado. Não houve reescrita integral das regras em C++.

## Validação confirmada

| Verificação | Resultado |
| --- | --- |
| Testes das regras na nova base | 288 aprovados |
| Integração com a DLL nativa | 62 verificações, zero falhas |
| Todos os 70 encontros da campanha | 141 verificações, zero falhas |
| Integração no executável Windows | 49 verificações, zero falhas |
| Interface no executável Windows | 16 capturas, zero falhas |
| Compilação e empacotamento Windows | Concluídos com saída 0 |

O teste dos encontros usa uma equipe de referência de nível 70: venceu 68 de 70 e verificou vitória, derrota e progressão. Não equivale a jogar toda a campanha desde um save novo. Os testes de interface acionam os controles programaticamente; não substituem avaliação manual completa.

## Limites visuais

Os personagens são modelos Manny/Quinn provisórios da Epic. Os modelos 3D definitivos de AKUROTH e dos heróis ainda não foram criados. As pinturas originais aparecem em menus, fichas, prólogo e fundos distantes; o piso, obstáculos, personagens e efeitos são geometria 3D.

A apresentação atual possui animações básicas e efeitos procedurais. As capturas reais mostram cenário simples e boss excessivamente escuro. Ainda faltam modelagem fiel à arte, animações autorais, efeitos Niagara finais e acabamento cinematográfico. A imagem conceitual não representa a qualidade atual do executável.

## Arquivos

- RealityOfEnigma-Unreal/Windows: jogo independente.
- RealityOfEnigmaUE: projeto, código e recursos para edição.
- unreal-windows-previews: capturas reais do executável.
- migration-validation: logs dos testes e do empacotamento.
- COMPILAR-JOGO-UNREAL.cmd: recompila e abre a versão do editor.
- EMPACOTAR-JOGO-UNREAL.cmd: gera outra versão independente.
- TESTAR-JOGO-UNREAL.cmd: testes de regras e integração.
- IMPORTAR-SAVE-UNREAL.ps1: importa para a versão do editor, sem substituir uma jornada existente. Não importa automaticamente para o save do executável independente.

A base Godot foi preservada. Esta entrega cobre a migração funcional; a produção artística continua pendente.
