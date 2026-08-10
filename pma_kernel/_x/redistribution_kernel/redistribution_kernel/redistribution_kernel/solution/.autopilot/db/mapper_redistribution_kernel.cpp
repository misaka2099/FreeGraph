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
unsigned int ap_apatb_pre_expand_in_cap_bc;
static AESL_RUNTIME_BC __xlx_pre_expand_in_V_size_Reader("../tv/stream_size/stream_size_in_pre_expand_in.dat");
unsigned int ap_apatb_task_in_cap_bc;
static AESL_RUNTIME_BC __xlx_task_in_V_size_Reader("../tv/stream_size/stream_size_in_task_in.dat");
unsigned int ap_apatb_pre_ack_out_cap_bc;
static AESL_RUNTIME_BC __xlx_pre_ack_out_V_size_Reader("../tv/stream_size/stream_size_out_pre_ack_out.dat");
unsigned int ap_apatb_redist_ack_out_cap_bc;
static AESL_RUNTIME_BC __xlx_redist_ack_out_V_size_Reader("../tv/stream_size/stream_size_out_redist_ack_out.dat");
unsigned int ap_apatb_alloc_cmd_stream_V_data_V_cap_bc;
static AESL_RUNTIME_BC __xlx_alloc_cmd_stream_V_data_V_V_size_Reader("../tv/stream_size/stream_size_out_alloc_cmd_stream_V_data_V.dat");
unsigned int ap_apatb_alloc_cmd_stream_V_keep_V_cap_bc;
static AESL_RUNTIME_BC __xlx_alloc_cmd_stream_V_keep_V_V_size_Reader("../tv/stream_size/stream_size_out_alloc_cmd_stream_V_keep_V.dat");
unsigned int ap_apatb_alloc_cmd_stream_V_strb_V_cap_bc;
static AESL_RUNTIME_BC __xlx_alloc_cmd_stream_V_strb_V_V_size_Reader("../tv/stream_size/stream_size_out_alloc_cmd_stream_V_strb_V.dat");
unsigned int ap_apatb__cap_bc;
static AESL_RUNTIME_BC __xlx__V_size_Reader("../tv/stream_size/stream_size_out_.dat");
unsigned int ap_apatb_alloc_cmd_stream_V_last_V_cap_bc;
static AESL_RUNTIME_BC __xlx_alloc_cmd_stream_V_last_V_V_size_Reader("../tv/stream_size/stream_size_out_alloc_cmd_stream_V_last_V.dat");
unsigned int ap_apatb__cap_bc;
static AESL_RUNTIME_BC __xlx__V_size_Reader("../tv/stream_size/stream_size_out_.dat");
unsigned int ap_apatb__cap_bc;
static AESL_RUNTIME_BC __xlx__V_size_Reader("../tv/stream_size/stream_size_out_.dat");
unsigned int ap_apatb_alloc_rsp_stream_V_data_V_cap_bc;
static AESL_RUNTIME_BC __xlx_alloc_rsp_stream_V_data_V_V_size_Reader("../tv/stream_size/stream_size_in_alloc_rsp_stream_V_data_V.dat");
unsigned int ap_apatb_alloc_rsp_stream_V_keep_V_cap_bc;
static AESL_RUNTIME_BC __xlx_alloc_rsp_stream_V_keep_V_V_size_Reader("../tv/stream_size/stream_size_in_alloc_rsp_stream_V_keep_V.dat");
unsigned int ap_apatb_alloc_rsp_stream_V_strb_V_cap_bc;
static AESL_RUNTIME_BC __xlx_alloc_rsp_stream_V_strb_V_V_size_Reader("../tv/stream_size/stream_size_in_alloc_rsp_stream_V_strb_V.dat");
unsigned int ap_apatb__cap_bc;
static AESL_RUNTIME_BC __xlx__V_size_Reader("../tv/stream_size/stream_size_in_.dat");
unsigned int ap_apatb_alloc_rsp_stream_V_last_V_cap_bc;
static AESL_RUNTIME_BC __xlx_alloc_rsp_stream_V_last_V_V_size_Reader("../tv/stream_size/stream_size_in_alloc_rsp_stream_V_last_V.dat");
unsigned int ap_apatb__cap_bc;
static AESL_RUNTIME_BC __xlx__V_size_Reader("../tv/stream_size/stream_size_in_.dat");
unsigned int ap_apatb__cap_bc;
static AESL_RUNTIME_BC __xlx__V_size_Reader("../tv/stream_size/stream_size_in_.dat");
unsigned int ap_apatb_bitmap_init_V_data_V_cap_bc;
static AESL_RUNTIME_BC __xlx_bitmap_init_V_data_V_V_size_Reader("../tv/stream_size/stream_size_out_bitmap_init_V_data_V.dat");
unsigned int ap_apatb_bitmap_init_V_keep_V_cap_bc;
static AESL_RUNTIME_BC __xlx_bitmap_init_V_keep_V_V_size_Reader("../tv/stream_size/stream_size_out_bitmap_init_V_keep_V.dat");
unsigned int ap_apatb_bitmap_init_V_strb_V_cap_bc;
static AESL_RUNTIME_BC __xlx_bitmap_init_V_strb_V_V_size_Reader("../tv/stream_size/stream_size_out_bitmap_init_V_strb_V.dat");
unsigned int ap_apatb__cap_bc;
static AESL_RUNTIME_BC __xlx__V_size_Reader("../tv/stream_size/stream_size_out_.dat");
unsigned int ap_apatb_bitmap_init_V_last_V_cap_bc;
static AESL_RUNTIME_BC __xlx_bitmap_init_V_last_V_V_size_Reader("../tv/stream_size/stream_size_out_bitmap_init_V_last_V.dat");
unsigned int ap_apatb__cap_bc;
static AESL_RUNTIME_BC __xlx__V_size_Reader("../tv/stream_size/stream_size_out_.dat");
unsigned int ap_apatb__cap_bc;
static AESL_RUNTIME_BC __xlx__V_size_Reader("../tv/stream_size/stream_size_out_.dat");
using hls::sim::Byte;
struct __cosim_s64__ { char data[64]; };
struct __cosim_s68__ { char data[68]; };
struct __cosim_s128__ { char data[128]; };
struct __cosim_s4__ { char data[4]; };
struct __cosim_s12__ { char data[16]; };
struct __cosim_s16__ { char data[16]; };
extern "C" void redistribution_kernel(Byte<64>*, Byte<64>*, int, int, int, int, int, __cosim_s68__*, __cosim_s128__*, __cosim_s4__*, __cosim_s4__*, volatile void *, volatile void *, volatile void *, , volatile void *, , , volatile void *, volatile void *, volatile void *, , volatile void *, , , volatile void *, volatile void *, volatile void *, , volatile void *, , );
extern "C" void apatb_redistribution_kernel_hw(volatile void * __xlx_apatb_param_hbm_red, volatile void * __xlx_apatb_param_hbm_buffer_for_red, int __xlx_apatb_param_pma_raw_data_start, int __xlx_apatb_param_pma_vecinfo_start, int __xlx_apatb_param_bitmap_offset, volatile void * __xlx_apatb_param_pre_expand_in, volatile void * __xlx_apatb_param_task_in, volatile void * __xlx_apatb_param_pre_ack_out, volatile void * __xlx_apatb_param_redist_ack_out, volatile void * __xlx_apatb_param_alloc_cmd_stream_V_data_V, volatile void * __xlx_apatb_param_alloc_cmd_stream_V_keep_V, volatile void * __xlx_apatb_param_alloc_cmd_stream_V_strb_V, volatile void * __xlx_apatb_param_alloc_cmd_stream_V_last_V, volatile void * __xlx_apatb_param_alloc_rsp_stream_V_data_V, volatile void * __xlx_apatb_param_alloc_rsp_stream_V_keep_V, volatile void * __xlx_apatb_param_alloc_rsp_stream_V_strb_V, volatile void * __xlx_apatb_param_alloc_rsp_stream_V_last_V, volatile void * __xlx_apatb_param_bitmap_init_V_data_V, volatile void * __xlx_apatb_param_bitmap_init_V_keep_V, volatile void * __xlx_apatb_param_bitmap_init_V_strb_V, volatile void * __xlx_apatb_param_bitmap_init_V_last_V) {
using hls::sim::createStream;
  // Collect __xlx_hbm_red__tmp_vec
std::vector<Byte<64>> __xlx_hbm_red__tmp_vec;
for (size_t i = 0; i < 1; ++i){
__xlx_hbm_red__tmp_vec.push_back(((Byte<64>*)__xlx_apatb_param_hbm_red)[i]);
}
  int __xlx_size_param_hbm_red = 1;
  int __xlx_offset_param_hbm_red = 0;
  int __xlx_offset_byte_param_hbm_red = 0*64;
  // Collect __xlx_hbm_buffer_for_red__tmp_vec
std::vector<Byte<64>> __xlx_hbm_buffer_for_red__tmp_vec;
for (size_t i = 0; i < 1; ++i){
__xlx_hbm_buffer_for_red__tmp_vec.push_back(((Byte<64>*)__xlx_apatb_param_hbm_buffer_for_red)[i]);
}
  int __xlx_size_param_hbm_buffer_for_red = 1;
  int __xlx_offset_param_hbm_buffer_for_red = 0;
  int __xlx_offset_byte_param_hbm_buffer_for_red = 0*64;
auto* spre_expand_in = createStream((hls::stream<__cosim_s68__>*)__xlx_apatb_param_pre_expand_in);
auto* stask_in = createStream((hls::stream<__cosim_s128__>*)__xlx_apatb_param_task_in);
  //Create input buffer for pre_ack_out
  ap_apatb_pre_ack_out_cap_bc = __xlx_pre_ack_out_V_size_Reader.read_size();
  __cosim_s4__* __xlx_pre_ack_out_input_buffer= new __cosim_s4__[ap_apatb_pre_ack_out_cap_bc];
auto* spre_ack_out = createStream((hls::stream<__cosim_s4__>*)__xlx_apatb_param_pre_ack_out);
  //Create input buffer for redist_ack_out
  ap_apatb_redist_ack_out_cap_bc = __xlx_redist_ack_out_V_size_Reader.read_size();
  __cosim_s4__* __xlx_redist_ack_out_input_buffer= new __cosim_s4__[ap_apatb_redist_ack_out_cap_bc];
auto* sredist_ack_out = createStream((hls::stream<__cosim_s4__>*)__xlx_apatb_param_redist_ack_out);
  //Create input buffer for alloc_cmd_stream_V_data_V
  ap_apatb_alloc_cmd_stream_V_data_V_cap_bc = __xlx_alloc_cmd_stream_V_data_V_V_size_Reader.read_size();
  __cosim_s12__* __xlx_alloc_cmd_stream_V_data_V_input_buffer= new __cosim_s12__[ap_apatb_alloc_cmd_stream_V_data_V_cap_bc];
auto* salloc_cmd_stream_V_data_V = createStream((hls::stream<__cosim_s12__>*)__xlx_apatb_param_alloc_cmd_stream_V_data_V);
  //Create input buffer for alloc_cmd_stream_V_keep_V
  ap_apatb_alloc_cmd_stream_V_keep_V_cap_bc = __xlx_alloc_cmd_stream_V_keep_V_V_size_Reader.read_size();
  short* __xlx_alloc_cmd_stream_V_keep_V_input_buffer= new short[ap_apatb_alloc_cmd_stream_V_keep_V_cap_bc];
auto* salloc_cmd_stream_V_keep_V = createStream((hls::stream<short>*)__xlx_apatb_param_alloc_cmd_stream_V_keep_V);
  //Create input buffer for alloc_cmd_stream_V_strb_V
  ap_apatb_alloc_cmd_stream_V_strb_V_cap_bc = __xlx_alloc_cmd_stream_V_strb_V_V_size_Reader.read_size();
  short* __xlx_alloc_cmd_stream_V_strb_V_input_buffer= new short[ap_apatb_alloc_cmd_stream_V_strb_V_cap_bc];
auto* salloc_cmd_stream_V_strb_V = createStream((hls::stream<short>*)__xlx_apatb_param_alloc_cmd_stream_V_strb_V);
  //Create input buffer for alloc_cmd_stream_V_last_V
  ap_apatb_alloc_cmd_stream_V_last_V_cap_bc = __xlx_alloc_cmd_stream_V_last_V_V_size_Reader.read_size();
  char* __xlx_alloc_cmd_stream_V_last_V_input_buffer= new char[ap_apatb_alloc_cmd_stream_V_last_V_cap_bc];
auto* salloc_cmd_stream_V_last_V = createStream((hls::stream<char>*)__xlx_apatb_param_alloc_cmd_stream_V_last_V);
auto* salloc_rsp_stream_V_data_V = createStream((hls::stream<long long>*)__xlx_apatb_param_alloc_rsp_stream_V_data_V);
auto* salloc_rsp_stream_V_keep_V = createStream((hls::stream<char>*)__xlx_apatb_param_alloc_rsp_stream_V_keep_V);
auto* salloc_rsp_stream_V_strb_V = createStream((hls::stream<char>*)__xlx_apatb_param_alloc_rsp_stream_V_strb_V);
auto* salloc_rsp_stream_V_last_V = createStream((hls::stream<char>*)__xlx_apatb_param_alloc_rsp_stream_V_last_V);
  //Create input buffer for bitmap_init_V_data_V
  ap_apatb_bitmap_init_V_data_V_cap_bc = __xlx_bitmap_init_V_data_V_V_size_Reader.read_size();
  __cosim_s64__* __xlx_bitmap_init_V_data_V_input_buffer= new __cosim_s64__[ap_apatb_bitmap_init_V_data_V_cap_bc];
auto* sbitmap_init_V_data_V = createStream((hls::stream<__cosim_s64__>*)__xlx_apatb_param_bitmap_init_V_data_V);
  //Create input buffer for bitmap_init_V_keep_V
  ap_apatb_bitmap_init_V_keep_V_cap_bc = __xlx_bitmap_init_V_keep_V_V_size_Reader.read_size();
  long long* __xlx_bitmap_init_V_keep_V_input_buffer= new long long[ap_apatb_bitmap_init_V_keep_V_cap_bc];
auto* sbitmap_init_V_keep_V = createStream((hls::stream<long long>*)__xlx_apatb_param_bitmap_init_V_keep_V);
  //Create input buffer for bitmap_init_V_strb_V
  ap_apatb_bitmap_init_V_strb_V_cap_bc = __xlx_bitmap_init_V_strb_V_V_size_Reader.read_size();
  long long* __xlx_bitmap_init_V_strb_V_input_buffer= new long long[ap_apatb_bitmap_init_V_strb_V_cap_bc];
auto* sbitmap_init_V_strb_V = createStream((hls::stream<long long>*)__xlx_apatb_param_bitmap_init_V_strb_V);
  //Create input buffer for bitmap_init_V_last_V
  ap_apatb_bitmap_init_V_last_V_cap_bc = __xlx_bitmap_init_V_last_V_V_size_Reader.read_size();
  char* __xlx_bitmap_init_V_last_V_input_buffer= new char[ap_apatb_bitmap_init_V_last_V_cap_bc];
auto* sbitmap_init_V_last_V = createStream((hls::stream<char>*)__xlx_apatb_param_bitmap_init_V_last_V);
  // DUT call
  redistribution_kernel(__xlx_hbm_red__tmp_vec.data(), __xlx_hbm_buffer_for_red__tmp_vec.data(), __xlx_offset_byte_param_hbm_red, __xlx_offset_byte_param_hbm_buffer_for_red, __xlx_apatb_param_pma_raw_data_start, __xlx_apatb_param_pma_vecinfo_start, __xlx_apatb_param_bitmap_offset, spre_expand_in->data<__cosim_s68__>(), stask_in->data<__cosim_s128__>(), spre_ack_out->data<__cosim_s4__>(), sredist_ack_out->data<__cosim_s4__>(), salloc_cmd_stream_V_data_V->data<__cosim_s12__>(), salloc_cmd_stream_V_keep_V->data<short>(), salloc_cmd_stream_V_strb_V->data<short>(), salloc_cmd_stream_V_last_V->data<char>(), salloc_rsp_stream_V_data_V->data<long long>(), salloc_rsp_stream_V_keep_V->data<char>(), salloc_rsp_stream_V_strb_V->data<char>(), salloc_rsp_stream_V_last_V->data<char>(), sbitmap_init_V_data_V->data<__cosim_s64__>(), sbitmap_init_V_keep_V->data<long long>(), sbitmap_init_V_strb_V->data<long long>(), sbitmap_init_V_last_V->data<char>());
// print __xlx_apatb_param_hbm_red
for (size_t i = 0; i < __xlx_size_param_hbm_red; ++i) {
((Byte<64>*)__xlx_apatb_param_hbm_red)[i] = __xlx_hbm_red__tmp_vec[__xlx_offset_param_hbm_red+i];
}
// print __xlx_apatb_param_hbm_buffer_for_red
for (size_t i = 0; i < __xlx_size_param_hbm_buffer_for_red; ++i) {
((Byte<64>*)__xlx_apatb_param_hbm_buffer_for_red)[i] = __xlx_hbm_buffer_for_red__tmp_vec[__xlx_offset_param_hbm_buffer_for_red+i];
}
spre_expand_in->transfer((hls::stream<__cosim_s68__>*)__xlx_apatb_param_pre_expand_in);
stask_in->transfer((hls::stream<__cosim_s128__>*)__xlx_apatb_param_task_in);
spre_ack_out->transfer((hls::stream<__cosim_s4__>*)__xlx_apatb_param_pre_ack_out);
sredist_ack_out->transfer((hls::stream<__cosim_s4__>*)__xlx_apatb_param_redist_ack_out);
salloc_cmd_stream_V_data_V->transfer((hls::stream<__cosim_s12__>*)__xlx_apatb_param_alloc_cmd_stream_V_data_V);
salloc_cmd_stream_V_keep_V->transfer((hls::stream<short>*)__xlx_apatb_param_alloc_cmd_stream_V_keep_V);
salloc_cmd_stream_V_strb_V->transfer((hls::stream<short>*)__xlx_apatb_param_alloc_cmd_stream_V_strb_V);
s->transfer((hls::stream<char>*)__xlx_apatb_param_);
salloc_cmd_stream_V_last_V->transfer((hls::stream<char>*)__xlx_apatb_param_alloc_cmd_stream_V_last_V);
s->transfer((hls::stream<char>*)__xlx_apatb_param_);
s->transfer((hls::stream<char>*)__xlx_apatb_param_);
salloc_rsp_stream_V_data_V->transfer((hls::stream<long long>*)__xlx_apatb_param_alloc_rsp_stream_V_data_V);
salloc_rsp_stream_V_keep_V->transfer((hls::stream<char>*)__xlx_apatb_param_alloc_rsp_stream_V_keep_V);
salloc_rsp_stream_V_strb_V->transfer((hls::stream<char>*)__xlx_apatb_param_alloc_rsp_stream_V_strb_V);
s->transfer((hls::stream<char>*)__xlx_apatb_param_);
salloc_rsp_stream_V_last_V->transfer((hls::stream<char>*)__xlx_apatb_param_alloc_rsp_stream_V_last_V);
s->transfer((hls::stream<char>*)__xlx_apatb_param_);
s->transfer((hls::stream<char>*)__xlx_apatb_param_);
sbitmap_init_V_data_V->transfer((hls::stream<__cosim_s64__>*)__xlx_apatb_param_bitmap_init_V_data_V);
sbitmap_init_V_keep_V->transfer((hls::stream<long long>*)__xlx_apatb_param_bitmap_init_V_keep_V);
sbitmap_init_V_strb_V->transfer((hls::stream<long long>*)__xlx_apatb_param_bitmap_init_V_strb_V);
s->transfer((hls::stream<char>*)__xlx_apatb_param_);
sbitmap_init_V_last_V->transfer((hls::stream<char>*)__xlx_apatb_param_bitmap_init_V_last_V);
s->transfer((hls::stream<char>*)__xlx_apatb_param_);
s->transfer((hls::stream<char>*)__xlx_apatb_param_);
}
