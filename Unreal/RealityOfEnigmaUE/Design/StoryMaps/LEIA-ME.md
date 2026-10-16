# Etapa 2 — Rotas da Ruptura

Primeira base jogável da etapa de mapas e história. O Ato I e suas 70 etapas permanecem intactos. Após o Palácio do Sol Partido, quatro rotas ligam os novos bosses à mesma ruptura.

| Rota | Encontro | Consequência narrativa |
|---|---|---|
| Jardim da Última Flor | Zhu Jin | As raízes revelam correntes que recolhem memórias e almas. |
| Fosso do Carrasco | Hajun | O tribunal cai, revelando quem exige as almas condenadas. |
| Corte do Abismo | Mazakin | A queda do selo expõe a cicatriz entre as realidades. |
| Cicatriz das Realidades | Akuroth | O grupo enfrenta a proposta de apagar todos os mundos. |

O atlas do Ato I dá acesso às rotas. Ao concluir a etapa 70, a entrada principal também passa a levar ao Ato II. Cada rota inclui introdução, objetivo, combate e conclusão. A vitória abre a próxima passagem; os encontros utilizam a equipe escolhida pelo jogador. Derrotas e abandono não avançam a história. Ouro e XP de reserva são concedidos uma única vez por rota. Replays são permitidos, com recompensas bloqueadas. ChronicleWins é adicionado ao save versão 1, com coleção vazia ao carregar saves antigos.

## Experimentar

- VER-TELA-INICIAL.cmd abre a campanha normal, respeitando o desbloqueio no Ato I.
- TESTAR-ROTAS-HISTORIA.cmd abre uma cópia isolada de teste, com Ato I concluído e equipe no nível 70. O save real não é alterado. A primeira rota fica liberada; as demais seguem a ordem das vitórias.
- As arenas de teste anteriores continuam acessíveis pelo menu Extras.

## Validação

Os testes nativos verificam bloqueios, identidade dos quatro bosses, uso da equipe escolhida, vitória, derrota, abandono, persistência e recompensas únicas. Para forçar vitória e derrota de forma determinística, os testes de recompensa usam catálogos temporários; o catálogo de produção não é alterado. Isso não certifica o balanceamento da dificuldade. O tour da Unreal verifica o mapa, entradas de história, encontro de Zhu Jin, bloqueios e entrada do Ato II em duas resoluções. A captura de conclusão no tour é uma prévia visual forçada; os testes nativos validam a conclusão por vitória real no cenário controlado. Contagens em validacao.json.

## Limites desta entrega

O mapa é uma interface de progressão em 2D com as ilustrações existentes. Os confrontos usam a apresentação 3D já disponível. Nenhum novo cenário 3D explorável, modelo de boss ou animação foi criado nesta etapa. A produção de cenários e o ajuste da dificuldade seguem a partir desta base narrativa. Não foram usados serviços pagos.
