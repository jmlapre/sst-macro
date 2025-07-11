/**
Copyright 2009-2024 National Technology and Engineering Solutions of Sandia,
LLC (NTESS).  Under the terms of Contract DE-NA-0003525, the U.S. Government
retains certain rights in this software.

Sandia National Laboratories is a multimission laboratory managed and operated
by National Technology and Engineering Solutions of Sandia, LLC., a wholly
owned subsidiary of Honeywell International, Inc., for the U.S. Department of
Energy's National Nuclear Security Administration under contract DE-NA0003525.

Copyright (c) 2009-2024, NTESS

All rights reserved.

Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.

    * Neither the name of the copyright holder nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Questions? Contact sst-macro-help@sandia.gov
*/

#ifndef UNIQUE_ID_H
#define UNIQUE_ID_H

#include <sstream>
#include <stdint.h>
#include <sstmac/common/serializable.h>

namespace sstmac {
namespace hw {

struct UniqueEventId {
  uint32_t src_node;
  uint32_t msg_num;

  UniqueEventId(uint32_t src, uint32_t num) :
    src_node(src), msg_num(num) {
  }

  UniqueEventId() :
    src_node(-1), msg_num(0) {
  }

  operator uint64_t() const {
    //map onto single 64 byte number
    uint64_t lo = msg_num;
    uint64_t hi = (((uint64_t)src_node)<<32);
    return lo | hi;
  }

  static void unpack(uint64_t id, uint32_t& node, uint32_t& num){
    num = id; //low 32
    node = (id>>32); //high 32
  }

  std::string toString() const {
    std::stringstream sstr;
    sstr << "unique_id(" << src_node << "," << msg_num << ")";
    return sstr.str();
  }

  void setSrcNode(uint32_t src) {
    src_node = src;
  }

  void setSeed(uint32_t seed) {
    msg_num = seed;
  }

  UniqueEventId& operator++() {
    ++msg_num;
    return *this;
  }

  UniqueEventId operator++(int) {
    UniqueEventId other(*this);
    ++msg_num;
    return other;
  }

  void serialize_order(SST::Core::Serialization::serializer& ser) {
    ser& src_node;
    ser& msg_num;
  }
};


}
}

namespace std {
template <>
struct hash<sstmac::hw::UniqueEventId>
  : public std::hash<uint64_t>
{ };
}




#endif // UNIQUE_ID_H
