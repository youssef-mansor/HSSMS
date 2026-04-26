    #include "contiki.h"
    #include "net/rime/rime.h"
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
     
    PROCESS (sensor_node_process, "Sensor Node");
    AUTOSTART_PROCESSES(&sensor_node_process);
     
    static struct broadcast_conn bc;
     
    static void broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from) {}
     
    static const struct broadcast_callbacks bc_cb = {broadcast_recv};
     
    PROCESS_THREAD(sensor_node_process, ev, data)
    {
    static struct etimer timer;
     
    PROCESS_EXITHANDLER(broadcast_close(&bc);)
    PROCESS_BEGIN();
     
    broadcast_open(&bc, 129, &bc_cb);
     
    while(1)
    {
    etimer_set(&timer, CLOCK_SECOND * 5);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
     
    // Simulated sensor values
    int distance = rand() % 200;
    // 0-200 cm
    int temp = 20 + rand() % 15;
    int gas = 300 + rand() % 200; // 300-500 ppm
     
    char msg[64];
    sprintf(msg, "D:%d, T:%d, G:%d", distance, temp, gas);
     
    packetbuf_copyfrom(msg, strlen(msg) + 1);
    broadcast_send(&bc);
     
    printf("Sent: %s\n", msg);
    }
     
    PROCESS_END();
    }
