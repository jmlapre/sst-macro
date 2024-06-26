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

#include <fstream>
#include <sstream>
#include <sstmac/backends/common/parallel_runtime.h>
#include <sstmac/software/launch/hostname_task_mapper.h>
#include <sstmac/software/launch/hostname_allocation.h>
#include <sstmac/software/process/operating_system.h>
#include <sstmac/common/runtime.h>
#include <sprockit/fileio.h>
#include <sprockit/errors.h>
#include <sprockit/sim_parameters.h>
#include <sprockit/keyword_registration.h>

RegisterKeywords(
 { "hostmap", "a line-by-line list of hostnames to map each task to" },
);

namespace sstmac {
namespace sw {

HostnameTaskMapper::HostnameTaskMapper(SST::Params& params) :
  TaskMapper(params)
{
  listfile_ = params.find<std::string>("hostmap");
}

void
HostnameTaskMapper::mapRanks(
  const ordered_node_set&  /*nodes*/,
  int  /*ppn*/,
  std::vector<NodeId> &result,
  int nproc)
{
  int nrank = nproc;
  result.resize(nrank);

  std::ifstream nodelist(listfile_);

  std::stringstream sstr;
  for (int i = 0; i < nrank; i++) {
    std::string hostname;
    nodelist >> hostname;

    sstr << hostname << "\n";

    NodeId nid;
    if (!topology_) {
      spkt_throw_printf(sprockit::ValueError, "hostname_task_mapper: null topology");
    }
    nid = topology_->nodeNameToId(hostname);

    debug_printf(sprockit::dbg::indexing,
        "hostname_task_mapper: rank %d is on hostname %s at nid=%d",
        i, hostname.c_str(), int(nid));

    result[i] = nid;

  }

}

}
}
