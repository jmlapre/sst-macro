/**
Copyright 2009-2018 National Technology and Engineering Solutions of Sandia, 
LLC (NTESS).  Under the terms of Contract DE-NA-0003525, the U.S.  Government 
retains certain rights in this software.

Sandia National Laboratories is a multimission laboratory managed and operated
by National Technology and Engineering Solutions of Sandia, LLC., a wholly 
owned subsidiary of Honeywell International, Inc., for the U.S. Department of 
Energy's National Nuclear Security Administration under contract DE-NA0003525.

Copyright (c) 2009-2018, NTESS

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

#ifndef SIMPLE_PARAM_EXPANDER_H
#define SIMPLE_PARAM_EXPANDER_H

#include <sstmac/hardware/common/param_expander.h>
#include <sprockit/sim_parameters_fwd.h>

namespace sstmac {
namespace hw {

class LogPParamExpander :
  public ParamExpander
{
  FactoryRegister("logP | simple | LogP | logp | macrels", sstmac::ParamExpander, LogPParamExpander)
 public:
  virtual void expand(sprockit::sim_parameters::ptr& params);

  void expandInto(sprockit::sim_parameters::ptr& dst_params,
    sprockit::sim_parameters::ptr& params,
    sprockit::sim_parameters::ptr& switch_params);

 protected:
  void expandAmm1Nic(sprockit::sim_parameters::ptr& params,
    sprockit::sim_parameters::ptr& nic_params,
    sprockit::sim_parameters::ptr& switch_params);

  void expandAmm1Network(sprockit::sim_parameters::ptr& params,
    sprockit::sim_parameters::ptr& switch_params);

  void expandAmm1Memory(sprockit::sim_parameters::ptr& params,
    sprockit::sim_parameters::ptr& mem_params);

  void expandAmm2Memory(sprockit::sim_parameters::ptr& params,
      sprockit::sim_parameters::ptr& mem_params);

  void expandAmm3Network(sprockit::sim_parameters::ptr& params,
    sprockit::sim_parameters::ptr& switch_params);

  void expandAmm4Nic(sprockit::sim_parameters::ptr& params,
    sprockit::sim_parameters::ptr& nic_params,
    sprockit::sim_parameters::ptr& switch_params);
};

}
}


#endif // SIMPLE_PARAM_EXPANDER_H
