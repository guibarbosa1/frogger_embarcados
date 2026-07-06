## ============================================================================
## hdmi.xdc  -  Constraints de pinos para a saida HDMI TX da Zybo Z7-20
## ----------------------------------------------------------------------------
## Pinos extraidos do Zybo-Z7-Master.xdc oficial da Digilent (Rev. B).
##
## IMPORTANTE: os nomes em get_ports precisam bater com o nome da porta de
## interface TMDS que voce criou no Block Design. O script build_hdmi.tcl
## cria essa porta com o nome "hdmi_out". Se voce mudar o nome no Block
## Design, altere tambem o prefixo aqui (ex.: troque "hdmi_out_clk_p" por
## "seu_nome_clk_p").
##
## O que esta incluido:
##   - Par diferencial do clock TMDS  (1 par)
##   - 3 pares diferenciais de dados TMDS (lanes 0, 1, 2)
##
## O que NAO esta incluido (nao e' necessario para video fixo em hardware):
##   - DDC / I2C (EDID do monitor) — so necessario para negociacao de resolucao
##   - HPD (Hot Plug Detect)       — so necessario para detectar conexao do cabo
## ============================================================================

## --- Clock TMDS (par diferencial) ---
set_property -dict { PACKAGE_PIN H16  IOSTANDARD TMDS_33 } [get_ports { hdmi_out_clk_p }];  # hdmi_tx_clk_p
set_property -dict { PACKAGE_PIN H17  IOSTANDARD TMDS_33 } [get_ports { hdmi_out_clk_n }];  # hdmi_tx_clk_n

## --- Dados TMDS — lane 0 ---
set_property -dict { PACKAGE_PIN D19  IOSTANDARD TMDS_33 } [get_ports { hdmi_out_data_p[0] }];  # hdmi_tx_p[0]
set_property -dict { PACKAGE_PIN D20  IOSTANDARD TMDS_33 } [get_ports { hdmi_out_data_n[0] }];  # hdmi_tx_n[0]

## --- Dados TMDS — lane 1 ---
set_property -dict { PACKAGE_PIN C20  IOSTANDARD TMDS_33 } [get_ports { hdmi_out_data_p[1] }];  # hdmi_tx_p[1]
set_property -dict { PACKAGE_PIN B20  IOSTANDARD TMDS_33 } [get_ports { hdmi_out_data_n[1] }];  # hdmi_tx_n[1]

## --- Dados TMDS — lane 2 ---
set_property -dict { PACKAGE_PIN B19  IOSTANDARD TMDS_33 } [get_ports { hdmi_out_data_p[2] }];  # hdmi_tx_p[2]
set_property -dict { PACKAGE_PIN A20  IOSTANDARD TMDS_33 } [get_ports { hdmi_out_data_n[2] }];  # hdmi_tx_n[2]

## ----------------------------------------------------------------------------
## Nota tecnica: cada pino _n esta mapeado para o complemento fisico real do
## par diferencial no banco do FPGA. Isso e o que o XDC mestre da Digilent
## especifica — nao remova as linhas _n, pois o Vivado precisa delas para
## inferir corretamente o par LVDS/TMDS e aplicar as regras de DRC.
## ----------------------------------------------------------------------------
