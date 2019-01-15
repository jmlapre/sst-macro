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

#ifndef PACKETFLOW_CREDITOR_H
#define PACKETFLOW_CREDITOR_H

#include <sprockit/util.h>
#include <sstmac/common/stats/stat_spyplot_fwd.h>
#include <sstmac/hardware/pisces/pisces.h>
#include <sstmac/hardware/pisces/pisces_arbitrator.h>
#include <sstmac/hardware/pisces/pisces_stats.h>
#include <sstmac/common/event_scheduler.h>

#define pisces_debug(...) \
  debug_printf(sprockit::dbg::pisces, __VA_ARGS__)

namespace sstmac {
namespace hw {

struct payload_queue {

  std::list<PiscesPacket*> queue;

  typedef std::list<PiscesPacket*>::iterator iterator;

  PiscesPacket* pop(int num_credits);

  PiscesPacket* front(){
    if (queue.empty()){
      return nullptr;
    }
    return queue.front();
  }


  size_t size() const {
    return queue.size();
  }

  void push_back(PiscesPacket* payload){
    queue.push_back(payload);
  }

};

class PiscesSender : public SubComponent
{
  DeclareFactory(PiscesSender, Component*)
 public:
  struct input {
    int port_to_credit;
    EventLink* link;
    input() : link(nullptr){}
  };

  struct output {
    int arrival_port;
    EventLink* link;
    output() : link(nullptr){}
  };

  virtual ~PiscesSender() {}

  virtual void setInput(sprockit::sim_parameters::ptr& params,
     int my_inport, int dst_outport,
     EventLink* link) = 0;

  virtual void setOutput(sprockit::sim_parameters::ptr& params,
    int my_outport, int dst_inport,
    EventLink* link) = 0;

  virtual void handlePayload(Event* ev) = 0;

  virtual void handleCredit(Event* ev) = 0;

  static void configureCreditPortLatency(sprockit::sim_parameters::ptr& params);

  static void configurePayloadPortLatency(sprockit::sim_parameters::ptr& params);

  void setStatCollector(PacketStatsCallback* c){
    stat_collector_ = c;
  }

  virtual std::string piscesName() const = 0;

  std::string toString() const override;

  Timestamp sendLatency() const {
    return send_lat_;
  }

  Timestamp creditLatency() const {
    return credit_lat_;
  }

 protected:
  PiscesSender(sprockit::sim_parameters::ptr& params,
               SST::Component* parent,
               bool update_vc);

  void sendCredit(input& inp, PiscesPacket* payload,
          Timestamp packet_tail_leaves);

  Timestamp send(PiscesBandwidthArbitrator* arb,
       PiscesPacket* pkt, input& to_credit, output& to_send);

 protected:
  PacketStatsCallback* stat_collector_;

  Timestamp send_lat_;

  Timestamp credit_lat_;

  bool update_vc_;

};

}
}

#endif // PACKETFLOW_CREDITOR_H
