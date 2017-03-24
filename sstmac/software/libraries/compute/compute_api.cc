#include <sstmac/software/process/operating_system.h>
#include <sstmac/software/process/thread.h>
#include <sstmac/software/process/app.h>
#include <sstmac/software/libraries/compute/compute_api.h>

extern "C" int
sstmac_sleep(unsigned int secs){
  sstmac::sw::operating_system::current_thread()->parent_app()->sleep(sstmac::timestamp(secs));
  return 0;
}

extern "C" int
sstmac_usleep(unsigned int usecs){
  sstmac::sw::operating_system::current_thread()->parent_app()->sleep(sstmac::timestamp(usecs*1e-6));
  return 0;
}

extern "C" void
sstmac_compute(double secs){
  sstmac::sw::operating_system::current_thread()->parent_app()->compute(sstmac::timestamp(secs));
}

extern "C" void
sstmac_memread(uint64_t bytes){
  sstmac::sw::operating_system::current_thread()->parent_app()
    ->compute_block_read(bytes);
}

extern "C" void
sstmac_memwrite(uint64_t bytes){
  sstmac::sw::operating_system::current_thread()->parent_app()
    ->compute_block_write(bytes);
}

extern "C" void
sstmac_memcpy(uint64_t bytes){
  sstmac::sw::operating_system::current_thread()->parent_app()
    ->compute_block_memcpy(bytes);
}

extern "C" void
sstmac_compute_detailed(uint64_t nflops, uint64_t nintops, uint64_t bytes){
  sstmac::sw::operating_system::current_thread()->parent_app()
    ->compute_detailed(nflops, nintops, bytes);
}

extern "C" void
sstmac_compute_loop(uint64_t num_loops, uint32_t nflops_per_loop,
                    uint32_t nintops_per_loop, uint32_t bytes_per_loop){
  sstmac::sw::operating_system::current_thread()->parent_app()
    ->compute_loop(num_loops, nflops_per_loop, nintops_per_loop, bytes_per_loop);
}

extern "C" void
sstmac_compute_loop2(uint64_t isize, uint64_t jsize,
                    uint32_t nflops_per_loop,
                    uint32_t nintops_per_loop, uint32_t bytes_per_loop){
  uint64_t num_loops = isize * jsize;
  sstmac::sw::operating_system::current_thread()->parent_app()
    ->compute_loop(num_loops, nflops_per_loop, nintops_per_loop, bytes_per_loop);
}

extern "C" void
sstmac_compute_loop3(uint64_t isize, uint64_t jsize, uint64_t ksize,
                    uint32_t nflops_per_loop,
                    uint32_t nintops_per_loop,
                    uint32_t bytes_per_loop){
  uint64_t num_loops = isize * jsize * ksize;
  sstmac::sw::operating_system::current_thread()->parent_app()
    ->compute_loop(num_loops, nflops_per_loop, nintops_per_loop, bytes_per_loop);
}

extern "C" void
sstmac_compute_loop4(uint64_t isize, uint64_t jsize, uint64_t ksize, uint64_t lsize,
                     uint32_t nflops_per_loop,
                     uint32_t nintops_per_loop,
                     uint32_t bytes_per_loop){
  uint64_t num_loops = isize * jsize * ksize * lsize;
  sstmac::sw::operating_system::current_thread()->parent_app()
    ->compute_loop(num_loops, nflops_per_loop, nintops_per_loop, bytes_per_loop);
}
