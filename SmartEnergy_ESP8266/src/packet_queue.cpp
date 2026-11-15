#include "packet_queue.h"

#include <string.h>

namespace {
size_t boundedLength(const char* value, size_t limit) {
  size_t length = 0;
  while (length < limit && value[length] != '\0') ++length;
  return length;
}
}

PacketQueue::PacketQueue() : head(0), tail(0), count(0) {
  reset();
}

void PacketQueue::reset() {
  head = 0;
  tail = 0;
  count = 0;

  for (size_t i = 0; i < kCapacity; ++i) {
    buffer[i][0] = '\0';
  }
}

bool PacketQueue::enqueue(const char* packet) {
  if (packet == nullptr || isFull()) {
    return false;
  }

  const size_t length = boundedLength(packet, kPacketLength);
  if (length == 0 || length >= kPacketLength) return false;
  memcpy(buffer[head], packet, length + 1);
  head = (head + 1) % kCapacity;
  ++count;
  return true;
}

bool PacketQueue::dequeue(char* out, size_t maxLength) {
  if (!peek(out, maxLength)) {
    return false;
  }
  return discardOldest();
}

bool PacketQueue::peek(char* out, size_t maxLength) const {
  if (out == nullptr || maxLength == 0 || isEmpty()) return false;
  const size_t length = boundedLength(buffer[tail], kPacketLength);
  if (length >= maxLength) return false;
  memcpy(out, buffer[tail], length + 1);
  return true;
}

bool PacketQueue::discardOldest() {
  if (isEmpty()) return false;
  buffer[tail][0] = '\0';
  tail = (tail + 1) % kCapacity;
  --count;
  return true;
}

bool PacketQueue::isEmpty() const {
  return count == 0;
}

bool PacketQueue::isFull() const {
  return count >= kCapacity;
}

size_t PacketQueue::size() const {
  return count;
}
