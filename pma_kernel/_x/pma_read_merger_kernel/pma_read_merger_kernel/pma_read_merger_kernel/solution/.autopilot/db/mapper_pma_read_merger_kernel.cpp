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
unsigned int ap_apatb_merger_cmd_stream_V_data_V_cap_bc;
static AESL_RUNTIME_BC __xlx_merger_cmd_stream_V_data_V_V_size_Reader("../tv/stream_size/stream_size_in_merger_cmd_stream_V_data_V.dat");
unsigned int ap_apatb_merger_cmd_stream_V_keep_V_cap_bc;
static AESL_RUNTIME_BC __xlx_merger_cmd_stream_V_keep_V_V_size_Reader("../tv/stream_size/stream_size_in_merger_cmd_stream_V_keep_V.dat");
unsigned int ap_apatb_merger_cmd_stream_V_strb_V_cap_bc;
static AESL_RUNTIME_BC __xlx_merger_cmd_stream_V_strb_V_V_size_Reader("../tv/stream_size/stream_size_in_merger_cmd_stream_V_strb_V.dat");
unsigned int ap_apatb__cap_bc;
static AESL_RUNTIME_BC __xlx__V_size_Reader("../tv/stream_size/stream_size_in_.dat");
unsigned int ap_apatb_merger_cmd_stream_V_last_V_cap_bc;
static AESL_RUNTIME_BC __xlx_merger_cmd_stream_V_last_V_V_size_Reader("../tv/stream_size/stream_size_in_merger_cmd_stream_V_last_V.dat");
unsigned int ap_apatb__cap_bc;
static AESL_RUNTIME_BC __xlx__V_size_Reader("../tv/stream_size/stream_size_in_.dat");
unsigned int ap_apatb__cap_bc;
static AESL_RUNTIME_BC __xlx__V_size_Reader("../tv/stream_size/stream_size_in_.dat");
unsigned int ap_apatb_worker0_pkt_stream_V_data_V_cap_bc;
static AESL_RUNTIME_BC __xlx_worker0_pkt_stream_V_data_V_V_size_Reader("../tv/stream_size/stream_size_in_worker0_pkt_stream_V_data_V.dat");
unsigned int ap_apatb_worker0_pkt_stream_V_keep_V_cap_bc;
static AESL_RUNTIME_BC __xlx_worker0_pkt_stream_V_keep_V_V_size_Reader("../tv/stream_size/stream_size_in_worker0_pkt_stream_V_keep_V.dat");
unsigned int ap_apatb_worker0_pkt_stream_V_strb_V_cap_bc;
static AESL_RUNTIME_BC __xlx_worker0_pkt_stream_V_strb_V_V_size_Reader("../tv/stream_size/stream_size_in_worker0_pkt_stream_V_strb_V.dat");
unsigned int ap_apatb_worker0_pkt_stream_V_user_V_cap_bc;
static AESL_RUNTIME_BC __xlx_worker0_pkt_stream_V_user_V_V_size_Reader("../tv/stream_size/stream_size_in_worker0_pkt_stream_V_user_V.dat");
unsigned int ap_apatb_worker0_pkt_stream_V_last_V_cap_bc;
static AESL_RUNTIME_BC __xlx_worker0_pkt_stream_V_last_V_V_size_Reader("../tv/stream_size/stream_size_in_worker0_pkt_stream_V_last_V.dat");
unsigned int ap_apatb__cap_bc;
static AESL_RUNTIME_BC __xlx__V_size_Reader("../tv/stream_size/stream_size_in_.dat");
unsigned int ap_apatb__cap_bc;
static AESL_RUNTIME_BC __xlx__V_size_Reader("../tv/stream_size/stream_size_in_.dat");
unsigned int ap_apatb_worker1_pkt_stream_V_data_V_cap_bc;
static AESL_RUNTIME_BC __xlx_worker1_pkt_stream_V_data_V_V_size_Reader("../tv/stream_size/stream_size_in_worker1_pkt_stream_V_data_V.dat");
unsigned int ap_apatb_worker1_pkt_stream_V_keep_V_cap_bc;
static AESL_RUNTIME_BC __xlx_worker1_pkt_stream_V_keep_V_V_size_Reader("../tv/stream_size/stream_size_in_worker1_pkt_stream_V_keep_V.dat");
unsigned int ap_apatb_worker1_pkt_stream_V_strb_V_cap_bc;
static AESL_RUNTIME_BC __xlx_worker1_pkt_stream_V_strb_V_V_size_Reader("../tv/stream_size/stream_size_in_worker1_pkt_stream_V_strb_V.dat");
unsigned int ap_apatb_worker1_pkt_stream_V_user_V_cap_bc;
static AESL_RUNTIME_BC __xlx_worker1_pkt_stream_V_user_V_V_size_Reader("../tv/stream_size/stream_size_in_worker1_pkt_stream_V_user_V.dat");
unsigned int ap_apatb_worker1_pkt_stream_V_last_V_cap_bc;
static AESL_RUNTIME_BC __xlx_worker1_pkt_stream_V_last_V_V_size_Reader("../tv/stream_size/stream_size_in_worker1_pkt_stream_V_last_V.dat");
unsigned int ap_apatb__cap_bc;
static AESL_RUNTIME_BC __xlx__V_size_Reader("../tv/stream_size/stream_size_in_.dat");
unsigned int ap_apatb__cap_bc;
static AESL_RUNTIME_BC __xlx__V_size_Reader("../tv/stream_size/stream_size_in_.dat");
unsigned int ap_apatb_worker2_pkt_stream_V_data_V_cap_bc;
static AESL_RUNTIME_BC __xlx_worker2_pkt_stream_V_data_V_V_size_Reader("../tv/stream_size/stream_size_in_worker2_pkt_stream_V_data_V.dat");
unsigned int ap_apatb_worker2_pkt_stream_V_keep_V_cap_bc;
static AESL_RUNTIME_BC __xlx_worker2_pkt_stream_V_keep_V_V_size_Reader("../tv/stream_size/stream_size_in_worker2_pkt_stream_V_keep_V.dat");
unsigned int ap_apatb_worker2_pkt_stream_V_strb_V_cap_bc;
static AESL_RUNTIME_BC __xlx_worker2_pkt_stream_V_strb_V_V_size_Reader("../tv/stream_size/stream_size_in_worker2_pkt_stream_V_strb_V.dat");
unsigned int ap_apatb_worker2_pkt_stream_V_user_V_cap_bc;
static AESL_RUNTIME_BC __xlx_worker2_pkt_stream_V_user_V_V_size_Reader("../tv/stream_size/stream_size_in_worker2_pkt_stream_V_user_V.dat");
unsigned int ap_apatb_worker2_pkt_stream_V_last_V_cap_bc;
static AESL_RUNTIME_BC __xlx_worker2_pkt_stream_V_last_V_V_size_Reader("../tv/stream_size/stream_size_in_worker2_pkt_stream_V_last_V.dat");
unsigned int ap_apatb__cap_bc;
static AESL_RUNTIME_BC __xlx__V_size_Reader("../tv/stream_size/stream_size_in_.dat");
unsigned int ap_apatb__cap_bc;
static AESL_RUNTIME_BC __xlx__V_size_Reader("../tv/stream_size/stream_size_in_.dat");
unsigned int ap_apatb_data_to_coo_stream_V_data_V_cap_bc;
static AESL_RUNTIME_BC __xlx_data_to_coo_stream_V_data_V_V_size_Reader("../tv/stream_size/stream_size_out_data_to_coo_stream_V_data_V.dat");
unsigned int ap_apatb_data_to_coo_stream_V_keep_V_cap_bc;
static AESL_RUNTIME_BC __xlx_data_to_coo_stream_V_keep_V_V_size_Reader("../tv/stream_size/stream_size_out_data_to_coo_stream_V_keep_V.dat");
unsigned int ap_apatb_data_to_coo_stream_V_strb_V_cap_bc;
static AESL_RUNTIME_BC __xlx_data_to_coo_stream_V_strb_V_V_size_Reader("../tv/stream_size/stream_size_out_data_to_coo_stream_V_strb_V.dat");
unsigned int ap_apatb__cap_bc;
static AESL_RUNTIME_BC __xlx__V_size_Reader("../tv/stream_size/stream_size_out_.dat");
unsigned int ap_apatb_data_to_coo_stream_V_last_V_cap_bc;
static AESL_RUNTIME_BC __xlx_data_to_coo_stream_V_last_V_V_size_Reader("../tv/stream_size/stream_size_out_data_to_coo_stream_V_last_V.dat");
unsigned int ap_apatb__cap_bc;
static AESL_RUNTIME_BC __xlx__V_size_Reader("../tv/stream_size/stream_size_out_.dat");
unsigned int ap_apatb__cap_bc;
static AESL_RUNTIME_BC __xlx__V_size_Reader("../tv/stream_size/stream_size_out_.dat");
using hls::sim::Byte;
struct __cosim_s20__ { char data[32]; };
struct __cosim_s32__ { char data[32]; };
extern "C" void pma_read_merger_kernel(volatile void *, volatile void *, volatile void *, , volatile void *, , , volatile void *, volatile void *, volatile void *, volatile void *, volatile void *, , , volatile void *, volatile void *, volatile void *, volatile void *, volatile void *, , , volatile void *, volatile void *, volatile void *, volatile void *, volatile void *, , , volatile void *, volatile void *, volatile void *, , volatile void *, , );
extern "C" void apatb_pma_read_merger_kernel_hw(volatile void * __xlx_apatb_param_merger_cmd_stream_V_data_V, volatile void * __xlx_apatb_param_merger_cmd_stream_V_keep_V, volatile void * __xlx_apatb_param_merger_cmd_stream_V_strb_V, volatile void * __xlx_apatb_param_merger_cmd_stream_V_last_V, volatile void * __xlx_apatb_param_worker0_pkt_stream_V_data_V, volatile void * __xlx_apatb_param_worker0_pkt_stream_V_keep_V, volatile void * __xlx_apatb_param_worker0_pkt_stream_V_strb_V, volatile void * __xlx_apatb_param_worker0_pkt_stream_V_user_V, volatile void * __xlx_apatb_param_worker0_pkt_stream_V_last_V, volatile void * __xlx_apatb_param_worker1_pkt_stream_V_data_V, volatile void * __xlx_apatb_param_worker1_pkt_stream_V_keep_V, volatile void * __xlx_apatb_param_worker1_pkt_stream_V_strb_V, volatile void * __xlx_apatb_param_worker1_pkt_stream_V_user_V, volatile void * __xlx_apatb_param_worker1_pkt_stream_V_last_V, volatile void * __xlx_apatb_param_worker2_pkt_stream_V_data_V, volatile void * __xlx_apatb_param_worker2_pkt_stream_V_keep_V, volatile void * __xlx_apatb_param_worker2_pkt_stream_V_strb_V, volatile void * __xlx_apatb_param_worker2_pkt_stream_V_user_V, volatile void * __xlx_apatb_param_worker2_pkt_stream_V_last_V, volatile void * __xlx_apatb_param_data_to_coo_stream_V_data_V, volatile void * __xlx_apatb_param_data_to_coo_stream_V_keep_V, volatile void * __xlx_apatb_param_data_to_coo_stream_V_strb_V, volatile void * __xlx_apatb_param_data_to_coo_stream_V_last_V) {
using hls::sim::createStream;
auto* smerger_cmd_stream_V_data_V = createStream((hls::stream<__cosim_s20__>*)__xlx_apatb_param_merger_cmd_stream_V_data_V);
auto* smerger_cmd_stream_V_keep_V = createStream((hls::stream<int>*)__xlx_apatb_param_merger_cmd_stream_V_keep_V);
auto* smerger_cmd_stream_V_strb_V = createStream((hls::stream<int>*)__xlx_apatb_param_merger_cmd_stream_V_strb_V);
auto* smerger_cmd_stream_V_last_V = createStream((hls::stream<char>*)__xlx_apatb_param_merger_cmd_stream_V_last_V);
auto* sworker0_pkt_stream_V_data_V = createStream((hls::stream<long long>*)__xlx_apatb_param_worker0_pkt_stream_V_data_V);
auto* sworker0_pkt_stream_V_keep_V = createStream((hls::stream<char>*)__xlx_apatb_param_worker0_pkt_stream_V_keep_V);
auto* sworker0_pkt_stream_V_strb_V = createStream((hls::stream<char>*)__xlx_apatb_param_worker0_pkt_stream_V_strb_V);
auto* sworker0_pkt_stream_V_user_V = createStream((hls::stream<char>*)__xlx_apatb_param_worker0_pkt_stream_V_user_V);
auto* sworker0_pkt_stream_V_last_V = createStream((hls::stream<char>*)__xlx_apatb_param_worker0_pkt_stream_V_last_V);
auto* sworker1_pkt_stream_V_data_V = createStream((hls::stream<long long>*)__xlx_apatb_param_worker1_pkt_stream_V_data_V);
auto* sworker1_pkt_stream_V_keep_V = createStream((hls::stream<char>*)__xlx_apatb_param_worker1_pkt_stream_V_keep_V);
auto* sworker1_pkt_stream_V_strb_V = createStream((hls::stream<char>*)__xlx_apatb_param_worker1_pkt_stream_V_strb_V);
auto* sworker1_pkt_stream_V_user_V = createStream((hls::stream<char>*)__xlx_apatb_param_worker1_pkt_stream_V_user_V);
auto* sworker1_pkt_stream_V_last_V = createStream((hls::stream<char>*)__xlx_apatb_param_worker1_pkt_stream_V_last_V);
auto* sworker2_pkt_stream_V_data_V = createStream((hls::stream<long long>*)__xlx_apatb_param_worker2_pkt_stream_V_data_V);
auto* sworker2_pkt_stream_V_keep_V = createStream((hls::stream<char>*)__xlx_apatb_param_worker2_pkt_stream_V_keep_V);
auto* sworker2_pkt_stream_V_strb_V = createStream((hls::stream<char>*)__xlx_apatb_param_worker2_pkt_stream_V_strb_V);
auto* sworker2_pkt_stream_V_user_V = createStream((hls::stream<char>*)__xlx_apatb_param_worker2_pkt_stream_V_user_V);
auto* sworker2_pkt_stream_V_last_V = createStream((hls::stream<char>*)__xlx_apatb_param_worker2_pkt_stream_V_last_V);
  //Create input buffer for data_to_coo_stream_V_data_V
  ap_apatb_data_to_coo_stream_V_data_V_cap_bc = __xlx_data_to_coo_stream_V_data_V_V_size_Reader.read_size();
  long long* __xlx_data_to_coo_stream_V_data_V_input_buffer= new long long[ap_apatb_data_to_coo_stream_V_data_V_cap_bc];
auto* sdata_to_coo_stream_V_data_V = createStream((hls::stream<long long>*)__xlx_apatb_param_data_to_coo_stream_V_data_V);
  //Create input buffer for data_to_coo_stream_V_keep_V
  ap_apatb_data_to_coo_stream_V_keep_V_cap_bc = __xlx_data_to_coo_stream_V_keep_V_V_size_Reader.read_size();
  char* __xlx_data_to_coo_stream_V_keep_V_input_buffer= new char[ap_apatb_data_to_coo_stream_V_keep_V_cap_bc];
auto* sdata_to_coo_stream_V_keep_V = createStream((hls::stream<char>*)__xlx_apatb_param_data_to_coo_stream_V_keep_V);
  //Create input buffer for data_to_coo_stream_V_strb_V
  ap_apatb_data_to_coo_stream_V_strb_V_cap_bc = __xlx_data_to_coo_stream_V_strb_V_V_size_Reader.read_size();
  char* __xlx_data_to_coo_stream_V_strb_V_input_buffer= new char[ap_apatb_data_to_coo_stream_V_strb_V_cap_bc];
auto* sdata_to_coo_stream_V_strb_V = createStream((hls::stream<char>*)__xlx_apatb_param_data_to_coo_stream_V_strb_V);
  //Create input buffer for data_to_coo_stream_V_last_V
  ap_apatb_data_to_coo_stream_V_last_V_cap_bc = __xlx_data_to_coo_stream_V_last_V_V_size_Reader.read_size();
  char* __xlx_data_to_coo_stream_V_last_V_input_buffer= new char[ap_apatb_data_to_coo_stream_V_last_V_cap_bc];
auto* sdata_to_coo_stream_V_last_V = createStream((hls::stream<char>*)__xlx_apatb_param_data_to_coo_stream_V_last_V);
  // DUT call
  pma_read_merger_kernel(smerger_cmd_stream_V_data_V->data<__cosim_s20__>(), smerger_cmd_stream_V_keep_V->data<int>(), smerger_cmd_stream_V_strb_V->data<int>(), smerger_cmd_stream_V_last_V->data<char>(), sworker0_pkt_stream_V_data_V->data<long long>(), sworker0_pkt_stream_V_keep_V->data<char>(), sworker0_pkt_stream_V_strb_V->data<char>(), sworker0_pkt_stream_V_user_V->data<char>(), sworker0_pkt_stream_V_last_V->data<char>(), sworker1_pkt_stream_V_data_V->data<long long>(), sworker1_pkt_stream_V_keep_V->data<char>(), sworker1_pkt_stream_V_strb_V->data<char>(), sworker1_pkt_stream_V_user_V->data<char>(), sworker1_pkt_stream_V_last_V->data<char>(), sworker2_pkt_stream_V_data_V->data<long long>(), sworker2_pkt_stream_V_keep_V->data<char>(), sworker2_pkt_stream_V_strb_V->data<char>(), sworker2_pkt_stream_V_user_V->data<char>(), sworker2_pkt_stream_V_last_V->data<char>(), sdata_to_coo_stream_V_data_V->data<long long>(), sdata_to_coo_stream_V_keep_V->data<char>(), sdata_to_coo_stream_V_strb_V->data<char>(), sdata_to_coo_stream_V_last_V->data<char>());
smerger_cmd_stream_V_data_V->transfer((hls::stream<__cosim_s20__>*)__xlx_apatb_param_merger_cmd_stream_V_data_V);
smerger_cmd_stream_V_keep_V->transfer((hls::stream<int>*)__xlx_apatb_param_merger_cmd_stream_V_keep_V);
smerger_cmd_stream_V_strb_V->transfer((hls::stream<int>*)__xlx_apatb_param_merger_cmd_stream_V_strb_V);
s->transfer((hls::stream<char>*)__xlx_apatb_param_);
smerger_cmd_stream_V_last_V->transfer((hls::stream<char>*)__xlx_apatb_param_merger_cmd_stream_V_last_V);
s->transfer((hls::stream<char>*)__xlx_apatb_param_);
s->transfer((hls::stream<char>*)__xlx_apatb_param_);
sworker0_pkt_stream_V_data_V->transfer((hls::stream<long long>*)__xlx_apatb_param_worker0_pkt_stream_V_data_V);
sworker0_pkt_stream_V_keep_V->transfer((hls::stream<char>*)__xlx_apatb_param_worker0_pkt_stream_V_keep_V);
sworker0_pkt_stream_V_strb_V->transfer((hls::stream<char>*)__xlx_apatb_param_worker0_pkt_stream_V_strb_V);
sworker0_pkt_stream_V_user_V->transfer((hls::stream<char>*)__xlx_apatb_param_worker0_pkt_stream_V_user_V);
sworker0_pkt_stream_V_last_V->transfer((hls::stream<char>*)__xlx_apatb_param_worker0_pkt_stream_V_last_V);
s->transfer((hls::stream<char>*)__xlx_apatb_param_);
s->transfer((hls::stream<char>*)__xlx_apatb_param_);
sworker1_pkt_stream_V_data_V->transfer((hls::stream<long long>*)__xlx_apatb_param_worker1_pkt_stream_V_data_V);
sworker1_pkt_stream_V_keep_V->transfer((hls::stream<char>*)__xlx_apatb_param_worker1_pkt_stream_V_keep_V);
sworker1_pkt_stream_V_strb_V->transfer((hls::stream<char>*)__xlx_apatb_param_worker1_pkt_stream_V_strb_V);
sworker1_pkt_stream_V_user_V->transfer((hls::stream<char>*)__xlx_apatb_param_worker1_pkt_stream_V_user_V);
sworker1_pkt_stream_V_last_V->transfer((hls::stream<char>*)__xlx_apatb_param_worker1_pkt_stream_V_last_V);
s->transfer((hls::stream<char>*)__xlx_apatb_param_);
s->transfer((hls::stream<char>*)__xlx_apatb_param_);
sworker2_pkt_stream_V_data_V->transfer((hls::stream<long long>*)__xlx_apatb_param_worker2_pkt_stream_V_data_V);
sworker2_pkt_stream_V_keep_V->transfer((hls::stream<char>*)__xlx_apatb_param_worker2_pkt_stream_V_keep_V);
sworker2_pkt_stream_V_strb_V->transfer((hls::stream<char>*)__xlx_apatb_param_worker2_pkt_stream_V_strb_V);
sworker2_pkt_stream_V_user_V->transfer((hls::stream<char>*)__xlx_apatb_param_worker2_pkt_stream_V_user_V);
sworker2_pkt_stream_V_last_V->transfer((hls::stream<char>*)__xlx_apatb_param_worker2_pkt_stream_V_last_V);
s->transfer((hls::stream<char>*)__xlx_apatb_param_);
s->transfer((hls::stream<char>*)__xlx_apatb_param_);
sdata_to_coo_stream_V_data_V->transfer((hls::stream<long long>*)__xlx_apatb_param_data_to_coo_stream_V_data_V);
sdata_to_coo_stream_V_keep_V->transfer((hls::stream<char>*)__xlx_apatb_param_data_to_coo_stream_V_keep_V);
sdata_to_coo_stream_V_strb_V->transfer((hls::stream<char>*)__xlx_apatb_param_data_to_coo_stream_V_strb_V);
s->transfer((hls::stream<char>*)__xlx_apatb_param_);
sdata_to_coo_stream_V_last_V->transfer((hls::stream<char>*)__xlx_apatb_param_data_to_coo_stream_V_last_V);
s->transfer((hls::stream<char>*)__xlx_apatb_param_);
s->transfer((hls::stream<char>*)__xlx_apatb_param_);
}
