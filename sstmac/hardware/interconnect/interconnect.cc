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

#include <sstmac/hardware/interconnect/interconnect.h>
#include <sstmac/hardware/topology/structured_topology.h>
#include <sstmac/hardware/node/node.h>
#include <sstmac/hardware/nic/nic.h>
#include <sstmac/hardware/network/network_message.h>
#include <sstmac/hardware/topology/topology.h>
#include <sstmac/hardware/pisces/pisces.h>
#include <sstmac/hardware/switch/network_switch.h>
#include <sstmac/hardware/logp/logp_param_expander.h>
#include <sstmac/hardware/logp/logp_switch.h>
#include <sstmac/backends/common/parallel_runtime.h>
#include <sstmac/backends/common/sim_partition.h>
#include <sstmac/common/runtime.h>
#include <sstmac/common/event_manager.h>
#include <sprockit/keyword_registration.h>
#include <sprockit/statics.h>
#include <sprockit/delete.h>
#include <sprockit/output.h>
#include <sprockit/sim_parameters.h>
#include <sprockit/util.h>

RegisterDebugSlot(interconnect);

namespace sstmac {
namespace hw {

Interconnect* Interconnect::static_interconnect_ = nullptr;

Interconnect*
Interconnect::staticInterconnect(sprockit::sim_parameters::ptr& params, EventManager* mgr)
{
  if (!static_interconnect_){
    ParallelRuntime* rt = ParallelRuntime::staticRuntime(params);
    Partition* part = rt ? rt->topologyPartition() : nullptr;
    static_interconnect_ = Interconnect::factory::get_value("switch", params,
      mgr, part, rt);
  }
  return static_interconnect_;
}

Interconnect*
Interconnect::staticInterconnect()
{
  if (!static_interconnect_){
    spkt_abort_printf("interconnect not initialized");
  }
  return static_interconnect_;
}

#if !SSTMAC_INTEGRATED_SST_CORE
Interconnect::~Interconnect()
{
  sprockit::delete_vector(nodes_);
  sprockit::delete_vector(logp_switches_);
  sprockit::delete_vector(switches_);
}
#endif

Interconnect::Interconnect(sprockit::sim_parameters::ptr& params, EventManager *mgr,
                           Partition *part, ParallelRuntime *rt)
{
  if (!static_interconnect_) static_interconnect_ = this;
  topology_ = Topology::staticTopology(params);
  num_nodes_ = topology_->numNodes();
  num_switches_ = topology_->numSwitches();
  num_leaf_switches_ = topology_->numLeafSwitches();
  Runtime::setTopology(topology_);

#if !SSTMAC_INTEGRATED_SST_CORE
  components_.resize(topology_->numNodes() + topology_->numSwitches());

  partition_ = part;
  rt_ = rt;
  int nproc = rt_->nproc();
  num_speedy_switches_with_extra_node_ = num_nodes_ % nproc;
  num_nodes_per_speedy_switch_ = num_nodes_ / nproc;

  sprockit::sim_parameters::ptr empty{};
  sprockit::sim_parameters::ptr node_params = params->get_namespace("node");
  sprockit::sim_parameters::ptr nic_params = node_params->get_namespace("nic");
  sprockit::sim_parameters::ptr inj_params = nic_params->get_namespace("injection");
  sprockit::sim_parameters::ptr switch_params = params->get_namespace("switch");
  sprockit::sim_parameters::ptr ej_params = switch_params->get_optional_namespace("ejection");

  Topology* top = topology_;

  std::string switch_model = switch_params->get_lowercase_param("name");
  bool logp_model = switch_model == "logp" || switch_model == "simple" || switch_model == "macrels";

  switches_.resize(num_switches_);
  nodes_.resize(num_nodes_);

  sprockit::sim_parameters::ptr logp_params = std::make_shared<sprockit::sim_parameters>();
  if (logp_model){
    switch_params->combine_into(logp_params);
  } else {
    LogPParamExpander expander;
    expander.expandInto(logp_params, params, switch_params);
  }

  logp_switches_.resize(rt_->nthread());
  uint32_t my_offset = rt_->me() * rt_->nthread() + top->numNodes() + top->numSwitches();
  for (int i=0; i < rt_->nthread(); ++i){
    uint32_t id = my_offset + i;
    logp_switches_[i] = new LogPSwitch(logp_params, id);
  }



  interconn_debug("Interconnect building endpoints");
  buildEndpoints(node_params, nic_params, mgr);
  connectLogP(mgr, node_params, nic_params);
  if (!logp_model){
    interconn_debug("Interconnect building switches");
    buildSwitches(switch_params, mgr);
    interconn_debug("Interconnect connecting switches");
    connectSwitches(mgr, switch_params);
    interconn_debug("Interconnect connecting endpoints");
    connectEndpoints(mgr, inj_params, inj_params, ej_params);
    configureInterconnectLookahead(params);
  } else {
    //lookahead is actually higher
    LogPSwitch* lsw = logp_switches_[0];
    lookahead_ = lsw->sendLatency(empty);
  }

  Timestamp lookahead_check = lookahead_;
  if (EventLink::minRemoteLatency().ticks() > 0){
    lookahead_check = EventLink::minRemoteLatency();
  }
  if (EventLink::minThreadLatency().ticks() > 0){
    lookahead_check = std::min(lookahead_check, EventLink::minThreadLatency());
  }

  if (lookahead_check < lookahead_){
    spkt_abort_printf("invalid lookahead compute: computed lookahead to be %8.4e, "
                      "but have link with lookahead %8.4e", lookahead_.sec(), lookahead_check.sec());
  }

#endif
}

#if !SSTMAC_INTEGRATED_SST_CORE
void
Interconnect::configureInterconnectLookahead(sprockit::sim_parameters::ptr& params)
{
  sprockit::sim_parameters::ptr switch_params = params->get_namespace("switch");
  sprockit::sim_parameters::ptr inj_params = params->get_namespace("node")
      ->get_namespace("nic")->get_namespace("injection");
  sprockit::sim_parameters::ptr ej_params = params->get_optional_namespace("ejection");

  sprockit::sim_parameters::ptr link_params = switch_params->get_namespace("link");
  Timestamp hop_latency;
  if (link_params->has_param("sendLatency")){
    hop_latency = link_params->get_time_param("sendLatency");
  } else {
    hop_latency = link_params->get_time_param("latency");
  }
  Timestamp injection_latency = inj_params->get_time_param("latency");

  Timestamp ejection_latency = injection_latency;
  if (ej_params->has_param("latency")){
    ejection_latency = ej_params->get_time_param("latency");
  } else if (ej_params->has_param("sendLatency")){
    ejection_latency = ej_params->get_time_param("sendLatency");
  }

  lookahead_ = std::min(injection_latency, hop_latency);
  lookahead_ = std::min(lookahead_, ejection_latency);
}
#endif

SwitchId
Interconnect::nodeToLogpSwitch(NodeId nid) const
{
#if ACTUAL_INTEGRATED_SST_CORE
  return topology_->nodeToLogpSwitch(nid);
#else
  SwitchId real_sw_id = topology_->endpointToSwitch(nid);
  int target_rank = partition_->lpidForSwitch(real_sw_id);
  return target_rank;
#endif
}


#if !ACTUAL_INTEGRATED_SST_CORE

#if 0
EventLink*
Interconnect::allocateIntraProcLink(Timestamp latency, EventManager* mgr, EventHandler* handler,
                                EventScheduler* src, EventScheduler* dst)
{
  return new LocalLink(latency,mgr,handler, src->componentId(), dst->componentId());
  if (src->threadId() == dst->threadId()){

  } else {
    return new MultithreadLink(handler,latency,mgr,dst,
                               src->componentId(), dst->componentId());
  }
}
#endif

void
Interconnect::connectEndpoints(EventManager* mgr,
                               sprockit::sim_parameters::ptr& ep_inj_params,
                               sprockit::sim_parameters::ptr& ep_ej_params,
                               sprockit::sim_parameters::ptr& sw_ej_params)
{
  int num_nodes = topology_->numNodes();
  int num_switches = topology_->numSwitches();
  int me = rt_->me();
  std::vector<Topology::injection_port> ports;
  for (int i=0; i < num_switches; ++i){
    //parallel - I don't own this
    int target_rank = partition_->lpidForSwitch(i);
    int target_thread = partition_->threadForSwitch(i);
    if (target_rank != me && target_thread != mgr->thread()){
      continue;
    }

    NetworkSwitch* injsw = switches_[i];
    NetworkSwitch* ejsw = switches_[i];

    topology_->endpointsConnectedToInjectionSwitch(i, ports);
    for (Topology::injection_port& p : ports){
      Node* ep = nodes_[p.nid];

      interconn_debug("connecting switch %d:%p to injector %d:%p on ports %d:%d",
          i, injsw, p.nid, ep, p.switch_port, p.ep_port);

      auto credit_link = new LocalLink(injsw->creditLatency(sw_ej_params), mgr,
                                       ep->creditHandler(p.ep_port));

      injsw->connectInput(sw_ej_params, p.ep_port, p.switch_port, credit_link);
      auto payload_link = new LocalLink(ep->sendLatency(ep_inj_params), mgr,
                                        injsw->payloadHandler(p.switch_port));
      ep->connectOutput(ep_inj_params, p.ep_port, p.switch_port, payload_link);
    }

    topology_->endpointsConnectedToEjectionSwitch(i, ports);
    for (Topology::injection_port& p : ports){
      Node* ep = nodes_[p.nid];

      interconn_debug("connecting switch %d:%p to ejector %d:%p on ports %d:%d",
          int(i), ejsw, p.nid, ep, p.switch_port, p.ep_port);

      auto payload_link = new LocalLink(ejsw->sendLatency(sw_ej_params), mgr, ep->payloadHandler(p.ep_port));
      ejsw->connectOutput(sw_ej_params, p.switch_port, p.ep_port, payload_link);

      auto credit_link = new LocalLink(ep->creditLatency(ep_ej_params), mgr, ejsw->creditHandler(p.switch_port));
      ep->connectInput(ep_ej_params, p.switch_port, p.ep_port, credit_link);
    }
  }

}

void
Interconnect::setup()
{
  for (Node* node : nodes_){
    if (node){
      node->init(0);
      node->setup();
    }
  }

  for (NetworkSwitch* sw : switches_){
    if (sw){
      sw->init(0);
      sw->setup();
    }
  }

  for (LogPSwitch* sw : logp_switches_){
    if (sw){
      sw->init(0);
      sw->setup();
    }
  }
}

void
Interconnect::connectLogP(EventManager* mgr,
  sprockit::sim_parameters::ptr& node_params,
  sprockit::sim_parameters::ptr& nic_params)
{
  sprockit::sim_parameters::ptr inj_params = nic_params->get_namespace("injection");
  sprockit::sim_parameters::ptr empty{};

  int my_rank = rt_->me();
  int my_thread = mgr->thread();

  LogPSwitch* local_logp_switch = logp_switches_[my_thread];
  for (int i=0; i < num_switches_; ++i){
    SwitchId sid(i);
    std::vector<Topology::injection_port> nodes;
    topology_->endpointsConnectedToInjectionSwitch(sid, nodes);
    if (nodes.empty())
      continue;

    int target_thread = partition_->threadForSwitch(i);
    int target_rank = partition_->lpidForSwitch(sid);

    for (Topology::injection_port& conn : nodes){
      Node* nd = nodes_[conn.nid];
      if (my_rank == target_rank && my_thread == target_thread){
        //nic sends to only its specific logp switch
        EventLink* logp_link = new LocalLink(Timestamp(0), mgr,
            local_logp_switch->payloadHandler(conn.switch_port));
        nd->nic()->connectOutput(inj_params, NIC::LogP, conn.switch_port, logp_link);

        EventLink* out_link = new LocalLink(local_logp_switch->sendLatency(empty), mgr, nd->payloadHandler(NIC::LogP));
        local_logp_switch->connectOutput(conn.nid, out_link);
      } else if (my_rank == target_rank){
        EventLink* out_link = new MultithreadLink(local_logp_switch->sendLatency(empty),
            mgr, EventManager::global->threadManager(target_thread), nd->payloadHandler(NIC::LogP));
        local_logp_switch->connectOutput(conn.nid, out_link);
      } else {
        EventLink* out_link = new IpcLink(local_logp_switch->sendLatency(empty), target_rank, mgr,
                                          local_logp_switch->componentId(), nodeComponentId(conn.nid), NIC::LogP, false);
        local_logp_switch->connectOutput(conn.nid, out_link);
      }
    }
  }
}

void
Interconnect::buildEndpoints(sprockit::sim_parameters::ptr& node_params,
                  sprockit::sim_parameters::ptr& nic_params,
                  EventManager* mgr)
{
  int my_rank = rt_->me();
  int my_thread = mgr->thread();

  for (int i=0; i < num_switches_; ++i){
    SwitchId sid(i);
    std::vector<Topology::injection_port> nodes;
    topology_->endpointsConnectedToInjectionSwitch(sid, nodes);
    if (nodes.empty())
      continue;
    int target_thread = partition_->threadForSwitch(i);
    int target_rank = partition_->lpidForSwitch(sid);
    interconn_debug("switch %d maps to target rank %d", i, target_rank);

    for (int n=0; n < nodes.size(); ++n){
      NodeId nid = nodes[n].nid;
      int ep_port = nodes[n].ep_port;
      int sw_port = nodes[n].switch_port;
      interconn_debug("building node %d on leaf switch %d", nid, i);

      if (my_rank == target_rank || my_thread == target_thread){
        //local node - actually build it
        node_params->add_param_override("id", int(nid));
        uint32_t comp_id = nid;
        Node* nd = Node::factory::get_optional_param("name", "simple", node_params, comp_id);
        node_params->remove_param("id"); //you don't have to let it linger
        nodes_[nid] = nd;
        components_[nid] = nd;
      }
    }
  }
}

void
Interconnect::buildSwitches(sprockit::sim_parameters::ptr& switch_params,
                            EventManager* mgr)
{
  bool simple_model = switch_params->get_param("name") == "simple";
  if (simple_model) return; //nothing to do

  int my_rank = rt_->me();
  bool all_switches_same = topology_->uniformSwitches();
  int id_offset = topology_->numNodes();
  for (SwitchId i=0; i < num_switches_; ++i){
    switch_params->add_param_override("id", int(i));
    if (partition_->lpidForSwitch(i) == my_rank){
      if (!all_switches_same)
        topology_->configureNonuniformSwitchParams(i, switch_params);
      int thread = partition_->threadForSwitch(i);
      uint32_t comp_id = switchComponentId(i);
      switches_[i] = NetworkSwitch::factory::get_param("name", switch_params, comp_id);
    } else {
      switches_[i] = nullptr;
    }
    switch_params->remove_param("id");
    components_[i+id_offset] = switches_[i];
  }
}

uint32_t
Interconnect::switchComponentId(SwitchId sid) const
{
  return topology_->numNodes() + sid;
}

void
Interconnect::connectSwitches(EventManager* mgr, sprockit::sim_parameters::ptr& switch_params)
{
  bool simple_model = switch_params->get_param("name") == "simple";
  if (simple_model) return; //nothing to do

  std::vector<Topology::connection> outports(64); //allocate 64 spaces optimistically

  int my_rank = rt_->me();
  int my_thread = mgr->thread();

  //might be super uniform in which all ports are the same
  //or it might be mostly uniform in which all the switches are the same
  //even if the individual ports on each switch are different
  bool all_ports_same = topology_->uniformSwitchPorts();
  bool all_switches_same = topology_->uniformSwitches();

  sprockit::sim_parameters::ptr port_params;
  if (all_ports_same){
    port_params = switch_params->get_namespace("link");
  } else if (all_switches_same && !all_ports_same){
    topology_->configureIndividualPortParams(SwitchId(0), switch_params);
  }

  for (int i=0; i < num_switches_; ++i){
    interconn_debug("interconnect: connecting switch %i", i);
    SwitchId src(i);
    int src_rank = partition_->lpidForSwitch(i);
    int src_thread = partition_->threadForSwitch(i);
    topology_->connectedOutports(src, outports);

    if (!all_switches_same && !all_ports_same){
      topology_->configureIndividualPortParams(src, switch_params);
    }
    for (Topology::connection& conn : outports){
      int dst_rank = partition_->lpidForSwitch(conn.dst);
      int dst_thread = partition_->threadForSwitch(conn.dst);

      if (!all_ports_same){
        port_params = switch_params->get_namespace(Topology::getPortNamespace(conn.src_outport));
      }

      interconn_debug("%s connecting to %s on ports %d:%d",
                topology_->switchLabel(src).c_str(),
                topology_->switchLabel(conn.dst).c_str(),
                conn.src_outport, conn.dst_inport);

      if (src_rank == my_rank && src_thread == my_thread){
        EventLink* payload_link = nullptr;
        Timestamp sendLatency = switches_[src]->sendLatency(port_params);
        if (dst_rank == my_rank && dst_thread == my_thread){
          payload_link = new LocalLink(sendLatency, mgr, switches_[conn.dst]->payloadHandler(conn.dst_inport));
        } else if (dst_rank == my_rank){
          payload_link = new MultithreadLink(sendLatency, mgr, EventManager::global->threadManager(dst_thread),
               switches_[conn.dst]->payloadHandler(conn.dst_inport));
        } else {
          payload_link = new IpcLink(sendLatency, dst_rank, mgr,
                                     switchComponentId(src), switchComponentId(conn.dst),
                                     conn.dst_inport, false);
        }
        switches_[src]->connectOutput(port_params, conn.src_outport, conn.dst_inport,
                                      payload_link);

      }

      if (dst_rank == my_rank && dst_thread == my_thread){
        EventLink* credit_link = nullptr;
        Timestamp creditLatency = switches_[conn.dst]->creditLatency(port_params);
        if (src_rank == my_rank && src_thread == my_thread){
          credit_link = new LocalLink(creditLatency, mgr, switches_[src]->creditHandler(conn.src_outport));
        } else if (src_rank == my_rank) {
          credit_link = new MultithreadLink(creditLatency, mgr, EventManager::global->threadManager(src_thread),
               switches_[src]->creditHandler(conn.src_outport));
        } else {
          credit_link = new IpcLink(creditLatency, src_rank, mgr,
                                    switchComponentId(conn.dst), switchComponentId(src),
                                    conn.src_outport, true);
        }
        switches_[conn.dst]->connectInput(port_params,
                                 conn.src_outport, conn.dst_inport, credit_link);
      }
    }
  }
}
#endif

}
}
