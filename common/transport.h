#pragma once
#include "serialization.h"
#include <zmq.hpp>
#include <string>

/**
 * Publish a typed payload on a PUB socket.
 *
 * Parameters:
 * - socket: PUB socket used for sending.
 * - topic: Topic string for SUB filtering.
 * - data: Typed payload to serialize and publish.
 *
 * Returns: void
 */
template <typename T>
void zmq_publish(zmq::socket_t &socket, const std::string &topic, const T &data)
{
    std::string payload = json_to_string<T>(data);
    socket.send(zmq::buffer(topic), zmq::send_flags::sndmore);
    socket.send(zmq::buffer(payload), zmq::send_flags::none);
}

/**
 * Receive one message from a SUB socket and deserialize payload into T.
 *
 * Parameters:
 * - socket: SUB socket configured with subscriptions.
 * - topic: Output topic from received frame 1.
 * - out: Output struct for parsed payload.
 *
 * Returns:
 * - true: Message was received and parsed into out.
 * - false: No message available now, or payload could not be parsed as T.
 */
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

/**
 * Send a typed payload on a PUSH socket as one JSON frame.
 *
 * Parameters:
 * - socket: PUSH socket used for sending.
 * - data: Typed payload to serialize and send.
 *
 * Returns: void
 */
template <typename T>
void zmq_push(zmq::socket_t &socket, const T &data)
{
    std::string payload = json_to_string<T>(data);
    socket.send(zmq::buffer(payload), zmq::send_flags::none);
}

/**
 * Receive one JSON frame and parse it into T.
 *
 * Parameters:
 * - socket: Socket to receive from.
 * - out: Output struct for parsed payload.
 *
 * Returns:
 * - true: Message received and parsed into out.
 * - false: No message available now, or payload could not be parsed as T.
 */
template <typename T>
bool zmq_pull_raw(zmq::socket_t &socket, T &out)
{
    zmq::message_t msg;
    auto result = socket.recv(msg, zmq::recv_flags::dontwait);
    if (!result)
        return false;

    return json_from_string<T>(msg.to_string(), out);
}

/**
 * Receive one JSON frame without deserializing.
 * Used by GameMaster to inspect message type before dispatch
 * (PULL socket receives both SpellMessage and PlayerAction).
 */
inline bool zmq_pull_raw(zmq::socket_t &socket, json &out)
{
    zmq::message_t msg;
    auto result = socket.recv(msg, zmq::recv_flags::dontwait);
    if (!result)
        return false;

    out = json::parse(msg.to_string());
    return true;
}