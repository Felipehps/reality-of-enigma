# Akuroth — modelos entregues pelo usuário

Fase1 e Fase2 preservam as malhas e texturas Tripo originais em cópias otimizadas de 90 mil triângulos. Fontes recebidas estão preservadas em outputs/Modelos3D/Akuroth-Tripo.

Os FBX originais não continham esqueleto ou animações. As versões de teste receberam um rig local compacto (10 e 16 ossos) e sete movimentos por fase: Idle, Slash, Cast, Chaos, Ultimate, Transform e Death. São animações iniciais específicas para as poses recebidas, com movimentos limitados; não equivalem a um rig humanoide de produção ou captura de movimento.

Arena1 e Arena2 chegaram sem texturas. As cópias de jogo têm 180 mil triângulos e materiais de basalto, pedra, bronze e lava emissiva, atribuídos às superfícies e reentrâncias da geometria. O piso central foi alinhado à altura dos combatentes.

Integração: /Game/CharacterModels/Akuroth. EnigmaTripoPresentation troca a malha e arena conforme o estado ascended do Core. Limiar de 30% e danos permanecem calculados pelo combate existente.

Verificações e capturas serão registradas após executar a batalha real na Unreal. A reimportação FBX das duas malhas confirmou pesos em todos os vértices e dimensões limitadas nos quadros de repouso amostrados. Isso ainda não substitui revisão visual de todos os movimentos.
