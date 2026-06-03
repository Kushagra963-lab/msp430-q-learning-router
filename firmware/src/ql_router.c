#include "ql_router.h"

#include <stddef.h>

static uint8_t clamp_action_count(uint8_t action_count)
{
    if (action_count == 0u) {
        return 1u;
    }
    if (action_count > QL_MAX_ACTIONS) {
        return QL_MAX_ACTIONS;
    }
    return action_count;
}

void ql_router_init(QlRouter *router, uint8_t node_id, uint8_t action_count)
{
    uint8_t state;
    uint8_t action;

    if (router == NULL) {
        return;
    }

    router->node_id = node_id;
    router->action_count = clamp_action_count(action_count);

    for (state = 0u; state < QL_STATE_COUNT; state += 1u) {
        for (action = 0u; action < QL_MAX_ACTIONS; action += 1u) {
            router->q[state][action] = 0.0f;
        }
    }
}

uint8_t ql_encode_state(uint16_t battery_mv, uint8_t link_quality, uint8_t queue_load)
{
    const uint8_t low_battery = battery_mv < 3000u;
    const uint8_t poor_link = (link_quality < 55u) || (queue_load > 75u);

    if (low_battery && poor_link) {
        return QL_STATE_LOW_BATTERY_POOR_LINK;
    }
    if (low_battery) {
        return QL_STATE_LOW_BATTERY_GOOD_LINK;
    }
    if (poor_link) {
        return QL_STATE_HIGH_BATTERY_POOR_LINK;
    }
    return QL_STATE_HIGH_BATTERY_GOOD_LINK;
}

uint8_t ql_best_action(const QlRouter *router, uint8_t state)
{
    uint8_t action;
    uint8_t best_action = 0u;
    float best_q;

    if ((router == NULL) || (state >= QL_STATE_COUNT)) {
        return 0u;
    }

    best_q = router->q[state][0];

    for (action = 1u; action < router->action_count; action += 1u) {
        if (router->q[state][action] > best_q) {
            best_q = router->q[state][action];
            best_action = action;
        }
    }

    return best_action;
}

uint8_t ql_select_action(const QlRouter *router, uint8_t state, uint8_t exploration_roll, uint8_t random_action)
{
    if (router == NULL) {
        return 0u;
    }

    if (exploration_roll < QL_EPSILON_PERCENT) {
        return (uint8_t)(random_action % router->action_count);
    }

    return ql_best_action(router, state);
}

int16_t ql_calculate_reward(const QlRouteMetrics *metrics)
{
    int16_t reward;
    int16_t latency_penalty;

    if (metrics == NULL) {
        return -10;
    }

    latency_penalty = (int16_t)(metrics->latency_ms / 8u);
    reward = metrics->delivered ? 10 : -10;
    reward -= (int16_t)metrics->energy_cost;
    reward -= latency_penalty;

    return reward;
}

void ql_update(QlRouter *router, uint8_t state, uint8_t action, int16_t reward, uint8_t next_state)
{
    float current_q;
    float max_next_q;
    float learned_value;
    uint8_t next_action;

    if ((router == NULL) ||
        (state >= QL_STATE_COUNT) ||
        (next_state >= QL_STATE_COUNT) ||
        (action >= router->action_count)) {
        return;
    }

    max_next_q = router->q[next_state][0];
    for (next_action = 1u; next_action < router->action_count; next_action += 1u) {
        if (router->q[next_state][next_action] > max_next_q) {
            max_next_q = router->q[next_state][next_action];
        }
    }

    current_q = router->q[state][action];
    learned_value = (float)reward + (QL_GAMMA * max_next_q);
    router->q[state][action] = current_q + (QL_ALPHA * (learned_value - current_q));
}

void ql_build_packet(QlPacket *packet, uint8_t source, uint8_t destination, uint8_t battery_percent)
{
    uint8_t index;

    if (packet == NULL) {
        return;
    }

    packet->source = source;
    packet->destination = destination;
    packet->hop_count = 0u;
    packet->battery_percent = battery_percent;
    packet->delay_ms = 0u;

    for (index = 0u; index < QL_PAYLOAD_BYTES; index += 1u) {
        packet->payload[index] = 0u;
    }
}

