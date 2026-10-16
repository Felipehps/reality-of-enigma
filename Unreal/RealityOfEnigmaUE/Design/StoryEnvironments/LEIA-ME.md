# Cenários da jornada — 16/10/2026

Primeira versão tridimensional dos 12 locais definidos pela história atual. Não são níveis de exploração: os ambientes funcionam como palcos de combate por turnos e como uma galeria navegável. O prólogo mantém suas ilustrações narrativas; Campos de Vaelorn pode ser visto na galeria. Os sete ambientes do Ato I são selecionados pela região da batalha. Os quatro bosses adicionais substituem o ambiente regional quando estão entre os inimigos, incluindo arenas de treinamento. Heróis aliados com os mesmos IDs não selecionam arenas inimigas.

## Relação com a história

- **Campos de Vaelorn**: Casa, cerca e árvores representam a vida de Kael antes da explosão.
- **Floresta do Limiar**: Árvores, raízes e cristais violetas assinalam a corrupção dos predadores.
- **Desfiladeiro dos Ecos**: Falésias, ponte de tábuas e cordas conduzem à sentinela Varkhan.
- **Vale das Cinzas**: Casas sem teto, vigas carbonizadas e brasas materializam a perda da família de Kael.
- **Serra do Véu Branco**: Picos claros, gelo e escadaria marcam a passagem que selava as águas antigas.
- **Arquivo das Marés**: Estantes, arcos inundados e raízes ligam Velmora aos registros de Nhal’Zor.
- **Travessia do Eclipse**: Ponte, pilares quebrados e fragmentos violetas recordam a prisão de Maltherion.
- **Palácio do Sol Partido**: Degraus, trono e halo solar partido representam o selo mantido por Solarius.
- **Jardim da Última Flor**: Flores, raízes e água cercam o portal; na transformação, a flor cresce e o jardim escurece.
- **Fosso do Carrasco**: Tribunal, grades, correntes e fosso formam a fortaleza do carrasco.
- **Corte do Abismo**: Trono com espinhos, selo e correntes representam o domínio do rei sobre as almas.
- **Cicatriz das Realidades**: Eclipse negro, rochas suspensas e fendas luminosas representam as realidades em colapso.

## Apresentação e limites

Geometria procedural original, materiais por região, pedra PBR com texturas CC0 já disponíveis localmente, névoa, iluminação e 96 partículas instanciadas por ambiente. Folhas/pólen, neve e brasas têm movimentos distintos. A configuração de movimento reduzido pausa o movimento ambiente. O início das ações pulsa discretamente a iluminação; não adiciona dano nem altera as regras do combate. Ambientes ocultos param de atualizar os efeitos. A geometria só é reconstruída quando muda o local ou a fase.

As quatro transformações usam os sinalizadores do combate existente (ascended / crimson), sem alterar os limiares de HP ou o balanceamento. Mudanças de cor, céu e iluminação acompanham a fase; o jardim também abre sua flor, e as arenas do Submundo ganham espinhos e fendas. Nesta versão a troca de geometria é instantânea no evento de transformação. Não há transição filmada nova nem animação de terreno deformável.

É uma base artística estilizada para avaliação de composição, diferente de um cenário final de grande produção. Ainda são necessários refinamento de materiais, vegetação e arquitetura, água com reflexos/refração (a superfície atual é opaca), áudio ambiente e revisão artística. Não foram adicionados novos modelos ou animações de personagens nesta etapa. O modelo Tripo dos personagens permanece; o palco procedural passa a ser usado na campanha. As arenas Tripo continuam nos arquivos e a arena cinematográfica legada separada permanece disponível.

## Como ver

Abra VER-CENARIOS-HISTORIA.cmd para uma prévia com save isolado, ou use Início → Extras → Cenários da jornada no jogo normal. Anterior / Próximo percorrem os ambientes; Transformação alterna a fase dos bosses. A prévia não desbloqueia capítulos nem muda o save da campanha.

## Reprodução

environments.json contém os textos, a ligação às regiões e as paletas. EnigmaStoryEnvironment gera os elementos 3D; EnigmaWorldPresentation faz a seleção em combate; EnigmaEnvironmentPreview implementa a galeria e a verificação. create-materials.py recria o material no editor usando as quatro texturas incluídas em ArtLibrary/FreeResources. A procedência CC0 está em stone-provenance.json. Não há dependência de assinatura ou recurso pago.

O teste -EnigmaSceneryTour=<pasta> percorre 16 vistas (12 locais e quatro transformações), mais cinco encontros reais de teste do núcleo nativo, verifica seleção regional, seleção de bosses, retorno à primeira fase, ocultação e limite de geometria. As verificações de seleção de bosses usam snapshots de teste alterados, sem atestar o balanceamento ou todas as animações de uma batalha completa. -EnigmaMigrationQA verifica o núcleo existente. Logs e resultados acompanham a entrega.

## Correção de mouse incluída

O HUD da Unreal era desenhado em coordenadas locais da câmera 16:9, mas recebia cliques em pixels do viewport completo. Após voltar do combate, barras laterais/superiores deslocavam os cliques. As áreas de clique e hover agora incluem a origem UnscaledViewRect e a margem segura do Canvas. Layouts antigos são recusados após troca de página, modal ou tamanho da janela. Os testes do mouse calculam posições visuais independentemente dos retângulos dos botões e cobrem 16:9, ultrawide horizontal e barras verticais; não equivalem a certificação em todos os monitores ou configurações de DPI.
