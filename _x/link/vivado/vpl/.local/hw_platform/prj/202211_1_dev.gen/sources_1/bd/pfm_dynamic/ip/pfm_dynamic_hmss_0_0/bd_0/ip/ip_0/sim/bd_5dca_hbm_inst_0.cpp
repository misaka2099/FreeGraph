// (c) Copyright 1995-2022 Xilinx, Inc. All rights reserved.
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


#include "bd_5dca_hbm_inst_0_sc.h"

#include "bd_5dca_hbm_inst_0.h"

#include "hbm_sc.h"

#include <map>
#include <string>





#ifdef XILINX_SIMULATOR
bd_5dca_hbm_inst_0::bd_5dca_hbm_inst_0(const sc_core::sc_module_name& nm) : bd_5dca_hbm_inst_0_sc(nm), HBM_REF_CLK_0("HBM_REF_CLK_0"), HBM_REF_CLK_1("HBM_REF_CLK_1"), AXI_12_ACLK("AXI_12_ACLK"), AXI_12_ARESET_N("AXI_12_ARESET_N"), AXI_12_ARADDR("AXI_12_ARADDR"), AXI_12_ARBURST("AXI_12_ARBURST"), AXI_12_ARID("AXI_12_ARID"), AXI_12_ARLEN("AXI_12_ARLEN"), AXI_12_ARSIZE("AXI_12_ARSIZE"), AXI_12_ARVALID("AXI_12_ARVALID"), AXI_12_AWADDR("AXI_12_AWADDR"), AXI_12_AWBURST("AXI_12_AWBURST"), AXI_12_AWID("AXI_12_AWID"), AXI_12_AWLEN("AXI_12_AWLEN"), AXI_12_AWSIZE("AXI_12_AWSIZE"), AXI_12_AWVALID("AXI_12_AWVALID"), AXI_12_RREADY("AXI_12_RREADY"), AXI_12_BREADY("AXI_12_BREADY"), AXI_12_WDATA("AXI_12_WDATA"), AXI_12_WLAST("AXI_12_WLAST"), AXI_12_WSTRB("AXI_12_WSTRB"), AXI_12_WDATA_PARITY("AXI_12_WDATA_PARITY"), AXI_12_WVALID("AXI_12_WVALID"), APB_0_PWDATA("APB_0_PWDATA"), APB_0_PADDR("APB_0_PADDR"), APB_0_PCLK("APB_0_PCLK"), APB_0_PENABLE("APB_0_PENABLE"), APB_0_PRESET_N("APB_0_PRESET_N"), APB_0_PSEL("APB_0_PSEL"), APB_0_PWRITE("APB_0_PWRITE"), APB_1_PWDATA("APB_1_PWDATA"), APB_1_PADDR("APB_1_PADDR"), APB_1_PCLK("APB_1_PCLK"), APB_1_PENABLE("APB_1_PENABLE"), APB_1_PRESET_N("APB_1_PRESET_N"), APB_1_PSEL("APB_1_PSEL"), APB_1_PWRITE("APB_1_PWRITE"), AXI_12_ARREADY("AXI_12_ARREADY"), AXI_12_AWREADY("AXI_12_AWREADY"), AXI_12_RDATA_PARITY("AXI_12_RDATA_PARITY"), AXI_12_RDATA("AXI_12_RDATA"), AXI_12_RID("AXI_12_RID"), AXI_12_RLAST("AXI_12_RLAST"), AXI_12_RRESP("AXI_12_RRESP"), AXI_12_RVALID("AXI_12_RVALID"), AXI_12_WREADY("AXI_12_WREADY"), AXI_12_BID("AXI_12_BID"), AXI_12_BRESP("AXI_12_BRESP"), AXI_12_BVALID("AXI_12_BVALID"), APB_0_PRDATA("APB_0_PRDATA"), APB_0_PREADY("APB_0_PREADY"), APB_0_PSLVERR("APB_0_PSLVERR"), APB_1_PRDATA("APB_1_PRDATA"), APB_1_PREADY("APB_1_PREADY"), APB_1_PSLVERR("APB_1_PSLVERR"), apb_complete_0("apb_complete_0"), apb_complete_1("apb_complete_1"), DRAM_0_STAT_CATTRIP("DRAM_0_STAT_CATTRIP"), DRAM_0_STAT_TEMP("DRAM_0_STAT_TEMP"), DRAM_1_STAT_CATTRIP("DRAM_1_STAT_CATTRIP"), DRAM_1_STAT_TEMP("DRAM_1_STAT_TEMP")
{

  // initialize pins
  mp_impl->HBM_REF_CLK_0(HBM_REF_CLK_0);
  mp_impl->HBM_REF_CLK_1(HBM_REF_CLK_1);
  mp_impl->AXI_12_ACLK(AXI_12_ACLK);
  mp_impl->AXI_12_ARESET_N(AXI_12_ARESET_N);
  mp_impl->AXI_12_WDATA_PARITY(AXI_12_WDATA_PARITY);
  mp_impl->APB_0_PWDATA(APB_0_PWDATA);
  mp_impl->APB_0_PADDR(APB_0_PADDR);
  mp_impl->APB_0_PCLK(APB_0_PCLK);
  mp_impl->APB_0_PENABLE(APB_0_PENABLE);
  mp_impl->APB_0_PRESET_N(APB_0_PRESET_N);
  mp_impl->APB_0_PSEL(APB_0_PSEL);
  mp_impl->APB_0_PWRITE(APB_0_PWRITE);
  mp_impl->APB_1_PWDATA(APB_1_PWDATA);
  mp_impl->APB_1_PADDR(APB_1_PADDR);
  mp_impl->APB_1_PCLK(APB_1_PCLK);
  mp_impl->APB_1_PENABLE(APB_1_PENABLE);
  mp_impl->APB_1_PRESET_N(APB_1_PRESET_N);
  mp_impl->APB_1_PSEL(APB_1_PSEL);
  mp_impl->APB_1_PWRITE(APB_1_PWRITE);
  mp_impl->AXI_12_RDATA_PARITY(AXI_12_RDATA_PARITY);
  mp_impl->APB_0_PRDATA(APB_0_PRDATA);
  mp_impl->APB_0_PREADY(APB_0_PREADY);
  mp_impl->APB_0_PSLVERR(APB_0_PSLVERR);
  mp_impl->APB_1_PRDATA(APB_1_PRDATA);
  mp_impl->APB_1_PREADY(APB_1_PREADY);
  mp_impl->APB_1_PSLVERR(APB_1_PSLVERR);
  mp_impl->apb_complete_0(apb_complete_0);
  mp_impl->apb_complete_1(apb_complete_1);
  mp_impl->DRAM_0_STAT_CATTRIP(DRAM_0_STAT_CATTRIP);
  mp_impl->DRAM_0_STAT_TEMP(DRAM_0_STAT_TEMP);
  mp_impl->DRAM_1_STAT_CATTRIP(DRAM_1_STAT_CATTRIP);
  mp_impl->DRAM_1_STAT_TEMP(DRAM_1_STAT_TEMP);

  // initialize transactors
  mp_SAXI_12_transactor = NULL;
  mp_AXI_12_ARLEN_converter = NULL;
  mp_AXI_12_AWLEN_converter = NULL;

  // initialize socket stubs

}

void bd_5dca_hbm_inst_0::before_end_of_elaboration()
{
  // configure 'SAXI_12' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("bd_5dca_hbm_inst_0", "SAXI_12_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'SAXI_12' transactor parameters
    xsc::common_cpp::properties SAXI_12_transactor_param_props;
    SAXI_12_transactor_param_props.addLong("DATA_WIDTH", "256");
    SAXI_12_transactor_param_props.addLong("FREQ_HZ", "450000000");
    SAXI_12_transactor_param_props.addLong("ID_WIDTH", "6");
    SAXI_12_transactor_param_props.addLong("ADDR_WIDTH", "33");
    SAXI_12_transactor_param_props.addLong("AWUSER_WIDTH", "0");
    SAXI_12_transactor_param_props.addLong("ARUSER_WIDTH", "0");
    SAXI_12_transactor_param_props.addLong("WUSER_WIDTH", "0");
    SAXI_12_transactor_param_props.addLong("RUSER_WIDTH", "0");
    SAXI_12_transactor_param_props.addLong("BUSER_WIDTH", "0");
    SAXI_12_transactor_param_props.addLong("HAS_BURST", "1");
    SAXI_12_transactor_param_props.addLong("HAS_LOCK", "0");
    SAXI_12_transactor_param_props.addLong("HAS_PROT", "0");
    SAXI_12_transactor_param_props.addLong("HAS_CACHE", "0");
    SAXI_12_transactor_param_props.addLong("HAS_QOS", "0");
    SAXI_12_transactor_param_props.addLong("HAS_REGION", "0");
    SAXI_12_transactor_param_props.addLong("HAS_WSTRB", "1");
    SAXI_12_transactor_param_props.addLong("HAS_BRESP", "1");
    SAXI_12_transactor_param_props.addLong("HAS_RRESP", "1");
    SAXI_12_transactor_param_props.addLong("SUPPORTS_NARROW_BURST", "1");
    SAXI_12_transactor_param_props.addLong("NUM_READ_OUTSTANDING", "2");
    SAXI_12_transactor_param_props.addLong("NUM_WRITE_OUTSTANDING", "2");
    SAXI_12_transactor_param_props.addLong("MAX_BURST_LENGTH", "16");
    SAXI_12_transactor_param_props.addLong("NUM_READ_THREADS", "1");
    SAXI_12_transactor_param_props.addLong("NUM_WRITE_THREADS", "1");
    SAXI_12_transactor_param_props.addLong("RUSER_BITS_PER_BYTE", "0");
    SAXI_12_transactor_param_props.addLong("WUSER_BITS_PER_BYTE", "0");
    SAXI_12_transactor_param_props.addLong("HAS_SIZE", "1");
    SAXI_12_transactor_param_props.addLong("HAS_RESET", "0");
    SAXI_12_transactor_param_props.addFloat("PHASE", "0.0");
    SAXI_12_transactor_param_props.addString("PROTOCOL", "AXI3");
    SAXI_12_transactor_param_props.addString("READ_WRITE_MODE", "READ_WRITE");
    SAXI_12_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_hbm_aclk_1_0_clk");
    SAXI_12_transactor_param_props.addString("SLR_ASSIGNMENT", "");

    mp_SAXI_12_transactor = new xtlm::xaximm_pin2xtlm_t<256,33,6,1,1,1,1,1>("SAXI_12_transactor", SAXI_12_transactor_param_props);

    // SAXI_12' transactor ports

    mp_SAXI_12_transactor->ARADDR(AXI_12_ARADDR);
    mp_SAXI_12_transactor->ARBURST(AXI_12_ARBURST);
    mp_SAXI_12_transactor->ARID(AXI_12_ARID);
    mp_AXI_12_ARLEN_converter = new xsc::common::vector2vector_converter<4,8>("AXI_12_ARLEN_converter");
    mp_AXI_12_ARLEN_converter->vector_in(AXI_12_ARLEN);
    mp_AXI_12_ARLEN_converter->vector_out(m_AXI_12_ARLEN_converter_signal);
    mp_SAXI_12_transactor->ARLEN(m_AXI_12_ARLEN_converter_signal);
    mp_SAXI_12_transactor->ARREADY(AXI_12_ARREADY);
    mp_SAXI_12_transactor->ARSIZE(AXI_12_ARSIZE);
    mp_SAXI_12_transactor->ARVALID(AXI_12_ARVALID);
    mp_SAXI_12_transactor->AWADDR(AXI_12_AWADDR);
    mp_SAXI_12_transactor->AWBURST(AXI_12_AWBURST);
    mp_SAXI_12_transactor->AWID(AXI_12_AWID);
    mp_AXI_12_AWLEN_converter = new xsc::common::vector2vector_converter<4,8>("AXI_12_AWLEN_converter");
    mp_AXI_12_AWLEN_converter->vector_in(AXI_12_AWLEN);
    mp_AXI_12_AWLEN_converter->vector_out(m_AXI_12_AWLEN_converter_signal);
    mp_SAXI_12_transactor->AWLEN(m_AXI_12_AWLEN_converter_signal);
    mp_SAXI_12_transactor->AWREADY(AXI_12_AWREADY);
    mp_SAXI_12_transactor->AWSIZE(AXI_12_AWSIZE);
    mp_SAXI_12_transactor->AWVALID(AXI_12_AWVALID);
    mp_SAXI_12_transactor->BID(AXI_12_BID);
    mp_SAXI_12_transactor->BREADY(AXI_12_BREADY);
    mp_SAXI_12_transactor->BRESP(AXI_12_BRESP);
    mp_SAXI_12_transactor->BVALID(AXI_12_BVALID);
    mp_SAXI_12_transactor->RDATA(AXI_12_RDATA);
    mp_SAXI_12_transactor->RID(AXI_12_RID);
    mp_SAXI_12_transactor->RLAST(AXI_12_RLAST);
    mp_SAXI_12_transactor->RREADY(AXI_12_RREADY);
    mp_SAXI_12_transactor->RRESP(AXI_12_RRESP);
    mp_SAXI_12_transactor->RVALID(AXI_12_RVALID);
    mp_SAXI_12_transactor->WDATA(AXI_12_WDATA);
    mp_SAXI_12_transactor->WLAST(AXI_12_WLAST);
    mp_SAXI_12_transactor->WREADY(AXI_12_WREADY);
    mp_SAXI_12_transactor->WSTRB(AXI_12_WSTRB);
    mp_SAXI_12_transactor->WVALID(AXI_12_WVALID);
    mp_SAXI_12_transactor->CLK(AXI_12_ACLK);
    m_SAXI_12_transactor_rst_signal.write(1);
    mp_SAXI_12_transactor->RST(m_SAXI_12_transactor_rst_signal);

    // SAXI_12' transactor sockets

    mp_impl->SAXI_12_rd_socket->bind(*(mp_SAXI_12_transactor->rd_socket));
    mp_impl->SAXI_12_wr_socket->bind(*(mp_SAXI_12_transactor->wr_socket));
  }
  else
  {
  }

}

#endif // XILINX_SIMULATOR




#ifdef XM_SYSTEMC
bd_5dca_hbm_inst_0::bd_5dca_hbm_inst_0(const sc_core::sc_module_name& nm) : bd_5dca_hbm_inst_0_sc(nm), HBM_REF_CLK_0("HBM_REF_CLK_0"), HBM_REF_CLK_1("HBM_REF_CLK_1"), AXI_12_ACLK("AXI_12_ACLK"), AXI_12_ARESET_N("AXI_12_ARESET_N"), AXI_12_ARADDR("AXI_12_ARADDR"), AXI_12_ARBURST("AXI_12_ARBURST"), AXI_12_ARID("AXI_12_ARID"), AXI_12_ARLEN("AXI_12_ARLEN"), AXI_12_ARSIZE("AXI_12_ARSIZE"), AXI_12_ARVALID("AXI_12_ARVALID"), AXI_12_AWADDR("AXI_12_AWADDR"), AXI_12_AWBURST("AXI_12_AWBURST"), AXI_12_AWID("AXI_12_AWID"), AXI_12_AWLEN("AXI_12_AWLEN"), AXI_12_AWSIZE("AXI_12_AWSIZE"), AXI_12_AWVALID("AXI_12_AWVALID"), AXI_12_RREADY("AXI_12_RREADY"), AXI_12_BREADY("AXI_12_BREADY"), AXI_12_WDATA("AXI_12_WDATA"), AXI_12_WLAST("AXI_12_WLAST"), AXI_12_WSTRB("AXI_12_WSTRB"), AXI_12_WDATA_PARITY("AXI_12_WDATA_PARITY"), AXI_12_WVALID("AXI_12_WVALID"), APB_0_PWDATA("APB_0_PWDATA"), APB_0_PADDR("APB_0_PADDR"), APB_0_PCLK("APB_0_PCLK"), APB_0_PENABLE("APB_0_PENABLE"), APB_0_PRESET_N("APB_0_PRESET_N"), APB_0_PSEL("APB_0_PSEL"), APB_0_PWRITE("APB_0_PWRITE"), APB_1_PWDATA("APB_1_PWDATA"), APB_1_PADDR("APB_1_PADDR"), APB_1_PCLK("APB_1_PCLK"), APB_1_PENABLE("APB_1_PENABLE"), APB_1_PRESET_N("APB_1_PRESET_N"), APB_1_PSEL("APB_1_PSEL"), APB_1_PWRITE("APB_1_PWRITE"), AXI_12_ARREADY("AXI_12_ARREADY"), AXI_12_AWREADY("AXI_12_AWREADY"), AXI_12_RDATA_PARITY("AXI_12_RDATA_PARITY"), AXI_12_RDATA("AXI_12_RDATA"), AXI_12_RID("AXI_12_RID"), AXI_12_RLAST("AXI_12_RLAST"), AXI_12_RRESP("AXI_12_RRESP"), AXI_12_RVALID("AXI_12_RVALID"), AXI_12_WREADY("AXI_12_WREADY"), AXI_12_BID("AXI_12_BID"), AXI_12_BRESP("AXI_12_BRESP"), AXI_12_BVALID("AXI_12_BVALID"), APB_0_PRDATA("APB_0_PRDATA"), APB_0_PREADY("APB_0_PREADY"), APB_0_PSLVERR("APB_0_PSLVERR"), APB_1_PRDATA("APB_1_PRDATA"), APB_1_PREADY("APB_1_PREADY"), APB_1_PSLVERR("APB_1_PSLVERR"), apb_complete_0("apb_complete_0"), apb_complete_1("apb_complete_1"), DRAM_0_STAT_CATTRIP("DRAM_0_STAT_CATTRIP"), DRAM_0_STAT_TEMP("DRAM_0_STAT_TEMP"), DRAM_1_STAT_CATTRIP("DRAM_1_STAT_CATTRIP"), DRAM_1_STAT_TEMP("DRAM_1_STAT_TEMP")
{

  // initialize pins
  mp_impl->HBM_REF_CLK_0(HBM_REF_CLK_0);
  mp_impl->HBM_REF_CLK_1(HBM_REF_CLK_1);
  mp_impl->AXI_12_ACLK(AXI_12_ACLK);
  mp_impl->AXI_12_ARESET_N(AXI_12_ARESET_N);
  mp_impl->AXI_12_WDATA_PARITY(AXI_12_WDATA_PARITY);
  mp_impl->APB_0_PWDATA(APB_0_PWDATA);
  mp_impl->APB_0_PADDR(APB_0_PADDR);
  mp_impl->APB_0_PCLK(APB_0_PCLK);
  mp_impl->APB_0_PENABLE(APB_0_PENABLE);
  mp_impl->APB_0_PRESET_N(APB_0_PRESET_N);
  mp_impl->APB_0_PSEL(APB_0_PSEL);
  mp_impl->APB_0_PWRITE(APB_0_PWRITE);
  mp_impl->APB_1_PWDATA(APB_1_PWDATA);
  mp_impl->APB_1_PADDR(APB_1_PADDR);
  mp_impl->APB_1_PCLK(APB_1_PCLK);
  mp_impl->APB_1_PENABLE(APB_1_PENABLE);
  mp_impl->APB_1_PRESET_N(APB_1_PRESET_N);
  mp_impl->APB_1_PSEL(APB_1_PSEL);
  mp_impl->APB_1_PWRITE(APB_1_PWRITE);
  mp_impl->AXI_12_RDATA_PARITY(AXI_12_RDATA_PARITY);
  mp_impl->APB_0_PRDATA(APB_0_PRDATA);
  mp_impl->APB_0_PREADY(APB_0_PREADY);
  mp_impl->APB_0_PSLVERR(APB_0_PSLVERR);
  mp_impl->APB_1_PRDATA(APB_1_PRDATA);
  mp_impl->APB_1_PREADY(APB_1_PREADY);
  mp_impl->APB_1_PSLVERR(APB_1_PSLVERR);
  mp_impl->apb_complete_0(apb_complete_0);
  mp_impl->apb_complete_1(apb_complete_1);
  mp_impl->DRAM_0_STAT_CATTRIP(DRAM_0_STAT_CATTRIP);
  mp_impl->DRAM_0_STAT_TEMP(DRAM_0_STAT_TEMP);
  mp_impl->DRAM_1_STAT_CATTRIP(DRAM_1_STAT_CATTRIP);
  mp_impl->DRAM_1_STAT_TEMP(DRAM_1_STAT_TEMP);

  // initialize transactors
  mp_SAXI_12_transactor = NULL;
  mp_AXI_12_ARLEN_converter = NULL;
  mp_AXI_12_AWLEN_converter = NULL;

  // initialize socket stubs

}

void bd_5dca_hbm_inst_0::before_end_of_elaboration()
{
  // configure 'SAXI_12' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("bd_5dca_hbm_inst_0", "SAXI_12_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'SAXI_12' transactor parameters
    xsc::common_cpp::properties SAXI_12_transactor_param_props;
    SAXI_12_transactor_param_props.addLong("DATA_WIDTH", "256");
    SAXI_12_transactor_param_props.addLong("FREQ_HZ", "450000000");
    SAXI_12_transactor_param_props.addLong("ID_WIDTH", "6");
    SAXI_12_transactor_param_props.addLong("ADDR_WIDTH", "33");
    SAXI_12_transactor_param_props.addLong("AWUSER_WIDTH", "0");
    SAXI_12_transactor_param_props.addLong("ARUSER_WIDTH", "0");
    SAXI_12_transactor_param_props.addLong("WUSER_WIDTH", "0");
    SAXI_12_transactor_param_props.addLong("RUSER_WIDTH", "0");
    SAXI_12_transactor_param_props.addLong("BUSER_WIDTH", "0");
    SAXI_12_transactor_param_props.addLong("HAS_BURST", "1");
    SAXI_12_transactor_param_props.addLong("HAS_LOCK", "0");
    SAXI_12_transactor_param_props.addLong("HAS_PROT", "0");
    SAXI_12_transactor_param_props.addLong("HAS_CACHE", "0");
    SAXI_12_transactor_param_props.addLong("HAS_QOS", "0");
    SAXI_12_transactor_param_props.addLong("HAS_REGION", "0");
    SAXI_12_transactor_param_props.addLong("HAS_WSTRB", "1");
    SAXI_12_transactor_param_props.addLong("HAS_BRESP", "1");
    SAXI_12_transactor_param_props.addLong("HAS_RRESP", "1");
    SAXI_12_transactor_param_props.addLong("SUPPORTS_NARROW_BURST", "1");
    SAXI_12_transactor_param_props.addLong("NUM_READ_OUTSTANDING", "2");
    SAXI_12_transactor_param_props.addLong("NUM_WRITE_OUTSTANDING", "2");
    SAXI_12_transactor_param_props.addLong("MAX_BURST_LENGTH", "16");
    SAXI_12_transactor_param_props.addLong("NUM_READ_THREADS", "1");
    SAXI_12_transactor_param_props.addLong("NUM_WRITE_THREADS", "1");
    SAXI_12_transactor_param_props.addLong("RUSER_BITS_PER_BYTE", "0");
    SAXI_12_transactor_param_props.addLong("WUSER_BITS_PER_BYTE", "0");
    SAXI_12_transactor_param_props.addLong("HAS_SIZE", "1");
    SAXI_12_transactor_param_props.addLong("HAS_RESET", "0");
    SAXI_12_transactor_param_props.addFloat("PHASE", "0.0");
    SAXI_12_transactor_param_props.addString("PROTOCOL", "AXI3");
    SAXI_12_transactor_param_props.addString("READ_WRITE_MODE", "READ_WRITE");
    SAXI_12_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_hbm_aclk_1_0_clk");
    SAXI_12_transactor_param_props.addString("SLR_ASSIGNMENT", "");

    mp_SAXI_12_transactor = new xtlm::xaximm_pin2xtlm_t<256,33,6,1,1,1,1,1>("SAXI_12_transactor", SAXI_12_transactor_param_props);

    // SAXI_12' transactor ports

    mp_SAXI_12_transactor->ARADDR(AXI_12_ARADDR);
    mp_SAXI_12_transactor->ARBURST(AXI_12_ARBURST);
    mp_SAXI_12_transactor->ARID(AXI_12_ARID);
    mp_AXI_12_ARLEN_converter = new xsc::common::vector2vector_converter<4,8>("AXI_12_ARLEN_converter");
    mp_AXI_12_ARLEN_converter->vector_in(AXI_12_ARLEN);
    mp_AXI_12_ARLEN_converter->vector_out(m_AXI_12_ARLEN_converter_signal);
    mp_SAXI_12_transactor->ARLEN(m_AXI_12_ARLEN_converter_signal);
    mp_SAXI_12_transactor->ARREADY(AXI_12_ARREADY);
    mp_SAXI_12_transactor->ARSIZE(AXI_12_ARSIZE);
    mp_SAXI_12_transactor->ARVALID(AXI_12_ARVALID);
    mp_SAXI_12_transactor->AWADDR(AXI_12_AWADDR);
    mp_SAXI_12_transactor->AWBURST(AXI_12_AWBURST);
    mp_SAXI_12_transactor->AWID(AXI_12_AWID);
    mp_AXI_12_AWLEN_converter = new xsc::common::vector2vector_converter<4,8>("AXI_12_AWLEN_converter");
    mp_AXI_12_AWLEN_converter->vector_in(AXI_12_AWLEN);
    mp_AXI_12_AWLEN_converter->vector_out(m_AXI_12_AWLEN_converter_signal);
    mp_SAXI_12_transactor->AWLEN(m_AXI_12_AWLEN_converter_signal);
    mp_SAXI_12_transactor->AWREADY(AXI_12_AWREADY);
    mp_SAXI_12_transactor->AWSIZE(AXI_12_AWSIZE);
    mp_SAXI_12_transactor->AWVALID(AXI_12_AWVALID);
    mp_SAXI_12_transactor->BID(AXI_12_BID);
    mp_SAXI_12_transactor->BREADY(AXI_12_BREADY);
    mp_SAXI_12_transactor->BRESP(AXI_12_BRESP);
    mp_SAXI_12_transactor->BVALID(AXI_12_BVALID);
    mp_SAXI_12_transactor->RDATA(AXI_12_RDATA);
    mp_SAXI_12_transactor->RID(AXI_12_RID);
    mp_SAXI_12_transactor->RLAST(AXI_12_RLAST);
    mp_SAXI_12_transactor->RREADY(AXI_12_RREADY);
    mp_SAXI_12_transactor->RRESP(AXI_12_RRESP);
    mp_SAXI_12_transactor->RVALID(AXI_12_RVALID);
    mp_SAXI_12_transactor->WDATA(AXI_12_WDATA);
    mp_SAXI_12_transactor->WLAST(AXI_12_WLAST);
    mp_SAXI_12_transactor->WREADY(AXI_12_WREADY);
    mp_SAXI_12_transactor->WSTRB(AXI_12_WSTRB);
    mp_SAXI_12_transactor->WVALID(AXI_12_WVALID);
    mp_SAXI_12_transactor->CLK(AXI_12_ACLK);
    m_SAXI_12_transactor_rst_signal.write(1);
    mp_SAXI_12_transactor->RST(m_SAXI_12_transactor_rst_signal);

    // SAXI_12' transactor sockets

    mp_impl->SAXI_12_rd_socket->bind(*(mp_SAXI_12_transactor->rd_socket));
    mp_impl->SAXI_12_wr_socket->bind(*(mp_SAXI_12_transactor->wr_socket));
  }
  else
  {
  }

}

#endif // XM_SYSTEMC




#ifdef RIVIERA
bd_5dca_hbm_inst_0::bd_5dca_hbm_inst_0(const sc_core::sc_module_name& nm) : bd_5dca_hbm_inst_0_sc(nm), HBM_REF_CLK_0("HBM_REF_CLK_0"), HBM_REF_CLK_1("HBM_REF_CLK_1"), AXI_12_ACLK("AXI_12_ACLK"), AXI_12_ARESET_N("AXI_12_ARESET_N"), AXI_12_ARADDR("AXI_12_ARADDR"), AXI_12_ARBURST("AXI_12_ARBURST"), AXI_12_ARID("AXI_12_ARID"), AXI_12_ARLEN("AXI_12_ARLEN"), AXI_12_ARSIZE("AXI_12_ARSIZE"), AXI_12_ARVALID("AXI_12_ARVALID"), AXI_12_AWADDR("AXI_12_AWADDR"), AXI_12_AWBURST("AXI_12_AWBURST"), AXI_12_AWID("AXI_12_AWID"), AXI_12_AWLEN("AXI_12_AWLEN"), AXI_12_AWSIZE("AXI_12_AWSIZE"), AXI_12_AWVALID("AXI_12_AWVALID"), AXI_12_RREADY("AXI_12_RREADY"), AXI_12_BREADY("AXI_12_BREADY"), AXI_12_WDATA("AXI_12_WDATA"), AXI_12_WLAST("AXI_12_WLAST"), AXI_12_WSTRB("AXI_12_WSTRB"), AXI_12_WDATA_PARITY("AXI_12_WDATA_PARITY"), AXI_12_WVALID("AXI_12_WVALID"), APB_0_PWDATA("APB_0_PWDATA"), APB_0_PADDR("APB_0_PADDR"), APB_0_PCLK("APB_0_PCLK"), APB_0_PENABLE("APB_0_PENABLE"), APB_0_PRESET_N("APB_0_PRESET_N"), APB_0_PSEL("APB_0_PSEL"), APB_0_PWRITE("APB_0_PWRITE"), APB_1_PWDATA("APB_1_PWDATA"), APB_1_PADDR("APB_1_PADDR"), APB_1_PCLK("APB_1_PCLK"), APB_1_PENABLE("APB_1_PENABLE"), APB_1_PRESET_N("APB_1_PRESET_N"), APB_1_PSEL("APB_1_PSEL"), APB_1_PWRITE("APB_1_PWRITE"), AXI_12_ARREADY("AXI_12_ARREADY"), AXI_12_AWREADY("AXI_12_AWREADY"), AXI_12_RDATA_PARITY("AXI_12_RDATA_PARITY"), AXI_12_RDATA("AXI_12_RDATA"), AXI_12_RID("AXI_12_RID"), AXI_12_RLAST("AXI_12_RLAST"), AXI_12_RRESP("AXI_12_RRESP"), AXI_12_RVALID("AXI_12_RVALID"), AXI_12_WREADY("AXI_12_WREADY"), AXI_12_BID("AXI_12_BID"), AXI_12_BRESP("AXI_12_BRESP"), AXI_12_BVALID("AXI_12_BVALID"), APB_0_PRDATA("APB_0_PRDATA"), APB_0_PREADY("APB_0_PREADY"), APB_0_PSLVERR("APB_0_PSLVERR"), APB_1_PRDATA("APB_1_PRDATA"), APB_1_PREADY("APB_1_PREADY"), APB_1_PSLVERR("APB_1_PSLVERR"), apb_complete_0("apb_complete_0"), apb_complete_1("apb_complete_1"), DRAM_0_STAT_CATTRIP("DRAM_0_STAT_CATTRIP"), DRAM_0_STAT_TEMP("DRAM_0_STAT_TEMP"), DRAM_1_STAT_CATTRIP("DRAM_1_STAT_CATTRIP"), DRAM_1_STAT_TEMP("DRAM_1_STAT_TEMP")
{

  // initialize pins
  mp_impl->HBM_REF_CLK_0(HBM_REF_CLK_0);
  mp_impl->HBM_REF_CLK_1(HBM_REF_CLK_1);
  mp_impl->AXI_12_ACLK(AXI_12_ACLK);
  mp_impl->AXI_12_ARESET_N(AXI_12_ARESET_N);
  mp_impl->AXI_12_WDATA_PARITY(AXI_12_WDATA_PARITY);
  mp_impl->APB_0_PWDATA(APB_0_PWDATA);
  mp_impl->APB_0_PADDR(APB_0_PADDR);
  mp_impl->APB_0_PCLK(APB_0_PCLK);
  mp_impl->APB_0_PENABLE(APB_0_PENABLE);
  mp_impl->APB_0_PRESET_N(APB_0_PRESET_N);
  mp_impl->APB_0_PSEL(APB_0_PSEL);
  mp_impl->APB_0_PWRITE(APB_0_PWRITE);
  mp_impl->APB_1_PWDATA(APB_1_PWDATA);
  mp_impl->APB_1_PADDR(APB_1_PADDR);
  mp_impl->APB_1_PCLK(APB_1_PCLK);
  mp_impl->APB_1_PENABLE(APB_1_PENABLE);
  mp_impl->APB_1_PRESET_N(APB_1_PRESET_N);
  mp_impl->APB_1_PSEL(APB_1_PSEL);
  mp_impl->APB_1_PWRITE(APB_1_PWRITE);
  mp_impl->AXI_12_RDATA_PARITY(AXI_12_RDATA_PARITY);
  mp_impl->APB_0_PRDATA(APB_0_PRDATA);
  mp_impl->APB_0_PREADY(APB_0_PREADY);
  mp_impl->APB_0_PSLVERR(APB_0_PSLVERR);
  mp_impl->APB_1_PRDATA(APB_1_PRDATA);
  mp_impl->APB_1_PREADY(APB_1_PREADY);
  mp_impl->APB_1_PSLVERR(APB_1_PSLVERR);
  mp_impl->apb_complete_0(apb_complete_0);
  mp_impl->apb_complete_1(apb_complete_1);
  mp_impl->DRAM_0_STAT_CATTRIP(DRAM_0_STAT_CATTRIP);
  mp_impl->DRAM_0_STAT_TEMP(DRAM_0_STAT_TEMP);
  mp_impl->DRAM_1_STAT_CATTRIP(DRAM_1_STAT_CATTRIP);
  mp_impl->DRAM_1_STAT_TEMP(DRAM_1_STAT_TEMP);

  // initialize transactors
  mp_SAXI_12_transactor = NULL;
  mp_AXI_12_ARLEN_converter = NULL;
  mp_AXI_12_AWLEN_converter = NULL;

  // initialize socket stubs

}

void bd_5dca_hbm_inst_0::before_end_of_elaboration()
{
  // configure 'SAXI_12' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("bd_5dca_hbm_inst_0", "SAXI_12_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'SAXI_12' transactor parameters
    xsc::common_cpp::properties SAXI_12_transactor_param_props;
    SAXI_12_transactor_param_props.addLong("DATA_WIDTH", "256");
    SAXI_12_transactor_param_props.addLong("FREQ_HZ", "450000000");
    SAXI_12_transactor_param_props.addLong("ID_WIDTH", "6");
    SAXI_12_transactor_param_props.addLong("ADDR_WIDTH", "33");
    SAXI_12_transactor_param_props.addLong("AWUSER_WIDTH", "0");
    SAXI_12_transactor_param_props.addLong("ARUSER_WIDTH", "0");
    SAXI_12_transactor_param_props.addLong("WUSER_WIDTH", "0");
    SAXI_12_transactor_param_props.addLong("RUSER_WIDTH", "0");
    SAXI_12_transactor_param_props.addLong("BUSER_WIDTH", "0");
    SAXI_12_transactor_param_props.addLong("HAS_BURST", "1");
    SAXI_12_transactor_param_props.addLong("HAS_LOCK", "0");
    SAXI_12_transactor_param_props.addLong("HAS_PROT", "0");
    SAXI_12_transactor_param_props.addLong("HAS_CACHE", "0");
    SAXI_12_transactor_param_props.addLong("HAS_QOS", "0");
    SAXI_12_transactor_param_props.addLong("HAS_REGION", "0");
    SAXI_12_transactor_param_props.addLong("HAS_WSTRB", "1");
    SAXI_12_transactor_param_props.addLong("HAS_BRESP", "1");
    SAXI_12_transactor_param_props.addLong("HAS_RRESP", "1");
    SAXI_12_transactor_param_props.addLong("SUPPORTS_NARROW_BURST", "1");
    SAXI_12_transactor_param_props.addLong("NUM_READ_OUTSTANDING", "2");
    SAXI_12_transactor_param_props.addLong("NUM_WRITE_OUTSTANDING", "2");
    SAXI_12_transactor_param_props.addLong("MAX_BURST_LENGTH", "16");
    SAXI_12_transactor_param_props.addLong("NUM_READ_THREADS", "1");
    SAXI_12_transactor_param_props.addLong("NUM_WRITE_THREADS", "1");
    SAXI_12_transactor_param_props.addLong("RUSER_BITS_PER_BYTE", "0");
    SAXI_12_transactor_param_props.addLong("WUSER_BITS_PER_BYTE", "0");
    SAXI_12_transactor_param_props.addLong("HAS_SIZE", "1");
    SAXI_12_transactor_param_props.addLong("HAS_RESET", "0");
    SAXI_12_transactor_param_props.addFloat("PHASE", "0.0");
    SAXI_12_transactor_param_props.addString("PROTOCOL", "AXI3");
    SAXI_12_transactor_param_props.addString("READ_WRITE_MODE", "READ_WRITE");
    SAXI_12_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_hbm_aclk_1_0_clk");
    SAXI_12_transactor_param_props.addString("SLR_ASSIGNMENT", "");

    mp_SAXI_12_transactor = new xtlm::xaximm_pin2xtlm_t<256,33,6,1,1,1,1,1>("SAXI_12_transactor", SAXI_12_transactor_param_props);

    // SAXI_12' transactor ports

    mp_SAXI_12_transactor->ARADDR(AXI_12_ARADDR);
    mp_SAXI_12_transactor->ARBURST(AXI_12_ARBURST);
    mp_SAXI_12_transactor->ARID(AXI_12_ARID);
    mp_AXI_12_ARLEN_converter = new xsc::common::vector2vector_converter<4,8>("AXI_12_ARLEN_converter");
    mp_AXI_12_ARLEN_converter->vector_in(AXI_12_ARLEN);
    mp_AXI_12_ARLEN_converter->vector_out(m_AXI_12_ARLEN_converter_signal);
    mp_SAXI_12_transactor->ARLEN(m_AXI_12_ARLEN_converter_signal);
    mp_SAXI_12_transactor->ARREADY(AXI_12_ARREADY);
    mp_SAXI_12_transactor->ARSIZE(AXI_12_ARSIZE);
    mp_SAXI_12_transactor->ARVALID(AXI_12_ARVALID);
    mp_SAXI_12_transactor->AWADDR(AXI_12_AWADDR);
    mp_SAXI_12_transactor->AWBURST(AXI_12_AWBURST);
    mp_SAXI_12_transactor->AWID(AXI_12_AWID);
    mp_AXI_12_AWLEN_converter = new xsc::common::vector2vector_converter<4,8>("AXI_12_AWLEN_converter");
    mp_AXI_12_AWLEN_converter->vector_in(AXI_12_AWLEN);
    mp_AXI_12_AWLEN_converter->vector_out(m_AXI_12_AWLEN_converter_signal);
    mp_SAXI_12_transactor->AWLEN(m_AXI_12_AWLEN_converter_signal);
    mp_SAXI_12_transactor->AWREADY(AXI_12_AWREADY);
    mp_SAXI_12_transactor->AWSIZE(AXI_12_AWSIZE);
    mp_SAXI_12_transactor->AWVALID(AXI_12_AWVALID);
    mp_SAXI_12_transactor->BID(AXI_12_BID);
    mp_SAXI_12_transactor->BREADY(AXI_12_BREADY);
    mp_SAXI_12_transactor->BRESP(AXI_12_BRESP);
    mp_SAXI_12_transactor->BVALID(AXI_12_BVALID);
    mp_SAXI_12_transactor->RDATA(AXI_12_RDATA);
    mp_SAXI_12_transactor->RID(AXI_12_RID);
    mp_SAXI_12_transactor->RLAST(AXI_12_RLAST);
    mp_SAXI_12_transactor->RREADY(AXI_12_RREADY);
    mp_SAXI_12_transactor->RRESP(AXI_12_RRESP);
    mp_SAXI_12_transactor->RVALID(AXI_12_RVALID);
    mp_SAXI_12_transactor->WDATA(AXI_12_WDATA);
    mp_SAXI_12_transactor->WLAST(AXI_12_WLAST);
    mp_SAXI_12_transactor->WREADY(AXI_12_WREADY);
    mp_SAXI_12_transactor->WSTRB(AXI_12_WSTRB);
    mp_SAXI_12_transactor->WVALID(AXI_12_WVALID);
    mp_SAXI_12_transactor->CLK(AXI_12_ACLK);
    m_SAXI_12_transactor_rst_signal.write(1);
    mp_SAXI_12_transactor->RST(m_SAXI_12_transactor_rst_signal);

    // SAXI_12' transactor sockets

    mp_impl->SAXI_12_rd_socket->bind(*(mp_SAXI_12_transactor->rd_socket));
    mp_impl->SAXI_12_wr_socket->bind(*(mp_SAXI_12_transactor->wr_socket));
  }
  else
  {
  }

}

#endif // RIVIERA




#ifdef VCSSYSTEMC
bd_5dca_hbm_inst_0::bd_5dca_hbm_inst_0(const sc_core::sc_module_name& nm) : bd_5dca_hbm_inst_0_sc(nm),  HBM_REF_CLK_0("HBM_REF_CLK_0"), HBM_REF_CLK_1("HBM_REF_CLK_1"), AXI_12_ACLK("AXI_12_ACLK"), AXI_12_ARESET_N("AXI_12_ARESET_N"), AXI_12_ARADDR("AXI_12_ARADDR"), AXI_12_ARBURST("AXI_12_ARBURST"), AXI_12_ARID("AXI_12_ARID"), AXI_12_ARLEN("AXI_12_ARLEN"), AXI_12_ARSIZE("AXI_12_ARSIZE"), AXI_12_ARVALID("AXI_12_ARVALID"), AXI_12_AWADDR("AXI_12_AWADDR"), AXI_12_AWBURST("AXI_12_AWBURST"), AXI_12_AWID("AXI_12_AWID"), AXI_12_AWLEN("AXI_12_AWLEN"), AXI_12_AWSIZE("AXI_12_AWSIZE"), AXI_12_AWVALID("AXI_12_AWVALID"), AXI_12_RREADY("AXI_12_RREADY"), AXI_12_BREADY("AXI_12_BREADY"), AXI_12_WDATA("AXI_12_WDATA"), AXI_12_WLAST("AXI_12_WLAST"), AXI_12_WSTRB("AXI_12_WSTRB"), AXI_12_WDATA_PARITY("AXI_12_WDATA_PARITY"), AXI_12_WVALID("AXI_12_WVALID"), APB_0_PWDATA("APB_0_PWDATA"), APB_0_PADDR("APB_0_PADDR"), APB_0_PCLK("APB_0_PCLK"), APB_0_PENABLE("APB_0_PENABLE"), APB_0_PRESET_N("APB_0_PRESET_N"), APB_0_PSEL("APB_0_PSEL"), APB_0_PWRITE("APB_0_PWRITE"), APB_1_PWDATA("APB_1_PWDATA"), APB_1_PADDR("APB_1_PADDR"), APB_1_PCLK("APB_1_PCLK"), APB_1_PENABLE("APB_1_PENABLE"), APB_1_PRESET_N("APB_1_PRESET_N"), APB_1_PSEL("APB_1_PSEL"), APB_1_PWRITE("APB_1_PWRITE"), AXI_12_ARREADY("AXI_12_ARREADY"), AXI_12_AWREADY("AXI_12_AWREADY"), AXI_12_RDATA_PARITY("AXI_12_RDATA_PARITY"), AXI_12_RDATA("AXI_12_RDATA"), AXI_12_RID("AXI_12_RID"), AXI_12_RLAST("AXI_12_RLAST"), AXI_12_RRESP("AXI_12_RRESP"), AXI_12_RVALID("AXI_12_RVALID"), AXI_12_WREADY("AXI_12_WREADY"), AXI_12_BID("AXI_12_BID"), AXI_12_BRESP("AXI_12_BRESP"), AXI_12_BVALID("AXI_12_BVALID"), APB_0_PRDATA("APB_0_PRDATA"), APB_0_PREADY("APB_0_PREADY"), APB_0_PSLVERR("APB_0_PSLVERR"), APB_1_PRDATA("APB_1_PRDATA"), APB_1_PREADY("APB_1_PREADY"), APB_1_PSLVERR("APB_1_PSLVERR"), apb_complete_0("apb_complete_0"), apb_complete_1("apb_complete_1"), DRAM_0_STAT_CATTRIP("DRAM_0_STAT_CATTRIP"), DRAM_0_STAT_TEMP("DRAM_0_STAT_TEMP"), DRAM_1_STAT_CATTRIP("DRAM_1_STAT_CATTRIP"), DRAM_1_STAT_TEMP("DRAM_1_STAT_TEMP")
{
  // initialize pins
  mp_impl->HBM_REF_CLK_0(HBM_REF_CLK_0);
  mp_impl->HBM_REF_CLK_1(HBM_REF_CLK_1);
  mp_impl->AXI_12_ACLK(AXI_12_ACLK);
  mp_impl->AXI_12_ARESET_N(AXI_12_ARESET_N);
  mp_impl->AXI_12_WDATA_PARITY(AXI_12_WDATA_PARITY);
  mp_impl->APB_0_PWDATA(APB_0_PWDATA);
  mp_impl->APB_0_PADDR(APB_0_PADDR);
  mp_impl->APB_0_PCLK(APB_0_PCLK);
  mp_impl->APB_0_PENABLE(APB_0_PENABLE);
  mp_impl->APB_0_PRESET_N(APB_0_PRESET_N);
  mp_impl->APB_0_PSEL(APB_0_PSEL);
  mp_impl->APB_0_PWRITE(APB_0_PWRITE);
  mp_impl->APB_1_PWDATA(APB_1_PWDATA);
  mp_impl->APB_1_PADDR(APB_1_PADDR);
  mp_impl->APB_1_PCLK(APB_1_PCLK);
  mp_impl->APB_1_PENABLE(APB_1_PENABLE);
  mp_impl->APB_1_PRESET_N(APB_1_PRESET_N);
  mp_impl->APB_1_PSEL(APB_1_PSEL);
  mp_impl->APB_1_PWRITE(APB_1_PWRITE);
  mp_impl->AXI_12_RDATA_PARITY(AXI_12_RDATA_PARITY);
  mp_impl->APB_0_PRDATA(APB_0_PRDATA);
  mp_impl->APB_0_PREADY(APB_0_PREADY);
  mp_impl->APB_0_PSLVERR(APB_0_PSLVERR);
  mp_impl->APB_1_PRDATA(APB_1_PRDATA);
  mp_impl->APB_1_PREADY(APB_1_PREADY);
  mp_impl->APB_1_PSLVERR(APB_1_PSLVERR);
  mp_impl->apb_complete_0(apb_complete_0);
  mp_impl->apb_complete_1(apb_complete_1);
  mp_impl->DRAM_0_STAT_CATTRIP(DRAM_0_STAT_CATTRIP);
  mp_impl->DRAM_0_STAT_TEMP(DRAM_0_STAT_TEMP);
  mp_impl->DRAM_1_STAT_CATTRIP(DRAM_1_STAT_CATTRIP);
  mp_impl->DRAM_1_STAT_TEMP(DRAM_1_STAT_TEMP);

  // initialize transactors
  mp_SAXI_12_transactor = NULL;
  mp_AXI_12_ARLEN_converter = NULL;
  mp_AXI_12_AWLEN_converter = NULL;

  // Instantiate Socket Stubs

  // configure SAXI_12_transactor
    xsc::common_cpp::properties SAXI_12_transactor_param_props;
    SAXI_12_transactor_param_props.addLong("DATA_WIDTH", "256");
    SAXI_12_transactor_param_props.addLong("FREQ_HZ", "450000000");
    SAXI_12_transactor_param_props.addLong("ID_WIDTH", "6");
    SAXI_12_transactor_param_props.addLong("ADDR_WIDTH", "33");
    SAXI_12_transactor_param_props.addLong("AWUSER_WIDTH", "0");
    SAXI_12_transactor_param_props.addLong("ARUSER_WIDTH", "0");
    SAXI_12_transactor_param_props.addLong("WUSER_WIDTH", "0");
    SAXI_12_transactor_param_props.addLong("RUSER_WIDTH", "0");
    SAXI_12_transactor_param_props.addLong("BUSER_WIDTH", "0");
    SAXI_12_transactor_param_props.addLong("HAS_BURST", "1");
    SAXI_12_transactor_param_props.addLong("HAS_LOCK", "0");
    SAXI_12_transactor_param_props.addLong("HAS_PROT", "0");
    SAXI_12_transactor_param_props.addLong("HAS_CACHE", "0");
    SAXI_12_transactor_param_props.addLong("HAS_QOS", "0");
    SAXI_12_transactor_param_props.addLong("HAS_REGION", "0");
    SAXI_12_transactor_param_props.addLong("HAS_WSTRB", "1");
    SAXI_12_transactor_param_props.addLong("HAS_BRESP", "1");
    SAXI_12_transactor_param_props.addLong("HAS_RRESP", "1");
    SAXI_12_transactor_param_props.addLong("SUPPORTS_NARROW_BURST", "1");
    SAXI_12_transactor_param_props.addLong("NUM_READ_OUTSTANDING", "2");
    SAXI_12_transactor_param_props.addLong("NUM_WRITE_OUTSTANDING", "2");
    SAXI_12_transactor_param_props.addLong("MAX_BURST_LENGTH", "16");
    SAXI_12_transactor_param_props.addLong("NUM_READ_THREADS", "1");
    SAXI_12_transactor_param_props.addLong("NUM_WRITE_THREADS", "1");
    SAXI_12_transactor_param_props.addLong("RUSER_BITS_PER_BYTE", "0");
    SAXI_12_transactor_param_props.addLong("WUSER_BITS_PER_BYTE", "0");
    SAXI_12_transactor_param_props.addLong("HAS_SIZE", "1");
    SAXI_12_transactor_param_props.addLong("HAS_RESET", "0");
    SAXI_12_transactor_param_props.addFloat("PHASE", "0.0");
    SAXI_12_transactor_param_props.addString("PROTOCOL", "AXI3");
    SAXI_12_transactor_param_props.addString("READ_WRITE_MODE", "READ_WRITE");
    SAXI_12_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_hbm_aclk_1_0_clk");
    SAXI_12_transactor_param_props.addString("SLR_ASSIGNMENT", "");

    mp_SAXI_12_transactor = new xtlm::xaximm_pin2xtlm_t<256,33,6,1,1,1,1,1>("SAXI_12_transactor", SAXI_12_transactor_param_props);
  mp_SAXI_12_transactor->ARADDR(AXI_12_ARADDR);
  mp_SAXI_12_transactor->ARBURST(AXI_12_ARBURST);
  mp_SAXI_12_transactor->ARID(AXI_12_ARID);
  mp_AXI_12_ARLEN_converter = new xsc::common::vector2vector_converter<4,8>("AXI_12_ARLEN_converter");
  mp_AXI_12_ARLEN_converter->vector_in(AXI_12_ARLEN);
  mp_AXI_12_ARLEN_converter->vector_out(m_AXI_12_ARLEN_converter_signal);
  mp_SAXI_12_transactor->ARLEN(m_AXI_12_ARLEN_converter_signal);
  mp_SAXI_12_transactor->ARREADY(AXI_12_ARREADY);
  mp_SAXI_12_transactor->ARSIZE(AXI_12_ARSIZE);
  mp_SAXI_12_transactor->ARVALID(AXI_12_ARVALID);
  mp_SAXI_12_transactor->AWADDR(AXI_12_AWADDR);
  mp_SAXI_12_transactor->AWBURST(AXI_12_AWBURST);
  mp_SAXI_12_transactor->AWID(AXI_12_AWID);
  mp_AXI_12_AWLEN_converter = new xsc::common::vector2vector_converter<4,8>("AXI_12_AWLEN_converter");
  mp_AXI_12_AWLEN_converter->vector_in(AXI_12_AWLEN);
  mp_AXI_12_AWLEN_converter->vector_out(m_AXI_12_AWLEN_converter_signal);
  mp_SAXI_12_transactor->AWLEN(m_AXI_12_AWLEN_converter_signal);
  mp_SAXI_12_transactor->AWREADY(AXI_12_AWREADY);
  mp_SAXI_12_transactor->AWSIZE(AXI_12_AWSIZE);
  mp_SAXI_12_transactor->AWVALID(AXI_12_AWVALID);
  mp_SAXI_12_transactor->BID(AXI_12_BID);
  mp_SAXI_12_transactor->BREADY(AXI_12_BREADY);
  mp_SAXI_12_transactor->BRESP(AXI_12_BRESP);
  mp_SAXI_12_transactor->BVALID(AXI_12_BVALID);
  mp_SAXI_12_transactor->RDATA(AXI_12_RDATA);
  mp_SAXI_12_transactor->RID(AXI_12_RID);
  mp_SAXI_12_transactor->RLAST(AXI_12_RLAST);
  mp_SAXI_12_transactor->RREADY(AXI_12_RREADY);
  mp_SAXI_12_transactor->RRESP(AXI_12_RRESP);
  mp_SAXI_12_transactor->RVALID(AXI_12_RVALID);
  mp_SAXI_12_transactor->WDATA(AXI_12_WDATA);
  mp_SAXI_12_transactor->WLAST(AXI_12_WLAST);
  mp_SAXI_12_transactor->WREADY(AXI_12_WREADY);
  mp_SAXI_12_transactor->WSTRB(AXI_12_WSTRB);
  mp_SAXI_12_transactor->WVALID(AXI_12_WVALID);
  mp_SAXI_12_transactor->CLK(AXI_12_ACLK);
  m_SAXI_12_transactor_rst_signal.write(1);
  mp_SAXI_12_transactor->RST(m_SAXI_12_transactor_rst_signal);

  // initialize transactors stubs
  SAXI_12_transactor_target_wr_socket_stub = nullptr;
  SAXI_12_transactor_target_rd_socket_stub = nullptr;

}

void bd_5dca_hbm_inst_0::before_end_of_elaboration()
{
  // configure 'SAXI_12' transactor
  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("bd_5dca_hbm_inst_0", "SAXI_12_TLM_MODE") != 1)
  {
    mp_impl->SAXI_12_rd_socket->bind(*(mp_SAXI_12_transactor->rd_socket));
    mp_impl->SAXI_12_wr_socket->bind(*(mp_SAXI_12_transactor->wr_socket));
  
  }
  else
  {
    SAXI_12_transactor_target_wr_socket_stub = new xtlm::xtlm_aximm_target_stub("wr_socket",0);
    SAXI_12_transactor_target_wr_socket_stub->bind(*(mp_SAXI_12_transactor->wr_socket));
    SAXI_12_transactor_target_rd_socket_stub = new xtlm::xtlm_aximm_target_stub("rd_socket",0);
    SAXI_12_transactor_target_rd_socket_stub->bind(*(mp_SAXI_12_transactor->rd_socket));
    mp_SAXI_12_transactor->disable_transactor();
  }

}

#endif // VCSSYSTEMC




#ifdef MTI_SYSTEMC
bd_5dca_hbm_inst_0::bd_5dca_hbm_inst_0(const sc_core::sc_module_name& nm) : bd_5dca_hbm_inst_0_sc(nm),  HBM_REF_CLK_0("HBM_REF_CLK_0"), HBM_REF_CLK_1("HBM_REF_CLK_1"), AXI_12_ACLK("AXI_12_ACLK"), AXI_12_ARESET_N("AXI_12_ARESET_N"), AXI_12_ARADDR("AXI_12_ARADDR"), AXI_12_ARBURST("AXI_12_ARBURST"), AXI_12_ARID("AXI_12_ARID"), AXI_12_ARLEN("AXI_12_ARLEN"), AXI_12_ARSIZE("AXI_12_ARSIZE"), AXI_12_ARVALID("AXI_12_ARVALID"), AXI_12_AWADDR("AXI_12_AWADDR"), AXI_12_AWBURST("AXI_12_AWBURST"), AXI_12_AWID("AXI_12_AWID"), AXI_12_AWLEN("AXI_12_AWLEN"), AXI_12_AWSIZE("AXI_12_AWSIZE"), AXI_12_AWVALID("AXI_12_AWVALID"), AXI_12_RREADY("AXI_12_RREADY"), AXI_12_BREADY("AXI_12_BREADY"), AXI_12_WDATA("AXI_12_WDATA"), AXI_12_WLAST("AXI_12_WLAST"), AXI_12_WSTRB("AXI_12_WSTRB"), AXI_12_WDATA_PARITY("AXI_12_WDATA_PARITY"), AXI_12_WVALID("AXI_12_WVALID"), APB_0_PWDATA("APB_0_PWDATA"), APB_0_PADDR("APB_0_PADDR"), APB_0_PCLK("APB_0_PCLK"), APB_0_PENABLE("APB_0_PENABLE"), APB_0_PRESET_N("APB_0_PRESET_N"), APB_0_PSEL("APB_0_PSEL"), APB_0_PWRITE("APB_0_PWRITE"), APB_1_PWDATA("APB_1_PWDATA"), APB_1_PADDR("APB_1_PADDR"), APB_1_PCLK("APB_1_PCLK"), APB_1_PENABLE("APB_1_PENABLE"), APB_1_PRESET_N("APB_1_PRESET_N"), APB_1_PSEL("APB_1_PSEL"), APB_1_PWRITE("APB_1_PWRITE"), AXI_12_ARREADY("AXI_12_ARREADY"), AXI_12_AWREADY("AXI_12_AWREADY"), AXI_12_RDATA_PARITY("AXI_12_RDATA_PARITY"), AXI_12_RDATA("AXI_12_RDATA"), AXI_12_RID("AXI_12_RID"), AXI_12_RLAST("AXI_12_RLAST"), AXI_12_RRESP("AXI_12_RRESP"), AXI_12_RVALID("AXI_12_RVALID"), AXI_12_WREADY("AXI_12_WREADY"), AXI_12_BID("AXI_12_BID"), AXI_12_BRESP("AXI_12_BRESP"), AXI_12_BVALID("AXI_12_BVALID"), APB_0_PRDATA("APB_0_PRDATA"), APB_0_PREADY("APB_0_PREADY"), APB_0_PSLVERR("APB_0_PSLVERR"), APB_1_PRDATA("APB_1_PRDATA"), APB_1_PREADY("APB_1_PREADY"), APB_1_PSLVERR("APB_1_PSLVERR"), apb_complete_0("apb_complete_0"), apb_complete_1("apb_complete_1"), DRAM_0_STAT_CATTRIP("DRAM_0_STAT_CATTRIP"), DRAM_0_STAT_TEMP("DRAM_0_STAT_TEMP"), DRAM_1_STAT_CATTRIP("DRAM_1_STAT_CATTRIP"), DRAM_1_STAT_TEMP("DRAM_1_STAT_TEMP")
{
  // initialize pins
  mp_impl->HBM_REF_CLK_0(HBM_REF_CLK_0);
  mp_impl->HBM_REF_CLK_1(HBM_REF_CLK_1);
  mp_impl->AXI_12_ACLK(AXI_12_ACLK);
  mp_impl->AXI_12_ARESET_N(AXI_12_ARESET_N);
  mp_impl->AXI_12_WDATA_PARITY(AXI_12_WDATA_PARITY);
  mp_impl->APB_0_PWDATA(APB_0_PWDATA);
  mp_impl->APB_0_PADDR(APB_0_PADDR);
  mp_impl->APB_0_PCLK(APB_0_PCLK);
  mp_impl->APB_0_PENABLE(APB_0_PENABLE);
  mp_impl->APB_0_PRESET_N(APB_0_PRESET_N);
  mp_impl->APB_0_PSEL(APB_0_PSEL);
  mp_impl->APB_0_PWRITE(APB_0_PWRITE);
  mp_impl->APB_1_PWDATA(APB_1_PWDATA);
  mp_impl->APB_1_PADDR(APB_1_PADDR);
  mp_impl->APB_1_PCLK(APB_1_PCLK);
  mp_impl->APB_1_PENABLE(APB_1_PENABLE);
  mp_impl->APB_1_PRESET_N(APB_1_PRESET_N);
  mp_impl->APB_1_PSEL(APB_1_PSEL);
  mp_impl->APB_1_PWRITE(APB_1_PWRITE);
  mp_impl->AXI_12_RDATA_PARITY(AXI_12_RDATA_PARITY);
  mp_impl->APB_0_PRDATA(APB_0_PRDATA);
  mp_impl->APB_0_PREADY(APB_0_PREADY);
  mp_impl->APB_0_PSLVERR(APB_0_PSLVERR);
  mp_impl->APB_1_PRDATA(APB_1_PRDATA);
  mp_impl->APB_1_PREADY(APB_1_PREADY);
  mp_impl->APB_1_PSLVERR(APB_1_PSLVERR);
  mp_impl->apb_complete_0(apb_complete_0);
  mp_impl->apb_complete_1(apb_complete_1);
  mp_impl->DRAM_0_STAT_CATTRIP(DRAM_0_STAT_CATTRIP);
  mp_impl->DRAM_0_STAT_TEMP(DRAM_0_STAT_TEMP);
  mp_impl->DRAM_1_STAT_CATTRIP(DRAM_1_STAT_CATTRIP);
  mp_impl->DRAM_1_STAT_TEMP(DRAM_1_STAT_TEMP);

  // initialize transactors
  mp_SAXI_12_transactor = NULL;
  mp_AXI_12_ARLEN_converter = NULL;
  mp_AXI_12_AWLEN_converter = NULL;

  // Instantiate Socket Stubs

  // configure SAXI_12_transactor
    xsc::common_cpp::properties SAXI_12_transactor_param_props;
    SAXI_12_transactor_param_props.addLong("DATA_WIDTH", "256");
    SAXI_12_transactor_param_props.addLong("FREQ_HZ", "450000000");
    SAXI_12_transactor_param_props.addLong("ID_WIDTH", "6");
    SAXI_12_transactor_param_props.addLong("ADDR_WIDTH", "33");
    SAXI_12_transactor_param_props.addLong("AWUSER_WIDTH", "0");
    SAXI_12_transactor_param_props.addLong("ARUSER_WIDTH", "0");
    SAXI_12_transactor_param_props.addLong("WUSER_WIDTH", "0");
    SAXI_12_transactor_param_props.addLong("RUSER_WIDTH", "0");
    SAXI_12_transactor_param_props.addLong("BUSER_WIDTH", "0");
    SAXI_12_transactor_param_props.addLong("HAS_BURST", "1");
    SAXI_12_transactor_param_props.addLong("HAS_LOCK", "0");
    SAXI_12_transactor_param_props.addLong("HAS_PROT", "0");
    SAXI_12_transactor_param_props.addLong("HAS_CACHE", "0");
    SAXI_12_transactor_param_props.addLong("HAS_QOS", "0");
    SAXI_12_transactor_param_props.addLong("HAS_REGION", "0");
    SAXI_12_transactor_param_props.addLong("HAS_WSTRB", "1");
    SAXI_12_transactor_param_props.addLong("HAS_BRESP", "1");
    SAXI_12_transactor_param_props.addLong("HAS_RRESP", "1");
    SAXI_12_transactor_param_props.addLong("SUPPORTS_NARROW_BURST", "1");
    SAXI_12_transactor_param_props.addLong("NUM_READ_OUTSTANDING", "2");
    SAXI_12_transactor_param_props.addLong("NUM_WRITE_OUTSTANDING", "2");
    SAXI_12_transactor_param_props.addLong("MAX_BURST_LENGTH", "16");
    SAXI_12_transactor_param_props.addLong("NUM_READ_THREADS", "1");
    SAXI_12_transactor_param_props.addLong("NUM_WRITE_THREADS", "1");
    SAXI_12_transactor_param_props.addLong("RUSER_BITS_PER_BYTE", "0");
    SAXI_12_transactor_param_props.addLong("WUSER_BITS_PER_BYTE", "0");
    SAXI_12_transactor_param_props.addLong("HAS_SIZE", "1");
    SAXI_12_transactor_param_props.addLong("HAS_RESET", "0");
    SAXI_12_transactor_param_props.addFloat("PHASE", "0.0");
    SAXI_12_transactor_param_props.addString("PROTOCOL", "AXI3");
    SAXI_12_transactor_param_props.addString("READ_WRITE_MODE", "READ_WRITE");
    SAXI_12_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_hbm_aclk_1_0_clk");
    SAXI_12_transactor_param_props.addString("SLR_ASSIGNMENT", "");

    mp_SAXI_12_transactor = new xtlm::xaximm_pin2xtlm_t<256,33,6,1,1,1,1,1>("SAXI_12_transactor", SAXI_12_transactor_param_props);
  mp_SAXI_12_transactor->ARADDR(AXI_12_ARADDR);
  mp_SAXI_12_transactor->ARBURST(AXI_12_ARBURST);
  mp_SAXI_12_transactor->ARID(AXI_12_ARID);
  mp_AXI_12_ARLEN_converter = new xsc::common::vector2vector_converter<4,8>("AXI_12_ARLEN_converter");
  mp_AXI_12_ARLEN_converter->vector_in(AXI_12_ARLEN);
  mp_AXI_12_ARLEN_converter->vector_out(m_AXI_12_ARLEN_converter_signal);
  mp_SAXI_12_transactor->ARLEN(m_AXI_12_ARLEN_converter_signal);
  mp_SAXI_12_transactor->ARREADY(AXI_12_ARREADY);
  mp_SAXI_12_transactor->ARSIZE(AXI_12_ARSIZE);
  mp_SAXI_12_transactor->ARVALID(AXI_12_ARVALID);
  mp_SAXI_12_transactor->AWADDR(AXI_12_AWADDR);
  mp_SAXI_12_transactor->AWBURST(AXI_12_AWBURST);
  mp_SAXI_12_transactor->AWID(AXI_12_AWID);
  mp_AXI_12_AWLEN_converter = new xsc::common::vector2vector_converter<4,8>("AXI_12_AWLEN_converter");
  mp_AXI_12_AWLEN_converter->vector_in(AXI_12_AWLEN);
  mp_AXI_12_AWLEN_converter->vector_out(m_AXI_12_AWLEN_converter_signal);
  mp_SAXI_12_transactor->AWLEN(m_AXI_12_AWLEN_converter_signal);
  mp_SAXI_12_transactor->AWREADY(AXI_12_AWREADY);
  mp_SAXI_12_transactor->AWSIZE(AXI_12_AWSIZE);
  mp_SAXI_12_transactor->AWVALID(AXI_12_AWVALID);
  mp_SAXI_12_transactor->BID(AXI_12_BID);
  mp_SAXI_12_transactor->BREADY(AXI_12_BREADY);
  mp_SAXI_12_transactor->BRESP(AXI_12_BRESP);
  mp_SAXI_12_transactor->BVALID(AXI_12_BVALID);
  mp_SAXI_12_transactor->RDATA(AXI_12_RDATA);
  mp_SAXI_12_transactor->RID(AXI_12_RID);
  mp_SAXI_12_transactor->RLAST(AXI_12_RLAST);
  mp_SAXI_12_transactor->RREADY(AXI_12_RREADY);
  mp_SAXI_12_transactor->RRESP(AXI_12_RRESP);
  mp_SAXI_12_transactor->RVALID(AXI_12_RVALID);
  mp_SAXI_12_transactor->WDATA(AXI_12_WDATA);
  mp_SAXI_12_transactor->WLAST(AXI_12_WLAST);
  mp_SAXI_12_transactor->WREADY(AXI_12_WREADY);
  mp_SAXI_12_transactor->WSTRB(AXI_12_WSTRB);
  mp_SAXI_12_transactor->WVALID(AXI_12_WVALID);
  mp_SAXI_12_transactor->CLK(AXI_12_ACLK);
  m_SAXI_12_transactor_rst_signal.write(1);
  mp_SAXI_12_transactor->RST(m_SAXI_12_transactor_rst_signal);

  // initialize transactors stubs
  SAXI_12_transactor_target_wr_socket_stub = nullptr;
  SAXI_12_transactor_target_rd_socket_stub = nullptr;

}

void bd_5dca_hbm_inst_0::before_end_of_elaboration()
{
  // configure 'SAXI_12' transactor
  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("bd_5dca_hbm_inst_0", "SAXI_12_TLM_MODE") != 1)
  {
    mp_impl->SAXI_12_rd_socket->bind(*(mp_SAXI_12_transactor->rd_socket));
    mp_impl->SAXI_12_wr_socket->bind(*(mp_SAXI_12_transactor->wr_socket));
  
  }
  else
  {
    SAXI_12_transactor_target_wr_socket_stub = new xtlm::xtlm_aximm_target_stub("wr_socket",0);
    SAXI_12_transactor_target_wr_socket_stub->bind(*(mp_SAXI_12_transactor->wr_socket));
    SAXI_12_transactor_target_rd_socket_stub = new xtlm::xtlm_aximm_target_stub("rd_socket",0);
    SAXI_12_transactor_target_rd_socket_stub->bind(*(mp_SAXI_12_transactor->rd_socket));
    mp_SAXI_12_transactor->disable_transactor();
  }

}

#endif // MTI_SYSTEMC




bd_5dca_hbm_inst_0::~bd_5dca_hbm_inst_0()
{
  delete mp_SAXI_12_transactor;
  delete mp_AXI_12_ARLEN_converter;
  delete mp_AXI_12_AWLEN_converter;

}

#ifdef MTI_SYSTEMC
SC_MODULE_EXPORT(bd_5dca_hbm_inst_0);
#endif

#ifdef XM_SYSTEMC
XMSC_MODULE_EXPORT(bd_5dca_hbm_inst_0);
#endif

#ifdef RIVIERA
SC_MODULE_EXPORT(bd_5dca_hbm_inst_0);
SC_REGISTER_BV(256);
#endif

