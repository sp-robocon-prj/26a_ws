#include "udp_can_bridge/udp_manager.hpp"
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <poll.h>
#include <thread>
#include <chrono>
#include <stdexcept>

UdpManager::UdpManager(const std::string& target_ip, uint16_t target_port, uint16_t local_port) {
    sockfd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd_ < 0) {
        throw std::runtime_error("Failed to create UDP socket");
    }

    // ローカルポートのバインド
    struct sockaddr_in local_addr{};
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = INADDR_ANY;
    local_addr.sin_port = htons(local_port);

    if (bind(sockfd_, (struct sockaddr*)&local_addr, sizeof(local_addr)) < 0) {
        close(sockfd_);
        throw std::runtime_error("Failed to bind UDP socket");
    }

    // ターゲットアドレスの設定
    memset(&target_addr_, 0, sizeof(target_addr_));
    target_addr_.sin_family = AF_INET;
    target_addr_.sin_port = htons(target_port);
    if (inet_pton(AF_INET, target_ip.c_str(), &target_addr_.sin_addr) <= 0) {
        close(sockfd_);
        throw std::runtime_error("Invalid IP address");
    }
}

UdpManager::~UdpManager() {
    if (sockfd_ >= 0) {
        close(sockfd_);
    }
}

void UdpManager::wait_for_connection() {
    std::cout << "[UdpManager] Waiting for UDP connection response from " 
              << inet_ntoa(target_addr_.sin_addr) << ":" << ntohs(target_addr_.sin_port) << "..." << std::endl;
              
    const std::string ping_msg = "PING";
    char buffer[1024];

    while (true) {
        // 確認用のパケットを送信
        sendto(sockfd_, ping_msg.c_str(), ping_msg.length(), 0, 
               (struct sockaddr*)&target_addr_, sizeof(target_addr_));

        // 応答を待つ（タイムアウト1000ms）
        struct pollfd pfd;
        pfd.fd = sockfd_;
        pfd.events = POLLIN;

        int ret = poll(&pfd, 1, 1000);

        if (ret > 0 && (pfd.revents & POLLIN)) {
            struct sockaddr_in from_addr;
            socklen_t from_len = sizeof(from_addr);
            ssize_t recv_len = recvfrom(sockfd_, buffer, sizeof(buffer) - 1, 0, 
                                        (struct sockaddr*)&from_addr, &from_len);

            if (recv_len > 0) {
                buffer[recv_len] = '\0';
                std::cout << "[UdpManager] Received response: " << buffer << std::endl;
                std::cout << "[UdpManager] Connection confirmed." << std::endl;
                break;
            }
        }

        std::cout << "[UdpManager] No response, retrying..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

bool UdpManager::send(const std::vector<uint8_t>& data) {
    ssize_t sent = sendto(sockfd_, data.data(), data.size(), 0,
                          (struct sockaddr*)&target_addr_, sizeof(target_addr_));
    return sent == static_cast<ssize_t>(data.size());
}

bool UdpManager::receive(std::vector<uint8_t>& out_data) {
    char buffer[2048];
    struct sockaddr_in from_addr;
    socklen_t from_len = sizeof(from_addr);
    
    // Non-blocking受信の例（必要に応じて poll などを併用）
    ssize_t recv_len = recvfrom(sockfd_, buffer, sizeof(buffer), MSG_DONTWAIT, 
                                (struct sockaddr*)&from_addr, &from_len);
    
    if (recv_len > 0) {
        out_data.assign(buffer, buffer + recv_len);
        return true;
    }
    return false;
}
