# AKUROTH — O Fim das Realidades

Abra a opção **AKUROTH • Desafio** no menu. O encontro usa Sevrin, Kael, Aelia, Brakk e Lyra no nível 70. É um desafio isolado, sem recompensas e sem alterar a campanha, ouro, recrutamento ou níveis do save.

## Combate

Na primeira fase: Corte do Vazio, Ondas da Escuridão, Prisão das Almas, Meteoro do Eclipse e Rasgo na Realidade. Aos 30% de HP, o boss se transforma permanentemente em Soberano do Absoluto, com Dança do Caos, Colapso Dimensional, Chuva de Eclipses, Reescrita da Existência e Aniquilação Absoluta. A transformação não concede uma ação adicional, cura nem ressurreição.

A Aniquilação tem uma ação de preparação e libera na próxima ação do boss. Controle interrompe a preparação. Na apresentação, uma esfera negra puxa os heróis: compressão em 1,10 s e explosão em 3,00 s da linha de animação. Cada alvo vivo recebe os dois impactos, limitados à vida restante. Os multiplicadores somados são 160% de Essência e 6% do HP máximo do alvo, sujeitos à defesa, proteção e críticos. A recarga começa após o disparo.

A segunda fase reduz dano contínuo em 65%. Reescrita bloqueia habilidades especiais durante uma ação do alvo, mantendo ataques básicos disponíveis. A IA respeita fases e recargas e usa um ataque básico enquanto as especiais recarregam.

## Apresentação

Animação 2D em tempo real: oito poses de ação por fase, movimento pela arena, tecido em movimento, efeitos deformados e deslocados, impactos sincronizados com números e barras de vida, recuo, queda, sangue, fragmentos, marcas no chão, pausas breves no impacto e câmera. A transformação dura 4,6 s; revela cabelos brancos e braços de sombra e muda o cenário para uma cidadela infernal. Chamas, brasas e raios continuam animados entre as ações.

Os efeitos não são vídeos sobrepostos ao combate. A resolução das regras continua no Core; a apresentação agenda impactos reais e retém visualmente o HP anterior até cada impacto. Sons originais são sintetizados em PCM estéreo, com camadas de vento, metal, graves, fragmentos e reverberação. Não há gravações de voz.

O vídeo de referência enviado pelo usuário orientou ritmo, antecipação, reação, movimento e ambiente. Esta implementação mantém a arquitetura 2D existente; não equivale a personagens 3D articulados ou à animação corporal contínua da referência. As oito poses são quadros principais, com movimento procedural entre eles. Uma produção com a mesma fidelidade corporal exigiria rigs e animações adicionais dos heróis e do boss.

## Arte e autorização

Como referência adicional de efeitos, o usuário indicou Dota 2. Foram acrescentadas trilhas curvas com núcleo luminoso e bordas transparentes, rastros de meteoros, ondas de choque no plano do chão e correntes espirais de energia no vórtice. São efeitos procedurais originais, adaptados ao combate 2D; não usam assets extraídos de Dota 2.

Referência original fornecida pelo usuário: **Akuroth_ Soberano do Vazio.png**. A autorização nesta tarefa cobre seu uso como referência para o personagem e suas habilidades. Não foi fornecida uma licença externa verificável; confirme os direitos da referência antes de distribuir comercialmente.

Assets derivados gerados com OpenAI ImageGen a partir da referência, sem copiar personagens de anime. Transparência produzida pelo gerador; metadados de regiões e âncoras calculados por leitura do canal alfa. Arquivos preservados sem recorte destrutivo. Resoluções:

| Arquivo em `Godot/Art` | Resolução |
| --- | --- |
| akuroth-phases-v1.png | 1670 × 942 |
| akuroth-phase1-actions-v2.png | 1670 × 942 |
| akuroth-phase2-actions-v2.png | 1670 × 941 |
| akuroth-inferno-v2.png | 1476 × 1066 |
| akuroth-fire-v1.png | 1024 × 1536 |
| akuroth-fx-cut-v2.png | 2172 × 724 |
| akuroth-fx-waves-v2.png | 2174 × 723 |
| akuroth-fx-prison-v2.png | 1223 × 1286 |
| akuroth-fx-meteor-v2.png | 1670 × 942 |
| akuroth-fx-rift-v2.png | 2046 × 768 |
| akuroth-fx-chaos-v2.png | 1670 × 941 |
| akuroth-fx-collapse-v2.png | 1670 × 942 |
| akuroth-fx-rain-v2.png | 1670 × 941 |
| akuroth-fx-rewrite-v2.png | 1670 × 941 |
| akuroth-fx-annihilation-v2.png | 1670 × 941 |

## Verificação

Regras: `dotnet run --project Tests`. Integração: `dotnet build Godot/ProjectVaelorn.csproj`, usando Godot 4.4.1 .NET e .NET 8.

Apresentação: execute `Godot/AkurothQa.tscn` com o Godot .NET e argumentos de usuário `PASTA_DE_CAPTURAS --frames`. Essa cena isolada verifica assets, fases, transformação, cenário, dez habilidades, preparação/disparo, entrada, derrota e áudio; exporta quadros e WAVs. Capturas usam uma linha de tempo determinística a 24 quadros por segundo. O jogo renderiza normalmente na frequência do dispositivo.

Menu e save: rode o projeto com argumentos de usuário `--qa-akuroth PASTA/akuroth-menu`. A verificação abre o desafio pelo botão real, conclui uma batalha e confere a preservação do progresso.
