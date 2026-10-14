# Migração para Unreal Engine 5.8.3

A versão funcional Unreal está em `Unreal/RealityOfEnigmaUE`. Ela inclui campanha, equipe, formação, progressão, invocação, treino, salvamento e combate por turnos. O núcleo C# foi compilado em DLL nativa com NativeAOT e roda dentro da Unreal; não depende do Godot ou de um runtime .NET instalado no computador do jogador.

Para editar, abra `Unreal/RealityOfEnigmaUE/RealityOfEnigmaUE.uproject`. Use `Unreal/COMPILAR-JOGO-UNREAL.cmd` para compilar ou `Unreal/EMPACOTAR-JOGO-UNREAL.cmd` para gerar o executável Windows. Requisitos de desenvolvimento: Unreal 5.8.3, ferramentas C++/Windows SDK e .NET SDK 10. Os scripts esperam a instalação em `C:/Program Files/Epic Games/UE_5.8`.

Consulte [o estado da migração](Unreal/STATUS-UNREAL.md) e [as instruções da Unreal](Unreal/RealityOfEnigmaUE/LEIA-ME.md). Os modelos 3D atuais são provisórios da Epic, os efeitos são procedurais e a arte definitiva está pendente. Os executáveis e arquivos de depuração não entram no histórico do repositório; os assets e a DLL nativa necessários estão incluídos.

A base Godot abaixo continua preservada como referência da migração.

---
# Reality Of Enigma

RPG gacha offline em desenvolvimento. Base compartilhada: **V30**, Godot **4.4.1 .NET**, C# e **SDK .NET8**. Personagens, interfaces e animações ainda estão em evolução.

## Começar
1. Instale Git, SDK .NET8 e Godot4.4.1 **.NET** (a edição Standard não executa C#).
2. Clone este repositório. Preserve `Core`, `Godot` e `Tests` lado a lado.
3. Na raiz, execute `dotnet build Godot/ProjectVaelorn.csproj` (o primeiro restore usa NuGet).
4. No Godot, importe `Godot/project.godot`, compile e pressione F5.
5. Execute os testes com `dotnet run --project Tests`.

A referência V30 passou por237 verificações e teste visual do executável. A primeira importação das artes pode demorar. Não é necessário Git LFS para este primeiro snapshot; todos os arquivos estão abaixo do limite individual de100MB. Não coloque builds ou vídeos enormes no histórico; combine a estratégia de armazenamento antes.

## Onde trabalhar
| Área | Arquivos |
| --- | --- |
| Regras independentes do Godot | `Core/` |
| Combate, menus e animações | `Godot/*.cs` |
| Personagens, habilidades, fases | `Godot/Data/catalog.json` |
| Sprites e quadros | `Godot/Art/`, `Godot/Data/animations.json` |
| Prólogo | `Godot/OpeningScene.cs`, `Godot/Main.Opening.cs` |
| Encerramentos dos cenários | `Godot/Data/regions.json`, `Godot/Main.Journey.cs` |
| História e fichas dos heróis | `Godot/Data/hero-stories.json`, `Godot/Main.CharacterCards.cs` |
| Sons e associação de efeitos | `Godot/Audio/`, `Godot/Data/audio*.json` |

Comece por [CONTRIBUTING.md](CONTRIBUTING.md), [guia de personagens e cutscenes](docs/COLABORACAO.md) e [tarefas iniciais](docs/TAREFAS_INICIAIS.md).

## Estado do jogo
Fenrath possui Corpo/Espírito e Sangramento; Sevrin usa lâminas e Sangramento; Velmora é chefe50 e Maltherion chefe60, com ascensão; Solarius encerra o primeiro ato na70. Kael marcial libera VIII estágio no nível70. Campanha, treinamento, gacha de Gold e progressão offline existem como protótipo.

[GDD consolidado](GDD_V25.md) · [adendo V30](GDD_V30_Elenco_e_Combate.md) · [alterações V30](ATUALIZACOES_V30.md). O adendo mais recente prevalece quando houver conflito. Documentos antigos registram versões anteriores, não a especificação atual.

## Exportar
Instale os templates de exportação correspondentes ao Godot4.4.1 .NET. Crie `build/Windows`, escolha o preset Windows Desktop e indique `build/Windows/RealityOfEnigma.exe` dentro da raiz do repositório. O executável precisa permanecer com seu `.pck` e pasta de dados. Builds não são versionadas neste repositório.

## Conteúdo e direitos
Repositório privado de colaboração; nenhuma licença pública é concedida por este envio. Preserve avisos de licenças existentes. Artes e nomes finais devem ser originais. Referências de anime/jogos são internas de design. Áudios fornecidos pelo proprietário permanecem no projeto; a documentação de direitos de distribuição comercial deve ser consolidada antes do lançamento.
