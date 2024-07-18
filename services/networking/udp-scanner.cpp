
#include <driver/spi_master.h>
#include <driver/gpio.h>
#include "esp_system.h"
#include "esp_log.h"

#include "lwip/sys.h"
#include "lwip/sockets.h"

#include <cstring>
#include <vector>

#include "../../main/main.h"

using namespace std;

static vector<pollfd> net_fds;
static char network_buffer[4096];

TaskHandle_t ptk1;

extern "C" void service_main_udp_scanner(void* param)
{
    int rd = 0;
    int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    int ud = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    net_fds.push_back({
                              .fd = fd,
                              .events = POLLIN,
                              .revents = 0
                      });
    net_fds.push_back({
                              .fd = ud,
                              .events = POLLIN,
                              .revents = 0
                      });

    sockaddr_in local[2];
    memset(local, 0, 2 * sizeof(sockaddr_in));

    local[0].sin_family = AF_INET;
    local[0].sin_addr.s_addr = IPADDR_ANY;
    local[0].sin_port = htons(4530);

    local[1].sin_family = AF_INET;
    local[1].sin_addr.s_addr = IPADDR_ANY;
    local[1].sin_port = htons(4531);

    rd = bind(fd, (const sockaddr*)&local[0], sizeof(sockaddr));
    listen(fd, 5);

    rd = bind(ud, (const sockaddr*)&local[1], sizeof(sockaddr));

    ESP_LOGI(TAG, "Start Listen");
    ESP_LOGI(TAG, "Enter Loop");

    while(1)
    {
        lwip_poll(net_fds.data(), net_fds.size(), 1000);

        if(net_fds[0].revents & POLLIN) //
        {
            sockaddr client_addr = {};
            socklen_t len;
            int client_fd = accept(fd, &client_addr, &len);

            net_fds.push_back({
                                      .fd = client_fd,
                                      .events = POLLIN,
                                      .revents = 0
                              });

            ESP_LOGI("NETWORK", "New client joined. fd : %d", client_fd);
        }
        if(net_fds[1].revents & POLLIN) // DGRAM Server
        {
            sockaddr_in client_addr = {};
            socklen_t len = sizeof(sockaddr);
            int cc = recvfrom(ud, network_buffer, 4096, 0, (sockaddr*)&client_addr, &len);
            if(cc == 0 || cc == -1) // Meets EOF or Error
            {
                ESP_LOGE("NETWORK", "UDP Server Meets Error or EOF.");
                esp_restart();
            }

            char src_ip[32], rep[] = "ACK", rep2[] = "NAK";
            inet_ntop(AF_INET, &client_addr.sin_addr, src_ip, 32);
            ESP_LOGI("NETWORK", "Received %d bytes from %s and %ul.", cc, src_ip, (unsigned  int)len);

            switch(network_buffer[0])
            {
                case 0: // Ping
                    sendto(ud, rep, 3, 0, (sockaddr*)&client_addr, len);
                    break;
                default:
                    sendto(ud, rep2, 3, 0, (sockaddr*)&client_addr, len);
                    break;
            }
        }

        for(int i=2;i<net_fds.size();i++)
        {
            if(net_fds[i].revents & POLLIN)
            {
                int sz = recv(net_fds[i].fd, network_buffer, 4096, 0);
                if(sz == 0 || sz == -1) // Meets EOF
                {
                    int old_fd = net_fds[i].fd;
                    closesocket(net_fds[i].fd);
                    net_fds.erase(net_fds.begin() + i);
                    i--;
                    ESP_LOGW("NETWORK", "Client : %d has broken the connection", old_fd);
                    continue;
                }

                ESP_LOGI("NETWORK", "Recived Data From Client : %d for %d bytes", net_fds[i].fd, sz);
                send(net_fds[i].fd, network_buffer, sz, 0);
            }
        }

    }
}