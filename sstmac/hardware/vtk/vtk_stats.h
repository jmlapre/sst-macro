#ifndef sstmac_hw_vtk_stats_included_h
#define sstmac_hw_vtk_stats_included_h

#include <sstmac/common/stats/stat_collector.h>
#include <vector>
#include <memory>

#if SSTMAC_INTEGRATED_SST_CORE
#include <sst/core/statapi/statfieldinfo.h>
//#include <sst/core/sst_types.h>
using namespace SST;
#endif
namespace sstmac {
namespace hw {

struct traffic_event {
    uint64_t time_; // progress time
    int id_; // Id of the switch
    int type_; // arrive or leave
    int p_; // port of the node Id
    int intensity_; // traffic intenisity
};

class stat_vtk : public stat_collector
{
  FactoryRegister("vtk", stat_collector, stat_vtk)
 public:
  stat_vtk(sprockit::sim_parameters* params);

  std::string to_string() const override {
    return "VTK stats";
  }
  static void outputExodus(const std::string& fileroot,
      const std::multimap<uint64_t, traffic_event> &traffMap,
      int count_x, int count_y);

  void dump_local_data() override;

  void dump_global_data() override;

  void global_reduce(parallel_runtime *rt) override;

  void clear() override;

  void collect_departure(uint64_t time, int switch_id, int port);

  void collect_arrival(uint64_t time, int switch_id, int port);

  void reduce(stat_collector *coll) override;

  stat_collector* do_clone(sprockit::sim_parameters* params) const override {
    return new stat_vtk(params);
  }

private:
  std::multimap<uint64_t, std::shared_ptr<traffic_event>> traffic_progress_map_;
  std::multimap<uint64_t, std::shared_ptr<traffic_event>> traffic_event_map_;
  int count_x_;
  int count_y_;
};

}
}

#endif
