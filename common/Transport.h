// Transport.h
// ZeroMQ wrapper functions for PUB/SUB and PUSH/PULL messaging

#pragma once
#include "Serialization.h"
#include <zmq.hpp>
#include <string>

/// Publish a typed payload on a PUB socket with a topic prefix
/// @tparam T Type with nlohmann to_json support
/// @param socket PUB socket used for sending
/// @param topic Topic string for SUB filtering (e.g. "player1")
/// @param data Typed payload to serialize and publish
template <typename T>
void zmq_publish(zmq::socket_t &socket, const std::string &topic, const T &data)
{
    std::string payload = json_to_string<T>(data);
    socket.send(zmq::buffer(topic), zmq::send_flags::sndmore);
    socket.send(zmq::buffer(payload), zmq::send_flags::none);
}

/// Receive one message from a SUB socket and deserialize into T
/// @tparam T Type with nlohmann from_json support
/// @param socket SUB socket configured with subscriptions
/// @param[out] topic Topic string from received message
/// @param[out] out Deserialized payload
/// @return true if a message was received and parsed, false if no message available
template <typename T>
bool zmq_subscribe_recv(zmq::socket_t &socket, std::string &topic, T &out)
{
    zmq::message_t topicMsg, bodyMsg;
    auto result = socket.recv(topicMsg, zmq::recv_flags::dontwait);
    if (!result)
        return false;

    (void)socket.recv(bodyMsg, zmq::recv_flags::none);
    topic = topicMsg.to_string();

    return json_from_string<T>(bodyMsg.to_string(), out);
}

/// Send a typed payload on a PUSH socket as one JSON frame
/// @tparam T Type with nlohmann to_json support
/// @param socket PUSH socket used for sending
/// @param data Typed payload to serialize and send
template <typename T>
void zmq_push(zmq::socket_t &socket, const T &data)
{
    std::string payload = json_to_string<T>(data);
    socket.send(zmq::buffer(payload), zmq::send_flags::none);
}

/// Receive one JSON frame and deserialize into T
/// @tparam T Type with nlohmann from_json support
/// @param socket Socket to receive from (non-blocking)
/// @param[out] out Deserialized payload
/// @return true if a message was received and parsed, false otherwise
template <typename T>
bool zmq_pull_raw(zmq::socket_t &socket, T &out)
{
    zmq::message_t msg;
    auto result = socket.recv(msg, zmq::recv_flags::dontwait);
    if (!result)
        return false;

    return json_from_string<T>(msg.to_string(), out);
}

/// Receive one raw JSON frame without deserializing.
/// Used by GameMaster to inspect message type before dispatch,
/// since the PULL socket receives both SpellMessage and PlayerAction.
/// @param socket Socket to receive from (non-blocking)
/// @param[out] out Parsed JSON object for type inspection
/// @return true if a message was received, false if no message available
inline bool zmq_pull_raw(zmq::socket_t &socket, json &out)
{
    zmq::message_t msg;
    auto result = socket.recv(msg, zmq::recv_flags::dontwait);
    if (!result)
        return false;

    out = json::parse(msg.to_string());
    return true;
}
