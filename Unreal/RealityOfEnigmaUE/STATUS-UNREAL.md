# Reality of Enigma — entrega funcional Unreal

Verificado em 14/10/2026. A versão Windows independente foi compilada, empacotada e executada. Use ABRIR-JOGO-UNREAL.cmd; não é necessário abrir o editor para jogar. O jogo permanece em desenvolvimento.

## Sistemas migrados

Campanha com 70 encontros e sete regiões, prólogo, fichas, equipe, formação, invocação, níveis, estrelas, caminhos, reconstrução, tickets, recompensas offline, salvamento, treino com três ondas, combate por turnos individuais e arenas de teste. AKUROTH mantém duas fases, transformação aos 30% de vida e dez habilidades. A arena cinematográfica anterior permanece separada da campanha. Foram acrescentados Mazakin, Hajun, Zhu Jin, Tenrin e Zevalon, com 29 habilidades, cinco arenas e duas opções novas no banner de invocação.

A Unreal controla apresentação, entrada e áudio. O núcleo original em C# foi preservado e compilado em uma DLL nativa Win64 que roda dentro do processo da Unreal. O jogador não depende do Godot nem de um runtime .NET separado. Não houve reescrita integral das regras em C++.

## Validação confirmada

| Verificação | Resultado |
| --- | --- |
| Testes das regras na nova base | 441 aprovados |
| Integração com a DLL nativa | 62 verificações, zero falhas |
| Todos os 70 encontros da campanha | 141 verificações, zero falhas |
| Integração no executável Windows | 74 verificações, zero falhas |
| Interface no executável Windows | 33 capturas, zero falhas |
| Replays dos personagens novos no executável | 15 estados de combate, 122 verificações, zero falhas |
| Compilação e empacotamento Windows | Concluídos com saída 0 |

O teste dos encontros usa uma equipe de referência de nível 70: venceu 68 de 70 e verificou vitória, derrota e progressão. Não equivale a jogar toda a campanha desde um save novo. Os testes de interface acionam os controles programaticamente; não substituem avaliação manual completa.

## Limites visuais

Os personagens em combate são modelos Manny/Quinn provisórios da Epic. Hajun agora tem uma malha real em uma prévia 3D giratória na ficha e um FBX com esqueleto preparado separadamente para retarget; consulte HAJUN-MODELO-3D.md. Os modelos 3D definitivos de AKUROTH e dos heróis ainda não foram criados. As pinturas originais aparecem em menus, fichas, prólogo e fundos distantes; o piso, obstáculos, personagens e efeitos são geometria 3D.

A apresentação atual possui animações básicas e efeitos procedurais. As capturas novas mostram iluminação corrigida, personagens visíveis, roupas e acessórios procedurais e efeitos 3D provisórios. Ainda faltam modelagem fiel à arte, animações autorais, efeitos Niagara finais e acabamento cinematográfico. A imagem conceitual não representa a qualidade atual do executável.

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

O traje provisório do AKUROTH foi ampliado; consulte AKUROTH-ARTE-ATUAL.md para a alteração e seus limites.

As artes individuais das fases de Hajun e Zhu Jin estão na galeria, com seleção de fase e proporções preservadas. Consulte ARTES-FASES.md.
