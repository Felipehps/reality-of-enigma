# Hajun — teste de animação experimental

A malha com esqueleto fornecida pelo Tripo foi importada como SK_HajunPhase1. Foram gerados dois IK Rigs, um IK Retargeter e cinco animações derivadas das referências Epic já existentes: repouso, três ataques e ataque carregado. A prévia da ficha ganhou controles de teste para essas animações.

O editor compilou e o teste automatizado acionou cinco controles. Isso confirmou o carregamento e o acionamento das animações, mas NÃO aprovou suas deformações. A análise dos movimentos detectou deslocamentos de mão entre 594 e 3.989 cm em 0,4 s, incompatíveis com o tamanho do personagem. O teste passou a rejeitar deslocamentos acima de 350 cm, além de poses inválidas ou sem movimento.

Estado: EXPERIMENTAL, NÃO APROVADO. Ainda é necessário corrigir a escala da hierarquia e do retarget e revisar corpo, roupa e machado. Esses recursos não substituem o personagem do combate. O executável Windows entregue anteriormente continua sendo a versão estável com a prévia estática, 74 verificações de integração e 33 capturas de interface; ele não foi reempacotado com o teste experimental.

Design/3D/Hajun/retarget-teste.json contém as medições. retarget-unreal-experimental.py registra a geração dos recursos, sem tratar sua criação como aprovação visual.
