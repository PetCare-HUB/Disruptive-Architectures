# PetCare Hub — comedouro virtual

Abra comedouro.html. A simulação começa automaticamente e funciona sem internet, Wokwi ou equipamento.

1. Escolha uma porção e clique em Dispensar ração.
2. A ração sai do reservatório de 200 g e cai no prato de até 40 g.
3. Clique em Pet comer para reduzir a ração do prato e acompanhar a câmera virtual e o gráfico.
4. Pause a refeição, reabasteça o reservatório ou altere a temperatura para testar alertas.
5. Reiniciar restaura o cenário inicial.

A câmera virtual é uma ilustração gerada a partir do estado da simulação. Não captura vídeo nem reconhece imagens. O peso consumido é acumulado durante a sessão; o percentual vazio do prato também muda quando você dispensa mais comida.

O seletor Wokwi / dispositivo via MQTT permite acompanhar o tópico original do firmware pela internet. Nesse modo não há controle remoto do servo: o firmware atual só publica leituras. A câmera virtual passa a representar o percentual recebido, sem exibir imagens reais.

index.html continua sendo o painel da coleira. Use os links no topo para alternar.
