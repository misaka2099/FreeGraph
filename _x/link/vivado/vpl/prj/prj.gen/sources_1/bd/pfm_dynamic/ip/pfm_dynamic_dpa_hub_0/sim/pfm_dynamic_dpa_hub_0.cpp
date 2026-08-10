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


#include "pfm_dynamic_dpa_hub_0_sc.h"

#include "pfm_dynamic_dpa_hub_0.h"

#include "emulation_profiler_core.h"

#include <map>
#include <string>





#ifdef XILINX_SIMULATOR
pfm_dynamic_dpa_hub_0::pfm_dynamic_dpa_hub_0(const sc_core::sc_module_name& nm) : pfm_dynamic_dpa_hub_0_sc(nm), s_aximm_clk("s_aximm_clk"), s_aximm_aresetn("s_aximm_aresetn"), trace_clk("trace_clk"), trace_aresetn("trace_aresetn"), axilite_clk("axilite_clk"), axilite_aresetn("axilite_aresetn"), trace_tready0("trace_tready0"), trace_tdata0("trace_tdata0"), trace_tlast0("trace_tlast0"), trace_tid0("trace_tid0"), trace_tdest0("trace_tdest0"), trace_tvalid0("trace_tvalid0"), trace_tready1("trace_tready1"), trace_tdata1("trace_tdata1"), trace_tlast1("trace_tlast1"), trace_tid1("trace_tid1"), trace_tdest1("trace_tdest1"), trace_tvalid1("trace_tvalid1"), trace_tdata2("trace_tdata2"), trace_tready2("trace_tready2"), trace_tlast2("trace_tlast2"), trace_tid2("trace_tid2"), trace_tdest2("trace_tdest2"), trace_tvalid2("trace_tvalid2"), trace_tdata3("trace_tdata3"), trace_tready3("trace_tready3"), trace_tlast3("trace_tlast3"), trace_tid3("trace_tid3"), trace_tdest3("trace_tdest3"), trace_tvalid3("trace_tvalid3"), trace_tdata4("trace_tdata4"), trace_tready4("trace_tready4"), trace_tlast4("trace_tlast4"), trace_tid4("trace_tid4"), trace_tdest4("trace_tdest4"), trace_tvalid4("trace_tvalid4"), trace_tdata5("trace_tdata5"), trace_tready5("trace_tready5"), trace_tlast5("trace_tlast5"), trace_tid5("trace_tid5"), trace_tdest5("trace_tdest5"), trace_tvalid5("trace_tvalid5"), trace_tdata6("trace_tdata6"), trace_tready6("trace_tready6"), trace_tlast6("trace_tlast6"), trace_tid6("trace_tid6"), trace_tdest6("trace_tdest6"), trace_tvalid6("trace_tvalid6"), trace_tdata7("trace_tdata7"), trace_tready7("trace_tready7"), trace_tlast7("trace_tlast7"), trace_tid7("trace_tid7"), trace_tdest7("trace_tdest7"), trace_tvalid7("trace_tvalid7"), trace_tdata8("trace_tdata8"), trace_tready8("trace_tready8"), trace_tlast8("trace_tlast8"), trace_tid8("trace_tid8"), trace_tdest8("trace_tdest8"), trace_tvalid8("trace_tvalid8"), trace_tdata9("trace_tdata9"), trace_tready9("trace_tready9"), trace_tlast9("trace_tlast9"), trace_tid9("trace_tid9"), trace_tdest9("trace_tdest9"), trace_tvalid9("trace_tvalid9"), trace_tdata10("trace_tdata10"), trace_tready10("trace_tready10"), trace_tlast10("trace_tlast10"), trace_tid10("trace_tid10"), trace_tdest10("trace_tdest10"), trace_tvalid10("trace_tvalid10"), trace_tdata11("trace_tdata11"), trace_tready11("trace_tready11"), trace_tlast11("trace_tlast11"), trace_tid11("trace_tid11"), trace_tdest11("trace_tdest11"), trace_tvalid11("trace_tvalid11"), trace_tdata12("trace_tdata12"), trace_tready12("trace_tready12"), trace_tlast12("trace_tlast12"), trace_tid12("trace_tid12"), trace_tdest12("trace_tdest12"), trace_tvalid12("trace_tvalid12"), trace_tdata13("trace_tdata13"), trace_tready13("trace_tready13"), trace_tlast13("trace_tlast13"), trace_tid13("trace_tid13"), trace_tdest13("trace_tdest13"), trace_tvalid13("trace_tvalid13"), trace_tdata14("trace_tdata14"), trace_tready14("trace_tready14"), trace_tlast14("trace_tlast14"), trace_tid14("trace_tid14"), trace_tdest14("trace_tdest14"), trace_tvalid14("trace_tvalid14"), trace_tdata15("trace_tdata15"), trace_tready15("trace_tready15"), trace_tlast15("trace_tlast15"), trace_tid15("trace_tid15"), trace_tdest15("trace_tdest15"), trace_tvalid15("trace_tvalid15"), trace_tdata16("trace_tdata16"), trace_tready16("trace_tready16"), trace_tlast16("trace_tlast16"), trace_tid16("trace_tid16"), trace_tdest16("trace_tdest16"), trace_tvalid16("trace_tvalid16"), trace_tdata17("trace_tdata17"), trace_tready17("trace_tready17"), trace_tlast17("trace_tlast17"), trace_tid17("trace_tid17"), trace_tdest17("trace_tdest17"), trace_tvalid17("trace_tvalid17"), trace_tdata18("trace_tdata18"), trace_tready18("trace_tready18"), trace_tlast18("trace_tlast18"), trace_tid18("trace_tid18"), trace_tdest18("trace_tdest18"), trace_tvalid18("trace_tvalid18"), trace_tdata19("trace_tdata19"), trace_tready19("trace_tready19"), trace_tlast19("trace_tlast19"), trace_tid19("trace_tid19"), trace_tdest19("trace_tdest19"), trace_tvalid19("trace_tvalid19"), trace_tdata20("trace_tdata20"), trace_tready20("trace_tready20"), trace_tlast20("trace_tlast20"), trace_tid20("trace_tid20"), trace_tdest20("trace_tdest20"), trace_tvalid20("trace_tvalid20"), trace_tdata21("trace_tdata21"), trace_tready21("trace_tready21"), trace_tlast21("trace_tlast21"), trace_tid21("trace_tid21"), trace_tdest21("trace_tdest21"), trace_tvalid21("trace_tvalid21"), trace_tdata22("trace_tdata22"), trace_tready22("trace_tready22"), trace_tlast22("trace_tlast22"), trace_tid22("trace_tid22"), trace_tdest22("trace_tdest22"), trace_tvalid22("trace_tvalid22"), trace_tdata23("trace_tdata23"), trace_tready23("trace_tready23"), trace_tlast23("trace_tlast23"), trace_tid23("trace_tid23"), trace_tdest23("trace_tdest23"), trace_tvalid23("trace_tvalid23"), trace_tdata24("trace_tdata24"), trace_tready24("trace_tready24"), trace_tlast24("trace_tlast24"), trace_tid24("trace_tid24"), trace_tdest24("trace_tdest24"), trace_tvalid24("trace_tvalid24"), trace_tdata25("trace_tdata25"), trace_tready25("trace_tready25"), trace_tlast25("trace_tlast25"), trace_tid25("trace_tid25"), trace_tdest25("trace_tdest25"), trace_tvalid25("trace_tvalid25"), trace_tdata26("trace_tdata26"), trace_tready26("trace_tready26"), trace_tlast26("trace_tlast26"), trace_tid26("trace_tid26"), trace_tdest26("trace_tdest26"), trace_tvalid26("trace_tvalid26"), trace_tdata27("trace_tdata27"), trace_tready27("trace_tready27"), trace_tlast27("trace_tlast27"), trace_tid27("trace_tid27"), trace_tdest27("trace_tdest27"), trace_tvalid27("trace_tvalid27"), trace_tdata28("trace_tdata28"), trace_tready28("trace_tready28"), trace_tlast28("trace_tlast28"), trace_tid28("trace_tid28"), trace_tdest28("trace_tdest28"), trace_tvalid28("trace_tvalid28"), trace_tdata29("trace_tdata29"), trace_tready29("trace_tready29"), trace_tlast29("trace_tlast29"), trace_tid29("trace_tid29"), trace_tdest29("trace_tdest29"), trace_tvalid29("trace_tvalid29"), trace_tdata30("trace_tdata30"), trace_tready30("trace_tready30"), trace_tlast30("trace_tlast30"), trace_tid30("trace_tid30"), trace_tdest30("trace_tdest30"), trace_tvalid30("trace_tvalid30"), trace_tdata31("trace_tdata31"), trace_tready31("trace_tready31"), trace_tlast31("trace_tlast31"), trace_tid31("trace_tid31"), trace_tdest31("trace_tdest31"), trace_tvalid31("trace_tvalid31"), trace_tdata32("trace_tdata32"), trace_tready32("trace_tready32"), trace_tlast32("trace_tlast32"), trace_tid32("trace_tid32"), trace_tdest32("trace_tdest32"), trace_tvalid32("trace_tvalid32"), trace_tdata33("trace_tdata33"), trace_tready33("trace_tready33"), trace_tlast33("trace_tlast33"), trace_tid33("trace_tid33"), trace_tdest33("trace_tdest33"), trace_tvalid33("trace_tvalid33"), trace_tdata34("trace_tdata34"), trace_tready34("trace_tready34"), trace_tlast34("trace_tlast34"), trace_tid34("trace_tid34"), trace_tdest34("trace_tdest34"), trace_tvalid34("trace_tvalid34"), trace_tdata35("trace_tdata35"), trace_tready35("trace_tready35"), trace_tlast35("trace_tlast35"), trace_tid35("trace_tid35"), trace_tdest35("trace_tdest35"), trace_tvalid35("trace_tvalid35"), trace_tdata36("trace_tdata36"), trace_tready36("trace_tready36"), trace_tlast36("trace_tlast36"), trace_tid36("trace_tid36"), trace_tdest36("trace_tdest36"), trace_tvalid36("trace_tvalid36"), trace_tdata37("trace_tdata37"), trace_tready37("trace_tready37"), trace_tlast37("trace_tlast37"), trace_tid37("trace_tid37"), trace_tdest37("trace_tdest37"), trace_tvalid37("trace_tvalid37"), trace_tdata38("trace_tdata38"), trace_tready38("trace_tready38"), trace_tlast38("trace_tlast38"), trace_tid38("trace_tid38"), trace_tdest38("trace_tdest38"), trace_tvalid38("trace_tvalid38"), trace_tdata39("trace_tdata39"), trace_tready39("trace_tready39"), trace_tlast39("trace_tlast39"), trace_tid39("trace_tid39"), trace_tdest39("trace_tdest39"), trace_tvalid39("trace_tvalid39"), trace_tdata40("trace_tdata40"), trace_tready40("trace_tready40"), trace_tlast40("trace_tlast40"), trace_tid40("trace_tid40"), trace_tdest40("trace_tdest40"), trace_tvalid40("trace_tvalid40"), s_axi_awaddr("s_axi_awaddr"), s_axi_awvalid("s_axi_awvalid"), s_axi_awready("s_axi_awready"), s_axi_wdata("s_axi_wdata"), s_axi_wstrb("s_axi_wstrb"), s_axi_wvalid("s_axi_wvalid"), s_axi_wready("s_axi_wready"), s_axi_bvalid("s_axi_bvalid"), s_axi_bready("s_axi_bready"), s_axi_bresp("s_axi_bresp"), s_axi_araddr("s_axi_araddr"), s_axi_arvalid("s_axi_arvalid"), s_axi_arready("s_axi_arready"), s_axi_rdata("s_axi_rdata"), s_axi_rresp("s_axi_rresp"), s_axi_rvalid("s_axi_rvalid"), s_axi_rready("s_axi_rready"), s_axihub_awaddr("s_axihub_awaddr"), s_axihub_awvalid("s_axihub_awvalid"), s_axihub_awready("s_axihub_awready"), s_axihub_wdata("s_axihub_wdata"), s_axihub_wstrb("s_axihub_wstrb"), s_axihub_wvalid("s_axihub_wvalid"), s_axihub_wready("s_axihub_wready"), s_axihub_bvalid("s_axihub_bvalid"), s_axihub_bready("s_axihub_bready"), s_axihub_bresp("s_axihub_bresp"), s_axihub_araddr("s_axihub_araddr"), s_axihub_arvalid("s_axihub_arvalid"), s_axihub_arready("s_axihub_arready"), s_axihub_rdata("s_axihub_rdata"), s_axihub_rresp("s_axihub_rresp"), s_axihub_rvalid("s_axihub_rvalid"), s_axihub_rready("s_axihub_rready"), s_aximm_awaddr("s_aximm_awaddr"), s_aximm_awvalid("s_aximm_awvalid"), s_aximm_awready("s_aximm_awready"), s_aximm_awlen("s_aximm_awlen"), s_aximm_awsize("s_aximm_awsize"), s_aximm_awburst("s_aximm_awburst"), s_aximm_awid("s_aximm_awid"), s_aximm_wdata("s_aximm_wdata"), s_aximm_wstrb("s_aximm_wstrb"), s_aximm_wvalid("s_aximm_wvalid"), s_aximm_wready("s_aximm_wready"), s_aximm_bvalid("s_aximm_bvalid"), s_aximm_bready("s_aximm_bready"), s_aximm_wlast("s_aximm_wlast"), s_aximm_rlast("s_aximm_rlast"), s_aximm_bresp("s_aximm_bresp"), s_aximm_bid("s_aximm_bid"), s_aximm_araddr("s_aximm_araddr"), s_aximm_arvalid("s_aximm_arvalid"), s_aximm_arready("s_aximm_arready"), s_aximm_arlen("s_aximm_arlen"), s_aximm_arsize("s_aximm_arsize"), s_aximm_arburst("s_aximm_arburst"), s_aximm_arid("s_aximm_arid"), s_aximm_rdata("s_aximm_rdata"), s_aximm_rid("s_aximm_rid"), s_aximm_rresp("s_aximm_rresp"), s_aximm_rvalid("s_aximm_rvalid"), s_aximm_rready("s_aximm_rready")
{

  // initialize pins
  mp_impl->s_aximm_clk(s_aximm_clk);
  mp_impl->s_aximm_aresetn(s_aximm_aresetn);
  mp_impl->trace_clk(trace_clk);
  mp_impl->trace_aresetn(trace_aresetn);
  mp_impl->axilite_clk(axilite_clk);
  mp_impl->axilite_aresetn(axilite_aresetn);

  // initialize transactors
  mp_S_AXIMM_transactor = NULL;
  mp_S_AXIFIFO_transactor = NULL;
  mp_S_AXIHUB_transactor = NULL;
  mp_TRACE_IN_0_transactor = NULL;
  mp_TRACE_IN_1_transactor = NULL;
  mp_TRACE_IN_2_transactor = NULL;
  mp_TRACE_IN_3_transactor = NULL;
  mp_TRACE_IN_4_transactor = NULL;
  mp_TRACE_IN_5_transactor = NULL;
  mp_TRACE_IN_6_transactor = NULL;
  mp_TRACE_IN_7_transactor = NULL;
  mp_TRACE_IN_8_transactor = NULL;
  mp_TRACE_IN_9_transactor = NULL;
  mp_TRACE_IN_10_transactor = NULL;
  mp_TRACE_IN_11_transactor = NULL;
  mp_TRACE_IN_12_transactor = NULL;
  mp_TRACE_IN_13_transactor = NULL;
  mp_TRACE_IN_14_transactor = NULL;
  mp_TRACE_IN_15_transactor = NULL;
  mp_TRACE_IN_16_transactor = NULL;
  mp_TRACE_IN_17_transactor = NULL;
  mp_TRACE_IN_18_transactor = NULL;
  mp_TRACE_IN_19_transactor = NULL;
  mp_TRACE_IN_20_transactor = NULL;
  mp_TRACE_IN_21_transactor = NULL;
  mp_TRACE_IN_22_transactor = NULL;
  mp_TRACE_IN_23_transactor = NULL;
  mp_TRACE_IN_24_transactor = NULL;
  mp_TRACE_IN_25_transactor = NULL;
  mp_TRACE_IN_26_transactor = NULL;
  mp_TRACE_IN_27_transactor = NULL;
  mp_TRACE_IN_28_transactor = NULL;
  mp_TRACE_IN_29_transactor = NULL;
  mp_TRACE_IN_30_transactor = NULL;
  mp_TRACE_IN_31_transactor = NULL;
  mp_TRACE_IN_32_transactor = NULL;
  mp_TRACE_IN_33_transactor = NULL;
  mp_TRACE_IN_34_transactor = NULL;
  mp_TRACE_IN_35_transactor = NULL;
  mp_TRACE_IN_36_transactor = NULL;
  mp_TRACE_IN_37_transactor = NULL;
  mp_TRACE_IN_38_transactor = NULL;
  mp_TRACE_IN_39_transactor = NULL;
  mp_TRACE_IN_40_transactor = NULL;

  // initialize socket stubs

}

void pfm_dynamic_dpa_hub_0::before_end_of_elaboration()
{
  // configure 'S_AXIMM' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "S_AXIMM_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'S_AXIMM' transactor parameters
    xsc::common_cpp::properties S_AXIMM_transactor_param_props;
    S_AXIMM_transactor_param_props.addLong("DATA_WIDTH", "64");
    S_AXIMM_transactor_param_props.addLong("FREQ_HZ", "300000000");
    S_AXIMM_transactor_param_props.addLong("ID_WIDTH", "1");
    S_AXIMM_transactor_param_props.addLong("ADDR_WIDTH", "32");
    S_AXIMM_transactor_param_props.addLong("AWUSER_WIDTH", "0");
    S_AXIMM_transactor_param_props.addLong("ARUSER_WIDTH", "0");
    S_AXIMM_transactor_param_props.addLong("WUSER_WIDTH", "0");
    S_AXIMM_transactor_param_props.addLong("RUSER_WIDTH", "0");
    S_AXIMM_transactor_param_props.addLong("BUSER_WIDTH", "0");
    S_AXIMM_transactor_param_props.addLong("HAS_BURST", "1");
    S_AXIMM_transactor_param_props.addLong("HAS_LOCK", "0");
    S_AXIMM_transactor_param_props.addLong("HAS_PROT", "0");
    S_AXIMM_transactor_param_props.addLong("HAS_CACHE", "0");
    S_AXIMM_transactor_param_props.addLong("HAS_QOS", "0");
    S_AXIMM_transactor_param_props.addLong("HAS_REGION", "0");
    S_AXIMM_transactor_param_props.addLong("HAS_WSTRB", "1");
    S_AXIMM_transactor_param_props.addLong("HAS_BRESP", "1");
    S_AXIMM_transactor_param_props.addLong("HAS_RRESP", "1");
    S_AXIMM_transactor_param_props.addLong("SUPPORTS_NARROW_BURST", "0");
    S_AXIMM_transactor_param_props.addLong("NUM_READ_OUTSTANDING", "2");
    S_AXIMM_transactor_param_props.addLong("NUM_WRITE_OUTSTANDING", "2");
    S_AXIMM_transactor_param_props.addLong("MAX_BURST_LENGTH", "128");
    S_AXIMM_transactor_param_props.addLong("NUM_READ_THREADS", "1");
    S_AXIMM_transactor_param_props.addLong("NUM_WRITE_THREADS", "1");
    S_AXIMM_transactor_param_props.addLong("RUSER_BITS_PER_BYTE", "0");
    S_AXIMM_transactor_param_props.addLong("WUSER_BITS_PER_BYTE", "0");
    S_AXIMM_transactor_param_props.addLong("HAS_SIZE", "1");
    S_AXIMM_transactor_param_props.addLong("HAS_RESET", "1");
    S_AXIMM_transactor_param_props.addFloat("PHASE", "0.0");
    S_AXIMM_transactor_param_props.addString("PROTOCOL", "AXI4");
    S_AXIMM_transactor_param_props.addString("READ_WRITE_MODE", "READ_WRITE");
    S_AXIMM_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_dma_pcie_aclk");
    S_AXIMM_transactor_param_props.addString("SLR_ASSIGNMENT", "");

    mp_S_AXIMM_transactor = new xtlm::xaximm_pin2xtlm_t<64,32,1,1,1,1,1,1>("S_AXIMM_transactor", S_AXIMM_transactor_param_props);

    // S_AXIMM' transactor ports

    mp_S_AXIMM_transactor->ARADDR(s_aximm_araddr);
    mp_S_AXIMM_transactor->ARBURST(s_aximm_arburst);
    mp_S_AXIMM_transactor->ARID(s_aximm_arid);
    mp_S_AXIMM_transactor->ARLEN(s_aximm_arlen);
    mp_S_AXIMM_transactor->ARREADY(s_aximm_arready);
    mp_S_AXIMM_transactor->ARSIZE(s_aximm_arsize);
    mp_S_AXIMM_transactor->ARVALID(s_aximm_arvalid);
    mp_S_AXIMM_transactor->AWADDR(s_aximm_awaddr);
    mp_S_AXIMM_transactor->AWBURST(s_aximm_awburst);
    mp_S_AXIMM_transactor->AWID(s_aximm_awid);
    mp_S_AXIMM_transactor->AWLEN(s_aximm_awlen);
    mp_S_AXIMM_transactor->AWREADY(s_aximm_awready);
    mp_S_AXIMM_transactor->AWSIZE(s_aximm_awsize);
    mp_S_AXIMM_transactor->AWVALID(s_aximm_awvalid);
    mp_S_AXIMM_transactor->BID(s_aximm_bid);
    mp_S_AXIMM_transactor->BREADY(s_aximm_bready);
    mp_S_AXIMM_transactor->BRESP(s_aximm_bresp);
    mp_S_AXIMM_transactor->BVALID(s_aximm_bvalid);
    mp_S_AXIMM_transactor->RDATA(s_aximm_rdata);
    mp_S_AXIMM_transactor->RID(s_aximm_rid);
    mp_S_AXIMM_transactor->RLAST(s_aximm_rlast);
    mp_S_AXIMM_transactor->RREADY(s_aximm_rready);
    mp_S_AXIMM_transactor->RRESP(s_aximm_rresp);
    mp_S_AXIMM_transactor->RVALID(s_aximm_rvalid);
    mp_S_AXIMM_transactor->WDATA(s_aximm_wdata);
    mp_S_AXIMM_transactor->WLAST(s_aximm_wlast);
    mp_S_AXIMM_transactor->WREADY(s_aximm_wready);
    mp_S_AXIMM_transactor->WSTRB(s_aximm_wstrb);
    mp_S_AXIMM_transactor->WVALID(s_aximm_wvalid);
    mp_S_AXIMM_transactor->CLK(s_aximm_clk);
    mp_S_AXIMM_transactor->RST(s_aximm_aresetn);

    // S_AXIMM' transactor sockets

    mp_impl->s_aximm_rd_socket->bind(*(mp_S_AXIMM_transactor->rd_socket));
    mp_impl->s_aximm_wr_socket->bind(*(mp_S_AXIMM_transactor->wr_socket));
  }
  else
  {
  }

  // configure 'S_AXIFIFO' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "S_AXIFIFO_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'S_AXIFIFO' transactor parameters
    xsc::common_cpp::properties S_AXIFIFO_transactor_param_props;
    S_AXIFIFO_transactor_param_props.addLong("DATA_WIDTH", "32");
    S_AXIFIFO_transactor_param_props.addLong("FREQ_HZ", "300000000");
    S_AXIFIFO_transactor_param_props.addLong("ID_WIDTH", "0");
    S_AXIFIFO_transactor_param_props.addLong("ADDR_WIDTH", "8");
    S_AXIFIFO_transactor_param_props.addLong("AWUSER_WIDTH", "0");
    S_AXIFIFO_transactor_param_props.addLong("ARUSER_WIDTH", "0");
    S_AXIFIFO_transactor_param_props.addLong("WUSER_WIDTH", "0");
    S_AXIFIFO_transactor_param_props.addLong("RUSER_WIDTH", "0");
    S_AXIFIFO_transactor_param_props.addLong("BUSER_WIDTH", "0");
    S_AXIFIFO_transactor_param_props.addLong("HAS_BURST", "0");
    S_AXIFIFO_transactor_param_props.addLong("HAS_LOCK", "0");
    S_AXIFIFO_transactor_param_props.addLong("HAS_PROT", "0");
    S_AXIFIFO_transactor_param_props.addLong("HAS_CACHE", "0");
    S_AXIFIFO_transactor_param_props.addLong("HAS_QOS", "0");
    S_AXIFIFO_transactor_param_props.addLong("HAS_REGION", "0");
    S_AXIFIFO_transactor_param_props.addLong("HAS_WSTRB", "1");
    S_AXIFIFO_transactor_param_props.addLong("HAS_BRESP", "1");
    S_AXIFIFO_transactor_param_props.addLong("HAS_RRESP", "1");
    S_AXIFIFO_transactor_param_props.addLong("SUPPORTS_NARROW_BURST", "0");
    S_AXIFIFO_transactor_param_props.addLong("NUM_READ_OUTSTANDING", "2");
    S_AXIFIFO_transactor_param_props.addLong("NUM_WRITE_OUTSTANDING", "2");
    S_AXIFIFO_transactor_param_props.addLong("MAX_BURST_LENGTH", "1");
    S_AXIFIFO_transactor_param_props.addLong("NUM_READ_THREADS", "1");
    S_AXIFIFO_transactor_param_props.addLong("NUM_WRITE_THREADS", "1");
    S_AXIFIFO_transactor_param_props.addLong("RUSER_BITS_PER_BYTE", "0");
    S_AXIFIFO_transactor_param_props.addLong("WUSER_BITS_PER_BYTE", "0");
    S_AXIFIFO_transactor_param_props.addLong("HAS_SIZE", "0");
    S_AXIFIFO_transactor_param_props.addLong("HAS_RESET", "1");
    S_AXIFIFO_transactor_param_props.addFloat("PHASE", "0.0");
    S_AXIFIFO_transactor_param_props.addString("PROTOCOL", "AXI4LITE");
    S_AXIFIFO_transactor_param_props.addString("READ_WRITE_MODE", "READ_WRITE");
    S_AXIFIFO_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    S_AXIFIFO_transactor_param_props.addString("SLR_ASSIGNMENT", "");

    mp_S_AXIFIFO_transactor = new xtlm::xaximm_pin2xtlm_t<32,8,1,1,1,1,1,1>("S_AXIFIFO_transactor", S_AXIFIFO_transactor_param_props);

    // S_AXIFIFO' transactor ports

    mp_S_AXIFIFO_transactor->ARADDR(s_axi_araddr);
    mp_S_AXIFIFO_transactor->ARREADY(s_axi_arready);
    mp_S_AXIFIFO_transactor->ARVALID(s_axi_arvalid);
    mp_S_AXIFIFO_transactor->AWADDR(s_axi_awaddr);
    mp_S_AXIFIFO_transactor->AWREADY(s_axi_awready);
    mp_S_AXIFIFO_transactor->AWVALID(s_axi_awvalid);
    mp_S_AXIFIFO_transactor->BREADY(s_axi_bready);
    mp_S_AXIFIFO_transactor->BRESP(s_axi_bresp);
    mp_S_AXIFIFO_transactor->BVALID(s_axi_bvalid);
    mp_S_AXIFIFO_transactor->RDATA(s_axi_rdata);
    mp_S_AXIFIFO_transactor->RREADY(s_axi_rready);
    mp_S_AXIFIFO_transactor->RRESP(s_axi_rresp);
    mp_S_AXIFIFO_transactor->RVALID(s_axi_rvalid);
    mp_S_AXIFIFO_transactor->WDATA(s_axi_wdata);
    mp_S_AXIFIFO_transactor->WREADY(s_axi_wready);
    mp_S_AXIFIFO_transactor->WSTRB(s_axi_wstrb);
    mp_S_AXIFIFO_transactor->WVALID(s_axi_wvalid);
    mp_S_AXIFIFO_transactor->CLK(axilite_clk);
    mp_S_AXIFIFO_transactor->RST(axilite_aresetn);

    // S_AXIFIFO' transactor sockets

    mp_impl->axilite_rd_socket->bind(*(mp_S_AXIFIFO_transactor->rd_socket));
    mp_impl->axilite_wr_socket->bind(*(mp_S_AXIFIFO_transactor->wr_socket));
  }
  else
  {
  }

  // configure 'S_AXIHUB' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "S_AXIHUB_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'S_AXIHUB' transactor parameters
    xsc::common_cpp::properties S_AXIHUB_transactor_param_props;
    S_AXIHUB_transactor_param_props.addLong("DATA_WIDTH", "32");
    S_AXIHUB_transactor_param_props.addLong("FREQ_HZ", "300000000");
    S_AXIHUB_transactor_param_props.addLong("ID_WIDTH", "0");
    S_AXIHUB_transactor_param_props.addLong("ADDR_WIDTH", "8");
    S_AXIHUB_transactor_param_props.addLong("AWUSER_WIDTH", "0");
    S_AXIHUB_transactor_param_props.addLong("ARUSER_WIDTH", "0");
    S_AXIHUB_transactor_param_props.addLong("WUSER_WIDTH", "0");
    S_AXIHUB_transactor_param_props.addLong("RUSER_WIDTH", "0");
    S_AXIHUB_transactor_param_props.addLong("BUSER_WIDTH", "0");
    S_AXIHUB_transactor_param_props.addLong("HAS_BURST", "0");
    S_AXIHUB_transactor_param_props.addLong("HAS_LOCK", "0");
    S_AXIHUB_transactor_param_props.addLong("HAS_PROT", "0");
    S_AXIHUB_transactor_param_props.addLong("HAS_CACHE", "0");
    S_AXIHUB_transactor_param_props.addLong("HAS_QOS", "0");
    S_AXIHUB_transactor_param_props.addLong("HAS_REGION", "0");
    S_AXIHUB_transactor_param_props.addLong("HAS_WSTRB", "1");
    S_AXIHUB_transactor_param_props.addLong("HAS_BRESP", "1");
    S_AXIHUB_transactor_param_props.addLong("HAS_RRESP", "1");
    S_AXIHUB_transactor_param_props.addLong("SUPPORTS_NARROW_BURST", "0");
    S_AXIHUB_transactor_param_props.addLong("NUM_READ_OUTSTANDING", "2");
    S_AXIHUB_transactor_param_props.addLong("NUM_WRITE_OUTSTANDING", "2");
    S_AXIHUB_transactor_param_props.addLong("MAX_BURST_LENGTH", "1");
    S_AXIHUB_transactor_param_props.addLong("NUM_READ_THREADS", "1");
    S_AXIHUB_transactor_param_props.addLong("NUM_WRITE_THREADS", "1");
    S_AXIHUB_transactor_param_props.addLong("RUSER_BITS_PER_BYTE", "0");
    S_AXIHUB_transactor_param_props.addLong("WUSER_BITS_PER_BYTE", "0");
    S_AXIHUB_transactor_param_props.addLong("HAS_SIZE", "0");
    S_AXIHUB_transactor_param_props.addLong("HAS_RESET", "1");
    S_AXIHUB_transactor_param_props.addFloat("PHASE", "0.0");
    S_AXIHUB_transactor_param_props.addString("PROTOCOL", "AXI4LITE");
    S_AXIHUB_transactor_param_props.addString("READ_WRITE_MODE", "READ_WRITE");
    S_AXIHUB_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    S_AXIHUB_transactor_param_props.addString("SLR_ASSIGNMENT", "");

    mp_S_AXIHUB_transactor = new xtlm::xaximm_pin2xtlm_t<32,8,1,1,1,1,1,1>("S_AXIHUB_transactor", S_AXIHUB_transactor_param_props);

    // S_AXIHUB' transactor ports

    mp_S_AXIHUB_transactor->ARADDR(s_axihub_araddr);
    mp_S_AXIHUB_transactor->ARREADY(s_axihub_arready);
    mp_S_AXIHUB_transactor->ARVALID(s_axihub_arvalid);
    mp_S_AXIHUB_transactor->AWADDR(s_axihub_awaddr);
    mp_S_AXIHUB_transactor->AWREADY(s_axihub_awready);
    mp_S_AXIHUB_transactor->AWVALID(s_axihub_awvalid);
    mp_S_AXIHUB_transactor->BREADY(s_axihub_bready);
    mp_S_AXIHUB_transactor->BRESP(s_axihub_bresp);
    mp_S_AXIHUB_transactor->BVALID(s_axihub_bvalid);
    mp_S_AXIHUB_transactor->RDATA(s_axihub_rdata);
    mp_S_AXIHUB_transactor->RREADY(s_axihub_rready);
    mp_S_AXIHUB_transactor->RRESP(s_axihub_rresp);
    mp_S_AXIHUB_transactor->RVALID(s_axihub_rvalid);
    mp_S_AXIHUB_transactor->WDATA(s_axihub_wdata);
    mp_S_AXIHUB_transactor->WREADY(s_axihub_wready);
    mp_S_AXIHUB_transactor->WSTRB(s_axihub_wstrb);
    mp_S_AXIHUB_transactor->WVALID(s_axihub_wvalid);
    mp_S_AXIHUB_transactor->CLK(axilite_clk);
    mp_S_AXIHUB_transactor->RST(axilite_aresetn);

    // S_AXIHUB' transactor sockets

    mp_impl->s_axihub_rd_socket->bind(*(mp_S_AXIHUB_transactor->rd_socket));
    mp_impl->s_axihub_wr_socket->bind(*(mp_S_AXIHUB_transactor->wr_socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_0' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_0_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_0' transactor parameters
    xsc::common_cpp::properties TRACE_IN_0_transactor_param_props;
    TRACE_IN_0_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_0_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_0_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_0_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_0_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_0_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_0_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_0_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_0_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_0_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_0_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_0_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_0_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_0_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_0_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_0_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_0_transactor", TRACE_IN_0_transactor_param_props);

    // TRACE_IN_0' transactor ports

    mp_TRACE_IN_0_transactor->TDATA(trace_tdata0);
    mp_TRACE_IN_0_transactor->TDEST(trace_tdest0);
    mp_TRACE_IN_0_transactor->TID(trace_tid0);
    mp_TRACE_IN_0_transactor->TLAST(trace_tlast0);
    mp_TRACE_IN_0_transactor->TREADY(trace_tready0);
    mp_TRACE_IN_0_transactor->TVALID(trace_tvalid0);
    mp_TRACE_IN_0_transactor->CLK(trace_clk);
    mp_TRACE_IN_0_transactor->RST(trace_aresetn);

    // TRACE_IN_0' transactor sockets

    mp_impl->trace_socket0->bind(*(mp_TRACE_IN_0_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_1' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_1_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_1' transactor parameters
    xsc::common_cpp::properties TRACE_IN_1_transactor_param_props;
    TRACE_IN_1_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_1_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_1_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_1_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_1_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_1_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_1_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_1_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_1_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_1_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_1_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_1_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_1_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_1_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_1_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_1_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_1_transactor", TRACE_IN_1_transactor_param_props);

    // TRACE_IN_1' transactor ports

    mp_TRACE_IN_1_transactor->TDATA(trace_tdata1);
    mp_TRACE_IN_1_transactor->TDEST(trace_tdest1);
    mp_TRACE_IN_1_transactor->TID(trace_tid1);
    mp_TRACE_IN_1_transactor->TLAST(trace_tlast1);
    mp_TRACE_IN_1_transactor->TREADY(trace_tready1);
    mp_TRACE_IN_1_transactor->TVALID(trace_tvalid1);
    mp_TRACE_IN_1_transactor->CLK(trace_clk);
    mp_TRACE_IN_1_transactor->RST(trace_aresetn);

    // TRACE_IN_1' transactor sockets

    mp_impl->trace_socket1->bind(*(mp_TRACE_IN_1_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_2' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_2_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_2' transactor parameters
    xsc::common_cpp::properties TRACE_IN_2_transactor_param_props;
    TRACE_IN_2_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_2_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_2_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_2_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_2_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_2_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_2_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_2_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_2_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_2_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_2_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_2_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_2_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_2_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_2_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_2_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_2_transactor", TRACE_IN_2_transactor_param_props);

    // TRACE_IN_2' transactor ports

    mp_TRACE_IN_2_transactor->TDATA(trace_tdata2);
    mp_TRACE_IN_2_transactor->TDEST(trace_tdest2);
    mp_TRACE_IN_2_transactor->TID(trace_tid2);
    mp_TRACE_IN_2_transactor->TLAST(trace_tlast2);
    mp_TRACE_IN_2_transactor->TREADY(trace_tready2);
    mp_TRACE_IN_2_transactor->TVALID(trace_tvalid2);
    mp_TRACE_IN_2_transactor->CLK(trace_clk);
    mp_TRACE_IN_2_transactor->RST(trace_aresetn);

    // TRACE_IN_2' transactor sockets

    mp_impl->trace_socket2->bind(*(mp_TRACE_IN_2_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_3' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_3_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_3' transactor parameters
    xsc::common_cpp::properties TRACE_IN_3_transactor_param_props;
    TRACE_IN_3_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_3_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_3_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_3_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_3_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_3_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_3_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_3_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_3_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_3_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_3_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_3_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_3_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_3_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_3_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_3_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_3_transactor", TRACE_IN_3_transactor_param_props);

    // TRACE_IN_3' transactor ports

    mp_TRACE_IN_3_transactor->TDATA(trace_tdata3);
    mp_TRACE_IN_3_transactor->TDEST(trace_tdest3);
    mp_TRACE_IN_3_transactor->TID(trace_tid3);
    mp_TRACE_IN_3_transactor->TLAST(trace_tlast3);
    mp_TRACE_IN_3_transactor->TREADY(trace_tready3);
    mp_TRACE_IN_3_transactor->TVALID(trace_tvalid3);
    mp_TRACE_IN_3_transactor->CLK(trace_clk);
    mp_TRACE_IN_3_transactor->RST(trace_aresetn);

    // TRACE_IN_3' transactor sockets

    mp_impl->trace_socket3->bind(*(mp_TRACE_IN_3_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_4' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_4_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_4' transactor parameters
    xsc::common_cpp::properties TRACE_IN_4_transactor_param_props;
    TRACE_IN_4_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_4_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_4_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_4_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_4_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_4_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_4_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_4_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_4_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_4_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_4_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_4_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_4_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_4_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_4_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_4_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_4_transactor", TRACE_IN_4_transactor_param_props);

    // TRACE_IN_4' transactor ports

    mp_TRACE_IN_4_transactor->TDATA(trace_tdata4);
    mp_TRACE_IN_4_transactor->TDEST(trace_tdest4);
    mp_TRACE_IN_4_transactor->TID(trace_tid4);
    mp_TRACE_IN_4_transactor->TLAST(trace_tlast4);
    mp_TRACE_IN_4_transactor->TREADY(trace_tready4);
    mp_TRACE_IN_4_transactor->TVALID(trace_tvalid4);
    mp_TRACE_IN_4_transactor->CLK(trace_clk);
    mp_TRACE_IN_4_transactor->RST(trace_aresetn);

    // TRACE_IN_4' transactor sockets

    mp_impl->trace_socket4->bind(*(mp_TRACE_IN_4_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_5' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_5_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_5' transactor parameters
    xsc::common_cpp::properties TRACE_IN_5_transactor_param_props;
    TRACE_IN_5_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_5_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_5_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_5_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_5_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_5_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_5_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_5_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_5_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_5_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_5_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_5_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_5_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_5_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_5_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_5_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_5_transactor", TRACE_IN_5_transactor_param_props);

    // TRACE_IN_5' transactor ports

    mp_TRACE_IN_5_transactor->TDATA(trace_tdata5);
    mp_TRACE_IN_5_transactor->TDEST(trace_tdest5);
    mp_TRACE_IN_5_transactor->TID(trace_tid5);
    mp_TRACE_IN_5_transactor->TLAST(trace_tlast5);
    mp_TRACE_IN_5_transactor->TREADY(trace_tready5);
    mp_TRACE_IN_5_transactor->TVALID(trace_tvalid5);
    mp_TRACE_IN_5_transactor->CLK(trace_clk);
    mp_TRACE_IN_5_transactor->RST(trace_aresetn);

    // TRACE_IN_5' transactor sockets

    mp_impl->trace_socket5->bind(*(mp_TRACE_IN_5_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_6' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_6_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_6' transactor parameters
    xsc::common_cpp::properties TRACE_IN_6_transactor_param_props;
    TRACE_IN_6_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_6_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_6_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_6_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_6_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_6_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_6_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_6_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_6_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_6_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_6_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_6_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_6_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_6_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_6_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_6_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_6_transactor", TRACE_IN_6_transactor_param_props);

    // TRACE_IN_6' transactor ports

    mp_TRACE_IN_6_transactor->TDATA(trace_tdata6);
    mp_TRACE_IN_6_transactor->TDEST(trace_tdest6);
    mp_TRACE_IN_6_transactor->TID(trace_tid6);
    mp_TRACE_IN_6_transactor->TLAST(trace_tlast6);
    mp_TRACE_IN_6_transactor->TREADY(trace_tready6);
    mp_TRACE_IN_6_transactor->TVALID(trace_tvalid6);
    mp_TRACE_IN_6_transactor->CLK(trace_clk);
    mp_TRACE_IN_6_transactor->RST(trace_aresetn);

    // TRACE_IN_6' transactor sockets

    mp_impl->trace_socket6->bind(*(mp_TRACE_IN_6_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_7' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_7_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_7' transactor parameters
    xsc::common_cpp::properties TRACE_IN_7_transactor_param_props;
    TRACE_IN_7_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_7_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_7_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_7_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_7_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_7_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_7_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_7_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_7_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_7_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_7_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_7_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_7_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_7_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_7_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_7_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_7_transactor", TRACE_IN_7_transactor_param_props);

    // TRACE_IN_7' transactor ports

    mp_TRACE_IN_7_transactor->TDATA(trace_tdata7);
    mp_TRACE_IN_7_transactor->TDEST(trace_tdest7);
    mp_TRACE_IN_7_transactor->TID(trace_tid7);
    mp_TRACE_IN_7_transactor->TLAST(trace_tlast7);
    mp_TRACE_IN_7_transactor->TREADY(trace_tready7);
    mp_TRACE_IN_7_transactor->TVALID(trace_tvalid7);
    mp_TRACE_IN_7_transactor->CLK(trace_clk);
    mp_TRACE_IN_7_transactor->RST(trace_aresetn);

    // TRACE_IN_7' transactor sockets

    mp_impl->trace_socket7->bind(*(mp_TRACE_IN_7_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_8' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_8_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_8' transactor parameters
    xsc::common_cpp::properties TRACE_IN_8_transactor_param_props;
    TRACE_IN_8_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_8_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_8_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_8_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_8_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_8_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_8_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_8_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_8_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_8_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_8_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_8_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_8_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_8_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_8_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_8_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_8_transactor", TRACE_IN_8_transactor_param_props);

    // TRACE_IN_8' transactor ports

    mp_TRACE_IN_8_transactor->TDATA(trace_tdata8);
    mp_TRACE_IN_8_transactor->TDEST(trace_tdest8);
    mp_TRACE_IN_8_transactor->TID(trace_tid8);
    mp_TRACE_IN_8_transactor->TLAST(trace_tlast8);
    mp_TRACE_IN_8_transactor->TREADY(trace_tready8);
    mp_TRACE_IN_8_transactor->TVALID(trace_tvalid8);
    mp_TRACE_IN_8_transactor->CLK(trace_clk);
    mp_TRACE_IN_8_transactor->RST(trace_aresetn);

    // TRACE_IN_8' transactor sockets

    mp_impl->trace_socket8->bind(*(mp_TRACE_IN_8_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_9' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_9_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_9' transactor parameters
    xsc::common_cpp::properties TRACE_IN_9_transactor_param_props;
    TRACE_IN_9_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_9_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_9_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_9_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_9_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_9_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_9_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_9_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_9_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_9_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_9_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_9_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_9_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_9_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_9_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_9_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_9_transactor", TRACE_IN_9_transactor_param_props);

    // TRACE_IN_9' transactor ports

    mp_TRACE_IN_9_transactor->TDATA(trace_tdata9);
    mp_TRACE_IN_9_transactor->TDEST(trace_tdest9);
    mp_TRACE_IN_9_transactor->TID(trace_tid9);
    mp_TRACE_IN_9_transactor->TLAST(trace_tlast9);
    mp_TRACE_IN_9_transactor->TREADY(trace_tready9);
    mp_TRACE_IN_9_transactor->TVALID(trace_tvalid9);
    mp_TRACE_IN_9_transactor->CLK(trace_clk);
    mp_TRACE_IN_9_transactor->RST(trace_aresetn);

    // TRACE_IN_9' transactor sockets

    mp_impl->trace_socket9->bind(*(mp_TRACE_IN_9_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_10' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_10_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_10' transactor parameters
    xsc::common_cpp::properties TRACE_IN_10_transactor_param_props;
    TRACE_IN_10_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_10_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_10_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_10_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_10_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_10_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_10_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_10_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_10_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_10_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_10_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_10_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_10_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_10_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_10_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_10_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_10_transactor", TRACE_IN_10_transactor_param_props);

    // TRACE_IN_10' transactor ports

    mp_TRACE_IN_10_transactor->TDATA(trace_tdata10);
    mp_TRACE_IN_10_transactor->TDEST(trace_tdest10);
    mp_TRACE_IN_10_transactor->TID(trace_tid10);
    mp_TRACE_IN_10_transactor->TLAST(trace_tlast10);
    mp_TRACE_IN_10_transactor->TREADY(trace_tready10);
    mp_TRACE_IN_10_transactor->TVALID(trace_tvalid10);
    mp_TRACE_IN_10_transactor->CLK(trace_clk);
    mp_TRACE_IN_10_transactor->RST(trace_aresetn);

    // TRACE_IN_10' transactor sockets

    mp_impl->trace_socket10->bind(*(mp_TRACE_IN_10_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_11' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_11_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_11' transactor parameters
    xsc::common_cpp::properties TRACE_IN_11_transactor_param_props;
    TRACE_IN_11_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_11_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_11_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_11_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_11_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_11_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_11_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_11_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_11_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_11_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_11_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_11_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_11_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_11_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_11_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_11_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_11_transactor", TRACE_IN_11_transactor_param_props);

    // TRACE_IN_11' transactor ports

    mp_TRACE_IN_11_transactor->TDATA(trace_tdata11);
    mp_TRACE_IN_11_transactor->TDEST(trace_tdest11);
    mp_TRACE_IN_11_transactor->TID(trace_tid11);
    mp_TRACE_IN_11_transactor->TLAST(trace_tlast11);
    mp_TRACE_IN_11_transactor->TREADY(trace_tready11);
    mp_TRACE_IN_11_transactor->TVALID(trace_tvalid11);
    mp_TRACE_IN_11_transactor->CLK(trace_clk);
    mp_TRACE_IN_11_transactor->RST(trace_aresetn);

    // TRACE_IN_11' transactor sockets

    mp_impl->trace_socket11->bind(*(mp_TRACE_IN_11_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_12' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_12_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_12' transactor parameters
    xsc::common_cpp::properties TRACE_IN_12_transactor_param_props;
    TRACE_IN_12_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_12_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_12_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_12_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_12_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_12_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_12_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_12_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_12_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_12_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_12_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_12_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_12_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_12_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_12_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_12_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_12_transactor", TRACE_IN_12_transactor_param_props);

    // TRACE_IN_12' transactor ports

    mp_TRACE_IN_12_transactor->TDATA(trace_tdata12);
    mp_TRACE_IN_12_transactor->TDEST(trace_tdest12);
    mp_TRACE_IN_12_transactor->TID(trace_tid12);
    mp_TRACE_IN_12_transactor->TLAST(trace_tlast12);
    mp_TRACE_IN_12_transactor->TREADY(trace_tready12);
    mp_TRACE_IN_12_transactor->TVALID(trace_tvalid12);
    mp_TRACE_IN_12_transactor->CLK(trace_clk);
    mp_TRACE_IN_12_transactor->RST(trace_aresetn);

    // TRACE_IN_12' transactor sockets

    mp_impl->trace_socket12->bind(*(mp_TRACE_IN_12_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_13' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_13_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_13' transactor parameters
    xsc::common_cpp::properties TRACE_IN_13_transactor_param_props;
    TRACE_IN_13_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_13_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_13_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_13_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_13_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_13_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_13_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_13_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_13_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_13_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_13_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_13_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_13_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_13_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_13_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_13_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_13_transactor", TRACE_IN_13_transactor_param_props);

    // TRACE_IN_13' transactor ports

    mp_TRACE_IN_13_transactor->TDATA(trace_tdata13);
    mp_TRACE_IN_13_transactor->TDEST(trace_tdest13);
    mp_TRACE_IN_13_transactor->TID(trace_tid13);
    mp_TRACE_IN_13_transactor->TLAST(trace_tlast13);
    mp_TRACE_IN_13_transactor->TREADY(trace_tready13);
    mp_TRACE_IN_13_transactor->TVALID(trace_tvalid13);
    mp_TRACE_IN_13_transactor->CLK(trace_clk);
    mp_TRACE_IN_13_transactor->RST(trace_aresetn);

    // TRACE_IN_13' transactor sockets

    mp_impl->trace_socket13->bind(*(mp_TRACE_IN_13_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_14' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_14_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_14' transactor parameters
    xsc::common_cpp::properties TRACE_IN_14_transactor_param_props;
    TRACE_IN_14_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_14_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_14_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_14_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_14_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_14_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_14_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_14_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_14_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_14_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_14_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_14_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_14_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_14_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_14_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_14_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_14_transactor", TRACE_IN_14_transactor_param_props);

    // TRACE_IN_14' transactor ports

    mp_TRACE_IN_14_transactor->TDATA(trace_tdata14);
    mp_TRACE_IN_14_transactor->TDEST(trace_tdest14);
    mp_TRACE_IN_14_transactor->TID(trace_tid14);
    mp_TRACE_IN_14_transactor->TLAST(trace_tlast14);
    mp_TRACE_IN_14_transactor->TREADY(trace_tready14);
    mp_TRACE_IN_14_transactor->TVALID(trace_tvalid14);
    mp_TRACE_IN_14_transactor->CLK(trace_clk);
    mp_TRACE_IN_14_transactor->RST(trace_aresetn);

    // TRACE_IN_14' transactor sockets

    mp_impl->trace_socket14->bind(*(mp_TRACE_IN_14_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_15' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_15_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_15' transactor parameters
    xsc::common_cpp::properties TRACE_IN_15_transactor_param_props;
    TRACE_IN_15_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_15_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_15_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_15_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_15_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_15_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_15_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_15_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_15_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_15_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_15_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_15_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_15_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_15_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_15_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_15_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_15_transactor", TRACE_IN_15_transactor_param_props);

    // TRACE_IN_15' transactor ports

    mp_TRACE_IN_15_transactor->TDATA(trace_tdata15);
    mp_TRACE_IN_15_transactor->TDEST(trace_tdest15);
    mp_TRACE_IN_15_transactor->TID(trace_tid15);
    mp_TRACE_IN_15_transactor->TLAST(trace_tlast15);
    mp_TRACE_IN_15_transactor->TREADY(trace_tready15);
    mp_TRACE_IN_15_transactor->TVALID(trace_tvalid15);
    mp_TRACE_IN_15_transactor->CLK(trace_clk);
    mp_TRACE_IN_15_transactor->RST(trace_aresetn);

    // TRACE_IN_15' transactor sockets

    mp_impl->trace_socket15->bind(*(mp_TRACE_IN_15_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_16' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_16_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_16' transactor parameters
    xsc::common_cpp::properties TRACE_IN_16_transactor_param_props;
    TRACE_IN_16_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_16_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_16_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_16_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_16_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_16_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_16_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_16_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_16_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_16_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_16_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_16_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_16_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_16_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_16_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_16_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_16_transactor", TRACE_IN_16_transactor_param_props);

    // TRACE_IN_16' transactor ports

    mp_TRACE_IN_16_transactor->TDATA(trace_tdata16);
    mp_TRACE_IN_16_transactor->TDEST(trace_tdest16);
    mp_TRACE_IN_16_transactor->TID(trace_tid16);
    mp_TRACE_IN_16_transactor->TLAST(trace_tlast16);
    mp_TRACE_IN_16_transactor->TREADY(trace_tready16);
    mp_TRACE_IN_16_transactor->TVALID(trace_tvalid16);
    mp_TRACE_IN_16_transactor->CLK(trace_clk);
    mp_TRACE_IN_16_transactor->RST(trace_aresetn);

    // TRACE_IN_16' transactor sockets

    mp_impl->trace_socket16->bind(*(mp_TRACE_IN_16_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_17' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_17_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_17' transactor parameters
    xsc::common_cpp::properties TRACE_IN_17_transactor_param_props;
    TRACE_IN_17_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_17_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_17_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_17_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_17_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_17_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_17_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_17_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_17_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_17_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_17_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_17_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_17_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_17_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_17_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_17_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_17_transactor", TRACE_IN_17_transactor_param_props);

    // TRACE_IN_17' transactor ports

    mp_TRACE_IN_17_transactor->TDATA(trace_tdata17);
    mp_TRACE_IN_17_transactor->TDEST(trace_tdest17);
    mp_TRACE_IN_17_transactor->TID(trace_tid17);
    mp_TRACE_IN_17_transactor->TLAST(trace_tlast17);
    mp_TRACE_IN_17_transactor->TREADY(trace_tready17);
    mp_TRACE_IN_17_transactor->TVALID(trace_tvalid17);
    mp_TRACE_IN_17_transactor->CLK(trace_clk);
    mp_TRACE_IN_17_transactor->RST(trace_aresetn);

    // TRACE_IN_17' transactor sockets

    mp_impl->trace_socket17->bind(*(mp_TRACE_IN_17_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_18' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_18_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_18' transactor parameters
    xsc::common_cpp::properties TRACE_IN_18_transactor_param_props;
    TRACE_IN_18_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_18_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_18_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_18_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_18_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_18_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_18_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_18_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_18_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_18_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_18_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_18_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_18_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_18_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_18_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_18_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_18_transactor", TRACE_IN_18_transactor_param_props);

    // TRACE_IN_18' transactor ports

    mp_TRACE_IN_18_transactor->TDATA(trace_tdata18);
    mp_TRACE_IN_18_transactor->TDEST(trace_tdest18);
    mp_TRACE_IN_18_transactor->TID(trace_tid18);
    mp_TRACE_IN_18_transactor->TLAST(trace_tlast18);
    mp_TRACE_IN_18_transactor->TREADY(trace_tready18);
    mp_TRACE_IN_18_transactor->TVALID(trace_tvalid18);
    mp_TRACE_IN_18_transactor->CLK(trace_clk);
    mp_TRACE_IN_18_transactor->RST(trace_aresetn);

    // TRACE_IN_18' transactor sockets

    mp_impl->trace_socket18->bind(*(mp_TRACE_IN_18_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_19' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_19_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_19' transactor parameters
    xsc::common_cpp::properties TRACE_IN_19_transactor_param_props;
    TRACE_IN_19_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_19_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_19_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_19_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_19_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_19_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_19_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_19_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_19_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_19_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_19_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_19_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_19_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_19_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_19_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_19_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_19_transactor", TRACE_IN_19_transactor_param_props);

    // TRACE_IN_19' transactor ports

    mp_TRACE_IN_19_transactor->TDATA(trace_tdata19);
    mp_TRACE_IN_19_transactor->TDEST(trace_tdest19);
    mp_TRACE_IN_19_transactor->TID(trace_tid19);
    mp_TRACE_IN_19_transactor->TLAST(trace_tlast19);
    mp_TRACE_IN_19_transactor->TREADY(trace_tready19);
    mp_TRACE_IN_19_transactor->TVALID(trace_tvalid19);
    mp_TRACE_IN_19_transactor->CLK(trace_clk);
    mp_TRACE_IN_19_transactor->RST(trace_aresetn);

    // TRACE_IN_19' transactor sockets

    mp_impl->trace_socket19->bind(*(mp_TRACE_IN_19_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_20' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_20_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_20' transactor parameters
    xsc::common_cpp::properties TRACE_IN_20_transactor_param_props;
    TRACE_IN_20_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_20_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_20_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_20_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_20_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_20_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_20_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_20_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_20_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_20_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_20_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_20_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_20_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_20_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_20_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_20_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_20_transactor", TRACE_IN_20_transactor_param_props);

    // TRACE_IN_20' transactor ports

    mp_TRACE_IN_20_transactor->TDATA(trace_tdata20);
    mp_TRACE_IN_20_transactor->TDEST(trace_tdest20);
    mp_TRACE_IN_20_transactor->TID(trace_tid20);
    mp_TRACE_IN_20_transactor->TLAST(trace_tlast20);
    mp_TRACE_IN_20_transactor->TREADY(trace_tready20);
    mp_TRACE_IN_20_transactor->TVALID(trace_tvalid20);
    mp_TRACE_IN_20_transactor->CLK(trace_clk);
    mp_TRACE_IN_20_transactor->RST(trace_aresetn);

    // TRACE_IN_20' transactor sockets

    mp_impl->trace_socket20->bind(*(mp_TRACE_IN_20_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_21' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_21_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_21' transactor parameters
    xsc::common_cpp::properties TRACE_IN_21_transactor_param_props;
    TRACE_IN_21_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_21_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_21_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_21_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_21_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_21_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_21_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_21_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_21_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_21_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_21_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_21_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_21_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_21_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_21_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_21_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_21_transactor", TRACE_IN_21_transactor_param_props);

    // TRACE_IN_21' transactor ports

    mp_TRACE_IN_21_transactor->TDATA(trace_tdata21);
    mp_TRACE_IN_21_transactor->TDEST(trace_tdest21);
    mp_TRACE_IN_21_transactor->TID(trace_tid21);
    mp_TRACE_IN_21_transactor->TLAST(trace_tlast21);
    mp_TRACE_IN_21_transactor->TREADY(trace_tready21);
    mp_TRACE_IN_21_transactor->TVALID(trace_tvalid21);
    mp_TRACE_IN_21_transactor->CLK(trace_clk);
    mp_TRACE_IN_21_transactor->RST(trace_aresetn);

    // TRACE_IN_21' transactor sockets

    mp_impl->trace_socket21->bind(*(mp_TRACE_IN_21_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_22' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_22_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_22' transactor parameters
    xsc::common_cpp::properties TRACE_IN_22_transactor_param_props;
    TRACE_IN_22_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_22_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_22_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_22_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_22_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_22_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_22_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_22_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_22_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_22_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_22_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_22_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_22_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_22_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_22_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_22_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_22_transactor", TRACE_IN_22_transactor_param_props);

    // TRACE_IN_22' transactor ports

    mp_TRACE_IN_22_transactor->TDATA(trace_tdata22);
    mp_TRACE_IN_22_transactor->TDEST(trace_tdest22);
    mp_TRACE_IN_22_transactor->TID(trace_tid22);
    mp_TRACE_IN_22_transactor->TLAST(trace_tlast22);
    mp_TRACE_IN_22_transactor->TREADY(trace_tready22);
    mp_TRACE_IN_22_transactor->TVALID(trace_tvalid22);
    mp_TRACE_IN_22_transactor->CLK(trace_clk);
    mp_TRACE_IN_22_transactor->RST(trace_aresetn);

    // TRACE_IN_22' transactor sockets

    mp_impl->trace_socket22->bind(*(mp_TRACE_IN_22_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_23' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_23_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_23' transactor parameters
    xsc::common_cpp::properties TRACE_IN_23_transactor_param_props;
    TRACE_IN_23_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_23_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_23_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_23_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_23_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_23_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_23_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_23_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_23_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_23_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_23_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_23_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_23_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_23_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_23_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_23_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_23_transactor", TRACE_IN_23_transactor_param_props);

    // TRACE_IN_23' transactor ports

    mp_TRACE_IN_23_transactor->TDATA(trace_tdata23);
    mp_TRACE_IN_23_transactor->TDEST(trace_tdest23);
    mp_TRACE_IN_23_transactor->TID(trace_tid23);
    mp_TRACE_IN_23_transactor->TLAST(trace_tlast23);
    mp_TRACE_IN_23_transactor->TREADY(trace_tready23);
    mp_TRACE_IN_23_transactor->TVALID(trace_tvalid23);
    mp_TRACE_IN_23_transactor->CLK(trace_clk);
    mp_TRACE_IN_23_transactor->RST(trace_aresetn);

    // TRACE_IN_23' transactor sockets

    mp_impl->trace_socket23->bind(*(mp_TRACE_IN_23_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_24' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_24_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_24' transactor parameters
    xsc::common_cpp::properties TRACE_IN_24_transactor_param_props;
    TRACE_IN_24_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_24_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_24_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_24_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_24_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_24_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_24_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_24_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_24_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_24_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_24_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_24_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_24_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_24_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_24_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_24_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_24_transactor", TRACE_IN_24_transactor_param_props);

    // TRACE_IN_24' transactor ports

    mp_TRACE_IN_24_transactor->TDATA(trace_tdata24);
    mp_TRACE_IN_24_transactor->TDEST(trace_tdest24);
    mp_TRACE_IN_24_transactor->TID(trace_tid24);
    mp_TRACE_IN_24_transactor->TLAST(trace_tlast24);
    mp_TRACE_IN_24_transactor->TREADY(trace_tready24);
    mp_TRACE_IN_24_transactor->TVALID(trace_tvalid24);
    mp_TRACE_IN_24_transactor->CLK(trace_clk);
    mp_TRACE_IN_24_transactor->RST(trace_aresetn);

    // TRACE_IN_24' transactor sockets

    mp_impl->trace_socket24->bind(*(mp_TRACE_IN_24_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_25' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_25_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_25' transactor parameters
    xsc::common_cpp::properties TRACE_IN_25_transactor_param_props;
    TRACE_IN_25_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_25_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_25_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_25_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_25_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_25_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_25_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_25_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_25_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_25_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_25_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_25_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_25_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_25_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_25_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_25_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_25_transactor", TRACE_IN_25_transactor_param_props);

    // TRACE_IN_25' transactor ports

    mp_TRACE_IN_25_transactor->TDATA(trace_tdata25);
    mp_TRACE_IN_25_transactor->TDEST(trace_tdest25);
    mp_TRACE_IN_25_transactor->TID(trace_tid25);
    mp_TRACE_IN_25_transactor->TLAST(trace_tlast25);
    mp_TRACE_IN_25_transactor->TREADY(trace_tready25);
    mp_TRACE_IN_25_transactor->TVALID(trace_tvalid25);
    mp_TRACE_IN_25_transactor->CLK(trace_clk);
    mp_TRACE_IN_25_transactor->RST(trace_aresetn);

    // TRACE_IN_25' transactor sockets

    mp_impl->trace_socket25->bind(*(mp_TRACE_IN_25_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_26' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_26_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_26' transactor parameters
    xsc::common_cpp::properties TRACE_IN_26_transactor_param_props;
    TRACE_IN_26_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_26_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_26_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_26_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_26_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_26_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_26_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_26_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_26_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_26_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_26_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_26_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_26_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_26_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_26_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_26_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_26_transactor", TRACE_IN_26_transactor_param_props);

    // TRACE_IN_26' transactor ports

    mp_TRACE_IN_26_transactor->TDATA(trace_tdata26);
    mp_TRACE_IN_26_transactor->TDEST(trace_tdest26);
    mp_TRACE_IN_26_transactor->TID(trace_tid26);
    mp_TRACE_IN_26_transactor->TLAST(trace_tlast26);
    mp_TRACE_IN_26_transactor->TREADY(trace_tready26);
    mp_TRACE_IN_26_transactor->TVALID(trace_tvalid26);
    mp_TRACE_IN_26_transactor->CLK(trace_clk);
    mp_TRACE_IN_26_transactor->RST(trace_aresetn);

    // TRACE_IN_26' transactor sockets

    mp_impl->trace_socket26->bind(*(mp_TRACE_IN_26_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_27' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_27_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_27' transactor parameters
    xsc::common_cpp::properties TRACE_IN_27_transactor_param_props;
    TRACE_IN_27_transactor_param_props.addLong("TDATA_NUM_BYTES", "1");
    TRACE_IN_27_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_27_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_27_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_27_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_27_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_27_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_27_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_27_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_27_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_27_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_27_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_27_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_27_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_27_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_27_transactor = new xtlm::xaxis_pin2xtlm_t<1,1,8,8,1,1>("TRACE_IN_27_transactor", TRACE_IN_27_transactor_param_props);

    // TRACE_IN_27' transactor ports

    mp_TRACE_IN_27_transactor->TDATA(trace_tdata27);
    mp_TRACE_IN_27_transactor->TDEST(trace_tdest27);
    mp_TRACE_IN_27_transactor->TID(trace_tid27);
    mp_TRACE_IN_27_transactor->TLAST(trace_tlast27);
    mp_TRACE_IN_27_transactor->TREADY(trace_tready27);
    mp_TRACE_IN_27_transactor->TVALID(trace_tvalid27);
    mp_TRACE_IN_27_transactor->CLK(trace_clk);
    mp_TRACE_IN_27_transactor->RST(trace_aresetn);

    // TRACE_IN_27' transactor sockets

    mp_impl->trace_socket27->bind(*(mp_TRACE_IN_27_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_28' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_28_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_28' transactor parameters
    xsc::common_cpp::properties TRACE_IN_28_transactor_param_props;
    TRACE_IN_28_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_28_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_28_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_28_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_28_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_28_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_28_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_28_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_28_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_28_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_28_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_28_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_28_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_28_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_28_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_28_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_28_transactor", TRACE_IN_28_transactor_param_props);

    // TRACE_IN_28' transactor ports

    mp_TRACE_IN_28_transactor->TDATA(trace_tdata28);
    mp_TRACE_IN_28_transactor->TDEST(trace_tdest28);
    mp_TRACE_IN_28_transactor->TID(trace_tid28);
    mp_TRACE_IN_28_transactor->TLAST(trace_tlast28);
    mp_TRACE_IN_28_transactor->TREADY(trace_tready28);
    mp_TRACE_IN_28_transactor->TVALID(trace_tvalid28);
    mp_TRACE_IN_28_transactor->CLK(trace_clk);
    mp_TRACE_IN_28_transactor->RST(trace_aresetn);

    // TRACE_IN_28' transactor sockets

    mp_impl->trace_socket28->bind(*(mp_TRACE_IN_28_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_29' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_29_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_29' transactor parameters
    xsc::common_cpp::properties TRACE_IN_29_transactor_param_props;
    TRACE_IN_29_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_29_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_29_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_29_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_29_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_29_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_29_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_29_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_29_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_29_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_29_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_29_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_29_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_29_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_29_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_29_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_29_transactor", TRACE_IN_29_transactor_param_props);

    // TRACE_IN_29' transactor ports

    mp_TRACE_IN_29_transactor->TDATA(trace_tdata29);
    mp_TRACE_IN_29_transactor->TDEST(trace_tdest29);
    mp_TRACE_IN_29_transactor->TID(trace_tid29);
    mp_TRACE_IN_29_transactor->TLAST(trace_tlast29);
    mp_TRACE_IN_29_transactor->TREADY(trace_tready29);
    mp_TRACE_IN_29_transactor->TVALID(trace_tvalid29);
    mp_TRACE_IN_29_transactor->CLK(trace_clk);
    mp_TRACE_IN_29_transactor->RST(trace_aresetn);

    // TRACE_IN_29' transactor sockets

    mp_impl->trace_socket29->bind(*(mp_TRACE_IN_29_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_30' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_30_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_30' transactor parameters
    xsc::common_cpp::properties TRACE_IN_30_transactor_param_props;
    TRACE_IN_30_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_30_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_30_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_30_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_30_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_30_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_30_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_30_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_30_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_30_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_30_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_30_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_30_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_30_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_30_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_30_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_30_transactor", TRACE_IN_30_transactor_param_props);

    // TRACE_IN_30' transactor ports

    mp_TRACE_IN_30_transactor->TDATA(trace_tdata30);
    mp_TRACE_IN_30_transactor->TDEST(trace_tdest30);
    mp_TRACE_IN_30_transactor->TID(trace_tid30);
    mp_TRACE_IN_30_transactor->TLAST(trace_tlast30);
    mp_TRACE_IN_30_transactor->TREADY(trace_tready30);
    mp_TRACE_IN_30_transactor->TVALID(trace_tvalid30);
    mp_TRACE_IN_30_transactor->CLK(trace_clk);
    mp_TRACE_IN_30_transactor->RST(trace_aresetn);

    // TRACE_IN_30' transactor sockets

    mp_impl->trace_socket30->bind(*(mp_TRACE_IN_30_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_31' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_31_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_31' transactor parameters
    xsc::common_cpp::properties TRACE_IN_31_transactor_param_props;
    TRACE_IN_31_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_31_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_31_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_31_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_31_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_31_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_31_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_31_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_31_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_31_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_31_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_31_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_31_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_31_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_31_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_31_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_31_transactor", TRACE_IN_31_transactor_param_props);

    // TRACE_IN_31' transactor ports

    mp_TRACE_IN_31_transactor->TDATA(trace_tdata31);
    mp_TRACE_IN_31_transactor->TDEST(trace_tdest31);
    mp_TRACE_IN_31_transactor->TID(trace_tid31);
    mp_TRACE_IN_31_transactor->TLAST(trace_tlast31);
    mp_TRACE_IN_31_transactor->TREADY(trace_tready31);
    mp_TRACE_IN_31_transactor->TVALID(trace_tvalid31);
    mp_TRACE_IN_31_transactor->CLK(trace_clk);
    mp_TRACE_IN_31_transactor->RST(trace_aresetn);

    // TRACE_IN_31' transactor sockets

    mp_impl->trace_socket31->bind(*(mp_TRACE_IN_31_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_32' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_32_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_32' transactor parameters
    xsc::common_cpp::properties TRACE_IN_32_transactor_param_props;
    TRACE_IN_32_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_32_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_32_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_32_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_32_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_32_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_32_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_32_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_32_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_32_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_32_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_32_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_32_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_32_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_32_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_32_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_32_transactor", TRACE_IN_32_transactor_param_props);

    // TRACE_IN_32' transactor ports

    mp_TRACE_IN_32_transactor->TDATA(trace_tdata32);
    mp_TRACE_IN_32_transactor->TDEST(trace_tdest32);
    mp_TRACE_IN_32_transactor->TID(trace_tid32);
    mp_TRACE_IN_32_transactor->TLAST(trace_tlast32);
    mp_TRACE_IN_32_transactor->TREADY(trace_tready32);
    mp_TRACE_IN_32_transactor->TVALID(trace_tvalid32);
    mp_TRACE_IN_32_transactor->CLK(trace_clk);
    mp_TRACE_IN_32_transactor->RST(trace_aresetn);

    // TRACE_IN_32' transactor sockets

    mp_impl->trace_socket32->bind(*(mp_TRACE_IN_32_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_33' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_33_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_33' transactor parameters
    xsc::common_cpp::properties TRACE_IN_33_transactor_param_props;
    TRACE_IN_33_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_33_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_33_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_33_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_33_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_33_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_33_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_33_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_33_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_33_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_33_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_33_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_33_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_33_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_33_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_33_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_33_transactor", TRACE_IN_33_transactor_param_props);

    // TRACE_IN_33' transactor ports

    mp_TRACE_IN_33_transactor->TDATA(trace_tdata33);
    mp_TRACE_IN_33_transactor->TDEST(trace_tdest33);
    mp_TRACE_IN_33_transactor->TID(trace_tid33);
    mp_TRACE_IN_33_transactor->TLAST(trace_tlast33);
    mp_TRACE_IN_33_transactor->TREADY(trace_tready33);
    mp_TRACE_IN_33_transactor->TVALID(trace_tvalid33);
    mp_TRACE_IN_33_transactor->CLK(trace_clk);
    mp_TRACE_IN_33_transactor->RST(trace_aresetn);

    // TRACE_IN_33' transactor sockets

    mp_impl->trace_socket33->bind(*(mp_TRACE_IN_33_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_34' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_34_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_34' transactor parameters
    xsc::common_cpp::properties TRACE_IN_34_transactor_param_props;
    TRACE_IN_34_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_34_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_34_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_34_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_34_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_34_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_34_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_34_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_34_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_34_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_34_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_34_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_34_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_34_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_34_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_34_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_34_transactor", TRACE_IN_34_transactor_param_props);

    // TRACE_IN_34' transactor ports

    mp_TRACE_IN_34_transactor->TDATA(trace_tdata34);
    mp_TRACE_IN_34_transactor->TDEST(trace_tdest34);
    mp_TRACE_IN_34_transactor->TID(trace_tid34);
    mp_TRACE_IN_34_transactor->TLAST(trace_tlast34);
    mp_TRACE_IN_34_transactor->TREADY(trace_tready34);
    mp_TRACE_IN_34_transactor->TVALID(trace_tvalid34);
    mp_TRACE_IN_34_transactor->CLK(trace_clk);
    mp_TRACE_IN_34_transactor->RST(trace_aresetn);

    // TRACE_IN_34' transactor sockets

    mp_impl->trace_socket34->bind(*(mp_TRACE_IN_34_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_35' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_35_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_35' transactor parameters
    xsc::common_cpp::properties TRACE_IN_35_transactor_param_props;
    TRACE_IN_35_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_35_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_35_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_35_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_35_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_35_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_35_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_35_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_35_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_35_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_35_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_35_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_35_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_35_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_35_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_35_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_35_transactor", TRACE_IN_35_transactor_param_props);

    // TRACE_IN_35' transactor ports

    mp_TRACE_IN_35_transactor->TDATA(trace_tdata35);
    mp_TRACE_IN_35_transactor->TDEST(trace_tdest35);
    mp_TRACE_IN_35_transactor->TID(trace_tid35);
    mp_TRACE_IN_35_transactor->TLAST(trace_tlast35);
    mp_TRACE_IN_35_transactor->TREADY(trace_tready35);
    mp_TRACE_IN_35_transactor->TVALID(trace_tvalid35);
    mp_TRACE_IN_35_transactor->CLK(trace_clk);
    mp_TRACE_IN_35_transactor->RST(trace_aresetn);

    // TRACE_IN_35' transactor sockets

    mp_impl->trace_socket35->bind(*(mp_TRACE_IN_35_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_36' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_36_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_36' transactor parameters
    xsc::common_cpp::properties TRACE_IN_36_transactor_param_props;
    TRACE_IN_36_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_36_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_36_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_36_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_36_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_36_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_36_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_36_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_36_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_36_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_36_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_36_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_36_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_36_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_36_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_36_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_36_transactor", TRACE_IN_36_transactor_param_props);

    // TRACE_IN_36' transactor ports

    mp_TRACE_IN_36_transactor->TDATA(trace_tdata36);
    mp_TRACE_IN_36_transactor->TDEST(trace_tdest36);
    mp_TRACE_IN_36_transactor->TID(trace_tid36);
    mp_TRACE_IN_36_transactor->TLAST(trace_tlast36);
    mp_TRACE_IN_36_transactor->TREADY(trace_tready36);
    mp_TRACE_IN_36_transactor->TVALID(trace_tvalid36);
    mp_TRACE_IN_36_transactor->CLK(trace_clk);
    mp_TRACE_IN_36_transactor->RST(trace_aresetn);

    // TRACE_IN_36' transactor sockets

    mp_impl->trace_socket36->bind(*(mp_TRACE_IN_36_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_37' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_37_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_37' transactor parameters
    xsc::common_cpp::properties TRACE_IN_37_transactor_param_props;
    TRACE_IN_37_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_37_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_37_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_37_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_37_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_37_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_37_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_37_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_37_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_37_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_37_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_37_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_37_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_37_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_37_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_37_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_37_transactor", TRACE_IN_37_transactor_param_props);

    // TRACE_IN_37' transactor ports

    mp_TRACE_IN_37_transactor->TDATA(trace_tdata37);
    mp_TRACE_IN_37_transactor->TDEST(trace_tdest37);
    mp_TRACE_IN_37_transactor->TID(trace_tid37);
    mp_TRACE_IN_37_transactor->TLAST(trace_tlast37);
    mp_TRACE_IN_37_transactor->TREADY(trace_tready37);
    mp_TRACE_IN_37_transactor->TVALID(trace_tvalid37);
    mp_TRACE_IN_37_transactor->CLK(trace_clk);
    mp_TRACE_IN_37_transactor->RST(trace_aresetn);

    // TRACE_IN_37' transactor sockets

    mp_impl->trace_socket37->bind(*(mp_TRACE_IN_37_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_38' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_38_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_38' transactor parameters
    xsc::common_cpp::properties TRACE_IN_38_transactor_param_props;
    TRACE_IN_38_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_38_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_38_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_38_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_38_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_38_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_38_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_38_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_38_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_38_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_38_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_38_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_38_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_38_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_38_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_38_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_38_transactor", TRACE_IN_38_transactor_param_props);

    // TRACE_IN_38' transactor ports

    mp_TRACE_IN_38_transactor->TDATA(trace_tdata38);
    mp_TRACE_IN_38_transactor->TDEST(trace_tdest38);
    mp_TRACE_IN_38_transactor->TID(trace_tid38);
    mp_TRACE_IN_38_transactor->TLAST(trace_tlast38);
    mp_TRACE_IN_38_transactor->TREADY(trace_tready38);
    mp_TRACE_IN_38_transactor->TVALID(trace_tvalid38);
    mp_TRACE_IN_38_transactor->CLK(trace_clk);
    mp_TRACE_IN_38_transactor->RST(trace_aresetn);

    // TRACE_IN_38' transactor sockets

    mp_impl->trace_socket38->bind(*(mp_TRACE_IN_38_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_39' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_39_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_39' transactor parameters
    xsc::common_cpp::properties TRACE_IN_39_transactor_param_props;
    TRACE_IN_39_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_39_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_39_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_39_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_39_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_39_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_39_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_39_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_39_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_39_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_39_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_39_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_39_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_39_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_39_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_39_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_39_transactor", TRACE_IN_39_transactor_param_props);

    // TRACE_IN_39' transactor ports

    mp_TRACE_IN_39_transactor->TDATA(trace_tdata39);
    mp_TRACE_IN_39_transactor->TDEST(trace_tdest39);
    mp_TRACE_IN_39_transactor->TID(trace_tid39);
    mp_TRACE_IN_39_transactor->TLAST(trace_tlast39);
    mp_TRACE_IN_39_transactor->TREADY(trace_tready39);
    mp_TRACE_IN_39_transactor->TVALID(trace_tvalid39);
    mp_TRACE_IN_39_transactor->CLK(trace_clk);
    mp_TRACE_IN_39_transactor->RST(trace_aresetn);

    // TRACE_IN_39' transactor sockets

    mp_impl->trace_socket39->bind(*(mp_TRACE_IN_39_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_40' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_40_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_40' transactor parameters
    xsc::common_cpp::properties TRACE_IN_40_transactor_param_props;
    TRACE_IN_40_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_40_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_40_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_40_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_40_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_40_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_40_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_40_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_40_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_40_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_40_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_40_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_40_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_40_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_40_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_40_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_40_transactor", TRACE_IN_40_transactor_param_props);

    // TRACE_IN_40' transactor ports

    mp_TRACE_IN_40_transactor->TDATA(trace_tdata40);
    mp_TRACE_IN_40_transactor->TDEST(trace_tdest40);
    mp_TRACE_IN_40_transactor->TID(trace_tid40);
    mp_TRACE_IN_40_transactor->TLAST(trace_tlast40);
    mp_TRACE_IN_40_transactor->TREADY(trace_tready40);
    mp_TRACE_IN_40_transactor->TVALID(trace_tvalid40);
    mp_TRACE_IN_40_transactor->CLK(trace_clk);
    mp_TRACE_IN_40_transactor->RST(trace_aresetn);

    // TRACE_IN_40' transactor sockets

    mp_impl->trace_socket40->bind(*(mp_TRACE_IN_40_transactor->socket));
  }
  else
  {
  }

}

#endif // XILINX_SIMULATOR




#ifdef XM_SYSTEMC
pfm_dynamic_dpa_hub_0::pfm_dynamic_dpa_hub_0(const sc_core::sc_module_name& nm) : pfm_dynamic_dpa_hub_0_sc(nm), s_aximm_clk("s_aximm_clk"), s_aximm_aresetn("s_aximm_aresetn"), trace_clk("trace_clk"), trace_aresetn("trace_aresetn"), axilite_clk("axilite_clk"), axilite_aresetn("axilite_aresetn"), trace_tready0("trace_tready0"), trace_tdata0("trace_tdata0"), trace_tlast0("trace_tlast0"), trace_tid0("trace_tid0"), trace_tdest0("trace_tdest0"), trace_tvalid0("trace_tvalid0"), trace_tready1("trace_tready1"), trace_tdata1("trace_tdata1"), trace_tlast1("trace_tlast1"), trace_tid1("trace_tid1"), trace_tdest1("trace_tdest1"), trace_tvalid1("trace_tvalid1"), trace_tdata2("trace_tdata2"), trace_tready2("trace_tready2"), trace_tlast2("trace_tlast2"), trace_tid2("trace_tid2"), trace_tdest2("trace_tdest2"), trace_tvalid2("trace_tvalid2"), trace_tdata3("trace_tdata3"), trace_tready3("trace_tready3"), trace_tlast3("trace_tlast3"), trace_tid3("trace_tid3"), trace_tdest3("trace_tdest3"), trace_tvalid3("trace_tvalid3"), trace_tdata4("trace_tdata4"), trace_tready4("trace_tready4"), trace_tlast4("trace_tlast4"), trace_tid4("trace_tid4"), trace_tdest4("trace_tdest4"), trace_tvalid4("trace_tvalid4"), trace_tdata5("trace_tdata5"), trace_tready5("trace_tready5"), trace_tlast5("trace_tlast5"), trace_tid5("trace_tid5"), trace_tdest5("trace_tdest5"), trace_tvalid5("trace_tvalid5"), trace_tdata6("trace_tdata6"), trace_tready6("trace_tready6"), trace_tlast6("trace_tlast6"), trace_tid6("trace_tid6"), trace_tdest6("trace_tdest6"), trace_tvalid6("trace_tvalid6"), trace_tdata7("trace_tdata7"), trace_tready7("trace_tready7"), trace_tlast7("trace_tlast7"), trace_tid7("trace_tid7"), trace_tdest7("trace_tdest7"), trace_tvalid7("trace_tvalid7"), trace_tdata8("trace_tdata8"), trace_tready8("trace_tready8"), trace_tlast8("trace_tlast8"), trace_tid8("trace_tid8"), trace_tdest8("trace_tdest8"), trace_tvalid8("trace_tvalid8"), trace_tdata9("trace_tdata9"), trace_tready9("trace_tready9"), trace_tlast9("trace_tlast9"), trace_tid9("trace_tid9"), trace_tdest9("trace_tdest9"), trace_tvalid9("trace_tvalid9"), trace_tdata10("trace_tdata10"), trace_tready10("trace_tready10"), trace_tlast10("trace_tlast10"), trace_tid10("trace_tid10"), trace_tdest10("trace_tdest10"), trace_tvalid10("trace_tvalid10"), trace_tdata11("trace_tdata11"), trace_tready11("trace_tready11"), trace_tlast11("trace_tlast11"), trace_tid11("trace_tid11"), trace_tdest11("trace_tdest11"), trace_tvalid11("trace_tvalid11"), trace_tdata12("trace_tdata12"), trace_tready12("trace_tready12"), trace_tlast12("trace_tlast12"), trace_tid12("trace_tid12"), trace_tdest12("trace_tdest12"), trace_tvalid12("trace_tvalid12"), trace_tdata13("trace_tdata13"), trace_tready13("trace_tready13"), trace_tlast13("trace_tlast13"), trace_tid13("trace_tid13"), trace_tdest13("trace_tdest13"), trace_tvalid13("trace_tvalid13"), trace_tdata14("trace_tdata14"), trace_tready14("trace_tready14"), trace_tlast14("trace_tlast14"), trace_tid14("trace_tid14"), trace_tdest14("trace_tdest14"), trace_tvalid14("trace_tvalid14"), trace_tdata15("trace_tdata15"), trace_tready15("trace_tready15"), trace_tlast15("trace_tlast15"), trace_tid15("trace_tid15"), trace_tdest15("trace_tdest15"), trace_tvalid15("trace_tvalid15"), trace_tdata16("trace_tdata16"), trace_tready16("trace_tready16"), trace_tlast16("trace_tlast16"), trace_tid16("trace_tid16"), trace_tdest16("trace_tdest16"), trace_tvalid16("trace_tvalid16"), trace_tdata17("trace_tdata17"), trace_tready17("trace_tready17"), trace_tlast17("trace_tlast17"), trace_tid17("trace_tid17"), trace_tdest17("trace_tdest17"), trace_tvalid17("trace_tvalid17"), trace_tdata18("trace_tdata18"), trace_tready18("trace_tready18"), trace_tlast18("trace_tlast18"), trace_tid18("trace_tid18"), trace_tdest18("trace_tdest18"), trace_tvalid18("trace_tvalid18"), trace_tdata19("trace_tdata19"), trace_tready19("trace_tready19"), trace_tlast19("trace_tlast19"), trace_tid19("trace_tid19"), trace_tdest19("trace_tdest19"), trace_tvalid19("trace_tvalid19"), trace_tdata20("trace_tdata20"), trace_tready20("trace_tready20"), trace_tlast20("trace_tlast20"), trace_tid20("trace_tid20"), trace_tdest20("trace_tdest20"), trace_tvalid20("trace_tvalid20"), trace_tdata21("trace_tdata21"), trace_tready21("trace_tready21"), trace_tlast21("trace_tlast21"), trace_tid21("trace_tid21"), trace_tdest21("trace_tdest21"), trace_tvalid21("trace_tvalid21"), trace_tdata22("trace_tdata22"), trace_tready22("trace_tready22"), trace_tlast22("trace_tlast22"), trace_tid22("trace_tid22"), trace_tdest22("trace_tdest22"), trace_tvalid22("trace_tvalid22"), trace_tdata23("trace_tdata23"), trace_tready23("trace_tready23"), trace_tlast23("trace_tlast23"), trace_tid23("trace_tid23"), trace_tdest23("trace_tdest23"), trace_tvalid23("trace_tvalid23"), trace_tdata24("trace_tdata24"), trace_tready24("trace_tready24"), trace_tlast24("trace_tlast24"), trace_tid24("trace_tid24"), trace_tdest24("trace_tdest24"), trace_tvalid24("trace_tvalid24"), trace_tdata25("trace_tdata25"), trace_tready25("trace_tready25"), trace_tlast25("trace_tlast25"), trace_tid25("trace_tid25"), trace_tdest25("trace_tdest25"), trace_tvalid25("trace_tvalid25"), trace_tdata26("trace_tdata26"), trace_tready26("trace_tready26"), trace_tlast26("trace_tlast26"), trace_tid26("trace_tid26"), trace_tdest26("trace_tdest26"), trace_tvalid26("trace_tvalid26"), trace_tdata27("trace_tdata27"), trace_tready27("trace_tready27"), trace_tlast27("trace_tlast27"), trace_tid27("trace_tid27"), trace_tdest27("trace_tdest27"), trace_tvalid27("trace_tvalid27"), trace_tdata28("trace_tdata28"), trace_tready28("trace_tready28"), trace_tlast28("trace_tlast28"), trace_tid28("trace_tid28"), trace_tdest28("trace_tdest28"), trace_tvalid28("trace_tvalid28"), trace_tdata29("trace_tdata29"), trace_tready29("trace_tready29"), trace_tlast29("trace_tlast29"), trace_tid29("trace_tid29"), trace_tdest29("trace_tdest29"), trace_tvalid29("trace_tvalid29"), trace_tdata30("trace_tdata30"), trace_tready30("trace_tready30"), trace_tlast30("trace_tlast30"), trace_tid30("trace_tid30"), trace_tdest30("trace_tdest30"), trace_tvalid30("trace_tvalid30"), trace_tdata31("trace_tdata31"), trace_tready31("trace_tready31"), trace_tlast31("trace_tlast31"), trace_tid31("trace_tid31"), trace_tdest31("trace_tdest31"), trace_tvalid31("trace_tvalid31"), trace_tdata32("trace_tdata32"), trace_tready32("trace_tready32"), trace_tlast32("trace_tlast32"), trace_tid32("trace_tid32"), trace_tdest32("trace_tdest32"), trace_tvalid32("trace_tvalid32"), trace_tdata33("trace_tdata33"), trace_tready33("trace_tready33"), trace_tlast33("trace_tlast33"), trace_tid33("trace_tid33"), trace_tdest33("trace_tdest33"), trace_tvalid33("trace_tvalid33"), trace_tdata34("trace_tdata34"), trace_tready34("trace_tready34"), trace_tlast34("trace_tlast34"), trace_tid34("trace_tid34"), trace_tdest34("trace_tdest34"), trace_tvalid34("trace_tvalid34"), trace_tdata35("trace_tdata35"), trace_tready35("trace_tready35"), trace_tlast35("trace_tlast35"), trace_tid35("trace_tid35"), trace_tdest35("trace_tdest35"), trace_tvalid35("trace_tvalid35"), trace_tdata36("trace_tdata36"), trace_tready36("trace_tready36"), trace_tlast36("trace_tlast36"), trace_tid36("trace_tid36"), trace_tdest36("trace_tdest36"), trace_tvalid36("trace_tvalid36"), trace_tdata37("trace_tdata37"), trace_tready37("trace_tready37"), trace_tlast37("trace_tlast37"), trace_tid37("trace_tid37"), trace_tdest37("trace_tdest37"), trace_tvalid37("trace_tvalid37"), trace_tdata38("trace_tdata38"), trace_tready38("trace_tready38"), trace_tlast38("trace_tlast38"), trace_tid38("trace_tid38"), trace_tdest38("trace_tdest38"), trace_tvalid38("trace_tvalid38"), trace_tdata39("trace_tdata39"), trace_tready39("trace_tready39"), trace_tlast39("trace_tlast39"), trace_tid39("trace_tid39"), trace_tdest39("trace_tdest39"), trace_tvalid39("trace_tvalid39"), trace_tdata40("trace_tdata40"), trace_tready40("trace_tready40"), trace_tlast40("trace_tlast40"), trace_tid40("trace_tid40"), trace_tdest40("trace_tdest40"), trace_tvalid40("trace_tvalid40"), s_axi_awaddr("s_axi_awaddr"), s_axi_awvalid("s_axi_awvalid"), s_axi_awready("s_axi_awready"), s_axi_wdata("s_axi_wdata"), s_axi_wstrb("s_axi_wstrb"), s_axi_wvalid("s_axi_wvalid"), s_axi_wready("s_axi_wready"), s_axi_bvalid("s_axi_bvalid"), s_axi_bready("s_axi_bready"), s_axi_bresp("s_axi_bresp"), s_axi_araddr("s_axi_araddr"), s_axi_arvalid("s_axi_arvalid"), s_axi_arready("s_axi_arready"), s_axi_rdata("s_axi_rdata"), s_axi_rresp("s_axi_rresp"), s_axi_rvalid("s_axi_rvalid"), s_axi_rready("s_axi_rready"), s_axihub_awaddr("s_axihub_awaddr"), s_axihub_awvalid("s_axihub_awvalid"), s_axihub_awready("s_axihub_awready"), s_axihub_wdata("s_axihub_wdata"), s_axihub_wstrb("s_axihub_wstrb"), s_axihub_wvalid("s_axihub_wvalid"), s_axihub_wready("s_axihub_wready"), s_axihub_bvalid("s_axihub_bvalid"), s_axihub_bready("s_axihub_bready"), s_axihub_bresp("s_axihub_bresp"), s_axihub_araddr("s_axihub_araddr"), s_axihub_arvalid("s_axihub_arvalid"), s_axihub_arready("s_axihub_arready"), s_axihub_rdata("s_axihub_rdata"), s_axihub_rresp("s_axihub_rresp"), s_axihub_rvalid("s_axihub_rvalid"), s_axihub_rready("s_axihub_rready"), s_aximm_awaddr("s_aximm_awaddr"), s_aximm_awvalid("s_aximm_awvalid"), s_aximm_awready("s_aximm_awready"), s_aximm_awlen("s_aximm_awlen"), s_aximm_awsize("s_aximm_awsize"), s_aximm_awburst("s_aximm_awburst"), s_aximm_awid("s_aximm_awid"), s_aximm_wdata("s_aximm_wdata"), s_aximm_wstrb("s_aximm_wstrb"), s_aximm_wvalid("s_aximm_wvalid"), s_aximm_wready("s_aximm_wready"), s_aximm_bvalid("s_aximm_bvalid"), s_aximm_bready("s_aximm_bready"), s_aximm_wlast("s_aximm_wlast"), s_aximm_rlast("s_aximm_rlast"), s_aximm_bresp("s_aximm_bresp"), s_aximm_bid("s_aximm_bid"), s_aximm_araddr("s_aximm_araddr"), s_aximm_arvalid("s_aximm_arvalid"), s_aximm_arready("s_aximm_arready"), s_aximm_arlen("s_aximm_arlen"), s_aximm_arsize("s_aximm_arsize"), s_aximm_arburst("s_aximm_arburst"), s_aximm_arid("s_aximm_arid"), s_aximm_rdata("s_aximm_rdata"), s_aximm_rid("s_aximm_rid"), s_aximm_rresp("s_aximm_rresp"), s_aximm_rvalid("s_aximm_rvalid"), s_aximm_rready("s_aximm_rready")
{

  // initialize pins
  mp_impl->s_aximm_clk(s_aximm_clk);
  mp_impl->s_aximm_aresetn(s_aximm_aresetn);
  mp_impl->trace_clk(trace_clk);
  mp_impl->trace_aresetn(trace_aresetn);
  mp_impl->axilite_clk(axilite_clk);
  mp_impl->axilite_aresetn(axilite_aresetn);

  // initialize transactors
  mp_S_AXIMM_transactor = NULL;
  mp_S_AXIFIFO_transactor = NULL;
  mp_S_AXIHUB_transactor = NULL;
  mp_TRACE_IN_0_transactor = NULL;
  mp_TRACE_IN_1_transactor = NULL;
  mp_TRACE_IN_2_transactor = NULL;
  mp_TRACE_IN_3_transactor = NULL;
  mp_TRACE_IN_4_transactor = NULL;
  mp_TRACE_IN_5_transactor = NULL;
  mp_TRACE_IN_6_transactor = NULL;
  mp_TRACE_IN_7_transactor = NULL;
  mp_TRACE_IN_8_transactor = NULL;
  mp_TRACE_IN_9_transactor = NULL;
  mp_TRACE_IN_10_transactor = NULL;
  mp_TRACE_IN_11_transactor = NULL;
  mp_TRACE_IN_12_transactor = NULL;
  mp_TRACE_IN_13_transactor = NULL;
  mp_TRACE_IN_14_transactor = NULL;
  mp_TRACE_IN_15_transactor = NULL;
  mp_TRACE_IN_16_transactor = NULL;
  mp_TRACE_IN_17_transactor = NULL;
  mp_TRACE_IN_18_transactor = NULL;
  mp_TRACE_IN_19_transactor = NULL;
  mp_TRACE_IN_20_transactor = NULL;
  mp_TRACE_IN_21_transactor = NULL;
  mp_TRACE_IN_22_transactor = NULL;
  mp_TRACE_IN_23_transactor = NULL;
  mp_TRACE_IN_24_transactor = NULL;
  mp_TRACE_IN_25_transactor = NULL;
  mp_TRACE_IN_26_transactor = NULL;
  mp_TRACE_IN_27_transactor = NULL;
  mp_TRACE_IN_28_transactor = NULL;
  mp_TRACE_IN_29_transactor = NULL;
  mp_TRACE_IN_30_transactor = NULL;
  mp_TRACE_IN_31_transactor = NULL;
  mp_TRACE_IN_32_transactor = NULL;
  mp_TRACE_IN_33_transactor = NULL;
  mp_TRACE_IN_34_transactor = NULL;
  mp_TRACE_IN_35_transactor = NULL;
  mp_TRACE_IN_36_transactor = NULL;
  mp_TRACE_IN_37_transactor = NULL;
  mp_TRACE_IN_38_transactor = NULL;
  mp_TRACE_IN_39_transactor = NULL;
  mp_TRACE_IN_40_transactor = NULL;

  // initialize socket stubs

}

void pfm_dynamic_dpa_hub_0::before_end_of_elaboration()
{
  // configure 'S_AXIMM' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "S_AXIMM_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'S_AXIMM' transactor parameters
    xsc::common_cpp::properties S_AXIMM_transactor_param_props;
    S_AXIMM_transactor_param_props.addLong("DATA_WIDTH", "64");
    S_AXIMM_transactor_param_props.addLong("FREQ_HZ", "300000000");
    S_AXIMM_transactor_param_props.addLong("ID_WIDTH", "1");
    S_AXIMM_transactor_param_props.addLong("ADDR_WIDTH", "32");
    S_AXIMM_transactor_param_props.addLong("AWUSER_WIDTH", "0");
    S_AXIMM_transactor_param_props.addLong("ARUSER_WIDTH", "0");
    S_AXIMM_transactor_param_props.addLong("WUSER_WIDTH", "0");
    S_AXIMM_transactor_param_props.addLong("RUSER_WIDTH", "0");
    S_AXIMM_transactor_param_props.addLong("BUSER_WIDTH", "0");
    S_AXIMM_transactor_param_props.addLong("HAS_BURST", "1");
    S_AXIMM_transactor_param_props.addLong("HAS_LOCK", "0");
    S_AXIMM_transactor_param_props.addLong("HAS_PROT", "0");
    S_AXIMM_transactor_param_props.addLong("HAS_CACHE", "0");
    S_AXIMM_transactor_param_props.addLong("HAS_QOS", "0");
    S_AXIMM_transactor_param_props.addLong("HAS_REGION", "0");
    S_AXIMM_transactor_param_props.addLong("HAS_WSTRB", "1");
    S_AXIMM_transactor_param_props.addLong("HAS_BRESP", "1");
    S_AXIMM_transactor_param_props.addLong("HAS_RRESP", "1");
    S_AXIMM_transactor_param_props.addLong("SUPPORTS_NARROW_BURST", "0");
    S_AXIMM_transactor_param_props.addLong("NUM_READ_OUTSTANDING", "2");
    S_AXIMM_transactor_param_props.addLong("NUM_WRITE_OUTSTANDING", "2");
    S_AXIMM_transactor_param_props.addLong("MAX_BURST_LENGTH", "128");
    S_AXIMM_transactor_param_props.addLong("NUM_READ_THREADS", "1");
    S_AXIMM_transactor_param_props.addLong("NUM_WRITE_THREADS", "1");
    S_AXIMM_transactor_param_props.addLong("RUSER_BITS_PER_BYTE", "0");
    S_AXIMM_transactor_param_props.addLong("WUSER_BITS_PER_BYTE", "0");
    S_AXIMM_transactor_param_props.addLong("HAS_SIZE", "1");
    S_AXIMM_transactor_param_props.addLong("HAS_RESET", "1");
    S_AXIMM_transactor_param_props.addFloat("PHASE", "0.0");
    S_AXIMM_transactor_param_props.addString("PROTOCOL", "AXI4");
    S_AXIMM_transactor_param_props.addString("READ_WRITE_MODE", "READ_WRITE");
    S_AXIMM_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_dma_pcie_aclk");
    S_AXIMM_transactor_param_props.addString("SLR_ASSIGNMENT", "");

    mp_S_AXIMM_transactor = new xtlm::xaximm_pin2xtlm_t<64,32,1,1,1,1,1,1>("S_AXIMM_transactor", S_AXIMM_transactor_param_props);

    // S_AXIMM' transactor ports

    mp_S_AXIMM_transactor->ARADDR(s_aximm_araddr);
    mp_S_AXIMM_transactor->ARBURST(s_aximm_arburst);
    mp_S_AXIMM_transactor->ARID(s_aximm_arid);
    mp_S_AXIMM_transactor->ARLEN(s_aximm_arlen);
    mp_S_AXIMM_transactor->ARREADY(s_aximm_arready);
    mp_S_AXIMM_transactor->ARSIZE(s_aximm_arsize);
    mp_S_AXIMM_transactor->ARVALID(s_aximm_arvalid);
    mp_S_AXIMM_transactor->AWADDR(s_aximm_awaddr);
    mp_S_AXIMM_transactor->AWBURST(s_aximm_awburst);
    mp_S_AXIMM_transactor->AWID(s_aximm_awid);
    mp_S_AXIMM_transactor->AWLEN(s_aximm_awlen);
    mp_S_AXIMM_transactor->AWREADY(s_aximm_awready);
    mp_S_AXIMM_transactor->AWSIZE(s_aximm_awsize);
    mp_S_AXIMM_transactor->AWVALID(s_aximm_awvalid);
    mp_S_AXIMM_transactor->BID(s_aximm_bid);
    mp_S_AXIMM_transactor->BREADY(s_aximm_bready);
    mp_S_AXIMM_transactor->BRESP(s_aximm_bresp);
    mp_S_AXIMM_transactor->BVALID(s_aximm_bvalid);
    mp_S_AXIMM_transactor->RDATA(s_aximm_rdata);
    mp_S_AXIMM_transactor->RID(s_aximm_rid);
    mp_S_AXIMM_transactor->RLAST(s_aximm_rlast);
    mp_S_AXIMM_transactor->RREADY(s_aximm_rready);
    mp_S_AXIMM_transactor->RRESP(s_aximm_rresp);
    mp_S_AXIMM_transactor->RVALID(s_aximm_rvalid);
    mp_S_AXIMM_transactor->WDATA(s_aximm_wdata);
    mp_S_AXIMM_transactor->WLAST(s_aximm_wlast);
    mp_S_AXIMM_transactor->WREADY(s_aximm_wready);
    mp_S_AXIMM_transactor->WSTRB(s_aximm_wstrb);
    mp_S_AXIMM_transactor->WVALID(s_aximm_wvalid);
    mp_S_AXIMM_transactor->CLK(s_aximm_clk);
    mp_S_AXIMM_transactor->RST(s_aximm_aresetn);

    // S_AXIMM' transactor sockets

    mp_impl->s_aximm_rd_socket->bind(*(mp_S_AXIMM_transactor->rd_socket));
    mp_impl->s_aximm_wr_socket->bind(*(mp_S_AXIMM_transactor->wr_socket));
  }
  else
  {
  }

  // configure 'S_AXIFIFO' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "S_AXIFIFO_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'S_AXIFIFO' transactor parameters
    xsc::common_cpp::properties S_AXIFIFO_transactor_param_props;
    S_AXIFIFO_transactor_param_props.addLong("DATA_WIDTH", "32");
    S_AXIFIFO_transactor_param_props.addLong("FREQ_HZ", "300000000");
    S_AXIFIFO_transactor_param_props.addLong("ID_WIDTH", "0");
    S_AXIFIFO_transactor_param_props.addLong("ADDR_WIDTH", "8");
    S_AXIFIFO_transactor_param_props.addLong("AWUSER_WIDTH", "0");
    S_AXIFIFO_transactor_param_props.addLong("ARUSER_WIDTH", "0");
    S_AXIFIFO_transactor_param_props.addLong("WUSER_WIDTH", "0");
    S_AXIFIFO_transactor_param_props.addLong("RUSER_WIDTH", "0");
    S_AXIFIFO_transactor_param_props.addLong("BUSER_WIDTH", "0");
    S_AXIFIFO_transactor_param_props.addLong("HAS_BURST", "0");
    S_AXIFIFO_transactor_param_props.addLong("HAS_LOCK", "0");
    S_AXIFIFO_transactor_param_props.addLong("HAS_PROT", "0");
    S_AXIFIFO_transactor_param_props.addLong("HAS_CACHE", "0");
    S_AXIFIFO_transactor_param_props.addLong("HAS_QOS", "0");
    S_AXIFIFO_transactor_param_props.addLong("HAS_REGION", "0");
    S_AXIFIFO_transactor_param_props.addLong("HAS_WSTRB", "1");
    S_AXIFIFO_transactor_param_props.addLong("HAS_BRESP", "1");
    S_AXIFIFO_transactor_param_props.addLong("HAS_RRESP", "1");
    S_AXIFIFO_transactor_param_props.addLong("SUPPORTS_NARROW_BURST", "0");
    S_AXIFIFO_transactor_param_props.addLong("NUM_READ_OUTSTANDING", "2");
    S_AXIFIFO_transactor_param_props.addLong("NUM_WRITE_OUTSTANDING", "2");
    S_AXIFIFO_transactor_param_props.addLong("MAX_BURST_LENGTH", "1");
    S_AXIFIFO_transactor_param_props.addLong("NUM_READ_THREADS", "1");
    S_AXIFIFO_transactor_param_props.addLong("NUM_WRITE_THREADS", "1");
    S_AXIFIFO_transactor_param_props.addLong("RUSER_BITS_PER_BYTE", "0");
    S_AXIFIFO_transactor_param_props.addLong("WUSER_BITS_PER_BYTE", "0");
    S_AXIFIFO_transactor_param_props.addLong("HAS_SIZE", "0");
    S_AXIFIFO_transactor_param_props.addLong("HAS_RESET", "1");
    S_AXIFIFO_transactor_param_props.addFloat("PHASE", "0.0");
    S_AXIFIFO_transactor_param_props.addString("PROTOCOL", "AXI4LITE");
    S_AXIFIFO_transactor_param_props.addString("READ_WRITE_MODE", "READ_WRITE");
    S_AXIFIFO_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    S_AXIFIFO_transactor_param_props.addString("SLR_ASSIGNMENT", "");

    mp_S_AXIFIFO_transactor = new xtlm::xaximm_pin2xtlm_t<32,8,1,1,1,1,1,1>("S_AXIFIFO_transactor", S_AXIFIFO_transactor_param_props);

    // S_AXIFIFO' transactor ports

    mp_S_AXIFIFO_transactor->ARADDR(s_axi_araddr);
    mp_S_AXIFIFO_transactor->ARREADY(s_axi_arready);
    mp_S_AXIFIFO_transactor->ARVALID(s_axi_arvalid);
    mp_S_AXIFIFO_transactor->AWADDR(s_axi_awaddr);
    mp_S_AXIFIFO_transactor->AWREADY(s_axi_awready);
    mp_S_AXIFIFO_transactor->AWVALID(s_axi_awvalid);
    mp_S_AXIFIFO_transactor->BREADY(s_axi_bready);
    mp_S_AXIFIFO_transactor->BRESP(s_axi_bresp);
    mp_S_AXIFIFO_transactor->BVALID(s_axi_bvalid);
    mp_S_AXIFIFO_transactor->RDATA(s_axi_rdata);
    mp_S_AXIFIFO_transactor->RREADY(s_axi_rready);
    mp_S_AXIFIFO_transactor->RRESP(s_axi_rresp);
    mp_S_AXIFIFO_transactor->RVALID(s_axi_rvalid);
    mp_S_AXIFIFO_transactor->WDATA(s_axi_wdata);
    mp_S_AXIFIFO_transactor->WREADY(s_axi_wready);
    mp_S_AXIFIFO_transactor->WSTRB(s_axi_wstrb);
    mp_S_AXIFIFO_transactor->WVALID(s_axi_wvalid);
    mp_S_AXIFIFO_transactor->CLK(axilite_clk);
    mp_S_AXIFIFO_transactor->RST(axilite_aresetn);

    // S_AXIFIFO' transactor sockets

    mp_impl->axilite_rd_socket->bind(*(mp_S_AXIFIFO_transactor->rd_socket));
    mp_impl->axilite_wr_socket->bind(*(mp_S_AXIFIFO_transactor->wr_socket));
  }
  else
  {
  }

  // configure 'S_AXIHUB' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "S_AXIHUB_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'S_AXIHUB' transactor parameters
    xsc::common_cpp::properties S_AXIHUB_transactor_param_props;
    S_AXIHUB_transactor_param_props.addLong("DATA_WIDTH", "32");
    S_AXIHUB_transactor_param_props.addLong("FREQ_HZ", "300000000");
    S_AXIHUB_transactor_param_props.addLong("ID_WIDTH", "0");
    S_AXIHUB_transactor_param_props.addLong("ADDR_WIDTH", "8");
    S_AXIHUB_transactor_param_props.addLong("AWUSER_WIDTH", "0");
    S_AXIHUB_transactor_param_props.addLong("ARUSER_WIDTH", "0");
    S_AXIHUB_transactor_param_props.addLong("WUSER_WIDTH", "0");
    S_AXIHUB_transactor_param_props.addLong("RUSER_WIDTH", "0");
    S_AXIHUB_transactor_param_props.addLong("BUSER_WIDTH", "0");
    S_AXIHUB_transactor_param_props.addLong("HAS_BURST", "0");
    S_AXIHUB_transactor_param_props.addLong("HAS_LOCK", "0");
    S_AXIHUB_transactor_param_props.addLong("HAS_PROT", "0");
    S_AXIHUB_transactor_param_props.addLong("HAS_CACHE", "0");
    S_AXIHUB_transactor_param_props.addLong("HAS_QOS", "0");
    S_AXIHUB_transactor_param_props.addLong("HAS_REGION", "0");
    S_AXIHUB_transactor_param_props.addLong("HAS_WSTRB", "1");
    S_AXIHUB_transactor_param_props.addLong("HAS_BRESP", "1");
    S_AXIHUB_transactor_param_props.addLong("HAS_RRESP", "1");
    S_AXIHUB_transactor_param_props.addLong("SUPPORTS_NARROW_BURST", "0");
    S_AXIHUB_transactor_param_props.addLong("NUM_READ_OUTSTANDING", "2");
    S_AXIHUB_transactor_param_props.addLong("NUM_WRITE_OUTSTANDING", "2");
    S_AXIHUB_transactor_param_props.addLong("MAX_BURST_LENGTH", "1");
    S_AXIHUB_transactor_param_props.addLong("NUM_READ_THREADS", "1");
    S_AXIHUB_transactor_param_props.addLong("NUM_WRITE_THREADS", "1");
    S_AXIHUB_transactor_param_props.addLong("RUSER_BITS_PER_BYTE", "0");
    S_AXIHUB_transactor_param_props.addLong("WUSER_BITS_PER_BYTE", "0");
    S_AXIHUB_transactor_param_props.addLong("HAS_SIZE", "0");
    S_AXIHUB_transactor_param_props.addLong("HAS_RESET", "1");
    S_AXIHUB_transactor_param_props.addFloat("PHASE", "0.0");
    S_AXIHUB_transactor_param_props.addString("PROTOCOL", "AXI4LITE");
    S_AXIHUB_transactor_param_props.addString("READ_WRITE_MODE", "READ_WRITE");
    S_AXIHUB_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    S_AXIHUB_transactor_param_props.addString("SLR_ASSIGNMENT", "");

    mp_S_AXIHUB_transactor = new xtlm::xaximm_pin2xtlm_t<32,8,1,1,1,1,1,1>("S_AXIHUB_transactor", S_AXIHUB_transactor_param_props);

    // S_AXIHUB' transactor ports

    mp_S_AXIHUB_transactor->ARADDR(s_axihub_araddr);
    mp_S_AXIHUB_transactor->ARREADY(s_axihub_arready);
    mp_S_AXIHUB_transactor->ARVALID(s_axihub_arvalid);
    mp_S_AXIHUB_transactor->AWADDR(s_axihub_awaddr);
    mp_S_AXIHUB_transactor->AWREADY(s_axihub_awready);
    mp_S_AXIHUB_transactor->AWVALID(s_axihub_awvalid);
    mp_S_AXIHUB_transactor->BREADY(s_axihub_bready);
    mp_S_AXIHUB_transactor->BRESP(s_axihub_bresp);
    mp_S_AXIHUB_transactor->BVALID(s_axihub_bvalid);
    mp_S_AXIHUB_transactor->RDATA(s_axihub_rdata);
    mp_S_AXIHUB_transactor->RREADY(s_axihub_rready);
    mp_S_AXIHUB_transactor->RRESP(s_axihub_rresp);
    mp_S_AXIHUB_transactor->RVALID(s_axihub_rvalid);
    mp_S_AXIHUB_transactor->WDATA(s_axihub_wdata);
    mp_S_AXIHUB_transactor->WREADY(s_axihub_wready);
    mp_S_AXIHUB_transactor->WSTRB(s_axihub_wstrb);
    mp_S_AXIHUB_transactor->WVALID(s_axihub_wvalid);
    mp_S_AXIHUB_transactor->CLK(axilite_clk);
    mp_S_AXIHUB_transactor->RST(axilite_aresetn);

    // S_AXIHUB' transactor sockets

    mp_impl->s_axihub_rd_socket->bind(*(mp_S_AXIHUB_transactor->rd_socket));
    mp_impl->s_axihub_wr_socket->bind(*(mp_S_AXIHUB_transactor->wr_socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_0' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_0_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_0' transactor parameters
    xsc::common_cpp::properties TRACE_IN_0_transactor_param_props;
    TRACE_IN_0_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_0_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_0_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_0_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_0_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_0_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_0_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_0_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_0_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_0_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_0_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_0_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_0_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_0_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_0_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_0_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_0_transactor", TRACE_IN_0_transactor_param_props);

    // TRACE_IN_0' transactor ports

    mp_TRACE_IN_0_transactor->TDATA(trace_tdata0);
    mp_TRACE_IN_0_transactor->TDEST(trace_tdest0);
    mp_TRACE_IN_0_transactor->TID(trace_tid0);
    mp_TRACE_IN_0_transactor->TLAST(trace_tlast0);
    mp_TRACE_IN_0_transactor->TREADY(trace_tready0);
    mp_TRACE_IN_0_transactor->TVALID(trace_tvalid0);
    mp_TRACE_IN_0_transactor->CLK(trace_clk);
    mp_TRACE_IN_0_transactor->RST(trace_aresetn);

    // TRACE_IN_0' transactor sockets

    mp_impl->trace_socket0->bind(*(mp_TRACE_IN_0_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_1' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_1_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_1' transactor parameters
    xsc::common_cpp::properties TRACE_IN_1_transactor_param_props;
    TRACE_IN_1_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_1_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_1_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_1_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_1_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_1_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_1_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_1_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_1_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_1_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_1_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_1_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_1_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_1_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_1_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_1_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_1_transactor", TRACE_IN_1_transactor_param_props);

    // TRACE_IN_1' transactor ports

    mp_TRACE_IN_1_transactor->TDATA(trace_tdata1);
    mp_TRACE_IN_1_transactor->TDEST(trace_tdest1);
    mp_TRACE_IN_1_transactor->TID(trace_tid1);
    mp_TRACE_IN_1_transactor->TLAST(trace_tlast1);
    mp_TRACE_IN_1_transactor->TREADY(trace_tready1);
    mp_TRACE_IN_1_transactor->TVALID(trace_tvalid1);
    mp_TRACE_IN_1_transactor->CLK(trace_clk);
    mp_TRACE_IN_1_transactor->RST(trace_aresetn);

    // TRACE_IN_1' transactor sockets

    mp_impl->trace_socket1->bind(*(mp_TRACE_IN_1_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_2' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_2_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_2' transactor parameters
    xsc::common_cpp::properties TRACE_IN_2_transactor_param_props;
    TRACE_IN_2_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_2_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_2_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_2_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_2_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_2_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_2_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_2_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_2_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_2_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_2_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_2_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_2_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_2_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_2_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_2_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_2_transactor", TRACE_IN_2_transactor_param_props);

    // TRACE_IN_2' transactor ports

    mp_TRACE_IN_2_transactor->TDATA(trace_tdata2);
    mp_TRACE_IN_2_transactor->TDEST(trace_tdest2);
    mp_TRACE_IN_2_transactor->TID(trace_tid2);
    mp_TRACE_IN_2_transactor->TLAST(trace_tlast2);
    mp_TRACE_IN_2_transactor->TREADY(trace_tready2);
    mp_TRACE_IN_2_transactor->TVALID(trace_tvalid2);
    mp_TRACE_IN_2_transactor->CLK(trace_clk);
    mp_TRACE_IN_2_transactor->RST(trace_aresetn);

    // TRACE_IN_2' transactor sockets

    mp_impl->trace_socket2->bind(*(mp_TRACE_IN_2_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_3' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_3_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_3' transactor parameters
    xsc::common_cpp::properties TRACE_IN_3_transactor_param_props;
    TRACE_IN_3_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_3_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_3_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_3_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_3_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_3_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_3_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_3_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_3_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_3_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_3_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_3_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_3_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_3_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_3_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_3_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_3_transactor", TRACE_IN_3_transactor_param_props);

    // TRACE_IN_3' transactor ports

    mp_TRACE_IN_3_transactor->TDATA(trace_tdata3);
    mp_TRACE_IN_3_transactor->TDEST(trace_tdest3);
    mp_TRACE_IN_3_transactor->TID(trace_tid3);
    mp_TRACE_IN_3_transactor->TLAST(trace_tlast3);
    mp_TRACE_IN_3_transactor->TREADY(trace_tready3);
    mp_TRACE_IN_3_transactor->TVALID(trace_tvalid3);
    mp_TRACE_IN_3_transactor->CLK(trace_clk);
    mp_TRACE_IN_3_transactor->RST(trace_aresetn);

    // TRACE_IN_3' transactor sockets

    mp_impl->trace_socket3->bind(*(mp_TRACE_IN_3_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_4' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_4_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_4' transactor parameters
    xsc::common_cpp::properties TRACE_IN_4_transactor_param_props;
    TRACE_IN_4_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_4_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_4_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_4_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_4_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_4_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_4_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_4_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_4_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_4_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_4_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_4_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_4_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_4_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_4_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_4_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_4_transactor", TRACE_IN_4_transactor_param_props);

    // TRACE_IN_4' transactor ports

    mp_TRACE_IN_4_transactor->TDATA(trace_tdata4);
    mp_TRACE_IN_4_transactor->TDEST(trace_tdest4);
    mp_TRACE_IN_4_transactor->TID(trace_tid4);
    mp_TRACE_IN_4_transactor->TLAST(trace_tlast4);
    mp_TRACE_IN_4_transactor->TREADY(trace_tready4);
    mp_TRACE_IN_4_transactor->TVALID(trace_tvalid4);
    mp_TRACE_IN_4_transactor->CLK(trace_clk);
    mp_TRACE_IN_4_transactor->RST(trace_aresetn);

    // TRACE_IN_4' transactor sockets

    mp_impl->trace_socket4->bind(*(mp_TRACE_IN_4_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_5' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_5_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_5' transactor parameters
    xsc::common_cpp::properties TRACE_IN_5_transactor_param_props;
    TRACE_IN_5_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_5_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_5_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_5_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_5_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_5_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_5_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_5_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_5_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_5_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_5_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_5_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_5_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_5_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_5_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_5_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_5_transactor", TRACE_IN_5_transactor_param_props);

    // TRACE_IN_5' transactor ports

    mp_TRACE_IN_5_transactor->TDATA(trace_tdata5);
    mp_TRACE_IN_5_transactor->TDEST(trace_tdest5);
    mp_TRACE_IN_5_transactor->TID(trace_tid5);
    mp_TRACE_IN_5_transactor->TLAST(trace_tlast5);
    mp_TRACE_IN_5_transactor->TREADY(trace_tready5);
    mp_TRACE_IN_5_transactor->TVALID(trace_tvalid5);
    mp_TRACE_IN_5_transactor->CLK(trace_clk);
    mp_TRACE_IN_5_transactor->RST(trace_aresetn);

    // TRACE_IN_5' transactor sockets

    mp_impl->trace_socket5->bind(*(mp_TRACE_IN_5_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_6' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_6_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_6' transactor parameters
    xsc::common_cpp::properties TRACE_IN_6_transactor_param_props;
    TRACE_IN_6_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_6_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_6_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_6_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_6_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_6_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_6_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_6_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_6_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_6_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_6_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_6_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_6_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_6_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_6_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_6_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_6_transactor", TRACE_IN_6_transactor_param_props);

    // TRACE_IN_6' transactor ports

    mp_TRACE_IN_6_transactor->TDATA(trace_tdata6);
    mp_TRACE_IN_6_transactor->TDEST(trace_tdest6);
    mp_TRACE_IN_6_transactor->TID(trace_tid6);
    mp_TRACE_IN_6_transactor->TLAST(trace_tlast6);
    mp_TRACE_IN_6_transactor->TREADY(trace_tready6);
    mp_TRACE_IN_6_transactor->TVALID(trace_tvalid6);
    mp_TRACE_IN_6_transactor->CLK(trace_clk);
    mp_TRACE_IN_6_transactor->RST(trace_aresetn);

    // TRACE_IN_6' transactor sockets

    mp_impl->trace_socket6->bind(*(mp_TRACE_IN_6_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_7' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_7_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_7' transactor parameters
    xsc::common_cpp::properties TRACE_IN_7_transactor_param_props;
    TRACE_IN_7_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_7_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_7_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_7_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_7_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_7_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_7_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_7_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_7_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_7_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_7_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_7_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_7_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_7_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_7_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_7_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_7_transactor", TRACE_IN_7_transactor_param_props);

    // TRACE_IN_7' transactor ports

    mp_TRACE_IN_7_transactor->TDATA(trace_tdata7);
    mp_TRACE_IN_7_transactor->TDEST(trace_tdest7);
    mp_TRACE_IN_7_transactor->TID(trace_tid7);
    mp_TRACE_IN_7_transactor->TLAST(trace_tlast7);
    mp_TRACE_IN_7_transactor->TREADY(trace_tready7);
    mp_TRACE_IN_7_transactor->TVALID(trace_tvalid7);
    mp_TRACE_IN_7_transactor->CLK(trace_clk);
    mp_TRACE_IN_7_transactor->RST(trace_aresetn);

    // TRACE_IN_7' transactor sockets

    mp_impl->trace_socket7->bind(*(mp_TRACE_IN_7_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_8' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_8_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_8' transactor parameters
    xsc::common_cpp::properties TRACE_IN_8_transactor_param_props;
    TRACE_IN_8_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_8_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_8_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_8_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_8_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_8_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_8_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_8_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_8_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_8_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_8_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_8_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_8_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_8_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_8_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_8_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_8_transactor", TRACE_IN_8_transactor_param_props);

    // TRACE_IN_8' transactor ports

    mp_TRACE_IN_8_transactor->TDATA(trace_tdata8);
    mp_TRACE_IN_8_transactor->TDEST(trace_tdest8);
    mp_TRACE_IN_8_transactor->TID(trace_tid8);
    mp_TRACE_IN_8_transactor->TLAST(trace_tlast8);
    mp_TRACE_IN_8_transactor->TREADY(trace_tready8);
    mp_TRACE_IN_8_transactor->TVALID(trace_tvalid8);
    mp_TRACE_IN_8_transactor->CLK(trace_clk);
    mp_TRACE_IN_8_transactor->RST(trace_aresetn);

    // TRACE_IN_8' transactor sockets

    mp_impl->trace_socket8->bind(*(mp_TRACE_IN_8_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_9' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_9_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_9' transactor parameters
    xsc::common_cpp::properties TRACE_IN_9_transactor_param_props;
    TRACE_IN_9_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_9_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_9_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_9_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_9_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_9_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_9_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_9_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_9_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_9_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_9_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_9_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_9_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_9_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_9_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_9_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_9_transactor", TRACE_IN_9_transactor_param_props);

    // TRACE_IN_9' transactor ports

    mp_TRACE_IN_9_transactor->TDATA(trace_tdata9);
    mp_TRACE_IN_9_transactor->TDEST(trace_tdest9);
    mp_TRACE_IN_9_transactor->TID(trace_tid9);
    mp_TRACE_IN_9_transactor->TLAST(trace_tlast9);
    mp_TRACE_IN_9_transactor->TREADY(trace_tready9);
    mp_TRACE_IN_9_transactor->TVALID(trace_tvalid9);
    mp_TRACE_IN_9_transactor->CLK(trace_clk);
    mp_TRACE_IN_9_transactor->RST(trace_aresetn);

    // TRACE_IN_9' transactor sockets

    mp_impl->trace_socket9->bind(*(mp_TRACE_IN_9_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_10' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_10_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_10' transactor parameters
    xsc::common_cpp::properties TRACE_IN_10_transactor_param_props;
    TRACE_IN_10_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_10_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_10_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_10_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_10_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_10_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_10_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_10_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_10_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_10_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_10_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_10_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_10_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_10_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_10_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_10_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_10_transactor", TRACE_IN_10_transactor_param_props);

    // TRACE_IN_10' transactor ports

    mp_TRACE_IN_10_transactor->TDATA(trace_tdata10);
    mp_TRACE_IN_10_transactor->TDEST(trace_tdest10);
    mp_TRACE_IN_10_transactor->TID(trace_tid10);
    mp_TRACE_IN_10_transactor->TLAST(trace_tlast10);
    mp_TRACE_IN_10_transactor->TREADY(trace_tready10);
    mp_TRACE_IN_10_transactor->TVALID(trace_tvalid10);
    mp_TRACE_IN_10_transactor->CLK(trace_clk);
    mp_TRACE_IN_10_transactor->RST(trace_aresetn);

    // TRACE_IN_10' transactor sockets

    mp_impl->trace_socket10->bind(*(mp_TRACE_IN_10_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_11' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_11_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_11' transactor parameters
    xsc::common_cpp::properties TRACE_IN_11_transactor_param_props;
    TRACE_IN_11_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_11_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_11_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_11_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_11_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_11_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_11_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_11_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_11_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_11_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_11_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_11_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_11_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_11_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_11_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_11_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_11_transactor", TRACE_IN_11_transactor_param_props);

    // TRACE_IN_11' transactor ports

    mp_TRACE_IN_11_transactor->TDATA(trace_tdata11);
    mp_TRACE_IN_11_transactor->TDEST(trace_tdest11);
    mp_TRACE_IN_11_transactor->TID(trace_tid11);
    mp_TRACE_IN_11_transactor->TLAST(trace_tlast11);
    mp_TRACE_IN_11_transactor->TREADY(trace_tready11);
    mp_TRACE_IN_11_transactor->TVALID(trace_tvalid11);
    mp_TRACE_IN_11_transactor->CLK(trace_clk);
    mp_TRACE_IN_11_transactor->RST(trace_aresetn);

    // TRACE_IN_11' transactor sockets

    mp_impl->trace_socket11->bind(*(mp_TRACE_IN_11_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_12' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_12_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_12' transactor parameters
    xsc::common_cpp::properties TRACE_IN_12_transactor_param_props;
    TRACE_IN_12_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_12_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_12_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_12_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_12_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_12_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_12_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_12_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_12_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_12_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_12_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_12_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_12_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_12_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_12_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_12_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_12_transactor", TRACE_IN_12_transactor_param_props);

    // TRACE_IN_12' transactor ports

    mp_TRACE_IN_12_transactor->TDATA(trace_tdata12);
    mp_TRACE_IN_12_transactor->TDEST(trace_tdest12);
    mp_TRACE_IN_12_transactor->TID(trace_tid12);
    mp_TRACE_IN_12_transactor->TLAST(trace_tlast12);
    mp_TRACE_IN_12_transactor->TREADY(trace_tready12);
    mp_TRACE_IN_12_transactor->TVALID(trace_tvalid12);
    mp_TRACE_IN_12_transactor->CLK(trace_clk);
    mp_TRACE_IN_12_transactor->RST(trace_aresetn);

    // TRACE_IN_12' transactor sockets

    mp_impl->trace_socket12->bind(*(mp_TRACE_IN_12_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_13' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_13_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_13' transactor parameters
    xsc::common_cpp::properties TRACE_IN_13_transactor_param_props;
    TRACE_IN_13_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_13_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_13_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_13_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_13_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_13_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_13_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_13_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_13_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_13_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_13_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_13_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_13_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_13_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_13_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_13_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_13_transactor", TRACE_IN_13_transactor_param_props);

    // TRACE_IN_13' transactor ports

    mp_TRACE_IN_13_transactor->TDATA(trace_tdata13);
    mp_TRACE_IN_13_transactor->TDEST(trace_tdest13);
    mp_TRACE_IN_13_transactor->TID(trace_tid13);
    mp_TRACE_IN_13_transactor->TLAST(trace_tlast13);
    mp_TRACE_IN_13_transactor->TREADY(trace_tready13);
    mp_TRACE_IN_13_transactor->TVALID(trace_tvalid13);
    mp_TRACE_IN_13_transactor->CLK(trace_clk);
    mp_TRACE_IN_13_transactor->RST(trace_aresetn);

    // TRACE_IN_13' transactor sockets

    mp_impl->trace_socket13->bind(*(mp_TRACE_IN_13_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_14' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_14_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_14' transactor parameters
    xsc::common_cpp::properties TRACE_IN_14_transactor_param_props;
    TRACE_IN_14_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_14_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_14_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_14_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_14_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_14_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_14_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_14_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_14_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_14_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_14_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_14_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_14_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_14_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_14_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_14_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_14_transactor", TRACE_IN_14_transactor_param_props);

    // TRACE_IN_14' transactor ports

    mp_TRACE_IN_14_transactor->TDATA(trace_tdata14);
    mp_TRACE_IN_14_transactor->TDEST(trace_tdest14);
    mp_TRACE_IN_14_transactor->TID(trace_tid14);
    mp_TRACE_IN_14_transactor->TLAST(trace_tlast14);
    mp_TRACE_IN_14_transactor->TREADY(trace_tready14);
    mp_TRACE_IN_14_transactor->TVALID(trace_tvalid14);
    mp_TRACE_IN_14_transactor->CLK(trace_clk);
    mp_TRACE_IN_14_transactor->RST(trace_aresetn);

    // TRACE_IN_14' transactor sockets

    mp_impl->trace_socket14->bind(*(mp_TRACE_IN_14_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_15' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_15_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_15' transactor parameters
    xsc::common_cpp::properties TRACE_IN_15_transactor_param_props;
    TRACE_IN_15_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_15_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_15_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_15_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_15_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_15_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_15_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_15_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_15_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_15_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_15_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_15_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_15_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_15_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_15_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_15_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_15_transactor", TRACE_IN_15_transactor_param_props);

    // TRACE_IN_15' transactor ports

    mp_TRACE_IN_15_transactor->TDATA(trace_tdata15);
    mp_TRACE_IN_15_transactor->TDEST(trace_tdest15);
    mp_TRACE_IN_15_transactor->TID(trace_tid15);
    mp_TRACE_IN_15_transactor->TLAST(trace_tlast15);
    mp_TRACE_IN_15_transactor->TREADY(trace_tready15);
    mp_TRACE_IN_15_transactor->TVALID(trace_tvalid15);
    mp_TRACE_IN_15_transactor->CLK(trace_clk);
    mp_TRACE_IN_15_transactor->RST(trace_aresetn);

    // TRACE_IN_15' transactor sockets

    mp_impl->trace_socket15->bind(*(mp_TRACE_IN_15_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_16' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_16_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_16' transactor parameters
    xsc::common_cpp::properties TRACE_IN_16_transactor_param_props;
    TRACE_IN_16_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_16_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_16_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_16_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_16_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_16_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_16_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_16_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_16_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_16_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_16_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_16_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_16_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_16_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_16_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_16_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_16_transactor", TRACE_IN_16_transactor_param_props);

    // TRACE_IN_16' transactor ports

    mp_TRACE_IN_16_transactor->TDATA(trace_tdata16);
    mp_TRACE_IN_16_transactor->TDEST(trace_tdest16);
    mp_TRACE_IN_16_transactor->TID(trace_tid16);
    mp_TRACE_IN_16_transactor->TLAST(trace_tlast16);
    mp_TRACE_IN_16_transactor->TREADY(trace_tready16);
    mp_TRACE_IN_16_transactor->TVALID(trace_tvalid16);
    mp_TRACE_IN_16_transactor->CLK(trace_clk);
    mp_TRACE_IN_16_transactor->RST(trace_aresetn);

    // TRACE_IN_16' transactor sockets

    mp_impl->trace_socket16->bind(*(mp_TRACE_IN_16_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_17' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_17_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_17' transactor parameters
    xsc::common_cpp::properties TRACE_IN_17_transactor_param_props;
    TRACE_IN_17_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_17_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_17_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_17_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_17_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_17_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_17_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_17_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_17_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_17_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_17_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_17_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_17_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_17_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_17_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_17_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_17_transactor", TRACE_IN_17_transactor_param_props);

    // TRACE_IN_17' transactor ports

    mp_TRACE_IN_17_transactor->TDATA(trace_tdata17);
    mp_TRACE_IN_17_transactor->TDEST(trace_tdest17);
    mp_TRACE_IN_17_transactor->TID(trace_tid17);
    mp_TRACE_IN_17_transactor->TLAST(trace_tlast17);
    mp_TRACE_IN_17_transactor->TREADY(trace_tready17);
    mp_TRACE_IN_17_transactor->TVALID(trace_tvalid17);
    mp_TRACE_IN_17_transactor->CLK(trace_clk);
    mp_TRACE_IN_17_transactor->RST(trace_aresetn);

    // TRACE_IN_17' transactor sockets

    mp_impl->trace_socket17->bind(*(mp_TRACE_IN_17_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_18' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_18_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_18' transactor parameters
    xsc::common_cpp::properties TRACE_IN_18_transactor_param_props;
    TRACE_IN_18_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_18_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_18_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_18_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_18_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_18_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_18_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_18_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_18_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_18_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_18_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_18_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_18_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_18_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_18_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_18_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_18_transactor", TRACE_IN_18_transactor_param_props);

    // TRACE_IN_18' transactor ports

    mp_TRACE_IN_18_transactor->TDATA(trace_tdata18);
    mp_TRACE_IN_18_transactor->TDEST(trace_tdest18);
    mp_TRACE_IN_18_transactor->TID(trace_tid18);
    mp_TRACE_IN_18_transactor->TLAST(trace_tlast18);
    mp_TRACE_IN_18_transactor->TREADY(trace_tready18);
    mp_TRACE_IN_18_transactor->TVALID(trace_tvalid18);
    mp_TRACE_IN_18_transactor->CLK(trace_clk);
    mp_TRACE_IN_18_transactor->RST(trace_aresetn);

    // TRACE_IN_18' transactor sockets

    mp_impl->trace_socket18->bind(*(mp_TRACE_IN_18_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_19' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_19_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_19' transactor parameters
    xsc::common_cpp::properties TRACE_IN_19_transactor_param_props;
    TRACE_IN_19_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_19_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_19_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_19_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_19_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_19_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_19_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_19_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_19_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_19_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_19_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_19_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_19_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_19_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_19_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_19_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_19_transactor", TRACE_IN_19_transactor_param_props);

    // TRACE_IN_19' transactor ports

    mp_TRACE_IN_19_transactor->TDATA(trace_tdata19);
    mp_TRACE_IN_19_transactor->TDEST(trace_tdest19);
    mp_TRACE_IN_19_transactor->TID(trace_tid19);
    mp_TRACE_IN_19_transactor->TLAST(trace_tlast19);
    mp_TRACE_IN_19_transactor->TREADY(trace_tready19);
    mp_TRACE_IN_19_transactor->TVALID(trace_tvalid19);
    mp_TRACE_IN_19_transactor->CLK(trace_clk);
    mp_TRACE_IN_19_transactor->RST(trace_aresetn);

    // TRACE_IN_19' transactor sockets

    mp_impl->trace_socket19->bind(*(mp_TRACE_IN_19_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_20' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_20_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_20' transactor parameters
    xsc::common_cpp::properties TRACE_IN_20_transactor_param_props;
    TRACE_IN_20_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_20_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_20_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_20_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_20_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_20_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_20_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_20_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_20_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_20_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_20_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_20_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_20_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_20_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_20_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_20_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_20_transactor", TRACE_IN_20_transactor_param_props);

    // TRACE_IN_20' transactor ports

    mp_TRACE_IN_20_transactor->TDATA(trace_tdata20);
    mp_TRACE_IN_20_transactor->TDEST(trace_tdest20);
    mp_TRACE_IN_20_transactor->TID(trace_tid20);
    mp_TRACE_IN_20_transactor->TLAST(trace_tlast20);
    mp_TRACE_IN_20_transactor->TREADY(trace_tready20);
    mp_TRACE_IN_20_transactor->TVALID(trace_tvalid20);
    mp_TRACE_IN_20_transactor->CLK(trace_clk);
    mp_TRACE_IN_20_transactor->RST(trace_aresetn);

    // TRACE_IN_20' transactor sockets

    mp_impl->trace_socket20->bind(*(mp_TRACE_IN_20_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_21' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_21_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_21' transactor parameters
    xsc::common_cpp::properties TRACE_IN_21_transactor_param_props;
    TRACE_IN_21_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_21_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_21_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_21_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_21_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_21_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_21_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_21_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_21_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_21_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_21_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_21_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_21_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_21_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_21_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_21_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_21_transactor", TRACE_IN_21_transactor_param_props);

    // TRACE_IN_21' transactor ports

    mp_TRACE_IN_21_transactor->TDATA(trace_tdata21);
    mp_TRACE_IN_21_transactor->TDEST(trace_tdest21);
    mp_TRACE_IN_21_transactor->TID(trace_tid21);
    mp_TRACE_IN_21_transactor->TLAST(trace_tlast21);
    mp_TRACE_IN_21_transactor->TREADY(trace_tready21);
    mp_TRACE_IN_21_transactor->TVALID(trace_tvalid21);
    mp_TRACE_IN_21_transactor->CLK(trace_clk);
    mp_TRACE_IN_21_transactor->RST(trace_aresetn);

    // TRACE_IN_21' transactor sockets

    mp_impl->trace_socket21->bind(*(mp_TRACE_IN_21_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_22' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_22_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_22' transactor parameters
    xsc::common_cpp::properties TRACE_IN_22_transactor_param_props;
    TRACE_IN_22_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_22_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_22_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_22_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_22_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_22_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_22_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_22_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_22_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_22_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_22_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_22_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_22_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_22_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_22_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_22_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_22_transactor", TRACE_IN_22_transactor_param_props);

    // TRACE_IN_22' transactor ports

    mp_TRACE_IN_22_transactor->TDATA(trace_tdata22);
    mp_TRACE_IN_22_transactor->TDEST(trace_tdest22);
    mp_TRACE_IN_22_transactor->TID(trace_tid22);
    mp_TRACE_IN_22_transactor->TLAST(trace_tlast22);
    mp_TRACE_IN_22_transactor->TREADY(trace_tready22);
    mp_TRACE_IN_22_transactor->TVALID(trace_tvalid22);
    mp_TRACE_IN_22_transactor->CLK(trace_clk);
    mp_TRACE_IN_22_transactor->RST(trace_aresetn);

    // TRACE_IN_22' transactor sockets

    mp_impl->trace_socket22->bind(*(mp_TRACE_IN_22_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_23' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_23_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_23' transactor parameters
    xsc::common_cpp::properties TRACE_IN_23_transactor_param_props;
    TRACE_IN_23_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_23_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_23_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_23_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_23_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_23_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_23_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_23_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_23_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_23_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_23_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_23_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_23_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_23_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_23_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_23_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_23_transactor", TRACE_IN_23_transactor_param_props);

    // TRACE_IN_23' transactor ports

    mp_TRACE_IN_23_transactor->TDATA(trace_tdata23);
    mp_TRACE_IN_23_transactor->TDEST(trace_tdest23);
    mp_TRACE_IN_23_transactor->TID(trace_tid23);
    mp_TRACE_IN_23_transactor->TLAST(trace_tlast23);
    mp_TRACE_IN_23_transactor->TREADY(trace_tready23);
    mp_TRACE_IN_23_transactor->TVALID(trace_tvalid23);
    mp_TRACE_IN_23_transactor->CLK(trace_clk);
    mp_TRACE_IN_23_transactor->RST(trace_aresetn);

    // TRACE_IN_23' transactor sockets

    mp_impl->trace_socket23->bind(*(mp_TRACE_IN_23_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_24' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_24_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_24' transactor parameters
    xsc::common_cpp::properties TRACE_IN_24_transactor_param_props;
    TRACE_IN_24_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_24_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_24_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_24_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_24_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_24_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_24_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_24_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_24_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_24_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_24_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_24_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_24_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_24_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_24_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_24_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_24_transactor", TRACE_IN_24_transactor_param_props);

    // TRACE_IN_24' transactor ports

    mp_TRACE_IN_24_transactor->TDATA(trace_tdata24);
    mp_TRACE_IN_24_transactor->TDEST(trace_tdest24);
    mp_TRACE_IN_24_transactor->TID(trace_tid24);
    mp_TRACE_IN_24_transactor->TLAST(trace_tlast24);
    mp_TRACE_IN_24_transactor->TREADY(trace_tready24);
    mp_TRACE_IN_24_transactor->TVALID(trace_tvalid24);
    mp_TRACE_IN_24_transactor->CLK(trace_clk);
    mp_TRACE_IN_24_transactor->RST(trace_aresetn);

    // TRACE_IN_24' transactor sockets

    mp_impl->trace_socket24->bind(*(mp_TRACE_IN_24_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_25' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_25_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_25' transactor parameters
    xsc::common_cpp::properties TRACE_IN_25_transactor_param_props;
    TRACE_IN_25_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_25_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_25_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_25_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_25_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_25_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_25_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_25_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_25_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_25_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_25_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_25_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_25_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_25_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_25_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_25_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_25_transactor", TRACE_IN_25_transactor_param_props);

    // TRACE_IN_25' transactor ports

    mp_TRACE_IN_25_transactor->TDATA(trace_tdata25);
    mp_TRACE_IN_25_transactor->TDEST(trace_tdest25);
    mp_TRACE_IN_25_transactor->TID(trace_tid25);
    mp_TRACE_IN_25_transactor->TLAST(trace_tlast25);
    mp_TRACE_IN_25_transactor->TREADY(trace_tready25);
    mp_TRACE_IN_25_transactor->TVALID(trace_tvalid25);
    mp_TRACE_IN_25_transactor->CLK(trace_clk);
    mp_TRACE_IN_25_transactor->RST(trace_aresetn);

    // TRACE_IN_25' transactor sockets

    mp_impl->trace_socket25->bind(*(mp_TRACE_IN_25_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_26' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_26_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_26' transactor parameters
    xsc::common_cpp::properties TRACE_IN_26_transactor_param_props;
    TRACE_IN_26_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_26_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_26_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_26_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_26_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_26_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_26_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_26_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_26_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_26_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_26_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_26_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_26_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_26_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_26_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_26_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_26_transactor", TRACE_IN_26_transactor_param_props);

    // TRACE_IN_26' transactor ports

    mp_TRACE_IN_26_transactor->TDATA(trace_tdata26);
    mp_TRACE_IN_26_transactor->TDEST(trace_tdest26);
    mp_TRACE_IN_26_transactor->TID(trace_tid26);
    mp_TRACE_IN_26_transactor->TLAST(trace_tlast26);
    mp_TRACE_IN_26_transactor->TREADY(trace_tready26);
    mp_TRACE_IN_26_transactor->TVALID(trace_tvalid26);
    mp_TRACE_IN_26_transactor->CLK(trace_clk);
    mp_TRACE_IN_26_transactor->RST(trace_aresetn);

    // TRACE_IN_26' transactor sockets

    mp_impl->trace_socket26->bind(*(mp_TRACE_IN_26_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_27' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_27_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_27' transactor parameters
    xsc::common_cpp::properties TRACE_IN_27_transactor_param_props;
    TRACE_IN_27_transactor_param_props.addLong("TDATA_NUM_BYTES", "1");
    TRACE_IN_27_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_27_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_27_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_27_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_27_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_27_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_27_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_27_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_27_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_27_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_27_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_27_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_27_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_27_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_27_transactor = new xtlm::xaxis_pin2xtlm_t<1,1,8,8,1,1>("TRACE_IN_27_transactor", TRACE_IN_27_transactor_param_props);

    // TRACE_IN_27' transactor ports

    mp_TRACE_IN_27_transactor->TDATA(trace_tdata27);
    mp_TRACE_IN_27_transactor->TDEST(trace_tdest27);
    mp_TRACE_IN_27_transactor->TID(trace_tid27);
    mp_TRACE_IN_27_transactor->TLAST(trace_tlast27);
    mp_TRACE_IN_27_transactor->TREADY(trace_tready27);
    mp_TRACE_IN_27_transactor->TVALID(trace_tvalid27);
    mp_TRACE_IN_27_transactor->CLK(trace_clk);
    mp_TRACE_IN_27_transactor->RST(trace_aresetn);

    // TRACE_IN_27' transactor sockets

    mp_impl->trace_socket27->bind(*(mp_TRACE_IN_27_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_28' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_28_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_28' transactor parameters
    xsc::common_cpp::properties TRACE_IN_28_transactor_param_props;
    TRACE_IN_28_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_28_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_28_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_28_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_28_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_28_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_28_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_28_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_28_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_28_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_28_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_28_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_28_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_28_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_28_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_28_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_28_transactor", TRACE_IN_28_transactor_param_props);

    // TRACE_IN_28' transactor ports

    mp_TRACE_IN_28_transactor->TDATA(trace_tdata28);
    mp_TRACE_IN_28_transactor->TDEST(trace_tdest28);
    mp_TRACE_IN_28_transactor->TID(trace_tid28);
    mp_TRACE_IN_28_transactor->TLAST(trace_tlast28);
    mp_TRACE_IN_28_transactor->TREADY(trace_tready28);
    mp_TRACE_IN_28_transactor->TVALID(trace_tvalid28);
    mp_TRACE_IN_28_transactor->CLK(trace_clk);
    mp_TRACE_IN_28_transactor->RST(trace_aresetn);

    // TRACE_IN_28' transactor sockets

    mp_impl->trace_socket28->bind(*(mp_TRACE_IN_28_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_29' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_29_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_29' transactor parameters
    xsc::common_cpp::properties TRACE_IN_29_transactor_param_props;
    TRACE_IN_29_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_29_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_29_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_29_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_29_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_29_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_29_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_29_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_29_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_29_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_29_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_29_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_29_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_29_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_29_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_29_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_29_transactor", TRACE_IN_29_transactor_param_props);

    // TRACE_IN_29' transactor ports

    mp_TRACE_IN_29_transactor->TDATA(trace_tdata29);
    mp_TRACE_IN_29_transactor->TDEST(trace_tdest29);
    mp_TRACE_IN_29_transactor->TID(trace_tid29);
    mp_TRACE_IN_29_transactor->TLAST(trace_tlast29);
    mp_TRACE_IN_29_transactor->TREADY(trace_tready29);
    mp_TRACE_IN_29_transactor->TVALID(trace_tvalid29);
    mp_TRACE_IN_29_transactor->CLK(trace_clk);
    mp_TRACE_IN_29_transactor->RST(trace_aresetn);

    // TRACE_IN_29' transactor sockets

    mp_impl->trace_socket29->bind(*(mp_TRACE_IN_29_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_30' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_30_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_30' transactor parameters
    xsc::common_cpp::properties TRACE_IN_30_transactor_param_props;
    TRACE_IN_30_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_30_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_30_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_30_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_30_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_30_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_30_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_30_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_30_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_30_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_30_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_30_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_30_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_30_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_30_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_30_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_30_transactor", TRACE_IN_30_transactor_param_props);

    // TRACE_IN_30' transactor ports

    mp_TRACE_IN_30_transactor->TDATA(trace_tdata30);
    mp_TRACE_IN_30_transactor->TDEST(trace_tdest30);
    mp_TRACE_IN_30_transactor->TID(trace_tid30);
    mp_TRACE_IN_30_transactor->TLAST(trace_tlast30);
    mp_TRACE_IN_30_transactor->TREADY(trace_tready30);
    mp_TRACE_IN_30_transactor->TVALID(trace_tvalid30);
    mp_TRACE_IN_30_transactor->CLK(trace_clk);
    mp_TRACE_IN_30_transactor->RST(trace_aresetn);

    // TRACE_IN_30' transactor sockets

    mp_impl->trace_socket30->bind(*(mp_TRACE_IN_30_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_31' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_31_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_31' transactor parameters
    xsc::common_cpp::properties TRACE_IN_31_transactor_param_props;
    TRACE_IN_31_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_31_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_31_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_31_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_31_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_31_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_31_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_31_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_31_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_31_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_31_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_31_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_31_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_31_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_31_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_31_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_31_transactor", TRACE_IN_31_transactor_param_props);

    // TRACE_IN_31' transactor ports

    mp_TRACE_IN_31_transactor->TDATA(trace_tdata31);
    mp_TRACE_IN_31_transactor->TDEST(trace_tdest31);
    mp_TRACE_IN_31_transactor->TID(trace_tid31);
    mp_TRACE_IN_31_transactor->TLAST(trace_tlast31);
    mp_TRACE_IN_31_transactor->TREADY(trace_tready31);
    mp_TRACE_IN_31_transactor->TVALID(trace_tvalid31);
    mp_TRACE_IN_31_transactor->CLK(trace_clk);
    mp_TRACE_IN_31_transactor->RST(trace_aresetn);

    // TRACE_IN_31' transactor sockets

    mp_impl->trace_socket31->bind(*(mp_TRACE_IN_31_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_32' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_32_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_32' transactor parameters
    xsc::common_cpp::properties TRACE_IN_32_transactor_param_props;
    TRACE_IN_32_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_32_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_32_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_32_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_32_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_32_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_32_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_32_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_32_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_32_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_32_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_32_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_32_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_32_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_32_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_32_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_32_transactor", TRACE_IN_32_transactor_param_props);

    // TRACE_IN_32' transactor ports

    mp_TRACE_IN_32_transactor->TDATA(trace_tdata32);
    mp_TRACE_IN_32_transactor->TDEST(trace_tdest32);
    mp_TRACE_IN_32_transactor->TID(trace_tid32);
    mp_TRACE_IN_32_transactor->TLAST(trace_tlast32);
    mp_TRACE_IN_32_transactor->TREADY(trace_tready32);
    mp_TRACE_IN_32_transactor->TVALID(trace_tvalid32);
    mp_TRACE_IN_32_transactor->CLK(trace_clk);
    mp_TRACE_IN_32_transactor->RST(trace_aresetn);

    // TRACE_IN_32' transactor sockets

    mp_impl->trace_socket32->bind(*(mp_TRACE_IN_32_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_33' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_33_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_33' transactor parameters
    xsc::common_cpp::properties TRACE_IN_33_transactor_param_props;
    TRACE_IN_33_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_33_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_33_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_33_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_33_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_33_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_33_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_33_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_33_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_33_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_33_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_33_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_33_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_33_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_33_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_33_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_33_transactor", TRACE_IN_33_transactor_param_props);

    // TRACE_IN_33' transactor ports

    mp_TRACE_IN_33_transactor->TDATA(trace_tdata33);
    mp_TRACE_IN_33_transactor->TDEST(trace_tdest33);
    mp_TRACE_IN_33_transactor->TID(trace_tid33);
    mp_TRACE_IN_33_transactor->TLAST(trace_tlast33);
    mp_TRACE_IN_33_transactor->TREADY(trace_tready33);
    mp_TRACE_IN_33_transactor->TVALID(trace_tvalid33);
    mp_TRACE_IN_33_transactor->CLK(trace_clk);
    mp_TRACE_IN_33_transactor->RST(trace_aresetn);

    // TRACE_IN_33' transactor sockets

    mp_impl->trace_socket33->bind(*(mp_TRACE_IN_33_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_34' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_34_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_34' transactor parameters
    xsc::common_cpp::properties TRACE_IN_34_transactor_param_props;
    TRACE_IN_34_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_34_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_34_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_34_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_34_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_34_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_34_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_34_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_34_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_34_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_34_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_34_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_34_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_34_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_34_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_34_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_34_transactor", TRACE_IN_34_transactor_param_props);

    // TRACE_IN_34' transactor ports

    mp_TRACE_IN_34_transactor->TDATA(trace_tdata34);
    mp_TRACE_IN_34_transactor->TDEST(trace_tdest34);
    mp_TRACE_IN_34_transactor->TID(trace_tid34);
    mp_TRACE_IN_34_transactor->TLAST(trace_tlast34);
    mp_TRACE_IN_34_transactor->TREADY(trace_tready34);
    mp_TRACE_IN_34_transactor->TVALID(trace_tvalid34);
    mp_TRACE_IN_34_transactor->CLK(trace_clk);
    mp_TRACE_IN_34_transactor->RST(trace_aresetn);

    // TRACE_IN_34' transactor sockets

    mp_impl->trace_socket34->bind(*(mp_TRACE_IN_34_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_35' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_35_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_35' transactor parameters
    xsc::common_cpp::properties TRACE_IN_35_transactor_param_props;
    TRACE_IN_35_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_35_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_35_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_35_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_35_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_35_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_35_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_35_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_35_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_35_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_35_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_35_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_35_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_35_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_35_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_35_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_35_transactor", TRACE_IN_35_transactor_param_props);

    // TRACE_IN_35' transactor ports

    mp_TRACE_IN_35_transactor->TDATA(trace_tdata35);
    mp_TRACE_IN_35_transactor->TDEST(trace_tdest35);
    mp_TRACE_IN_35_transactor->TID(trace_tid35);
    mp_TRACE_IN_35_transactor->TLAST(trace_tlast35);
    mp_TRACE_IN_35_transactor->TREADY(trace_tready35);
    mp_TRACE_IN_35_transactor->TVALID(trace_tvalid35);
    mp_TRACE_IN_35_transactor->CLK(trace_clk);
    mp_TRACE_IN_35_transactor->RST(trace_aresetn);

    // TRACE_IN_35' transactor sockets

    mp_impl->trace_socket35->bind(*(mp_TRACE_IN_35_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_36' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_36_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_36' transactor parameters
    xsc::common_cpp::properties TRACE_IN_36_transactor_param_props;
    TRACE_IN_36_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_36_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_36_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_36_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_36_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_36_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_36_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_36_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_36_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_36_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_36_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_36_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_36_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_36_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_36_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_36_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_36_transactor", TRACE_IN_36_transactor_param_props);

    // TRACE_IN_36' transactor ports

    mp_TRACE_IN_36_transactor->TDATA(trace_tdata36);
    mp_TRACE_IN_36_transactor->TDEST(trace_tdest36);
    mp_TRACE_IN_36_transactor->TID(trace_tid36);
    mp_TRACE_IN_36_transactor->TLAST(trace_tlast36);
    mp_TRACE_IN_36_transactor->TREADY(trace_tready36);
    mp_TRACE_IN_36_transactor->TVALID(trace_tvalid36);
    mp_TRACE_IN_36_transactor->CLK(trace_clk);
    mp_TRACE_IN_36_transactor->RST(trace_aresetn);

    // TRACE_IN_36' transactor sockets

    mp_impl->trace_socket36->bind(*(mp_TRACE_IN_36_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_37' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_37_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_37' transactor parameters
    xsc::common_cpp::properties TRACE_IN_37_transactor_param_props;
    TRACE_IN_37_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_37_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_37_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_37_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_37_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_37_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_37_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_37_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_37_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_37_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_37_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_37_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_37_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_37_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_37_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_37_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_37_transactor", TRACE_IN_37_transactor_param_props);

    // TRACE_IN_37' transactor ports

    mp_TRACE_IN_37_transactor->TDATA(trace_tdata37);
    mp_TRACE_IN_37_transactor->TDEST(trace_tdest37);
    mp_TRACE_IN_37_transactor->TID(trace_tid37);
    mp_TRACE_IN_37_transactor->TLAST(trace_tlast37);
    mp_TRACE_IN_37_transactor->TREADY(trace_tready37);
    mp_TRACE_IN_37_transactor->TVALID(trace_tvalid37);
    mp_TRACE_IN_37_transactor->CLK(trace_clk);
    mp_TRACE_IN_37_transactor->RST(trace_aresetn);

    // TRACE_IN_37' transactor sockets

    mp_impl->trace_socket37->bind(*(mp_TRACE_IN_37_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_38' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_38_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_38' transactor parameters
    xsc::common_cpp::properties TRACE_IN_38_transactor_param_props;
    TRACE_IN_38_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_38_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_38_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_38_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_38_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_38_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_38_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_38_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_38_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_38_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_38_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_38_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_38_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_38_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_38_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_38_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_38_transactor", TRACE_IN_38_transactor_param_props);

    // TRACE_IN_38' transactor ports

    mp_TRACE_IN_38_transactor->TDATA(trace_tdata38);
    mp_TRACE_IN_38_transactor->TDEST(trace_tdest38);
    mp_TRACE_IN_38_transactor->TID(trace_tid38);
    mp_TRACE_IN_38_transactor->TLAST(trace_tlast38);
    mp_TRACE_IN_38_transactor->TREADY(trace_tready38);
    mp_TRACE_IN_38_transactor->TVALID(trace_tvalid38);
    mp_TRACE_IN_38_transactor->CLK(trace_clk);
    mp_TRACE_IN_38_transactor->RST(trace_aresetn);

    // TRACE_IN_38' transactor sockets

    mp_impl->trace_socket38->bind(*(mp_TRACE_IN_38_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_39' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_39_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_39' transactor parameters
    xsc::common_cpp::properties TRACE_IN_39_transactor_param_props;
    TRACE_IN_39_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_39_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_39_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_39_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_39_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_39_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_39_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_39_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_39_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_39_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_39_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_39_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_39_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_39_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_39_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_39_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_39_transactor", TRACE_IN_39_transactor_param_props);

    // TRACE_IN_39' transactor ports

    mp_TRACE_IN_39_transactor->TDATA(trace_tdata39);
    mp_TRACE_IN_39_transactor->TDEST(trace_tdest39);
    mp_TRACE_IN_39_transactor->TID(trace_tid39);
    mp_TRACE_IN_39_transactor->TLAST(trace_tlast39);
    mp_TRACE_IN_39_transactor->TREADY(trace_tready39);
    mp_TRACE_IN_39_transactor->TVALID(trace_tvalid39);
    mp_TRACE_IN_39_transactor->CLK(trace_clk);
    mp_TRACE_IN_39_transactor->RST(trace_aresetn);

    // TRACE_IN_39' transactor sockets

    mp_impl->trace_socket39->bind(*(mp_TRACE_IN_39_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_40' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_40_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_40' transactor parameters
    xsc::common_cpp::properties TRACE_IN_40_transactor_param_props;
    TRACE_IN_40_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_40_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_40_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_40_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_40_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_40_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_40_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_40_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_40_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_40_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_40_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_40_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_40_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_40_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_40_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_40_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_40_transactor", TRACE_IN_40_transactor_param_props);

    // TRACE_IN_40' transactor ports

    mp_TRACE_IN_40_transactor->TDATA(trace_tdata40);
    mp_TRACE_IN_40_transactor->TDEST(trace_tdest40);
    mp_TRACE_IN_40_transactor->TID(trace_tid40);
    mp_TRACE_IN_40_transactor->TLAST(trace_tlast40);
    mp_TRACE_IN_40_transactor->TREADY(trace_tready40);
    mp_TRACE_IN_40_transactor->TVALID(trace_tvalid40);
    mp_TRACE_IN_40_transactor->CLK(trace_clk);
    mp_TRACE_IN_40_transactor->RST(trace_aresetn);

    // TRACE_IN_40' transactor sockets

    mp_impl->trace_socket40->bind(*(mp_TRACE_IN_40_transactor->socket));
  }
  else
  {
  }

}

#endif // XM_SYSTEMC




#ifdef RIVIERA
pfm_dynamic_dpa_hub_0::pfm_dynamic_dpa_hub_0(const sc_core::sc_module_name& nm) : pfm_dynamic_dpa_hub_0_sc(nm), s_aximm_clk("s_aximm_clk"), s_aximm_aresetn("s_aximm_aresetn"), trace_clk("trace_clk"), trace_aresetn("trace_aresetn"), axilite_clk("axilite_clk"), axilite_aresetn("axilite_aresetn"), trace_tready0("trace_tready0"), trace_tdata0("trace_tdata0"), trace_tlast0("trace_tlast0"), trace_tid0("trace_tid0"), trace_tdest0("trace_tdest0"), trace_tvalid0("trace_tvalid0"), trace_tready1("trace_tready1"), trace_tdata1("trace_tdata1"), trace_tlast1("trace_tlast1"), trace_tid1("trace_tid1"), trace_tdest1("trace_tdest1"), trace_tvalid1("trace_tvalid1"), trace_tdata2("trace_tdata2"), trace_tready2("trace_tready2"), trace_tlast2("trace_tlast2"), trace_tid2("trace_tid2"), trace_tdest2("trace_tdest2"), trace_tvalid2("trace_tvalid2"), trace_tdata3("trace_tdata3"), trace_tready3("trace_tready3"), trace_tlast3("trace_tlast3"), trace_tid3("trace_tid3"), trace_tdest3("trace_tdest3"), trace_tvalid3("trace_tvalid3"), trace_tdata4("trace_tdata4"), trace_tready4("trace_tready4"), trace_tlast4("trace_tlast4"), trace_tid4("trace_tid4"), trace_tdest4("trace_tdest4"), trace_tvalid4("trace_tvalid4"), trace_tdata5("trace_tdata5"), trace_tready5("trace_tready5"), trace_tlast5("trace_tlast5"), trace_tid5("trace_tid5"), trace_tdest5("trace_tdest5"), trace_tvalid5("trace_tvalid5"), trace_tdata6("trace_tdata6"), trace_tready6("trace_tready6"), trace_tlast6("trace_tlast6"), trace_tid6("trace_tid6"), trace_tdest6("trace_tdest6"), trace_tvalid6("trace_tvalid6"), trace_tdata7("trace_tdata7"), trace_tready7("trace_tready7"), trace_tlast7("trace_tlast7"), trace_tid7("trace_tid7"), trace_tdest7("trace_tdest7"), trace_tvalid7("trace_tvalid7"), trace_tdata8("trace_tdata8"), trace_tready8("trace_tready8"), trace_tlast8("trace_tlast8"), trace_tid8("trace_tid8"), trace_tdest8("trace_tdest8"), trace_tvalid8("trace_tvalid8"), trace_tdata9("trace_tdata9"), trace_tready9("trace_tready9"), trace_tlast9("trace_tlast9"), trace_tid9("trace_tid9"), trace_tdest9("trace_tdest9"), trace_tvalid9("trace_tvalid9"), trace_tdata10("trace_tdata10"), trace_tready10("trace_tready10"), trace_tlast10("trace_tlast10"), trace_tid10("trace_tid10"), trace_tdest10("trace_tdest10"), trace_tvalid10("trace_tvalid10"), trace_tdata11("trace_tdata11"), trace_tready11("trace_tready11"), trace_tlast11("trace_tlast11"), trace_tid11("trace_tid11"), trace_tdest11("trace_tdest11"), trace_tvalid11("trace_tvalid11"), trace_tdata12("trace_tdata12"), trace_tready12("trace_tready12"), trace_tlast12("trace_tlast12"), trace_tid12("trace_tid12"), trace_tdest12("trace_tdest12"), trace_tvalid12("trace_tvalid12"), trace_tdata13("trace_tdata13"), trace_tready13("trace_tready13"), trace_tlast13("trace_tlast13"), trace_tid13("trace_tid13"), trace_tdest13("trace_tdest13"), trace_tvalid13("trace_tvalid13"), trace_tdata14("trace_tdata14"), trace_tready14("trace_tready14"), trace_tlast14("trace_tlast14"), trace_tid14("trace_tid14"), trace_tdest14("trace_tdest14"), trace_tvalid14("trace_tvalid14"), trace_tdata15("trace_tdata15"), trace_tready15("trace_tready15"), trace_tlast15("trace_tlast15"), trace_tid15("trace_tid15"), trace_tdest15("trace_tdest15"), trace_tvalid15("trace_tvalid15"), trace_tdata16("trace_tdata16"), trace_tready16("trace_tready16"), trace_tlast16("trace_tlast16"), trace_tid16("trace_tid16"), trace_tdest16("trace_tdest16"), trace_tvalid16("trace_tvalid16"), trace_tdata17("trace_tdata17"), trace_tready17("trace_tready17"), trace_tlast17("trace_tlast17"), trace_tid17("trace_tid17"), trace_tdest17("trace_tdest17"), trace_tvalid17("trace_tvalid17"), trace_tdata18("trace_tdata18"), trace_tready18("trace_tready18"), trace_tlast18("trace_tlast18"), trace_tid18("trace_tid18"), trace_tdest18("trace_tdest18"), trace_tvalid18("trace_tvalid18"), trace_tdata19("trace_tdata19"), trace_tready19("trace_tready19"), trace_tlast19("trace_tlast19"), trace_tid19("trace_tid19"), trace_tdest19("trace_tdest19"), trace_tvalid19("trace_tvalid19"), trace_tdata20("trace_tdata20"), trace_tready20("trace_tready20"), trace_tlast20("trace_tlast20"), trace_tid20("trace_tid20"), trace_tdest20("trace_tdest20"), trace_tvalid20("trace_tvalid20"), trace_tdata21("trace_tdata21"), trace_tready21("trace_tready21"), trace_tlast21("trace_tlast21"), trace_tid21("trace_tid21"), trace_tdest21("trace_tdest21"), trace_tvalid21("trace_tvalid21"), trace_tdata22("trace_tdata22"), trace_tready22("trace_tready22"), trace_tlast22("trace_tlast22"), trace_tid22("trace_tid22"), trace_tdest22("trace_tdest22"), trace_tvalid22("trace_tvalid22"), trace_tdata23("trace_tdata23"), trace_tready23("trace_tready23"), trace_tlast23("trace_tlast23"), trace_tid23("trace_tid23"), trace_tdest23("trace_tdest23"), trace_tvalid23("trace_tvalid23"), trace_tdata24("trace_tdata24"), trace_tready24("trace_tready24"), trace_tlast24("trace_tlast24"), trace_tid24("trace_tid24"), trace_tdest24("trace_tdest24"), trace_tvalid24("trace_tvalid24"), trace_tdata25("trace_tdata25"), trace_tready25("trace_tready25"), trace_tlast25("trace_tlast25"), trace_tid25("trace_tid25"), trace_tdest25("trace_tdest25"), trace_tvalid25("trace_tvalid25"), trace_tdata26("trace_tdata26"), trace_tready26("trace_tready26"), trace_tlast26("trace_tlast26"), trace_tid26("trace_tid26"), trace_tdest26("trace_tdest26"), trace_tvalid26("trace_tvalid26"), trace_tdata27("trace_tdata27"), trace_tready27("trace_tready27"), trace_tlast27("trace_tlast27"), trace_tid27("trace_tid27"), trace_tdest27("trace_tdest27"), trace_tvalid27("trace_tvalid27"), trace_tdata28("trace_tdata28"), trace_tready28("trace_tready28"), trace_tlast28("trace_tlast28"), trace_tid28("trace_tid28"), trace_tdest28("trace_tdest28"), trace_tvalid28("trace_tvalid28"), trace_tdata29("trace_tdata29"), trace_tready29("trace_tready29"), trace_tlast29("trace_tlast29"), trace_tid29("trace_tid29"), trace_tdest29("trace_tdest29"), trace_tvalid29("trace_tvalid29"), trace_tdata30("trace_tdata30"), trace_tready30("trace_tready30"), trace_tlast30("trace_tlast30"), trace_tid30("trace_tid30"), trace_tdest30("trace_tdest30"), trace_tvalid30("trace_tvalid30"), trace_tdata31("trace_tdata31"), trace_tready31("trace_tready31"), trace_tlast31("trace_tlast31"), trace_tid31("trace_tid31"), trace_tdest31("trace_tdest31"), trace_tvalid31("trace_tvalid31"), trace_tdata32("trace_tdata32"), trace_tready32("trace_tready32"), trace_tlast32("trace_tlast32"), trace_tid32("trace_tid32"), trace_tdest32("trace_tdest32"), trace_tvalid32("trace_tvalid32"), trace_tdata33("trace_tdata33"), trace_tready33("trace_tready33"), trace_tlast33("trace_tlast33"), trace_tid33("trace_tid33"), trace_tdest33("trace_tdest33"), trace_tvalid33("trace_tvalid33"), trace_tdata34("trace_tdata34"), trace_tready34("trace_tready34"), trace_tlast34("trace_tlast34"), trace_tid34("trace_tid34"), trace_tdest34("trace_tdest34"), trace_tvalid34("trace_tvalid34"), trace_tdata35("trace_tdata35"), trace_tready35("trace_tready35"), trace_tlast35("trace_tlast35"), trace_tid35("trace_tid35"), trace_tdest35("trace_tdest35"), trace_tvalid35("trace_tvalid35"), trace_tdata36("trace_tdata36"), trace_tready36("trace_tready36"), trace_tlast36("trace_tlast36"), trace_tid36("trace_tid36"), trace_tdest36("trace_tdest36"), trace_tvalid36("trace_tvalid36"), trace_tdata37("trace_tdata37"), trace_tready37("trace_tready37"), trace_tlast37("trace_tlast37"), trace_tid37("trace_tid37"), trace_tdest37("trace_tdest37"), trace_tvalid37("trace_tvalid37"), trace_tdata38("trace_tdata38"), trace_tready38("trace_tready38"), trace_tlast38("trace_tlast38"), trace_tid38("trace_tid38"), trace_tdest38("trace_tdest38"), trace_tvalid38("trace_tvalid38"), trace_tdata39("trace_tdata39"), trace_tready39("trace_tready39"), trace_tlast39("trace_tlast39"), trace_tid39("trace_tid39"), trace_tdest39("trace_tdest39"), trace_tvalid39("trace_tvalid39"), trace_tdata40("trace_tdata40"), trace_tready40("trace_tready40"), trace_tlast40("trace_tlast40"), trace_tid40("trace_tid40"), trace_tdest40("trace_tdest40"), trace_tvalid40("trace_tvalid40"), s_axi_awaddr("s_axi_awaddr"), s_axi_awvalid("s_axi_awvalid"), s_axi_awready("s_axi_awready"), s_axi_wdata("s_axi_wdata"), s_axi_wstrb("s_axi_wstrb"), s_axi_wvalid("s_axi_wvalid"), s_axi_wready("s_axi_wready"), s_axi_bvalid("s_axi_bvalid"), s_axi_bready("s_axi_bready"), s_axi_bresp("s_axi_bresp"), s_axi_araddr("s_axi_araddr"), s_axi_arvalid("s_axi_arvalid"), s_axi_arready("s_axi_arready"), s_axi_rdata("s_axi_rdata"), s_axi_rresp("s_axi_rresp"), s_axi_rvalid("s_axi_rvalid"), s_axi_rready("s_axi_rready"), s_axihub_awaddr("s_axihub_awaddr"), s_axihub_awvalid("s_axihub_awvalid"), s_axihub_awready("s_axihub_awready"), s_axihub_wdata("s_axihub_wdata"), s_axihub_wstrb("s_axihub_wstrb"), s_axihub_wvalid("s_axihub_wvalid"), s_axihub_wready("s_axihub_wready"), s_axihub_bvalid("s_axihub_bvalid"), s_axihub_bready("s_axihub_bready"), s_axihub_bresp("s_axihub_bresp"), s_axihub_araddr("s_axihub_araddr"), s_axihub_arvalid("s_axihub_arvalid"), s_axihub_arready("s_axihub_arready"), s_axihub_rdata("s_axihub_rdata"), s_axihub_rresp("s_axihub_rresp"), s_axihub_rvalid("s_axihub_rvalid"), s_axihub_rready("s_axihub_rready"), s_aximm_awaddr("s_aximm_awaddr"), s_aximm_awvalid("s_aximm_awvalid"), s_aximm_awready("s_aximm_awready"), s_aximm_awlen("s_aximm_awlen"), s_aximm_awsize("s_aximm_awsize"), s_aximm_awburst("s_aximm_awburst"), s_aximm_awid("s_aximm_awid"), s_aximm_wdata("s_aximm_wdata"), s_aximm_wstrb("s_aximm_wstrb"), s_aximm_wvalid("s_aximm_wvalid"), s_aximm_wready("s_aximm_wready"), s_aximm_bvalid("s_aximm_bvalid"), s_aximm_bready("s_aximm_bready"), s_aximm_wlast("s_aximm_wlast"), s_aximm_rlast("s_aximm_rlast"), s_aximm_bresp("s_aximm_bresp"), s_aximm_bid("s_aximm_bid"), s_aximm_araddr("s_aximm_araddr"), s_aximm_arvalid("s_aximm_arvalid"), s_aximm_arready("s_aximm_arready"), s_aximm_arlen("s_aximm_arlen"), s_aximm_arsize("s_aximm_arsize"), s_aximm_arburst("s_aximm_arburst"), s_aximm_arid("s_aximm_arid"), s_aximm_rdata("s_aximm_rdata"), s_aximm_rid("s_aximm_rid"), s_aximm_rresp("s_aximm_rresp"), s_aximm_rvalid("s_aximm_rvalid"), s_aximm_rready("s_aximm_rready")
{

  // initialize pins
  mp_impl->s_aximm_clk(s_aximm_clk);
  mp_impl->s_aximm_aresetn(s_aximm_aresetn);
  mp_impl->trace_clk(trace_clk);
  mp_impl->trace_aresetn(trace_aresetn);
  mp_impl->axilite_clk(axilite_clk);
  mp_impl->axilite_aresetn(axilite_aresetn);

  // initialize transactors
  mp_S_AXIMM_transactor = NULL;
  mp_S_AXIFIFO_transactor = NULL;
  mp_S_AXIHUB_transactor = NULL;
  mp_TRACE_IN_0_transactor = NULL;
  mp_TRACE_IN_1_transactor = NULL;
  mp_TRACE_IN_2_transactor = NULL;
  mp_TRACE_IN_3_transactor = NULL;
  mp_TRACE_IN_4_transactor = NULL;
  mp_TRACE_IN_5_transactor = NULL;
  mp_TRACE_IN_6_transactor = NULL;
  mp_TRACE_IN_7_transactor = NULL;
  mp_TRACE_IN_8_transactor = NULL;
  mp_TRACE_IN_9_transactor = NULL;
  mp_TRACE_IN_10_transactor = NULL;
  mp_TRACE_IN_11_transactor = NULL;
  mp_TRACE_IN_12_transactor = NULL;
  mp_TRACE_IN_13_transactor = NULL;
  mp_TRACE_IN_14_transactor = NULL;
  mp_TRACE_IN_15_transactor = NULL;
  mp_TRACE_IN_16_transactor = NULL;
  mp_TRACE_IN_17_transactor = NULL;
  mp_TRACE_IN_18_transactor = NULL;
  mp_TRACE_IN_19_transactor = NULL;
  mp_TRACE_IN_20_transactor = NULL;
  mp_TRACE_IN_21_transactor = NULL;
  mp_TRACE_IN_22_transactor = NULL;
  mp_TRACE_IN_23_transactor = NULL;
  mp_TRACE_IN_24_transactor = NULL;
  mp_TRACE_IN_25_transactor = NULL;
  mp_TRACE_IN_26_transactor = NULL;
  mp_TRACE_IN_27_transactor = NULL;
  mp_TRACE_IN_28_transactor = NULL;
  mp_TRACE_IN_29_transactor = NULL;
  mp_TRACE_IN_30_transactor = NULL;
  mp_TRACE_IN_31_transactor = NULL;
  mp_TRACE_IN_32_transactor = NULL;
  mp_TRACE_IN_33_transactor = NULL;
  mp_TRACE_IN_34_transactor = NULL;
  mp_TRACE_IN_35_transactor = NULL;
  mp_TRACE_IN_36_transactor = NULL;
  mp_TRACE_IN_37_transactor = NULL;
  mp_TRACE_IN_38_transactor = NULL;
  mp_TRACE_IN_39_transactor = NULL;
  mp_TRACE_IN_40_transactor = NULL;

  // initialize socket stubs

}

void pfm_dynamic_dpa_hub_0::before_end_of_elaboration()
{
  // configure 'S_AXIMM' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "S_AXIMM_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'S_AXIMM' transactor parameters
    xsc::common_cpp::properties S_AXIMM_transactor_param_props;
    S_AXIMM_transactor_param_props.addLong("DATA_WIDTH", "64");
    S_AXIMM_transactor_param_props.addLong("FREQ_HZ", "300000000");
    S_AXIMM_transactor_param_props.addLong("ID_WIDTH", "1");
    S_AXIMM_transactor_param_props.addLong("ADDR_WIDTH", "32");
    S_AXIMM_transactor_param_props.addLong("AWUSER_WIDTH", "0");
    S_AXIMM_transactor_param_props.addLong("ARUSER_WIDTH", "0");
    S_AXIMM_transactor_param_props.addLong("WUSER_WIDTH", "0");
    S_AXIMM_transactor_param_props.addLong("RUSER_WIDTH", "0");
    S_AXIMM_transactor_param_props.addLong("BUSER_WIDTH", "0");
    S_AXIMM_transactor_param_props.addLong("HAS_BURST", "1");
    S_AXIMM_transactor_param_props.addLong("HAS_LOCK", "0");
    S_AXIMM_transactor_param_props.addLong("HAS_PROT", "0");
    S_AXIMM_transactor_param_props.addLong("HAS_CACHE", "0");
    S_AXIMM_transactor_param_props.addLong("HAS_QOS", "0");
    S_AXIMM_transactor_param_props.addLong("HAS_REGION", "0");
    S_AXIMM_transactor_param_props.addLong("HAS_WSTRB", "1");
    S_AXIMM_transactor_param_props.addLong("HAS_BRESP", "1");
    S_AXIMM_transactor_param_props.addLong("HAS_RRESP", "1");
    S_AXIMM_transactor_param_props.addLong("SUPPORTS_NARROW_BURST", "0");
    S_AXIMM_transactor_param_props.addLong("NUM_READ_OUTSTANDING", "2");
    S_AXIMM_transactor_param_props.addLong("NUM_WRITE_OUTSTANDING", "2");
    S_AXIMM_transactor_param_props.addLong("MAX_BURST_LENGTH", "128");
    S_AXIMM_transactor_param_props.addLong("NUM_READ_THREADS", "1");
    S_AXIMM_transactor_param_props.addLong("NUM_WRITE_THREADS", "1");
    S_AXIMM_transactor_param_props.addLong("RUSER_BITS_PER_BYTE", "0");
    S_AXIMM_transactor_param_props.addLong("WUSER_BITS_PER_BYTE", "0");
    S_AXIMM_transactor_param_props.addLong("HAS_SIZE", "1");
    S_AXIMM_transactor_param_props.addLong("HAS_RESET", "1");
    S_AXIMM_transactor_param_props.addFloat("PHASE", "0.0");
    S_AXIMM_transactor_param_props.addString("PROTOCOL", "AXI4");
    S_AXIMM_transactor_param_props.addString("READ_WRITE_MODE", "READ_WRITE");
    S_AXIMM_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_dma_pcie_aclk");
    S_AXIMM_transactor_param_props.addString("SLR_ASSIGNMENT", "");

    mp_S_AXIMM_transactor = new xtlm::xaximm_pin2xtlm_t<64,32,1,1,1,1,1,1>("S_AXIMM_transactor", S_AXIMM_transactor_param_props);

    // S_AXIMM' transactor ports

    mp_S_AXIMM_transactor->ARADDR(s_aximm_araddr);
    mp_S_AXIMM_transactor->ARBURST(s_aximm_arburst);
    mp_S_AXIMM_transactor->ARID(s_aximm_arid);
    mp_S_AXIMM_transactor->ARLEN(s_aximm_arlen);
    mp_S_AXIMM_transactor->ARREADY(s_aximm_arready);
    mp_S_AXIMM_transactor->ARSIZE(s_aximm_arsize);
    mp_S_AXIMM_transactor->ARVALID(s_aximm_arvalid);
    mp_S_AXIMM_transactor->AWADDR(s_aximm_awaddr);
    mp_S_AXIMM_transactor->AWBURST(s_aximm_awburst);
    mp_S_AXIMM_transactor->AWID(s_aximm_awid);
    mp_S_AXIMM_transactor->AWLEN(s_aximm_awlen);
    mp_S_AXIMM_transactor->AWREADY(s_aximm_awready);
    mp_S_AXIMM_transactor->AWSIZE(s_aximm_awsize);
    mp_S_AXIMM_transactor->AWVALID(s_aximm_awvalid);
    mp_S_AXIMM_transactor->BID(s_aximm_bid);
    mp_S_AXIMM_transactor->BREADY(s_aximm_bready);
    mp_S_AXIMM_transactor->BRESP(s_aximm_bresp);
    mp_S_AXIMM_transactor->BVALID(s_aximm_bvalid);
    mp_S_AXIMM_transactor->RDATA(s_aximm_rdata);
    mp_S_AXIMM_transactor->RID(s_aximm_rid);
    mp_S_AXIMM_transactor->RLAST(s_aximm_rlast);
    mp_S_AXIMM_transactor->RREADY(s_aximm_rready);
    mp_S_AXIMM_transactor->RRESP(s_aximm_rresp);
    mp_S_AXIMM_transactor->RVALID(s_aximm_rvalid);
    mp_S_AXIMM_transactor->WDATA(s_aximm_wdata);
    mp_S_AXIMM_transactor->WLAST(s_aximm_wlast);
    mp_S_AXIMM_transactor->WREADY(s_aximm_wready);
    mp_S_AXIMM_transactor->WSTRB(s_aximm_wstrb);
    mp_S_AXIMM_transactor->WVALID(s_aximm_wvalid);
    mp_S_AXIMM_transactor->CLK(s_aximm_clk);
    mp_S_AXIMM_transactor->RST(s_aximm_aresetn);

    // S_AXIMM' transactor sockets

    mp_impl->s_aximm_rd_socket->bind(*(mp_S_AXIMM_transactor->rd_socket));
    mp_impl->s_aximm_wr_socket->bind(*(mp_S_AXIMM_transactor->wr_socket));
  }
  else
  {
  }

  // configure 'S_AXIFIFO' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "S_AXIFIFO_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'S_AXIFIFO' transactor parameters
    xsc::common_cpp::properties S_AXIFIFO_transactor_param_props;
    S_AXIFIFO_transactor_param_props.addLong("DATA_WIDTH", "32");
    S_AXIFIFO_transactor_param_props.addLong("FREQ_HZ", "300000000");
    S_AXIFIFO_transactor_param_props.addLong("ID_WIDTH", "0");
    S_AXIFIFO_transactor_param_props.addLong("ADDR_WIDTH", "8");
    S_AXIFIFO_transactor_param_props.addLong("AWUSER_WIDTH", "0");
    S_AXIFIFO_transactor_param_props.addLong("ARUSER_WIDTH", "0");
    S_AXIFIFO_transactor_param_props.addLong("WUSER_WIDTH", "0");
    S_AXIFIFO_transactor_param_props.addLong("RUSER_WIDTH", "0");
    S_AXIFIFO_transactor_param_props.addLong("BUSER_WIDTH", "0");
    S_AXIFIFO_transactor_param_props.addLong("HAS_BURST", "0");
    S_AXIFIFO_transactor_param_props.addLong("HAS_LOCK", "0");
    S_AXIFIFO_transactor_param_props.addLong("HAS_PROT", "0");
    S_AXIFIFO_transactor_param_props.addLong("HAS_CACHE", "0");
    S_AXIFIFO_transactor_param_props.addLong("HAS_QOS", "0");
    S_AXIFIFO_transactor_param_props.addLong("HAS_REGION", "0");
    S_AXIFIFO_transactor_param_props.addLong("HAS_WSTRB", "1");
    S_AXIFIFO_transactor_param_props.addLong("HAS_BRESP", "1");
    S_AXIFIFO_transactor_param_props.addLong("HAS_RRESP", "1");
    S_AXIFIFO_transactor_param_props.addLong("SUPPORTS_NARROW_BURST", "0");
    S_AXIFIFO_transactor_param_props.addLong("NUM_READ_OUTSTANDING", "2");
    S_AXIFIFO_transactor_param_props.addLong("NUM_WRITE_OUTSTANDING", "2");
    S_AXIFIFO_transactor_param_props.addLong("MAX_BURST_LENGTH", "1");
    S_AXIFIFO_transactor_param_props.addLong("NUM_READ_THREADS", "1");
    S_AXIFIFO_transactor_param_props.addLong("NUM_WRITE_THREADS", "1");
    S_AXIFIFO_transactor_param_props.addLong("RUSER_BITS_PER_BYTE", "0");
    S_AXIFIFO_transactor_param_props.addLong("WUSER_BITS_PER_BYTE", "0");
    S_AXIFIFO_transactor_param_props.addLong("HAS_SIZE", "0");
    S_AXIFIFO_transactor_param_props.addLong("HAS_RESET", "1");
    S_AXIFIFO_transactor_param_props.addFloat("PHASE", "0.0");
    S_AXIFIFO_transactor_param_props.addString("PROTOCOL", "AXI4LITE");
    S_AXIFIFO_transactor_param_props.addString("READ_WRITE_MODE", "READ_WRITE");
    S_AXIFIFO_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    S_AXIFIFO_transactor_param_props.addString("SLR_ASSIGNMENT", "");

    mp_S_AXIFIFO_transactor = new xtlm::xaximm_pin2xtlm_t<32,8,1,1,1,1,1,1>("S_AXIFIFO_transactor", S_AXIFIFO_transactor_param_props);

    // S_AXIFIFO' transactor ports

    mp_S_AXIFIFO_transactor->ARADDR(s_axi_araddr);
    mp_S_AXIFIFO_transactor->ARREADY(s_axi_arready);
    mp_S_AXIFIFO_transactor->ARVALID(s_axi_arvalid);
    mp_S_AXIFIFO_transactor->AWADDR(s_axi_awaddr);
    mp_S_AXIFIFO_transactor->AWREADY(s_axi_awready);
    mp_S_AXIFIFO_transactor->AWVALID(s_axi_awvalid);
    mp_S_AXIFIFO_transactor->BREADY(s_axi_bready);
    mp_S_AXIFIFO_transactor->BRESP(s_axi_bresp);
    mp_S_AXIFIFO_transactor->BVALID(s_axi_bvalid);
    mp_S_AXIFIFO_transactor->RDATA(s_axi_rdata);
    mp_S_AXIFIFO_transactor->RREADY(s_axi_rready);
    mp_S_AXIFIFO_transactor->RRESP(s_axi_rresp);
    mp_S_AXIFIFO_transactor->RVALID(s_axi_rvalid);
    mp_S_AXIFIFO_transactor->WDATA(s_axi_wdata);
    mp_S_AXIFIFO_transactor->WREADY(s_axi_wready);
    mp_S_AXIFIFO_transactor->WSTRB(s_axi_wstrb);
    mp_S_AXIFIFO_transactor->WVALID(s_axi_wvalid);
    mp_S_AXIFIFO_transactor->CLK(axilite_clk);
    mp_S_AXIFIFO_transactor->RST(axilite_aresetn);

    // S_AXIFIFO' transactor sockets

    mp_impl->axilite_rd_socket->bind(*(mp_S_AXIFIFO_transactor->rd_socket));
    mp_impl->axilite_wr_socket->bind(*(mp_S_AXIFIFO_transactor->wr_socket));
  }
  else
  {
  }

  // configure 'S_AXIHUB' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "S_AXIHUB_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'S_AXIHUB' transactor parameters
    xsc::common_cpp::properties S_AXIHUB_transactor_param_props;
    S_AXIHUB_transactor_param_props.addLong("DATA_WIDTH", "32");
    S_AXIHUB_transactor_param_props.addLong("FREQ_HZ", "300000000");
    S_AXIHUB_transactor_param_props.addLong("ID_WIDTH", "0");
    S_AXIHUB_transactor_param_props.addLong("ADDR_WIDTH", "8");
    S_AXIHUB_transactor_param_props.addLong("AWUSER_WIDTH", "0");
    S_AXIHUB_transactor_param_props.addLong("ARUSER_WIDTH", "0");
    S_AXIHUB_transactor_param_props.addLong("WUSER_WIDTH", "0");
    S_AXIHUB_transactor_param_props.addLong("RUSER_WIDTH", "0");
    S_AXIHUB_transactor_param_props.addLong("BUSER_WIDTH", "0");
    S_AXIHUB_transactor_param_props.addLong("HAS_BURST", "0");
    S_AXIHUB_transactor_param_props.addLong("HAS_LOCK", "0");
    S_AXIHUB_transactor_param_props.addLong("HAS_PROT", "0");
    S_AXIHUB_transactor_param_props.addLong("HAS_CACHE", "0");
    S_AXIHUB_transactor_param_props.addLong("HAS_QOS", "0");
    S_AXIHUB_transactor_param_props.addLong("HAS_REGION", "0");
    S_AXIHUB_transactor_param_props.addLong("HAS_WSTRB", "1");
    S_AXIHUB_transactor_param_props.addLong("HAS_BRESP", "1");
    S_AXIHUB_transactor_param_props.addLong("HAS_RRESP", "1");
    S_AXIHUB_transactor_param_props.addLong("SUPPORTS_NARROW_BURST", "0");
    S_AXIHUB_transactor_param_props.addLong("NUM_READ_OUTSTANDING", "2");
    S_AXIHUB_transactor_param_props.addLong("NUM_WRITE_OUTSTANDING", "2");
    S_AXIHUB_transactor_param_props.addLong("MAX_BURST_LENGTH", "1");
    S_AXIHUB_transactor_param_props.addLong("NUM_READ_THREADS", "1");
    S_AXIHUB_transactor_param_props.addLong("NUM_WRITE_THREADS", "1");
    S_AXIHUB_transactor_param_props.addLong("RUSER_BITS_PER_BYTE", "0");
    S_AXIHUB_transactor_param_props.addLong("WUSER_BITS_PER_BYTE", "0");
    S_AXIHUB_transactor_param_props.addLong("HAS_SIZE", "0");
    S_AXIHUB_transactor_param_props.addLong("HAS_RESET", "1");
    S_AXIHUB_transactor_param_props.addFloat("PHASE", "0.0");
    S_AXIHUB_transactor_param_props.addString("PROTOCOL", "AXI4LITE");
    S_AXIHUB_transactor_param_props.addString("READ_WRITE_MODE", "READ_WRITE");
    S_AXIHUB_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    S_AXIHUB_transactor_param_props.addString("SLR_ASSIGNMENT", "");

    mp_S_AXIHUB_transactor = new xtlm::xaximm_pin2xtlm_t<32,8,1,1,1,1,1,1>("S_AXIHUB_transactor", S_AXIHUB_transactor_param_props);

    // S_AXIHUB' transactor ports

    mp_S_AXIHUB_transactor->ARADDR(s_axihub_araddr);
    mp_S_AXIHUB_transactor->ARREADY(s_axihub_arready);
    mp_S_AXIHUB_transactor->ARVALID(s_axihub_arvalid);
    mp_S_AXIHUB_transactor->AWADDR(s_axihub_awaddr);
    mp_S_AXIHUB_transactor->AWREADY(s_axihub_awready);
    mp_S_AXIHUB_transactor->AWVALID(s_axihub_awvalid);
    mp_S_AXIHUB_transactor->BREADY(s_axihub_bready);
    mp_S_AXIHUB_transactor->BRESP(s_axihub_bresp);
    mp_S_AXIHUB_transactor->BVALID(s_axihub_bvalid);
    mp_S_AXIHUB_transactor->RDATA(s_axihub_rdata);
    mp_S_AXIHUB_transactor->RREADY(s_axihub_rready);
    mp_S_AXIHUB_transactor->RRESP(s_axihub_rresp);
    mp_S_AXIHUB_transactor->RVALID(s_axihub_rvalid);
    mp_S_AXIHUB_transactor->WDATA(s_axihub_wdata);
    mp_S_AXIHUB_transactor->WREADY(s_axihub_wready);
    mp_S_AXIHUB_transactor->WSTRB(s_axihub_wstrb);
    mp_S_AXIHUB_transactor->WVALID(s_axihub_wvalid);
    mp_S_AXIHUB_transactor->CLK(axilite_clk);
    mp_S_AXIHUB_transactor->RST(axilite_aresetn);

    // S_AXIHUB' transactor sockets

    mp_impl->s_axihub_rd_socket->bind(*(mp_S_AXIHUB_transactor->rd_socket));
    mp_impl->s_axihub_wr_socket->bind(*(mp_S_AXIHUB_transactor->wr_socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_0' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_0_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_0' transactor parameters
    xsc::common_cpp::properties TRACE_IN_0_transactor_param_props;
    TRACE_IN_0_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_0_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_0_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_0_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_0_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_0_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_0_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_0_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_0_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_0_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_0_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_0_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_0_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_0_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_0_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_0_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_0_transactor", TRACE_IN_0_transactor_param_props);

    // TRACE_IN_0' transactor ports

    mp_TRACE_IN_0_transactor->TDATA(trace_tdata0);
    mp_TRACE_IN_0_transactor->TDEST(trace_tdest0);
    mp_TRACE_IN_0_transactor->TID(trace_tid0);
    mp_TRACE_IN_0_transactor->TLAST(trace_tlast0);
    mp_TRACE_IN_0_transactor->TREADY(trace_tready0);
    mp_TRACE_IN_0_transactor->TVALID(trace_tvalid0);
    mp_TRACE_IN_0_transactor->CLK(trace_clk);
    mp_TRACE_IN_0_transactor->RST(trace_aresetn);

    // TRACE_IN_0' transactor sockets

    mp_impl->trace_socket0->bind(*(mp_TRACE_IN_0_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_1' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_1_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_1' transactor parameters
    xsc::common_cpp::properties TRACE_IN_1_transactor_param_props;
    TRACE_IN_1_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_1_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_1_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_1_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_1_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_1_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_1_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_1_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_1_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_1_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_1_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_1_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_1_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_1_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_1_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_1_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_1_transactor", TRACE_IN_1_transactor_param_props);

    // TRACE_IN_1' transactor ports

    mp_TRACE_IN_1_transactor->TDATA(trace_tdata1);
    mp_TRACE_IN_1_transactor->TDEST(trace_tdest1);
    mp_TRACE_IN_1_transactor->TID(trace_tid1);
    mp_TRACE_IN_1_transactor->TLAST(trace_tlast1);
    mp_TRACE_IN_1_transactor->TREADY(trace_tready1);
    mp_TRACE_IN_1_transactor->TVALID(trace_tvalid1);
    mp_TRACE_IN_1_transactor->CLK(trace_clk);
    mp_TRACE_IN_1_transactor->RST(trace_aresetn);

    // TRACE_IN_1' transactor sockets

    mp_impl->trace_socket1->bind(*(mp_TRACE_IN_1_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_2' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_2_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_2' transactor parameters
    xsc::common_cpp::properties TRACE_IN_2_transactor_param_props;
    TRACE_IN_2_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_2_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_2_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_2_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_2_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_2_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_2_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_2_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_2_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_2_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_2_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_2_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_2_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_2_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_2_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_2_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_2_transactor", TRACE_IN_2_transactor_param_props);

    // TRACE_IN_2' transactor ports

    mp_TRACE_IN_2_transactor->TDATA(trace_tdata2);
    mp_TRACE_IN_2_transactor->TDEST(trace_tdest2);
    mp_TRACE_IN_2_transactor->TID(trace_tid2);
    mp_TRACE_IN_2_transactor->TLAST(trace_tlast2);
    mp_TRACE_IN_2_transactor->TREADY(trace_tready2);
    mp_TRACE_IN_2_transactor->TVALID(trace_tvalid2);
    mp_TRACE_IN_2_transactor->CLK(trace_clk);
    mp_TRACE_IN_2_transactor->RST(trace_aresetn);

    // TRACE_IN_2' transactor sockets

    mp_impl->trace_socket2->bind(*(mp_TRACE_IN_2_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_3' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_3_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_3' transactor parameters
    xsc::common_cpp::properties TRACE_IN_3_transactor_param_props;
    TRACE_IN_3_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_3_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_3_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_3_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_3_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_3_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_3_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_3_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_3_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_3_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_3_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_3_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_3_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_3_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_3_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_3_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_3_transactor", TRACE_IN_3_transactor_param_props);

    // TRACE_IN_3' transactor ports

    mp_TRACE_IN_3_transactor->TDATA(trace_tdata3);
    mp_TRACE_IN_3_transactor->TDEST(trace_tdest3);
    mp_TRACE_IN_3_transactor->TID(trace_tid3);
    mp_TRACE_IN_3_transactor->TLAST(trace_tlast3);
    mp_TRACE_IN_3_transactor->TREADY(trace_tready3);
    mp_TRACE_IN_3_transactor->TVALID(trace_tvalid3);
    mp_TRACE_IN_3_transactor->CLK(trace_clk);
    mp_TRACE_IN_3_transactor->RST(trace_aresetn);

    // TRACE_IN_3' transactor sockets

    mp_impl->trace_socket3->bind(*(mp_TRACE_IN_3_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_4' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_4_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_4' transactor parameters
    xsc::common_cpp::properties TRACE_IN_4_transactor_param_props;
    TRACE_IN_4_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_4_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_4_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_4_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_4_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_4_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_4_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_4_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_4_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_4_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_4_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_4_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_4_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_4_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_4_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_4_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_4_transactor", TRACE_IN_4_transactor_param_props);

    // TRACE_IN_4' transactor ports

    mp_TRACE_IN_4_transactor->TDATA(trace_tdata4);
    mp_TRACE_IN_4_transactor->TDEST(trace_tdest4);
    mp_TRACE_IN_4_transactor->TID(trace_tid4);
    mp_TRACE_IN_4_transactor->TLAST(trace_tlast4);
    mp_TRACE_IN_4_transactor->TREADY(trace_tready4);
    mp_TRACE_IN_4_transactor->TVALID(trace_tvalid4);
    mp_TRACE_IN_4_transactor->CLK(trace_clk);
    mp_TRACE_IN_4_transactor->RST(trace_aresetn);

    // TRACE_IN_4' transactor sockets

    mp_impl->trace_socket4->bind(*(mp_TRACE_IN_4_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_5' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_5_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_5' transactor parameters
    xsc::common_cpp::properties TRACE_IN_5_transactor_param_props;
    TRACE_IN_5_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_5_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_5_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_5_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_5_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_5_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_5_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_5_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_5_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_5_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_5_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_5_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_5_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_5_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_5_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_5_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_5_transactor", TRACE_IN_5_transactor_param_props);

    // TRACE_IN_5' transactor ports

    mp_TRACE_IN_5_transactor->TDATA(trace_tdata5);
    mp_TRACE_IN_5_transactor->TDEST(trace_tdest5);
    mp_TRACE_IN_5_transactor->TID(trace_tid5);
    mp_TRACE_IN_5_transactor->TLAST(trace_tlast5);
    mp_TRACE_IN_5_transactor->TREADY(trace_tready5);
    mp_TRACE_IN_5_transactor->TVALID(trace_tvalid5);
    mp_TRACE_IN_5_transactor->CLK(trace_clk);
    mp_TRACE_IN_5_transactor->RST(trace_aresetn);

    // TRACE_IN_5' transactor sockets

    mp_impl->trace_socket5->bind(*(mp_TRACE_IN_5_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_6' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_6_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_6' transactor parameters
    xsc::common_cpp::properties TRACE_IN_6_transactor_param_props;
    TRACE_IN_6_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_6_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_6_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_6_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_6_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_6_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_6_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_6_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_6_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_6_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_6_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_6_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_6_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_6_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_6_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_6_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_6_transactor", TRACE_IN_6_transactor_param_props);

    // TRACE_IN_6' transactor ports

    mp_TRACE_IN_6_transactor->TDATA(trace_tdata6);
    mp_TRACE_IN_6_transactor->TDEST(trace_tdest6);
    mp_TRACE_IN_6_transactor->TID(trace_tid6);
    mp_TRACE_IN_6_transactor->TLAST(trace_tlast6);
    mp_TRACE_IN_6_transactor->TREADY(trace_tready6);
    mp_TRACE_IN_6_transactor->TVALID(trace_tvalid6);
    mp_TRACE_IN_6_transactor->CLK(trace_clk);
    mp_TRACE_IN_6_transactor->RST(trace_aresetn);

    // TRACE_IN_6' transactor sockets

    mp_impl->trace_socket6->bind(*(mp_TRACE_IN_6_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_7' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_7_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_7' transactor parameters
    xsc::common_cpp::properties TRACE_IN_7_transactor_param_props;
    TRACE_IN_7_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_7_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_7_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_7_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_7_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_7_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_7_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_7_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_7_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_7_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_7_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_7_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_7_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_7_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_7_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_7_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_7_transactor", TRACE_IN_7_transactor_param_props);

    // TRACE_IN_7' transactor ports

    mp_TRACE_IN_7_transactor->TDATA(trace_tdata7);
    mp_TRACE_IN_7_transactor->TDEST(trace_tdest7);
    mp_TRACE_IN_7_transactor->TID(trace_tid7);
    mp_TRACE_IN_7_transactor->TLAST(trace_tlast7);
    mp_TRACE_IN_7_transactor->TREADY(trace_tready7);
    mp_TRACE_IN_7_transactor->TVALID(trace_tvalid7);
    mp_TRACE_IN_7_transactor->CLK(trace_clk);
    mp_TRACE_IN_7_transactor->RST(trace_aresetn);

    // TRACE_IN_7' transactor sockets

    mp_impl->trace_socket7->bind(*(mp_TRACE_IN_7_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_8' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_8_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_8' transactor parameters
    xsc::common_cpp::properties TRACE_IN_8_transactor_param_props;
    TRACE_IN_8_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_8_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_8_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_8_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_8_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_8_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_8_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_8_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_8_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_8_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_8_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_8_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_8_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_8_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_8_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_8_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_8_transactor", TRACE_IN_8_transactor_param_props);

    // TRACE_IN_8' transactor ports

    mp_TRACE_IN_8_transactor->TDATA(trace_tdata8);
    mp_TRACE_IN_8_transactor->TDEST(trace_tdest8);
    mp_TRACE_IN_8_transactor->TID(trace_tid8);
    mp_TRACE_IN_8_transactor->TLAST(trace_tlast8);
    mp_TRACE_IN_8_transactor->TREADY(trace_tready8);
    mp_TRACE_IN_8_transactor->TVALID(trace_tvalid8);
    mp_TRACE_IN_8_transactor->CLK(trace_clk);
    mp_TRACE_IN_8_transactor->RST(trace_aresetn);

    // TRACE_IN_8' transactor sockets

    mp_impl->trace_socket8->bind(*(mp_TRACE_IN_8_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_9' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_9_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_9' transactor parameters
    xsc::common_cpp::properties TRACE_IN_9_transactor_param_props;
    TRACE_IN_9_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_9_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_9_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_9_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_9_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_9_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_9_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_9_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_9_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_9_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_9_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_9_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_9_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_9_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_9_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_9_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_9_transactor", TRACE_IN_9_transactor_param_props);

    // TRACE_IN_9' transactor ports

    mp_TRACE_IN_9_transactor->TDATA(trace_tdata9);
    mp_TRACE_IN_9_transactor->TDEST(trace_tdest9);
    mp_TRACE_IN_9_transactor->TID(trace_tid9);
    mp_TRACE_IN_9_transactor->TLAST(trace_tlast9);
    mp_TRACE_IN_9_transactor->TREADY(trace_tready9);
    mp_TRACE_IN_9_transactor->TVALID(trace_tvalid9);
    mp_TRACE_IN_9_transactor->CLK(trace_clk);
    mp_TRACE_IN_9_transactor->RST(trace_aresetn);

    // TRACE_IN_9' transactor sockets

    mp_impl->trace_socket9->bind(*(mp_TRACE_IN_9_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_10' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_10_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_10' transactor parameters
    xsc::common_cpp::properties TRACE_IN_10_transactor_param_props;
    TRACE_IN_10_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_10_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_10_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_10_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_10_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_10_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_10_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_10_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_10_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_10_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_10_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_10_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_10_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_10_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_10_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_10_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_10_transactor", TRACE_IN_10_transactor_param_props);

    // TRACE_IN_10' transactor ports

    mp_TRACE_IN_10_transactor->TDATA(trace_tdata10);
    mp_TRACE_IN_10_transactor->TDEST(trace_tdest10);
    mp_TRACE_IN_10_transactor->TID(trace_tid10);
    mp_TRACE_IN_10_transactor->TLAST(trace_tlast10);
    mp_TRACE_IN_10_transactor->TREADY(trace_tready10);
    mp_TRACE_IN_10_transactor->TVALID(trace_tvalid10);
    mp_TRACE_IN_10_transactor->CLK(trace_clk);
    mp_TRACE_IN_10_transactor->RST(trace_aresetn);

    // TRACE_IN_10' transactor sockets

    mp_impl->trace_socket10->bind(*(mp_TRACE_IN_10_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_11' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_11_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_11' transactor parameters
    xsc::common_cpp::properties TRACE_IN_11_transactor_param_props;
    TRACE_IN_11_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_11_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_11_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_11_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_11_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_11_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_11_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_11_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_11_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_11_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_11_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_11_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_11_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_11_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_11_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_11_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_11_transactor", TRACE_IN_11_transactor_param_props);

    // TRACE_IN_11' transactor ports

    mp_TRACE_IN_11_transactor->TDATA(trace_tdata11);
    mp_TRACE_IN_11_transactor->TDEST(trace_tdest11);
    mp_TRACE_IN_11_transactor->TID(trace_tid11);
    mp_TRACE_IN_11_transactor->TLAST(trace_tlast11);
    mp_TRACE_IN_11_transactor->TREADY(trace_tready11);
    mp_TRACE_IN_11_transactor->TVALID(trace_tvalid11);
    mp_TRACE_IN_11_transactor->CLK(trace_clk);
    mp_TRACE_IN_11_transactor->RST(trace_aresetn);

    // TRACE_IN_11' transactor sockets

    mp_impl->trace_socket11->bind(*(mp_TRACE_IN_11_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_12' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_12_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_12' transactor parameters
    xsc::common_cpp::properties TRACE_IN_12_transactor_param_props;
    TRACE_IN_12_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_12_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_12_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_12_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_12_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_12_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_12_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_12_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_12_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_12_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_12_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_12_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_12_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_12_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_12_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_12_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_12_transactor", TRACE_IN_12_transactor_param_props);

    // TRACE_IN_12' transactor ports

    mp_TRACE_IN_12_transactor->TDATA(trace_tdata12);
    mp_TRACE_IN_12_transactor->TDEST(trace_tdest12);
    mp_TRACE_IN_12_transactor->TID(trace_tid12);
    mp_TRACE_IN_12_transactor->TLAST(trace_tlast12);
    mp_TRACE_IN_12_transactor->TREADY(trace_tready12);
    mp_TRACE_IN_12_transactor->TVALID(trace_tvalid12);
    mp_TRACE_IN_12_transactor->CLK(trace_clk);
    mp_TRACE_IN_12_transactor->RST(trace_aresetn);

    // TRACE_IN_12' transactor sockets

    mp_impl->trace_socket12->bind(*(mp_TRACE_IN_12_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_13' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_13_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_13' transactor parameters
    xsc::common_cpp::properties TRACE_IN_13_transactor_param_props;
    TRACE_IN_13_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_13_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_13_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_13_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_13_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_13_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_13_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_13_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_13_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_13_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_13_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_13_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_13_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_13_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_13_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_13_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_13_transactor", TRACE_IN_13_transactor_param_props);

    // TRACE_IN_13' transactor ports

    mp_TRACE_IN_13_transactor->TDATA(trace_tdata13);
    mp_TRACE_IN_13_transactor->TDEST(trace_tdest13);
    mp_TRACE_IN_13_transactor->TID(trace_tid13);
    mp_TRACE_IN_13_transactor->TLAST(trace_tlast13);
    mp_TRACE_IN_13_transactor->TREADY(trace_tready13);
    mp_TRACE_IN_13_transactor->TVALID(trace_tvalid13);
    mp_TRACE_IN_13_transactor->CLK(trace_clk);
    mp_TRACE_IN_13_transactor->RST(trace_aresetn);

    // TRACE_IN_13' transactor sockets

    mp_impl->trace_socket13->bind(*(mp_TRACE_IN_13_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_14' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_14_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_14' transactor parameters
    xsc::common_cpp::properties TRACE_IN_14_transactor_param_props;
    TRACE_IN_14_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_14_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_14_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_14_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_14_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_14_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_14_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_14_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_14_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_14_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_14_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_14_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_14_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_14_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_14_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_14_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_14_transactor", TRACE_IN_14_transactor_param_props);

    // TRACE_IN_14' transactor ports

    mp_TRACE_IN_14_transactor->TDATA(trace_tdata14);
    mp_TRACE_IN_14_transactor->TDEST(trace_tdest14);
    mp_TRACE_IN_14_transactor->TID(trace_tid14);
    mp_TRACE_IN_14_transactor->TLAST(trace_tlast14);
    mp_TRACE_IN_14_transactor->TREADY(trace_tready14);
    mp_TRACE_IN_14_transactor->TVALID(trace_tvalid14);
    mp_TRACE_IN_14_transactor->CLK(trace_clk);
    mp_TRACE_IN_14_transactor->RST(trace_aresetn);

    // TRACE_IN_14' transactor sockets

    mp_impl->trace_socket14->bind(*(mp_TRACE_IN_14_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_15' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_15_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_15' transactor parameters
    xsc::common_cpp::properties TRACE_IN_15_transactor_param_props;
    TRACE_IN_15_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_15_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_15_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_15_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_15_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_15_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_15_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_15_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_15_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_15_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_15_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_15_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_15_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_15_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_15_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_15_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_15_transactor", TRACE_IN_15_transactor_param_props);

    // TRACE_IN_15' transactor ports

    mp_TRACE_IN_15_transactor->TDATA(trace_tdata15);
    mp_TRACE_IN_15_transactor->TDEST(trace_tdest15);
    mp_TRACE_IN_15_transactor->TID(trace_tid15);
    mp_TRACE_IN_15_transactor->TLAST(trace_tlast15);
    mp_TRACE_IN_15_transactor->TREADY(trace_tready15);
    mp_TRACE_IN_15_transactor->TVALID(trace_tvalid15);
    mp_TRACE_IN_15_transactor->CLK(trace_clk);
    mp_TRACE_IN_15_transactor->RST(trace_aresetn);

    // TRACE_IN_15' transactor sockets

    mp_impl->trace_socket15->bind(*(mp_TRACE_IN_15_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_16' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_16_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_16' transactor parameters
    xsc::common_cpp::properties TRACE_IN_16_transactor_param_props;
    TRACE_IN_16_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_16_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_16_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_16_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_16_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_16_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_16_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_16_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_16_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_16_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_16_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_16_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_16_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_16_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_16_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_16_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_16_transactor", TRACE_IN_16_transactor_param_props);

    // TRACE_IN_16' transactor ports

    mp_TRACE_IN_16_transactor->TDATA(trace_tdata16);
    mp_TRACE_IN_16_transactor->TDEST(trace_tdest16);
    mp_TRACE_IN_16_transactor->TID(trace_tid16);
    mp_TRACE_IN_16_transactor->TLAST(trace_tlast16);
    mp_TRACE_IN_16_transactor->TREADY(trace_tready16);
    mp_TRACE_IN_16_transactor->TVALID(trace_tvalid16);
    mp_TRACE_IN_16_transactor->CLK(trace_clk);
    mp_TRACE_IN_16_transactor->RST(trace_aresetn);

    // TRACE_IN_16' transactor sockets

    mp_impl->trace_socket16->bind(*(mp_TRACE_IN_16_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_17' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_17_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_17' transactor parameters
    xsc::common_cpp::properties TRACE_IN_17_transactor_param_props;
    TRACE_IN_17_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_17_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_17_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_17_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_17_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_17_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_17_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_17_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_17_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_17_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_17_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_17_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_17_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_17_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_17_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_17_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_17_transactor", TRACE_IN_17_transactor_param_props);

    // TRACE_IN_17' transactor ports

    mp_TRACE_IN_17_transactor->TDATA(trace_tdata17);
    mp_TRACE_IN_17_transactor->TDEST(trace_tdest17);
    mp_TRACE_IN_17_transactor->TID(trace_tid17);
    mp_TRACE_IN_17_transactor->TLAST(trace_tlast17);
    mp_TRACE_IN_17_transactor->TREADY(trace_tready17);
    mp_TRACE_IN_17_transactor->TVALID(trace_tvalid17);
    mp_TRACE_IN_17_transactor->CLK(trace_clk);
    mp_TRACE_IN_17_transactor->RST(trace_aresetn);

    // TRACE_IN_17' transactor sockets

    mp_impl->trace_socket17->bind(*(mp_TRACE_IN_17_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_18' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_18_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_18' transactor parameters
    xsc::common_cpp::properties TRACE_IN_18_transactor_param_props;
    TRACE_IN_18_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_18_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_18_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_18_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_18_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_18_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_18_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_18_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_18_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_18_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_18_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_18_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_18_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_18_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_18_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_18_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_18_transactor", TRACE_IN_18_transactor_param_props);

    // TRACE_IN_18' transactor ports

    mp_TRACE_IN_18_transactor->TDATA(trace_tdata18);
    mp_TRACE_IN_18_transactor->TDEST(trace_tdest18);
    mp_TRACE_IN_18_transactor->TID(trace_tid18);
    mp_TRACE_IN_18_transactor->TLAST(trace_tlast18);
    mp_TRACE_IN_18_transactor->TREADY(trace_tready18);
    mp_TRACE_IN_18_transactor->TVALID(trace_tvalid18);
    mp_TRACE_IN_18_transactor->CLK(trace_clk);
    mp_TRACE_IN_18_transactor->RST(trace_aresetn);

    // TRACE_IN_18' transactor sockets

    mp_impl->trace_socket18->bind(*(mp_TRACE_IN_18_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_19' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_19_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_19' transactor parameters
    xsc::common_cpp::properties TRACE_IN_19_transactor_param_props;
    TRACE_IN_19_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_19_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_19_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_19_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_19_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_19_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_19_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_19_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_19_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_19_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_19_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_19_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_19_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_19_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_19_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_19_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_19_transactor", TRACE_IN_19_transactor_param_props);

    // TRACE_IN_19' transactor ports

    mp_TRACE_IN_19_transactor->TDATA(trace_tdata19);
    mp_TRACE_IN_19_transactor->TDEST(trace_tdest19);
    mp_TRACE_IN_19_transactor->TID(trace_tid19);
    mp_TRACE_IN_19_transactor->TLAST(trace_tlast19);
    mp_TRACE_IN_19_transactor->TREADY(trace_tready19);
    mp_TRACE_IN_19_transactor->TVALID(trace_tvalid19);
    mp_TRACE_IN_19_transactor->CLK(trace_clk);
    mp_TRACE_IN_19_transactor->RST(trace_aresetn);

    // TRACE_IN_19' transactor sockets

    mp_impl->trace_socket19->bind(*(mp_TRACE_IN_19_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_20' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_20_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_20' transactor parameters
    xsc::common_cpp::properties TRACE_IN_20_transactor_param_props;
    TRACE_IN_20_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_20_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_20_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_20_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_20_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_20_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_20_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_20_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_20_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_20_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_20_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_20_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_20_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_20_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_20_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_20_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_20_transactor", TRACE_IN_20_transactor_param_props);

    // TRACE_IN_20' transactor ports

    mp_TRACE_IN_20_transactor->TDATA(trace_tdata20);
    mp_TRACE_IN_20_transactor->TDEST(trace_tdest20);
    mp_TRACE_IN_20_transactor->TID(trace_tid20);
    mp_TRACE_IN_20_transactor->TLAST(trace_tlast20);
    mp_TRACE_IN_20_transactor->TREADY(trace_tready20);
    mp_TRACE_IN_20_transactor->TVALID(trace_tvalid20);
    mp_TRACE_IN_20_transactor->CLK(trace_clk);
    mp_TRACE_IN_20_transactor->RST(trace_aresetn);

    // TRACE_IN_20' transactor sockets

    mp_impl->trace_socket20->bind(*(mp_TRACE_IN_20_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_21' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_21_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_21' transactor parameters
    xsc::common_cpp::properties TRACE_IN_21_transactor_param_props;
    TRACE_IN_21_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_21_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_21_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_21_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_21_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_21_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_21_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_21_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_21_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_21_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_21_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_21_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_21_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_21_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_21_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_21_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_21_transactor", TRACE_IN_21_transactor_param_props);

    // TRACE_IN_21' transactor ports

    mp_TRACE_IN_21_transactor->TDATA(trace_tdata21);
    mp_TRACE_IN_21_transactor->TDEST(trace_tdest21);
    mp_TRACE_IN_21_transactor->TID(trace_tid21);
    mp_TRACE_IN_21_transactor->TLAST(trace_tlast21);
    mp_TRACE_IN_21_transactor->TREADY(trace_tready21);
    mp_TRACE_IN_21_transactor->TVALID(trace_tvalid21);
    mp_TRACE_IN_21_transactor->CLK(trace_clk);
    mp_TRACE_IN_21_transactor->RST(trace_aresetn);

    // TRACE_IN_21' transactor sockets

    mp_impl->trace_socket21->bind(*(mp_TRACE_IN_21_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_22' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_22_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_22' transactor parameters
    xsc::common_cpp::properties TRACE_IN_22_transactor_param_props;
    TRACE_IN_22_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_22_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_22_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_22_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_22_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_22_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_22_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_22_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_22_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_22_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_22_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_22_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_22_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_22_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_22_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_22_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_22_transactor", TRACE_IN_22_transactor_param_props);

    // TRACE_IN_22' transactor ports

    mp_TRACE_IN_22_transactor->TDATA(trace_tdata22);
    mp_TRACE_IN_22_transactor->TDEST(trace_tdest22);
    mp_TRACE_IN_22_transactor->TID(trace_tid22);
    mp_TRACE_IN_22_transactor->TLAST(trace_tlast22);
    mp_TRACE_IN_22_transactor->TREADY(trace_tready22);
    mp_TRACE_IN_22_transactor->TVALID(trace_tvalid22);
    mp_TRACE_IN_22_transactor->CLK(trace_clk);
    mp_TRACE_IN_22_transactor->RST(trace_aresetn);

    // TRACE_IN_22' transactor sockets

    mp_impl->trace_socket22->bind(*(mp_TRACE_IN_22_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_23' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_23_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_23' transactor parameters
    xsc::common_cpp::properties TRACE_IN_23_transactor_param_props;
    TRACE_IN_23_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_23_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_23_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_23_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_23_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_23_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_23_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_23_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_23_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_23_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_23_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_23_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_23_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_23_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_23_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_23_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_23_transactor", TRACE_IN_23_transactor_param_props);

    // TRACE_IN_23' transactor ports

    mp_TRACE_IN_23_transactor->TDATA(trace_tdata23);
    mp_TRACE_IN_23_transactor->TDEST(trace_tdest23);
    mp_TRACE_IN_23_transactor->TID(trace_tid23);
    mp_TRACE_IN_23_transactor->TLAST(trace_tlast23);
    mp_TRACE_IN_23_transactor->TREADY(trace_tready23);
    mp_TRACE_IN_23_transactor->TVALID(trace_tvalid23);
    mp_TRACE_IN_23_transactor->CLK(trace_clk);
    mp_TRACE_IN_23_transactor->RST(trace_aresetn);

    // TRACE_IN_23' transactor sockets

    mp_impl->trace_socket23->bind(*(mp_TRACE_IN_23_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_24' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_24_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_24' transactor parameters
    xsc::common_cpp::properties TRACE_IN_24_transactor_param_props;
    TRACE_IN_24_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_24_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_24_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_24_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_24_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_24_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_24_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_24_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_24_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_24_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_24_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_24_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_24_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_24_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_24_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_24_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_24_transactor", TRACE_IN_24_transactor_param_props);

    // TRACE_IN_24' transactor ports

    mp_TRACE_IN_24_transactor->TDATA(trace_tdata24);
    mp_TRACE_IN_24_transactor->TDEST(trace_tdest24);
    mp_TRACE_IN_24_transactor->TID(trace_tid24);
    mp_TRACE_IN_24_transactor->TLAST(trace_tlast24);
    mp_TRACE_IN_24_transactor->TREADY(trace_tready24);
    mp_TRACE_IN_24_transactor->TVALID(trace_tvalid24);
    mp_TRACE_IN_24_transactor->CLK(trace_clk);
    mp_TRACE_IN_24_transactor->RST(trace_aresetn);

    // TRACE_IN_24' transactor sockets

    mp_impl->trace_socket24->bind(*(mp_TRACE_IN_24_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_25' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_25_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_25' transactor parameters
    xsc::common_cpp::properties TRACE_IN_25_transactor_param_props;
    TRACE_IN_25_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_25_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_25_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_25_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_25_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_25_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_25_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_25_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_25_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_25_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_25_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_25_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_25_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_25_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_25_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_25_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_25_transactor", TRACE_IN_25_transactor_param_props);

    // TRACE_IN_25' transactor ports

    mp_TRACE_IN_25_transactor->TDATA(trace_tdata25);
    mp_TRACE_IN_25_transactor->TDEST(trace_tdest25);
    mp_TRACE_IN_25_transactor->TID(trace_tid25);
    mp_TRACE_IN_25_transactor->TLAST(trace_tlast25);
    mp_TRACE_IN_25_transactor->TREADY(trace_tready25);
    mp_TRACE_IN_25_transactor->TVALID(trace_tvalid25);
    mp_TRACE_IN_25_transactor->CLK(trace_clk);
    mp_TRACE_IN_25_transactor->RST(trace_aresetn);

    // TRACE_IN_25' transactor sockets

    mp_impl->trace_socket25->bind(*(mp_TRACE_IN_25_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_26' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_26_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_26' transactor parameters
    xsc::common_cpp::properties TRACE_IN_26_transactor_param_props;
    TRACE_IN_26_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_26_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_26_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_26_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_26_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_26_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_26_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_26_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_26_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_26_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_26_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_26_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_26_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_26_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_26_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_26_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_26_transactor", TRACE_IN_26_transactor_param_props);

    // TRACE_IN_26' transactor ports

    mp_TRACE_IN_26_transactor->TDATA(trace_tdata26);
    mp_TRACE_IN_26_transactor->TDEST(trace_tdest26);
    mp_TRACE_IN_26_transactor->TID(trace_tid26);
    mp_TRACE_IN_26_transactor->TLAST(trace_tlast26);
    mp_TRACE_IN_26_transactor->TREADY(trace_tready26);
    mp_TRACE_IN_26_transactor->TVALID(trace_tvalid26);
    mp_TRACE_IN_26_transactor->CLK(trace_clk);
    mp_TRACE_IN_26_transactor->RST(trace_aresetn);

    // TRACE_IN_26' transactor sockets

    mp_impl->trace_socket26->bind(*(mp_TRACE_IN_26_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_27' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_27_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_27' transactor parameters
    xsc::common_cpp::properties TRACE_IN_27_transactor_param_props;
    TRACE_IN_27_transactor_param_props.addLong("TDATA_NUM_BYTES", "1");
    TRACE_IN_27_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_27_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_27_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_27_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_27_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_27_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_27_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_27_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_27_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_27_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_27_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_27_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_27_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_27_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_27_transactor = new xtlm::xaxis_pin2xtlm_t<1,1,8,8,1,1>("TRACE_IN_27_transactor", TRACE_IN_27_transactor_param_props);

    // TRACE_IN_27' transactor ports

    mp_TRACE_IN_27_transactor->TDATA(trace_tdata27);
    mp_TRACE_IN_27_transactor->TDEST(trace_tdest27);
    mp_TRACE_IN_27_transactor->TID(trace_tid27);
    mp_TRACE_IN_27_transactor->TLAST(trace_tlast27);
    mp_TRACE_IN_27_transactor->TREADY(trace_tready27);
    mp_TRACE_IN_27_transactor->TVALID(trace_tvalid27);
    mp_TRACE_IN_27_transactor->CLK(trace_clk);
    mp_TRACE_IN_27_transactor->RST(trace_aresetn);

    // TRACE_IN_27' transactor sockets

    mp_impl->trace_socket27->bind(*(mp_TRACE_IN_27_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_28' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_28_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_28' transactor parameters
    xsc::common_cpp::properties TRACE_IN_28_transactor_param_props;
    TRACE_IN_28_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_28_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_28_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_28_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_28_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_28_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_28_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_28_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_28_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_28_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_28_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_28_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_28_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_28_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_28_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_28_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_28_transactor", TRACE_IN_28_transactor_param_props);

    // TRACE_IN_28' transactor ports

    mp_TRACE_IN_28_transactor->TDATA(trace_tdata28);
    mp_TRACE_IN_28_transactor->TDEST(trace_tdest28);
    mp_TRACE_IN_28_transactor->TID(trace_tid28);
    mp_TRACE_IN_28_transactor->TLAST(trace_tlast28);
    mp_TRACE_IN_28_transactor->TREADY(trace_tready28);
    mp_TRACE_IN_28_transactor->TVALID(trace_tvalid28);
    mp_TRACE_IN_28_transactor->CLK(trace_clk);
    mp_TRACE_IN_28_transactor->RST(trace_aresetn);

    // TRACE_IN_28' transactor sockets

    mp_impl->trace_socket28->bind(*(mp_TRACE_IN_28_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_29' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_29_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_29' transactor parameters
    xsc::common_cpp::properties TRACE_IN_29_transactor_param_props;
    TRACE_IN_29_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_29_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_29_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_29_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_29_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_29_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_29_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_29_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_29_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_29_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_29_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_29_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_29_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_29_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_29_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_29_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_29_transactor", TRACE_IN_29_transactor_param_props);

    // TRACE_IN_29' transactor ports

    mp_TRACE_IN_29_transactor->TDATA(trace_tdata29);
    mp_TRACE_IN_29_transactor->TDEST(trace_tdest29);
    mp_TRACE_IN_29_transactor->TID(trace_tid29);
    mp_TRACE_IN_29_transactor->TLAST(trace_tlast29);
    mp_TRACE_IN_29_transactor->TREADY(trace_tready29);
    mp_TRACE_IN_29_transactor->TVALID(trace_tvalid29);
    mp_TRACE_IN_29_transactor->CLK(trace_clk);
    mp_TRACE_IN_29_transactor->RST(trace_aresetn);

    // TRACE_IN_29' transactor sockets

    mp_impl->trace_socket29->bind(*(mp_TRACE_IN_29_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_30' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_30_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_30' transactor parameters
    xsc::common_cpp::properties TRACE_IN_30_transactor_param_props;
    TRACE_IN_30_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_30_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_30_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_30_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_30_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_30_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_30_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_30_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_30_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_30_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_30_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_30_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_30_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_30_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_30_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_30_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_30_transactor", TRACE_IN_30_transactor_param_props);

    // TRACE_IN_30' transactor ports

    mp_TRACE_IN_30_transactor->TDATA(trace_tdata30);
    mp_TRACE_IN_30_transactor->TDEST(trace_tdest30);
    mp_TRACE_IN_30_transactor->TID(trace_tid30);
    mp_TRACE_IN_30_transactor->TLAST(trace_tlast30);
    mp_TRACE_IN_30_transactor->TREADY(trace_tready30);
    mp_TRACE_IN_30_transactor->TVALID(trace_tvalid30);
    mp_TRACE_IN_30_transactor->CLK(trace_clk);
    mp_TRACE_IN_30_transactor->RST(trace_aresetn);

    // TRACE_IN_30' transactor sockets

    mp_impl->trace_socket30->bind(*(mp_TRACE_IN_30_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_31' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_31_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_31' transactor parameters
    xsc::common_cpp::properties TRACE_IN_31_transactor_param_props;
    TRACE_IN_31_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_31_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_31_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_31_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_31_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_31_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_31_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_31_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_31_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_31_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_31_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_31_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_31_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_31_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_31_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_31_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_31_transactor", TRACE_IN_31_transactor_param_props);

    // TRACE_IN_31' transactor ports

    mp_TRACE_IN_31_transactor->TDATA(trace_tdata31);
    mp_TRACE_IN_31_transactor->TDEST(trace_tdest31);
    mp_TRACE_IN_31_transactor->TID(trace_tid31);
    mp_TRACE_IN_31_transactor->TLAST(trace_tlast31);
    mp_TRACE_IN_31_transactor->TREADY(trace_tready31);
    mp_TRACE_IN_31_transactor->TVALID(trace_tvalid31);
    mp_TRACE_IN_31_transactor->CLK(trace_clk);
    mp_TRACE_IN_31_transactor->RST(trace_aresetn);

    // TRACE_IN_31' transactor sockets

    mp_impl->trace_socket31->bind(*(mp_TRACE_IN_31_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_32' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_32_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_32' transactor parameters
    xsc::common_cpp::properties TRACE_IN_32_transactor_param_props;
    TRACE_IN_32_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_32_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_32_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_32_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_32_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_32_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_32_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_32_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_32_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_32_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_32_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_32_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_32_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_32_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_32_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_32_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_32_transactor", TRACE_IN_32_transactor_param_props);

    // TRACE_IN_32' transactor ports

    mp_TRACE_IN_32_transactor->TDATA(trace_tdata32);
    mp_TRACE_IN_32_transactor->TDEST(trace_tdest32);
    mp_TRACE_IN_32_transactor->TID(trace_tid32);
    mp_TRACE_IN_32_transactor->TLAST(trace_tlast32);
    mp_TRACE_IN_32_transactor->TREADY(trace_tready32);
    mp_TRACE_IN_32_transactor->TVALID(trace_tvalid32);
    mp_TRACE_IN_32_transactor->CLK(trace_clk);
    mp_TRACE_IN_32_transactor->RST(trace_aresetn);

    // TRACE_IN_32' transactor sockets

    mp_impl->trace_socket32->bind(*(mp_TRACE_IN_32_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_33' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_33_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_33' transactor parameters
    xsc::common_cpp::properties TRACE_IN_33_transactor_param_props;
    TRACE_IN_33_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_33_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_33_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_33_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_33_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_33_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_33_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_33_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_33_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_33_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_33_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_33_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_33_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_33_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_33_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_33_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_33_transactor", TRACE_IN_33_transactor_param_props);

    // TRACE_IN_33' transactor ports

    mp_TRACE_IN_33_transactor->TDATA(trace_tdata33);
    mp_TRACE_IN_33_transactor->TDEST(trace_tdest33);
    mp_TRACE_IN_33_transactor->TID(trace_tid33);
    mp_TRACE_IN_33_transactor->TLAST(trace_tlast33);
    mp_TRACE_IN_33_transactor->TREADY(trace_tready33);
    mp_TRACE_IN_33_transactor->TVALID(trace_tvalid33);
    mp_TRACE_IN_33_transactor->CLK(trace_clk);
    mp_TRACE_IN_33_transactor->RST(trace_aresetn);

    // TRACE_IN_33' transactor sockets

    mp_impl->trace_socket33->bind(*(mp_TRACE_IN_33_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_34' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_34_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_34' transactor parameters
    xsc::common_cpp::properties TRACE_IN_34_transactor_param_props;
    TRACE_IN_34_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_34_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_34_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_34_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_34_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_34_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_34_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_34_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_34_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_34_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_34_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_34_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_34_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_34_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_34_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_34_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_34_transactor", TRACE_IN_34_transactor_param_props);

    // TRACE_IN_34' transactor ports

    mp_TRACE_IN_34_transactor->TDATA(trace_tdata34);
    mp_TRACE_IN_34_transactor->TDEST(trace_tdest34);
    mp_TRACE_IN_34_transactor->TID(trace_tid34);
    mp_TRACE_IN_34_transactor->TLAST(trace_tlast34);
    mp_TRACE_IN_34_transactor->TREADY(trace_tready34);
    mp_TRACE_IN_34_transactor->TVALID(trace_tvalid34);
    mp_TRACE_IN_34_transactor->CLK(trace_clk);
    mp_TRACE_IN_34_transactor->RST(trace_aresetn);

    // TRACE_IN_34' transactor sockets

    mp_impl->trace_socket34->bind(*(mp_TRACE_IN_34_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_35' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_35_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_35' transactor parameters
    xsc::common_cpp::properties TRACE_IN_35_transactor_param_props;
    TRACE_IN_35_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_35_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_35_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_35_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_35_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_35_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_35_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_35_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_35_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_35_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_35_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_35_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_35_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_35_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_35_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_35_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_35_transactor", TRACE_IN_35_transactor_param_props);

    // TRACE_IN_35' transactor ports

    mp_TRACE_IN_35_transactor->TDATA(trace_tdata35);
    mp_TRACE_IN_35_transactor->TDEST(trace_tdest35);
    mp_TRACE_IN_35_transactor->TID(trace_tid35);
    mp_TRACE_IN_35_transactor->TLAST(trace_tlast35);
    mp_TRACE_IN_35_transactor->TREADY(trace_tready35);
    mp_TRACE_IN_35_transactor->TVALID(trace_tvalid35);
    mp_TRACE_IN_35_transactor->CLK(trace_clk);
    mp_TRACE_IN_35_transactor->RST(trace_aresetn);

    // TRACE_IN_35' transactor sockets

    mp_impl->trace_socket35->bind(*(mp_TRACE_IN_35_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_36' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_36_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_36' transactor parameters
    xsc::common_cpp::properties TRACE_IN_36_transactor_param_props;
    TRACE_IN_36_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_36_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_36_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_36_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_36_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_36_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_36_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_36_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_36_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_36_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_36_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_36_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_36_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_36_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_36_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_36_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_36_transactor", TRACE_IN_36_transactor_param_props);

    // TRACE_IN_36' transactor ports

    mp_TRACE_IN_36_transactor->TDATA(trace_tdata36);
    mp_TRACE_IN_36_transactor->TDEST(trace_tdest36);
    mp_TRACE_IN_36_transactor->TID(trace_tid36);
    mp_TRACE_IN_36_transactor->TLAST(trace_tlast36);
    mp_TRACE_IN_36_transactor->TREADY(trace_tready36);
    mp_TRACE_IN_36_transactor->TVALID(trace_tvalid36);
    mp_TRACE_IN_36_transactor->CLK(trace_clk);
    mp_TRACE_IN_36_transactor->RST(trace_aresetn);

    // TRACE_IN_36' transactor sockets

    mp_impl->trace_socket36->bind(*(mp_TRACE_IN_36_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_37' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_37_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_37' transactor parameters
    xsc::common_cpp::properties TRACE_IN_37_transactor_param_props;
    TRACE_IN_37_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_37_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_37_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_37_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_37_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_37_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_37_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_37_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_37_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_37_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_37_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_37_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_37_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_37_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_37_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_37_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_37_transactor", TRACE_IN_37_transactor_param_props);

    // TRACE_IN_37' transactor ports

    mp_TRACE_IN_37_transactor->TDATA(trace_tdata37);
    mp_TRACE_IN_37_transactor->TDEST(trace_tdest37);
    mp_TRACE_IN_37_transactor->TID(trace_tid37);
    mp_TRACE_IN_37_transactor->TLAST(trace_tlast37);
    mp_TRACE_IN_37_transactor->TREADY(trace_tready37);
    mp_TRACE_IN_37_transactor->TVALID(trace_tvalid37);
    mp_TRACE_IN_37_transactor->CLK(trace_clk);
    mp_TRACE_IN_37_transactor->RST(trace_aresetn);

    // TRACE_IN_37' transactor sockets

    mp_impl->trace_socket37->bind(*(mp_TRACE_IN_37_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_38' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_38_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_38' transactor parameters
    xsc::common_cpp::properties TRACE_IN_38_transactor_param_props;
    TRACE_IN_38_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_38_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_38_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_38_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_38_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_38_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_38_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_38_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_38_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_38_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_38_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_38_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_38_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_38_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_38_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_38_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_38_transactor", TRACE_IN_38_transactor_param_props);

    // TRACE_IN_38' transactor ports

    mp_TRACE_IN_38_transactor->TDATA(trace_tdata38);
    mp_TRACE_IN_38_transactor->TDEST(trace_tdest38);
    mp_TRACE_IN_38_transactor->TID(trace_tid38);
    mp_TRACE_IN_38_transactor->TLAST(trace_tlast38);
    mp_TRACE_IN_38_transactor->TREADY(trace_tready38);
    mp_TRACE_IN_38_transactor->TVALID(trace_tvalid38);
    mp_TRACE_IN_38_transactor->CLK(trace_clk);
    mp_TRACE_IN_38_transactor->RST(trace_aresetn);

    // TRACE_IN_38' transactor sockets

    mp_impl->trace_socket38->bind(*(mp_TRACE_IN_38_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_39' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_39_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_39' transactor parameters
    xsc::common_cpp::properties TRACE_IN_39_transactor_param_props;
    TRACE_IN_39_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_39_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_39_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_39_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_39_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_39_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_39_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_39_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_39_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_39_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_39_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_39_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_39_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_39_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_39_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_39_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_39_transactor", TRACE_IN_39_transactor_param_props);

    // TRACE_IN_39' transactor ports

    mp_TRACE_IN_39_transactor->TDATA(trace_tdata39);
    mp_TRACE_IN_39_transactor->TDEST(trace_tdest39);
    mp_TRACE_IN_39_transactor->TID(trace_tid39);
    mp_TRACE_IN_39_transactor->TLAST(trace_tlast39);
    mp_TRACE_IN_39_transactor->TREADY(trace_tready39);
    mp_TRACE_IN_39_transactor->TVALID(trace_tvalid39);
    mp_TRACE_IN_39_transactor->CLK(trace_clk);
    mp_TRACE_IN_39_transactor->RST(trace_aresetn);

    // TRACE_IN_39' transactor sockets

    mp_impl->trace_socket39->bind(*(mp_TRACE_IN_39_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_40' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_40_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_40' transactor parameters
    xsc::common_cpp::properties TRACE_IN_40_transactor_param_props;
    TRACE_IN_40_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_40_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_40_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_40_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_40_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_40_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_40_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_40_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_40_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_40_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_40_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_40_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_40_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_40_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_40_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_40_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_40_transactor", TRACE_IN_40_transactor_param_props);

    // TRACE_IN_40' transactor ports

    mp_TRACE_IN_40_transactor->TDATA(trace_tdata40);
    mp_TRACE_IN_40_transactor->TDEST(trace_tdest40);
    mp_TRACE_IN_40_transactor->TID(trace_tid40);
    mp_TRACE_IN_40_transactor->TLAST(trace_tlast40);
    mp_TRACE_IN_40_transactor->TREADY(trace_tready40);
    mp_TRACE_IN_40_transactor->TVALID(trace_tvalid40);
    mp_TRACE_IN_40_transactor->CLK(trace_clk);
    mp_TRACE_IN_40_transactor->RST(trace_aresetn);

    // TRACE_IN_40' transactor sockets

    mp_impl->trace_socket40->bind(*(mp_TRACE_IN_40_transactor->socket));
  }
  else
  {
  }

}

#endif // RIVIERA




#ifdef VCSSYSTEMC
pfm_dynamic_dpa_hub_0::pfm_dynamic_dpa_hub_0(const sc_core::sc_module_name& nm) : pfm_dynamic_dpa_hub_0_sc(nm),  s_aximm_clk("s_aximm_clk"), s_aximm_aresetn("s_aximm_aresetn"), trace_clk("trace_clk"), trace_aresetn("trace_aresetn"), axilite_clk("axilite_clk"), axilite_aresetn("axilite_aresetn"), trace_tready0("trace_tready0"), trace_tdata0("trace_tdata0"), trace_tlast0("trace_tlast0"), trace_tid0("trace_tid0"), trace_tdest0("trace_tdest0"), trace_tvalid0("trace_tvalid0"), trace_tready1("trace_tready1"), trace_tdata1("trace_tdata1"), trace_tlast1("trace_tlast1"), trace_tid1("trace_tid1"), trace_tdest1("trace_tdest1"), trace_tvalid1("trace_tvalid1"), trace_tdata2("trace_tdata2"), trace_tready2("trace_tready2"), trace_tlast2("trace_tlast2"), trace_tid2("trace_tid2"), trace_tdest2("trace_tdest2"), trace_tvalid2("trace_tvalid2"), trace_tdata3("trace_tdata3"), trace_tready3("trace_tready3"), trace_tlast3("trace_tlast3"), trace_tid3("trace_tid3"), trace_tdest3("trace_tdest3"), trace_tvalid3("trace_tvalid3"), trace_tdata4("trace_tdata4"), trace_tready4("trace_tready4"), trace_tlast4("trace_tlast4"), trace_tid4("trace_tid4"), trace_tdest4("trace_tdest4"), trace_tvalid4("trace_tvalid4"), trace_tdata5("trace_tdata5"), trace_tready5("trace_tready5"), trace_tlast5("trace_tlast5"), trace_tid5("trace_tid5"), trace_tdest5("trace_tdest5"), trace_tvalid5("trace_tvalid5"), trace_tdata6("trace_tdata6"), trace_tready6("trace_tready6"), trace_tlast6("trace_tlast6"), trace_tid6("trace_tid6"), trace_tdest6("trace_tdest6"), trace_tvalid6("trace_tvalid6"), trace_tdata7("trace_tdata7"), trace_tready7("trace_tready7"), trace_tlast7("trace_tlast7"), trace_tid7("trace_tid7"), trace_tdest7("trace_tdest7"), trace_tvalid7("trace_tvalid7"), trace_tdata8("trace_tdata8"), trace_tready8("trace_tready8"), trace_tlast8("trace_tlast8"), trace_tid8("trace_tid8"), trace_tdest8("trace_tdest8"), trace_tvalid8("trace_tvalid8"), trace_tdata9("trace_tdata9"), trace_tready9("trace_tready9"), trace_tlast9("trace_tlast9"), trace_tid9("trace_tid9"), trace_tdest9("trace_tdest9"), trace_tvalid9("trace_tvalid9"), trace_tdata10("trace_tdata10"), trace_tready10("trace_tready10"), trace_tlast10("trace_tlast10"), trace_tid10("trace_tid10"), trace_tdest10("trace_tdest10"), trace_tvalid10("trace_tvalid10"), trace_tdata11("trace_tdata11"), trace_tready11("trace_tready11"), trace_tlast11("trace_tlast11"), trace_tid11("trace_tid11"), trace_tdest11("trace_tdest11"), trace_tvalid11("trace_tvalid11"), trace_tdata12("trace_tdata12"), trace_tready12("trace_tready12"), trace_tlast12("trace_tlast12"), trace_tid12("trace_tid12"), trace_tdest12("trace_tdest12"), trace_tvalid12("trace_tvalid12"), trace_tdata13("trace_tdata13"), trace_tready13("trace_tready13"), trace_tlast13("trace_tlast13"), trace_tid13("trace_tid13"), trace_tdest13("trace_tdest13"), trace_tvalid13("trace_tvalid13"), trace_tdata14("trace_tdata14"), trace_tready14("trace_tready14"), trace_tlast14("trace_tlast14"), trace_tid14("trace_tid14"), trace_tdest14("trace_tdest14"), trace_tvalid14("trace_tvalid14"), trace_tdata15("trace_tdata15"), trace_tready15("trace_tready15"), trace_tlast15("trace_tlast15"), trace_tid15("trace_tid15"), trace_tdest15("trace_tdest15"), trace_tvalid15("trace_tvalid15"), trace_tdata16("trace_tdata16"), trace_tready16("trace_tready16"), trace_tlast16("trace_tlast16"), trace_tid16("trace_tid16"), trace_tdest16("trace_tdest16"), trace_tvalid16("trace_tvalid16"), trace_tdata17("trace_tdata17"), trace_tready17("trace_tready17"), trace_tlast17("trace_tlast17"), trace_tid17("trace_tid17"), trace_tdest17("trace_tdest17"), trace_tvalid17("trace_tvalid17"), trace_tdata18("trace_tdata18"), trace_tready18("trace_tready18"), trace_tlast18("trace_tlast18"), trace_tid18("trace_tid18"), trace_tdest18("trace_tdest18"), trace_tvalid18("trace_tvalid18"), trace_tdata19("trace_tdata19"), trace_tready19("trace_tready19"), trace_tlast19("trace_tlast19"), trace_tid19("trace_tid19"), trace_tdest19("trace_tdest19"), trace_tvalid19("trace_tvalid19"), trace_tdata20("trace_tdata20"), trace_tready20("trace_tready20"), trace_tlast20("trace_tlast20"), trace_tid20("trace_tid20"), trace_tdest20("trace_tdest20"), trace_tvalid20("trace_tvalid20"), trace_tdata21("trace_tdata21"), trace_tready21("trace_tready21"), trace_tlast21("trace_tlast21"), trace_tid21("trace_tid21"), trace_tdest21("trace_tdest21"), trace_tvalid21("trace_tvalid21"), trace_tdata22("trace_tdata22"), trace_tready22("trace_tready22"), trace_tlast22("trace_tlast22"), trace_tid22("trace_tid22"), trace_tdest22("trace_tdest22"), trace_tvalid22("trace_tvalid22"), trace_tdata23("trace_tdata23"), trace_tready23("trace_tready23"), trace_tlast23("trace_tlast23"), trace_tid23("trace_tid23"), trace_tdest23("trace_tdest23"), trace_tvalid23("trace_tvalid23"), trace_tdata24("trace_tdata24"), trace_tready24("trace_tready24"), trace_tlast24("trace_tlast24"), trace_tid24("trace_tid24"), trace_tdest24("trace_tdest24"), trace_tvalid24("trace_tvalid24"), trace_tdata25("trace_tdata25"), trace_tready25("trace_tready25"), trace_tlast25("trace_tlast25"), trace_tid25("trace_tid25"), trace_tdest25("trace_tdest25"), trace_tvalid25("trace_tvalid25"), trace_tdata26("trace_tdata26"), trace_tready26("trace_tready26"), trace_tlast26("trace_tlast26"), trace_tid26("trace_tid26"), trace_tdest26("trace_tdest26"), trace_tvalid26("trace_tvalid26"), trace_tdata27("trace_tdata27"), trace_tready27("trace_tready27"), trace_tlast27("trace_tlast27"), trace_tid27("trace_tid27"), trace_tdest27("trace_tdest27"), trace_tvalid27("trace_tvalid27"), trace_tdata28("trace_tdata28"), trace_tready28("trace_tready28"), trace_tlast28("trace_tlast28"), trace_tid28("trace_tid28"), trace_tdest28("trace_tdest28"), trace_tvalid28("trace_tvalid28"), trace_tdata29("trace_tdata29"), trace_tready29("trace_tready29"), trace_tlast29("trace_tlast29"), trace_tid29("trace_tid29"), trace_tdest29("trace_tdest29"), trace_tvalid29("trace_tvalid29"), trace_tdata30("trace_tdata30"), trace_tready30("trace_tready30"), trace_tlast30("trace_tlast30"), trace_tid30("trace_tid30"), trace_tdest30("trace_tdest30"), trace_tvalid30("trace_tvalid30"), trace_tdata31("trace_tdata31"), trace_tready31("trace_tready31"), trace_tlast31("trace_tlast31"), trace_tid31("trace_tid31"), trace_tdest31("trace_tdest31"), trace_tvalid31("trace_tvalid31"), trace_tdata32("trace_tdata32"), trace_tready32("trace_tready32"), trace_tlast32("trace_tlast32"), trace_tid32("trace_tid32"), trace_tdest32("trace_tdest32"), trace_tvalid32("trace_tvalid32"), trace_tdata33("trace_tdata33"), trace_tready33("trace_tready33"), trace_tlast33("trace_tlast33"), trace_tid33("trace_tid33"), trace_tdest33("trace_tdest33"), trace_tvalid33("trace_tvalid33"), trace_tdata34("trace_tdata34"), trace_tready34("trace_tready34"), trace_tlast34("trace_tlast34"), trace_tid34("trace_tid34"), trace_tdest34("trace_tdest34"), trace_tvalid34("trace_tvalid34"), trace_tdata35("trace_tdata35"), trace_tready35("trace_tready35"), trace_tlast35("trace_tlast35"), trace_tid35("trace_tid35"), trace_tdest35("trace_tdest35"), trace_tvalid35("trace_tvalid35"), trace_tdata36("trace_tdata36"), trace_tready36("trace_tready36"), trace_tlast36("trace_tlast36"), trace_tid36("trace_tid36"), trace_tdest36("trace_tdest36"), trace_tvalid36("trace_tvalid36"), trace_tdata37("trace_tdata37"), trace_tready37("trace_tready37"), trace_tlast37("trace_tlast37"), trace_tid37("trace_tid37"), trace_tdest37("trace_tdest37"), trace_tvalid37("trace_tvalid37"), trace_tdata38("trace_tdata38"), trace_tready38("trace_tready38"), trace_tlast38("trace_tlast38"), trace_tid38("trace_tid38"), trace_tdest38("trace_tdest38"), trace_tvalid38("trace_tvalid38"), trace_tdata39("trace_tdata39"), trace_tready39("trace_tready39"), trace_tlast39("trace_tlast39"), trace_tid39("trace_tid39"), trace_tdest39("trace_tdest39"), trace_tvalid39("trace_tvalid39"), trace_tdata40("trace_tdata40"), trace_tready40("trace_tready40"), trace_tlast40("trace_tlast40"), trace_tid40("trace_tid40"), trace_tdest40("trace_tdest40"), trace_tvalid40("trace_tvalid40"), s_axi_awaddr("s_axi_awaddr"), s_axi_awvalid("s_axi_awvalid"), s_axi_awready("s_axi_awready"), s_axi_wdata("s_axi_wdata"), s_axi_wstrb("s_axi_wstrb"), s_axi_wvalid("s_axi_wvalid"), s_axi_wready("s_axi_wready"), s_axi_bvalid("s_axi_bvalid"), s_axi_bready("s_axi_bready"), s_axi_bresp("s_axi_bresp"), s_axi_araddr("s_axi_araddr"), s_axi_arvalid("s_axi_arvalid"), s_axi_arready("s_axi_arready"), s_axi_rdata("s_axi_rdata"), s_axi_rresp("s_axi_rresp"), s_axi_rvalid("s_axi_rvalid"), s_axi_rready("s_axi_rready"), s_axihub_awaddr("s_axihub_awaddr"), s_axihub_awvalid("s_axihub_awvalid"), s_axihub_awready("s_axihub_awready"), s_axihub_wdata("s_axihub_wdata"), s_axihub_wstrb("s_axihub_wstrb"), s_axihub_wvalid("s_axihub_wvalid"), s_axihub_wready("s_axihub_wready"), s_axihub_bvalid("s_axihub_bvalid"), s_axihub_bready("s_axihub_bready"), s_axihub_bresp("s_axihub_bresp"), s_axihub_araddr("s_axihub_araddr"), s_axihub_arvalid("s_axihub_arvalid"), s_axihub_arready("s_axihub_arready"), s_axihub_rdata("s_axihub_rdata"), s_axihub_rresp("s_axihub_rresp"), s_axihub_rvalid("s_axihub_rvalid"), s_axihub_rready("s_axihub_rready"), s_aximm_awaddr("s_aximm_awaddr"), s_aximm_awvalid("s_aximm_awvalid"), s_aximm_awready("s_aximm_awready"), s_aximm_awlen("s_aximm_awlen"), s_aximm_awsize("s_aximm_awsize"), s_aximm_awburst("s_aximm_awburst"), s_aximm_awid("s_aximm_awid"), s_aximm_wdata("s_aximm_wdata"), s_aximm_wstrb("s_aximm_wstrb"), s_aximm_wvalid("s_aximm_wvalid"), s_aximm_wready("s_aximm_wready"), s_aximm_bvalid("s_aximm_bvalid"), s_aximm_bready("s_aximm_bready"), s_aximm_wlast("s_aximm_wlast"), s_aximm_rlast("s_aximm_rlast"), s_aximm_bresp("s_aximm_bresp"), s_aximm_bid("s_aximm_bid"), s_aximm_araddr("s_aximm_araddr"), s_aximm_arvalid("s_aximm_arvalid"), s_aximm_arready("s_aximm_arready"), s_aximm_arlen("s_aximm_arlen"), s_aximm_arsize("s_aximm_arsize"), s_aximm_arburst("s_aximm_arburst"), s_aximm_arid("s_aximm_arid"), s_aximm_rdata("s_aximm_rdata"), s_aximm_rid("s_aximm_rid"), s_aximm_rresp("s_aximm_rresp"), s_aximm_rvalid("s_aximm_rvalid"), s_aximm_rready("s_aximm_rready")
{
  // initialize pins
  mp_impl->s_aximm_clk(s_aximm_clk);
  mp_impl->s_aximm_aresetn(s_aximm_aresetn);
  mp_impl->trace_clk(trace_clk);
  mp_impl->trace_aresetn(trace_aresetn);
  mp_impl->axilite_clk(axilite_clk);
  mp_impl->axilite_aresetn(axilite_aresetn);

  // initialize transactors
  mp_S_AXIMM_transactor = NULL;
  mp_S_AXIFIFO_transactor = NULL;
  mp_S_AXIHUB_transactor = NULL;
  mp_TRACE_IN_0_transactor = NULL;
  mp_TRACE_IN_1_transactor = NULL;
  mp_TRACE_IN_2_transactor = NULL;
  mp_TRACE_IN_3_transactor = NULL;
  mp_TRACE_IN_4_transactor = NULL;
  mp_TRACE_IN_5_transactor = NULL;
  mp_TRACE_IN_6_transactor = NULL;
  mp_TRACE_IN_7_transactor = NULL;
  mp_TRACE_IN_8_transactor = NULL;
  mp_TRACE_IN_9_transactor = NULL;
  mp_TRACE_IN_10_transactor = NULL;
  mp_TRACE_IN_11_transactor = NULL;
  mp_TRACE_IN_12_transactor = NULL;
  mp_TRACE_IN_13_transactor = NULL;
  mp_TRACE_IN_14_transactor = NULL;
  mp_TRACE_IN_15_transactor = NULL;
  mp_TRACE_IN_16_transactor = NULL;
  mp_TRACE_IN_17_transactor = NULL;
  mp_TRACE_IN_18_transactor = NULL;
  mp_TRACE_IN_19_transactor = NULL;
  mp_TRACE_IN_20_transactor = NULL;
  mp_TRACE_IN_21_transactor = NULL;
  mp_TRACE_IN_22_transactor = NULL;
  mp_TRACE_IN_23_transactor = NULL;
  mp_TRACE_IN_24_transactor = NULL;
  mp_TRACE_IN_25_transactor = NULL;
  mp_TRACE_IN_26_transactor = NULL;
  mp_TRACE_IN_27_transactor = NULL;
  mp_TRACE_IN_28_transactor = NULL;
  mp_TRACE_IN_29_transactor = NULL;
  mp_TRACE_IN_30_transactor = NULL;
  mp_TRACE_IN_31_transactor = NULL;
  mp_TRACE_IN_32_transactor = NULL;
  mp_TRACE_IN_33_transactor = NULL;
  mp_TRACE_IN_34_transactor = NULL;
  mp_TRACE_IN_35_transactor = NULL;
  mp_TRACE_IN_36_transactor = NULL;
  mp_TRACE_IN_37_transactor = NULL;
  mp_TRACE_IN_38_transactor = NULL;
  mp_TRACE_IN_39_transactor = NULL;
  mp_TRACE_IN_40_transactor = NULL;

  // Instantiate Socket Stubs

  // configure S_AXIMM_transactor
    xsc::common_cpp::properties S_AXIMM_transactor_param_props;
    S_AXIMM_transactor_param_props.addLong("DATA_WIDTH", "64");
    S_AXIMM_transactor_param_props.addLong("FREQ_HZ", "300000000");
    S_AXIMM_transactor_param_props.addLong("ID_WIDTH", "1");
    S_AXIMM_transactor_param_props.addLong("ADDR_WIDTH", "32");
    S_AXIMM_transactor_param_props.addLong("AWUSER_WIDTH", "0");
    S_AXIMM_transactor_param_props.addLong("ARUSER_WIDTH", "0");
    S_AXIMM_transactor_param_props.addLong("WUSER_WIDTH", "0");
    S_AXIMM_transactor_param_props.addLong("RUSER_WIDTH", "0");
    S_AXIMM_transactor_param_props.addLong("BUSER_WIDTH", "0");
    S_AXIMM_transactor_param_props.addLong("HAS_BURST", "1");
    S_AXIMM_transactor_param_props.addLong("HAS_LOCK", "0");
    S_AXIMM_transactor_param_props.addLong("HAS_PROT", "0");
    S_AXIMM_transactor_param_props.addLong("HAS_CACHE", "0");
    S_AXIMM_transactor_param_props.addLong("HAS_QOS", "0");
    S_AXIMM_transactor_param_props.addLong("HAS_REGION", "0");
    S_AXIMM_transactor_param_props.addLong("HAS_WSTRB", "1");
    S_AXIMM_transactor_param_props.addLong("HAS_BRESP", "1");
    S_AXIMM_transactor_param_props.addLong("HAS_RRESP", "1");
    S_AXIMM_transactor_param_props.addLong("SUPPORTS_NARROW_BURST", "0");
    S_AXIMM_transactor_param_props.addLong("NUM_READ_OUTSTANDING", "2");
    S_AXIMM_transactor_param_props.addLong("NUM_WRITE_OUTSTANDING", "2");
    S_AXIMM_transactor_param_props.addLong("MAX_BURST_LENGTH", "128");
    S_AXIMM_transactor_param_props.addLong("NUM_READ_THREADS", "1");
    S_AXIMM_transactor_param_props.addLong("NUM_WRITE_THREADS", "1");
    S_AXIMM_transactor_param_props.addLong("RUSER_BITS_PER_BYTE", "0");
    S_AXIMM_transactor_param_props.addLong("WUSER_BITS_PER_BYTE", "0");
    S_AXIMM_transactor_param_props.addLong("HAS_SIZE", "1");
    S_AXIMM_transactor_param_props.addLong("HAS_RESET", "1");
    S_AXIMM_transactor_param_props.addFloat("PHASE", "0.0");
    S_AXIMM_transactor_param_props.addString("PROTOCOL", "AXI4");
    S_AXIMM_transactor_param_props.addString("READ_WRITE_MODE", "READ_WRITE");
    S_AXIMM_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_dma_pcie_aclk");
    S_AXIMM_transactor_param_props.addString("SLR_ASSIGNMENT", "");

    mp_S_AXIMM_transactor = new xtlm::xaximm_pin2xtlm_t<64,32,1,1,1,1,1,1>("S_AXIMM_transactor", S_AXIMM_transactor_param_props);
  mp_S_AXIMM_transactor->ARADDR(s_aximm_araddr);
  mp_S_AXIMM_transactor->ARBURST(s_aximm_arburst);
  mp_S_AXIMM_transactor->ARID(s_aximm_arid);
  mp_S_AXIMM_transactor->ARLEN(s_aximm_arlen);
  mp_S_AXIMM_transactor->ARREADY(s_aximm_arready);
  mp_S_AXIMM_transactor->ARSIZE(s_aximm_arsize);
  mp_S_AXIMM_transactor->ARVALID(s_aximm_arvalid);
  mp_S_AXIMM_transactor->AWADDR(s_aximm_awaddr);
  mp_S_AXIMM_transactor->AWBURST(s_aximm_awburst);
  mp_S_AXIMM_transactor->AWID(s_aximm_awid);
  mp_S_AXIMM_transactor->AWLEN(s_aximm_awlen);
  mp_S_AXIMM_transactor->AWREADY(s_aximm_awready);
  mp_S_AXIMM_transactor->AWSIZE(s_aximm_awsize);
  mp_S_AXIMM_transactor->AWVALID(s_aximm_awvalid);
  mp_S_AXIMM_transactor->BID(s_aximm_bid);
  mp_S_AXIMM_transactor->BREADY(s_aximm_bready);
  mp_S_AXIMM_transactor->BRESP(s_aximm_bresp);
  mp_S_AXIMM_transactor->BVALID(s_aximm_bvalid);
  mp_S_AXIMM_transactor->RDATA(s_aximm_rdata);
  mp_S_AXIMM_transactor->RID(s_aximm_rid);
  mp_S_AXIMM_transactor->RLAST(s_aximm_rlast);
  mp_S_AXIMM_transactor->RREADY(s_aximm_rready);
  mp_S_AXIMM_transactor->RRESP(s_aximm_rresp);
  mp_S_AXIMM_transactor->RVALID(s_aximm_rvalid);
  mp_S_AXIMM_transactor->WDATA(s_aximm_wdata);
  mp_S_AXIMM_transactor->WLAST(s_aximm_wlast);
  mp_S_AXIMM_transactor->WREADY(s_aximm_wready);
  mp_S_AXIMM_transactor->WSTRB(s_aximm_wstrb);
  mp_S_AXIMM_transactor->WVALID(s_aximm_wvalid);
  mp_S_AXIMM_transactor->CLK(s_aximm_clk);
  mp_S_AXIMM_transactor->RST(s_aximm_aresetn);
  // configure S_AXIFIFO_transactor
    xsc::common_cpp::properties S_AXIFIFO_transactor_param_props;
    S_AXIFIFO_transactor_param_props.addLong("DATA_WIDTH", "32");
    S_AXIFIFO_transactor_param_props.addLong("FREQ_HZ", "300000000");
    S_AXIFIFO_transactor_param_props.addLong("ID_WIDTH", "0");
    S_AXIFIFO_transactor_param_props.addLong("ADDR_WIDTH", "8");
    S_AXIFIFO_transactor_param_props.addLong("AWUSER_WIDTH", "0");
    S_AXIFIFO_transactor_param_props.addLong("ARUSER_WIDTH", "0");
    S_AXIFIFO_transactor_param_props.addLong("WUSER_WIDTH", "0");
    S_AXIFIFO_transactor_param_props.addLong("RUSER_WIDTH", "0");
    S_AXIFIFO_transactor_param_props.addLong("BUSER_WIDTH", "0");
    S_AXIFIFO_transactor_param_props.addLong("HAS_BURST", "0");
    S_AXIFIFO_transactor_param_props.addLong("HAS_LOCK", "0");
    S_AXIFIFO_transactor_param_props.addLong("HAS_PROT", "0");
    S_AXIFIFO_transactor_param_props.addLong("HAS_CACHE", "0");
    S_AXIFIFO_transactor_param_props.addLong("HAS_QOS", "0");
    S_AXIFIFO_transactor_param_props.addLong("HAS_REGION", "0");
    S_AXIFIFO_transactor_param_props.addLong("HAS_WSTRB", "1");
    S_AXIFIFO_transactor_param_props.addLong("HAS_BRESP", "1");
    S_AXIFIFO_transactor_param_props.addLong("HAS_RRESP", "1");
    S_AXIFIFO_transactor_param_props.addLong("SUPPORTS_NARROW_BURST", "0");
    S_AXIFIFO_transactor_param_props.addLong("NUM_READ_OUTSTANDING", "2");
    S_AXIFIFO_transactor_param_props.addLong("NUM_WRITE_OUTSTANDING", "2");
    S_AXIFIFO_transactor_param_props.addLong("MAX_BURST_LENGTH", "1");
    S_AXIFIFO_transactor_param_props.addLong("NUM_READ_THREADS", "1");
    S_AXIFIFO_transactor_param_props.addLong("NUM_WRITE_THREADS", "1");
    S_AXIFIFO_transactor_param_props.addLong("RUSER_BITS_PER_BYTE", "0");
    S_AXIFIFO_transactor_param_props.addLong("WUSER_BITS_PER_BYTE", "0");
    S_AXIFIFO_transactor_param_props.addLong("HAS_SIZE", "0");
    S_AXIFIFO_transactor_param_props.addLong("HAS_RESET", "1");
    S_AXIFIFO_transactor_param_props.addFloat("PHASE", "0.0");
    S_AXIFIFO_transactor_param_props.addString("PROTOCOL", "AXI4LITE");
    S_AXIFIFO_transactor_param_props.addString("READ_WRITE_MODE", "READ_WRITE");
    S_AXIFIFO_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    S_AXIFIFO_transactor_param_props.addString("SLR_ASSIGNMENT", "");

    mp_S_AXIFIFO_transactor = new xtlm::xaximm_pin2xtlm_t<32,8,1,1,1,1,1,1>("S_AXIFIFO_transactor", S_AXIFIFO_transactor_param_props);
  mp_S_AXIFIFO_transactor->ARADDR(s_axi_araddr);
  mp_S_AXIFIFO_transactor->ARREADY(s_axi_arready);
  mp_S_AXIFIFO_transactor->ARVALID(s_axi_arvalid);
  mp_S_AXIFIFO_transactor->AWADDR(s_axi_awaddr);
  mp_S_AXIFIFO_transactor->AWREADY(s_axi_awready);
  mp_S_AXIFIFO_transactor->AWVALID(s_axi_awvalid);
  mp_S_AXIFIFO_transactor->BREADY(s_axi_bready);
  mp_S_AXIFIFO_transactor->BRESP(s_axi_bresp);
  mp_S_AXIFIFO_transactor->BVALID(s_axi_bvalid);
  mp_S_AXIFIFO_transactor->RDATA(s_axi_rdata);
  mp_S_AXIFIFO_transactor->RREADY(s_axi_rready);
  mp_S_AXIFIFO_transactor->RRESP(s_axi_rresp);
  mp_S_AXIFIFO_transactor->RVALID(s_axi_rvalid);
  mp_S_AXIFIFO_transactor->WDATA(s_axi_wdata);
  mp_S_AXIFIFO_transactor->WREADY(s_axi_wready);
  mp_S_AXIFIFO_transactor->WSTRB(s_axi_wstrb);
  mp_S_AXIFIFO_transactor->WVALID(s_axi_wvalid);
  mp_S_AXIFIFO_transactor->CLK(axilite_clk);
  mp_S_AXIFIFO_transactor->RST(axilite_aresetn);
  // configure S_AXIHUB_transactor
    xsc::common_cpp::properties S_AXIHUB_transactor_param_props;
    S_AXIHUB_transactor_param_props.addLong("DATA_WIDTH", "32");
    S_AXIHUB_transactor_param_props.addLong("FREQ_HZ", "300000000");
    S_AXIHUB_transactor_param_props.addLong("ID_WIDTH", "0");
    S_AXIHUB_transactor_param_props.addLong("ADDR_WIDTH", "8");
    S_AXIHUB_transactor_param_props.addLong("AWUSER_WIDTH", "0");
    S_AXIHUB_transactor_param_props.addLong("ARUSER_WIDTH", "0");
    S_AXIHUB_transactor_param_props.addLong("WUSER_WIDTH", "0");
    S_AXIHUB_transactor_param_props.addLong("RUSER_WIDTH", "0");
    S_AXIHUB_transactor_param_props.addLong("BUSER_WIDTH", "0");
    S_AXIHUB_transactor_param_props.addLong("HAS_BURST", "0");
    S_AXIHUB_transactor_param_props.addLong("HAS_LOCK", "0");
    S_AXIHUB_transactor_param_props.addLong("HAS_PROT", "0");
    S_AXIHUB_transactor_param_props.addLong("HAS_CACHE", "0");
    S_AXIHUB_transactor_param_props.addLong("HAS_QOS", "0");
    S_AXIHUB_transactor_param_props.addLong("HAS_REGION", "0");
    S_AXIHUB_transactor_param_props.addLong("HAS_WSTRB", "1");
    S_AXIHUB_transactor_param_props.addLong("HAS_BRESP", "1");
    S_AXIHUB_transactor_param_props.addLong("HAS_RRESP", "1");
    S_AXIHUB_transactor_param_props.addLong("SUPPORTS_NARROW_BURST", "0");
    S_AXIHUB_transactor_param_props.addLong("NUM_READ_OUTSTANDING", "2");
    S_AXIHUB_transactor_param_props.addLong("NUM_WRITE_OUTSTANDING", "2");
    S_AXIHUB_transactor_param_props.addLong("MAX_BURST_LENGTH", "1");
    S_AXIHUB_transactor_param_props.addLong("NUM_READ_THREADS", "1");
    S_AXIHUB_transactor_param_props.addLong("NUM_WRITE_THREADS", "1");
    S_AXIHUB_transactor_param_props.addLong("RUSER_BITS_PER_BYTE", "0");
    S_AXIHUB_transactor_param_props.addLong("WUSER_BITS_PER_BYTE", "0");
    S_AXIHUB_transactor_param_props.addLong("HAS_SIZE", "0");
    S_AXIHUB_transactor_param_props.addLong("HAS_RESET", "1");
    S_AXIHUB_transactor_param_props.addFloat("PHASE", "0.0");
    S_AXIHUB_transactor_param_props.addString("PROTOCOL", "AXI4LITE");
    S_AXIHUB_transactor_param_props.addString("READ_WRITE_MODE", "READ_WRITE");
    S_AXIHUB_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    S_AXIHUB_transactor_param_props.addString("SLR_ASSIGNMENT", "");

    mp_S_AXIHUB_transactor = new xtlm::xaximm_pin2xtlm_t<32,8,1,1,1,1,1,1>("S_AXIHUB_transactor", S_AXIHUB_transactor_param_props);
  mp_S_AXIHUB_transactor->ARADDR(s_axihub_araddr);
  mp_S_AXIHUB_transactor->ARREADY(s_axihub_arready);
  mp_S_AXIHUB_transactor->ARVALID(s_axihub_arvalid);
  mp_S_AXIHUB_transactor->AWADDR(s_axihub_awaddr);
  mp_S_AXIHUB_transactor->AWREADY(s_axihub_awready);
  mp_S_AXIHUB_transactor->AWVALID(s_axihub_awvalid);
  mp_S_AXIHUB_transactor->BREADY(s_axihub_bready);
  mp_S_AXIHUB_transactor->BRESP(s_axihub_bresp);
  mp_S_AXIHUB_transactor->BVALID(s_axihub_bvalid);
  mp_S_AXIHUB_transactor->RDATA(s_axihub_rdata);
  mp_S_AXIHUB_transactor->RREADY(s_axihub_rready);
  mp_S_AXIHUB_transactor->RRESP(s_axihub_rresp);
  mp_S_AXIHUB_transactor->RVALID(s_axihub_rvalid);
  mp_S_AXIHUB_transactor->WDATA(s_axihub_wdata);
  mp_S_AXIHUB_transactor->WREADY(s_axihub_wready);
  mp_S_AXIHUB_transactor->WSTRB(s_axihub_wstrb);
  mp_S_AXIHUB_transactor->WVALID(s_axihub_wvalid);
  mp_S_AXIHUB_transactor->CLK(axilite_clk);
  mp_S_AXIHUB_transactor->RST(axilite_aresetn);
  // configure TRACE_IN_0_transactor
    xsc::common_cpp::properties TRACE_IN_0_transactor_param_props;
    TRACE_IN_0_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_0_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_0_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_0_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_0_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_0_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_0_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_0_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_0_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_0_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_0_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_0_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_0_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_0_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_0_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_0_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_0_transactor", TRACE_IN_0_transactor_param_props);
  mp_TRACE_IN_0_transactor->TDATA(trace_tdata0);
  mp_TRACE_IN_0_transactor->TDEST(trace_tdest0);
  mp_TRACE_IN_0_transactor->TID(trace_tid0);
  mp_TRACE_IN_0_transactor->TLAST(trace_tlast0);
  mp_TRACE_IN_0_transactor->TREADY(trace_tready0);
  mp_TRACE_IN_0_transactor->TVALID(trace_tvalid0);
  mp_TRACE_IN_0_transactor->CLK(trace_clk);
  mp_TRACE_IN_0_transactor->RST(trace_aresetn);
  // configure TRACE_IN_1_transactor
    xsc::common_cpp::properties TRACE_IN_1_transactor_param_props;
    TRACE_IN_1_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_1_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_1_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_1_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_1_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_1_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_1_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_1_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_1_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_1_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_1_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_1_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_1_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_1_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_1_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_1_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_1_transactor", TRACE_IN_1_transactor_param_props);
  mp_TRACE_IN_1_transactor->TDATA(trace_tdata1);
  mp_TRACE_IN_1_transactor->TDEST(trace_tdest1);
  mp_TRACE_IN_1_transactor->TID(trace_tid1);
  mp_TRACE_IN_1_transactor->TLAST(trace_tlast1);
  mp_TRACE_IN_1_transactor->TREADY(trace_tready1);
  mp_TRACE_IN_1_transactor->TVALID(trace_tvalid1);
  mp_TRACE_IN_1_transactor->CLK(trace_clk);
  mp_TRACE_IN_1_transactor->RST(trace_aresetn);
  // configure TRACE_IN_2_transactor
    xsc::common_cpp::properties TRACE_IN_2_transactor_param_props;
    TRACE_IN_2_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_2_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_2_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_2_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_2_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_2_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_2_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_2_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_2_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_2_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_2_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_2_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_2_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_2_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_2_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_2_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_2_transactor", TRACE_IN_2_transactor_param_props);
  mp_TRACE_IN_2_transactor->TDATA(trace_tdata2);
  mp_TRACE_IN_2_transactor->TDEST(trace_tdest2);
  mp_TRACE_IN_2_transactor->TID(trace_tid2);
  mp_TRACE_IN_2_transactor->TLAST(trace_tlast2);
  mp_TRACE_IN_2_transactor->TREADY(trace_tready2);
  mp_TRACE_IN_2_transactor->TVALID(trace_tvalid2);
  mp_TRACE_IN_2_transactor->CLK(trace_clk);
  mp_TRACE_IN_2_transactor->RST(trace_aresetn);
  // configure TRACE_IN_3_transactor
    xsc::common_cpp::properties TRACE_IN_3_transactor_param_props;
    TRACE_IN_3_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_3_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_3_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_3_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_3_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_3_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_3_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_3_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_3_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_3_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_3_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_3_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_3_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_3_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_3_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_3_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_3_transactor", TRACE_IN_3_transactor_param_props);
  mp_TRACE_IN_3_transactor->TDATA(trace_tdata3);
  mp_TRACE_IN_3_transactor->TDEST(trace_tdest3);
  mp_TRACE_IN_3_transactor->TID(trace_tid3);
  mp_TRACE_IN_3_transactor->TLAST(trace_tlast3);
  mp_TRACE_IN_3_transactor->TREADY(trace_tready3);
  mp_TRACE_IN_3_transactor->TVALID(trace_tvalid3);
  mp_TRACE_IN_3_transactor->CLK(trace_clk);
  mp_TRACE_IN_3_transactor->RST(trace_aresetn);
  // configure TRACE_IN_4_transactor
    xsc::common_cpp::properties TRACE_IN_4_transactor_param_props;
    TRACE_IN_4_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_4_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_4_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_4_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_4_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_4_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_4_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_4_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_4_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_4_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_4_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_4_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_4_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_4_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_4_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_4_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_4_transactor", TRACE_IN_4_transactor_param_props);
  mp_TRACE_IN_4_transactor->TDATA(trace_tdata4);
  mp_TRACE_IN_4_transactor->TDEST(trace_tdest4);
  mp_TRACE_IN_4_transactor->TID(trace_tid4);
  mp_TRACE_IN_4_transactor->TLAST(trace_tlast4);
  mp_TRACE_IN_4_transactor->TREADY(trace_tready4);
  mp_TRACE_IN_4_transactor->TVALID(trace_tvalid4);
  mp_TRACE_IN_4_transactor->CLK(trace_clk);
  mp_TRACE_IN_4_transactor->RST(trace_aresetn);
  // configure TRACE_IN_5_transactor
    xsc::common_cpp::properties TRACE_IN_5_transactor_param_props;
    TRACE_IN_5_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_5_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_5_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_5_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_5_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_5_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_5_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_5_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_5_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_5_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_5_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_5_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_5_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_5_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_5_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_5_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_5_transactor", TRACE_IN_5_transactor_param_props);
  mp_TRACE_IN_5_transactor->TDATA(trace_tdata5);
  mp_TRACE_IN_5_transactor->TDEST(trace_tdest5);
  mp_TRACE_IN_5_transactor->TID(trace_tid5);
  mp_TRACE_IN_5_transactor->TLAST(trace_tlast5);
  mp_TRACE_IN_5_transactor->TREADY(trace_tready5);
  mp_TRACE_IN_5_transactor->TVALID(trace_tvalid5);
  mp_TRACE_IN_5_transactor->CLK(trace_clk);
  mp_TRACE_IN_5_transactor->RST(trace_aresetn);
  // configure TRACE_IN_6_transactor
    xsc::common_cpp::properties TRACE_IN_6_transactor_param_props;
    TRACE_IN_6_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_6_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_6_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_6_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_6_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_6_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_6_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_6_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_6_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_6_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_6_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_6_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_6_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_6_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_6_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_6_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_6_transactor", TRACE_IN_6_transactor_param_props);
  mp_TRACE_IN_6_transactor->TDATA(trace_tdata6);
  mp_TRACE_IN_6_transactor->TDEST(trace_tdest6);
  mp_TRACE_IN_6_transactor->TID(trace_tid6);
  mp_TRACE_IN_6_transactor->TLAST(trace_tlast6);
  mp_TRACE_IN_6_transactor->TREADY(trace_tready6);
  mp_TRACE_IN_6_transactor->TVALID(trace_tvalid6);
  mp_TRACE_IN_6_transactor->CLK(trace_clk);
  mp_TRACE_IN_6_transactor->RST(trace_aresetn);
  // configure TRACE_IN_7_transactor
    xsc::common_cpp::properties TRACE_IN_7_transactor_param_props;
    TRACE_IN_7_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_7_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_7_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_7_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_7_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_7_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_7_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_7_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_7_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_7_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_7_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_7_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_7_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_7_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_7_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_7_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_7_transactor", TRACE_IN_7_transactor_param_props);
  mp_TRACE_IN_7_transactor->TDATA(trace_tdata7);
  mp_TRACE_IN_7_transactor->TDEST(trace_tdest7);
  mp_TRACE_IN_7_transactor->TID(trace_tid7);
  mp_TRACE_IN_7_transactor->TLAST(trace_tlast7);
  mp_TRACE_IN_7_transactor->TREADY(trace_tready7);
  mp_TRACE_IN_7_transactor->TVALID(trace_tvalid7);
  mp_TRACE_IN_7_transactor->CLK(trace_clk);
  mp_TRACE_IN_7_transactor->RST(trace_aresetn);
  // configure TRACE_IN_8_transactor
    xsc::common_cpp::properties TRACE_IN_8_transactor_param_props;
    TRACE_IN_8_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_8_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_8_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_8_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_8_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_8_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_8_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_8_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_8_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_8_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_8_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_8_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_8_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_8_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_8_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_8_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_8_transactor", TRACE_IN_8_transactor_param_props);
  mp_TRACE_IN_8_transactor->TDATA(trace_tdata8);
  mp_TRACE_IN_8_transactor->TDEST(trace_tdest8);
  mp_TRACE_IN_8_transactor->TID(trace_tid8);
  mp_TRACE_IN_8_transactor->TLAST(trace_tlast8);
  mp_TRACE_IN_8_transactor->TREADY(trace_tready8);
  mp_TRACE_IN_8_transactor->TVALID(trace_tvalid8);
  mp_TRACE_IN_8_transactor->CLK(trace_clk);
  mp_TRACE_IN_8_transactor->RST(trace_aresetn);
  // configure TRACE_IN_9_transactor
    xsc::common_cpp::properties TRACE_IN_9_transactor_param_props;
    TRACE_IN_9_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_9_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_9_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_9_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_9_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_9_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_9_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_9_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_9_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_9_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_9_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_9_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_9_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_9_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_9_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_9_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_9_transactor", TRACE_IN_9_transactor_param_props);
  mp_TRACE_IN_9_transactor->TDATA(trace_tdata9);
  mp_TRACE_IN_9_transactor->TDEST(trace_tdest9);
  mp_TRACE_IN_9_transactor->TID(trace_tid9);
  mp_TRACE_IN_9_transactor->TLAST(trace_tlast9);
  mp_TRACE_IN_9_transactor->TREADY(trace_tready9);
  mp_TRACE_IN_9_transactor->TVALID(trace_tvalid9);
  mp_TRACE_IN_9_transactor->CLK(trace_clk);
  mp_TRACE_IN_9_transactor->RST(trace_aresetn);
  // configure TRACE_IN_10_transactor
    xsc::common_cpp::properties TRACE_IN_10_transactor_param_props;
    TRACE_IN_10_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_10_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_10_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_10_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_10_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_10_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_10_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_10_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_10_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_10_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_10_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_10_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_10_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_10_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_10_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_10_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_10_transactor", TRACE_IN_10_transactor_param_props);
  mp_TRACE_IN_10_transactor->TDATA(trace_tdata10);
  mp_TRACE_IN_10_transactor->TDEST(trace_tdest10);
  mp_TRACE_IN_10_transactor->TID(trace_tid10);
  mp_TRACE_IN_10_transactor->TLAST(trace_tlast10);
  mp_TRACE_IN_10_transactor->TREADY(trace_tready10);
  mp_TRACE_IN_10_transactor->TVALID(trace_tvalid10);
  mp_TRACE_IN_10_transactor->CLK(trace_clk);
  mp_TRACE_IN_10_transactor->RST(trace_aresetn);
  // configure TRACE_IN_11_transactor
    xsc::common_cpp::properties TRACE_IN_11_transactor_param_props;
    TRACE_IN_11_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_11_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_11_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_11_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_11_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_11_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_11_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_11_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_11_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_11_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_11_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_11_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_11_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_11_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_11_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_11_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_11_transactor", TRACE_IN_11_transactor_param_props);
  mp_TRACE_IN_11_transactor->TDATA(trace_tdata11);
  mp_TRACE_IN_11_transactor->TDEST(trace_tdest11);
  mp_TRACE_IN_11_transactor->TID(trace_tid11);
  mp_TRACE_IN_11_transactor->TLAST(trace_tlast11);
  mp_TRACE_IN_11_transactor->TREADY(trace_tready11);
  mp_TRACE_IN_11_transactor->TVALID(trace_tvalid11);
  mp_TRACE_IN_11_transactor->CLK(trace_clk);
  mp_TRACE_IN_11_transactor->RST(trace_aresetn);
  // configure TRACE_IN_12_transactor
    xsc::common_cpp::properties TRACE_IN_12_transactor_param_props;
    TRACE_IN_12_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_12_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_12_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_12_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_12_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_12_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_12_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_12_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_12_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_12_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_12_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_12_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_12_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_12_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_12_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_12_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_12_transactor", TRACE_IN_12_transactor_param_props);
  mp_TRACE_IN_12_transactor->TDATA(trace_tdata12);
  mp_TRACE_IN_12_transactor->TDEST(trace_tdest12);
  mp_TRACE_IN_12_transactor->TID(trace_tid12);
  mp_TRACE_IN_12_transactor->TLAST(trace_tlast12);
  mp_TRACE_IN_12_transactor->TREADY(trace_tready12);
  mp_TRACE_IN_12_transactor->TVALID(trace_tvalid12);
  mp_TRACE_IN_12_transactor->CLK(trace_clk);
  mp_TRACE_IN_12_transactor->RST(trace_aresetn);
  // configure TRACE_IN_13_transactor
    xsc::common_cpp::properties TRACE_IN_13_transactor_param_props;
    TRACE_IN_13_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_13_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_13_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_13_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_13_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_13_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_13_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_13_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_13_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_13_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_13_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_13_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_13_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_13_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_13_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_13_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_13_transactor", TRACE_IN_13_transactor_param_props);
  mp_TRACE_IN_13_transactor->TDATA(trace_tdata13);
  mp_TRACE_IN_13_transactor->TDEST(trace_tdest13);
  mp_TRACE_IN_13_transactor->TID(trace_tid13);
  mp_TRACE_IN_13_transactor->TLAST(trace_tlast13);
  mp_TRACE_IN_13_transactor->TREADY(trace_tready13);
  mp_TRACE_IN_13_transactor->TVALID(trace_tvalid13);
  mp_TRACE_IN_13_transactor->CLK(trace_clk);
  mp_TRACE_IN_13_transactor->RST(trace_aresetn);
  // configure TRACE_IN_14_transactor
    xsc::common_cpp::properties TRACE_IN_14_transactor_param_props;
    TRACE_IN_14_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_14_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_14_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_14_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_14_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_14_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_14_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_14_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_14_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_14_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_14_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_14_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_14_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_14_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_14_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_14_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_14_transactor", TRACE_IN_14_transactor_param_props);
  mp_TRACE_IN_14_transactor->TDATA(trace_tdata14);
  mp_TRACE_IN_14_transactor->TDEST(trace_tdest14);
  mp_TRACE_IN_14_transactor->TID(trace_tid14);
  mp_TRACE_IN_14_transactor->TLAST(trace_tlast14);
  mp_TRACE_IN_14_transactor->TREADY(trace_tready14);
  mp_TRACE_IN_14_transactor->TVALID(trace_tvalid14);
  mp_TRACE_IN_14_transactor->CLK(trace_clk);
  mp_TRACE_IN_14_transactor->RST(trace_aresetn);
  // configure TRACE_IN_15_transactor
    xsc::common_cpp::properties TRACE_IN_15_transactor_param_props;
    TRACE_IN_15_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_15_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_15_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_15_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_15_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_15_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_15_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_15_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_15_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_15_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_15_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_15_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_15_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_15_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_15_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_15_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_15_transactor", TRACE_IN_15_transactor_param_props);
  mp_TRACE_IN_15_transactor->TDATA(trace_tdata15);
  mp_TRACE_IN_15_transactor->TDEST(trace_tdest15);
  mp_TRACE_IN_15_transactor->TID(trace_tid15);
  mp_TRACE_IN_15_transactor->TLAST(trace_tlast15);
  mp_TRACE_IN_15_transactor->TREADY(trace_tready15);
  mp_TRACE_IN_15_transactor->TVALID(trace_tvalid15);
  mp_TRACE_IN_15_transactor->CLK(trace_clk);
  mp_TRACE_IN_15_transactor->RST(trace_aresetn);
  // configure TRACE_IN_16_transactor
    xsc::common_cpp::properties TRACE_IN_16_transactor_param_props;
    TRACE_IN_16_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_16_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_16_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_16_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_16_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_16_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_16_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_16_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_16_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_16_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_16_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_16_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_16_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_16_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_16_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_16_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_16_transactor", TRACE_IN_16_transactor_param_props);
  mp_TRACE_IN_16_transactor->TDATA(trace_tdata16);
  mp_TRACE_IN_16_transactor->TDEST(trace_tdest16);
  mp_TRACE_IN_16_transactor->TID(trace_tid16);
  mp_TRACE_IN_16_transactor->TLAST(trace_tlast16);
  mp_TRACE_IN_16_transactor->TREADY(trace_tready16);
  mp_TRACE_IN_16_transactor->TVALID(trace_tvalid16);
  mp_TRACE_IN_16_transactor->CLK(trace_clk);
  mp_TRACE_IN_16_transactor->RST(trace_aresetn);
  // configure TRACE_IN_17_transactor
    xsc::common_cpp::properties TRACE_IN_17_transactor_param_props;
    TRACE_IN_17_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_17_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_17_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_17_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_17_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_17_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_17_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_17_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_17_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_17_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_17_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_17_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_17_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_17_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_17_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_17_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_17_transactor", TRACE_IN_17_transactor_param_props);
  mp_TRACE_IN_17_transactor->TDATA(trace_tdata17);
  mp_TRACE_IN_17_transactor->TDEST(trace_tdest17);
  mp_TRACE_IN_17_transactor->TID(trace_tid17);
  mp_TRACE_IN_17_transactor->TLAST(trace_tlast17);
  mp_TRACE_IN_17_transactor->TREADY(trace_tready17);
  mp_TRACE_IN_17_transactor->TVALID(trace_tvalid17);
  mp_TRACE_IN_17_transactor->CLK(trace_clk);
  mp_TRACE_IN_17_transactor->RST(trace_aresetn);
  // configure TRACE_IN_18_transactor
    xsc::common_cpp::properties TRACE_IN_18_transactor_param_props;
    TRACE_IN_18_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_18_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_18_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_18_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_18_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_18_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_18_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_18_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_18_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_18_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_18_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_18_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_18_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_18_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_18_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_18_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_18_transactor", TRACE_IN_18_transactor_param_props);
  mp_TRACE_IN_18_transactor->TDATA(trace_tdata18);
  mp_TRACE_IN_18_transactor->TDEST(trace_tdest18);
  mp_TRACE_IN_18_transactor->TID(trace_tid18);
  mp_TRACE_IN_18_transactor->TLAST(trace_tlast18);
  mp_TRACE_IN_18_transactor->TREADY(trace_tready18);
  mp_TRACE_IN_18_transactor->TVALID(trace_tvalid18);
  mp_TRACE_IN_18_transactor->CLK(trace_clk);
  mp_TRACE_IN_18_transactor->RST(trace_aresetn);
  // configure TRACE_IN_19_transactor
    xsc::common_cpp::properties TRACE_IN_19_transactor_param_props;
    TRACE_IN_19_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_19_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_19_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_19_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_19_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_19_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_19_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_19_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_19_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_19_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_19_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_19_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_19_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_19_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_19_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_19_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_19_transactor", TRACE_IN_19_transactor_param_props);
  mp_TRACE_IN_19_transactor->TDATA(trace_tdata19);
  mp_TRACE_IN_19_transactor->TDEST(trace_tdest19);
  mp_TRACE_IN_19_transactor->TID(trace_tid19);
  mp_TRACE_IN_19_transactor->TLAST(trace_tlast19);
  mp_TRACE_IN_19_transactor->TREADY(trace_tready19);
  mp_TRACE_IN_19_transactor->TVALID(trace_tvalid19);
  mp_TRACE_IN_19_transactor->CLK(trace_clk);
  mp_TRACE_IN_19_transactor->RST(trace_aresetn);
  // configure TRACE_IN_20_transactor
    xsc::common_cpp::properties TRACE_IN_20_transactor_param_props;
    TRACE_IN_20_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_20_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_20_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_20_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_20_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_20_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_20_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_20_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_20_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_20_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_20_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_20_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_20_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_20_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_20_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_20_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_20_transactor", TRACE_IN_20_transactor_param_props);
  mp_TRACE_IN_20_transactor->TDATA(trace_tdata20);
  mp_TRACE_IN_20_transactor->TDEST(trace_tdest20);
  mp_TRACE_IN_20_transactor->TID(trace_tid20);
  mp_TRACE_IN_20_transactor->TLAST(trace_tlast20);
  mp_TRACE_IN_20_transactor->TREADY(trace_tready20);
  mp_TRACE_IN_20_transactor->TVALID(trace_tvalid20);
  mp_TRACE_IN_20_transactor->CLK(trace_clk);
  mp_TRACE_IN_20_transactor->RST(trace_aresetn);
  // configure TRACE_IN_21_transactor
    xsc::common_cpp::properties TRACE_IN_21_transactor_param_props;
    TRACE_IN_21_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_21_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_21_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_21_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_21_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_21_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_21_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_21_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_21_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_21_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_21_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_21_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_21_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_21_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_21_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_21_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_21_transactor", TRACE_IN_21_transactor_param_props);
  mp_TRACE_IN_21_transactor->TDATA(trace_tdata21);
  mp_TRACE_IN_21_transactor->TDEST(trace_tdest21);
  mp_TRACE_IN_21_transactor->TID(trace_tid21);
  mp_TRACE_IN_21_transactor->TLAST(trace_tlast21);
  mp_TRACE_IN_21_transactor->TREADY(trace_tready21);
  mp_TRACE_IN_21_transactor->TVALID(trace_tvalid21);
  mp_TRACE_IN_21_transactor->CLK(trace_clk);
  mp_TRACE_IN_21_transactor->RST(trace_aresetn);
  // configure TRACE_IN_22_transactor
    xsc::common_cpp::properties TRACE_IN_22_transactor_param_props;
    TRACE_IN_22_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_22_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_22_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_22_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_22_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_22_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_22_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_22_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_22_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_22_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_22_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_22_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_22_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_22_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_22_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_22_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_22_transactor", TRACE_IN_22_transactor_param_props);
  mp_TRACE_IN_22_transactor->TDATA(trace_tdata22);
  mp_TRACE_IN_22_transactor->TDEST(trace_tdest22);
  mp_TRACE_IN_22_transactor->TID(trace_tid22);
  mp_TRACE_IN_22_transactor->TLAST(trace_tlast22);
  mp_TRACE_IN_22_transactor->TREADY(trace_tready22);
  mp_TRACE_IN_22_transactor->TVALID(trace_tvalid22);
  mp_TRACE_IN_22_transactor->CLK(trace_clk);
  mp_TRACE_IN_22_transactor->RST(trace_aresetn);
  // configure TRACE_IN_23_transactor
    xsc::common_cpp::properties TRACE_IN_23_transactor_param_props;
    TRACE_IN_23_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_23_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_23_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_23_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_23_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_23_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_23_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_23_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_23_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_23_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_23_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_23_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_23_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_23_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_23_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_23_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_23_transactor", TRACE_IN_23_transactor_param_props);
  mp_TRACE_IN_23_transactor->TDATA(trace_tdata23);
  mp_TRACE_IN_23_transactor->TDEST(trace_tdest23);
  mp_TRACE_IN_23_transactor->TID(trace_tid23);
  mp_TRACE_IN_23_transactor->TLAST(trace_tlast23);
  mp_TRACE_IN_23_transactor->TREADY(trace_tready23);
  mp_TRACE_IN_23_transactor->TVALID(trace_tvalid23);
  mp_TRACE_IN_23_transactor->CLK(trace_clk);
  mp_TRACE_IN_23_transactor->RST(trace_aresetn);
  // configure TRACE_IN_24_transactor
    xsc::common_cpp::properties TRACE_IN_24_transactor_param_props;
    TRACE_IN_24_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_24_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_24_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_24_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_24_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_24_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_24_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_24_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_24_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_24_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_24_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_24_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_24_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_24_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_24_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_24_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_24_transactor", TRACE_IN_24_transactor_param_props);
  mp_TRACE_IN_24_transactor->TDATA(trace_tdata24);
  mp_TRACE_IN_24_transactor->TDEST(trace_tdest24);
  mp_TRACE_IN_24_transactor->TID(trace_tid24);
  mp_TRACE_IN_24_transactor->TLAST(trace_tlast24);
  mp_TRACE_IN_24_transactor->TREADY(trace_tready24);
  mp_TRACE_IN_24_transactor->TVALID(trace_tvalid24);
  mp_TRACE_IN_24_transactor->CLK(trace_clk);
  mp_TRACE_IN_24_transactor->RST(trace_aresetn);
  // configure TRACE_IN_25_transactor
    xsc::common_cpp::properties TRACE_IN_25_transactor_param_props;
    TRACE_IN_25_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_25_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_25_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_25_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_25_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_25_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_25_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_25_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_25_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_25_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_25_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_25_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_25_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_25_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_25_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_25_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_25_transactor", TRACE_IN_25_transactor_param_props);
  mp_TRACE_IN_25_transactor->TDATA(trace_tdata25);
  mp_TRACE_IN_25_transactor->TDEST(trace_tdest25);
  mp_TRACE_IN_25_transactor->TID(trace_tid25);
  mp_TRACE_IN_25_transactor->TLAST(trace_tlast25);
  mp_TRACE_IN_25_transactor->TREADY(trace_tready25);
  mp_TRACE_IN_25_transactor->TVALID(trace_tvalid25);
  mp_TRACE_IN_25_transactor->CLK(trace_clk);
  mp_TRACE_IN_25_transactor->RST(trace_aresetn);
  // configure TRACE_IN_26_transactor
    xsc::common_cpp::properties TRACE_IN_26_transactor_param_props;
    TRACE_IN_26_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_26_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_26_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_26_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_26_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_26_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_26_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_26_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_26_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_26_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_26_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_26_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_26_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_26_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_26_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_26_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_26_transactor", TRACE_IN_26_transactor_param_props);
  mp_TRACE_IN_26_transactor->TDATA(trace_tdata26);
  mp_TRACE_IN_26_transactor->TDEST(trace_tdest26);
  mp_TRACE_IN_26_transactor->TID(trace_tid26);
  mp_TRACE_IN_26_transactor->TLAST(trace_tlast26);
  mp_TRACE_IN_26_transactor->TREADY(trace_tready26);
  mp_TRACE_IN_26_transactor->TVALID(trace_tvalid26);
  mp_TRACE_IN_26_transactor->CLK(trace_clk);
  mp_TRACE_IN_26_transactor->RST(trace_aresetn);
  // configure TRACE_IN_27_transactor
    xsc::common_cpp::properties TRACE_IN_27_transactor_param_props;
    TRACE_IN_27_transactor_param_props.addLong("TDATA_NUM_BYTES", "1");
    TRACE_IN_27_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_27_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_27_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_27_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_27_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_27_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_27_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_27_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_27_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_27_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_27_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_27_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_27_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_27_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_27_transactor = new xtlm::xaxis_pin2xtlm_t<1,1,8,8,1,1>("TRACE_IN_27_transactor", TRACE_IN_27_transactor_param_props);
  mp_TRACE_IN_27_transactor->TDATA(trace_tdata27);
  mp_TRACE_IN_27_transactor->TDEST(trace_tdest27);
  mp_TRACE_IN_27_transactor->TID(trace_tid27);
  mp_TRACE_IN_27_transactor->TLAST(trace_tlast27);
  mp_TRACE_IN_27_transactor->TREADY(trace_tready27);
  mp_TRACE_IN_27_transactor->TVALID(trace_tvalid27);
  mp_TRACE_IN_27_transactor->CLK(trace_clk);
  mp_TRACE_IN_27_transactor->RST(trace_aresetn);
  // configure TRACE_IN_28_transactor
    xsc::common_cpp::properties TRACE_IN_28_transactor_param_props;
    TRACE_IN_28_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_28_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_28_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_28_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_28_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_28_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_28_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_28_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_28_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_28_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_28_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_28_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_28_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_28_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_28_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_28_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_28_transactor", TRACE_IN_28_transactor_param_props);
  mp_TRACE_IN_28_transactor->TDATA(trace_tdata28);
  mp_TRACE_IN_28_transactor->TDEST(trace_tdest28);
  mp_TRACE_IN_28_transactor->TID(trace_tid28);
  mp_TRACE_IN_28_transactor->TLAST(trace_tlast28);
  mp_TRACE_IN_28_transactor->TREADY(trace_tready28);
  mp_TRACE_IN_28_transactor->TVALID(trace_tvalid28);
  mp_TRACE_IN_28_transactor->CLK(trace_clk);
  mp_TRACE_IN_28_transactor->RST(trace_aresetn);
  // configure TRACE_IN_29_transactor
    xsc::common_cpp::properties TRACE_IN_29_transactor_param_props;
    TRACE_IN_29_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_29_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_29_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_29_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_29_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_29_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_29_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_29_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_29_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_29_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_29_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_29_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_29_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_29_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_29_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_29_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_29_transactor", TRACE_IN_29_transactor_param_props);
  mp_TRACE_IN_29_transactor->TDATA(trace_tdata29);
  mp_TRACE_IN_29_transactor->TDEST(trace_tdest29);
  mp_TRACE_IN_29_transactor->TID(trace_tid29);
  mp_TRACE_IN_29_transactor->TLAST(trace_tlast29);
  mp_TRACE_IN_29_transactor->TREADY(trace_tready29);
  mp_TRACE_IN_29_transactor->TVALID(trace_tvalid29);
  mp_TRACE_IN_29_transactor->CLK(trace_clk);
  mp_TRACE_IN_29_transactor->RST(trace_aresetn);
  // configure TRACE_IN_30_transactor
    xsc::common_cpp::properties TRACE_IN_30_transactor_param_props;
    TRACE_IN_30_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_30_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_30_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_30_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_30_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_30_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_30_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_30_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_30_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_30_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_30_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_30_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_30_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_30_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_30_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_30_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_30_transactor", TRACE_IN_30_transactor_param_props);
  mp_TRACE_IN_30_transactor->TDATA(trace_tdata30);
  mp_TRACE_IN_30_transactor->TDEST(trace_tdest30);
  mp_TRACE_IN_30_transactor->TID(trace_tid30);
  mp_TRACE_IN_30_transactor->TLAST(trace_tlast30);
  mp_TRACE_IN_30_transactor->TREADY(trace_tready30);
  mp_TRACE_IN_30_transactor->TVALID(trace_tvalid30);
  mp_TRACE_IN_30_transactor->CLK(trace_clk);
  mp_TRACE_IN_30_transactor->RST(trace_aresetn);
  // configure TRACE_IN_31_transactor
    xsc::common_cpp::properties TRACE_IN_31_transactor_param_props;
    TRACE_IN_31_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_31_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_31_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_31_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_31_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_31_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_31_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_31_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_31_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_31_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_31_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_31_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_31_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_31_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_31_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_31_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_31_transactor", TRACE_IN_31_transactor_param_props);
  mp_TRACE_IN_31_transactor->TDATA(trace_tdata31);
  mp_TRACE_IN_31_transactor->TDEST(trace_tdest31);
  mp_TRACE_IN_31_transactor->TID(trace_tid31);
  mp_TRACE_IN_31_transactor->TLAST(trace_tlast31);
  mp_TRACE_IN_31_transactor->TREADY(trace_tready31);
  mp_TRACE_IN_31_transactor->TVALID(trace_tvalid31);
  mp_TRACE_IN_31_transactor->CLK(trace_clk);
  mp_TRACE_IN_31_transactor->RST(trace_aresetn);
  // configure TRACE_IN_32_transactor
    xsc::common_cpp::properties TRACE_IN_32_transactor_param_props;
    TRACE_IN_32_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_32_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_32_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_32_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_32_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_32_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_32_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_32_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_32_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_32_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_32_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_32_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_32_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_32_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_32_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_32_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_32_transactor", TRACE_IN_32_transactor_param_props);
  mp_TRACE_IN_32_transactor->TDATA(trace_tdata32);
  mp_TRACE_IN_32_transactor->TDEST(trace_tdest32);
  mp_TRACE_IN_32_transactor->TID(trace_tid32);
  mp_TRACE_IN_32_transactor->TLAST(trace_tlast32);
  mp_TRACE_IN_32_transactor->TREADY(trace_tready32);
  mp_TRACE_IN_32_transactor->TVALID(trace_tvalid32);
  mp_TRACE_IN_32_transactor->CLK(trace_clk);
  mp_TRACE_IN_32_transactor->RST(trace_aresetn);
  // configure TRACE_IN_33_transactor
    xsc::common_cpp::properties TRACE_IN_33_transactor_param_props;
    TRACE_IN_33_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_33_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_33_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_33_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_33_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_33_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_33_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_33_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_33_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_33_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_33_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_33_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_33_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_33_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_33_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_33_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_33_transactor", TRACE_IN_33_transactor_param_props);
  mp_TRACE_IN_33_transactor->TDATA(trace_tdata33);
  mp_TRACE_IN_33_transactor->TDEST(trace_tdest33);
  mp_TRACE_IN_33_transactor->TID(trace_tid33);
  mp_TRACE_IN_33_transactor->TLAST(trace_tlast33);
  mp_TRACE_IN_33_transactor->TREADY(trace_tready33);
  mp_TRACE_IN_33_transactor->TVALID(trace_tvalid33);
  mp_TRACE_IN_33_transactor->CLK(trace_clk);
  mp_TRACE_IN_33_transactor->RST(trace_aresetn);
  // configure TRACE_IN_34_transactor
    xsc::common_cpp::properties TRACE_IN_34_transactor_param_props;
    TRACE_IN_34_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_34_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_34_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_34_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_34_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_34_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_34_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_34_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_34_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_34_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_34_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_34_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_34_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_34_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_34_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_34_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_34_transactor", TRACE_IN_34_transactor_param_props);
  mp_TRACE_IN_34_transactor->TDATA(trace_tdata34);
  mp_TRACE_IN_34_transactor->TDEST(trace_tdest34);
  mp_TRACE_IN_34_transactor->TID(trace_tid34);
  mp_TRACE_IN_34_transactor->TLAST(trace_tlast34);
  mp_TRACE_IN_34_transactor->TREADY(trace_tready34);
  mp_TRACE_IN_34_transactor->TVALID(trace_tvalid34);
  mp_TRACE_IN_34_transactor->CLK(trace_clk);
  mp_TRACE_IN_34_transactor->RST(trace_aresetn);
  // configure TRACE_IN_35_transactor
    xsc::common_cpp::properties TRACE_IN_35_transactor_param_props;
    TRACE_IN_35_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_35_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_35_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_35_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_35_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_35_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_35_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_35_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_35_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_35_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_35_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_35_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_35_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_35_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_35_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_35_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_35_transactor", TRACE_IN_35_transactor_param_props);
  mp_TRACE_IN_35_transactor->TDATA(trace_tdata35);
  mp_TRACE_IN_35_transactor->TDEST(trace_tdest35);
  mp_TRACE_IN_35_transactor->TID(trace_tid35);
  mp_TRACE_IN_35_transactor->TLAST(trace_tlast35);
  mp_TRACE_IN_35_transactor->TREADY(trace_tready35);
  mp_TRACE_IN_35_transactor->TVALID(trace_tvalid35);
  mp_TRACE_IN_35_transactor->CLK(trace_clk);
  mp_TRACE_IN_35_transactor->RST(trace_aresetn);
  // configure TRACE_IN_36_transactor
    xsc::common_cpp::properties TRACE_IN_36_transactor_param_props;
    TRACE_IN_36_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_36_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_36_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_36_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_36_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_36_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_36_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_36_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_36_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_36_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_36_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_36_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_36_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_36_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_36_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_36_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_36_transactor", TRACE_IN_36_transactor_param_props);
  mp_TRACE_IN_36_transactor->TDATA(trace_tdata36);
  mp_TRACE_IN_36_transactor->TDEST(trace_tdest36);
  mp_TRACE_IN_36_transactor->TID(trace_tid36);
  mp_TRACE_IN_36_transactor->TLAST(trace_tlast36);
  mp_TRACE_IN_36_transactor->TREADY(trace_tready36);
  mp_TRACE_IN_36_transactor->TVALID(trace_tvalid36);
  mp_TRACE_IN_36_transactor->CLK(trace_clk);
  mp_TRACE_IN_36_transactor->RST(trace_aresetn);
  // configure TRACE_IN_37_transactor
    xsc::common_cpp::properties TRACE_IN_37_transactor_param_props;
    TRACE_IN_37_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_37_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_37_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_37_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_37_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_37_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_37_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_37_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_37_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_37_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_37_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_37_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_37_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_37_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_37_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_37_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_37_transactor", TRACE_IN_37_transactor_param_props);
  mp_TRACE_IN_37_transactor->TDATA(trace_tdata37);
  mp_TRACE_IN_37_transactor->TDEST(trace_tdest37);
  mp_TRACE_IN_37_transactor->TID(trace_tid37);
  mp_TRACE_IN_37_transactor->TLAST(trace_tlast37);
  mp_TRACE_IN_37_transactor->TREADY(trace_tready37);
  mp_TRACE_IN_37_transactor->TVALID(trace_tvalid37);
  mp_TRACE_IN_37_transactor->CLK(trace_clk);
  mp_TRACE_IN_37_transactor->RST(trace_aresetn);
  // configure TRACE_IN_38_transactor
    xsc::common_cpp::properties TRACE_IN_38_transactor_param_props;
    TRACE_IN_38_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_38_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_38_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_38_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_38_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_38_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_38_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_38_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_38_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_38_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_38_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_38_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_38_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_38_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_38_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_38_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_38_transactor", TRACE_IN_38_transactor_param_props);
  mp_TRACE_IN_38_transactor->TDATA(trace_tdata38);
  mp_TRACE_IN_38_transactor->TDEST(trace_tdest38);
  mp_TRACE_IN_38_transactor->TID(trace_tid38);
  mp_TRACE_IN_38_transactor->TLAST(trace_tlast38);
  mp_TRACE_IN_38_transactor->TREADY(trace_tready38);
  mp_TRACE_IN_38_transactor->TVALID(trace_tvalid38);
  mp_TRACE_IN_38_transactor->CLK(trace_clk);
  mp_TRACE_IN_38_transactor->RST(trace_aresetn);
  // configure TRACE_IN_39_transactor
    xsc::common_cpp::properties TRACE_IN_39_transactor_param_props;
    TRACE_IN_39_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_39_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_39_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_39_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_39_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_39_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_39_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_39_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_39_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_39_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_39_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_39_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_39_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_39_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_39_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_39_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_39_transactor", TRACE_IN_39_transactor_param_props);
  mp_TRACE_IN_39_transactor->TDATA(trace_tdata39);
  mp_TRACE_IN_39_transactor->TDEST(trace_tdest39);
  mp_TRACE_IN_39_transactor->TID(trace_tid39);
  mp_TRACE_IN_39_transactor->TLAST(trace_tlast39);
  mp_TRACE_IN_39_transactor->TREADY(trace_tready39);
  mp_TRACE_IN_39_transactor->TVALID(trace_tvalid39);
  mp_TRACE_IN_39_transactor->CLK(trace_clk);
  mp_TRACE_IN_39_transactor->RST(trace_aresetn);
  // configure TRACE_IN_40_transactor
    xsc::common_cpp::properties TRACE_IN_40_transactor_param_props;
    TRACE_IN_40_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_40_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_40_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_40_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_40_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_40_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_40_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_40_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_40_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_40_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_40_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_40_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_40_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_40_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_40_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_40_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_40_transactor", TRACE_IN_40_transactor_param_props);
  mp_TRACE_IN_40_transactor->TDATA(trace_tdata40);
  mp_TRACE_IN_40_transactor->TDEST(trace_tdest40);
  mp_TRACE_IN_40_transactor->TID(trace_tid40);
  mp_TRACE_IN_40_transactor->TLAST(trace_tlast40);
  mp_TRACE_IN_40_transactor->TREADY(trace_tready40);
  mp_TRACE_IN_40_transactor->TVALID(trace_tvalid40);
  mp_TRACE_IN_40_transactor->CLK(trace_clk);
  mp_TRACE_IN_40_transactor->RST(trace_aresetn);

  // initialize transactors stubs
  S_AXIMM_transactor_target_wr_socket_stub = nullptr;
  S_AXIMM_transactor_target_rd_socket_stub = nullptr;
  S_AXIFIFO_transactor_target_wr_socket_stub = nullptr;
  S_AXIFIFO_transactor_target_rd_socket_stub = nullptr;
  S_AXIHUB_transactor_target_wr_socket_stub = nullptr;
  S_AXIHUB_transactor_target_rd_socket_stub = nullptr;
  TRACE_IN_0_transactor_target_socket_stub = nullptr;
  TRACE_IN_1_transactor_target_socket_stub = nullptr;
  TRACE_IN_2_transactor_target_socket_stub = nullptr;
  TRACE_IN_3_transactor_target_socket_stub = nullptr;
  TRACE_IN_4_transactor_target_socket_stub = nullptr;
  TRACE_IN_5_transactor_target_socket_stub = nullptr;
  TRACE_IN_6_transactor_target_socket_stub = nullptr;
  TRACE_IN_7_transactor_target_socket_stub = nullptr;
  TRACE_IN_8_transactor_target_socket_stub = nullptr;
  TRACE_IN_9_transactor_target_socket_stub = nullptr;
  TRACE_IN_10_transactor_target_socket_stub = nullptr;
  TRACE_IN_11_transactor_target_socket_stub = nullptr;
  TRACE_IN_12_transactor_target_socket_stub = nullptr;
  TRACE_IN_13_transactor_target_socket_stub = nullptr;
  TRACE_IN_14_transactor_target_socket_stub = nullptr;
  TRACE_IN_15_transactor_target_socket_stub = nullptr;
  TRACE_IN_16_transactor_target_socket_stub = nullptr;
  TRACE_IN_17_transactor_target_socket_stub = nullptr;
  TRACE_IN_18_transactor_target_socket_stub = nullptr;
  TRACE_IN_19_transactor_target_socket_stub = nullptr;
  TRACE_IN_20_transactor_target_socket_stub = nullptr;
  TRACE_IN_21_transactor_target_socket_stub = nullptr;
  TRACE_IN_22_transactor_target_socket_stub = nullptr;
  TRACE_IN_23_transactor_target_socket_stub = nullptr;
  TRACE_IN_24_transactor_target_socket_stub = nullptr;
  TRACE_IN_25_transactor_target_socket_stub = nullptr;
  TRACE_IN_26_transactor_target_socket_stub = nullptr;
  TRACE_IN_27_transactor_target_socket_stub = nullptr;
  TRACE_IN_28_transactor_target_socket_stub = nullptr;
  TRACE_IN_29_transactor_target_socket_stub = nullptr;
  TRACE_IN_30_transactor_target_socket_stub = nullptr;
  TRACE_IN_31_transactor_target_socket_stub = nullptr;
  TRACE_IN_32_transactor_target_socket_stub = nullptr;
  TRACE_IN_33_transactor_target_socket_stub = nullptr;
  TRACE_IN_34_transactor_target_socket_stub = nullptr;
  TRACE_IN_35_transactor_target_socket_stub = nullptr;
  TRACE_IN_36_transactor_target_socket_stub = nullptr;
  TRACE_IN_37_transactor_target_socket_stub = nullptr;
  TRACE_IN_38_transactor_target_socket_stub = nullptr;
  TRACE_IN_39_transactor_target_socket_stub = nullptr;
  TRACE_IN_40_transactor_target_socket_stub = nullptr;

}

void pfm_dynamic_dpa_hub_0::before_end_of_elaboration()
{
  // configure 'S_AXIMM' transactor
  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "S_AXIMM_TLM_MODE") != 1)
  {
    mp_impl->s_aximm_rd_socket->bind(*(mp_S_AXIMM_transactor->rd_socket));
    mp_impl->s_aximm_wr_socket->bind(*(mp_S_AXIMM_transactor->wr_socket));
  
  }
  else
  {
    S_AXIMM_transactor_target_wr_socket_stub = new xtlm::xtlm_aximm_target_stub("wr_socket",0);
    S_AXIMM_transactor_target_wr_socket_stub->bind(*(mp_S_AXIMM_transactor->wr_socket));
    S_AXIMM_transactor_target_rd_socket_stub = new xtlm::xtlm_aximm_target_stub("rd_socket",0);
    S_AXIMM_transactor_target_rd_socket_stub->bind(*(mp_S_AXIMM_transactor->rd_socket));
    mp_S_AXIMM_transactor->disable_transactor();
  }

  // configure 'S_AXIFIFO' transactor
  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "S_AXIFIFO_TLM_MODE") != 1)
  {
    mp_impl->axilite_rd_socket->bind(*(mp_S_AXIFIFO_transactor->rd_socket));
    mp_impl->axilite_wr_socket->bind(*(mp_S_AXIFIFO_transactor->wr_socket));
  
  }
  else
  {
    S_AXIFIFO_transactor_target_wr_socket_stub = new xtlm::xtlm_aximm_target_stub("wr_socket",0);
    S_AXIFIFO_transactor_target_wr_socket_stub->bind(*(mp_S_AXIFIFO_transactor->wr_socket));
    S_AXIFIFO_transactor_target_rd_socket_stub = new xtlm::xtlm_aximm_target_stub("rd_socket",0);
    S_AXIFIFO_transactor_target_rd_socket_stub->bind(*(mp_S_AXIFIFO_transactor->rd_socket));
    mp_S_AXIFIFO_transactor->disable_transactor();
  }

  // configure 'S_AXIHUB' transactor
  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "S_AXIHUB_TLM_MODE") != 1)
  {
    mp_impl->s_axihub_rd_socket->bind(*(mp_S_AXIHUB_transactor->rd_socket));
    mp_impl->s_axihub_wr_socket->bind(*(mp_S_AXIHUB_transactor->wr_socket));
  
  }
  else
  {
    S_AXIHUB_transactor_target_wr_socket_stub = new xtlm::xtlm_aximm_target_stub("wr_socket",0);
    S_AXIHUB_transactor_target_wr_socket_stub->bind(*(mp_S_AXIHUB_transactor->wr_socket));
    S_AXIHUB_transactor_target_rd_socket_stub = new xtlm::xtlm_aximm_target_stub("rd_socket",0);
    S_AXIHUB_transactor_target_rd_socket_stub->bind(*(mp_S_AXIHUB_transactor->rd_socket));
    mp_S_AXIHUB_transactor->disable_transactor();
  }

  // configure 'TRACE_IN_0' transactor
  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_0_TLM_MODE") != 1)
  {
    mp_impl->trace_socket0->bind(*(mp_TRACE_IN_0_transactor->socket));
  
  }
  else
  {
    TRACE_IN_0_transactor_target_socket_stub = new xtlm::xtlm_axis_target_stub("socket",0);
    TRACE_IN_0_transactor_target_socket_stub->bind(*(mp_TRACE_IN_0_transactor->socket));
    mp_TRACE_IN_0_transactor->disable_transactor();
  }

  // configure 'TRACE_IN_1' transactor
  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_1_TLM_MODE") != 1)
  {
    mp_impl->trace_socket1->bind(*(mp_TRACE_IN_1_transactor->socket));
  
  }
  else
  {
    TRACE_IN_1_transactor_target_socket_stub = new xtlm::xtlm_axis_target_stub("socket",0);
    TRACE_IN_1_transactor_target_socket_stub->bind(*(mp_TRACE_IN_1_transactor->socket));
    mp_TRACE_IN_1_transactor->disable_transactor();
  }

  // configure 'TRACE_IN_2' transactor
  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_2_TLM_MODE") != 1)
  {
    mp_impl->trace_socket2->bind(*(mp_TRACE_IN_2_transactor->socket));
  
  }
  else
  {
    TRACE_IN_2_transactor_target_socket_stub = new xtlm::xtlm_axis_target_stub("socket",0);
    TRACE_IN_2_transactor_target_socket_stub->bind(*(mp_TRACE_IN_2_transactor->socket));
    mp_TRACE_IN_2_transactor->disable_transactor();
  }

  // configure 'TRACE_IN_3' transactor
  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_3_TLM_MODE") != 1)
  {
    mp_impl->trace_socket3->bind(*(mp_TRACE_IN_3_transactor->socket));
  
  }
  else
  {
    TRACE_IN_3_transactor_target_socket_stub = new xtlm::xtlm_axis_target_stub("socket",0);
    TRACE_IN_3_transactor_target_socket_stub->bind(*(mp_TRACE_IN_3_transactor->socket));
    mp_TRACE_IN_3_transactor->disable_transactor();
  }

  // configure 'TRACE_IN_4' transactor
  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_4_TLM_MODE") != 1)
  {
    mp_impl->trace_socket4->bind(*(mp_TRACE_IN_4_transactor->socket));
  
  }
  else
  {
    TRACE_IN_4_transactor_target_socket_stub = new xtlm::xtlm_axis_target_stub("socket",0);
    TRACE_IN_4_transactor_target_socket_stub->bind(*(mp_TRACE_IN_4_transactor->socket));
    mp_TRACE_IN_4_transactor->disable_transactor();
  }

  // configure 'TRACE_IN_5' transactor
  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_5_TLM_MODE") != 1)
  {
    mp_impl->trace_socket5->bind(*(mp_TRACE_IN_5_transactor->socket));
  
  }
  else
  {
    TRACE_IN_5_transactor_target_socket_stub = new xtlm::xtlm_axis_target_stub("socket",0);
    TRACE_IN_5_transactor_target_socket_stub->bind(*(mp_TRACE_IN_5_transactor->socket));
    mp_TRACE_IN_5_transactor->disable_transactor();
  }

  // configure 'TRACE_IN_6' transactor
  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_6_TLM_MODE") != 1)
  {
    mp_impl->trace_socket6->bind(*(mp_TRACE_IN_6_transactor->socket));
  
  }
  else
  {
    TRACE_IN_6_transactor_target_socket_stub = new xtlm::xtlm_axis_target_stub("socket",0);
    TRACE_IN_6_transactor_target_socket_stub->bind(*(mp_TRACE_IN_6_transactor->socket));
    mp_TRACE_IN_6_transactor->disable_transactor();
  }

  // configure 'TRACE_IN_7' transactor
  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_7_TLM_MODE") != 1)
  {
    mp_impl->trace_socket7->bind(*(mp_TRACE_IN_7_transactor->socket));
  
  }
  else
  {
    TRACE_IN_7_transactor_target_socket_stub = new xtlm::xtlm_axis_target_stub("socket",0);
    TRACE_IN_7_transactor_target_socket_stub->bind(*(mp_TRACE_IN_7_transactor->socket));
    mp_TRACE_IN_7_transactor->disable_transactor();
  }

  // configure 'TRACE_IN_8' transactor
  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_8_TLM_MODE") != 1)
  {
    mp_impl->trace_socket8->bind(*(mp_TRACE_IN_8_transactor->socket));
  
  }
  else
  {
    TRACE_IN_8_transactor_target_socket_stub = new xtlm::xtlm_axis_target_stub("socket",0);
    TRACE_IN_8_transactor_target_socket_stub->bind(*(mp_TRACE_IN_8_transactor->socket));
    mp_TRACE_IN_8_transactor->disable_transactor();
  }

  // configure 'TRACE_IN_9' transactor
  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_9_TLM_MODE") != 1)
  {
    mp_impl->trace_socket9->bind(*(mp_TRACE_IN_9_transactor->socket));
  
  }
  else
  {
    TRACE_IN_9_transactor_target_socket_stub = new xtlm::xtlm_axis_target_stub("socket",0);
    TRACE_IN_9_transactor_target_socket_stub->bind(*(mp_TRACE_IN_9_transactor->socket));
    mp_TRACE_IN_9_transactor->disable_transactor();
  }

  // configure 'TRACE_IN_10' transactor
  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_10_TLM_MODE") != 1)
  {
    mp_impl->trace_socket10->bind(*(mp_TRACE_IN_10_transactor->socket));
  
  }
  else
  {
    TRACE_IN_10_transactor_target_socket_stub = new xtlm::xtlm_axis_target_stub("socket",0);
    TRACE_IN_10_transactor_target_socket_stub->bind(*(mp_TRACE_IN_10_transactor->socket));
    mp_TRACE_IN_10_transactor->disable_transactor();
  }

  // configure 'TRACE_IN_11' transactor
  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_11_TLM_MODE") != 1)
  {
    mp_impl->trace_socket11->bind(*(mp_TRACE_IN_11_transactor->socket));
  
  }
  else
  {
    TRACE_IN_11_transactor_target_socket_stub = new xtlm::xtlm_axis_target_stub("socket",0);
    TRACE_IN_11_transactor_target_socket_stub->bind(*(mp_TRACE_IN_11_transactor->socket));
    mp_TRACE_IN_11_transactor->disable_transactor();
  }

  // configure 'TRACE_IN_12' transactor
  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_12_TLM_MODE") != 1)
  {
    mp_impl->trace_socket12->bind(*(mp_TRACE_IN_12_transactor->socket));
  
  }
  else
  {
    TRACE_IN_12_transactor_target_socket_stub = new xtlm::xtlm_axis_target_stub("socket",0);
    TRACE_IN_12_transactor_target_socket_stub->bind(*(mp_TRACE_IN_12_transactor->socket));
    mp_TRACE_IN_12_transactor->disable_transactor();
  }

  // configure 'TRACE_IN_13' transactor
  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_13_TLM_MODE") != 1)
  {
    mp_impl->trace_socket13->bind(*(mp_TRACE_IN_13_transactor->socket));
  
  }
  else
  {
    TRACE_IN_13_transactor_target_socket_stub = new xtlm::xtlm_axis_target_stub("socket",0);
    TRACE_IN_13_transactor_target_socket_stub->bind(*(mp_TRACE_IN_13_transactor->socket));
    mp_TRACE_IN_13_transactor->disable_transactor();
  }

  // configure 'TRACE_IN_14' transactor
  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_14_TLM_MODE") != 1)
  {
    mp_impl->trace_socket14->bind(*(mp_TRACE_IN_14_transactor->socket));
  
  }
  else
  {
    TRACE_IN_14_transactor_target_socket_stub = new xtlm::xtlm_axis_target_stub("socket",0);
    TRACE_IN_14_transactor_target_socket_stub->bind(*(mp_TRACE_IN_14_transactor->socket));
    mp_TRACE_IN_14_transactor->disable_transactor();
  }

  // configure 'TRACE_IN_15' transactor
  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_15_TLM_MODE") != 1)
  {
    mp_impl->trace_socket15->bind(*(mp_TRACE_IN_15_transactor->socket));
  
  }
  else
  {
    TRACE_IN_15_transactor_target_socket_stub = new xtlm::xtlm_axis_target_stub("socket",0);
    TRACE_IN_15_transactor_target_socket_stub->bind(*(mp_TRACE_IN_15_transactor->socket));
    mp_TRACE_IN_15_transactor->disable_transactor();
  }

  // configure 'TRACE_IN_16' transactor
  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_16_TLM_MODE") != 1)
  {
    mp_impl->trace_socket16->bind(*(mp_TRACE_IN_16_transactor->socket));
  
  }
  else
  {
    TRACE_IN_16_transactor_target_socket_stub = new xtlm::xtlm_axis_target_stub("socket",0);
    TRACE_IN_16_transactor_target_socket_stub->bind(*(mp_TRACE_IN_16_transactor->socket));
    mp_TRACE_IN_16_transactor->disable_transactor();
  }

  // configure 'TRACE_IN_17' transactor
  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_17_TLM_MODE") != 1)
  {
    mp_impl->trace_socket17->bind(*(mp_TRACE_IN_17_transactor->socket));
  
  }
  else
  {
    TRACE_IN_17_transactor_target_socket_stub = new xtlm::xtlm_axis_target_stub("socket",0);
    TRACE_IN_17_transactor_target_socket_stub->bind(*(mp_TRACE_IN_17_transactor->socket));
    mp_TRACE_IN_17_transactor->disable_transactor();
  }

  // configure 'TRACE_IN_18' transactor
  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_18_TLM_MODE") != 1)
  {
    mp_impl->trace_socket18->bind(*(mp_TRACE_IN_18_transactor->socket));
  
  }
  else
  {
    TRACE_IN_18_transactor_target_socket_stub = new xtlm::xtlm_axis_target_stub("socket",0);
    TRACE_IN_18_transactor_target_socket_stub->bind(*(mp_TRACE_IN_18_transactor->socket));
    mp_TRACE_IN_18_transactor->disable_transactor();
  }

  // configure 'TRACE_IN_19' transactor
  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_19_TLM_MODE") != 1)
  {
    mp_impl->trace_socket19->bind(*(mp_TRACE_IN_19_transactor->socket));
  
  }
  else
  {
    TRACE_IN_19_transactor_target_socket_stub = new xtlm::xtlm_axis_target_stub("socket",0);
    TRACE_IN_19_transactor_target_socket_stub->bind(*(mp_TRACE_IN_19_transactor->socket));
    mp_TRACE_IN_19_transactor->disable_transactor();
  }

  // configure 'TRACE_IN_20' transactor
  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_20_TLM_MODE") != 1)
  {
    mp_impl->trace_socket20->bind(*(mp_TRACE_IN_20_transactor->socket));
  
  }
  else
  {
    TRACE_IN_20_transactor_target_socket_stub = new xtlm::xtlm_axis_target_stub("socket",0);
    TRACE_IN_20_transactor_target_socket_stub->bind(*(mp_TRACE_IN_20_transactor->socket));
    mp_TRACE_IN_20_transactor->disable_transactor();
  }

  // configure 'TRACE_IN_21' transactor
  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_21_TLM_MODE") != 1)
  {
    mp_impl->trace_socket21->bind(*(mp_TRACE_IN_21_transactor->socket));
  
  }
  else
  {
    TRACE_IN_21_transactor_target_socket_stub = new xtlm::xtlm_axis_target_stub("socket",0);
    TRACE_IN_21_transactor_target_socket_stub->bind(*(mp_TRACE_IN_21_transactor->socket));
    mp_TRACE_IN_21_transactor->disable_transactor();
  }

  // configure 'TRACE_IN_22' transactor
  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_22_TLM_MODE") != 1)
  {
    mp_impl->trace_socket22->bind(*(mp_TRACE_IN_22_transactor->socket));
  
  }
  else
  {
    TRACE_IN_22_transactor_target_socket_stub = new xtlm::xtlm_axis_target_stub("socket",0);
    TRACE_IN_22_transactor_target_socket_stub->bind(*(mp_TRACE_IN_22_transactor->socket));
    mp_TRACE_IN_22_transactor->disable_transactor();
  }

  // configure 'TRACE_IN_23' transactor
  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_23_TLM_MODE") != 1)
  {
    mp_impl->trace_socket23->bind(*(mp_TRACE_IN_23_transactor->socket));
  
  }
  else
  {
    TRACE_IN_23_transactor_target_socket_stub = new xtlm::xtlm_axis_target_stub("socket",0);
    TRACE_IN_23_transactor_target_socket_stub->bind(*(mp_TRACE_IN_23_transactor->socket));
    mp_TRACE_IN_23_transactor->disable_transactor();
  }

  // configure 'TRACE_IN_24' transactor
  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_24_TLM_MODE") != 1)
  {
    mp_impl->trace_socket24->bind(*(mp_TRACE_IN_24_transactor->socket));
  
  }
  else
  {
    TRACE_IN_24_transactor_target_socket_stub = new xtlm::xtlm_axis_target_stub("socket",0);
    TRACE_IN_24_transactor_target_socket_stub->bind(*(mp_TRACE_IN_24_transactor->socket));
    mp_TRACE_IN_24_transactor->disable_transactor();
  }

  // configure 'TRACE_IN_25' transactor
  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_25_TLM_MODE") != 1)
  {
    mp_impl->trace_socket25->bind(*(mp_TRACE_IN_25_transactor->socket));
  
  }
  else
  {
    TRACE_IN_25_transactor_target_socket_stub = new xtlm::xtlm_axis_target_stub("socket",0);
    TRACE_IN_25_transactor_target_socket_stub->bind(*(mp_TRACE_IN_25_transactor->socket));
    mp_TRACE_IN_25_transactor->disable_transactor();
  }

  // configure 'TRACE_IN_26' transactor
  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_26_TLM_MODE") != 1)
  {
    mp_impl->trace_socket26->bind(*(mp_TRACE_IN_26_transactor->socket));
  
  }
  else
  {
    TRACE_IN_26_transactor_target_socket_stub = new xtlm::xtlm_axis_target_stub("socket",0);
    TRACE_IN_26_transactor_target_socket_stub->bind(*(mp_TRACE_IN_26_transactor->socket));
    mp_TRACE_IN_26_transactor->disable_transactor();
  }

  // configure 'TRACE_IN_27' transactor
  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_27_TLM_MODE") != 1)
  {
    mp_impl->trace_socket27->bind(*(mp_TRACE_IN_27_transactor->socket));
  
  }
  else
  {
    TRACE_IN_27_transactor_target_socket_stub = new xtlm::xtlm_axis_target_stub("socket",0);
    TRACE_IN_27_transactor_target_socket_stub->bind(*(mp_TRACE_IN_27_transactor->socket));
    mp_TRACE_IN_27_transactor->disable_transactor();
  }

  // configure 'TRACE_IN_28' transactor
  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_28_TLM_MODE") != 1)
  {
    mp_impl->trace_socket28->bind(*(mp_TRACE_IN_28_transactor->socket));
  
  }
  else
  {
    TRACE_IN_28_transactor_target_socket_stub = new xtlm::xtlm_axis_target_stub("socket",0);
    TRACE_IN_28_transactor_target_socket_stub->bind(*(mp_TRACE_IN_28_transactor->socket));
    mp_TRACE_IN_28_transactor->disable_transactor();
  }

  // configure 'TRACE_IN_29' transactor
  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_29_TLM_MODE") != 1)
  {
    mp_impl->trace_socket29->bind(*(mp_TRACE_IN_29_transactor->socket));
  
  }
  else
  {
    TRACE_IN_29_transactor_target_socket_stub = new xtlm::xtlm_axis_target_stub("socket",0);
    TRACE_IN_29_transactor_target_socket_stub->bind(*(mp_TRACE_IN_29_transactor->socket));
    mp_TRACE_IN_29_transactor->disable_transactor();
  }

  // configure 'TRACE_IN_30' transactor
  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_30_TLM_MODE") != 1)
  {
    mp_impl->trace_socket30->bind(*(mp_TRACE_IN_30_transactor->socket));
  
  }
  else
  {
    TRACE_IN_30_transactor_target_socket_stub = new xtlm::xtlm_axis_target_stub("socket",0);
    TRACE_IN_30_transactor_target_socket_stub->bind(*(mp_TRACE_IN_30_transactor->socket));
    mp_TRACE_IN_30_transactor->disable_transactor();
  }

  // configure 'TRACE_IN_31' transactor
  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_31_TLM_MODE") != 1)
  {
    mp_impl->trace_socket31->bind(*(mp_TRACE_IN_31_transactor->socket));
  
  }
  else
  {
    TRACE_IN_31_transactor_target_socket_stub = new xtlm::xtlm_axis_target_stub("socket",0);
    TRACE_IN_31_transactor_target_socket_stub->bind(*(mp_TRACE_IN_31_transactor->socket));
    mp_TRACE_IN_31_transactor->disable_transactor();
  }

  // configure 'TRACE_IN_32' transactor
  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_32_TLM_MODE") != 1)
  {
    mp_impl->trace_socket32->bind(*(mp_TRACE_IN_32_transactor->socket));
  
  }
  else
  {
    TRACE_IN_32_transactor_target_socket_stub = new xtlm::xtlm_axis_target_stub("socket",0);
    TRACE_IN_32_transactor_target_socket_stub->bind(*(mp_TRACE_IN_32_transactor->socket));
    mp_TRACE_IN_32_transactor->disable_transactor();
  }

  // configure 'TRACE_IN_33' transactor
  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_33_TLM_MODE") != 1)
  {
    mp_impl->trace_socket33->bind(*(mp_TRACE_IN_33_transactor->socket));
  
  }
  else
  {
    TRACE_IN_33_transactor_target_socket_stub = new xtlm::xtlm_axis_target_stub("socket",0);
    TRACE_IN_33_transactor_target_socket_stub->bind(*(mp_TRACE_IN_33_transactor->socket));
    mp_TRACE_IN_33_transactor->disable_transactor();
  }

  // configure 'TRACE_IN_34' transactor
  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_34_TLM_MODE") != 1)
  {
    mp_impl->trace_socket34->bind(*(mp_TRACE_IN_34_transactor->socket));
  
  }
  else
  {
    TRACE_IN_34_transactor_target_socket_stub = new xtlm::xtlm_axis_target_stub("socket",0);
    TRACE_IN_34_transactor_target_socket_stub->bind(*(mp_TRACE_IN_34_transactor->socket));
    mp_TRACE_IN_34_transactor->disable_transactor();
  }

  // configure 'TRACE_IN_35' transactor
  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_35_TLM_MODE") != 1)
  {
    mp_impl->trace_socket35->bind(*(mp_TRACE_IN_35_transactor->socket));
  
  }
  else
  {
    TRACE_IN_35_transactor_target_socket_stub = new xtlm::xtlm_axis_target_stub("socket",0);
    TRACE_IN_35_transactor_target_socket_stub->bind(*(mp_TRACE_IN_35_transactor->socket));
    mp_TRACE_IN_35_transactor->disable_transactor();
  }

  // configure 'TRACE_IN_36' transactor
  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_36_TLM_MODE") != 1)
  {
    mp_impl->trace_socket36->bind(*(mp_TRACE_IN_36_transactor->socket));
  
  }
  else
  {
    TRACE_IN_36_transactor_target_socket_stub = new xtlm::xtlm_axis_target_stub("socket",0);
    TRACE_IN_36_transactor_target_socket_stub->bind(*(mp_TRACE_IN_36_transactor->socket));
    mp_TRACE_IN_36_transactor->disable_transactor();
  }

  // configure 'TRACE_IN_37' transactor
  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_37_TLM_MODE") != 1)
  {
    mp_impl->trace_socket37->bind(*(mp_TRACE_IN_37_transactor->socket));
  
  }
  else
  {
    TRACE_IN_37_transactor_target_socket_stub = new xtlm::xtlm_axis_target_stub("socket",0);
    TRACE_IN_37_transactor_target_socket_stub->bind(*(mp_TRACE_IN_37_transactor->socket));
    mp_TRACE_IN_37_transactor->disable_transactor();
  }

  // configure 'TRACE_IN_38' transactor
  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_38_TLM_MODE") != 1)
  {
    mp_impl->trace_socket38->bind(*(mp_TRACE_IN_38_transactor->socket));
  
  }
  else
  {
    TRACE_IN_38_transactor_target_socket_stub = new xtlm::xtlm_axis_target_stub("socket",0);
    TRACE_IN_38_transactor_target_socket_stub->bind(*(mp_TRACE_IN_38_transactor->socket));
    mp_TRACE_IN_38_transactor->disable_transactor();
  }

  // configure 'TRACE_IN_39' transactor
  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_39_TLM_MODE") != 1)
  {
    mp_impl->trace_socket39->bind(*(mp_TRACE_IN_39_transactor->socket));
  
  }
  else
  {
    TRACE_IN_39_transactor_target_socket_stub = new xtlm::xtlm_axis_target_stub("socket",0);
    TRACE_IN_39_transactor_target_socket_stub->bind(*(mp_TRACE_IN_39_transactor->socket));
    mp_TRACE_IN_39_transactor->disable_transactor();
  }

  // configure 'TRACE_IN_40' transactor
  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_40_TLM_MODE") != 1)
  {
    mp_impl->trace_socket40->bind(*(mp_TRACE_IN_40_transactor->socket));
  
  }
  else
  {
    TRACE_IN_40_transactor_target_socket_stub = new xtlm::xtlm_axis_target_stub("socket",0);
    TRACE_IN_40_transactor_target_socket_stub->bind(*(mp_TRACE_IN_40_transactor->socket));
    mp_TRACE_IN_40_transactor->disable_transactor();
  }

}

#endif // VCSSYSTEMC




#ifdef MTI_SYSTEMC
pfm_dynamic_dpa_hub_0::pfm_dynamic_dpa_hub_0(const sc_core::sc_module_name& nm) : pfm_dynamic_dpa_hub_0_sc(nm), s_aximm_clk("s_aximm_clk"), s_aximm_aresetn("s_aximm_aresetn"), trace_clk("trace_clk"), trace_aresetn("trace_aresetn"), axilite_clk("axilite_clk"), axilite_aresetn("axilite_aresetn"), trace_tready0("trace_tready0"), trace_tdata0("trace_tdata0"), trace_tlast0("trace_tlast0"), trace_tid0("trace_tid0"), trace_tdest0("trace_tdest0"), trace_tvalid0("trace_tvalid0"), trace_tready1("trace_tready1"), trace_tdata1("trace_tdata1"), trace_tlast1("trace_tlast1"), trace_tid1("trace_tid1"), trace_tdest1("trace_tdest1"), trace_tvalid1("trace_tvalid1"), trace_tdata2("trace_tdata2"), trace_tready2("trace_tready2"), trace_tlast2("trace_tlast2"), trace_tid2("trace_tid2"), trace_tdest2("trace_tdest2"), trace_tvalid2("trace_tvalid2"), trace_tdata3("trace_tdata3"), trace_tready3("trace_tready3"), trace_tlast3("trace_tlast3"), trace_tid3("trace_tid3"), trace_tdest3("trace_tdest3"), trace_tvalid3("trace_tvalid3"), trace_tdata4("trace_tdata4"), trace_tready4("trace_tready4"), trace_tlast4("trace_tlast4"), trace_tid4("trace_tid4"), trace_tdest4("trace_tdest4"), trace_tvalid4("trace_tvalid4"), trace_tdata5("trace_tdata5"), trace_tready5("trace_tready5"), trace_tlast5("trace_tlast5"), trace_tid5("trace_tid5"), trace_tdest5("trace_tdest5"), trace_tvalid5("trace_tvalid5"), trace_tdata6("trace_tdata6"), trace_tready6("trace_tready6"), trace_tlast6("trace_tlast6"), trace_tid6("trace_tid6"), trace_tdest6("trace_tdest6"), trace_tvalid6("trace_tvalid6"), trace_tdata7("trace_tdata7"), trace_tready7("trace_tready7"), trace_tlast7("trace_tlast7"), trace_tid7("trace_tid7"), trace_tdest7("trace_tdest7"), trace_tvalid7("trace_tvalid7"), trace_tdata8("trace_tdata8"), trace_tready8("trace_tready8"), trace_tlast8("trace_tlast8"), trace_tid8("trace_tid8"), trace_tdest8("trace_tdest8"), trace_tvalid8("trace_tvalid8"), trace_tdata9("trace_tdata9"), trace_tready9("trace_tready9"), trace_tlast9("trace_tlast9"), trace_tid9("trace_tid9"), trace_tdest9("trace_tdest9"), trace_tvalid9("trace_tvalid9"), trace_tdata10("trace_tdata10"), trace_tready10("trace_tready10"), trace_tlast10("trace_tlast10"), trace_tid10("trace_tid10"), trace_tdest10("trace_tdest10"), trace_tvalid10("trace_tvalid10"), trace_tdata11("trace_tdata11"), trace_tready11("trace_tready11"), trace_tlast11("trace_tlast11"), trace_tid11("trace_tid11"), trace_tdest11("trace_tdest11"), trace_tvalid11("trace_tvalid11"), trace_tdata12("trace_tdata12"), trace_tready12("trace_tready12"), trace_tlast12("trace_tlast12"), trace_tid12("trace_tid12"), trace_tdest12("trace_tdest12"), trace_tvalid12("trace_tvalid12"), trace_tdata13("trace_tdata13"), trace_tready13("trace_tready13"), trace_tlast13("trace_tlast13"), trace_tid13("trace_tid13"), trace_tdest13("trace_tdest13"), trace_tvalid13("trace_tvalid13"), trace_tdata14("trace_tdata14"), trace_tready14("trace_tready14"), trace_tlast14("trace_tlast14"), trace_tid14("trace_tid14"), trace_tdest14("trace_tdest14"), trace_tvalid14("trace_tvalid14"), trace_tdata15("trace_tdata15"), trace_tready15("trace_tready15"), trace_tlast15("trace_tlast15"), trace_tid15("trace_tid15"), trace_tdest15("trace_tdest15"), trace_tvalid15("trace_tvalid15"), trace_tdata16("trace_tdata16"), trace_tready16("trace_tready16"), trace_tlast16("trace_tlast16"), trace_tid16("trace_tid16"), trace_tdest16("trace_tdest16"), trace_tvalid16("trace_tvalid16"), trace_tdata17("trace_tdata17"), trace_tready17("trace_tready17"), trace_tlast17("trace_tlast17"), trace_tid17("trace_tid17"), trace_tdest17("trace_tdest17"), trace_tvalid17("trace_tvalid17"), trace_tdata18("trace_tdata18"), trace_tready18("trace_tready18"), trace_tlast18("trace_tlast18"), trace_tid18("trace_tid18"), trace_tdest18("trace_tdest18"), trace_tvalid18("trace_tvalid18"), trace_tdata19("trace_tdata19"), trace_tready19("trace_tready19"), trace_tlast19("trace_tlast19"), trace_tid19("trace_tid19"), trace_tdest19("trace_tdest19"), trace_tvalid19("trace_tvalid19"), trace_tdata20("trace_tdata20"), trace_tready20("trace_tready20"), trace_tlast20("trace_tlast20"), trace_tid20("trace_tid20"), trace_tdest20("trace_tdest20"), trace_tvalid20("trace_tvalid20"), trace_tdata21("trace_tdata21"), trace_tready21("trace_tready21"), trace_tlast21("trace_tlast21"), trace_tid21("trace_tid21"), trace_tdest21("trace_tdest21"), trace_tvalid21("trace_tvalid21"), trace_tdata22("trace_tdata22"), trace_tready22("trace_tready22"), trace_tlast22("trace_tlast22"), trace_tid22("trace_tid22"), trace_tdest22("trace_tdest22"), trace_tvalid22("trace_tvalid22"), trace_tdata23("trace_tdata23"), trace_tready23("trace_tready23"), trace_tlast23("trace_tlast23"), trace_tid23("trace_tid23"), trace_tdest23("trace_tdest23"), trace_tvalid23("trace_tvalid23"), trace_tdata24("trace_tdata24"), trace_tready24("trace_tready24"), trace_tlast24("trace_tlast24"), trace_tid24("trace_tid24"), trace_tdest24("trace_tdest24"), trace_tvalid24("trace_tvalid24"), trace_tdata25("trace_tdata25"), trace_tready25("trace_tready25"), trace_tlast25("trace_tlast25"), trace_tid25("trace_tid25"), trace_tdest25("trace_tdest25"), trace_tvalid25("trace_tvalid25"), trace_tdata26("trace_tdata26"), trace_tready26("trace_tready26"), trace_tlast26("trace_tlast26"), trace_tid26("trace_tid26"), trace_tdest26("trace_tdest26"), trace_tvalid26("trace_tvalid26"), trace_tdata27("trace_tdata27"), trace_tready27("trace_tready27"), trace_tlast27("trace_tlast27"), trace_tid27("trace_tid27"), trace_tdest27("trace_tdest27"), trace_tvalid27("trace_tvalid27"), trace_tdata28("trace_tdata28"), trace_tready28("trace_tready28"), trace_tlast28("trace_tlast28"), trace_tid28("trace_tid28"), trace_tdest28("trace_tdest28"), trace_tvalid28("trace_tvalid28"), trace_tdata29("trace_tdata29"), trace_tready29("trace_tready29"), trace_tlast29("trace_tlast29"), trace_tid29("trace_tid29"), trace_tdest29("trace_tdest29"), trace_tvalid29("trace_tvalid29"), trace_tdata30("trace_tdata30"), trace_tready30("trace_tready30"), trace_tlast30("trace_tlast30"), trace_tid30("trace_tid30"), trace_tdest30("trace_tdest30"), trace_tvalid30("trace_tvalid30"), trace_tdata31("trace_tdata31"), trace_tready31("trace_tready31"), trace_tlast31("trace_tlast31"), trace_tid31("trace_tid31"), trace_tdest31("trace_tdest31"), trace_tvalid31("trace_tvalid31"), trace_tdata32("trace_tdata32"), trace_tready32("trace_tready32"), trace_tlast32("trace_tlast32"), trace_tid32("trace_tid32"), trace_tdest32("trace_tdest32"), trace_tvalid32("trace_tvalid32"), trace_tdata33("trace_tdata33"), trace_tready33("trace_tready33"), trace_tlast33("trace_tlast33"), trace_tid33("trace_tid33"), trace_tdest33("trace_tdest33"), trace_tvalid33("trace_tvalid33"), trace_tdata34("trace_tdata34"), trace_tready34("trace_tready34"), trace_tlast34("trace_tlast34"), trace_tid34("trace_tid34"), trace_tdest34("trace_tdest34"), trace_tvalid34("trace_tvalid34"), trace_tdata35("trace_tdata35"), trace_tready35("trace_tready35"), trace_tlast35("trace_tlast35"), trace_tid35("trace_tid35"), trace_tdest35("trace_tdest35"), trace_tvalid35("trace_tvalid35"), trace_tdata36("trace_tdata36"), trace_tready36("trace_tready36"), trace_tlast36("trace_tlast36"), trace_tid36("trace_tid36"), trace_tdest36("trace_tdest36"), trace_tvalid36("trace_tvalid36"), trace_tdata37("trace_tdata37"), trace_tready37("trace_tready37"), trace_tlast37("trace_tlast37"), trace_tid37("trace_tid37"), trace_tdest37("trace_tdest37"), trace_tvalid37("trace_tvalid37"), trace_tdata38("trace_tdata38"), trace_tready38("trace_tready38"), trace_tlast38("trace_tlast38"), trace_tid38("trace_tid38"), trace_tdest38("trace_tdest38"), trace_tvalid38("trace_tvalid38"), trace_tdata39("trace_tdata39"), trace_tready39("trace_tready39"), trace_tlast39("trace_tlast39"), trace_tid39("trace_tid39"), trace_tdest39("trace_tdest39"), trace_tvalid39("trace_tvalid39"), trace_tdata40("trace_tdata40"), trace_tready40("trace_tready40"), trace_tlast40("trace_tlast40"), trace_tid40("trace_tid40"), trace_tdest40("trace_tdest40"), trace_tvalid40("trace_tvalid40"), s_axi_awaddr("s_axi_awaddr"), s_axi_awvalid("s_axi_awvalid"), s_axi_awready("s_axi_awready"), s_axi_wdata("s_axi_wdata"), s_axi_wstrb("s_axi_wstrb"), s_axi_wvalid("s_axi_wvalid"), s_axi_wready("s_axi_wready"), s_axi_bvalid("s_axi_bvalid"), s_axi_bready("s_axi_bready"), s_axi_bresp("s_axi_bresp"), s_axi_araddr("s_axi_araddr"), s_axi_arvalid("s_axi_arvalid"), s_axi_arready("s_axi_arready"), s_axi_rdata("s_axi_rdata"), s_axi_rresp("s_axi_rresp"), s_axi_rvalid("s_axi_rvalid"), s_axi_rready("s_axi_rready"), s_axihub_awaddr("s_axihub_awaddr"), s_axihub_awvalid("s_axihub_awvalid"), s_axihub_awready("s_axihub_awready"), s_axihub_wdata("s_axihub_wdata"), s_axihub_wstrb("s_axihub_wstrb"), s_axihub_wvalid("s_axihub_wvalid"), s_axihub_wready("s_axihub_wready"), s_axihub_bvalid("s_axihub_bvalid"), s_axihub_bready("s_axihub_bready"), s_axihub_bresp("s_axihub_bresp"), s_axihub_araddr("s_axihub_araddr"), s_axihub_arvalid("s_axihub_arvalid"), s_axihub_arready("s_axihub_arready"), s_axihub_rdata("s_axihub_rdata"), s_axihub_rresp("s_axihub_rresp"), s_axihub_rvalid("s_axihub_rvalid"), s_axihub_rready("s_axihub_rready"), s_aximm_awaddr("s_aximm_awaddr"), s_aximm_awvalid("s_aximm_awvalid"), s_aximm_awready("s_aximm_awready"), s_aximm_awlen("s_aximm_awlen"), s_aximm_awsize("s_aximm_awsize"), s_aximm_awburst("s_aximm_awburst"), s_aximm_awid("s_aximm_awid"), s_aximm_wdata("s_aximm_wdata"), s_aximm_wstrb("s_aximm_wstrb"), s_aximm_wvalid("s_aximm_wvalid"), s_aximm_wready("s_aximm_wready"), s_aximm_bvalid("s_aximm_bvalid"), s_aximm_bready("s_aximm_bready"), s_aximm_wlast("s_aximm_wlast"), s_aximm_rlast("s_aximm_rlast"), s_aximm_bresp("s_aximm_bresp"), s_aximm_bid("s_aximm_bid"), s_aximm_araddr("s_aximm_araddr"), s_aximm_arvalid("s_aximm_arvalid"), s_aximm_arready("s_aximm_arready"), s_aximm_arlen("s_aximm_arlen"), s_aximm_arsize("s_aximm_arsize"), s_aximm_arburst("s_aximm_arburst"), s_aximm_arid("s_aximm_arid"), s_aximm_rdata("s_aximm_rdata"), s_aximm_rid("s_aximm_rid"), s_aximm_rresp("s_aximm_rresp"), s_aximm_rvalid("s_aximm_rvalid"), s_aximm_rready("s_aximm_rready")
{

  // initialize pins
  mp_impl->s_aximm_clk(s_aximm_clk);
  mp_impl->s_aximm_aresetn(s_aximm_aresetn);
  mp_impl->trace_clk(trace_clk);
  mp_impl->trace_aresetn(trace_aresetn);
  mp_impl->axilite_clk(axilite_clk);
  mp_impl->axilite_aresetn(axilite_aresetn);

  // initialize transactors
  mp_S_AXIMM_transactor = NULL;
  mp_S_AXIFIFO_transactor = NULL;
  mp_S_AXIHUB_transactor = NULL;
  mp_TRACE_IN_0_transactor = NULL;
  mp_TRACE_IN_1_transactor = NULL;
  mp_TRACE_IN_2_transactor = NULL;
  mp_TRACE_IN_3_transactor = NULL;
  mp_TRACE_IN_4_transactor = NULL;
  mp_TRACE_IN_5_transactor = NULL;
  mp_TRACE_IN_6_transactor = NULL;
  mp_TRACE_IN_7_transactor = NULL;
  mp_TRACE_IN_8_transactor = NULL;
  mp_TRACE_IN_9_transactor = NULL;
  mp_TRACE_IN_10_transactor = NULL;
  mp_TRACE_IN_11_transactor = NULL;
  mp_TRACE_IN_12_transactor = NULL;
  mp_TRACE_IN_13_transactor = NULL;
  mp_TRACE_IN_14_transactor = NULL;
  mp_TRACE_IN_15_transactor = NULL;
  mp_TRACE_IN_16_transactor = NULL;
  mp_TRACE_IN_17_transactor = NULL;
  mp_TRACE_IN_18_transactor = NULL;
  mp_TRACE_IN_19_transactor = NULL;
  mp_TRACE_IN_20_transactor = NULL;
  mp_TRACE_IN_21_transactor = NULL;
  mp_TRACE_IN_22_transactor = NULL;
  mp_TRACE_IN_23_transactor = NULL;
  mp_TRACE_IN_24_transactor = NULL;
  mp_TRACE_IN_25_transactor = NULL;
  mp_TRACE_IN_26_transactor = NULL;
  mp_TRACE_IN_27_transactor = NULL;
  mp_TRACE_IN_28_transactor = NULL;
  mp_TRACE_IN_29_transactor = NULL;
  mp_TRACE_IN_30_transactor = NULL;
  mp_TRACE_IN_31_transactor = NULL;
  mp_TRACE_IN_32_transactor = NULL;
  mp_TRACE_IN_33_transactor = NULL;
  mp_TRACE_IN_34_transactor = NULL;
  mp_TRACE_IN_35_transactor = NULL;
  mp_TRACE_IN_36_transactor = NULL;
  mp_TRACE_IN_37_transactor = NULL;
  mp_TRACE_IN_38_transactor = NULL;
  mp_TRACE_IN_39_transactor = NULL;
  mp_TRACE_IN_40_transactor = NULL;

  // initialize socket stubs

}

void pfm_dynamic_dpa_hub_0::before_end_of_elaboration()
{
  // configure 'S_AXIMM' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "S_AXIMM_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'S_AXIMM' transactor parameters
    xsc::common_cpp::properties S_AXIMM_transactor_param_props;
    S_AXIMM_transactor_param_props.addLong("DATA_WIDTH", "64");
    S_AXIMM_transactor_param_props.addLong("FREQ_HZ", "300000000");
    S_AXIMM_transactor_param_props.addLong("ID_WIDTH", "1");
    S_AXIMM_transactor_param_props.addLong("ADDR_WIDTH", "32");
    S_AXIMM_transactor_param_props.addLong("AWUSER_WIDTH", "0");
    S_AXIMM_transactor_param_props.addLong("ARUSER_WIDTH", "0");
    S_AXIMM_transactor_param_props.addLong("WUSER_WIDTH", "0");
    S_AXIMM_transactor_param_props.addLong("RUSER_WIDTH", "0");
    S_AXIMM_transactor_param_props.addLong("BUSER_WIDTH", "0");
    S_AXIMM_transactor_param_props.addLong("HAS_BURST", "1");
    S_AXIMM_transactor_param_props.addLong("HAS_LOCK", "0");
    S_AXIMM_transactor_param_props.addLong("HAS_PROT", "0");
    S_AXIMM_transactor_param_props.addLong("HAS_CACHE", "0");
    S_AXIMM_transactor_param_props.addLong("HAS_QOS", "0");
    S_AXIMM_transactor_param_props.addLong("HAS_REGION", "0");
    S_AXIMM_transactor_param_props.addLong("HAS_WSTRB", "1");
    S_AXIMM_transactor_param_props.addLong("HAS_BRESP", "1");
    S_AXIMM_transactor_param_props.addLong("HAS_RRESP", "1");
    S_AXIMM_transactor_param_props.addLong("SUPPORTS_NARROW_BURST", "0");
    S_AXIMM_transactor_param_props.addLong("NUM_READ_OUTSTANDING", "2");
    S_AXIMM_transactor_param_props.addLong("NUM_WRITE_OUTSTANDING", "2");
    S_AXIMM_transactor_param_props.addLong("MAX_BURST_LENGTH", "128");
    S_AXIMM_transactor_param_props.addLong("NUM_READ_THREADS", "1");
    S_AXIMM_transactor_param_props.addLong("NUM_WRITE_THREADS", "1");
    S_AXIMM_transactor_param_props.addLong("RUSER_BITS_PER_BYTE", "0");
    S_AXIMM_transactor_param_props.addLong("WUSER_BITS_PER_BYTE", "0");
    S_AXIMM_transactor_param_props.addLong("HAS_SIZE", "1");
    S_AXIMM_transactor_param_props.addLong("HAS_RESET", "1");
    S_AXIMM_transactor_param_props.addFloat("PHASE", "0.0");
    S_AXIMM_transactor_param_props.addString("PROTOCOL", "AXI4");
    S_AXIMM_transactor_param_props.addString("READ_WRITE_MODE", "READ_WRITE");
    S_AXIMM_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_dma_pcie_aclk");
    S_AXIMM_transactor_param_props.addString("SLR_ASSIGNMENT", "");

    mp_S_AXIMM_transactor = new xtlm::xaximm_pin2xtlm_t<64,32,1,1,1,1,1,1>("S_AXIMM_transactor", S_AXIMM_transactor_param_props);

    // S_AXIMM' transactor ports

    mp_S_AXIMM_transactor->ARADDR(s_aximm_araddr);
    mp_S_AXIMM_transactor->ARBURST(s_aximm_arburst);
    mp_S_AXIMM_transactor->ARID(s_aximm_arid);
    mp_S_AXIMM_transactor->ARLEN(s_aximm_arlen);
    mp_S_AXIMM_transactor->ARREADY(s_aximm_arready);
    mp_S_AXIMM_transactor->ARSIZE(s_aximm_arsize);
    mp_S_AXIMM_transactor->ARVALID(s_aximm_arvalid);
    mp_S_AXIMM_transactor->AWADDR(s_aximm_awaddr);
    mp_S_AXIMM_transactor->AWBURST(s_aximm_awburst);
    mp_S_AXIMM_transactor->AWID(s_aximm_awid);
    mp_S_AXIMM_transactor->AWLEN(s_aximm_awlen);
    mp_S_AXIMM_transactor->AWREADY(s_aximm_awready);
    mp_S_AXIMM_transactor->AWSIZE(s_aximm_awsize);
    mp_S_AXIMM_transactor->AWVALID(s_aximm_awvalid);
    mp_S_AXIMM_transactor->BID(s_aximm_bid);
    mp_S_AXIMM_transactor->BREADY(s_aximm_bready);
    mp_S_AXIMM_transactor->BRESP(s_aximm_bresp);
    mp_S_AXIMM_transactor->BVALID(s_aximm_bvalid);
    mp_S_AXIMM_transactor->RDATA(s_aximm_rdata);
    mp_S_AXIMM_transactor->RID(s_aximm_rid);
    mp_S_AXIMM_transactor->RLAST(s_aximm_rlast);
    mp_S_AXIMM_transactor->RREADY(s_aximm_rready);
    mp_S_AXIMM_transactor->RRESP(s_aximm_rresp);
    mp_S_AXIMM_transactor->RVALID(s_aximm_rvalid);
    mp_S_AXIMM_transactor->WDATA(s_aximm_wdata);
    mp_S_AXIMM_transactor->WLAST(s_aximm_wlast);
    mp_S_AXIMM_transactor->WREADY(s_aximm_wready);
    mp_S_AXIMM_transactor->WSTRB(s_aximm_wstrb);
    mp_S_AXIMM_transactor->WVALID(s_aximm_wvalid);
    mp_S_AXIMM_transactor->CLK(s_aximm_clk);
    mp_S_AXIMM_transactor->RST(s_aximm_aresetn);

    // S_AXIMM' transactor sockets

    mp_impl->s_aximm_rd_socket->bind(*(mp_S_AXIMM_transactor->rd_socket));
    mp_impl->s_aximm_wr_socket->bind(*(mp_S_AXIMM_transactor->wr_socket));
  }
  else
  {
  }

  // configure 'S_AXIFIFO' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "S_AXIFIFO_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'S_AXIFIFO' transactor parameters
    xsc::common_cpp::properties S_AXIFIFO_transactor_param_props;
    S_AXIFIFO_transactor_param_props.addLong("DATA_WIDTH", "32");
    S_AXIFIFO_transactor_param_props.addLong("FREQ_HZ", "300000000");
    S_AXIFIFO_transactor_param_props.addLong("ID_WIDTH", "0");
    S_AXIFIFO_transactor_param_props.addLong("ADDR_WIDTH", "8");
    S_AXIFIFO_transactor_param_props.addLong("AWUSER_WIDTH", "0");
    S_AXIFIFO_transactor_param_props.addLong("ARUSER_WIDTH", "0");
    S_AXIFIFO_transactor_param_props.addLong("WUSER_WIDTH", "0");
    S_AXIFIFO_transactor_param_props.addLong("RUSER_WIDTH", "0");
    S_AXIFIFO_transactor_param_props.addLong("BUSER_WIDTH", "0");
    S_AXIFIFO_transactor_param_props.addLong("HAS_BURST", "0");
    S_AXIFIFO_transactor_param_props.addLong("HAS_LOCK", "0");
    S_AXIFIFO_transactor_param_props.addLong("HAS_PROT", "0");
    S_AXIFIFO_transactor_param_props.addLong("HAS_CACHE", "0");
    S_AXIFIFO_transactor_param_props.addLong("HAS_QOS", "0");
    S_AXIFIFO_transactor_param_props.addLong("HAS_REGION", "0");
    S_AXIFIFO_transactor_param_props.addLong("HAS_WSTRB", "1");
    S_AXIFIFO_transactor_param_props.addLong("HAS_BRESP", "1");
    S_AXIFIFO_transactor_param_props.addLong("HAS_RRESP", "1");
    S_AXIFIFO_transactor_param_props.addLong("SUPPORTS_NARROW_BURST", "0");
    S_AXIFIFO_transactor_param_props.addLong("NUM_READ_OUTSTANDING", "2");
    S_AXIFIFO_transactor_param_props.addLong("NUM_WRITE_OUTSTANDING", "2");
    S_AXIFIFO_transactor_param_props.addLong("MAX_BURST_LENGTH", "1");
    S_AXIFIFO_transactor_param_props.addLong("NUM_READ_THREADS", "1");
    S_AXIFIFO_transactor_param_props.addLong("NUM_WRITE_THREADS", "1");
    S_AXIFIFO_transactor_param_props.addLong("RUSER_BITS_PER_BYTE", "0");
    S_AXIFIFO_transactor_param_props.addLong("WUSER_BITS_PER_BYTE", "0");
    S_AXIFIFO_transactor_param_props.addLong("HAS_SIZE", "0");
    S_AXIFIFO_transactor_param_props.addLong("HAS_RESET", "1");
    S_AXIFIFO_transactor_param_props.addFloat("PHASE", "0.0");
    S_AXIFIFO_transactor_param_props.addString("PROTOCOL", "AXI4LITE");
    S_AXIFIFO_transactor_param_props.addString("READ_WRITE_MODE", "READ_WRITE");
    S_AXIFIFO_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    S_AXIFIFO_transactor_param_props.addString("SLR_ASSIGNMENT", "");

    mp_S_AXIFIFO_transactor = new xtlm::xaximm_pin2xtlm_t<32,8,1,1,1,1,1,1>("S_AXIFIFO_transactor", S_AXIFIFO_transactor_param_props);

    // S_AXIFIFO' transactor ports

    mp_S_AXIFIFO_transactor->ARADDR(s_axi_araddr);
    mp_S_AXIFIFO_transactor->ARREADY(s_axi_arready);
    mp_S_AXIFIFO_transactor->ARVALID(s_axi_arvalid);
    mp_S_AXIFIFO_transactor->AWADDR(s_axi_awaddr);
    mp_S_AXIFIFO_transactor->AWREADY(s_axi_awready);
    mp_S_AXIFIFO_transactor->AWVALID(s_axi_awvalid);
    mp_S_AXIFIFO_transactor->BREADY(s_axi_bready);
    mp_S_AXIFIFO_transactor->BRESP(s_axi_bresp);
    mp_S_AXIFIFO_transactor->BVALID(s_axi_bvalid);
    mp_S_AXIFIFO_transactor->RDATA(s_axi_rdata);
    mp_S_AXIFIFO_transactor->RREADY(s_axi_rready);
    mp_S_AXIFIFO_transactor->RRESP(s_axi_rresp);
    mp_S_AXIFIFO_transactor->RVALID(s_axi_rvalid);
    mp_S_AXIFIFO_transactor->WDATA(s_axi_wdata);
    mp_S_AXIFIFO_transactor->WREADY(s_axi_wready);
    mp_S_AXIFIFO_transactor->WSTRB(s_axi_wstrb);
    mp_S_AXIFIFO_transactor->WVALID(s_axi_wvalid);
    mp_S_AXIFIFO_transactor->CLK(axilite_clk);
    mp_S_AXIFIFO_transactor->RST(axilite_aresetn);

    // S_AXIFIFO' transactor sockets

    mp_impl->axilite_rd_socket->bind(*(mp_S_AXIFIFO_transactor->rd_socket));
    mp_impl->axilite_wr_socket->bind(*(mp_S_AXIFIFO_transactor->wr_socket));
  }
  else
  {
  }

  // configure 'S_AXIHUB' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "S_AXIHUB_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'S_AXIHUB' transactor parameters
    xsc::common_cpp::properties S_AXIHUB_transactor_param_props;
    S_AXIHUB_transactor_param_props.addLong("DATA_WIDTH", "32");
    S_AXIHUB_transactor_param_props.addLong("FREQ_HZ", "300000000");
    S_AXIHUB_transactor_param_props.addLong("ID_WIDTH", "0");
    S_AXIHUB_transactor_param_props.addLong("ADDR_WIDTH", "8");
    S_AXIHUB_transactor_param_props.addLong("AWUSER_WIDTH", "0");
    S_AXIHUB_transactor_param_props.addLong("ARUSER_WIDTH", "0");
    S_AXIHUB_transactor_param_props.addLong("WUSER_WIDTH", "0");
    S_AXIHUB_transactor_param_props.addLong("RUSER_WIDTH", "0");
    S_AXIHUB_transactor_param_props.addLong("BUSER_WIDTH", "0");
    S_AXIHUB_transactor_param_props.addLong("HAS_BURST", "0");
    S_AXIHUB_transactor_param_props.addLong("HAS_LOCK", "0");
    S_AXIHUB_transactor_param_props.addLong("HAS_PROT", "0");
    S_AXIHUB_transactor_param_props.addLong("HAS_CACHE", "0");
    S_AXIHUB_transactor_param_props.addLong("HAS_QOS", "0");
    S_AXIHUB_transactor_param_props.addLong("HAS_REGION", "0");
    S_AXIHUB_transactor_param_props.addLong("HAS_WSTRB", "1");
    S_AXIHUB_transactor_param_props.addLong("HAS_BRESP", "1");
    S_AXIHUB_transactor_param_props.addLong("HAS_RRESP", "1");
    S_AXIHUB_transactor_param_props.addLong("SUPPORTS_NARROW_BURST", "0");
    S_AXIHUB_transactor_param_props.addLong("NUM_READ_OUTSTANDING", "2");
    S_AXIHUB_transactor_param_props.addLong("NUM_WRITE_OUTSTANDING", "2");
    S_AXIHUB_transactor_param_props.addLong("MAX_BURST_LENGTH", "1");
    S_AXIHUB_transactor_param_props.addLong("NUM_READ_THREADS", "1");
    S_AXIHUB_transactor_param_props.addLong("NUM_WRITE_THREADS", "1");
    S_AXIHUB_transactor_param_props.addLong("RUSER_BITS_PER_BYTE", "0");
    S_AXIHUB_transactor_param_props.addLong("WUSER_BITS_PER_BYTE", "0");
    S_AXIHUB_transactor_param_props.addLong("HAS_SIZE", "0");
    S_AXIHUB_transactor_param_props.addLong("HAS_RESET", "1");
    S_AXIHUB_transactor_param_props.addFloat("PHASE", "0.0");
    S_AXIHUB_transactor_param_props.addString("PROTOCOL", "AXI4LITE");
    S_AXIHUB_transactor_param_props.addString("READ_WRITE_MODE", "READ_WRITE");
    S_AXIHUB_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    S_AXIHUB_transactor_param_props.addString("SLR_ASSIGNMENT", "");

    mp_S_AXIHUB_transactor = new xtlm::xaximm_pin2xtlm_t<32,8,1,1,1,1,1,1>("S_AXIHUB_transactor", S_AXIHUB_transactor_param_props);

    // S_AXIHUB' transactor ports

    mp_S_AXIHUB_transactor->ARADDR(s_axihub_araddr);
    mp_S_AXIHUB_transactor->ARREADY(s_axihub_arready);
    mp_S_AXIHUB_transactor->ARVALID(s_axihub_arvalid);
    mp_S_AXIHUB_transactor->AWADDR(s_axihub_awaddr);
    mp_S_AXIHUB_transactor->AWREADY(s_axihub_awready);
    mp_S_AXIHUB_transactor->AWVALID(s_axihub_awvalid);
    mp_S_AXIHUB_transactor->BREADY(s_axihub_bready);
    mp_S_AXIHUB_transactor->BRESP(s_axihub_bresp);
    mp_S_AXIHUB_transactor->BVALID(s_axihub_bvalid);
    mp_S_AXIHUB_transactor->RDATA(s_axihub_rdata);
    mp_S_AXIHUB_transactor->RREADY(s_axihub_rready);
    mp_S_AXIHUB_transactor->RRESP(s_axihub_rresp);
    mp_S_AXIHUB_transactor->RVALID(s_axihub_rvalid);
    mp_S_AXIHUB_transactor->WDATA(s_axihub_wdata);
    mp_S_AXIHUB_transactor->WREADY(s_axihub_wready);
    mp_S_AXIHUB_transactor->WSTRB(s_axihub_wstrb);
    mp_S_AXIHUB_transactor->WVALID(s_axihub_wvalid);
    mp_S_AXIHUB_transactor->CLK(axilite_clk);
    mp_S_AXIHUB_transactor->RST(axilite_aresetn);

    // S_AXIHUB' transactor sockets

    mp_impl->s_axihub_rd_socket->bind(*(mp_S_AXIHUB_transactor->rd_socket));
    mp_impl->s_axihub_wr_socket->bind(*(mp_S_AXIHUB_transactor->wr_socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_0' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_0_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_0' transactor parameters
    xsc::common_cpp::properties TRACE_IN_0_transactor_param_props;
    TRACE_IN_0_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_0_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_0_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_0_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_0_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_0_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_0_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_0_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_0_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_0_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_0_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_0_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_0_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_0_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_0_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_0_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_0_transactor", TRACE_IN_0_transactor_param_props);

    // TRACE_IN_0' transactor ports

    mp_TRACE_IN_0_transactor->TDATA(trace_tdata0);
    mp_TRACE_IN_0_transactor->TDEST(trace_tdest0);
    mp_TRACE_IN_0_transactor->TID(trace_tid0);
    mp_TRACE_IN_0_transactor->TLAST(trace_tlast0);
    mp_TRACE_IN_0_transactor->TREADY(trace_tready0);
    mp_TRACE_IN_0_transactor->TVALID(trace_tvalid0);
    mp_TRACE_IN_0_transactor->CLK(trace_clk);
    mp_TRACE_IN_0_transactor->RST(trace_aresetn);

    // TRACE_IN_0' transactor sockets

    mp_impl->trace_socket0->bind(*(mp_TRACE_IN_0_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_1' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_1_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_1' transactor parameters
    xsc::common_cpp::properties TRACE_IN_1_transactor_param_props;
    TRACE_IN_1_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_1_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_1_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_1_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_1_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_1_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_1_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_1_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_1_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_1_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_1_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_1_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_1_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_1_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_1_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_1_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_1_transactor", TRACE_IN_1_transactor_param_props);

    // TRACE_IN_1' transactor ports

    mp_TRACE_IN_1_transactor->TDATA(trace_tdata1);
    mp_TRACE_IN_1_transactor->TDEST(trace_tdest1);
    mp_TRACE_IN_1_transactor->TID(trace_tid1);
    mp_TRACE_IN_1_transactor->TLAST(trace_tlast1);
    mp_TRACE_IN_1_transactor->TREADY(trace_tready1);
    mp_TRACE_IN_1_transactor->TVALID(trace_tvalid1);
    mp_TRACE_IN_1_transactor->CLK(trace_clk);
    mp_TRACE_IN_1_transactor->RST(trace_aresetn);

    // TRACE_IN_1' transactor sockets

    mp_impl->trace_socket1->bind(*(mp_TRACE_IN_1_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_2' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_2_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_2' transactor parameters
    xsc::common_cpp::properties TRACE_IN_2_transactor_param_props;
    TRACE_IN_2_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_2_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_2_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_2_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_2_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_2_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_2_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_2_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_2_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_2_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_2_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_2_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_2_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_2_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_2_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_2_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_2_transactor", TRACE_IN_2_transactor_param_props);

    // TRACE_IN_2' transactor ports

    mp_TRACE_IN_2_transactor->TDATA(trace_tdata2);
    mp_TRACE_IN_2_transactor->TDEST(trace_tdest2);
    mp_TRACE_IN_2_transactor->TID(trace_tid2);
    mp_TRACE_IN_2_transactor->TLAST(trace_tlast2);
    mp_TRACE_IN_2_transactor->TREADY(trace_tready2);
    mp_TRACE_IN_2_transactor->TVALID(trace_tvalid2);
    mp_TRACE_IN_2_transactor->CLK(trace_clk);
    mp_TRACE_IN_2_transactor->RST(trace_aresetn);

    // TRACE_IN_2' transactor sockets

    mp_impl->trace_socket2->bind(*(mp_TRACE_IN_2_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_3' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_3_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_3' transactor parameters
    xsc::common_cpp::properties TRACE_IN_3_transactor_param_props;
    TRACE_IN_3_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_3_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_3_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_3_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_3_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_3_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_3_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_3_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_3_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_3_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_3_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_3_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_3_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_3_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_3_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_3_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_3_transactor", TRACE_IN_3_transactor_param_props);

    // TRACE_IN_3' transactor ports

    mp_TRACE_IN_3_transactor->TDATA(trace_tdata3);
    mp_TRACE_IN_3_transactor->TDEST(trace_tdest3);
    mp_TRACE_IN_3_transactor->TID(trace_tid3);
    mp_TRACE_IN_3_transactor->TLAST(trace_tlast3);
    mp_TRACE_IN_3_transactor->TREADY(trace_tready3);
    mp_TRACE_IN_3_transactor->TVALID(trace_tvalid3);
    mp_TRACE_IN_3_transactor->CLK(trace_clk);
    mp_TRACE_IN_3_transactor->RST(trace_aresetn);

    // TRACE_IN_3' transactor sockets

    mp_impl->trace_socket3->bind(*(mp_TRACE_IN_3_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_4' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_4_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_4' transactor parameters
    xsc::common_cpp::properties TRACE_IN_4_transactor_param_props;
    TRACE_IN_4_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_4_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_4_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_4_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_4_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_4_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_4_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_4_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_4_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_4_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_4_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_4_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_4_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_4_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_4_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_4_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_4_transactor", TRACE_IN_4_transactor_param_props);

    // TRACE_IN_4' transactor ports

    mp_TRACE_IN_4_transactor->TDATA(trace_tdata4);
    mp_TRACE_IN_4_transactor->TDEST(trace_tdest4);
    mp_TRACE_IN_4_transactor->TID(trace_tid4);
    mp_TRACE_IN_4_transactor->TLAST(trace_tlast4);
    mp_TRACE_IN_4_transactor->TREADY(trace_tready4);
    mp_TRACE_IN_4_transactor->TVALID(trace_tvalid4);
    mp_TRACE_IN_4_transactor->CLK(trace_clk);
    mp_TRACE_IN_4_transactor->RST(trace_aresetn);

    // TRACE_IN_4' transactor sockets

    mp_impl->trace_socket4->bind(*(mp_TRACE_IN_4_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_5' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_5_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_5' transactor parameters
    xsc::common_cpp::properties TRACE_IN_5_transactor_param_props;
    TRACE_IN_5_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_5_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_5_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_5_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_5_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_5_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_5_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_5_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_5_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_5_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_5_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_5_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_5_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_5_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_5_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_5_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_5_transactor", TRACE_IN_5_transactor_param_props);

    // TRACE_IN_5' transactor ports

    mp_TRACE_IN_5_transactor->TDATA(trace_tdata5);
    mp_TRACE_IN_5_transactor->TDEST(trace_tdest5);
    mp_TRACE_IN_5_transactor->TID(trace_tid5);
    mp_TRACE_IN_5_transactor->TLAST(trace_tlast5);
    mp_TRACE_IN_5_transactor->TREADY(trace_tready5);
    mp_TRACE_IN_5_transactor->TVALID(trace_tvalid5);
    mp_TRACE_IN_5_transactor->CLK(trace_clk);
    mp_TRACE_IN_5_transactor->RST(trace_aresetn);

    // TRACE_IN_5' transactor sockets

    mp_impl->trace_socket5->bind(*(mp_TRACE_IN_5_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_6' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_6_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_6' transactor parameters
    xsc::common_cpp::properties TRACE_IN_6_transactor_param_props;
    TRACE_IN_6_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_6_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_6_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_6_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_6_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_6_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_6_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_6_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_6_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_6_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_6_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_6_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_6_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_6_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_6_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_6_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_6_transactor", TRACE_IN_6_transactor_param_props);

    // TRACE_IN_6' transactor ports

    mp_TRACE_IN_6_transactor->TDATA(trace_tdata6);
    mp_TRACE_IN_6_transactor->TDEST(trace_tdest6);
    mp_TRACE_IN_6_transactor->TID(trace_tid6);
    mp_TRACE_IN_6_transactor->TLAST(trace_tlast6);
    mp_TRACE_IN_6_transactor->TREADY(trace_tready6);
    mp_TRACE_IN_6_transactor->TVALID(trace_tvalid6);
    mp_TRACE_IN_6_transactor->CLK(trace_clk);
    mp_TRACE_IN_6_transactor->RST(trace_aresetn);

    // TRACE_IN_6' transactor sockets

    mp_impl->trace_socket6->bind(*(mp_TRACE_IN_6_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_7' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_7_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_7' transactor parameters
    xsc::common_cpp::properties TRACE_IN_7_transactor_param_props;
    TRACE_IN_7_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_7_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_7_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_7_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_7_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_7_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_7_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_7_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_7_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_7_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_7_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_7_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_7_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_7_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_7_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_7_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_7_transactor", TRACE_IN_7_transactor_param_props);

    // TRACE_IN_7' transactor ports

    mp_TRACE_IN_7_transactor->TDATA(trace_tdata7);
    mp_TRACE_IN_7_transactor->TDEST(trace_tdest7);
    mp_TRACE_IN_7_transactor->TID(trace_tid7);
    mp_TRACE_IN_7_transactor->TLAST(trace_tlast7);
    mp_TRACE_IN_7_transactor->TREADY(trace_tready7);
    mp_TRACE_IN_7_transactor->TVALID(trace_tvalid7);
    mp_TRACE_IN_7_transactor->CLK(trace_clk);
    mp_TRACE_IN_7_transactor->RST(trace_aresetn);

    // TRACE_IN_7' transactor sockets

    mp_impl->trace_socket7->bind(*(mp_TRACE_IN_7_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_8' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_8_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_8' transactor parameters
    xsc::common_cpp::properties TRACE_IN_8_transactor_param_props;
    TRACE_IN_8_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_8_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_8_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_8_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_8_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_8_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_8_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_8_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_8_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_8_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_8_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_8_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_8_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_8_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_8_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_8_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_8_transactor", TRACE_IN_8_transactor_param_props);

    // TRACE_IN_8' transactor ports

    mp_TRACE_IN_8_transactor->TDATA(trace_tdata8);
    mp_TRACE_IN_8_transactor->TDEST(trace_tdest8);
    mp_TRACE_IN_8_transactor->TID(trace_tid8);
    mp_TRACE_IN_8_transactor->TLAST(trace_tlast8);
    mp_TRACE_IN_8_transactor->TREADY(trace_tready8);
    mp_TRACE_IN_8_transactor->TVALID(trace_tvalid8);
    mp_TRACE_IN_8_transactor->CLK(trace_clk);
    mp_TRACE_IN_8_transactor->RST(trace_aresetn);

    // TRACE_IN_8' transactor sockets

    mp_impl->trace_socket8->bind(*(mp_TRACE_IN_8_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_9' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_9_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_9' transactor parameters
    xsc::common_cpp::properties TRACE_IN_9_transactor_param_props;
    TRACE_IN_9_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_9_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_9_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_9_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_9_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_9_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_9_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_9_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_9_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_9_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_9_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_9_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_9_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_9_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_9_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_9_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_9_transactor", TRACE_IN_9_transactor_param_props);

    // TRACE_IN_9' transactor ports

    mp_TRACE_IN_9_transactor->TDATA(trace_tdata9);
    mp_TRACE_IN_9_transactor->TDEST(trace_tdest9);
    mp_TRACE_IN_9_transactor->TID(trace_tid9);
    mp_TRACE_IN_9_transactor->TLAST(trace_tlast9);
    mp_TRACE_IN_9_transactor->TREADY(trace_tready9);
    mp_TRACE_IN_9_transactor->TVALID(trace_tvalid9);
    mp_TRACE_IN_9_transactor->CLK(trace_clk);
    mp_TRACE_IN_9_transactor->RST(trace_aresetn);

    // TRACE_IN_9' transactor sockets

    mp_impl->trace_socket9->bind(*(mp_TRACE_IN_9_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_10' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_10_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_10' transactor parameters
    xsc::common_cpp::properties TRACE_IN_10_transactor_param_props;
    TRACE_IN_10_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_10_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_10_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_10_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_10_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_10_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_10_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_10_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_10_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_10_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_10_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_10_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_10_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_10_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_10_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_10_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_10_transactor", TRACE_IN_10_transactor_param_props);

    // TRACE_IN_10' transactor ports

    mp_TRACE_IN_10_transactor->TDATA(trace_tdata10);
    mp_TRACE_IN_10_transactor->TDEST(trace_tdest10);
    mp_TRACE_IN_10_transactor->TID(trace_tid10);
    mp_TRACE_IN_10_transactor->TLAST(trace_tlast10);
    mp_TRACE_IN_10_transactor->TREADY(trace_tready10);
    mp_TRACE_IN_10_transactor->TVALID(trace_tvalid10);
    mp_TRACE_IN_10_transactor->CLK(trace_clk);
    mp_TRACE_IN_10_transactor->RST(trace_aresetn);

    // TRACE_IN_10' transactor sockets

    mp_impl->trace_socket10->bind(*(mp_TRACE_IN_10_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_11' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_11_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_11' transactor parameters
    xsc::common_cpp::properties TRACE_IN_11_transactor_param_props;
    TRACE_IN_11_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_11_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_11_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_11_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_11_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_11_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_11_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_11_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_11_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_11_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_11_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_11_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_11_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_11_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_11_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_11_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_11_transactor", TRACE_IN_11_transactor_param_props);

    // TRACE_IN_11' transactor ports

    mp_TRACE_IN_11_transactor->TDATA(trace_tdata11);
    mp_TRACE_IN_11_transactor->TDEST(trace_tdest11);
    mp_TRACE_IN_11_transactor->TID(trace_tid11);
    mp_TRACE_IN_11_transactor->TLAST(trace_tlast11);
    mp_TRACE_IN_11_transactor->TREADY(trace_tready11);
    mp_TRACE_IN_11_transactor->TVALID(trace_tvalid11);
    mp_TRACE_IN_11_transactor->CLK(trace_clk);
    mp_TRACE_IN_11_transactor->RST(trace_aresetn);

    // TRACE_IN_11' transactor sockets

    mp_impl->trace_socket11->bind(*(mp_TRACE_IN_11_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_12' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_12_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_12' transactor parameters
    xsc::common_cpp::properties TRACE_IN_12_transactor_param_props;
    TRACE_IN_12_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_12_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_12_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_12_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_12_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_12_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_12_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_12_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_12_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_12_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_12_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_12_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_12_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_12_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_12_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_12_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_12_transactor", TRACE_IN_12_transactor_param_props);

    // TRACE_IN_12' transactor ports

    mp_TRACE_IN_12_transactor->TDATA(trace_tdata12);
    mp_TRACE_IN_12_transactor->TDEST(trace_tdest12);
    mp_TRACE_IN_12_transactor->TID(trace_tid12);
    mp_TRACE_IN_12_transactor->TLAST(trace_tlast12);
    mp_TRACE_IN_12_transactor->TREADY(trace_tready12);
    mp_TRACE_IN_12_transactor->TVALID(trace_tvalid12);
    mp_TRACE_IN_12_transactor->CLK(trace_clk);
    mp_TRACE_IN_12_transactor->RST(trace_aresetn);

    // TRACE_IN_12' transactor sockets

    mp_impl->trace_socket12->bind(*(mp_TRACE_IN_12_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_13' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_13_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_13' transactor parameters
    xsc::common_cpp::properties TRACE_IN_13_transactor_param_props;
    TRACE_IN_13_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_13_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_13_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_13_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_13_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_13_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_13_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_13_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_13_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_13_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_13_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_13_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_13_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_13_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_13_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_13_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_13_transactor", TRACE_IN_13_transactor_param_props);

    // TRACE_IN_13' transactor ports

    mp_TRACE_IN_13_transactor->TDATA(trace_tdata13);
    mp_TRACE_IN_13_transactor->TDEST(trace_tdest13);
    mp_TRACE_IN_13_transactor->TID(trace_tid13);
    mp_TRACE_IN_13_transactor->TLAST(trace_tlast13);
    mp_TRACE_IN_13_transactor->TREADY(trace_tready13);
    mp_TRACE_IN_13_transactor->TVALID(trace_tvalid13);
    mp_TRACE_IN_13_transactor->CLK(trace_clk);
    mp_TRACE_IN_13_transactor->RST(trace_aresetn);

    // TRACE_IN_13' transactor sockets

    mp_impl->trace_socket13->bind(*(mp_TRACE_IN_13_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_14' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_14_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_14' transactor parameters
    xsc::common_cpp::properties TRACE_IN_14_transactor_param_props;
    TRACE_IN_14_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_14_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_14_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_14_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_14_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_14_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_14_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_14_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_14_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_14_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_14_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_14_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_14_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_14_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_14_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_14_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_14_transactor", TRACE_IN_14_transactor_param_props);

    // TRACE_IN_14' transactor ports

    mp_TRACE_IN_14_transactor->TDATA(trace_tdata14);
    mp_TRACE_IN_14_transactor->TDEST(trace_tdest14);
    mp_TRACE_IN_14_transactor->TID(trace_tid14);
    mp_TRACE_IN_14_transactor->TLAST(trace_tlast14);
    mp_TRACE_IN_14_transactor->TREADY(trace_tready14);
    mp_TRACE_IN_14_transactor->TVALID(trace_tvalid14);
    mp_TRACE_IN_14_transactor->CLK(trace_clk);
    mp_TRACE_IN_14_transactor->RST(trace_aresetn);

    // TRACE_IN_14' transactor sockets

    mp_impl->trace_socket14->bind(*(mp_TRACE_IN_14_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_15' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_15_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_15' transactor parameters
    xsc::common_cpp::properties TRACE_IN_15_transactor_param_props;
    TRACE_IN_15_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_15_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_15_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_15_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_15_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_15_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_15_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_15_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_15_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_15_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_15_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_15_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_15_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_15_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_15_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_15_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_15_transactor", TRACE_IN_15_transactor_param_props);

    // TRACE_IN_15' transactor ports

    mp_TRACE_IN_15_transactor->TDATA(trace_tdata15);
    mp_TRACE_IN_15_transactor->TDEST(trace_tdest15);
    mp_TRACE_IN_15_transactor->TID(trace_tid15);
    mp_TRACE_IN_15_transactor->TLAST(trace_tlast15);
    mp_TRACE_IN_15_transactor->TREADY(trace_tready15);
    mp_TRACE_IN_15_transactor->TVALID(trace_tvalid15);
    mp_TRACE_IN_15_transactor->CLK(trace_clk);
    mp_TRACE_IN_15_transactor->RST(trace_aresetn);

    // TRACE_IN_15' transactor sockets

    mp_impl->trace_socket15->bind(*(mp_TRACE_IN_15_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_16' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_16_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_16' transactor parameters
    xsc::common_cpp::properties TRACE_IN_16_transactor_param_props;
    TRACE_IN_16_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_16_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_16_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_16_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_16_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_16_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_16_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_16_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_16_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_16_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_16_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_16_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_16_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_16_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_16_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_16_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_16_transactor", TRACE_IN_16_transactor_param_props);

    // TRACE_IN_16' transactor ports

    mp_TRACE_IN_16_transactor->TDATA(trace_tdata16);
    mp_TRACE_IN_16_transactor->TDEST(trace_tdest16);
    mp_TRACE_IN_16_transactor->TID(trace_tid16);
    mp_TRACE_IN_16_transactor->TLAST(trace_tlast16);
    mp_TRACE_IN_16_transactor->TREADY(trace_tready16);
    mp_TRACE_IN_16_transactor->TVALID(trace_tvalid16);
    mp_TRACE_IN_16_transactor->CLK(trace_clk);
    mp_TRACE_IN_16_transactor->RST(trace_aresetn);

    // TRACE_IN_16' transactor sockets

    mp_impl->trace_socket16->bind(*(mp_TRACE_IN_16_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_17' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_17_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_17' transactor parameters
    xsc::common_cpp::properties TRACE_IN_17_transactor_param_props;
    TRACE_IN_17_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_17_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_17_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_17_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_17_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_17_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_17_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_17_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_17_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_17_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_17_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_17_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_17_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_17_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_17_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_17_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_17_transactor", TRACE_IN_17_transactor_param_props);

    // TRACE_IN_17' transactor ports

    mp_TRACE_IN_17_transactor->TDATA(trace_tdata17);
    mp_TRACE_IN_17_transactor->TDEST(trace_tdest17);
    mp_TRACE_IN_17_transactor->TID(trace_tid17);
    mp_TRACE_IN_17_transactor->TLAST(trace_tlast17);
    mp_TRACE_IN_17_transactor->TREADY(trace_tready17);
    mp_TRACE_IN_17_transactor->TVALID(trace_tvalid17);
    mp_TRACE_IN_17_transactor->CLK(trace_clk);
    mp_TRACE_IN_17_transactor->RST(trace_aresetn);

    // TRACE_IN_17' transactor sockets

    mp_impl->trace_socket17->bind(*(mp_TRACE_IN_17_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_18' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_18_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_18' transactor parameters
    xsc::common_cpp::properties TRACE_IN_18_transactor_param_props;
    TRACE_IN_18_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_18_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_18_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_18_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_18_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_18_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_18_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_18_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_18_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_18_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_18_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_18_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_18_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_18_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_18_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_18_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_18_transactor", TRACE_IN_18_transactor_param_props);

    // TRACE_IN_18' transactor ports

    mp_TRACE_IN_18_transactor->TDATA(trace_tdata18);
    mp_TRACE_IN_18_transactor->TDEST(trace_tdest18);
    mp_TRACE_IN_18_transactor->TID(trace_tid18);
    mp_TRACE_IN_18_transactor->TLAST(trace_tlast18);
    mp_TRACE_IN_18_transactor->TREADY(trace_tready18);
    mp_TRACE_IN_18_transactor->TVALID(trace_tvalid18);
    mp_TRACE_IN_18_transactor->CLK(trace_clk);
    mp_TRACE_IN_18_transactor->RST(trace_aresetn);

    // TRACE_IN_18' transactor sockets

    mp_impl->trace_socket18->bind(*(mp_TRACE_IN_18_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_19' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_19_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_19' transactor parameters
    xsc::common_cpp::properties TRACE_IN_19_transactor_param_props;
    TRACE_IN_19_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_19_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_19_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_19_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_19_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_19_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_19_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_19_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_19_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_19_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_19_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_19_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_19_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_19_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_19_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_19_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_19_transactor", TRACE_IN_19_transactor_param_props);

    // TRACE_IN_19' transactor ports

    mp_TRACE_IN_19_transactor->TDATA(trace_tdata19);
    mp_TRACE_IN_19_transactor->TDEST(trace_tdest19);
    mp_TRACE_IN_19_transactor->TID(trace_tid19);
    mp_TRACE_IN_19_transactor->TLAST(trace_tlast19);
    mp_TRACE_IN_19_transactor->TREADY(trace_tready19);
    mp_TRACE_IN_19_transactor->TVALID(trace_tvalid19);
    mp_TRACE_IN_19_transactor->CLK(trace_clk);
    mp_TRACE_IN_19_transactor->RST(trace_aresetn);

    // TRACE_IN_19' transactor sockets

    mp_impl->trace_socket19->bind(*(mp_TRACE_IN_19_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_20' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_20_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_20' transactor parameters
    xsc::common_cpp::properties TRACE_IN_20_transactor_param_props;
    TRACE_IN_20_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_20_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_20_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_20_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_20_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_20_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_20_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_20_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_20_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_20_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_20_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_20_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_20_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_20_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_20_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_20_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_20_transactor", TRACE_IN_20_transactor_param_props);

    // TRACE_IN_20' transactor ports

    mp_TRACE_IN_20_transactor->TDATA(trace_tdata20);
    mp_TRACE_IN_20_transactor->TDEST(trace_tdest20);
    mp_TRACE_IN_20_transactor->TID(trace_tid20);
    mp_TRACE_IN_20_transactor->TLAST(trace_tlast20);
    mp_TRACE_IN_20_transactor->TREADY(trace_tready20);
    mp_TRACE_IN_20_transactor->TVALID(trace_tvalid20);
    mp_TRACE_IN_20_transactor->CLK(trace_clk);
    mp_TRACE_IN_20_transactor->RST(trace_aresetn);

    // TRACE_IN_20' transactor sockets

    mp_impl->trace_socket20->bind(*(mp_TRACE_IN_20_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_21' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_21_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_21' transactor parameters
    xsc::common_cpp::properties TRACE_IN_21_transactor_param_props;
    TRACE_IN_21_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_21_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_21_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_21_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_21_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_21_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_21_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_21_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_21_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_21_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_21_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_21_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_21_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_21_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_21_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_21_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_21_transactor", TRACE_IN_21_transactor_param_props);

    // TRACE_IN_21' transactor ports

    mp_TRACE_IN_21_transactor->TDATA(trace_tdata21);
    mp_TRACE_IN_21_transactor->TDEST(trace_tdest21);
    mp_TRACE_IN_21_transactor->TID(trace_tid21);
    mp_TRACE_IN_21_transactor->TLAST(trace_tlast21);
    mp_TRACE_IN_21_transactor->TREADY(trace_tready21);
    mp_TRACE_IN_21_transactor->TVALID(trace_tvalid21);
    mp_TRACE_IN_21_transactor->CLK(trace_clk);
    mp_TRACE_IN_21_transactor->RST(trace_aresetn);

    // TRACE_IN_21' transactor sockets

    mp_impl->trace_socket21->bind(*(mp_TRACE_IN_21_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_22' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_22_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_22' transactor parameters
    xsc::common_cpp::properties TRACE_IN_22_transactor_param_props;
    TRACE_IN_22_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_22_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_22_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_22_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_22_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_22_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_22_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_22_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_22_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_22_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_22_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_22_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_22_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_22_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_22_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_22_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_22_transactor", TRACE_IN_22_transactor_param_props);

    // TRACE_IN_22' transactor ports

    mp_TRACE_IN_22_transactor->TDATA(trace_tdata22);
    mp_TRACE_IN_22_transactor->TDEST(trace_tdest22);
    mp_TRACE_IN_22_transactor->TID(trace_tid22);
    mp_TRACE_IN_22_transactor->TLAST(trace_tlast22);
    mp_TRACE_IN_22_transactor->TREADY(trace_tready22);
    mp_TRACE_IN_22_transactor->TVALID(trace_tvalid22);
    mp_TRACE_IN_22_transactor->CLK(trace_clk);
    mp_TRACE_IN_22_transactor->RST(trace_aresetn);

    // TRACE_IN_22' transactor sockets

    mp_impl->trace_socket22->bind(*(mp_TRACE_IN_22_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_23' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_23_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_23' transactor parameters
    xsc::common_cpp::properties TRACE_IN_23_transactor_param_props;
    TRACE_IN_23_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_23_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_23_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_23_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_23_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_23_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_23_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_23_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_23_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_23_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_23_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_23_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_23_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_23_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_23_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_23_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_23_transactor", TRACE_IN_23_transactor_param_props);

    // TRACE_IN_23' transactor ports

    mp_TRACE_IN_23_transactor->TDATA(trace_tdata23);
    mp_TRACE_IN_23_transactor->TDEST(trace_tdest23);
    mp_TRACE_IN_23_transactor->TID(trace_tid23);
    mp_TRACE_IN_23_transactor->TLAST(trace_tlast23);
    mp_TRACE_IN_23_transactor->TREADY(trace_tready23);
    mp_TRACE_IN_23_transactor->TVALID(trace_tvalid23);
    mp_TRACE_IN_23_transactor->CLK(trace_clk);
    mp_TRACE_IN_23_transactor->RST(trace_aresetn);

    // TRACE_IN_23' transactor sockets

    mp_impl->trace_socket23->bind(*(mp_TRACE_IN_23_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_24' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_24_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_24' transactor parameters
    xsc::common_cpp::properties TRACE_IN_24_transactor_param_props;
    TRACE_IN_24_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_24_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_24_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_24_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_24_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_24_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_24_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_24_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_24_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_24_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_24_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_24_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_24_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_24_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_24_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_24_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_24_transactor", TRACE_IN_24_transactor_param_props);

    // TRACE_IN_24' transactor ports

    mp_TRACE_IN_24_transactor->TDATA(trace_tdata24);
    mp_TRACE_IN_24_transactor->TDEST(trace_tdest24);
    mp_TRACE_IN_24_transactor->TID(trace_tid24);
    mp_TRACE_IN_24_transactor->TLAST(trace_tlast24);
    mp_TRACE_IN_24_transactor->TREADY(trace_tready24);
    mp_TRACE_IN_24_transactor->TVALID(trace_tvalid24);
    mp_TRACE_IN_24_transactor->CLK(trace_clk);
    mp_TRACE_IN_24_transactor->RST(trace_aresetn);

    // TRACE_IN_24' transactor sockets

    mp_impl->trace_socket24->bind(*(mp_TRACE_IN_24_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_25' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_25_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_25' transactor parameters
    xsc::common_cpp::properties TRACE_IN_25_transactor_param_props;
    TRACE_IN_25_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_25_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_25_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_25_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_25_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_25_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_25_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_25_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_25_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_25_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_25_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_25_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_25_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_25_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_25_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_25_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_25_transactor", TRACE_IN_25_transactor_param_props);

    // TRACE_IN_25' transactor ports

    mp_TRACE_IN_25_transactor->TDATA(trace_tdata25);
    mp_TRACE_IN_25_transactor->TDEST(trace_tdest25);
    mp_TRACE_IN_25_transactor->TID(trace_tid25);
    mp_TRACE_IN_25_transactor->TLAST(trace_tlast25);
    mp_TRACE_IN_25_transactor->TREADY(trace_tready25);
    mp_TRACE_IN_25_transactor->TVALID(trace_tvalid25);
    mp_TRACE_IN_25_transactor->CLK(trace_clk);
    mp_TRACE_IN_25_transactor->RST(trace_aresetn);

    // TRACE_IN_25' transactor sockets

    mp_impl->trace_socket25->bind(*(mp_TRACE_IN_25_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_26' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_26_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_26' transactor parameters
    xsc::common_cpp::properties TRACE_IN_26_transactor_param_props;
    TRACE_IN_26_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_26_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_26_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_26_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_26_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_26_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_26_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_26_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_26_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_26_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_26_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_26_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_26_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_26_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_26_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_26_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_26_transactor", TRACE_IN_26_transactor_param_props);

    // TRACE_IN_26' transactor ports

    mp_TRACE_IN_26_transactor->TDATA(trace_tdata26);
    mp_TRACE_IN_26_transactor->TDEST(trace_tdest26);
    mp_TRACE_IN_26_transactor->TID(trace_tid26);
    mp_TRACE_IN_26_transactor->TLAST(trace_tlast26);
    mp_TRACE_IN_26_transactor->TREADY(trace_tready26);
    mp_TRACE_IN_26_transactor->TVALID(trace_tvalid26);
    mp_TRACE_IN_26_transactor->CLK(trace_clk);
    mp_TRACE_IN_26_transactor->RST(trace_aresetn);

    // TRACE_IN_26' transactor sockets

    mp_impl->trace_socket26->bind(*(mp_TRACE_IN_26_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_27' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_27_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_27' transactor parameters
    xsc::common_cpp::properties TRACE_IN_27_transactor_param_props;
    TRACE_IN_27_transactor_param_props.addLong("TDATA_NUM_BYTES", "1");
    TRACE_IN_27_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_27_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_27_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_27_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_27_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_27_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_27_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_27_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_27_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_27_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_27_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_27_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_27_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_27_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_27_transactor = new xtlm::xaxis_pin2xtlm_t<1,1,8,8,1,1>("TRACE_IN_27_transactor", TRACE_IN_27_transactor_param_props);

    // TRACE_IN_27' transactor ports

    mp_TRACE_IN_27_transactor->TDATA(trace_tdata27);
    mp_TRACE_IN_27_transactor->TDEST(trace_tdest27);
    mp_TRACE_IN_27_transactor->TID(trace_tid27);
    mp_TRACE_IN_27_transactor->TLAST(trace_tlast27);
    mp_TRACE_IN_27_transactor->TREADY(trace_tready27);
    mp_TRACE_IN_27_transactor->TVALID(trace_tvalid27);
    mp_TRACE_IN_27_transactor->CLK(trace_clk);
    mp_TRACE_IN_27_transactor->RST(trace_aresetn);

    // TRACE_IN_27' transactor sockets

    mp_impl->trace_socket27->bind(*(mp_TRACE_IN_27_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_28' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_28_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_28' transactor parameters
    xsc::common_cpp::properties TRACE_IN_28_transactor_param_props;
    TRACE_IN_28_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_28_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_28_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_28_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_28_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_28_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_28_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_28_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_28_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_28_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_28_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_28_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_28_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_28_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_28_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_28_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_28_transactor", TRACE_IN_28_transactor_param_props);

    // TRACE_IN_28' transactor ports

    mp_TRACE_IN_28_transactor->TDATA(trace_tdata28);
    mp_TRACE_IN_28_transactor->TDEST(trace_tdest28);
    mp_TRACE_IN_28_transactor->TID(trace_tid28);
    mp_TRACE_IN_28_transactor->TLAST(trace_tlast28);
    mp_TRACE_IN_28_transactor->TREADY(trace_tready28);
    mp_TRACE_IN_28_transactor->TVALID(trace_tvalid28);
    mp_TRACE_IN_28_transactor->CLK(trace_clk);
    mp_TRACE_IN_28_transactor->RST(trace_aresetn);

    // TRACE_IN_28' transactor sockets

    mp_impl->trace_socket28->bind(*(mp_TRACE_IN_28_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_29' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_29_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_29' transactor parameters
    xsc::common_cpp::properties TRACE_IN_29_transactor_param_props;
    TRACE_IN_29_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_29_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_29_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_29_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_29_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_29_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_29_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_29_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_29_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_29_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_29_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_29_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_29_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_29_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_29_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_29_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_29_transactor", TRACE_IN_29_transactor_param_props);

    // TRACE_IN_29' transactor ports

    mp_TRACE_IN_29_transactor->TDATA(trace_tdata29);
    mp_TRACE_IN_29_transactor->TDEST(trace_tdest29);
    mp_TRACE_IN_29_transactor->TID(trace_tid29);
    mp_TRACE_IN_29_transactor->TLAST(trace_tlast29);
    mp_TRACE_IN_29_transactor->TREADY(trace_tready29);
    mp_TRACE_IN_29_transactor->TVALID(trace_tvalid29);
    mp_TRACE_IN_29_transactor->CLK(trace_clk);
    mp_TRACE_IN_29_transactor->RST(trace_aresetn);

    // TRACE_IN_29' transactor sockets

    mp_impl->trace_socket29->bind(*(mp_TRACE_IN_29_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_30' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_30_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_30' transactor parameters
    xsc::common_cpp::properties TRACE_IN_30_transactor_param_props;
    TRACE_IN_30_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_30_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_30_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_30_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_30_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_30_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_30_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_30_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_30_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_30_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_30_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_30_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_30_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_30_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_30_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_30_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_30_transactor", TRACE_IN_30_transactor_param_props);

    // TRACE_IN_30' transactor ports

    mp_TRACE_IN_30_transactor->TDATA(trace_tdata30);
    mp_TRACE_IN_30_transactor->TDEST(trace_tdest30);
    mp_TRACE_IN_30_transactor->TID(trace_tid30);
    mp_TRACE_IN_30_transactor->TLAST(trace_tlast30);
    mp_TRACE_IN_30_transactor->TREADY(trace_tready30);
    mp_TRACE_IN_30_transactor->TVALID(trace_tvalid30);
    mp_TRACE_IN_30_transactor->CLK(trace_clk);
    mp_TRACE_IN_30_transactor->RST(trace_aresetn);

    // TRACE_IN_30' transactor sockets

    mp_impl->trace_socket30->bind(*(mp_TRACE_IN_30_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_31' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_31_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_31' transactor parameters
    xsc::common_cpp::properties TRACE_IN_31_transactor_param_props;
    TRACE_IN_31_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_31_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_31_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_31_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_31_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_31_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_31_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_31_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_31_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_31_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_31_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_31_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_31_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_31_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_31_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_31_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_31_transactor", TRACE_IN_31_transactor_param_props);

    // TRACE_IN_31' transactor ports

    mp_TRACE_IN_31_transactor->TDATA(trace_tdata31);
    mp_TRACE_IN_31_transactor->TDEST(trace_tdest31);
    mp_TRACE_IN_31_transactor->TID(trace_tid31);
    mp_TRACE_IN_31_transactor->TLAST(trace_tlast31);
    mp_TRACE_IN_31_transactor->TREADY(trace_tready31);
    mp_TRACE_IN_31_transactor->TVALID(trace_tvalid31);
    mp_TRACE_IN_31_transactor->CLK(trace_clk);
    mp_TRACE_IN_31_transactor->RST(trace_aresetn);

    // TRACE_IN_31' transactor sockets

    mp_impl->trace_socket31->bind(*(mp_TRACE_IN_31_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_32' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_32_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_32' transactor parameters
    xsc::common_cpp::properties TRACE_IN_32_transactor_param_props;
    TRACE_IN_32_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_32_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_32_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_32_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_32_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_32_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_32_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_32_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_32_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_32_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_32_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_32_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_32_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_32_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_32_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_32_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_32_transactor", TRACE_IN_32_transactor_param_props);

    // TRACE_IN_32' transactor ports

    mp_TRACE_IN_32_transactor->TDATA(trace_tdata32);
    mp_TRACE_IN_32_transactor->TDEST(trace_tdest32);
    mp_TRACE_IN_32_transactor->TID(trace_tid32);
    mp_TRACE_IN_32_transactor->TLAST(trace_tlast32);
    mp_TRACE_IN_32_transactor->TREADY(trace_tready32);
    mp_TRACE_IN_32_transactor->TVALID(trace_tvalid32);
    mp_TRACE_IN_32_transactor->CLK(trace_clk);
    mp_TRACE_IN_32_transactor->RST(trace_aresetn);

    // TRACE_IN_32' transactor sockets

    mp_impl->trace_socket32->bind(*(mp_TRACE_IN_32_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_33' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_33_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_33' transactor parameters
    xsc::common_cpp::properties TRACE_IN_33_transactor_param_props;
    TRACE_IN_33_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_33_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_33_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_33_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_33_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_33_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_33_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_33_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_33_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_33_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_33_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_33_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_33_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_33_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_33_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_33_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_33_transactor", TRACE_IN_33_transactor_param_props);

    // TRACE_IN_33' transactor ports

    mp_TRACE_IN_33_transactor->TDATA(trace_tdata33);
    mp_TRACE_IN_33_transactor->TDEST(trace_tdest33);
    mp_TRACE_IN_33_transactor->TID(trace_tid33);
    mp_TRACE_IN_33_transactor->TLAST(trace_tlast33);
    mp_TRACE_IN_33_transactor->TREADY(trace_tready33);
    mp_TRACE_IN_33_transactor->TVALID(trace_tvalid33);
    mp_TRACE_IN_33_transactor->CLK(trace_clk);
    mp_TRACE_IN_33_transactor->RST(trace_aresetn);

    // TRACE_IN_33' transactor sockets

    mp_impl->trace_socket33->bind(*(mp_TRACE_IN_33_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_34' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_34_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_34' transactor parameters
    xsc::common_cpp::properties TRACE_IN_34_transactor_param_props;
    TRACE_IN_34_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_34_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_34_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_34_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_34_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_34_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_34_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_34_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_34_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_34_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_34_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_34_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_34_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_34_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_34_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_34_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_34_transactor", TRACE_IN_34_transactor_param_props);

    // TRACE_IN_34' transactor ports

    mp_TRACE_IN_34_transactor->TDATA(trace_tdata34);
    mp_TRACE_IN_34_transactor->TDEST(trace_tdest34);
    mp_TRACE_IN_34_transactor->TID(trace_tid34);
    mp_TRACE_IN_34_transactor->TLAST(trace_tlast34);
    mp_TRACE_IN_34_transactor->TREADY(trace_tready34);
    mp_TRACE_IN_34_transactor->TVALID(trace_tvalid34);
    mp_TRACE_IN_34_transactor->CLK(trace_clk);
    mp_TRACE_IN_34_transactor->RST(trace_aresetn);

    // TRACE_IN_34' transactor sockets

    mp_impl->trace_socket34->bind(*(mp_TRACE_IN_34_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_35' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_35_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_35' transactor parameters
    xsc::common_cpp::properties TRACE_IN_35_transactor_param_props;
    TRACE_IN_35_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_35_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_35_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_35_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_35_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_35_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_35_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_35_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_35_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_35_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_35_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_35_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_35_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_35_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_35_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_35_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_35_transactor", TRACE_IN_35_transactor_param_props);

    // TRACE_IN_35' transactor ports

    mp_TRACE_IN_35_transactor->TDATA(trace_tdata35);
    mp_TRACE_IN_35_transactor->TDEST(trace_tdest35);
    mp_TRACE_IN_35_transactor->TID(trace_tid35);
    mp_TRACE_IN_35_transactor->TLAST(trace_tlast35);
    mp_TRACE_IN_35_transactor->TREADY(trace_tready35);
    mp_TRACE_IN_35_transactor->TVALID(trace_tvalid35);
    mp_TRACE_IN_35_transactor->CLK(trace_clk);
    mp_TRACE_IN_35_transactor->RST(trace_aresetn);

    // TRACE_IN_35' transactor sockets

    mp_impl->trace_socket35->bind(*(mp_TRACE_IN_35_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_36' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_36_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_36' transactor parameters
    xsc::common_cpp::properties TRACE_IN_36_transactor_param_props;
    TRACE_IN_36_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_36_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_36_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_36_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_36_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_36_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_36_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_36_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_36_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_36_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_36_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_36_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_36_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_36_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_36_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_36_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_36_transactor", TRACE_IN_36_transactor_param_props);

    // TRACE_IN_36' transactor ports

    mp_TRACE_IN_36_transactor->TDATA(trace_tdata36);
    mp_TRACE_IN_36_transactor->TDEST(trace_tdest36);
    mp_TRACE_IN_36_transactor->TID(trace_tid36);
    mp_TRACE_IN_36_transactor->TLAST(trace_tlast36);
    mp_TRACE_IN_36_transactor->TREADY(trace_tready36);
    mp_TRACE_IN_36_transactor->TVALID(trace_tvalid36);
    mp_TRACE_IN_36_transactor->CLK(trace_clk);
    mp_TRACE_IN_36_transactor->RST(trace_aresetn);

    // TRACE_IN_36' transactor sockets

    mp_impl->trace_socket36->bind(*(mp_TRACE_IN_36_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_37' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_37_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_37' transactor parameters
    xsc::common_cpp::properties TRACE_IN_37_transactor_param_props;
    TRACE_IN_37_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_37_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_37_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_37_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_37_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_37_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_37_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_37_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_37_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_37_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_37_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_37_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_37_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_37_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_37_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_37_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_37_transactor", TRACE_IN_37_transactor_param_props);

    // TRACE_IN_37' transactor ports

    mp_TRACE_IN_37_transactor->TDATA(trace_tdata37);
    mp_TRACE_IN_37_transactor->TDEST(trace_tdest37);
    mp_TRACE_IN_37_transactor->TID(trace_tid37);
    mp_TRACE_IN_37_transactor->TLAST(trace_tlast37);
    mp_TRACE_IN_37_transactor->TREADY(trace_tready37);
    mp_TRACE_IN_37_transactor->TVALID(trace_tvalid37);
    mp_TRACE_IN_37_transactor->CLK(trace_clk);
    mp_TRACE_IN_37_transactor->RST(trace_aresetn);

    // TRACE_IN_37' transactor sockets

    mp_impl->trace_socket37->bind(*(mp_TRACE_IN_37_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_38' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_38_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_38' transactor parameters
    xsc::common_cpp::properties TRACE_IN_38_transactor_param_props;
    TRACE_IN_38_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_38_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_38_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_38_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_38_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_38_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_38_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_38_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_38_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_38_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_38_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_38_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_38_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_38_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_38_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_38_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_38_transactor", TRACE_IN_38_transactor_param_props);

    // TRACE_IN_38' transactor ports

    mp_TRACE_IN_38_transactor->TDATA(trace_tdata38);
    mp_TRACE_IN_38_transactor->TDEST(trace_tdest38);
    mp_TRACE_IN_38_transactor->TID(trace_tid38);
    mp_TRACE_IN_38_transactor->TLAST(trace_tlast38);
    mp_TRACE_IN_38_transactor->TREADY(trace_tready38);
    mp_TRACE_IN_38_transactor->TVALID(trace_tvalid38);
    mp_TRACE_IN_38_transactor->CLK(trace_clk);
    mp_TRACE_IN_38_transactor->RST(trace_aresetn);

    // TRACE_IN_38' transactor sockets

    mp_impl->trace_socket38->bind(*(mp_TRACE_IN_38_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_39' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_39_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_39' transactor parameters
    xsc::common_cpp::properties TRACE_IN_39_transactor_param_props;
    TRACE_IN_39_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_39_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_39_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_39_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_39_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_39_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_39_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_39_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_39_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_39_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_39_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_39_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_39_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_39_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_39_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_39_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_39_transactor", TRACE_IN_39_transactor_param_props);

    // TRACE_IN_39' transactor ports

    mp_TRACE_IN_39_transactor->TDATA(trace_tdata39);
    mp_TRACE_IN_39_transactor->TDEST(trace_tdest39);
    mp_TRACE_IN_39_transactor->TID(trace_tid39);
    mp_TRACE_IN_39_transactor->TLAST(trace_tlast39);
    mp_TRACE_IN_39_transactor->TREADY(trace_tready39);
    mp_TRACE_IN_39_transactor->TVALID(trace_tvalid39);
    mp_TRACE_IN_39_transactor->CLK(trace_clk);
    mp_TRACE_IN_39_transactor->RST(trace_aresetn);

    // TRACE_IN_39' transactor sockets

    mp_impl->trace_socket39->bind(*(mp_TRACE_IN_39_transactor->socket));
  }
  else
  {
  }

  // configure 'TRACE_IN_40' transactor

  if (xsc::utils::xsc_sim_manager::getInstanceParameterInt("pfm_dynamic_dpa_hub_0", "TRACE_IN_40_TLM_MODE") != 1)
  {
    // Instantiate Socket Stubs

  // 'TRACE_IN_40' transactor parameters
    xsc::common_cpp::properties TRACE_IN_40_transactor_param_props;
    TRACE_IN_40_transactor_param_props.addLong("TDATA_NUM_BYTES", "8");
    TRACE_IN_40_transactor_param_props.addLong("TDEST_WIDTH", "8");
    TRACE_IN_40_transactor_param_props.addLong("TID_WIDTH", "8");
    TRACE_IN_40_transactor_param_props.addLong("TUSER_WIDTH", "0");
    TRACE_IN_40_transactor_param_props.addLong("HAS_TREADY", "1");
    TRACE_IN_40_transactor_param_props.addLong("HAS_TSTRB", "0");
    TRACE_IN_40_transactor_param_props.addLong("HAS_TKEEP", "0");
    TRACE_IN_40_transactor_param_props.addLong("HAS_TLAST", "1");
    TRACE_IN_40_transactor_param_props.addLong("FREQ_HZ", "300000000");
    TRACE_IN_40_transactor_param_props.addLong("HAS_RESET", "1");
    TRACE_IN_40_transactor_param_props.addFloat("PHASE", "0.0");
    TRACE_IN_40_transactor_param_props.addString("CLK_DOMAIN", "pfm_dynamic_kernel_clk_0_clk");
    TRACE_IN_40_transactor_param_props.addString("LAYERED_METADATA", "undef");
    TRACE_IN_40_transactor_param_props.addLong("TSIDE_BAND_1_WIDTH", "0");
    TRACE_IN_40_transactor_param_props.addLong("TSIDE_BAND_2_WIDTH", "0");

    mp_TRACE_IN_40_transactor = new xtlm::xaxis_pin2xtlm_t<8,1,8,8,1,1>("TRACE_IN_40_transactor", TRACE_IN_40_transactor_param_props);

    // TRACE_IN_40' transactor ports

    mp_TRACE_IN_40_transactor->TDATA(trace_tdata40);
    mp_TRACE_IN_40_transactor->TDEST(trace_tdest40);
    mp_TRACE_IN_40_transactor->TID(trace_tid40);
    mp_TRACE_IN_40_transactor->TLAST(trace_tlast40);
    mp_TRACE_IN_40_transactor->TREADY(trace_tready40);
    mp_TRACE_IN_40_transactor->TVALID(trace_tvalid40);
    mp_TRACE_IN_40_transactor->CLK(trace_clk);
    mp_TRACE_IN_40_transactor->RST(trace_aresetn);

    // TRACE_IN_40' transactor sockets

    mp_impl->trace_socket40->bind(*(mp_TRACE_IN_40_transactor->socket));
  }
  else
  {
  }

}

#endif // MTI_SYSTEMC




pfm_dynamic_dpa_hub_0::~pfm_dynamic_dpa_hub_0()
{
  delete mp_S_AXIMM_transactor;

  delete mp_S_AXIFIFO_transactor;

  delete mp_S_AXIHUB_transactor;

  delete mp_TRACE_IN_0_transactor;

  delete mp_TRACE_IN_1_transactor;

  delete mp_TRACE_IN_2_transactor;

  delete mp_TRACE_IN_3_transactor;

  delete mp_TRACE_IN_4_transactor;

  delete mp_TRACE_IN_5_transactor;

  delete mp_TRACE_IN_6_transactor;

  delete mp_TRACE_IN_7_transactor;

  delete mp_TRACE_IN_8_transactor;

  delete mp_TRACE_IN_9_transactor;

  delete mp_TRACE_IN_10_transactor;

  delete mp_TRACE_IN_11_transactor;

  delete mp_TRACE_IN_12_transactor;

  delete mp_TRACE_IN_13_transactor;

  delete mp_TRACE_IN_14_transactor;

  delete mp_TRACE_IN_15_transactor;

  delete mp_TRACE_IN_16_transactor;

  delete mp_TRACE_IN_17_transactor;

  delete mp_TRACE_IN_18_transactor;

  delete mp_TRACE_IN_19_transactor;

  delete mp_TRACE_IN_20_transactor;

  delete mp_TRACE_IN_21_transactor;

  delete mp_TRACE_IN_22_transactor;

  delete mp_TRACE_IN_23_transactor;

  delete mp_TRACE_IN_24_transactor;

  delete mp_TRACE_IN_25_transactor;

  delete mp_TRACE_IN_26_transactor;

  delete mp_TRACE_IN_27_transactor;

  delete mp_TRACE_IN_28_transactor;

  delete mp_TRACE_IN_29_transactor;

  delete mp_TRACE_IN_30_transactor;

  delete mp_TRACE_IN_31_transactor;

  delete mp_TRACE_IN_32_transactor;

  delete mp_TRACE_IN_33_transactor;

  delete mp_TRACE_IN_34_transactor;

  delete mp_TRACE_IN_35_transactor;

  delete mp_TRACE_IN_36_transactor;

  delete mp_TRACE_IN_37_transactor;

  delete mp_TRACE_IN_38_transactor;

  delete mp_TRACE_IN_39_transactor;

  delete mp_TRACE_IN_40_transactor;

}

#ifdef MTI_SYSTEMC
SC_MODULE_EXPORT(pfm_dynamic_dpa_hub_0);
#endif

#ifdef XM_SYSTEMC
XMSC_MODULE_EXPORT(pfm_dynamic_dpa_hub_0);
#endif

#ifdef RIVIERA
SC_MODULE_EXPORT(pfm_dynamic_dpa_hub_0);
SC_REGISTER_BV(64);
#endif

