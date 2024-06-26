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

#ifndef SSTMAC_COMMON_MESSAGES_SST_MESSAGE_H_INCLUDED
#define SSTMAC_COMMON_MESSAGES_SST_MESSAGE_H_INCLUDED


#include <sstmac/common/serializable.h>
#include <sstmac/common/node_address.h>
#include <sstmac/common/request.h>
#include <sprockit/printable.h>


namespace sstmac {

class Flow : public Request
{
 public:
  /**
   * Virtual function to return size. Child classes should impement this
   * if they want any size tracked / modeled.
   * @return Zero size, meant to be implemented by children.
   */
  uint64_t byteLength() const {
    return byte_length_;
  }

  ~Flow() override{}

  void setFlowId(uint64_t id) {
    flow_id_ = id;
  }

  uint64_t flowId() const {
    return flow_id_;
  }

  std::string libname() const {
    return libname_;
  }

  void setFlowSize(uint64_t sz) {
    byte_length_ = sz;
  }

  void serialize_order(sstmac::serializer& ser) override {
    ser & flow_id_;
    ser & byte_length_;
    ser & libname_;
  }

 protected:
  Flow(uint64_t id, uint64_t size, const std::string& libname = "") :
    flow_id_(id), byte_length_(size), libname_(libname)
  {
  }

  uint64_t flow_id_;
  uint64_t byte_length_;
  std::string libname_;

};



} // end of namespace sstmac
#endif
