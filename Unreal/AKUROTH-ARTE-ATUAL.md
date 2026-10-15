# AKUROTH — teste com as artes 3D do Tripo

As duas malhas de personagem e os dois cenários fornecidos pelo usuário estão integrados ao combate da Unreal. O Core continua calculando turnos, IA, dano, estados e transformação aos 30% de vida. A apresentação troca a malha e a arena quando o estado ascended é ativado.

Os personagens preservam suas texturas PBR. Os FBX recebidos não tinham esqueleto: as cópias de teste usam rigs locais compactos de 10 e 16 ossos, com sete movimentos iniciais por fase (repouso, corte, conjuração, caos, ultimate, transformação e morte). Os cenários chegaram brancos; receberam materiais de pedra escura com variação procedural, bronze e lava emissiva oscilante. O segundo cenário usa a iluminação infernal do combate.

As malhas originais de cerca de 1,9 milhão de faces foram preservadas fora do projeto. As cópias de jogo têm 90 mil faces por personagem e aproximadamente 180 mil por arena. Os FBX otimizados, movimentos e arquivos Blender estão em Design/3D/Akuroth; os recursos importados estão em Content/CharacterModels/Akuroth.

## Verificação desta versão

Windows compilado, empacotado e executado. 75 verificações de integração e 179 verificações em 12 replays de combate, sem falhas. Foram inspecionadas capturas reais das duas fases, com as texturas do personagem e os materiais dos cenários. Não houve falha de shader de material no teste Windows final.

O teste de reimportação FBX confirmou pesos em todos os vértices e dimensões limitadas nos quadros de repouso amostrados. Isso não certifica todos os movimentos como animação final de produção. Não houve gasto adicional de créditos Tripo.

## Como ver

- VER-BATALHA-AKUROTH-TRIPO.cmd: demonstração roteirizada das habilidades usando estados e impactos gerados pelo Core existente. Mantém a janela aberta ao terminar. É uma demonstração, não uma partida interativa.
- JOGAR-TESTE-AKUROTH-TRIPO.cmd: inicia a arena de Akuroth em modo automático, com os controles de combate disponíveis e um save isolado do progresso normal.

## Limites

É um protótipo com a arte 3D real do boss, não a qualidade final da imagem conceitual. Os movimentos ainda precisam de refinamento, especialmente a articulação das roupas e braços. Os heróis continuam usando modelos provisórios da Epic, e os efeitos de habilidade são os efeitos 3D procedurais existentes. O retarget experimental de Hajun permanece separado e não aprovado; esta atualização não o integra ao combate.
