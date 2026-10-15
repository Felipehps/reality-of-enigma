# Reality of Enigma — entrega funcional Unreal

Verificado em 15/10/2026. A versão Windows independente foi compilada, empacotada e executada. Use ABRIR-JOGO-UNREAL.cmd; não é necessário abrir o editor para jogar. O jogo permanece em desenvolvimento.

## Sistemas migrados

Campanha com 70 encontros e sete regiões, prólogo, fichas, equipe, formação, invocação, níveis, estrelas, caminhos, reconstrução, tickets, recompensas offline, salvamento, treino com três ondas, combate por turnos individuais e arenas de teste. AKUROTH mantém duas fases, transformação aos 30% de vida e dez habilidades. A arena cinematográfica anterior permanece separada da campanha. Foram acrescentados Mazakin, Hajun, Zhu Jin, Tenrin e Zevalon, com 29 habilidades, cinco arenas e duas opções novas no banner de invocação.

A Unreal controla apresentação, entrada e áudio. O núcleo original em C# foi preservado e compilado em uma DLL nativa Win64 que roda dentro do processo da Unreal. O jogador não depende do Godot nem de um runtime .NET separado. Não houve reescrita integral das regras em C++.

## Validação confirmada

| Verificação | Resultado |
| --- | --- |
| Testes das regras na nova base | 441 aprovados |
| Integração com a DLL nativa | 62 verificações, zero falhas |
| Todos os 70 encontros da campanha | 141 verificações, zero falhas |
| Integração no executável Windows atual | 75 verificações, zero falhas |
| Interface no executável Windows | 33 capturas, zero falhas |
| Replays dos personagens novos no executável | 15 estados de combate, 122 verificações, zero falhas |
| Compilação e empacotamento Windows | Concluídos com saída 0 |

O teste dos encontros usa uma equipe de referência de nível 70: venceu 68 de 70 e verificou vitória, derrota e progressão. Não equivale a jogar toda a campanha desde um save novo. Os testes de interface acionam os controles programaticamente; não substituem avaliação manual completa.

## Limites visuais

AKUROTH usa as duas malhas Tripo fornecidas pelo usuário em combate e troca para a segunda forma aos 30% de vida. Os dois cenários Tripo também estão integrados, com materiais de pedra, bronze e lava animada. A versão Windows atual passou por 75 verificações de integração e 179 em 12 replays de AKUROTH, com capturas reais revisadas.

Os heróis e os outros inimigos ainda usam modelos provisórios da Epic. Hajun tem uma prévia estática com seu modelo real na ficha, mas seu retarget de ataques permanece experimental e não aprovado. Os movimentos locais do novo Akuroth também são iniciais, com rigs compactos e efeitos procedurais; não representam acabamento cinematográfico final. Consulte AKUROTH-ARTE-ATUAL.md para os limites e os atalhos de teste. A imagem conceitual não representa a qualidade final do executável.

## Arquivos

- RealityOfEnigma-Unreal/Windows: jogo independente.
- RealityOfEnigmaUE: projeto, código e recursos para edição.
- new-character-previews e new-character-combat-previews: capturas reais da nova versão Windows.
- migration-validation: logs dos testes e do empacotamento.
- COMPILAR-JOGO-UNREAL.cmd: recompila e abre a versão do editor.
- EMPACOTAR-JOGO-UNREAL.cmd: gera outra versão independente.
- TESTAR-JOGO-UNREAL.cmd: testes de regras e integração.
- IMPORTAR-SAVE-UNREAL.ps1: importa para a versão do editor, sem substituir uma jornada existente. Não importa automaticamente para o save do executável independente.

A base Godot foi preservada. Esta entrega cobre a migração funcional; a produção artística continua pendente.

As duas artes 3D do AKUROTH e seus cenários estão em teste no combate; consulte AKUROTH-ARTE-ATUAL.md para a alteração e seus limites.

As artes individuais das fases de Hajun e Zhu Jin estão na galeria, com seleção de fase e proporções preservadas. Consulte ARTES-FASES.md.
