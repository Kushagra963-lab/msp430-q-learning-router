#include "ql_router.h"

#ifdef HOST_SIMULATION
#include <stdio.h>
#endif

#ifndef HOST_SIMULATION
#include <msp430.h>
#endif

#define NODE_ID 1u
#define ACTION_COUNT 3u

static QlRouter router;

#ifdef HOST_SIMULATION
static const char *state_name(uint8_t state)
{
    switch (state) {
    case QL_STATE_HIGH_BATTERY_GOOD_LINK:
        return "S0 high battery, good link";
    case QL_STATE_HIGH_BATTERY_POOR_LINK:
        return "S1 high battery, poor link";
    case QL_STATE_LOW_BATTERY_GOOD_LINK:
        return "S2 low battery, good link";
    case QL_STATE_LOW_BATTERY_POOR_LINK:
        return "S3 low battery, poor link";
    default:
        return "unknown";
    }
}

static void print_q_table(void)
{
    uint8_t state;
    uint8_t action;

    puts("Learned Q-table:");
    for (state = 0u; state < QL_STATE_COUNT; state += 1u) {
        printf("  %s:", state_name(state));
        for (action = 0u; action < ACTION_COUNT; action += 1u) {
            printf(" A%u=%6.2f", action, router.q[state][action]);
        }
        printf(" best=A%u\n", ql_best_action(&router, state));
    }
}

int main(void)
{
    uint16_t episode;

    ql_router_init(&router, NODE_ID, ACTION_COUNT);

    for (episode = 0u; episode < 180u; episode += 1u) {
        const uint16_t battery_mv = (episode < 120u) ? 3250u : 2920u;
        const uint8_t link_quality = (episode % 9u < 6u) ? 78u : 42u;
        const uint8_t queue_load = (episode % 17u) * 5u;
        const uint8_t state = ql_encode_state(battery_mv, link_quality, queue_load);
        const uint8_t explore_roll = (uint8_t)((episode * 37u) % 100u);
        const uint8_t random_action = (uint8_t)((episode * 11u) % ACTION_COUNT);
        const uint8_t action = ql_select_action(&router, state, explore_roll, random_action);
        QlRouteMetrics metrics;
        uint8_t next_state;
        int16_t reward;

        metrics.delivered = (action == 1u || action == 2u || link_quality > 70u) ? 1u : 0u;
        metrics.energy_cost = (uint8_t)(2u + action + (queue_load / 35u));
        metrics.latency_ms = (uint16_t)(12u + (action * 4u) + (100u - link_quality) / 4u);
        reward = ql_calculate_reward(&metrics);
        next_state = ql_encode_state((uint16_t)(battery_mv - metrics.energy_cost), link_quality, queue_load);

        ql_update(&router, state, action, reward, next_state);
    }

    print_q_table();
    return 0;
}
#else
static uint16_t read_battery_mv(void)
{
    return 3200u;
}

static uint8_t read_link_quality(void)
{
    return 75u;
}

static uint8_t read_queue_load(void)
{
    return 10u;
}

static uint8_t next_random_percent(void)
{
    static uint16_t lfsr = 0xACE1u;
    lfsr = (uint16_t)((lfsr >> 1u) ^ (uint16_t)(-(int16_t)(lfsr & 1u) & 0xB400u));
    return (uint8_t)(lfsr % 100u);
}

static void radio_send_packet(uint8_t next_hop, const QlPacket *packet)
{
    (void)next_hop;
    (void)packet;
}

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;

    ql_router_init(&router, NODE_ID, ACTION_COUNT);

    for (;;) {
        uint8_t state;
        uint8_t action;
        QlPacket packet;

        state = ql_encode_state(read_battery_mv(), read_link_quality(), read_queue_load());
        action = ql_select_action(&router, state, next_random_percent(), next_random_percent());
        ql_build_packet(&packet, NODE_ID, 0u, 95u);
        radio_send_packet(action, &packet);
    }
}
#endif

