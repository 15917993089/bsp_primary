#ifndef LED_DRIVER_H
#define LED_DRIVER_H

#define LED_DEV_MAGIC 'g'
#define MY_LED_OFF _IO(LED_DEV_MAGIC,0)
#define MY_LED_ON _IO(LED_DEV_MAGIC,1)

#endif

