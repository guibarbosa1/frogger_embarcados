## ============================================================================
## build_hdmi.tcl  -  Gera um projeto Vivado 2017.4 completo para saida
##                    HDMI (HDMI TX) na Zybo Z7-20, com pipeline de video
##                    baseado em AXI VDMA + rgb2dvi (Digilent).
## ----------------------------------------------------------------------------
## DEPENDENCIAS — clonar na mesma pasta deste script ANTES de rodar:
##
##   git clone https://github.com/Digilent/vivado-boards
##   git clone https://github.com/Digilent/Zybo-Z7-20-HDMI
##
## COMO USAR:
##   1) Abra o Vivado 2017.4. No Tcl Console:
##        cd {<caminho completo desta pasta>}
##        source build_hdmi.tcl
##   2) Aguarde a mensagem "OK! Block design montado."
##   3) Clique em Generate Bitstream (pode demorar varios minutos).
##   4) File -> Export -> Export Hardware (Include bitstream) -> Launch SDK.
##
## RESOLUCAO PADRAO: 640x480 @ 60 Hz. Para alterar, veja a secao
## "Video Timing Controller" abaixo e ajuste VIDEO_MODE + frequencias do
## clk_wiz (pixel clock e o serial = 5x o pixel).
## ============================================================================

## ========================= CONFIGURACOES DO USUARIO =========================
## Edite apenas esta secao para adaptar o script ao seu projeto.

# Caminhos derivados da localizacao DESTE script (nao de [pwd]).
# No Tcl Console do Vivado o [pwd] aponta para AppData/Roaming — nao confie.
set script_dir [file dirname [file normalize [info script]]]
puts "INFO: pasta do script = $script_dir"

set proj_name  "zybo_hdmi"                        ;# nome do projeto Vivado
set proj_dir   "$script_dir/vivado_proj_hdmi"     ;# onde criar o projeto
set part       "xc7z020clg400-1"                  ;# part number da Zybo Z7-20
set board      "digilentinc.com:zybo-z7-20:part0:1.0"

# Pasta da vivado-library da Digilent (onde esta o IP rgb2dvi).
set ip_repo    "$script_dir/Zybo-Z7-20-HDMI/repo/vivado-library"

# Board files da Digilent (necessarios para o preset do PS7: DDR, UART, clocks).
set board_repo "$script_dir/vivado-boards/new/board_files"

# Arquivo de constraints dos pinos HDMI (deve estar nesta mesma pasta).
set xdc_file   "$script_dir/hdmi.xdc"

## ===========================================================================

# --- Registra os board files SEM precisar copiar para a instalacao do Vivado.
#     Deve acontecer ANTES do create_project. ---
if {[file isdirectory $board_repo]} {
    puts "INFO: registrando board files de $board_repo"
    set_param board.repoPaths $board_repo
} else {
    puts "WARN: $board_repo nao encontrado."
    puts "WARN: clone https://github.com/Digilent/vivado-boards nesta pasta."
}

puts "INFO: criando projeto \"$proj_name\" em $proj_dir"
create_project $proj_name $proj_dir -part $part -force
set_property target_language Verilog [current_project]

# --- Detecta o board part da Zybo Z7-20.
#     O preset do PS7 (DDR / UART / clocks) depende dos board files — sem
#     isso o hardware nao inicializa corretamente na placa. ---
set avail [get_board_parts -quiet]
if {[lsearch -exact $avail $board] >= 0} {
    set_property board_part $board [current_project]
} else {
    set match [lsearch -inline -glob $avail "*zybo-z7-20*"]
    if {$match ne ""} {
        puts "INFO: usando board_part detectado automaticamente: $match"
        set_property board_part $match [current_project]
    } else {
        error "Board files da Zybo Z7-20 nao encontrados.\
\n   Clone https://github.com/Digilent/vivado-boards nesta pasta e rode novamente.\
\n   Sem o preset do PS7 (DDR/UART) o hardware nao funciona na placa.\
\n   get_board_parts retornou: $avail"
    }
}

# --- Registra o IP repo da Digilent (rgb2dvi e outros IPs). ---
if {![file isdirectory $ip_repo]} {
    error "IP repo nao encontrado em: $ip_repo\
\n   Clone https://github.com/Digilent/Zybo-Z7-20-HDMI nesta pasta."
}
set_property ip_repo_paths $ip_repo [current_project]
update_ip_catalog

## ============================================================================
## Block Design "system"
## ============================================================================
create_bd_design "system"

## --- Zynq PS (processador ARM + DDR + perifericos) ---
## O apply_bd_automation com apply_board_preset aplica automaticamente as
## configuracoes de DDR e UART corretas para a Zybo Z7-20.
set ps [create_bd_cell -type ip -vlnv xilinx.com:ip:processing_system7:5.5 \
            processing_system7_0]
apply_bd_automation \
    -rule xilinx.com:bd_rule:processing_system7 \
    -config {make_external "FIXED_IO, DDR" apply_board_preset "1" \
             Master "Disable" Slave "Disable"} \
    [get_bd_cells processing_system7_0]

# Habilita a porta HP0 (acesso direto a DDR pelo VDMA) e fixa FCLK0 = 100 MHz.
set_property -dict [list \
    CONFIG.PCW_USE_S_AXI_HP0          {1}   \
    CONFIG.PCW_FPGA0_PERIPHERAL_FREQMHZ {100} \
] $ps

## --- Clocking Wizard ---
## clk_out1 = pixel clock  (25.175 MHz para 640x480@60)
## clk_out2 = serial clock (125.875 MHz = 5x pixel, para TMDS do rgb2dvi)
## Para outra resolucao: altere CLKOUT1 para o pixel clock correto e
## CLKOUT2 para 5x esse valor.
set clkw [create_bd_cell -type ip -vlnv xilinx.com:ip:clk_wiz:5.4 clk_wiz_0]
set_property -dict [list \
    CONFIG.PRIM_IN_FREQ              {100.000} \
    CONFIG.CLKOUT2_USED              {true}    \
    CONFIG.CLKOUT1_REQUESTED_OUT_FREQ {25.175} \
    CONFIG.CLKOUT2_REQUESTED_OUT_FREQ {125.875} \
    CONFIG.USE_LOCKED                {true}    \
    CONFIG.USE_RESET                 {true}    \
    CONFIG.RESET_TYPE                {ACTIVE_LOW} \
    CONFIG.RESET_PORT                {resetn}  \
] $clkw

## --- Resets ---
## rst_100M : dominio AXI (100 MHz)
## rst_pix  : dominio de video (pixel clock)
set rst100 [create_bd_cell -type ip -vlnv xilinx.com:ip:proc_sys_reset:5.0 rst_100M]
set rstpix [create_bd_cell -type ip -vlnv xilinx.com:ip:proc_sys_reset:5.0 rst_pix]

## --- AXI VDMA (Video Direct Memory Access — somente leitura, MM2S) ---
## Le frames da DDR e os envia como AXI4-Stream para o pipeline de video.
## c_m_axis_mm2s_tdata_width = 32 bits (formato 0x00RRGGBB por pixel).
## c_num_fstores = 2 habilita double buffering.
set vdma [create_bd_cell -type ip -vlnv xilinx.com:ip:axi_vdma:6.3 axi_vdma_0]
set_property -dict [list \
    CONFIG.c_include_s2mm            {0}    \
    CONFIG.c_include_mm2s            {1}    \
    CONFIG.c_num_fstores             {2}    \
    CONFIG.c_m_axis_mm2s_tdata_width {32}   \
    CONFIG.c_mm2s_linebuffer_depth   {2048} \
    CONFIG.c_mm2s_max_burst_length   {32}   \
] $vdma

## --- AXI4-Stream Subset Converter (32 -> 24 bits, remap de canais) ---
## O rgb2dvi interpreta vid_pData[23:16]=R, [15:8]=B, [7:0]=G.
## Para um framebuffer 0x00RRGGBB sair com as cores corretas no monitor,
## o remap e': {R[23:16], B[7:0], G[15:8]}.
## Se as cores aparecerem trocadas, ajuste o TDATA_REMAP.
set ssc [create_bd_cell -type ip \
             -vlnv xilinx.com:ip:axis_subset_converter:1.1 \
             axis_subset_converter_0]
set_property -dict [list \
    CONFIG.S_TDATA_NUM_BYTES {4} \
    CONFIG.M_TDATA_NUM_BYTES {3} \
    CONFIG.TDATA_REMAP       {tdata[23:16],tdata[7:0],tdata[15:8]} \
] $ssc

## --- Video Timing Controller (VTC) ---
## Gera os sinais de sincronismo (hsync, vsync, de) para 640x480 @ 60 Hz.
## VIDEO_MODE {640x480p} preenche automaticamente todos os parametros de
## timing — nao edite os campos GEN_* manualmente (so ficam ativos com
## VIDEO_MODE=Custom). Sem AXI4-Lite, a geracao comeca sozinha.
## Para outra resolucao: troque VIDEO_MODE (ex.: {720p}, {1080p}) e atualize
## os clocks do clk_wiz.
set vtc [create_bd_cell -type ip -vlnv xilinx.com:ip:v_tc:6.1 v_tc_0]
set_property -dict [list \
    CONFIG.enable_detection {false}   \
    CONFIG.enable_generation {true}   \
    CONFIG.HAS_AXI4_LITE    {false}   \
    CONFIG.HAS_INTC_IF      {false}   \
    CONFIG.VIDEO_MODE        {640x480p} \
] $vtc

## --- AXI4-Stream to Video Out ---
## Converte o stream AXI (vindo do VDMA/subset) em sinais de video paralelos
## (data + sync) sincronizados pelo VTC.
set vout [create_bd_cell -type ip \
              -vlnv xilinx.com:ip:v_axi4s_vid_out:4.0 \
              v_axi4s_vid_out_0]
set_property -dict [list \
    CONFIG.C_ADDR_WIDTH    {12} \
    CONFIG.C_HAS_ASYNC_CLK {1}  \
    CONFIG.C_VTG_MASTER_SLAVE {1} \
] $vout

## --- rgb2dvi (Digilent IP) ---
## Converte RGB paralelo em TMDS (sinal HDMI). O serial clock (5x pixel)
## e fornecido externamente pelo clk_wiz (kGenerateSerialClk=false).
## kRstActiveHigh=false pois o sinal de reset ativo-alto viria do locked
## invertido — aqui usamos locked direto como aRst_n (ativo-baixo).
set r2d [create_bd_cell -type ip \
             -vlnv digilentinc.com:ip:rgb2dvi:1.4 \
             rgb2dvi_0]
set_property -dict [list \
    CONFIG.kGenerateSerialClk {false} \
    CONFIG.kRstActiveHigh     {false} \
] $r2d

## --- Constante logica '1' (para pinos de enable sempre ativos) ---
set one [create_bd_cell -type ip -vlnv xilinx.com:ip:xlconstant:1.1 const_one]
set_property -dict [list CONFIG.CONST_WIDTH {1} CONFIG.CONST_VAL {1}] $one

## --- AXI GPIO para os 4 botoes da placa ---
## Util para leitura dos botoes no software bare-metal.
## Se nao precisar de GPIO, remova este bloco e ajuste o NUM_MI do
## ps7_0_axi_periph de 2 para 1.
set gpio [create_bd_cell -type ip -vlnv xilinx.com:ip:axi_gpio:2.0 axi_gpio_0]
if {[catch {
    apply_bd_automation \
        -rule xilinx.com:bd_rule:board \
        -config {Board_Interface {btns_4bits ( 4 Buttons )} Manual_Source {Auto}} \
        [get_bd_intf_pins axi_gpio_0/GPIO]
} emsg]} {
    puts "WARN: automacao do GPIO falhou ($emsg)."
    puts "WARN: configure o axi_gpio_0 manualmente (4 bits, all inputs)."
    set_property -dict [list CONFIG.C_GPIO_WIDTH {4} CONFIG.C_ALL_INPUTS {1}] $gpio
}

## ============================================================================
## AXI Interconnects
## ============================================================================
## ps7_0_axi_periph : GP0 do PS -> perifericos (VDMA control, GPIO)
set icp [create_bd_cell -type ip \
             -vlnv xilinx.com:ip:axi_interconnect:2.1 \
             ps7_0_axi_periph]
set_property -dict [list CONFIG.NUM_MI {2}] $icp

## axi_mem_intercon : VDMA master -> HP0 (acesso a DDR)
set icm [create_bd_cell -type ip \
             -vlnv xilinx.com:ip:axi_interconnect:2.1 \
             axi_mem_intercon]
set_property -dict [list CONFIG.NUM_MI {1}] $icm

## ============================================================================
## Conexoes de interface AXI e Stream de video
## ============================================================================
# GP0 -> perifericos
connect_bd_intf_net [get_bd_intf_pins processing_system7_0/M_AXI_GP0] \
                    [get_bd_intf_pins ps7_0_axi_periph/S00_AXI]
connect_bd_intf_net [get_bd_intf_pins ps7_0_axi_periph/M00_AXI] \
                    [get_bd_intf_pins axi_vdma_0/S_AXI_LITE]
connect_bd_intf_net [get_bd_intf_pins ps7_0_axi_periph/M01_AXI] \
                    [get_bd_intf_pins axi_gpio_0/S_AXI]

# VDMA -> DDR (via HP0)
connect_bd_intf_net [get_bd_intf_pins axi_vdma_0/M_AXI_MM2S] \
                    [get_bd_intf_pins axi_mem_intercon/S00_AXI]
connect_bd_intf_net [get_bd_intf_pins axi_mem_intercon/M00_AXI] \
                    [get_bd_intf_pins processing_system7_0/S_AXI_HP0]

# Pipeline de video: VDMA -> SubsetConverter -> vid_out -> rgb2dvi
connect_bd_intf_net [get_bd_intf_pins axi_vdma_0/M_AXIS_MM2S] \
                    [get_bd_intf_pins axis_subset_converter_0/S_AXIS]
connect_bd_intf_net [get_bd_intf_pins axis_subset_converter_0/M_AXIS] \
                    [get_bd_intf_pins v_axi4s_vid_out_0/video_in]
connect_bd_intf_net [get_bd_intf_pins v_tc_0/vtiming_out] \
                    [get_bd_intf_pins v_axi4s_vid_out_0/vtiming_in]
connect_bd_intf_net [get_bd_intf_pins v_axi4s_vid_out_0/vid_io_out] \
                    [get_bd_intf_pins rgb2dvi_0/RGB]

# Porta TMDS externa -> casa com hdmi.xdc (nome "hdmi_out").
# Nota: make_bd_intf_pins_external nao aceita -name no Vivado 2017.4;
# por isso criamos a porta explicitamente e conectamos.
create_bd_intf_port -mode Master -vlnv digilentinc.com:interface:tmds_rtl:1.0 hdmi_out
connect_bd_intf_net [get_bd_intf_ports hdmi_out] [get_bd_intf_pins rgb2dvi_0/TMDS]

## ============================================================================
## Conexoes de clock e reset
## ============================================================================
set fclk   [get_bd_pins processing_system7_0/FCLK_CLK0]    ;# 100 MHz
set frst_n [get_bd_pins processing_system7_0/FCLK_RESET0_N]
set pix    [get_bd_pins clk_wiz_0/clk_out1]                ;# pixel clock
set ser    [get_bd_pins clk_wiz_0/clk_out2]                ;# serial clock (5x)
set lck    [get_bd_pins clk_wiz_0/locked]

# Clocking Wizard
connect_bd_net $fclk   [get_bd_pins clk_wiz_0/clk_in1]
connect_bd_net $frst_n [get_bd_pins clk_wiz_0/resetn]

# Resets (aguardam o PLL travar antes de liberar o restante do circuito)
connect_bd_net $fclk   [get_bd_pins rst_100M/slowest_sync_clk]
connect_bd_net $frst_n [get_bd_pins rst_100M/ext_reset_in]
connect_bd_net $lck    [get_bd_pins rst_100M/dcm_locked]
connect_bd_net $pix    [get_bd_pins rst_pix/slowest_sync_clk]
connect_bd_net $frst_n [get_bd_pins rst_pix/ext_reset_in]
connect_bd_net $lck    [get_bd_pins rst_pix/dcm_locked]

# Clocks AXI (dominio 100 MHz)
connect_bd_net $fclk [get_bd_pins processing_system7_0/M_AXI_GP0_ACLK]
connect_bd_net $fclk [get_bd_pins processing_system7_0/S_AXI_HP0_ACLK]
connect_bd_net $fclk [get_bd_pins axi_vdma_0/s_axi_lite_aclk]
connect_bd_net $fclk [get_bd_pins axi_vdma_0/m_axi_mm2s_aclk]
connect_bd_net $fclk [get_bd_pins axi_vdma_0/m_axis_mm2s_aclk]
connect_bd_net $fclk [get_bd_pins axis_subset_converter_0/aclk]
connect_bd_net $fclk [get_bd_pins axi_gpio_0/s_axi_aclk]
connect_bd_net $fclk [get_bd_pins ps7_0_axi_periph/ACLK]
connect_bd_net $fclk [get_bd_pins ps7_0_axi_periph/S00_ACLK]
connect_bd_net $fclk [get_bd_pins ps7_0_axi_periph/M00_ACLK]
connect_bd_net $fclk [get_bd_pins ps7_0_axi_periph/M01_ACLK]
connect_bd_net $fclk [get_bd_pins axi_mem_intercon/ACLK]
connect_bd_net $fclk [get_bd_pins axi_mem_intercon/S00_ACLK]
connect_bd_net $fclk [get_bd_pins axi_mem_intercon/M00_ACLK]
connect_bd_net $fclk [get_bd_pins v_axi4s_vid_out_0/aclk]

# Resets AXI
set prst100  [get_bd_pins rst_100M/peripheral_aresetn]
set icrst100 [get_bd_pins rst_100M/interconnect_aresetn]
connect_bd_net $prst100  [get_bd_pins axi_vdma_0/axi_resetn]
connect_bd_net $prst100  [get_bd_pins axis_subset_converter_0/aresetn]
connect_bd_net $prst100  [get_bd_pins axi_gpio_0/s_axi_aresetn]
connect_bd_net $prst100  [get_bd_pins v_axi4s_vid_out_0/aresetn]
connect_bd_net $icrst100 [get_bd_pins ps7_0_axi_periph/ARESETN]
connect_bd_net $prst100  [get_bd_pins ps7_0_axi_periph/S00_ARESETN]
connect_bd_net $prst100  [get_bd_pins ps7_0_axi_periph/M00_ARESETN]
connect_bd_net $prst100  [get_bd_pins ps7_0_axi_periph/M01_ARESETN]
connect_bd_net $icrst100 [get_bd_pins axi_mem_intercon/ARESETN]
connect_bd_net $prst100  [get_bd_pins axi_mem_intercon/S00_ARESETN]
connect_bd_net $prst100  [get_bd_pins axi_mem_intercon/M00_ARESETN]

# Dominio de video (pixel clock)
connect_bd_net $pix [get_bd_pins v_tc_0/clk]
connect_bd_net [get_bd_pins rst_pix/peripheral_aresetn] [get_bd_pins v_tc_0/resetn]
connect_bd_net $pix [get_bd_pins v_axi4s_vid_out_0/vid_io_out_clk]
connect_bd_net $pix [get_bd_pins rgb2dvi_0/PixelClk]
connect_bd_net $ser [get_bd_pins rgb2dvi_0/SerialClk]
connect_bd_net $lck [get_bd_pins rgb2dvi_0/aRst_n]
# ATENCAO: v_axi4s_vid_out_0/locked e' uma SAIDA (status do IP), nao uma entrada.
# Nao conecte o "locked" do clk_wiz nele. O locked ja alimenta os dcm_locked
# dos dois resets e o aRst_n do rgb2dvi, que e' o uso correto.

# Handshake de sincronismo VTC <-> vid_out
connect_bd_net [get_bd_pins v_axi4s_vid_out_0/vtg_ce] [get_bd_pins v_tc_0/gen_clken]

# Sinais de enable sempre ativos
set c1 [get_bd_pins const_one/dout]
connect_bd_net $c1 [get_bd_pins v_tc_0/clken]
connect_bd_net $c1 [get_bd_pins v_axi4s_vid_out_0/aclken]
connect_bd_net $c1 [get_bd_pins v_axi4s_vid_out_0/vid_io_out_ce]

## ============================================================================
## Mapa de enderecamento
## ============================================================================
assign_bd_address
# Garante que o VDMA enxergue toda a DDR (512 MB a partir do endereco 0)
catch { set_property range  512M \
    [get_bd_addr_segs {axi_vdma_0/Data_MM2S/SEG_processing_system7_0_HP0_DDR_LOWOCM}] }
catch { set_property offset 0x00000000 \
    [get_bd_addr_segs {axi_vdma_0/Data_MM2S/SEG_processing_system7_0_HP0_DDR_LOWOCM}] }

## ============================================================================
## Finalizar: validar, gerar wrapper e adicionar XDC
## ============================================================================
regenerate_bd_layout
validate_bd_design
save_bd_design

set bd_file [get_files system.bd]
make_wrapper -files $bd_file -top

# Localiza o wrapper pela pasta real do projeto (nao confiar em [pwd])
set real_dir [get_property DIRECTORY [current_project]]
set wrapper  [glob -nocomplain "$real_dir/*.srcs/sources_1/bd/system/hdl/system_wrapper.*"]
if {$wrapper eq ""} {
    error "Wrapper nao encontrado em $real_dir — make_wrapper falhou?"
}
add_files -norecurse $wrapper
set_property top system_wrapper [current_fileset]
update_compile_order -fileset sources_1

if {[file exists $xdc_file]} {
    add_files -fileset constrs_1 -norecurse $xdc_file
} else {
    puts "WARN: $xdc_file nao encontrado — adicione o hdmi.xdc manualmente."
}
update_compile_order -fileset sources_1

puts "============================================================"
puts " OK! Block design montado com sucesso."
puts " Proximo passo: clique em Generate Bitstream."
puts " Depois: File -> Export Hardware (Include bitstream)"
puts "         -> File -> Launch SDK"
puts "============================================================"
