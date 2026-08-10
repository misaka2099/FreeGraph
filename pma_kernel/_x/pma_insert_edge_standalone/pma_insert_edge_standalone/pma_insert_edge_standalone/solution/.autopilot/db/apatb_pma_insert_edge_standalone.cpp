#include <algorithm>
#include <complex>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include "ap_fixed.h"
#include "ap_int.h"
#include "autopilot_cbe.h"
#include "hls_half.h"
#include "hls_signal_handler.h"
#include "hls_stream.h"

using namespace std;

// wrapc file define:
#define AUTOTB_TVIN_hbm_gate "../tv/cdatafile/c.pma_insert_edge_standalone.autotvin_hbm_gate.dat"
#define AUTOTB_TVOUT_hbm_gate "../tv/cdatafile/c.pma_insert_edge_standalone.autotvout_hbm_gate.dat"
#define AUTOTB_TVIN_hbm_bs "../tv/cdatafile/c.pma_insert_edge_standalone.autotvin_hbm_bs.dat"
#define AUTOTB_TVOUT_hbm_bs "../tv/cdatafile/c.pma_insert_edge_standalone.autotvout_hbm_bs.dat"
#define AUTOTB_TVIN_hbm_rw "../tv/cdatafile/c.pma_insert_edge_standalone.autotvin_hbm_rw.dat"
#define AUTOTB_TVOUT_hbm_rw "../tv/cdatafile/c.pma_insert_edge_standalone.autotvout_hbm_rw.dat"
#define AUTOTB_TVIN_hbm_red "../tv/cdatafile/c.pma_insert_edge_standalone.autotvin_hbm_red.dat"
#define AUTOTB_TVOUT_hbm_red "../tv/cdatafile/c.pma_insert_edge_standalone.autotvout_hbm_red.dat"
#define AUTOTB_TVIN_hbm_buffer_for_red "../tv/cdatafile/c.pma_insert_edge_standalone.autotvin_hbm_buffer_for_red.dat"
#define AUTOTB_TVOUT_hbm_buffer_for_red "../tv/cdatafile/c.pma_insert_edge_standalone.autotvout_hbm_buffer_for_red.dat"
#define AUTOTB_TVIN_hbm_merge "../tv/cdatafile/c.pma_insert_edge_standalone.autotvin_hbm_merge.dat"
#define AUTOTB_TVOUT_hbm_merge "../tv/cdatafile/c.pma_insert_edge_standalone.autotvout_hbm_merge.dat"
#define AUTOTB_TVIN_hbm_merge_buffer "../tv/cdatafile/c.pma_insert_edge_standalone.autotvin_hbm_merge_buffer.dat"
#define AUTOTB_TVOUT_hbm_merge_buffer "../tv/cdatafile/c.pma_insert_edge_standalone.autotvout_hbm_merge_buffer.dat"
#define AUTOTB_TVIN_task_buffer "../tv/cdatafile/c.pma_insert_edge_standalone.autotvin_task_buffer.dat"
#define AUTOTB_TVOUT_task_buffer "../tv/cdatafile/c.pma_insert_edge_standalone.autotvout_task_buffer.dat"
#define AUTOTB_TVIN_pma_raw_data_start "../tv/cdatafile/c.pma_insert_edge_standalone.autotvin_pma_raw_data_start.dat"
#define AUTOTB_TVOUT_pma_raw_data_start "../tv/cdatafile/c.pma_insert_edge_standalone.autotvout_pma_raw_data_start.dat"
#define AUTOTB_TVIN_pma_vecinfo_start "../tv/cdatafile/c.pma_insert_edge_standalone.autotvin_pma_vecinfo_start.dat"
#define AUTOTB_TVOUT_pma_vecinfo_start "../tv/cdatafile/c.pma_insert_edge_standalone.autotvout_pma_vecinfo_start.dat"
#define AUTOTB_TVIN_edge_count "../tv/cdatafile/c.pma_insert_edge_standalone.autotvin_edge_count.dat"
#define AUTOTB_TVOUT_edge_count "../tv/cdatafile/c.pma_insert_edge_standalone.autotvout_edge_count.dat"
#define AUTOTB_TVIN_worker_id "../tv/cdatafile/c.pma_insert_edge_standalone.autotvin_worker_id.dat"
#define AUTOTB_TVOUT_worker_id "../tv/cdatafile/c.pma_insert_edge_standalone.autotvout_worker_id.dat"
#define AUTOTB_TVIN_bitmap_offset "../tv/cdatafile/c.pma_insert_edge_standalone.autotvin_bitmap_offset.dat"
#define AUTOTB_TVOUT_bitmap_offset "../tv/cdatafile/c.pma_insert_edge_standalone.autotvout_bitmap_offset.dat"
#define AUTOTB_TVIN_alloc_cmd_stream_V_data_V "../tv/cdatafile/c.pma_insert_edge_standalone.autotvin_alloc_cmd_stream_V_data_V.dat"
#define AUTOTB_TVOUT_alloc_cmd_stream_V_data_V "../tv/cdatafile/c.pma_insert_edge_standalone.autotvout_alloc_cmd_stream_V_data_V.dat"
#define AUTOTB_TVIN_alloc_cmd_stream_V_keep_V "../tv/cdatafile/c.pma_insert_edge_standalone.autotvin_alloc_cmd_stream_V_keep_V.dat"
#define AUTOTB_TVOUT_alloc_cmd_stream_V_keep_V "../tv/cdatafile/c.pma_insert_edge_standalone.autotvout_alloc_cmd_stream_V_keep_V.dat"
#define AUTOTB_TVIN_alloc_cmd_stream_V_strb_V "../tv/cdatafile/c.pma_insert_edge_standalone.autotvin_alloc_cmd_stream_V_strb_V.dat"
#define AUTOTB_TVOUT_alloc_cmd_stream_V_strb_V "../tv/cdatafile/c.pma_insert_edge_standalone.autotvout_alloc_cmd_stream_V_strb_V.dat"
#define AUTOTB_TVIN_alloc_cmd_stream_V_last_V "../tv/cdatafile/c.pma_insert_edge_standalone.autotvin_alloc_cmd_stream_V_last_V.dat"
#define AUTOTB_TVOUT_alloc_cmd_stream_V_last_V "../tv/cdatafile/c.pma_insert_edge_standalone.autotvout_alloc_cmd_stream_V_last_V.dat"
#define WRAPC_STREAM_SIZE_OUT_alloc_cmd_stream_V_data_V "../tv/stream_size/stream_size_out_alloc_cmd_stream_V_data_V.dat"
#define WRAPC_STREAM_EGRESS_STATUS_alloc_cmd_stream_V_data_V "../tv/stream_size/stream_egress_status_alloc_cmd_stream_V_data_V.dat"
#define WRAPC_STREAM_SIZE_OUT_alloc_cmd_stream_V_keep_V "../tv/stream_size/stream_size_out_alloc_cmd_stream_V_keep_V.dat"
#define WRAPC_STREAM_EGRESS_STATUS_alloc_cmd_stream_V_keep_V "../tv/stream_size/stream_egress_status_alloc_cmd_stream_V_keep_V.dat"
#define WRAPC_STREAM_SIZE_OUT_alloc_cmd_stream_V_strb_V "../tv/stream_size/stream_size_out_alloc_cmd_stream_V_strb_V.dat"
#define WRAPC_STREAM_EGRESS_STATUS_alloc_cmd_stream_V_strb_V "../tv/stream_size/stream_egress_status_alloc_cmd_stream_V_strb_V.dat"
#define WRAPC_STREAM_SIZE_OUT_alloc_cmd_stream_V_last_V "../tv/stream_size/stream_size_out_alloc_cmd_stream_V_last_V.dat"
#define WRAPC_STREAM_EGRESS_STATUS_alloc_cmd_stream_V_last_V "../tv/stream_size/stream_egress_status_alloc_cmd_stream_V_last_V.dat"
#define AUTOTB_TVIN_alloc_rsp_stream_V_data_V "../tv/cdatafile/c.pma_insert_edge_standalone.autotvin_alloc_rsp_stream_V_data_V.dat"
#define AUTOTB_TVOUT_alloc_rsp_stream_V_data_V "../tv/cdatafile/c.pma_insert_edge_standalone.autotvout_alloc_rsp_stream_V_data_V.dat"
#define AUTOTB_TVIN_alloc_rsp_stream_V_keep_V "../tv/cdatafile/c.pma_insert_edge_standalone.autotvin_alloc_rsp_stream_V_keep_V.dat"
#define AUTOTB_TVOUT_alloc_rsp_stream_V_keep_V "../tv/cdatafile/c.pma_insert_edge_standalone.autotvout_alloc_rsp_stream_V_keep_V.dat"
#define AUTOTB_TVIN_alloc_rsp_stream_V_strb_V "../tv/cdatafile/c.pma_insert_edge_standalone.autotvin_alloc_rsp_stream_V_strb_V.dat"
#define AUTOTB_TVOUT_alloc_rsp_stream_V_strb_V "../tv/cdatafile/c.pma_insert_edge_standalone.autotvout_alloc_rsp_stream_V_strb_V.dat"
#define AUTOTB_TVIN_alloc_rsp_stream_V_last_V "../tv/cdatafile/c.pma_insert_edge_standalone.autotvin_alloc_rsp_stream_V_last_V.dat"
#define AUTOTB_TVOUT_alloc_rsp_stream_V_last_V "../tv/cdatafile/c.pma_insert_edge_standalone.autotvout_alloc_rsp_stream_V_last_V.dat"
#define WRAPC_STREAM_SIZE_IN_alloc_rsp_stream_V_data_V "../tv/stream_size/stream_size_in_alloc_rsp_stream_V_data_V.dat"
#define WRAPC_STREAM_INGRESS_STATUS_alloc_rsp_stream_V_data_V "../tv/stream_size/stream_ingress_status_alloc_rsp_stream_V_data_V.dat"
#define WRAPC_STREAM_SIZE_IN_alloc_rsp_stream_V_keep_V "../tv/stream_size/stream_size_in_alloc_rsp_stream_V_keep_V.dat"
#define WRAPC_STREAM_INGRESS_STATUS_alloc_rsp_stream_V_keep_V "../tv/stream_size/stream_ingress_status_alloc_rsp_stream_V_keep_V.dat"
#define WRAPC_STREAM_SIZE_IN_alloc_rsp_stream_V_strb_V "../tv/stream_size/stream_size_in_alloc_rsp_stream_V_strb_V.dat"
#define WRAPC_STREAM_INGRESS_STATUS_alloc_rsp_stream_V_strb_V "../tv/stream_size/stream_ingress_status_alloc_rsp_stream_V_strb_V.dat"
#define WRAPC_STREAM_SIZE_IN_alloc_rsp_stream_V_last_V "../tv/stream_size/stream_size_in_alloc_rsp_stream_V_last_V.dat"
#define WRAPC_STREAM_INGRESS_STATUS_alloc_rsp_stream_V_last_V "../tv/stream_size/stream_ingress_status_alloc_rsp_stream_V_last_V.dat"
#define AUTOTB_TVIN_bitmap_init_V_data_V "../tv/cdatafile/c.pma_insert_edge_standalone.autotvin_bitmap_init_V_data_V.dat"
#define AUTOTB_TVOUT_bitmap_init_V_data_V "../tv/cdatafile/c.pma_insert_edge_standalone.autotvout_bitmap_init_V_data_V.dat"
#define AUTOTB_TVIN_bitmap_init_V_keep_V "../tv/cdatafile/c.pma_insert_edge_standalone.autotvin_bitmap_init_V_keep_V.dat"
#define AUTOTB_TVOUT_bitmap_init_V_keep_V "../tv/cdatafile/c.pma_insert_edge_standalone.autotvout_bitmap_init_V_keep_V.dat"
#define AUTOTB_TVIN_bitmap_init_V_strb_V "../tv/cdatafile/c.pma_insert_edge_standalone.autotvin_bitmap_init_V_strb_V.dat"
#define AUTOTB_TVOUT_bitmap_init_V_strb_V "../tv/cdatafile/c.pma_insert_edge_standalone.autotvout_bitmap_init_V_strb_V.dat"
#define AUTOTB_TVIN_bitmap_init_V_last_V "../tv/cdatafile/c.pma_insert_edge_standalone.autotvin_bitmap_init_V_last_V.dat"
#define AUTOTB_TVOUT_bitmap_init_V_last_V "../tv/cdatafile/c.pma_insert_edge_standalone.autotvout_bitmap_init_V_last_V.dat"
#define WRAPC_STREAM_SIZE_OUT_bitmap_init_V_data_V "../tv/stream_size/stream_size_out_bitmap_init_V_data_V.dat"
#define WRAPC_STREAM_EGRESS_STATUS_bitmap_init_V_data_V "../tv/stream_size/stream_egress_status_bitmap_init_V_data_V.dat"
#define WRAPC_STREAM_SIZE_OUT_bitmap_init_V_keep_V "../tv/stream_size/stream_size_out_bitmap_init_V_keep_V.dat"
#define WRAPC_STREAM_EGRESS_STATUS_bitmap_init_V_keep_V "../tv/stream_size/stream_egress_status_bitmap_init_V_keep_V.dat"
#define WRAPC_STREAM_SIZE_OUT_bitmap_init_V_strb_V "../tv/stream_size/stream_size_out_bitmap_init_V_strb_V.dat"
#define WRAPC_STREAM_EGRESS_STATUS_bitmap_init_V_strb_V "../tv/stream_size/stream_egress_status_bitmap_init_V_strb_V.dat"
#define WRAPC_STREAM_SIZE_OUT_bitmap_init_V_last_V "../tv/stream_size/stream_size_out_bitmap_init_V_last_V.dat"
#define WRAPC_STREAM_EGRESS_STATUS_bitmap_init_V_last_V "../tv/stream_size/stream_egress_status_bitmap_init_V_last_V.dat"
#define AUTOTB_TVIN_online_in "../tv/cdatafile/c.pma_insert_edge_standalone.autotvin_online_in.dat"
#define WRAPC_STREAM_SIZE_IN_online_in "../tv/stream_size/stream_size_in_online_in.dat"
#define WRAPC_STREAM_INGRESS_STATUS_online_in "../tv/stream_size/stream_ingress_status_online_in.dat"
#define AUTOTB_TVIN_gmem0 "../tv/cdatafile/c.pma_insert_edge_standalone.autotvin_gmem0.dat"
#define AUTOTB_TVOUT_gmem0 "../tv/cdatafile/c.pma_insert_edge_standalone.autotvout_gmem0.dat"
#define AUTOTB_TVIN_gmem10 "../tv/cdatafile/c.pma_insert_edge_standalone.autotvin_gmem10.dat"
#define AUTOTB_TVOUT_gmem10 "../tv/cdatafile/c.pma_insert_edge_standalone.autotvout_gmem10.dat"
#define AUTOTB_TVIN_gmem4 "../tv/cdatafile/c.pma_insert_edge_standalone.autotvin_gmem4.dat"
#define AUTOTB_TVOUT_gmem4 "../tv/cdatafile/c.pma_insert_edge_standalone.autotvout_gmem4.dat"
#define AUTOTB_TVIN_gmem5 "../tv/cdatafile/c.pma_insert_edge_standalone.autotvin_gmem5.dat"
#define AUTOTB_TVOUT_gmem5 "../tv/cdatafile/c.pma_insert_edge_standalone.autotvout_gmem5.dat"
#define AUTOTB_TVIN_gmem6 "../tv/cdatafile/c.pma_insert_edge_standalone.autotvin_gmem6.dat"
#define AUTOTB_TVOUT_gmem6 "../tv/cdatafile/c.pma_insert_edge_standalone.autotvout_gmem6.dat"
#define AUTOTB_TVIN_gmem7 "../tv/cdatafile/c.pma_insert_edge_standalone.autotvin_gmem7.dat"
#define AUTOTB_TVOUT_gmem7 "../tv/cdatafile/c.pma_insert_edge_standalone.autotvout_gmem7.dat"
#define AUTOTB_TVIN_gmem8 "../tv/cdatafile/c.pma_insert_edge_standalone.autotvin_gmem8.dat"
#define AUTOTB_TVOUT_gmem8 "../tv/cdatafile/c.pma_insert_edge_standalone.autotvout_gmem8.dat"
#define AUTOTB_TVIN_gmem9 "../tv/cdatafile/c.pma_insert_edge_standalone.autotvin_gmem9.dat"
#define AUTOTB_TVOUT_gmem9 "../tv/cdatafile/c.pma_insert_edge_standalone.autotvout_gmem9.dat"


// tvout file define:
#define AUTOTB_TVOUT_PC_alloc_cmd_stream_V_data_V "../tv/rtldatafile/rtl.pma_insert_edge_standalone.autotvout_alloc_cmd_stream_V_data_V.dat"
#define AUTOTB_TVOUT_PC_alloc_cmd_stream_V_keep_V "../tv/rtldatafile/rtl.pma_insert_edge_standalone.autotvout_alloc_cmd_stream_V_keep_V.dat"
#define AUTOTB_TVOUT_PC_alloc_cmd_stream_V_strb_V "../tv/rtldatafile/rtl.pma_insert_edge_standalone.autotvout_alloc_cmd_stream_V_strb_V.dat"
#define AUTOTB_TVOUT_PC_alloc_cmd_stream_V_last_V "../tv/rtldatafile/rtl.pma_insert_edge_standalone.autotvout_alloc_cmd_stream_V_last_V.dat"
#define AUTOTB_TVOUT_PC_alloc_rsp_stream_V_data_V "../tv/rtldatafile/rtl.pma_insert_edge_standalone.autotvout_alloc_rsp_stream_V_data_V.dat"
#define AUTOTB_TVOUT_PC_alloc_rsp_stream_V_keep_V "../tv/rtldatafile/rtl.pma_insert_edge_standalone.autotvout_alloc_rsp_stream_V_keep_V.dat"
#define AUTOTB_TVOUT_PC_alloc_rsp_stream_V_strb_V "../tv/rtldatafile/rtl.pma_insert_edge_standalone.autotvout_alloc_rsp_stream_V_strb_V.dat"
#define AUTOTB_TVOUT_PC_alloc_rsp_stream_V_last_V "../tv/rtldatafile/rtl.pma_insert_edge_standalone.autotvout_alloc_rsp_stream_V_last_V.dat"
#define AUTOTB_TVOUT_PC_bitmap_init_V_data_V "../tv/rtldatafile/rtl.pma_insert_edge_standalone.autotvout_bitmap_init_V_data_V.dat"
#define AUTOTB_TVOUT_PC_bitmap_init_V_keep_V "../tv/rtldatafile/rtl.pma_insert_edge_standalone.autotvout_bitmap_init_V_keep_V.dat"
#define AUTOTB_TVOUT_PC_bitmap_init_V_strb_V "../tv/rtldatafile/rtl.pma_insert_edge_standalone.autotvout_bitmap_init_V_strb_V.dat"
#define AUTOTB_TVOUT_PC_bitmap_init_V_last_V "../tv/rtldatafile/rtl.pma_insert_edge_standalone.autotvout_bitmap_init_V_last_V.dat"
#define AUTOTB_TVOUT_PC_gmem0 "../tv/rtldatafile/rtl.pma_insert_edge_standalone.autotvout_gmem0.dat"
#define AUTOTB_TVOUT_PC_gmem10 "../tv/rtldatafile/rtl.pma_insert_edge_standalone.autotvout_gmem10.dat"
#define AUTOTB_TVOUT_PC_gmem4 "../tv/rtldatafile/rtl.pma_insert_edge_standalone.autotvout_gmem4.dat"
#define AUTOTB_TVOUT_PC_gmem5 "../tv/rtldatafile/rtl.pma_insert_edge_standalone.autotvout_gmem5.dat"
#define AUTOTB_TVOUT_PC_gmem6 "../tv/rtldatafile/rtl.pma_insert_edge_standalone.autotvout_gmem6.dat"
#define AUTOTB_TVOUT_PC_gmem7 "../tv/rtldatafile/rtl.pma_insert_edge_standalone.autotvout_gmem7.dat"
#define AUTOTB_TVOUT_PC_gmem8 "../tv/rtldatafile/rtl.pma_insert_edge_standalone.autotvout_gmem8.dat"
#define AUTOTB_TVOUT_PC_gmem9 "../tv/rtldatafile/rtl.pma_insert_edge_standalone.autotvout_gmem9.dat"


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
  const bool little_endian()
  {
    int a = 1;
    return *(char*)&a == 1;
  }

  inline void rev_endian(unsigned char *p, size_t nbytes)
  {
    std::reverse(p, p+nbytes);
  }

  const bool LE = little_endian();

  inline size_t least_nbyte(size_t width)
  {
    return (width+7)>>3;
  }

  std::string formatData(unsigned char *pos, size_t wbits)
  {
    size_t wbytes = least_nbyte(wbits);
    size_t i = LE ? wbytes-1 : 0;
    auto next = [&] () {
      auto c = pos[i];
      LE ? --i : ++i;
      return c;
    };
    std::ostringstream ss;
    ss << "0x";
    if (int t = (wbits & 0x7)) {
      if (t <= 4) {
        unsigned char mask = (1<<t)-1;
        ss << std::hex << std::setfill('0') << std::setw(1)
           << (int) (next() & mask);
        wbytes -= 1;
      }
    }
    for (size_t i = 0; i < wbytes; ++i) {
      ss << std::hex << std::setfill('0') << std::setw(2) << (int)next();
    }
    return ss.str();
  }

  char ord(char c)
  {
    if (c >= 'a' && c <= 'f') {
      return c-'a'+10;
    } else if (c >= 'A' && c <= 'F') {
      return c-'A'+10;
    } else if (c >= '0' && c <= '9') {
      return c-'0';
    } else {
      throw SimException("Not Hexdecimal Digit", __LINE__);
    }
  }

  void unformatData(const char *data, unsigned char *put)
  {
    size_t wbytes = (strlen(data)-2+1)>>1;
    put = LE ? put : put+wbytes-1;
    auto nextp = [&] () {
      return LE ? put++ : put--;
    };
    const char *c = data + strlen(data) - 1;
    auto next = [&] () {
      char res = ord(*c);
      --c;
      return res;
    };
    size_t fbytes = (strlen(data)-2)>>1;
    for (size_t i = 0; i < fbytes; ++i) {
      char l = next();
      char h = next();
      *nextp() = (h<<4)+l;
    }
    if (wbytes > fbytes) {
      *nextp() = next();
    }
  }

  char* strip(char *s)
  {
    while (isspace(*s)) {
      ++s;
    }
    for (char *p = s+strlen(s)-1; p >= s; --p) {
      if (isspace(*p)) {
        *p = 0;
      } else {
        return s;
      }
    }
    return s;
  }

  size_t sum(const std::vector<size_t> &v)
  {
    size_t res = 0;
    for (const auto &e : v) {
      res += e;
    }
    return res;
  }

  const char* bad = "Bad TV file";
  const char* err = "Error on TV file";

  const unsigned char bmark[] = {
    0x5a, 0x5a, 0xa5, 0xa5, 0x0f, 0x0f, 0xf0, 0xf0
  };

#ifdef USE_BINARY_TV_FILE
  class Input {
    FILE *fp;
    long pos;

    void read(unsigned char *buf, size_t size)
    {
      if (fread(buf, size, 1, fp) != 1) {
        throw SimException(bad, __LINE__);
      }
      if (LE) {
        rev_endian(buf, size);
      }
    }

  public:
    void advance(size_t nbytes)
    {
      if (fseek(fp, nbytes, SEEK_CUR) == -1) {
        throw SimException(bad, __LINE__);
      }
    }

    Input(const char *path) : fp(nullptr)
    {
      fp = fopen(path, "rb");
      if (fp == nullptr) {
        errExit(__LINE__, err);
      }
    }

    void begin()
    {
      advance(8);
      pos = ftell(fp);
    }

    void reset()
    {
      fseek(fp, pos, SEEK_SET);
    }

    void into(unsigned char *param, size_t wbytes, size_t psize, size_t depth)
    {
      for (size_t i = 0; i < depth; ++i) {
        read(param, wbytes);
        param += psize;
      }
    }

    ~Input()
    {
      unsigned char buf[8];
      size_t res = fread(buf, 8, 1, fp);
      fclose(fp);
      if (res != 1) {
        errExit(__LINE__, bad);
      }
      if (std::memcmp(buf, bmark, 8) != 0) {
        errExit(__LINE__, bad);
      }
    }
  };

  class Output {
    FILE *fp;

    void write(unsigned char *buf, size_t size)
    {
      if (LE) {
        rev_endian(buf, size);
      }
      if (fwrite(buf, size, 1, fp) != 1) {
        throw SimException(err, __LINE__);
      }
      if (LE) {
        rev_endian(buf, size);
      }
    }

  public:
    Output(const char *path) : fp(nullptr)
    {
      fp = fopen(path, "wb");
      if (fp == nullptr) {
        errExit(__LINE__, err);
      }
    }

    void begin(size_t total)
    {
      unsigned char buf[8] = {0};
      std::memcpy(buf, &total, sizeof(buf));
      write(buf, sizeof(buf));
    }

    void from(unsigned char *param, size_t wbytes, size_t psize, size_t depth, size_t skip)
    {
      param -= psize*skip;
      for (size_t i = 0; i < depth; ++i) {
        write(param, wbytes);
        param += psize;
      }
    }

    ~Output()
    {
      size_t res = fwrite(bmark, 8, 1, fp);
      fclose(fp);
      if (res != 1) {
        errExit(__LINE__, err);
      }
    }
  };
#endif

  class Reader {
    FILE *fp;
    long pos;
    int size;
    char *s;

    void readline()
    {
      s = fgets(s, size, fp);
      if (s == nullptr) {
        throw SimException(bad, __LINE__);
      }
    }

  public:
    Reader(const char *path) : fp(nullptr), size(1<<12), s(new char[size])
    {
      try {
        fp = fopen(path, "r");
        if (fp == nullptr) {
          throw SimException(err, __LINE__);
        } else {
          readline();
          static const char mark[] = "[[[runtime]]]\n";
          if (strcmp(s, mark) != 0) {
            throw SimException(bad, __LINE__);
          }
        }
      } catch (const hls::sim::SimException &e) {
        errExit(e.line, e.msg);
      }
    }

    ~Reader()
    {
      fclose(fp);
      delete[] s;
    }

    void begin()
    {
      readline();
      static const char mark[] = "[[transaction]]";
      if (strncmp(s, mark, strlen(mark)) != 0) {
        throw SimException(bad, __LINE__);
      }
      pos = ftell(fp);
    }

    void reset()
    {
      fseek(fp, pos, SEEK_SET);
    }

    void skip(size_t n)
    {
      for (size_t i = 0; i < n; ++i) {
        readline();
      }
    }

    char* next()
    {
      long pos = ftell(fp);
      readline();
      if (*s == '[') {
        fseek(fp, pos, SEEK_SET);
        return nullptr;
      }
      return strip(s);
    }

    void end()
    {
      do {
        readline();
      } while (strcmp(s, "[[/transaction]]\n") != 0);
    }
  };

  class Writer {
    FILE *fp;

    void write(const char *s)
    {
      if (fputs(s, fp) == EOF) {
        throw SimException(err, __LINE__);
      }
    }

  public:
    Writer(const char *path) : fp(nullptr)
    {
      try {
        fp = fopen(path, "w");
        if (fp == nullptr) {
          throw SimException(err, __LINE__);
        } else {
          static const char mark[] = "[[[runtime]]]\n";
          write(mark);
        }
      } catch (const hls::sim::SimException &e) {
        errExit(e.line, e.msg);
      }
    }

    virtual ~Writer()
    {
      try {
        static const char mark[] = "[[[/runtime]]]\n";
        write(mark);
      } catch (const hls::sim::SimException &e) {
        errExit(e.line, e.msg);
      }
      fclose(fp);
    }

    void begin(size_t AESL_transaction)
    {
      static const char mark[] = "[[transaction]]           ";
      write(mark);
      auto buf = std::to_string(AESL_transaction);
      buf.push_back('\n');
      buf.push_back('\0');
      write(buf.data());
    }

    void next(const char *s)
    {
      write(s);
      write("\n");
    }

    void end()
    {
      static const char mark[] = "[[/transaction]]\n";
      write(mark);
    }
  };

  bool RTLOutputCheckAndReplacement(char *data)
  {
    bool changed = false;
    for (size_t i = 2; i < strlen(data); ++i) {
      if (data[i] == 'X' || data[i] == 'x') {
        data[i] = '0';
        changed = true;
      }
    }
    return changed;
  }

  void warnOnX()
  {
    static const char msg[] =
      "WARNING: [SIM 212-201] RTL produces unknown value "
      "'x' or 'X' on some port, possible cause: "
      "There are uninitialized variables in the design.\n";
    fprintf(stderr, msg);
  }

#ifndef POST_CHECK
  class RefTCL {
    FILE *fp;
    std::ostringstream ss;

    void formatDepth()
    {
      ss << "set depth_list {\n";
      for (auto &p : depth) {
        ss << "  {" << p.first << " " << p.second << "}\n";
      }
      if (nameHBM != "") {
        ss << "  {" << nameHBM << " " << depthHBM << "}\n";
      }
      ss << "}\n";
    }

    void formatTransNum()
    {
      ss << "set trans_num " << AESL_transaction << "\n";
    }

    void formatHBM()
    {
      ss << "set HBM_ArgDict {\n"
         << "  Name " << nameHBM << "\n"
         << "  Port " << portHBM << "\n"
         << "  BitWidth " << widthHBM << "\n"
         << "}\n";
    }

    void close()
    {
      formatDepth();
      formatTransNum();
      if (nameHBM != "") {
        formatHBM();
      }
      std::string &&s { ss.str() };
      size_t res = fwrite(s.data(), s.size(), 1, fp);
      fclose(fp);
      if (res != 1) {
        errExit(__LINE__, err);
      }
    }

  public:
    std::map<const std::string, size_t> depth;
    std::string nameHBM;
    size_t depthHBM;
    std::string portHBM;
    unsigned widthHBM;
    size_t AESL_transaction;

    RefTCL(const char *path)
    {
      fp = fopen(path, "w");
      if (fp == nullptr) {
        errExit(__LINE__, err);
      }
    }

    void set(const char* name, size_t dep)
    {
      if (depth[name] < dep) {
        depth[name] = dep;
      }
    }

    ~RefTCL()
    {
      close();
    }
  };

#endif

  struct Register {
    const char* name;
    unsigned width;
#ifdef POST_CHECK
    Reader* reader;
#else
    Writer* owriter;
    Writer* iwriter;
#endif
    void* param;

#ifndef POST_CHECK
    void doTCL(RefTCL &tcl)
    {
      if (strcmp(name, "return") == 0) {
        tcl.set("ap_return", 1);
      } else {
        tcl.set(name, 1);
      }
    }
#endif
    ~Register()
    {
#ifdef POST_CHECK
      delete reader;
#else
      delete owriter;
      delete iwriter;
#endif
    }
  };

  template<typename Reader, typename Writer>
  struct Memory {
    unsigned width;
    unsigned asize;
    bool hbm;
    std::vector<const char*> name;
#ifdef POST_CHECK
    Reader* reader;
#else
    Writer* owriter;
    Writer* iwriter;
#endif
    std::vector<void*> param;
    std::vector<size_t> depth;
    std::vector<size_t> offset;
    std::vector<bool> hasWrite;

#ifndef POST_CHECK
    void doTCL(RefTCL &tcl)
    {
      if (hbm) {
        tcl.nameHBM.append(name[0]);
        tcl.portHBM.append("{").append(name[0]);
        for (size_t i = 1; i < name.size(); ++i) {
          tcl.nameHBM.append("_").append(name[i]);
          tcl.portHBM.append(" ").append(name[i]);
        }
        tcl.nameHBM.append("_HBM");
        tcl.portHBM.append("}");
        tcl.widthHBM = width;
        tcl.depthHBM = depth[0];
      } else {
        tcl.set(name[0], sum(depth));
      }
    }
#endif

    ~Memory()
    {
#ifdef POST_CHECK
      delete reader;
#else
      delete owriter;
      delete iwriter;
#endif
    }
  };

  struct FIFO {
    unsigned width;
    unsigned asize;
    const char* name;
#ifdef POST_CHECK
    Reader* reader;
#else
    Writer* owriter;
    Writer* iwriter;
#endif
    void* param;
    size_t depth;
    bool hasWrite;

#ifndef POST_CHECK
    void doTCL(RefTCL &tcl)
    {
      tcl.set(name, depth);
    }
#endif

    ~FIFO()
    {
#ifdef POST_CHECK
      delete reader;
#else
      delete owriter;
      delete iwriter;
#endif
    }
  };

  template<typename E>
  struct Stream {
    unsigned width;
    const char* name;
#ifdef POST_CHECK
    Reader* reader;
#else
    Writer* writer;
    Writer* swriter;
    Writer* gwriter;
#endif
    hls::stream<E>* param;
    std::vector<E> buf;
    size_t initSize;
    size_t depth;
    bool hasWrite;

    void markSize()
    {
      initSize = param->size();
    }

    void buffer()
    {
      buf.clear();
      while (!param->empty()) {
        buf.push_back(param->read());
      }
      for (auto &e : buf) {
        param->write(e);
      }
    }

#ifndef POST_CHECK
    void doTCL(RefTCL &tcl)
    {
      tcl.set(name, depth);
    }
#endif

    ~Stream()
    {
#ifdef POST_CHECK
      delete reader;
#else
      delete writer;
      delete swriter;
      delete gwriter;
#endif
    }
  };

#ifdef POST_CHECK
  void check(Register &port)
  {
    port.reader->begin();
    bool foundX = false;
    if (char *s = port.reader->next()) {
      foundX |= RTLOutputCheckAndReplacement(s);
      unformatData(s, (unsigned char*)port.param);
    }
    port.reader->end();
    if (foundX) {
      warnOnX();
    }
  }

#ifdef USE_BINARY_TV_FILE
  void checkHBM(Memory<Input, Output> &port)
  {
    port.reader->begin();
    size_t wbytes = least_nbyte(port.width);
    for (size_t i = 0; i < port.param.size(); ++i) {
      if (port.hasWrite[i]) {
        port.reader->reset();
        size_t skip = port.offset[i];
        size_t depth = port.depth[i] - skip;
        port.reader->advance(wbytes*skip);
        port.reader->into((unsigned char*)port.param[i], wbytes,
                          port.asize, depth);
      }
    }
  }

  void check(Memory<Input, Output> &port)
  {
    if (port.hbm) {
      return checkHBM(port);
    } else {
      port.reader->begin();
      size_t wbytes = least_nbyte(port.width);
      for (size_t i = 0; i < port.param.size(); ++i) {
        if (port.hasWrite[i]) {
          port.reader->into((unsigned char*)port.param[i], wbytes,
                            port.asize, port.depth[i]);
        } else {
          port.reader->advance(wbytes*port.depth[i]);
        }
      }
    }
  }
#endif
  void checkHBM(Memory<Reader, Writer> &port)
  {
    port.reader->begin();
    bool foundX = false;
    for (size_t i = 0, last = port.param.size()-1; i <= last; ++i) {
      if (port.hasWrite[i]) {
        port.reader->skip(port.offset[i]);
        for (size_t j = 0; j < port.depth[i]-port.offset[i]; ++j) {
          if (char *s = port.reader->next()) {
            foundX |= RTLOutputCheckAndReplacement(s);
            unformatData(s, (unsigned char*)port.param[i]+j*port.asize);
          }
        }
        if (i < last) {
          port.reader->reset();
        }
      }
    }
    port.reader->end();
    if (foundX) {
      warnOnX();
    }
  }

  void check(Memory<Reader, Writer> &port)
  {
    if (port.hbm) {
      return checkHBM(port);
    } else {
      port.reader->begin();
      bool foundX = false;
      for (size_t i = 0; i < port.param.size(); ++i) {
        if (port.hasWrite[i]) {
          for (size_t j = 0; j < port.depth[i]; ++j) {
            if (char *s = port.reader->next()) {
              foundX |= RTLOutputCheckAndReplacement(s);
              unformatData(s, (unsigned char*)port.param[i]+j*port.asize);
            }
          }
        } else {
          port.reader->skip(port.depth[i]);
        }
      }
      port.reader->end();
      if (foundX) {
        warnOnX();
      }
    }
  }

  void check(FIFO &port)
  {
    port.reader->begin();
    bool foundX = false;
    if (port.hasWrite) {
      for (size_t j = 0; j < port.depth; ++j) {
        if (char *s = port.reader->next()) {
          foundX |= RTLOutputCheckAndReplacement(s);
          unformatData(s, (unsigned char*)port.param+j*port.asize);
        }
      }
    }
    port.reader->end();
    if (foundX) {
      warnOnX();
    }
  }

  template<typename E>
  void check(Stream<E> &port)
  {
    if (port.hasWrite) {
      port.reader->begin();
      bool foundX = false;
      E *p = new E;
      while (char *s = port.reader->next()) {
        foundX |= RTLOutputCheckAndReplacement(s);
        unformatData(s, (unsigned char*)p);
        port.param->write(*p);
      }
      delete p;
      port.reader->end();
      if (foundX) {
        warnOnX();
      }
    } else {
      port.reader->begin();
      size_t n = 0;
      if (char *s = port.reader->next()) {
        std::istringstream ss(s);
        ss >> n;
      } else {
        throw SimException(bad, __LINE__);
      }
      port.reader->end();
      for (size_t j = 0; j < n; ++j) {
        port.param->read();
      }
    }
  }
#else
  void dump(Register &port, Writer *writer, size_t AESL_transaction)
  {
    writer->begin(AESL_transaction);
    std::string &&s { formatData((unsigned char*)port.param, port.width) };
    writer->next(s.data());
    writer->end();
  }

#ifdef USE_BINARY_TV_FILE
  void dump(Memory<Input, Output> &port, Output *writer, size_t AESL_transaction)
  {
    writer->begin(sum(port.depth));
    size_t wbytes = least_nbyte(port.width);
    for (size_t i = 0; i < port.param.size(); ++i) {
      writer->from((unsigned char*)port.param[i], wbytes, port.asize,
                   port.depth[i], 0);
    }
  }

#endif
  void dump(Memory<Reader, Writer> &port, Writer *writer, size_t AESL_transaction)
  {
    writer->begin(AESL_transaction);
    for (size_t i = 0; i < port.param.size(); ++i) {
      for (size_t j = 0; j < port.depth[i]; ++j) {
        std::string &&s {
          formatData((unsigned char*)port.param[i]+j*port.asize, port.width)
        };
        writer->next(s.data());
      }
      if (port.hbm) {
        break;
      }
    }
    writer->end();
  }

  void dump(FIFO &port, Writer *writer, size_t AESL_transaction)
  {
    writer->begin(AESL_transaction);
    for (size_t j = 0; j < port.depth; ++j) {
      std::string &&s {
        formatData((unsigned char*)port.param+j*port.asize, port.width)
      };
      writer->next(s.data());
    }
    writer->end();
  }


  template<typename E>
  void dump(Stream<E> &port, size_t AESL_transaction)
  {
    if (port.hasWrite) {
      port.writer->begin(AESL_transaction);
      port.depth = port.param->size()-port.initSize;
      for (size_t j = 0; j < port.depth; ++j) {
        std::string &&s {
          formatData((unsigned char*)&port.buf[port.initSize+j], port.width)
        };
        port.writer->next(s.c_str());
      }
      port.writer->end();

      port.swriter->begin(AESL_transaction);
      port.swriter->next(std::to_string(port.depth).c_str());
      port.swriter->end();
    } else {
      port.writer->begin(AESL_transaction);
      port.depth = port.initSize-port.param->size();
      for (size_t j = 0; j < port.depth; ++j) {
        std::string &&s {
          formatData((unsigned char*)&port.buf[j], port.width)
        };
        port.writer->next(s.c_str());
      }
      port.writer->end();

      port.swriter->begin(AESL_transaction);
      port.swriter->next(std::to_string(port.depth).c_str());
      port.swriter->end();

      port.gwriter->begin(AESL_transaction);
      size_t n = (port.depth ? port.initSize : port.depth);
      size_t d = port.depth;
      do {
        port.gwriter->next(std::to_string(n--).c_str());
      } while (d--);
      port.gwriter->end();
    }
  }
#endif
}



extern "C"
void pma_insert_edge_standalone_hw_stub_wrapper(void*, void*, void*, void*, void*, void*, void*, void*, hls::sim::Byte<4>, hls::sim::Byte<4>, hls::sim::Byte<4>, hls::sim::Byte<4>, hls::sim::Byte<4>, void*, void*, void*, void*, void*, void*, void*, void*, void*, void*, void*, void*, void*);

extern "C"
void apatb_pma_insert_edge_standalone_hw(void* __xlx_apatb_param_hbm_gate, void* __xlx_apatb_param_hbm_bs, void* __xlx_apatb_param_hbm_rw, void* __xlx_apatb_param_hbm_red, void* __xlx_apatb_param_hbm_buffer_for_red, void* __xlx_apatb_param_hbm_merge, void* __xlx_apatb_param_hbm_merge_buffer, void* __xlx_apatb_param_task_buffer, hls::sim::Byte<4> __xlx_apatb_param_pma_raw_data_start, hls::sim::Byte<4> __xlx_apatb_param_pma_vecinfo_start, hls::sim::Byte<4> __xlx_apatb_param_edge_count, hls::sim::Byte<4> __xlx_apatb_param_worker_id, hls::sim::Byte<4> __xlx_apatb_param_bitmap_offset, void* __xlx_apatb_param_alloc_cmd_stream_V_data_V, void* __xlx_apatb_param_alloc_cmd_stream_V_keep_V, void* __xlx_apatb_param_alloc_cmd_stream_V_strb_V, void* __xlx_apatb_param_alloc_cmd_stream_V_last_V, void* __xlx_apatb_param_alloc_rsp_stream_V_data_V, void* __xlx_apatb_param_alloc_rsp_stream_V_keep_V, void* __xlx_apatb_param_alloc_rsp_stream_V_strb_V, void* __xlx_apatb_param_alloc_rsp_stream_V_last_V, void* __xlx_apatb_param_bitmap_init_V_data_V, void* __xlx_apatb_param_bitmap_init_V_keep_V, void* __xlx_apatb_param_bitmap_init_V_strb_V, void* __xlx_apatb_param_bitmap_init_V_last_V, void* __xlx_apatb_param_online_in)
{
  hls::sim::Byte<4> __xlx_offset_byte_param_hbm_gate;
  static hls::sim::Register port0 {
    .name = "hbm_gate",
    .width = 32,
#ifdef POST_CHECK
#else
    .owriter = nullptr,
    .iwriter = new hls::sim::Writer(AUTOTB_TVIN_hbm_gate),
#endif
  };
  port0.param = &__xlx_offset_byte_param_hbm_gate;

  hls::sim::Byte<4> __xlx_offset_byte_param_hbm_bs;
  static hls::sim::Register port1 {
    .name = "hbm_bs",
    .width = 32,
#ifdef POST_CHECK
#else
    .owriter = nullptr,
    .iwriter = new hls::sim::Writer(AUTOTB_TVIN_hbm_bs),
#endif
  };
  port1.param = &__xlx_offset_byte_param_hbm_bs;

  hls::sim::Byte<4> __xlx_offset_byte_param_hbm_rw;
  static hls::sim::Register port2 {
    .name = "hbm_rw",
    .width = 32,
#ifdef POST_CHECK
#else
    .owriter = nullptr,
    .iwriter = new hls::sim::Writer(AUTOTB_TVIN_hbm_rw),
#endif
  };
  port2.param = &__xlx_offset_byte_param_hbm_rw;

  hls::sim::Byte<4> __xlx_offset_byte_param_hbm_red;
  static hls::sim::Register port3 {
    .name = "hbm_red",
    .width = 32,
#ifdef POST_CHECK
#else
    .owriter = nullptr,
    .iwriter = new hls::sim::Writer(AUTOTB_TVIN_hbm_red),
#endif
  };
  port3.param = &__xlx_offset_byte_param_hbm_red;

  hls::sim::Byte<4> __xlx_offset_byte_param_hbm_buffer_for_red;
  static hls::sim::Register port4 {
    .name = "hbm_buffer_for_red",
    .width = 32,
#ifdef POST_CHECK
#else
    .owriter = nullptr,
    .iwriter = new hls::sim::Writer(AUTOTB_TVIN_hbm_buffer_for_red),
#endif
  };
  port4.param = &__xlx_offset_byte_param_hbm_buffer_for_red;

  hls::sim::Byte<4> __xlx_offset_byte_param_hbm_merge;
  static hls::sim::Register port5 {
    .name = "hbm_merge",
    .width = 32,
#ifdef POST_CHECK
#else
    .owriter = nullptr,
    .iwriter = new hls::sim::Writer(AUTOTB_TVIN_hbm_merge),
#endif
  };
  port5.param = &__xlx_offset_byte_param_hbm_merge;

  hls::sim::Byte<4> __xlx_offset_byte_param_hbm_merge_buffer;
  static hls::sim::Register port6 {
    .name = "hbm_merge_buffer",
    .width = 32,
#ifdef POST_CHECK
#else
    .owriter = nullptr,
    .iwriter = new hls::sim::Writer(AUTOTB_TVIN_hbm_merge_buffer),
#endif
  };
  port6.param = &__xlx_offset_byte_param_hbm_merge_buffer;

  hls::sim::Byte<4> __xlx_offset_byte_param_task_buffer;
  static hls::sim::Register port7 {
    .name = "task_buffer",
    .width = 32,
#ifdef POST_CHECK
#else
    .owriter = nullptr,
    .iwriter = new hls::sim::Writer(AUTOTB_TVIN_task_buffer),
#endif
  };
  port7.param = &__xlx_offset_byte_param_task_buffer;

  static hls::sim::Register port8 {
    .name = "pma_raw_data_start",
    .width = 32,
#ifdef POST_CHECK
#else
    .owriter = nullptr,
    .iwriter = new hls::sim::Writer(AUTOTB_TVIN_pma_raw_data_start),
#endif
  };
  port8.param = &__xlx_apatb_param_pma_raw_data_start;

  static hls::sim::Register port9 {
    .name = "pma_vecinfo_start",
    .width = 32,
#ifdef POST_CHECK
#else
    .owriter = nullptr,
    .iwriter = new hls::sim::Writer(AUTOTB_TVIN_pma_vecinfo_start),
#endif
  };
  port9.param = &__xlx_apatb_param_pma_vecinfo_start;

  static hls::sim::Register port10 {
    .name = "edge_count",
    .width = 32,
#ifdef POST_CHECK
#else
    .owriter = nullptr,
    .iwriter = new hls::sim::Writer(AUTOTB_TVIN_edge_count),
#endif
  };
  port10.param = &__xlx_apatb_param_edge_count;

  static hls::sim::Register port11 {
    .name = "worker_id",
    .width = 32,
#ifdef POST_CHECK
#else
    .owriter = nullptr,
    .iwriter = new hls::sim::Writer(AUTOTB_TVIN_worker_id),
#endif
  };
  port11.param = &__xlx_apatb_param_worker_id;

  static hls::sim::Register port12 {
    .name = "bitmap_offset",
    .width = 32,
#ifdef POST_CHECK
#else
    .owriter = nullptr,
    .iwriter = new hls::sim::Writer(AUTOTB_TVIN_bitmap_offset),
#endif
  };
  port12.param = &__xlx_apatb_param_bitmap_offset;

  static hls::sim::Stream<hls::sim::Byte<16>> port13 {
    .width = 96,
    .name = "alloc_cmd_stream_V_data_V",
#ifdef POST_CHECK
    .reader = new hls::sim::Reader(AUTOTB_TVOUT_PC_alloc_cmd_stream_V_data_V),
#else
    .writer = new hls::sim::Writer(AUTOTB_TVOUT_alloc_cmd_stream_V_data_V),
    .swriter = new hls::sim::Writer(WRAPC_STREAM_SIZE_OUT_alloc_cmd_stream_V_data_V),
    .gwriter = new hls::sim::Writer(WRAPC_STREAM_EGRESS_STATUS_alloc_cmd_stream_V_data_V),
#endif
  };
  port13.param = (hls::stream<hls::sim::Byte<16>>*)__xlx_apatb_param_alloc_cmd_stream_V_data_V;
  port13.hasWrite = true;

  static hls::sim::Stream<hls::sim::Byte<2>> port14 {
    .width = 12,
    .name = "alloc_cmd_stream_V_keep_V",
#ifdef POST_CHECK
    .reader = new hls::sim::Reader(AUTOTB_TVOUT_PC_alloc_cmd_stream_V_keep_V),
#else
    .writer = new hls::sim::Writer(AUTOTB_TVOUT_alloc_cmd_stream_V_keep_V),
    .swriter = new hls::sim::Writer(WRAPC_STREAM_SIZE_OUT_alloc_cmd_stream_V_keep_V),
    .gwriter = new hls::sim::Writer(WRAPC_STREAM_EGRESS_STATUS_alloc_cmd_stream_V_keep_V),
#endif
  };
  port14.param = (hls::stream<hls::sim::Byte<2>>*)__xlx_apatb_param_alloc_cmd_stream_V_keep_V;
  port14.hasWrite = true;

  static hls::sim::Stream<hls::sim::Byte<2>> port15 {
    .width = 12,
    .name = "alloc_cmd_stream_V_strb_V",
#ifdef POST_CHECK
    .reader = new hls::sim::Reader(AUTOTB_TVOUT_PC_alloc_cmd_stream_V_strb_V),
#else
    .writer = new hls::sim::Writer(AUTOTB_TVOUT_alloc_cmd_stream_V_strb_V),
    .swriter = new hls::sim::Writer(WRAPC_STREAM_SIZE_OUT_alloc_cmd_stream_V_strb_V),
    .gwriter = new hls::sim::Writer(WRAPC_STREAM_EGRESS_STATUS_alloc_cmd_stream_V_strb_V),
#endif
  };
  port15.param = (hls::stream<hls::sim::Byte<2>>*)__xlx_apatb_param_alloc_cmd_stream_V_strb_V;
  port15.hasWrite = true;

  static hls::sim::Stream<hls::sim::Byte<1>> port16 {
    .width = 1,
    .name = "alloc_cmd_stream_V_last_V",
#ifdef POST_CHECK
    .reader = new hls::sim::Reader(AUTOTB_TVOUT_PC_alloc_cmd_stream_V_last_V),
#else
    .writer = new hls::sim::Writer(AUTOTB_TVOUT_alloc_cmd_stream_V_last_V),
    .swriter = new hls::sim::Writer(WRAPC_STREAM_SIZE_OUT_alloc_cmd_stream_V_last_V),
    .gwriter = new hls::sim::Writer(WRAPC_STREAM_EGRESS_STATUS_alloc_cmd_stream_V_last_V),
#endif
  };
  port16.param = (hls::stream<hls::sim::Byte<1>>*)__xlx_apatb_param_alloc_cmd_stream_V_last_V;
  port16.hasWrite = true;

  static hls::sim::Stream<hls::sim::Byte<8>> port17 {
    .width = 64,
    .name = "alloc_rsp_stream_V_data_V",
#ifdef POST_CHECK
    .reader = new hls::sim::Reader(WRAPC_STREAM_SIZE_IN_alloc_rsp_stream_V_data_V),
#else
    .writer = new hls::sim::Writer(AUTOTB_TVIN_alloc_rsp_stream_V_data_V),
    .swriter = new hls::sim::Writer(WRAPC_STREAM_SIZE_IN_alloc_rsp_stream_V_data_V),
    .gwriter = new hls::sim::Writer(WRAPC_STREAM_INGRESS_STATUS_alloc_rsp_stream_V_data_V),
#endif
  };
  port17.param = (hls::stream<hls::sim::Byte<8>>*)__xlx_apatb_param_alloc_rsp_stream_V_data_V;
  port17.hasWrite = false;

  static hls::sim::Stream<hls::sim::Byte<1>> port18 {
    .width = 8,
    .name = "alloc_rsp_stream_V_keep_V",
#ifdef POST_CHECK
    .reader = new hls::sim::Reader(WRAPC_STREAM_SIZE_IN_alloc_rsp_stream_V_keep_V),
#else
    .writer = new hls::sim::Writer(AUTOTB_TVIN_alloc_rsp_stream_V_keep_V),
    .swriter = new hls::sim::Writer(WRAPC_STREAM_SIZE_IN_alloc_rsp_stream_V_keep_V),
    .gwriter = new hls::sim::Writer(WRAPC_STREAM_INGRESS_STATUS_alloc_rsp_stream_V_keep_V),
#endif
  };
  port18.param = (hls::stream<hls::sim::Byte<1>>*)__xlx_apatb_param_alloc_rsp_stream_V_keep_V;
  port18.hasWrite = false;

  static hls::sim::Stream<hls::sim::Byte<1>> port19 {
    .width = 8,
    .name = "alloc_rsp_stream_V_strb_V",
#ifdef POST_CHECK
    .reader = new hls::sim::Reader(WRAPC_STREAM_SIZE_IN_alloc_rsp_stream_V_strb_V),
#else
    .writer = new hls::sim::Writer(AUTOTB_TVIN_alloc_rsp_stream_V_strb_V),
    .swriter = new hls::sim::Writer(WRAPC_STREAM_SIZE_IN_alloc_rsp_stream_V_strb_V),
    .gwriter = new hls::sim::Writer(WRAPC_STREAM_INGRESS_STATUS_alloc_rsp_stream_V_strb_V),
#endif
  };
  port19.param = (hls::stream<hls::sim::Byte<1>>*)__xlx_apatb_param_alloc_rsp_stream_V_strb_V;
  port19.hasWrite = false;

  static hls::sim::Stream<hls::sim::Byte<1>> port20 {
    .width = 1,
    .name = "alloc_rsp_stream_V_last_V",
#ifdef POST_CHECK
    .reader = new hls::sim::Reader(WRAPC_STREAM_SIZE_IN_alloc_rsp_stream_V_last_V),
#else
    .writer = new hls::sim::Writer(AUTOTB_TVIN_alloc_rsp_stream_V_last_V),
    .swriter = new hls::sim::Writer(WRAPC_STREAM_SIZE_IN_alloc_rsp_stream_V_last_V),
    .gwriter = new hls::sim::Writer(WRAPC_STREAM_INGRESS_STATUS_alloc_rsp_stream_V_last_V),
#endif
  };
  port20.param = (hls::stream<hls::sim::Byte<1>>*)__xlx_apatb_param_alloc_rsp_stream_V_last_V;
  port20.hasWrite = false;

  static hls::sim::Stream<hls::sim::Byte<64>> port21 {
    .width = 512,
    .name = "bitmap_init_V_data_V",
#ifdef POST_CHECK
    .reader = new hls::sim::Reader(AUTOTB_TVOUT_PC_bitmap_init_V_data_V),
#else
    .writer = new hls::sim::Writer(AUTOTB_TVOUT_bitmap_init_V_data_V),
    .swriter = new hls::sim::Writer(WRAPC_STREAM_SIZE_OUT_bitmap_init_V_data_V),
    .gwriter = new hls::sim::Writer(WRAPC_STREAM_EGRESS_STATUS_bitmap_init_V_data_V),
#endif
  };
  port21.param = (hls::stream<hls::sim::Byte<64>>*)__xlx_apatb_param_bitmap_init_V_data_V;
  port21.hasWrite = true;

  static hls::sim::Stream<hls::sim::Byte<8>> port22 {
    .width = 64,
    .name = "bitmap_init_V_keep_V",
#ifdef POST_CHECK
    .reader = new hls::sim::Reader(AUTOTB_TVOUT_PC_bitmap_init_V_keep_V),
#else
    .writer = new hls::sim::Writer(AUTOTB_TVOUT_bitmap_init_V_keep_V),
    .swriter = new hls::sim::Writer(WRAPC_STREAM_SIZE_OUT_bitmap_init_V_keep_V),
    .gwriter = new hls::sim::Writer(WRAPC_STREAM_EGRESS_STATUS_bitmap_init_V_keep_V),
#endif
  };
  port22.param = (hls::stream<hls::sim::Byte<8>>*)__xlx_apatb_param_bitmap_init_V_keep_V;
  port22.hasWrite = true;

  static hls::sim::Stream<hls::sim::Byte<8>> port23 {
    .width = 64,
    .name = "bitmap_init_V_strb_V",
#ifdef POST_CHECK
    .reader = new hls::sim::Reader(AUTOTB_TVOUT_PC_bitmap_init_V_strb_V),
#else
    .writer = new hls::sim::Writer(AUTOTB_TVOUT_bitmap_init_V_strb_V),
    .swriter = new hls::sim::Writer(WRAPC_STREAM_SIZE_OUT_bitmap_init_V_strb_V),
    .gwriter = new hls::sim::Writer(WRAPC_STREAM_EGRESS_STATUS_bitmap_init_V_strb_V),
#endif
  };
  port23.param = (hls::stream<hls::sim::Byte<8>>*)__xlx_apatb_param_bitmap_init_V_strb_V;
  port23.hasWrite = true;

  static hls::sim::Stream<hls::sim::Byte<1>> port24 {
    .width = 1,
    .name = "bitmap_init_V_last_V",
#ifdef POST_CHECK
    .reader = new hls::sim::Reader(AUTOTB_TVOUT_PC_bitmap_init_V_last_V),
#else
    .writer = new hls::sim::Writer(AUTOTB_TVOUT_bitmap_init_V_last_V),
    .swriter = new hls::sim::Writer(WRAPC_STREAM_SIZE_OUT_bitmap_init_V_last_V),
    .gwriter = new hls::sim::Writer(WRAPC_STREAM_EGRESS_STATUS_bitmap_init_V_last_V),
#endif
  };
  port24.param = (hls::stream<hls::sim::Byte<1>>*)__xlx_apatb_param_bitmap_init_V_last_V;
  port24.hasWrite = true;

  static hls::sim::Stream<hls::sim::Byte<16>> port25 {
    .width = 128,
    .name = "online_in",
#ifdef POST_CHECK
    .reader = new hls::sim::Reader(WRAPC_STREAM_SIZE_IN_online_in),
#else
    .writer = new hls::sim::Writer(AUTOTB_TVIN_online_in),
    .swriter = new hls::sim::Writer(WRAPC_STREAM_SIZE_IN_online_in),
    .gwriter = new hls::sim::Writer(WRAPC_STREAM_INGRESS_STATUS_online_in),
#endif
  };
  port25.param = (hls::stream<hls::sim::Byte<16>>*)__xlx_apatb_param_online_in;
  port25.hasWrite = false;

  static hls::sim::Memory<hls::sim::Reader, hls::sim::Writer> port26 {
    .width = 512,
    .asize = 64,
    .hbm = false,
    .name = { "gmem0" },
#ifdef POST_CHECK
#else
    .owriter = nullptr,
    .iwriter = new hls::sim::Writer(AUTOTB_TVIN_gmem0),
#endif
  };
  port26.param = { __xlx_apatb_param_hbm_bs };
  port26.depth = { 1 };
  port26.offset = {  };
  port26.hasWrite = { false };

  static hls::sim::Memory<hls::sim::Reader, hls::sim::Writer> port27 {
    .width = 512,
    .asize = 64,
    .hbm = false,
    .name = { "gmem10" },
#ifdef POST_CHECK
    .reader = new hls::sim::Reader(AUTOTB_TVOUT_PC_gmem10),
#else
    .owriter = new hls::sim::Writer(AUTOTB_TVOUT_gmem10),
    .iwriter = new hls::sim::Writer(AUTOTB_TVIN_gmem10),
#endif
  };
  port27.param = { __xlx_apatb_param_hbm_merge_buffer };
  port27.depth = { 1 };
  port27.offset = {  };
  port27.hasWrite = { true };

  static hls::sim::Memory<hls::sim::Reader, hls::sim::Writer> port28 {
    .width = 512,
    .asize = 64,
    .hbm = false,
    .name = { "gmem4" },
#ifdef POST_CHECK
    .reader = new hls::sim::Reader(AUTOTB_TVOUT_PC_gmem4),
#else
    .owriter = new hls::sim::Writer(AUTOTB_TVOUT_gmem4),
    .iwriter = new hls::sim::Writer(AUTOTB_TVIN_gmem4),
#endif
  };
  port28.param = { __xlx_apatb_param_hbm_rw };
  port28.depth = { 1 };
  port28.offset = {  };
  port28.hasWrite = { true };

  static hls::sim::Memory<hls::sim::Reader, hls::sim::Writer> port29 {
    .width = 512,
    .asize = 64,
    .hbm = false,
    .name = { "gmem5" },
#ifdef POST_CHECK
    .reader = new hls::sim::Reader(AUTOTB_TVOUT_PC_gmem5),
#else
    .owriter = new hls::sim::Writer(AUTOTB_TVOUT_gmem5),
    .iwriter = new hls::sim::Writer(AUTOTB_TVIN_gmem5),
#endif
  };
  port29.param = { __xlx_apatb_param_hbm_red };
  port29.depth = { 1 };
  port29.offset = {  };
  port29.hasWrite = { true };

  static hls::sim::Memory<hls::sim::Reader, hls::sim::Writer> port30 {
    .width = 512,
    .asize = 64,
    .hbm = false,
    .name = { "gmem6" },
#ifdef POST_CHECK
#else
    .owriter = nullptr,
    .iwriter = new hls::sim::Writer(AUTOTB_TVIN_gmem6),
#endif
  };
  port30.param = { __xlx_apatb_param_hbm_gate };
  port30.depth = { 1 };
  port30.offset = {  };
  port30.hasWrite = { false };

  static hls::sim::Memory<hls::sim::Reader, hls::sim::Writer> port31 {
    .width = 32,
    .asize = 4,
    .hbm = false,
    .name = { "gmem7" },
#ifdef POST_CHECK
    .reader = new hls::sim::Reader(AUTOTB_TVOUT_PC_gmem7),
#else
    .owriter = new hls::sim::Writer(AUTOTB_TVOUT_gmem7),
    .iwriter = new hls::sim::Writer(AUTOTB_TVIN_gmem7),
#endif
  };
  port31.param = { __xlx_apatb_param_task_buffer };
  port31.depth = { 1 };
  port31.offset = {  };
  port31.hasWrite = { true };

  static hls::sim::Memory<hls::sim::Reader, hls::sim::Writer> port32 {
    .width = 512,
    .asize = 64,
    .hbm = false,
    .name = { "gmem8" },
#ifdef POST_CHECK
    .reader = new hls::sim::Reader(AUTOTB_TVOUT_PC_gmem8),
#else
    .owriter = new hls::sim::Writer(AUTOTB_TVOUT_gmem8),
    .iwriter = new hls::sim::Writer(AUTOTB_TVIN_gmem8),
#endif
  };
  port32.param = { __xlx_apatb_param_hbm_buffer_for_red };
  port32.depth = { 1 };
  port32.offset = {  };
  port32.hasWrite = { true };

  static hls::sim::Memory<hls::sim::Reader, hls::sim::Writer> port33 {
    .width = 512,
    .asize = 64,
    .hbm = false,
    .name = { "gmem9" },
#ifdef POST_CHECK
    .reader = new hls::sim::Reader(AUTOTB_TVOUT_PC_gmem9),
#else
    .owriter = new hls::sim::Writer(AUTOTB_TVOUT_gmem9),
    .iwriter = new hls::sim::Writer(AUTOTB_TVIN_gmem9),
#endif
  };
  port33.param = { __xlx_apatb_param_hbm_merge };
  port33.depth = { 1 };
  port33.offset = {  };
  port33.hasWrite = { true };

  refine_signal_handler();
  try {
#ifdef POST_CHECK
    CodeState = ENTER_WRAPC_PC;
    check(port27);
    check(port28);
    check(port29);
    check(port31);
    check(port32);
    check(port33);
    check(port13);
    check(port14);
    check(port15);
    check(port16);
    check(port17);
    check(port18);
    check(port19);
    check(port20);
    check(port21);
    check(port22);
    check(port23);
    check(port24);
    check(port25);
#else
    static hls::sim::RefTCL tcl("../tv/cdatafile/ref.tcl");
    CodeState = DUMP_INPUTS;
    dump(port0, port0.iwriter, tcl.AESL_transaction);
    dump(port1, port1.iwriter, tcl.AESL_transaction);
    dump(port2, port2.iwriter, tcl.AESL_transaction);
    dump(port3, port3.iwriter, tcl.AESL_transaction);
    dump(port4, port4.iwriter, tcl.AESL_transaction);
    dump(port5, port5.iwriter, tcl.AESL_transaction);
    dump(port6, port6.iwriter, tcl.AESL_transaction);
    dump(port7, port7.iwriter, tcl.AESL_transaction);
    dump(port8, port8.iwriter, tcl.AESL_transaction);
    dump(port9, port9.iwriter, tcl.AESL_transaction);
    dump(port10, port10.iwriter, tcl.AESL_transaction);
    dump(port11, port11.iwriter, tcl.AESL_transaction);
    dump(port12, port12.iwriter, tcl.AESL_transaction);
    dump(port26, port26.iwriter, tcl.AESL_transaction);
    dump(port27, port27.iwriter, tcl.AESL_transaction);
    dump(port28, port28.iwriter, tcl.AESL_transaction);
    dump(port29, port29.iwriter, tcl.AESL_transaction);
    dump(port30, port30.iwriter, tcl.AESL_transaction);
    dump(port31, port31.iwriter, tcl.AESL_transaction);
    dump(port32, port32.iwriter, tcl.AESL_transaction);
    dump(port33, port33.iwriter, tcl.AESL_transaction);
    port0.doTCL(tcl);
    port1.doTCL(tcl);
    port2.doTCL(tcl);
    port3.doTCL(tcl);
    port4.doTCL(tcl);
    port5.doTCL(tcl);
    port6.doTCL(tcl);
    port7.doTCL(tcl);
    port8.doTCL(tcl);
    port9.doTCL(tcl);
    port10.doTCL(tcl);
    port11.doTCL(tcl);
    port12.doTCL(tcl);
    port26.doTCL(tcl);
    port27.doTCL(tcl);
    port28.doTCL(tcl);
    port29.doTCL(tcl);
    port30.doTCL(tcl);
    port31.doTCL(tcl);
    port32.doTCL(tcl);
    port33.doTCL(tcl);
    port17.markSize();
    port18.markSize();
    port19.markSize();
    port20.markSize();
    port25.markSize();
    port17.buffer();
    port18.buffer();
    port19.buffer();
    port20.buffer();
    port25.buffer();
    port13.markSize();
    port14.markSize();
    port15.markSize();
    port16.markSize();
    port21.markSize();
    port22.markSize();
    port23.markSize();
    port24.markSize();
    CodeState = CALL_C_DUT;
    pma_insert_edge_standalone_hw_stub_wrapper(__xlx_apatb_param_hbm_gate, __xlx_apatb_param_hbm_bs, __xlx_apatb_param_hbm_rw, __xlx_apatb_param_hbm_red, __xlx_apatb_param_hbm_buffer_for_red, __xlx_apatb_param_hbm_merge, __xlx_apatb_param_hbm_merge_buffer, __xlx_apatb_param_task_buffer, __xlx_apatb_param_pma_raw_data_start, __xlx_apatb_param_pma_vecinfo_start, __xlx_apatb_param_edge_count, __xlx_apatb_param_worker_id, __xlx_apatb_param_bitmap_offset, __xlx_apatb_param_alloc_cmd_stream_V_data_V, __xlx_apatb_param_alloc_cmd_stream_V_keep_V, __xlx_apatb_param_alloc_cmd_stream_V_strb_V, __xlx_apatb_param_alloc_cmd_stream_V_last_V, __xlx_apatb_param_alloc_rsp_stream_V_data_V, __xlx_apatb_param_alloc_rsp_stream_V_keep_V, __xlx_apatb_param_alloc_rsp_stream_V_strb_V, __xlx_apatb_param_alloc_rsp_stream_V_last_V, __xlx_apatb_param_bitmap_init_V_data_V, __xlx_apatb_param_bitmap_init_V_keep_V, __xlx_apatb_param_bitmap_init_V_strb_V, __xlx_apatb_param_bitmap_init_V_last_V, __xlx_apatb_param_online_in);
    port13.buffer();
    port14.buffer();
    port15.buffer();
    port16.buffer();
    port21.buffer();
    port22.buffer();
    port23.buffer();
    port24.buffer();
    dump(port17, tcl.AESL_transaction);
    dump(port18, tcl.AESL_transaction);
    dump(port19, tcl.AESL_transaction);
    dump(port20, tcl.AESL_transaction);
    dump(port25, tcl.AESL_transaction);
    port17.doTCL(tcl);
    port18.doTCL(tcl);
    port19.doTCL(tcl);
    port20.doTCL(tcl);
    port25.doTCL(tcl);
    CodeState = DUMP_OUTPUTS;
    dump(port27, port27.owriter, tcl.AESL_transaction);
    dump(port28, port28.owriter, tcl.AESL_transaction);
    dump(port29, port29.owriter, tcl.AESL_transaction);
    dump(port31, port31.owriter, tcl.AESL_transaction);
    dump(port32, port32.owriter, tcl.AESL_transaction);
    dump(port33, port33.owriter, tcl.AESL_transaction);
    dump(port13, tcl.AESL_transaction);
    dump(port14, tcl.AESL_transaction);
    dump(port15, tcl.AESL_transaction);
    dump(port16, tcl.AESL_transaction);
    dump(port21, tcl.AESL_transaction);
    dump(port22, tcl.AESL_transaction);
    dump(port23, tcl.AESL_transaction);
    dump(port24, tcl.AESL_transaction);
    port13.doTCL(tcl);
    port14.doTCL(tcl);
    port15.doTCL(tcl);
    port16.doTCL(tcl);
    port21.doTCL(tcl);
    port22.doTCL(tcl);
    port23.doTCL(tcl);
    port24.doTCL(tcl);
    tcl.AESL_transaction++;
#endif
  } catch (const hls::sim::SimException &e) {
    hls::sim::errExit(e.line, e.msg);
  }
}