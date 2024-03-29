/*
 Copyright 2016-2020 Intel Corporation
 
 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at
 
     http://www.apache.org/licenses/LICENSE-2.0
 
 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
*/
#pragma once

#include "oneapi/ccl/config.h"

#if defined(CCL_ENABLE_SYCL) && defined(MULTI_GPU_SUPPORT)

#include <iostream>
#include <limits>
#include <string>
#include <vector>

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/un.h>
#include <unistd.h>

#include <ze_api.h>

#include "common/comm/comm.hpp"
#include "sched/entry/entry.hpp"
#include "sched/sched.hpp"

#if defined(CCL_ENABLE_SYCL) && defined(MULTI_GPU_SUPPORT)
#include "sched/ze_handle_manager.hpp"
#endif /* CCL_ENABLE_SYCL and MULTI_GPU_SUPPORT */

class ze_handle_exchange_entry : public sched_entry {
public:
    static constexpr const char* class_name() noexcept {
        return "ZE_HANDLES";
    }

    ze_handle_exchange_entry() = delete;
    ze_handle_exchange_entry(ccl_sched* sched,
                             ccl_comm* comm,
                             const std::vector<void*>& in_buffers,
                             ze_context_handle_t context);

    void start() override;
    void update() override;

    const char* name() const override {
        return class_name();
    }

protected:
    void dump_detail(std::stringstream& str) const override {
        ccl_logger::format(str,
                           "rank ",
                           rank,
                           ", comm_size ",
                           comm_size,
                           ", right_peer_socket_name ",
                           right_peer_socket_name,
                           ", left_peer_socket_name ",
                           left_peer_socket_name,
                           ", context ",
                           context,
                           ", in_buffers size ",
                           in_buffers.size(),
                           ", handles size ",
                           handles.size(),
                           "\n");
    }

private:
    static constexpr size_t socket_max_str_len = 100;
    static constexpr int poll_expire_err_code = 0;
    static constexpr int timeout_ms = 1;
    static constexpr size_t max_pfds = 1;

    const ccl_comm* comm;

    std::vector<void*> in_buffers;
    ze_context_handle_t context;

    std::vector<std::vector<ipc_handle_info>> handles;

    const int rank;
    const int comm_size;

    int start_buf_idx;
    int start_peer_idx;

    std::vector<struct pollfd> poll_fds;

    int right_peer_socket;
    int left_peer_socket;
    int left_peer_connect_socket;

    struct sockaddr_un right_peer_addr, left_peer_addr;
    int right_peer_addr_len, left_peer_addr_len;

    std::string right_peer_socket_name;
    std::string left_peer_socket_name;

    bool is_connected;
    bool is_accepted;

    int get_fd_from_handle(const ze_ipc_mem_handle_t* handle, int* fd);
    int get_handle_from_fd(int* fd, ze_ipc_mem_handle_t* handle);

    int create_server_socket(const std::string& socket_name,
                             struct sockaddr_un* socket_addr,
                             int* addr_len,
                             int comm_size);
    int create_client_socket(const std::string& left_peer_socket_name,
                             struct sockaddr_un* sockaddr_cli,
                             int* len);

    int accept_call(int connect_socket,
                    struct sockaddr_un* socket_addr,
                    int* addr_len,
                    const std::string& socket_name,
                    int& sock);
    int connect_call(int sock,
                     struct sockaddr_un* socket_addr,
                     int addr_len,
                     const std::string& socket_name);

    int sendmsg_fd(int sock, int fd, size_t mem_offset);
    int recvmsg_fd(int sock, int& fd, size_t& mem_offset);

    void sendmsg_call(int sock, int fd, size_t mem_offset);
    void recvmsg_call(int sock, int& fd, size_t& mem_offset);

    void get_handle(ze_context_handle_t context, const void* buffer, ze_ipc_mem_handle_t* handle);
    size_t get_mem_offset(ze_context_handle_t context, void* ptr);

    void unlink_sockets();
    void close_sockets();
};

#endif // CCL_ENABLE_SYCL && MULTI_GPU_SUPPORT
