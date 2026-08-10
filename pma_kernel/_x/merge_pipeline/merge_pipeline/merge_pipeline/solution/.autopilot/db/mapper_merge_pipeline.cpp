#include <algorithm>
#include <cassert>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <vector>
#include "ap_fixed.h"
#include "ap_int.h"
#include "hls_stream.h"
using namespace std;

namespace hls::sim
{
  template<size_t n>
  struct Byte {
    unsigned char a[n];

    Byte()
    {
      for (size_t i = 0; i < n; ++i) {
        a[i] = 0;
      }
    }

    template<typename T>
    Byte<n>& operator= (const T &val)
    {
      std::memcpy(a, &val, n);
      return *this;
    }
  };

  struct SimException : public std::exception {
    const char *msg;
    const size_t line;
    SimException(const char *msg, const size_t line)
      : msg(msg), line(line)
    {
    }
  };

  void errExit(const size_t line, const char *msg)
  {
    std::string s;
    s += "at line ";
    s += std::to_string(line);
    s += " occurred problem: ";
    s += msg;
    s += "\n";
    fputs(s.c_str(), stderr);
    exit(1);
  }
}


namespace hls::sim
{
  struct Buffer {
    char *first;
    Buffer(char *addr) : first(addr)
    {
    }
  };

  struct DBuffer : public Buffer {
    static const size_t total = 1<<10;
    size_t ufree;

    DBuffer(size_t usize) : Buffer(nullptr), ufree(total)
    {
      first = new char[usize*ufree];
    }

    ~DBuffer()
    {
      delete[] first;
    }
  };

  struct CStream {
    char *front;
    char *back;
    size_t num;
    size_t usize;
    std::list<Buffer*> bufs;
    bool dynamic;

    CStream() : front(nullptr), back(nullptr),
                num(0), usize(0), dynamic(true)
    {
    }

    ~CStream()
    {
      for (Buffer *p : bufs) {
        delete p;
      }
    }

    template<typename T>
    T* data()
    {
      return (T*)front;
    }

    template<typename T>
    void transfer(hls::stream<T> *param)
    {
      while (!empty()) {
        param->write(*(T*)nextRead());
      }
    }

    bool empty();
    char* nextRead();
    char* nextWrite();
  };

  bool CStream::empty()
  {
    return num == 0;
  }

  char* CStream::nextRead()
  {
    assert(num > 0);
    char *res = front;
    front += usize;
    if (dynamic) {
      if (++static_cast<DBuffer*>(bufs.front())->ufree == DBuffer::total) {
        if (bufs.size() > 1) {
          bufs.pop_front();
          front = bufs.front()->first;
        } else {
          front = back = bufs.front()->first;
        }
      }
    }
    --num;
    return res;
  }

  char* CStream::nextWrite()
  {
    if (dynamic) {
      if (static_cast<DBuffer*>(bufs.back())->ufree == 0) {
        bufs.push_back(new DBuffer(usize));
        back = bufs.back()->first;
      }
      --static_cast<DBuffer*>(bufs.back())->ufree;
    }
    char *res = back;
    back += usize;
    ++num;
    return res;
  }

  std::list<CStream> streams;
  std::map<char*, CStream*> prebuilt;

  CStream* createStream(size_t usize)
  {
    streams.emplace_front();
    CStream &s = streams.front();
    {
      s.dynamic = true;
      s.bufs.push_back(new DBuffer(usize));
      s.front = s.bufs.back()->first;
      s.back = s.front;
      s.num = 0;
      s.usize = usize;
    }
    return &s;
  }

  template<typename T>
  CStream* createStream(hls::stream<T> *param)
  {
    CStream *s = createStream(sizeof(T));
    {
      s->dynamic = true;
      while (!param->empty()) {
        T data = param->read();
        memcpy(s->nextWrite(), (char*)&data, sizeof(T));
      }
      prebuilt[s->front] = s;
    }
    return s;
  }

  template<typename T>
  CStream* createStream(T *param, size_t usize)
  {
    streams.emplace_front();
    CStream &s = streams.front();
    {
      s.dynamic = false;
      s.bufs.push_back(new Buffer((char*)param));
      s.front = s.back = s.bufs.back()->first;
      s.usize = usize;
      s.num = ~0UL;
    }
    prebuilt[s.front] = &s;
    return &s;
  }

  CStream* findStream(char *buf)
  {
    return prebuilt.at(buf);
  }
}
class AESL_RUNTIME_BC {
  public:
    AESL_RUNTIME_BC(const char* name) {
      file_token.open( name);
      if (!file_token.good()) {
        cout << "Failed to open tv file " << name << endl;
        exit (1);
      }
      file_token >> mName;//[[[runtime]]]
    }
    ~AESL_RUNTIME_BC() {
      file_token.close();
    }
    int read_size () {
      int size = 0;
      file_token >> mName;//[[transaction]]
      file_token >> mName;//transaction number
      file_token >> mName;//pop_size
      size = atoi(mName.c_str());
      file_token >> mName;//[[/transaction]]
      return size;
    }
  public:
    fstream file_token;
    string mName;
};
unsigned int ap_apatb_edge_in_cap_bc;
static AESL_RUNTIME_BC __xlx_edge_in_V_size_Reader("../tv/stream_size/stream_size_in_edge_in.dat");
unsigned int ap_apatb_task_out_cap_bc;
static AESL_RUNTIME_BC __xlx_task_out_V_size_Reader("../tv/stream_size/stream_size_out_task_out.dat");
unsigned int ap_apatb_redist_ack_in_cap_bc;
static AESL_RUNTIME_BC __xlx_redist_ack_in_V_size_Reader("../tv/stream_size/stream_size_in_redist_ack_in.dat");
unsigned int ap_apatb_ack_out_cap_bc;
static AESL_RUNTIME_BC __xlx_ack_out_V_size_Reader("../tv/stream_size/stream_size_out_ack_out.dat");
using hls::sim::Byte;
struct __cosim_s64__ { char data[64]; };
struct __cosim_s16__ { char data[16]; };
struct __cosim_s128__ { char data[128]; };
struct __cosim_s4__ { char data[4]; };
extern "C" void merge_pipeline(Byte<64>*, Byte<64>*, int, int, int, int, __cosim_s16__*, __cosim_s128__*, __cosim_s4__*, __cosim_s4__*);
extern "C" void apatb_merge_pipeline_hw(volatile void * __xlx_apatb_param_hbm_pma, volatile void * __xlx_apatb_param_hbm_buffer, int __xlx_apatb_param_pma_raw_data_start, int __xlx_apatb_param_pma_vecinfo_start, volatile void * __xlx_apatb_param_edge_in, volatile void * __xlx_apatb_param_task_out, volatile void * __xlx_apatb_param_redist_ack_in, volatile void * __xlx_apatb_param_ack_out) {
using hls::sim::createStream;
  // Collect __xlx_hbm_pma__tmp_vec
std::vector<Byte<64>> __xlx_hbm_pma__tmp_vec;
for (size_t i = 0; i < 1; ++i){
__xlx_hbm_pma__tmp_vec.push_back(((Byte<64>*)__xlx_apatb_param_hbm_pma)[i]);
}
  int __xlx_size_param_hbm_pma = 1;
  int __xlx_offset_param_hbm_pma = 0;
  int __xlx_offset_byte_param_hbm_pma = 0*64;
  // Collect __xlx_hbm_buffer__tmp_vec
std::vector<Byte<64>> __xlx_hbm_buffer__tmp_vec;
for (size_t i = 0; i < 1; ++i){
__xlx_hbm_buffer__tmp_vec.push_back(((Byte<64>*)__xlx_apatb_param_hbm_buffer)[i]);
}
  int __xlx_size_param_hbm_buffer = 1;
  int __xlx_offset_param_hbm_buffer = 0;
  int __xlx_offset_byte_param_hbm_buffer = 0*64;
auto* sedge_in = createStream((hls::stream<__cosim_s16__>*)__xlx_apatb_param_edge_in);
  //Create input buffer for task_out
  ap_apatb_task_out_cap_bc = __xlx_task_out_V_size_Reader.read_size();
  __cosim_s128__* __xlx_task_out_input_buffer= new __cosim_s128__[ap_apatb_task_out_cap_bc];
auto* stask_out = createStream((hls::stream<__cosim_s128__>*)__xlx_apatb_param_task_out);
auto* sredist_ack_in = createStream((hls::stream<__cosim_s4__>*)__xlx_apatb_param_redist_ack_in);
  //Create input buffer for ack_out
  ap_apatb_ack_out_cap_bc = __xlx_ack_out_V_size_Reader.read_size();
  __cosim_s4__* __xlx_ack_out_input_buffer= new __cosim_s4__[ap_apatb_ack_out_cap_bc];
auto* sack_out = createStream((hls::stream<__cosim_s4__>*)__xlx_apatb_param_ack_out);
  // DUT call
  merge_pipeline(__xlx_hbm_pma__tmp_vec.data(), __xlx_hbm_buffer__tmp_vec.data(), __xlx_offset_byte_param_hbm_pma, __xlx_offset_byte_param_hbm_buffer, __xlx_apatb_param_pma_raw_data_start, __xlx_apatb_param_pma_vecinfo_start, sedge_in->data<__cosim_s16__>(), stask_out->data<__cosim_s128__>(), sredist_ack_in->data<__cosim_s4__>(), sack_out->data<__cosim_s4__>());
// print __xlx_apatb_param_hbm_pma
for (size_t i = 0; i < __xlx_size_param_hbm_pma; ++i) {
((Byte<64>*)__xlx_apatb_param_hbm_pma)[i] = __xlx_hbm_pma__tmp_vec[__xlx_offset_param_hbm_pma+i];
}
// print __xlx_apatb_param_hbm_buffer
for (size_t i = 0; i < __xlx_size_param_hbm_buffer; ++i) {
((Byte<64>*)__xlx_apatb_param_hbm_buffer)[i] = __xlx_hbm_buffer__tmp_vec[__xlx_offset_param_hbm_buffer+i];
}
sedge_in->transfer((hls::stream<__cosim_s16__>*)__xlx_apatb_param_edge_in);
stask_out->transfer((hls::stream<__cosim_s128__>*)__xlx_apatb_param_task_out);
sredist_ack_in->transfer((hls::stream<__cosim_s4__>*)__xlx_apatb_param_redist_ack_in);
sack_out->transfer((hls::stream<__cosim_s4__>*)__xlx_apatb_param_ack_out);
}
