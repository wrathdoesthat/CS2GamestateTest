#pragma once

#include <WinSock2.h> // If this isnt at the top compilation explodes
#include <Ws2tcpip.h>
#include <windows.h>

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <ranges>
#include <string>
#include <vector>

#include <chrono>
using namespace std::chrono;
using namespace std::chrono_literals;
using namespace std::string_view_literals;

#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

#include "CS2GamestateData.hh"
#include "ConfigInteraction.hh"
#include "json.hpp"

// How many bytes the recv buffer is (Increase if messages are partially cut off)
// TODO maybe implement buffering the last message and building the full json when its valid
const int32_t recv_buffer_size{10000};
char recv_buffer[recv_buffer_size];

// They expect a 200 OK to recieve the "previously" and "added" fields
const std::string ok_response = "HTTP/1.1 200 OK\r\n";

std::vector<WSAPOLLFD> fds{};

struct StateUpdateData {
    std::chrono::high_resolution_clock::time_point last_successful_poll{};
    bool initial_update{true};
};

int gamestate_thread_fn(nlohmann::json config) {
    auto logger = spdlog::get("main");

    WSADATA wsaData;

    // Reused for all the codes
    int win_result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (win_result != 0) {
        logger->critical("WSA startup failed WSAGetLastError()={}", WSAGetLastError());
        return 1;
    }

    addrinfo hints;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    std::unique_ptr<addrinfo> addrinfo_result = nullptr;
    win_result = getaddrinfo(NULL, config["port"].dump().c_str(), &hints, std::out_ptr(addrinfo_result));
    if (win_result != 0) {
        logger->critical("getaddrinfo failed WSAGetLastError()={}", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    SOCKET listen_socket =
        socket(addrinfo_result->ai_family, addrinfo_result->ai_socktype, addrinfo_result->ai_protocol);
    if (listen_socket == INVALID_SOCKET) {
        logger->critical("listen socket creation failed WSAGetLastError()={}", WSAGetLastError());
        closesocket(listen_socket);
        WSACleanup();
        return 1;
    }

    unsigned long non_blocking = 1;
    if (ioctlsocket(listen_socket, FIONBIO, &non_blocking) == SOCKET_ERROR) {
        logger->critical("Somehow failed to set listen socket to non blocking WSAGetLastError()={}", WSAGetLastError());
        closesocket(listen_socket);
        WSACleanup();
        return 1;
    }

    win_result = bind(listen_socket, addrinfo_result->ai_addr, (int)addrinfo_result->ai_addrlen);
    if (win_result == SOCKET_ERROR) {
        logger->critical("listen sock bind failed WSAGetLastError()={}", WSAGetLastError());
        closesocket(listen_socket);
        WSACleanup();
        return 1;
    }

    if (listen(listen_socket, SOMAXCONN) == SOCKET_ERROR) {
        logger->critical("listen socket listen failed WSAGetLastError()=", WSAGetLastError());
        closesocket(listen_socket);
        WSACleanup();
        return 1;
    }

    fds.push_back(WSAPOLLFD{
        .fd = listen_socket,
        .events = POLLRDNORM,
    });

    StateUpdateData state{.last_successful_poll = high_resolution_clock::now()};

    while (true) {
        int poll_result = WSAPoll(fds.data(), static_cast<ULONG>(fds.size()), 5);
        if (poll_result == SOCKET_ERROR) {
            logger->critical("WSAPoll WSAGetLastError()={}", WSAGetLastError());
            return 1;
        }

        // 10 seconds since the server has said anything
        // we can assume its disconnected for some reason and we need a full update of state
        if (state.last_successful_poll + 10s < high_resolution_clock::now()) {
            state.initial_update = true;
        }

        if (poll_result == 0)
            continue;

        // They seem to spam create and close client sockets
        // I do not know if this is my fault but they seem to send the data fine albeit maybe slow
        if (fds[0].revents & POLLRDNORM) {
            sockaddr_in client_addr{};
            socklen_t client_addr_size = sizeof(client_addr);

            SOCKET client_socket = accept(listen_socket, reinterpret_cast<sockaddr*>(&client_addr), &client_addr_size);
            if (client_socket == INVALID_SOCKET) {
                logger->critical("Failed to accept a socket WSAGetLastError()={}", WSAGetLastError());
                closesocket(client_socket);
            } else if (ioctlsocket(client_socket, FIONBIO, &non_blocking) == SOCKET_ERROR) {
                logger->critical(
                    "Somehow failed to set client socket to non blocking WSAGetLastError()={}", WSAGetLastError());
                closesocket(client_socket);
            } else {
                WSAPOLLFD client_fd{.fd = client_socket, .events = POLLRDNORM, .revents = 0};
                fds.push_back(client_fd);
                logger->debug("created client socket: {}", client_fd.fd);
            }
        }

        //auto start = high_resolution_clock::now();
        for (auto i = fds.begin() + 1; i < fds.end(); i++) {
            if (i->revents & POLLRDNORM) {
                int bytes_read = recv(i->fd, recv_buffer, sizeof(recv_buffer), 0);

                if (bytes_read == 0) {
                    i->revents |= POLLHUP;
                    continue;
                }

                // Send OK first thing
                send(i->fd, ok_response.data(), static_cast<int>(ok_response.size()), 0);
                state.last_successful_poll = high_resolution_clock::now();
                std::string recv_string(recv_buffer, bytes_read);

                size_t found_opening_bracket = recv_string.find_first_of("{");
                if ((found_opening_bracket == std::string::npos) || (recv_string.back() != '}')) {
                    // Sometimes they just send the http header
                    logger->debug("Partial json or only header recieved");
                    continue;
                }

                std::string_view extracted_json(&recv_string[found_opening_bracket], recv_string.size());
                nlohmann::json json = nlohmann::json::parse(extracted_json, nullptr, false);
                if (json.is_discarded()) {
                    logger->warn("JSON parse error (should have been caught by our earlier check)");
                    continue;
                }

                if (state.initial_update) {
                    update_state(gamestate, json);
                    state.initial_update = false;
                } else {
                    // We have to update
                    if (json.contains("previously") || json.contains("added")) {
                        update_state(gamestate, json);
                    }
                }

                //logger->info(json.dump(2));
            }
        }
        //auto time_taken = (high_resolution_clock::now() - start);
        /* logger->info("Main loop took {}ms {}us", std::to_string(duration_cast<milliseconds>(time_taken).count()),
            std::to_string(duration_cast<microseconds>(time_taken).count())); */

        std::erase_if(fds, [listen_socket, &logger](const WSAPOLLFD& fd) {
            if (fd.revents & (POLLERR | POLLHUP)) {
                if (fd.fd != listen_socket) { // is recieving these on the listen socket even possible?
                    if (fd.revents & POLLHUP) {
                        logger->debug("closed client socket: {}", fd.fd);
                    } else {
                        logger->info("Some form of error reading the client causing it to close WSAGetLastError()={}",
                            WSAGetLastError()); // critical or not?
                    }
                    closesocket(fd.fd);
                    return true;
                }
            }
            return false;
        });
    }
};