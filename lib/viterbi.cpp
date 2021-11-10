// Implementation of ViterbiCodec.
//
// Author: Min Xu <xukmin@gmail.com>
// Date: 01/30/2015

#include <viterbi.h>

#include <algorithm>
#include <cassert>
#include <iostream>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace {

int HammingDistance(const bitarr_t& x, const bitarr_t& y) {
  assert(x.size() == y.size());
  int distance = 0;
  for (int i = 0; i < x.size(); i++) {
    distance += (x[i] != y[i]);
  }
  return distance;
}

}  // namespace

int ReverseBits(int num_bits, int input) {
  assert(input < (1 << num_bits));
  int output = 0;
  while (num_bits-- > 0) {
    output = (output << 1) + (input & 1);
    input >>= 1;
  }
  return output;
}

ViterbiCodec::ViterbiCodec(int constraint, const std::vector<int>& polynomials)
    : constraint_(constraint), polynomials_(polynomials) {
  assert(!polynomials_.empty());
  for (int i = 0; i < polynomials_.size(); i++) {
    assert(polynomials_[i] > 0);
    assert(polynomials_[i] < (1 << constraint_));
  }
  InitializeOutputs();
}

int ViterbiCodec::num_parity_bits() const {
  return polynomials_.size();
}

int ViterbiCodec::NextState(int current_state, int input) const {
  return (current_state >> 1) | (input << (constraint_ - 2));
}

bitarr_t ViterbiCodec::Output(int current_state, int input) const {
  return outputs_.at(current_state | (input << (constraint_ - 1)));
}

bitarr_t ViterbiCodec::Encode(const bitarr_t& bits) const {
  bitarr_t encoded;
  int state = 0;

  // Encode the message bits.
  for (int i = 0; i < bits.size(); i++) {
    auto output = Output(state, bits[i]);
    encoded.insert(encoded.end(), output.begin(), output.end());
    state = NextState(state, bits[i]);
  }

  return encoded;
}

void ViterbiCodec::InitializeOutputs() {
  outputs_.resize(1 << constraint_);
  for (int i = 0; i < outputs_.size(); i++) {
    for (int j = 0; j < num_parity_bits(); j++) {
      // Reverse polynomial bits to make the convolution code simpler.
      int polynomial = ReverseBits(constraint_, polynomials_[j]);
      int input = i;
      int output = 0;
      for (int k = 0; k < constraint_; k++) {
        output ^= (input & 1) & (polynomial & 1);
        polynomial >>= 1;
        input >>= 1;
      }
      outputs_[i].push_back(output);;
    }
  }
}

int ViterbiCodec::BranchMetric(const bitarr_t& bits,
                               int source_state,
                               int target_state) const {
  assert(bits.size() == num_parity_bits());
  assert((target_state & ((1 << (constraint_ - 2)) - 1)) == source_state >> 1);
  const bitarr_t output =
      Output(source_state, target_state >> (constraint_ - 2));

  return HammingDistance(bits, output);
}

std::pair<int, int> ViterbiCodec::PathMetric(
    const bitarr_t& bits,
    const std::vector<int>& prev_path_metrics,
    int state) const {
  int s = (state & ((1 << (constraint_ - 2)) - 1)) << 1;
  int source_state1 = s | 0;
  int source_state2 = s | 1;

  int pm1 = prev_path_metrics[source_state1];
  if (pm1 < std::numeric_limits<int>::max()) {
    pm1 += BranchMetric(bits, source_state1, state);
  }
  int pm2 = prev_path_metrics[source_state2];
  if (pm2 < std::numeric_limits<int>::max()) {
    pm2 += BranchMetric(bits, source_state2, state);
  }

  if (pm1 <= pm2) {
    return std::make_pair(pm1, source_state1);
  } else {
    return std::make_pair(pm2, source_state2);
  }
}

void ViterbiCodec::UpdatePathMetrics(const bitarr_t& bits,
                                     std::vector<int>* path_metrics,
                                     Trellis* trellis) const {
  std::vector<int> new_path_metrics(path_metrics->size());
  std::vector<int> new_trellis_column(1 << (constraint_ - 1));
  for (int i = 0; i < path_metrics->size(); i++) {
    std::pair<int, int> p = PathMetric(bits, *path_metrics, i);
    new_path_metrics[i] = p.first;
    new_trellis_column[i] = p.second;
  }

  *path_metrics = new_path_metrics;
  trellis->push_back(new_trellis_column);
}

bitarr_t ViterbiCodec::Decode(const bitarr_t& bits) const {
  // Compute path metrics and generate trellis.
  Trellis trellis;
  std::vector<int> path_metrics(1 << (constraint_ - 1),
                                std::numeric_limits<int>::max());
  path_metrics.front() = 0;
  for (int i = 0; i < bits.size(); i += num_parity_bits()) {
    bitarr_t current_bits((bits.begin() + i), (bits.begin() + i + num_parity_bits()));
    // If some bits are missing, fill with trailing zeros.
    // This is not ideal but it is the best we can do.
    if (current_bits.size() < num_parity_bits()) {
      int len = num_parity_bits() - current_bits.size();
      current_bits.resize(current_bits.size() + len);
    }
    UpdatePathMetrics(current_bits, &path_metrics, &trellis);
  }

  // Traceback.
  bitarr_t decoded;
  int state = std::min_element(path_metrics.begin(), path_metrics.end()) -
              path_metrics.begin();
  for (int i = trellis.size() - 1; i >= 0; i--) {
    decoded.push_back(state >> (constraint_ - 2));
    state = trellis[i][state];
  }
  std::reverse(decoded.begin(), decoded.end());
  return decoded;
}

