#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/dma.h"
#include "hardware/pio.h"
#include "pico/binary_info.h"

#include "read_buf.pio.h"
// #include "modulate.pio.h"

const uint LED_PIN = 25;
const uint OUT_PIN = 22;

int main() {
  stdio_init_all();

  // init pio
  PIO pio = pio0;
  uint sm1 = pio_claim_unused_sm(pio, true);
  uint offset = pio_add_program(pio, &pio_read_buf_program);
  pio_read_buf_program_init(pio, sm1, offset, OUT_PIN, 520.);

  // init buffer
  uint8_t address_boundary = 11;
  uint16_t buff_size = 1 << address_boundary; // 2048
  uint32_t buff1[buff_size] __attribute__((aligned(2048)));
  uint32_t buff2[buff_size] __attribute__((aligned(2048)));

  for (uint16_t i = 0; i < buff_size; i++) {
    buff1[i] = 2863311530; // 1010101.....
    buff2[i] = 2013106178; // sine wave
  }

  // init dma
  int tx_chan_a = dma_claim_unused_channel(true);
  int tx_chan_b = dma_claim_unused_channel(true);

  dma_channel_config c_a = dma_channel_get_default_config(tx_chan_a);
  channel_config_set_transfer_data_size(&c_a, DMA_SIZE_32);
  channel_config_set_read_increment(&c_a, true);
  channel_config_set_write_increment(&c_a, false);
  channel_config_set_chain_to(&c_a, tx_chan_b);
  channel_config_set_dreq(&c_a, pio_get_dreq(pio, sm1, true));
  channel_config_set_ring(&c_a, false, 11);
  dma_channel_configure(
      tx_chan_a,
      &c_a,
      &pio->txf[sm1],
      buff1,
      buff_size,
      false
  );

  dma_channel_config c_b = dma_channel_get_default_config(tx_chan_b);
  channel_config_set_transfer_data_size(&c_b, DMA_SIZE_32);
  channel_config_set_read_increment(&c_b, true);
  channel_config_set_write_increment(&c_b, false);
  channel_config_set_chain_to(&c_b, tx_chan_a);
  channel_config_set_dreq(&c_b, pio_get_dreq(pio, sm1, true));
  channel_config_set_ring(&c_b, false, 8);
  dma_channel_configure(
      tx_chan_b,
      &c_b,
      &pio->txf[sm1],
      buff2,
      buff_size,
      false
  );

  // start state machine
  pio_sm_set_enabled(pio, sm1, true);

  // start dma
  dma_channel_start(tx_chan_a);

  while (true) {
    tight_loop_contents();
  }
}
