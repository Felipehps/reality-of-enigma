# Novos personagens — referência e implementação

## Jogar

Abra ABRIR-JOGO-UNREAL.cmd e entre em Arenas de teste. Mazakin, Hajun e Zhu Jin têm encontros próprios com uma equipe de referência de nível 70. Tenrin e Zevalon têm arenas individuais de demonstração e também foram adicionados ao banner de invocação. A jornada e os saves existentes foram preservados; os 70 encontros originais não foram substituídos.

Em Fichas do elenco, selecione o personagem e use Ver arte completa para abrir a imagem original dentro do jogo. Testar personagem abre a arena correspondente. Durante a luta, Reposicionar consome a ação do herói para mudar a uma casa livre; as Correntes do Submundo impedem esse movimento.

## Conteúdo

| Personagem | Sistemas implementados |
| --- | --- |
| Mazakin | Condenação com seis acúmulos, redução de Defesa e resistência mágica, três orbes destrutíveis, correntes, ritual por casa e transformação aos 20%. Na segunda fase: aura, investida em linha, atração ao centro e Juízo Eterno com rastro de chamas. |
| Hajun | Carnificina ao acertar e receber ataques, ascensão aos 50%, limites de cinco/sete acúmulos, machado, esmagamento, defesa, espada de ossos, três cortes, ultimates e regeneração. Resistência a sangramento de 50%/70%. |
| Zhu Jin | Veneno, queimadura, espinhos, sono e transformações aos 50%/25%. A forma carmesim aumenta poder e controle; o florescimento causa dano contínuo em área. |
| Tenrin | Marcas Divinas, bônus de ataque/velocidade/crítico, proteção na frente, golpe adicional, julgamento, purificação, escudo de equipe e rugido com aceleração e cura. |
| Zevalon | Marcas Celestiais, redução de Defesa/controle, raios e cura ao alcançar cinco marcas, Energia Celestial, dano verdadeiro, escudo com reflexão, purificação e Ira de Zeus. |

São 29 habilidades ativas novas. Os bosses não entram no banner de invocação; os dois heróis entram com o mesmo peso dos seis heróis anteriores. A equipe inicial de uma jornada nova continua igual.

## Adaptações ao combate por turnos

- Os percentuais de ATK/Defesa/Velocidade dos heróis usam bases 140/25/120. Hajun usa essas mesmas bases para reproduzir 100%/120%/90%, passando a 140%/160%/110%. Os bosses recebem HP próprio para enfrentar cinco heróis.
- A Alma Condenada de dez segundos foi adaptada para duas rodadas de bônus de ataque.
- Orbes não possuem turno próprio. Elas pulsam uma rodada após a invocação e, depois, a cada duas rodadas; destruí-las impede esses pulsos. São removidas quando o invocador morre.
- Ritual ocupa a casa do alvo por três rodadas. Sua descrição na imagem mistura duração e acúmulos: foi implementado com dois acúmulos adicionais e extensão de duas ações dos acúmulos existentes.
- A execução do Juízo Eterno segue a explicação entre parênteses da imagem: ignora 80% da Defesa contra Condenação máxima; não garante morte instantânea.
- Hajun mantém habilidades anteriores que têm efeitos explicitamente descritos para a segunda fase, além dos três ataques novos.
- Veneno II de Zhu Jin reutiliza a representação do kit floral existente: veneno com acúmulos adicionais e duração de três ações. Não foi inventada uma porcentagem diferente para um nível de veneno sem valor definido na imagem.
- Zhu Jin transforma aos 50% e floresce aos 25%. Para velocidade, foi priorizado o bônus explícito de 15% da habilidade especial, que difere da tabela ilustrada.
- Energia Celestial tem limite de cinco usos de habilidades especiais. O próximo ataque causa dano verdadeiro, ignorando Defesa. A referência menciona também ignorar 30% da Defesa, o que é redundante para dano verdadeiro.
- A barra de ação de Zevalon foi adaptada para 30% de iniciativa na próxima rodada, preservando o sistema de ordem existente.
- Marcas e Condenação duram quatro ações do alvo quando a ficha não define duração. Purificação remove um efeito negativo, sem apagar todos os efeitos do personagem.

## Arte e animação — estado real

As cinco imagens originais estão preservadas em Design/CharacterReferences e importadas em Content/GameArt. Elas aparecem nas fichas; não são usadas como imagens de ataques.

Os modelos 3D definitivos não foram produzidos. A apresentação reutiliza Manny/Quinn, materiais e sete sequências e reação procedural de impacto da instalação Unreal da Epic. Roupas são superfícies procedurais animadas; cabelos, armas, orbes, corcel e pétalas são representações geométricas provisórias. Não substituem modelagem, texturas, rig e animações autorais fiéis às referências. A forma floral e o corcel são bloqueios visuais para testar a transformação.

Os efeitos atuais usam geometria 3D e instâncias animadas; Niagara está habilitado, mas não foram entregues gráficos Niagara autorais finais. Os sons reutilizam os recursos anteriores. O tempo de impacto passou a ser compartilhado por áudio, vida, reação e queda dos personagens.

## Verificações

- 441 verificações das regras, incluindo as 288 anteriores e os novos kits.
- 62 verificações da biblioteca nativa e 141 dos 70 encontros anteriores, sem falhas.
- 69 verificações de integração no executável Windows, sem falhas.
- 26 telas verificadas na versão Windows, sem falhas.
- 15 replays com estados produzidos pela biblioteca nativa: 122 verificações de vida e áudio no executável Windows, sem falhas.

Nos dez testes automáticos com Tenrin, Zevalon, Aelia, Sevrin e Lyra no nível 70: Mazakin teve nove vitórias da equipe; Hajun, duas; Zhu Jin, dez. Todos atingiram a segunda fase. Isso é uma referência de balanceamento, não uma garantia para qualquer equipe ou modo de jogar.

## Reproduzir os replays

Execute NativeCore/Tests/combat_replay_fixtures.py ou new_character_replays.py com Python, passando uma pasta de saída. Depois inicie a Unreal ou o executável com -EnigmaCombatReplay=<pasta>. O parâmetro usa um save isolado, carrega os estados registrados e verifica HP e horários dos sons; não é um modo normal de campanha. -EnigmaUITour=<pasta> verifica e captura as 26 telas. Os testes de interface usam ações programáticas, sem substituir a avaliação manual de mouse, teclado, som e acabamento artístico.
