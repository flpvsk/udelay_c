#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/dma.h"
#include "hardware/pio.h"
#include "pico/binary_info.h"

#include "read_buf.pio.h"
#include "modulate.pio.h"

// typedef struct {
//   PIO pio;
//   uint sm;
//   uint pin;
//   uint32_t* buff1;
//   uint32_t* buff2;
//   uint16_t buff_size;
//   uint8_t buff_address_boundary;
//   int dma_chan_a;
//   int dma_chan_b;
// } read_process_config;
//
// void read_process_configure(const read_process_config* cfg) {
//   dma_channel_config c_a = dma_channel_get_default_config(
//       cfg->dma_chan_a
//   );
//   channel_config_set_transfer_data_size(&c_a, DMA_SIZE_32);
//   channel_config_set_read_increment(&c_a, true);
//   channel_config_set_write_increment(&c_a, false);
//   channel_config_set_chain_to(&c_a, cfg->dma_chan_b);
//   channel_config_set_dreq(
//       &c_a,
//       pio_get_dreq(cfg->pio, cfg->sm, true)
//   );
//   channel_config_set_ring(
//       &c_a,
//       false,
//       cfg->buff_address_boundary
//   );
//   dma_channel_configure(
//       cfg->dma_chan_a,
//       &c_a,
//       &(cfg->pio)->txf[cfg->sm],
//       cfg->buff1,
//       cfg->buff_size,
//       false
//   );
//
//   dma_channel_config c_b = dma_channel_get_default_config(
//       cfg->dma_chan_b
//   );
//   channel_config_set_transfer_data_size(&c_b, DMA_SIZE_32);
//   channel_config_set_read_increment(&c_b, true);
//   channel_config_set_write_increment(&c_b, false);
//   channel_config_set_chain_to(&c_b, cfg->dma_chan_a);
//   channel_config_set_dreq(
//       &c_b,
//       pio_get_dreq(cfg->pio, cfg->sm, true)
//   );
//   channel_config_set_ring(&c_b, false, cfg->buff_address_boundary);
//   dma_channel_configure(
//       cfg->dma_chan_b,
//       &c_b,
//       &(cfg->pio)->txf[cfg->sm],
//       cfg->buff2,
//       cfg->buff_size,
//       false
//   );
// }
//
// void read_process_start(const read_process_config* cfg) {
//
// }

const uint LED_PIN = 25;
const uint OUT_PIN = 22;
const uint MODULATOR_IN_PIN = 21;
const uint MODULATOR_OUT_PIN = 20;

int main() {
  stdio_init_all();
  // const float clk_div = 520.;
  const float clk_div = 520.;

  // init buffer
  const uint16_t buff_size = 2048; // 8192 in bytes
  const uint8_t address_boundary = 13; // (2 ^ 13) = 8192
  uint32_t buff1[2048] __attribute__((aligned(8192)));
  uint32_t buff2[2048] __attribute__((aligned(8192)));

  for (uint16_t i = 0; i < buff_size; i++) {
    // buff1[i] = 2863311530; // 1010101.....
    buff1[i] = 0;
    buff2[i] = 0;
    if (i < buff_size / 2) {
      buff1[i] = 2013106178; // sine wave
    }
  }

  // output
  // init pio
  PIO pio = pio0;
  uint sm1 = pio_claim_unused_sm(pio, true);
  uint sm1_offset = pio_add_program(pio, &pio_read_buf_program);
  pio_read_buf_program_init(pio, sm1, sm1_offset, OUT_PIN, clk_div);

  // init tx dma
  int tx_chan_a = dma_claim_unused_channel(true);
  int tx_chan_b = dma_claim_unused_channel(true);

  // const read_process_config read_cfg = {
  // }

  dma_channel_config tx_a = dma_channel_get_default_config(tx_chan_a);
  channel_config_set_transfer_data_size(&tx_a, DMA_SIZE_32);
  channel_config_set_read_increment(&tx_a, true);
  channel_config_set_write_increment(&tx_a, false);
  channel_config_set_chain_to(&tx_a, tx_chan_b);
  channel_config_set_dreq(&tx_a, pio_get_dreq(pio, sm1, true));
  channel_config_set_ring(&tx_a, false, address_boundary);
  dma_channel_configure(
      tx_chan_a,
      &tx_a,
      &pio->txf[sm1],
      buff1,
      buff_size,
      false
  );

  dma_channel_config tx_b = dma_channel_get_default_config(tx_chan_b);
  channel_config_set_transfer_data_size(&tx_b, DMA_SIZE_32);
  channel_config_set_read_increment(&tx_b, true);
  channel_config_set_write_increment(&tx_b, false);
  channel_config_set_chain_to(&tx_b, tx_chan_a);
  channel_config_set_dreq(&tx_b, pio_get_dreq(pio, sm1, true));
  channel_config_set_ring(&tx_b, false, address_boundary);
  dma_channel_configure(
      tx_chan_b,
      &tx_b,
      &pio->txf[sm1],
      buff2,
      buff_size,
      false
  );

  // modulator
  // init modulator pio
  uint sm2 = pio_claim_unused_sm(pio, true);
  uint sm2_offset = pio_add_program(pio, &pio_modulate_program);
  pio_modulate_program_init(
      pio,
      sm2,
      sm2_offset,
      MODULATOR_IN_PIN,
      MODULATOR_OUT_PIN,
      clk_div
    );


  // init rx dma
  int rx_chan_a = dma_claim_unused_channel(true);
  int rx_chan_b = dma_claim_unused_channel(true);

  dma_channel_config rx_a = dma_channel_get_default_config(rx_chan_a);
  channel_config_set_transfer_data_size(&rx_a, DMA_SIZE_32);
  channel_config_set_read_increment(&rx_a, false);
  channel_config_set_write_increment(&rx_a, true);
  channel_config_set_chain_to(&rx_a, rx_chan_b);
  channel_config_set_dreq(&rx_a, pio_get_dreq(pio, sm2, false));
  channel_config_set_ring(&rx_a, true, address_boundary);
  dma_channel_configure(
      rx_chan_a,
      &rx_a,
      buff2,
      &pio->rxf[sm2],
      buff_size,
      false
  );

  dma_channel_config rx_b = dma_channel_get_default_config(rx_chan_b);
  channel_config_set_transfer_data_size(&rx_b, DMA_SIZE_32);
  channel_config_set_read_increment(&rx_b, false);
  channel_config_set_write_increment(&rx_b, true);
  channel_config_set_chain_to(&rx_b, rx_chan_a);
  channel_config_set_dreq(&rx_b, pio_get_dreq(pio, sm2, false));
  channel_config_set_ring(&rx_b, true, address_boundary);
  dma_channel_configure(
      rx_chan_b,
      &rx_b,
      buff1,
      &pio->rxf[sm2],
      buff_size,
      false
  );

  // start state machines
  pio_enable_sm_mask_in_sync(pio,
      (1u << sm1) |
      (1u << sm2)
  );

  // start both tx & rx dma
  dma_start_channel_mask(
      (1u << tx_chan_a) |
      (1u << rx_chan_a)
  );

  while (true) {
    tight_loop_contents();
  }
}
