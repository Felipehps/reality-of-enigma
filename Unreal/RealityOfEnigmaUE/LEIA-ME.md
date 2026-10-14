# Reality of Enigma — migração para Unreal Engine 5.8.3

A Unreal agora hospeda os sistemas do jogo: menus, campanha, equipe, formação, invocação, progressão, salvamento e combate com turnos individuais. A apresentação visual continua em desenvolvimento: as batalhas usam modelos Manny/Quinn provisórios, animação de ataque e repouso da Epic e efeitos em geometria 3D. Os personagens definitivos e a qualidade da imagem conceitual ainda não foram entregues. Mazakin, Hajun, Zhu Jin, Tenrin e Zevalon foram adicionados com 29 habilidades; consulte NOVOS-PERSONAGENS.md para as regras, artes e adaptações ao sistema por turnos.

## Jogar

Use ABRIR-JOGO-UNREAL.cmd na pasta acima para iniciar a jornada. O lançador usa a versão independente quando disponível; alternativamente inicia o projeto na Unreal instalada. Os menus são controlados pelo mouse. Na batalha, selecione um alvo e clique na habilidade do personagem da vez. 1–9 acionam as habilidades; Espaço resolve a próxima ação automaticamente; Esc volta ao início. A formação pode ser editada antes de iniciar o combate. A barra de cada unidade é individual.

Arenas de teste incluem grupo, elenco, Fenrath/Corpo e Espírito, Sevrin, Velmora, Maltherion, Kael VIII e AKUROTH. A demonstração cinematográfica anterior de AKUROTH permanece separada, com seu balanceamento simplificado e HP compartilhado; ela não é a campanha migrada.

## Sistemas preservados

O catálogo completo contém as 70 fases e as sete regiões. A campanha mantém bloqueios, recompensas, XP dos participantes, convidados narrativos e encerramentos das regiões. O prólogo mantém oito cenas e seus textos. A equipe inicial continua Kael, Savor, Aelia, Brakk e Lyra; Raizen entra como convidado narrativo na floresta. O banner utiliza os pesos do catálogo, incluindo Sevrin.

A biblioteca de regras preserva a formação 3 × 3, ordem por velocidade, alvos, recargas, efeitos de controle, gelo, veneno, sangramento, bônus, sinergias de Lutadores, caminhos e estágios de Kael, Corpo/Espírito de Fenrath, kits de Velmora, Maltherion, Sevrin e as duas fases e dez habilidades de AKUROTH. Treinos mantêm HP e estados entre três ondas e concedem uma única recompensa. Invocação, cópias, Coringas, estrelas, níveis, fragmentos, reconstrução, simulação com tickets e recompensas offline usam as regras originais.

## Arquitetura

Source/RealityOfEnigmaUE contém a interface, os controles e a apresentação Unreal em C++. EnigmaGameInstance carrega Content/Runtime/Win64/Enigma.Native.dll e comunica-se com ela por uma interface JSON. NativeCore contém todo o núcleo independente do Godot em C#, compilado com NativeAOT em uma DLL nativa Win64. A DLL roda no processo da Unreal; o jogador não precisa instalar Godot nem um runtime .NET separado. Esta abordagem preserva as regras; não é uma reescrita de todo o núcleo em C++.

NativeCore/Core é a base de regras da nova versão. A diferença nas rotinas de JSON usa um contexto gerado, compatível com a compilação nativa. Content/GameData contém os dados originais. Dados e DLL são incluídos fora do pacote de assets para permitir a leitura pela biblioteca. Config/DefaultGame.ini inclui explicitamente os diretórios de recursos usados em tempo de execução.

A arena é construída em tempo de execução sobre /Engine/Maps/Entry. Os fundos distantes reutilizam pinturas de regiões em planos; o piso, obstáculos, modelos e efeitos são geometria. A fase 2 de AKUROTH altera o fundo para o inferno, a iluminação, o cabelo e os braços de sombra. A arte 2D de fichas, menus e prólogo foi importada como texturas; isso não equivale à criação de modelos 3D definitivos. Os efeitos atuais são procedurais; ainda não são efeitos Niagara autorais finais.

## Salvamento

A nova jornada usa Saved/Campaign/save-v1.json do projeto. O Godot permanece preservado. O esquema SaveData versão 1 mantém as mesmas propriedades. Arquivos incompatíveis geram erro sem sobrescrita; as gravações usam substituição e backup .bak. Uma versão independente pode usar sua própria pasta Saved, determinada pela Unreal.

## Compilar e testar

A interface requer Unreal 5.8.3, compilador C++ e Windows SDK. Para recompilar as regras, use .NET SDK 10 e execute dotnet publish NativeCore/Enigma.Native.csproj -c Release -r win-x64 -o Content/Runtime/Win64. Para os testes originais aplicados à nova base: dotnet run --project NativeCore/Tests/Enigma.Native.Tests.csproj -c Release.

A integração na Unreal é testada com -game -NullRHI -unattended -nosound -EnigmaMigrationQA. O parâmetro cria um salvamento isolado. -EnigmaUITour=<pasta absoluta> captura os menus e três arenas e verifica limites de botões e ações da interface. -EnigmaCombatReplay=<pasta> reproduz estados reais de combate, verificando vida e áudio e capturando transformações. As verificações chamam os controles programaticamente; não representam um teste manual de todos os dispositivos de entrada.

Consulte o relatório e os logs ao lado deste projeto para os resultados confirmados. O teste de campanha executa todos os encontros com uma equipe de referência de nível 70, verificando encerramento e progressão tanto em vitória quanto em derrota; não comprova que qualquer equipe vence todas as fases.

## Recursos

Os modelos, esqueletos e oito animações são dos templates da instalação Unreal da Epic. Os sons gravados reutilizam os arquivos do projeto original; os sons sintetizados reutilizam as funções originais preservadas em NativeCore/Tools/AudioExporter. Content/GameAudio contém os sons importados; os perfis de cada personagem e habilidade permanecem em GameData/audio.json. A pintura de inferno e o conceito anteriores mantêm sua documentação em Design/ARTE-E-PROMPT.md. O conceito é uma referência artística, não uma captura do jogo.
