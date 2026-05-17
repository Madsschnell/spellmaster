// zmq_latency_test.cpp
// ZeroMQ latency test for SpellMaster
//
// Measures round-trip time through the same ZeroMQ pattern used in production:
//   Client --PUSH--> Server --PUB--> Client
//
// This mirrors the real data flow:
//   Player --PUSH(spell)--> GameMaster --PUB(GameStatus)--> Player
//
// Build:
//   g++ -std=c++17 zmq_latency_test.cpp -o zmq_latency_test -lzmq
//
// Run (two terminals, same machine or across network):
//   ./zmq_latency_test server
//   ./zmq_latency_test client <server-ip> [message-count] [payload-bytes]
//
// Examples:
//   ./zmq_latency_test server
//   ./zmq_latency_test client localhost              # 100 messages, 256 bytes (default)
//   ./zmq_latency_test client localhost 500          # 500 messages, 256 bytes
//   ./zmq_latency_test client localhost 100 1024     # 100 messages, 1024 bytes
//   ./zmq_latency_test client 192.168.8.100          # test over WiFi to GameMaster RPi

#include <zmq.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <numeric>
#include <csignal>
#include <atomic>

static std::atomic<bool> running{true};

void signalHandler(int) {
    running = false;
}

void runServer() {
    zmq::context_t context(1);

    zmq::socket_t publisher(context, zmq::socket_type::pub);
    publisher.bind("tcp://*:5555");
    std::cout << "PUB bound to port 5555" << std::endl;

    zmq::socket_t receiver(context, zmq::socket_type::pull);
    receiver.bind("tcp://*:5556");
    std::cout << "PULL bound to port 5556" << std::endl;

    std::cout << "Server ready. Waiting for messages..." << std::endl;

    while (running) {
        zmq::message_t msg;
        auto result = receiver.recv(msg, zmq::recv_flags::dontwait);
        if (result) {
            // Echo the message back via PUB with "echo" topic
            publisher.send(zmq::buffer(std::string("echo")), zmq::send_flags::sndmore);
            publisher.send(zmq::message_t(msg.data(), msg.size()), zmq::send_flags::none);
        }
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }

    std::cout << "\nServer stopped." << std::endl;
}

void runClient(const std::string& serverIp, int messageCount, int payloadSize) {
    zmq::context_t context(1);

    zmq::socket_t subscriber(context, zmq::socket_type::sub);
    std::string subAddr = "tcp://" + serverIp + ":5555";
    subscriber.connect(subAddr);
    subscriber.set(zmq::sockopt::subscribe, "echo");

    zmq::socket_t sender(context, zmq::socket_type::push);
    std::string pushAddr = "tcp://" + serverIp + ":5556";
    sender.connect(pushAddr);

    // Let connections stabilize
    std::cout << "Connecting to " << serverIp << "..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Warm up (PUB/SUB needs a moment after subscribe)
    std::cout << "Warming up..." << std::endl;
    for (int i = 0; i < 10; i++) {
        std::vector<char> warmup(payloadSize, 'W');
        sender.send(zmq::buffer(warmup), zmq::send_flags::none);
        zmq::message_t topicMsg, bodyMsg;
        (void)subscriber.recv(topicMsg, zmq::recv_flags::none);
        (void)subscriber.recv(bodyMsg, zmq::recv_flags::none);
    }

    std::cout << "Measuring " << messageCount << " round-trips (" << payloadSize << " bytes each)..." << std::endl;
    std::cout << std::endl;

    std::vector<double> latencies;
    latencies.reserve(messageCount);

    for (int i = 0; i < messageCount && running; i++) {
        // Build payload with sequence number
        std::vector<char> payload(payloadSize, 'X');
        int seq = i;
        std::memcpy(payload.data(), &seq, sizeof(int));

        // Send via PUSH and measure round-trip
        auto start = std::chrono::high_resolution_clock::now();

        sender.send(zmq::buffer(payload), zmq::send_flags::none);

        zmq::message_t topicMsg, bodyMsg;
        (void)subscriber.recv(topicMsg, zmq::recv_flags::none);
        (void)subscriber.recv(bodyMsg, zmq::recv_flags::none);

        auto end = std::chrono::high_resolution_clock::now();

        double us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        latencies.push_back(us);
    }

    if (latencies.empty()) {
        std::cerr << "No measurements taken." << std::endl;
        return;
    }

    // Calculate statistics
    std::sort(latencies.begin(), latencies.end());

    double sum = std::accumulate(latencies.begin(), latencies.end(), 0.0);
    double avg = sum / latencies.size();
    double minVal = latencies.front();
    double maxVal = latencies.back();
    double median = latencies[latencies.size() / 2];

    // Standard deviation
    double sqSum = 0;
    for (double v : latencies) sqSum += (v - avg) * (v - avg);
    double stddev = std::sqrt(sqSum / latencies.size());

    // Percentiles
    double p99 = latencies[(int)(latencies.size() * 0.99)];
    double p95 = latencies[(int)(latencies.size() * 0.95)];

    // Print results
    std::cout << "=== ZeroMQ Latency Test Results ===" << std::endl;
    std::cout << "Pattern:    PUSH/PULL + PUB/SUB round-trip" << std::endl;
    std::cout << "Server:     " << serverIp << std::endl;
    std::cout << "Messages:   " << latencies.size() << std::endl;
    std::cout << "Payload:    " << payloadSize << " bytes" << std::endl;
    std::cout << std::endl;
    std::cout << "Round-trip latency (microseconds):" << std::endl;
    std::cout << "  avg:      " << (int)avg << " us (" << avg / 1000.0 << " ms)" << std::endl;
    std::cout << "  min:      " << (int)minVal << " us (" << minVal / 1000.0 << " ms)" << std::endl;
    std::cout << "  max:      " << (int)maxVal << " us (" << maxVal / 1000.0 << " ms)" << std::endl;
    std::cout << "  median:   " << (int)median << " us (" << median / 1000.0 << " ms)" << std::endl;
    std::cout << "  stddev:   " << (int)stddev << " us (" << stddev / 1000.0 << " ms)" << std::endl;
    std::cout << "  p95:      " << (int)p95 << " us (" << p95 / 1000.0 << " ms)" << std::endl;
    std::cout << "  p99:      " << (int)p99 << " us (" << p99 / 1000.0 << " ms)" << std::endl;
    std::cout << std::endl;
    std::cout << "One-way estimate (half round-trip):" << std::endl;
    std::cout << "  avg:      " << (int)(avg / 2) << " us (" << avg / 2000.0 << " ms)" << std::endl;
    std::cout << "  max:      " << (int)(maxVal / 2) << " us (" << maxVal / 2000.0 << " ms)" << std::endl;
    std::cout << std::endl;

    // Context for the report
    std::cout << "For reference:" << std::endl;
    std::cout << "  GameStatus size:  ~200-300 bytes (JSON serialized)" << std::endl;
    std::cout << "  SpellMessage size: ~50 bytes (JSON serialized)" << std::endl;
    std::cout << "  Human perception:  ~100 ms (100000 us)" << std::endl;
}

int main(int argc, char* argv[]) {
    signal(SIGINT, signalHandler);

    if (argc < 2) {
        std::cerr << "Usage:" << std::endl;
        std::cerr << "  " << argv[0] << " server" << std::endl;
        std::cerr << "  " << argv[0] << " client <server-ip> [message-count] [payload-bytes]" << std::endl;
        std::cerr << std::endl;
        std::cerr << "Defaults: 100 messages, 256 bytes payload" << std::endl;
        return 1;
    }

    std::string mode = argv[1];

    if (mode == "server") {
        runServer();
    } else if (mode == "client") {
        if (argc < 3) {
            std::cerr << "Error: Missing server IP." << std::endl;
            std::cerr << "Usage: " << argv[0] << " client <server-ip> [message-count] [payload-bytes]" << std::endl;
            return 1;
        }
        std::string serverIp = argv[2];
        int messageCount = (argc > 3) ? std::atoi(argv[3]) : 100;
        int payloadSize = (argc > 4) ? std::atoi(argv[4]) : 256;
        runClient(serverIp, messageCount, payloadSize);
    } else {
        std::cerr << "Unknown mode: " << mode << std::endl;
        std::cerr << "Use 'server' or 'client'" << std::endl;
        return 1;
    }

    return 0;
}
