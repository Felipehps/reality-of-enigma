# Floresta do Limiar — cenário modelado no Blender

A composição substitui o palco procedural anterior somente na Floresta do Limiar. Ela aparece na galeria e nas batalhas que usam a primeira região. Os demais locais permanecem disponíveis. A pesquisa de jogos atuais, fontes oficiais e a direção original estão em REFERENCIAS.md.

## Conteúdo

- Floresta-do-Limiar.blend: composição editável, com 17 imagens usadas pelos materiais incorporadas ao arquivo. Blender 5.1.1. A árvore e a samambaia de origem ficam ocultas no render; suas instâncias compõem o cenário.
- Export/: três FBX: terreno/arquitetura, árvore e samambaia. As malhas repetidas são compartilhadas em dois grupos de instâncias hierárquicas na Unreal.
- forest-layout.json: posições, escalas e rotações usadas pelo Blender e pela Unreal. A conversão de coordenadas do FBX é aplicada no carregamento da cena.
- Sources/: modelos e texturas CC0; procedência, URLs e hashes em provenance.json.
- create-forest.py e import-unreal.py: geração no Blender e importação/materiais na Unreal.

Foram criados terreno, caminho, arco quebrado, muro, pedras de cantaria, raízes, cristais, fragmentos de selo, riacho e grama. A árvore-base tem 42.080 triângulos e a samambaia-base 2.384. São 79 árvores e 433 samambaias no conjunto completo. A reutilização de malhas evita armazenar centenas de cópias; isso não equivale a uma garantia de FPS em todos os computadores.

Os materiais incluem máscaras de folhas, iluminação de folhagem dos dois lados, relevo, rugosidade, musgo e vento. Névoa volumétrica, luz ambiente e luzes do selo acompanham a composição. Movimento reduzido congela o relógio dos materiais. A zona central de formação permanece nivelada, sem colisão de vegetação interferindo nas regras de combate.

## Como ver

VER-FLORESTA-LIMIAR.cmd abre a galeria diretamente neste local, com save isolado. No jogo normal, use Extras → Cenários da jornada → Floresta do Limiar, ou uma batalha da primeira região. As regras de combate, recompensas e personagens não foram alterados nesta etapa.

floresta-na-unreal.png é a captura limpa do executável Windows, com o HUD oculto apenas para a fotografia. A galeria normal conserva os controles. As capturas com interface e encontros de teste acompanham a validação. blender-preview.png, quando presente, é um estudo renderizado no Blender, não uma captura do jogo.

## Reprodução

Abra o .blend diretamente para editar a composição. Para regenerar, execute create-forest.py no Blender, mantendo a estrutura do projeto e as texturas CC0 da pasta StoryEnvironments. No editor Unreal 5.8, execute import-unreal.py pelo plugin Python ou comando -run=pythonscript. O importador associa explicitamente cada material com set_material e valida as referências; o carregamento em C++ também recusa considerar o conjunto pronto se encontrar materiais de teste.

O cenário é um palco de combate e uma vista de galeria, sem exploração livre. Árvores e samambaias usam modelos gratuitos preparados para o jogo; não são modelos inteiramente esculpidos do zero. A água não tem refração física e ainda cabe refinamento de margens, rochas e animação ambiental. Não se trata de uma promessa de equivalência gráfica a uma produção AAA. Os arquivos e capturas permitem avaliar o resultado efetivo.
