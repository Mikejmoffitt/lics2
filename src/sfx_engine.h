#ifndef SFX_ENGINE_H
#define SFX_ENGINE_H

void sfx_engine_init(void);
void sfx_engine_play(uint16_t id, uint16_t prio);
void sfx_engine_stop(uint16_t id);
void sfx_engine_irq_handler(void);

#endif  // SFX_ENGINE_H
