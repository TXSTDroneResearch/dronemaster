/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "ring_buffer.h"

#include <assert.h>
#include <algorithm>
#include <cstring>

RingBuffer::RingBuffer(uint8_t* buffer, size_t capacity)
    : buffer_(buffer), capacity_(capacity) {}

void RingBuffer::AdvanceRead(size_t count) {
  if (read_offset_ + count < capacity_) {
    read_offset_ += count;
  } else {
    size_t left_half = capacity_ - read_offset_;
    size_t right_half = count - left_half;
    read_offset_ = right_half;
  }
}

void RingBuffer::AdvanceWrite(size_t count) {
  if (write_offset_ + count < capacity_) {
    write_offset_ += count;
  } else {
    size_t left_half = capacity_ - write_offset_;
    size_t right_half = count - left_half;
    write_offset_ = right_half;
  }
}

RingBuffer::ReadRange RingBuffer::BeginRead(size_t count) {
  count = std::min(count, capacity_);
  if (!count) {
    return {0};
  }
  if (read_offset_ + count < capacity_) {
    return {buffer_ + read_offset_, count, nullptr, 0};
  } else {
    size_t left_half = capacity_ - read_offset_;
    size_t right_half = count - left_half;
    return {buffer_ + read_offset_, left_half, buffer_, right_half};
  }
}

void RingBuffer::EndRead(ReadRange read_range) {
  if (read_range.second) {
    read_offset_ = read_range.second_length;
  } else {
    read_offset_ += read_range.first_length;
  }
}

size_t RingBuffer::Read(uint8_t* buffer, size_t count) {
  count = std::min(count, capacity_);
  if (!count) {
    return 0;
  }

  // Sanity check: Make sure we don't read over the write offset.
  if (read_offset_ < write_offset_) {
    assert(read_offset_ + count <= write_offset_);
  } else if (read_offset_ + count >= capacity_) {
    size_t left_half = capacity_ - read_offset_;
    assert(count - left_half <= write_offset_);
  }

  if (read_offset_ + count < capacity_) {
    std::memcpy(buffer, buffer_ + read_offset_, count);
    read_offset_ += count;
  } else {
    size_t left_half = capacity_ - read_offset_;
    size_t right_half = count - left_half;
    std::memcpy(buffer, buffer_ + read_offset_, left_half);
    std::memcpy(buffer + left_half, buffer_, right_half);
    read_offset_ = right_half;
  }

  return count;
}

size_t RingBuffer::Write(const uint8_t* buffer, size_t count) {
  count = std::min(count, capacity_);
  if (!count) {
    return 0;
  }

  // Sanity check: Make sure we don't write over the read offset.
  if (write_offset_ < read_offset_) {
    assert(write_offset_ + count <= read_offset_);
  } else if (write_offset_ + count >= capacity_) {
    size_t left_half = capacity_ - write_offset_;
    assert(count - left_half <= read_offset_);
  }

  if (write_offset_ + count < capacity_) {
    std::memcpy(buffer_ + write_offset_, buffer, count);
    write_offset_ += count;
  } else {
    size_t left_half = capacity_ - write_offset_;
    size_t right_half = count - left_half;
    std::memcpy(buffer_ + write_offset_, buffer, left_half);
    std::memcpy(buffer_, buffer + left_half, right_half);
    write_offset_ = right_half;
  }

  return count;
}