#ifndef PACKET_QUEUE_H
#define PACKET_QUEUE_H

#include <Arduino.h>

struct PacketQueue {
  static constexpr size_t kCapacity = 32;
  static constexpr size_t kPacketLength = 256;

  char buffer[kCapacity][kPacketLength];
  uint16_t head;
  uint16_t tail;
  uint16_t count;

  PacketQueue();
  void reset();
  bool enqueue(const char* packet);
  bool dequeue(char* out, size_t maxLength);
  bool peek(char* out, size_t maxLength) const;
  bool discardOldest();
  bool isEmpty() const;
  bool isFull() const;
  size_t size() const;
};

#endif
