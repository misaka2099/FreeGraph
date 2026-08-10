#ifndef IP_BD_85AD_HBM_INST_0_SC_H_
#define IP_BD_85AD_HBM_INST_0_SC_H_

// (c) Copyright 1995-2026 Xilinx, Inc. All rights reserved.
// 
// This file contains confidential and proprietary information
// of Xilinx, Inc. and is protected under U.S. and
// international copyright and other intellectual property
// laws.
// 
// DISCLAIMER
// This disclaimer is not a license and does not grant any
// rights to the materials distributed herewith. Except as
// otherwise provided in a valid license issued to you by
// Xilinx, and to the maximum extent permitted by applicable
// law: (1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND
// WITH ALL FAULTS, AND XILINX HEREBY DISCLAIMS ALL WARRANTIES
// AND CONDITIONS, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
// BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, NON-
// INFRINGEMENT, OR FITNESS FOR ANY PARTICULAR PURPOSE; and
// (2) Xilinx shall not be liable (whether in contract or tort,
// including negligence, or under any other theory of
// liability) for any loss or damage of any kind or nature
// related to, arising under or in connection with these
// materials, including for any direct, or any indirect,
// special, incidental, or consequential loss or damage
// (including loss of data, profits, goodwill, or any type of
// loss or damage suffered as a result of any action brought
// by a third party) even if such damage or loss was
// reasonably foreseeable or Xilinx had been advised of the
// possibility of the same.
// 
// CRITICAL APPLICATIONS
// Xilinx products are not designed or intended to be fail-
// safe, or for use in any application requiring fail-safe
// performance, such as life-support or safety devices or
// systems, Class III medical devices, nuclear facilities,
// applications related to the deployment of airbags, or any
// other applications that could lead to death, personal
// injury, or severe property or environmental damage
// (individually and collectively, "Critical
// Applications"). Customer assumes the sole risk and
// liability of any use of Xilinx products in Critical
// Applications, subject only to applicable laws and
// regulations governing limitations on product liability.
// 
// THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS
// PART OF THIS FILE AT ALL TIMES.
// 
// DO NOT MODIFY THIS FILE.


#ifndef XTLM
#include "xtlm.h"
#endif
#ifndef SYSTEMC_INCLUDED
#include <systemc>
#endif

#if defined(_MSC_VER)
#define DllExport __declspec(dllexport)
#elif defined(__GNUC__)
#define DllExport __attribute__ ((visibility("default")))
#else
#define DllExport
#endif

class hbm_sc;

class DllExport bd_85ad_hbm_inst_0_sc : public sc_core::sc_module
{
public:

  bd_85ad_hbm_inst_0_sc(const sc_core::sc_module_name& nm);
  virtual ~bd_85ad_hbm_inst_0_sc();

  // module socket-to-socket AXI TLM interfaces

  xtlm::xtlm_aximm_target_socket* SAXI_00_rd_socket;
  xtlm::xtlm_aximm_target_socket* SAXI_00_wr_socket;
  xtlm::xtlm_aximm_target_socket* SAXI_01_rd_socket;
  xtlm::xtlm_aximm_target_socket* SAXI_01_wr_socket;
  xtlm::xtlm_aximm_target_socket* SAXI_02_rd_socket;
  xtlm::xtlm_aximm_target_socket* SAXI_02_wr_socket;
  xtlm::xtlm_aximm_target_socket* SAXI_04_rd_socket;
  xtlm::xtlm_aximm_target_socket* SAXI_04_wr_socket;
  xtlm::xtlm_aximm_target_socket* SAXI_05_rd_socket;
  xtlm::xtlm_aximm_target_socket* SAXI_05_wr_socket;
  xtlm::xtlm_aximm_target_socket* SAXI_06_rd_socket;
  xtlm::xtlm_aximm_target_socket* SAXI_06_wr_socket;
  xtlm::xtlm_aximm_target_socket* SAXI_07_rd_socket;
  xtlm::xtlm_aximm_target_socket* SAXI_07_wr_socket;
  xtlm::xtlm_aximm_target_socket* SAXI_08_rd_socket;
  xtlm::xtlm_aximm_target_socket* SAXI_08_wr_socket;
  xtlm::xtlm_aximm_target_socket* SAXI_09_rd_socket;
  xtlm::xtlm_aximm_target_socket* SAXI_09_wr_socket;
  xtlm::xtlm_aximm_target_socket* SAXI_10_rd_socket;
  xtlm::xtlm_aximm_target_socket* SAXI_10_wr_socket;
  xtlm::xtlm_aximm_target_socket* SAXI_11_rd_socket;
  xtlm::xtlm_aximm_target_socket* SAXI_11_wr_socket;
  xtlm::xtlm_aximm_target_socket* SAXI_12_rd_socket;
  xtlm::xtlm_aximm_target_socket* SAXI_12_wr_socket;
  xtlm::xtlm_aximm_target_socket* SAXI_13_rd_socket;
  xtlm::xtlm_aximm_target_socket* SAXI_13_wr_socket;
  xtlm::xtlm_aximm_target_socket* SAXI_14_rd_socket;
  xtlm::xtlm_aximm_target_socket* SAXI_14_wr_socket;
  xtlm::xtlm_aximm_target_socket* SAXI_15_rd_socket;
  xtlm::xtlm_aximm_target_socket* SAXI_15_wr_socket;
  xtlm::xtlm_aximm_target_socket* SAXI_16_rd_socket;
  xtlm::xtlm_aximm_target_socket* SAXI_16_wr_socket;
  xtlm::xtlm_aximm_target_socket* SAXI_17_rd_socket;
  xtlm::xtlm_aximm_target_socket* SAXI_17_wr_socket;
  xtlm::xtlm_aximm_target_socket* SAXI_18_rd_socket;
  xtlm::xtlm_aximm_target_socket* SAXI_18_wr_socket;
  xtlm::xtlm_aximm_target_socket* SAXI_19_rd_socket;
  xtlm::xtlm_aximm_target_socket* SAXI_19_wr_socket;
  xtlm::xtlm_aximm_target_socket* SAXI_20_rd_socket;
  xtlm::xtlm_aximm_target_socket* SAXI_20_wr_socket;
  xtlm::xtlm_aximm_target_socket* SAXI_21_rd_socket;
  xtlm::xtlm_aximm_target_socket* SAXI_21_wr_socket;
  xtlm::xtlm_aximm_target_socket* SAXI_22_rd_socket;
  xtlm::xtlm_aximm_target_socket* SAXI_22_wr_socket;
  xtlm::xtlm_aximm_target_socket* SAXI_23_rd_socket;
  xtlm::xtlm_aximm_target_socket* SAXI_23_wr_socket;
  xtlm::xtlm_aximm_target_socket* SAXI_24_rd_socket;
  xtlm::xtlm_aximm_target_socket* SAXI_24_wr_socket;
  xtlm::xtlm_aximm_target_socket* SAXI_25_rd_socket;
  xtlm::xtlm_aximm_target_socket* SAXI_25_wr_socket;
  xtlm::xtlm_aximm_target_socket* SAXI_26_rd_socket;
  xtlm::xtlm_aximm_target_socket* SAXI_26_wr_socket;
  xtlm::xtlm_aximm_target_socket* SAXI_27_rd_socket;
  xtlm::xtlm_aximm_target_socket* SAXI_27_wr_socket;
  xtlm::xtlm_aximm_target_socket* SAXI_28_rd_socket;
  xtlm::xtlm_aximm_target_socket* SAXI_28_wr_socket;
  xtlm::xtlm_aximm_target_socket* SAXI_29_rd_socket;
  xtlm::xtlm_aximm_target_socket* SAXI_29_wr_socket;
  xtlm::xtlm_aximm_target_socket* SAXI_30_rd_socket;
  xtlm::xtlm_aximm_target_socket* SAXI_30_wr_socket;
  xtlm::xtlm_aximm_target_socket* SAXI_31_rd_socket;
  xtlm::xtlm_aximm_target_socket* SAXI_31_wr_socket;

  // module socket-to-socket TLM interfaces


protected:

  hbm_sc* mp_impl;

private:

  bd_85ad_hbm_inst_0_sc(const bd_85ad_hbm_inst_0_sc&);
  const bd_85ad_hbm_inst_0_sc& operator=(const bd_85ad_hbm_inst_0_sc&);

};

#endif // IP_BD_85AD_HBM_INST_0_SC_H_
