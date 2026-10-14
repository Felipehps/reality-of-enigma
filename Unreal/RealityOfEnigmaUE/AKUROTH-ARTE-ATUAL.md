# AKUROTH — melhoria do traje provisório

O AKUROTH recebeu um manto 3D com doze painéis separados e pontas irregulares, costuras vermelhas, correntes na cintura, placas e espinhos nos ombros, eclipse no peito, coroa e espada de sombra. O cabelo escuro aparece na primeira fase; o cabelo branco e os seis braços de sombra da segunda fase foram preservados.

O manto acompanha os ossos animados do corpo e tem movimento de vento. A opção de movimento reduzido desativa essa oscilação; personagens mortos também deixam de oscilar. A espada acompanha a mão durante a animação.

Uma reação aditiva da Epic era usada como animação independente, condição que a Unreal informou não suportar no jogo empacotado. Ela foi removida desse caminho. A reação atual usa recuo e inclinação do corpo no momento de impacto, preservando a animação principal. Não há animação de reação autoral final.

Ainda é um modelo provisório sobre Manny. Não foram entregues rosto, anatomia, texturas, cabelo com mechas modeladas, rig próprio ou acabamento fiel à referência. Os braços, correntes e espada continuam geométricos. Esta alteração melhora o traje de teste; não reproduz a imagem cinematográfica.

## Verificação

A compilação do editor e 12 replays reais de combate foram verificados antes do empacotamento, com 179 verificações de vida, duração e horários de áudio, sem falhas. As capturas reais estão em akuroth-costume-previews. A validação da versão Windows atualizada está registrada em migration-validation/akuroth-costume-packaged.log.

Abra ABRIR-JOGO-UNREAL.cmd e entre na arena de teste do AKUROTH. A segunda fase começa aos 30% de vida.

A versão Windows foi empacotada e executada: 69 verificações de integração e 179 em 12 replays, sem falhas. O aviso da reação aditiva não apareceu no novo teste. O Blender 5.1.1 instalado foi testado e produziu uma base editável separada em Design/3D/Akuroth; consulte seu LEIA-ME.md. Esses arquivos não são o corpo final nem um recurso já importado no jogo.
