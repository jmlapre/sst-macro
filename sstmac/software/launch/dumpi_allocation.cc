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

#include <sstmac/common/sstmac_config.h>

#include <sstmac/hardware/interconnect/interconnect.h>
#include <sstmac/hardware/topology/cartesian_topology.h>

#include <sstmac/software/launch/dumpi_allocation.h>
#include <sstmac/software/launch/node_allocator.h>
#include <sstmac/dumpi_util/dumpi_util.h>
#include <sstmac/dumpi_util/dumpi_meta.h>

#include <dumpi/libundumpi/libundumpi.h>

#include <sprockit/fileio.h>
#include <sprockit/util.h>
#include <sprockit/sim_parameters.h>

namespace sstmac {
namespace sw {

DumpiAllocation::DumpiAllocation(SST::Params& params)
 : NodeAllocator(params)
{
  metafile_ = params.find<std::string>("dumpi_metaname");
}

bool
DumpiAllocation::allocate(
  int  /*nnode_requested*/,
   const ordered_node_set&  /*available*/,
   ordered_node_set& allocation) const
{
  DumpiMeta* meta = new DumpiMeta(metafile_);
  int nrank = meta->numProcs();
  for (int i = 0; i < nrank; i++) {
    std::string fname = dumpiFileName(i, meta->dirplusfileprefix_);
    dumpi_profile *profile = undumpi_open(fname.c_str());
    if (profile == NULL) {
      spkt_abort_printf("dumpi_allocation::allocate: unable to open %s", fname.c_str());
    }
    dumpi_header *header = dumpi_build_header();
    if (header == NULL) {
      spkt_abort_printf("dumpi_allocation::allocate: header is null");
    }
    dumpi_read_header(profile, header);
    if (header->hostname == NULL || *header->hostname == 0) {
      spkt_abort_printf("dumpi_allocation::allocate: hostname is null or empty");
    }

    if (header->meshdim == 0) {
      //there better be a topology initialized that maps hostnames
      NodeId nid = topology_->nodeNameToId(header->hostname);
      allocation.insert(nid);
    } else { //read mesh coords directly
      hw::CartesianTopology* regtop = safe_cast(hw::CartesianTopology, topology_);
      hw::coordinates coord_vec(header->meshdim);
      for (int i=0; i < header->meshdim; ++i) {
        coord_vec[i] = header->meshcrd[i];
      }
      NodeId nid = regtop->node_addr(coord_vec);
      allocation.insert(nid);
      //spkt_throw_printf(sprockit::input_error,
      //                 "dumpi_allocation::allocate: trace file %s contains no mesh info. No mesh coordinates found.\n"
      //                 "To run the trace you will need to use launch_allocation = hostname.\n"
      //                 "You will also need to give a hostname_map file giving the machine topology.\n"
      //                 "Alternatively, the trace file may just be corrupted.",
      //                 fname.c_str());
    }
    dumpi_free_header(header);
  }
  return true;
}

}
}
