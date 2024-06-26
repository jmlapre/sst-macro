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

#ifndef SSTMAC_BACKENDS_NATIVE_LAUNCH_ROUNDROBININDEXING_H_INCLUDED
#define SSTMAC_BACKENDS_NATIVE_LAUNCH_ROUNDROBININDEXING_H_INCLUDED

#include <sstmac/software/launch/task_mapper.h>

namespace sstmac {
namespace sw {

/**
 * An index strategy that allocates indices using a round robin.
 */
class RoundRobinTaskMapper : public TaskMapper
{

 public:
  SST_ELI_REGISTER_DERIVED(
    TaskMapper,
    RoundRobinTaskMapper,
    "macro",
    "round",
    SST_ELI_ELEMENT_VERSION(1,0,0),
    "Tries to spread out ranks across the nodes. Ranks 0,1,2,3 are on different nodes."
    "Ranks 0,4,8 are on the same node. Round robin and block indexing are equivalent "
    "if there is one process per node")

  RoundRobinTaskMapper(SST::Params& params) :
    TaskMapper(params)
  {
  }

  std::string toString() const override {
    return "round robin task mapper";
  }

  ~RoundRobinTaskMapper() throw () override {}

  void mapRanks(
    const ordered_node_set& nodes,
    int ppn,
    std::vector<NodeId> &result,
    int nproc) override;

};

}
} // end of namespace sstmac.

#endif
