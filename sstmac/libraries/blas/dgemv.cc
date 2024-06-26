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

#include <sstmac/libraries/blas/blas_api.h>
#include <sprockit/sim_parameters.h>
#include <algorithm>
#include <sstmac/software/libraries/compute/compute_event.h>

namespace sstmac {
namespace sw {

class DefaultDGEMV :
  public BlasKernel
{
 public:
  SST_ELI_REGISTER_DERIVED(
    BlasKernel,
    DefaultDGEMV,
    "macro",
    "default_dgemv",
    SST_ELI_ELEMENT_VERSION(1,0,0),
    "defaut DGEMV kernel")

  DefaultDGEMV(SST::Params& params){
    loop_unroll_ = params.find<double>("dgemv_loop_unroll", 4);
    pipeline_ = params.find<double>("dgemv_pipeline_efficiency", 2);
  }

  std::string toString() const override {
    return "default dgemv";
  }

  ComputeEvent* op_2d(int m, int n) override;

 protected:
  double loop_unroll_;
  double pipeline_;

};

ComputeEvent*
DefaultDGEMV::op_2d(int m, int n)
{
  BasicComputeEvent* msg = new BasicComputeEvent;
  basic_instructions_st& st = msg->data();

  long nops = long(m) * long(n);
  st.flops= nops / long(pipeline_);
  st.intops = nops / long(loop_unroll_) / long(pipeline_);
  st.mem_sequential = long(m)*long(n)*sizeof(double);
  return msg;
}

}
}
