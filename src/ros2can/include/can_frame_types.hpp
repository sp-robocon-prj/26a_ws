#ifndef ROS2CAN_CAN_FRAME_TYPES_HPP_
#define ROS2CAN_CAN_FRAME_TYPES_HPP_

#include <cstdint>
#include <vector>

namespace ros2can {

// 26a CANフォーマットに準拠
struct CanRegisterFrame {
    uint8_t priority; 
    uint8_t data_type; 
    uint8_t board_num; 
    uint16_t register_id;
    std::vector<uint8_t> data;

    // 29bit Extended IDを生成する
    uint32_t build_can_id() const {
        uint32_t id = 0;
        id |= (static_cast<uint32_t>(priority & 0x0F) << 24);
        id |= (static_cast<uint32_t>(data_type & 0x0F) << 20);
        id |= (static_cast<uint32_t>(board_num & 0x0F) << 16);
        id |= (static_cast<uint32_t>(register_id & 0xFFFF));
        return id;
    }

    // 29bit Extended IDから構造体に復元する
    static CanRegisterFrame from_can_id(uint32_t id, const std::vector<uint8_t>& payload = {}) {
        CanRegisterFrame frame;
        frame.priority = (id >> 24) & 0x0F;
        frame.data_type = (id >> 20) & 0x0F;
        frame.board_num = (id >> 16) & 0x0F;
        frame.register_id = id & 0xFFFF;
        frame.data = payload;
        return frame;
    }
};

}

#endif
