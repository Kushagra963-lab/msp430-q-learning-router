#ifndef QL_ROUTER_H
#define QL_ROUTER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define QL_STATE_COUNT 4u
#define QL_MAX_ACTIONS 3u
#define QL_PAYLOAD_BYTES 16u

#define QL_ALPHA 0.2f
#define QL_GAMMA 0.8f
#define QL_EPSILON_PERCENT 10u

typedef enum {
    QL_STATE_HIGH_BATTERY_GOOD_LINK = 0,
    QL_STATE_HIGH_BATTERY_POOR_LINK = 1,
    QL_STATE_LOW_BATTERY_GOOD_LINK = 2,
    QL_STATE_LOW_BATTERY_POOR_LINK = 3
} QlState;

typedef struct {
    uint8_t source;
    uint8_t destination;
    uint8_t hop_count;
    uint8_t battery_percent;
    uint16_t delay_ms;
    uint8_t payload[QL_PAYLOAD_BYTES];
} QlPacket;

typedef struct {
    uint8_t delivered;
    uint8_t energy_cost;
    uint16_t latency_ms;
} QlRouteMetrics;

typedef struct {
    uint8_t node_id;
    uint8_t action_count;
    float q[QL_STATE_COUNT][QL_MAX_ACTIONS];
} QlRouter;

void ql_router_init(QlRouter *router, uint8_t node_id, uint8_t action_count);
uint8_t ql_encode_state(uint16_t battery_mv, uint8_t link_quality, uint8_t queue_load);
uint8_t ql_best_action(const QlRouter *router, uint8_t state);
uint8_t ql_select_action(const QlRouter *router, uint8_t state, uint8_t exploration_roll, uint8_t random_action);
int16_t ql_calculate_reward(const QlRouteMetrics *metrics);
void ql_update(QlRouter *router, uint8_t state, uint8_t action, int16_t reward, uint8_t next_state);
void ql_build_packet(QlPacket *packet, uint8_t source, uint8_t destination, uint8_t battery_percent);

#ifdef __cplusplus
}
#endif

#endif

