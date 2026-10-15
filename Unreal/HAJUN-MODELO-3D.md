# Hajun — fase 1: primeira malha fornecida

O usuário confirmou que fantasy+warrior+3d+model.zip representa Hajun na primeira fase. O arquivo contém corpo, roupa e machado em uma malha estática, com texturas PBR. Não há esqueleto ou animações.

O Blender 5.1.1 reduziu 1.903.068 triângulos para 120.000, aplicou escala para 1,80 m e exportou o FBX de 4,16 MB. O original e os arquivos Blender permanecem em outputs/Modelos3D/Modelo-recebido-01; a derivação otimizada e as texturas estão em RealityOfEnigmaUE/Design/3D/Hajun. O importador import-unreal.py reconstrói o material na Unreal.

Para consultar: Fichas dos personagens → Hajun → Ver arte completa → Modelo 3D • fase 1. Os botões Girar à esquerda e Girar à direita mudam a câmera em 45 graus. Ficha original, Fase 1 e Fase 2 retornam às referências 2D. A captura da prévia fica desativada fora dessa tela.

A malha real aparece somente na prévia da ficha. O combate permanece com o personagem provisório animado da Epic, até a preparação de arma, esqueleto, pesos e animações próprias. Cabelo, cores e detalhes da prévia correspondem ao modelo recebido; não foi feita reconstrução facial ou finalização artística nesta entrega.

## Esqueleto recebido e corrigido

O Tripo gerou um esqueleto na versão de 80.000 triângulos, com a predefinição UE5 Mannequin solicitada. O FBX baixado foi inspecionado no Blender: 61 ossos, 39.681 vértices e nenhuma animação. Um vértice sem peso foi corrigido por transferência dos pesos do vizinho mais próximo. A reimportação do FBX corrigido confirmou 61 ossos, 80.000 triângulos e zero vértices sem peso.

O FBX corrigido e seu relatório estão em Design/3D/Hajun/Rig. Os Blender editáveis e o ZIP original com rig permanecem em outputs/Modelos3D/Hajun-fase1-com-rig. Este esqueleto ainda não foi retargetado, importado como personagem de combate ou validado em animações na Unreal. A prévia do jogo continua usando a malha estática de 120.000 triângulos.

## Validação da prévia

Compilação e empacotamento Windows concluídos com saída 0. A versão Windows passou em 74 verificações de integração e 33 capturas/verificações de interface, sem falhas. As imagens de frente, costas e retorno à fase 2 foram conferidas. Logs: migration-validation/hajun-model-integration.log, hajun-model-ui.log e hajun-model-package.log.

## Atualização experimental

Foi criado um teste de retarget com cinco movimentos no editor, ainda não aprovado por apresentar deslocamentos exagerados. Consulte HAJUN-TESTE-ANIMACAO.md. Isso não altera o executável Windows estável ou os personagens do combate.
