F_CSRCS ?= 
F_INCLUDES ?= 

# F_CSRCS += FreeRTOS/FreeRTOSConfig.c
# F_CSRCS += FreeRTOS/src/croutine.c
# F_CSRCS += FreeRTOS/src/event_groups.c
# F_CSRCS += FreeRTOS/src/list.c
# F_CSRCS += FreeRTOS/src/queue.c
# F_CSRCS += FreeRTOS/src/stream_buffer.c
# F_CSRCS += FreeRTOS/src/tasks.c
# F_CSRCS += FreeRTOS/src/timers.c
# F_CSRCS += FreeRTOS/src/croutine.c
# F_CSRCS += FreeRTOS/src/croutine.c
# F_CSRCS += FreeRTOS/portable/port.c
# F_CSRCS += FreeRTOS/portable/heap_4.c

F_CSRCS += $(shell find ./FreeRTOS -name "*.c")

F_INCLUDES += -IFreeRTOS \
-IFreeRTOS/include \
-IFreeRTOS/portable 

