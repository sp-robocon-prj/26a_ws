#pragma once

#include <string>
#include <vector>
#include <netinet/in.h>

class UdpManager {
public:
    UdpManager(const std::string& target_ip, uint16_t target_port, uint16_t local_port);
    ~UdpManager();

    // 接続確認が終わるまでブロックする
    void wait_for_connection();

    // 任意のデータを送信（将来の拡張用）
    bool send(const std::vector<uint8_t>& data);

    // データを受信（将来の拡張用）
    bool receive(std::vector<uint8_t>& out_data);

private:
    int sockfd_;
    struct sockaddr_in target_addr_;
};
