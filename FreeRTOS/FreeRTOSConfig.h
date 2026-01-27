#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include <stdio.h>
/*-----------------------------------------------------------
 * Application specific definitions.
 *
 * These definitions should be adjusted for your particular hardware and
 * application requirements.
 *
 * THESE PARAMETERS ARE DESCRIBED WITHIN THE 'CONFIGURATION' SECTION OF THE
 * FreeRTOS API DOCUMENTATION AVAILABLE ON THE FreeRTOS.org WEB SITE.
 *
 * See http://www.freertos.org/a00110.html
 *----------------------------------------------------------*/

/* Ensure stdint is only used by the compiler, and not the assembler. */
#if defined(__ICCARM__) || defined(__CC_ARM) || defined(__GNUC__)
  	#include <stdint.h>
  	extern uint32_t SystemCoreClock;
#endif

/*常用设置*/
#define configUSE_PREEMPTION							1	//	启用抢占
#define configUSE_PORT_OPTIMISED_TASK_SELECTION         1	//	1: 使用硬件计算下一个要运行的任务, 0: 使用软件算法计算下一个要运行的任务, 默认: 0
#define configUSE_TICKLESS_IDLE     				    0 	//	1: 使能tickless低功耗模式, 默认: 0
#define configCPU_CLOCK_HZ								( SystemCoreClock )
#define configTICK_RATE_HZ								( ( TickType_t ) 1000 )
// #define configSYSTICK_CLOCK_HZ							//	SysTick 定时器的频率与 MCU 核心不同
#define configMAX_PRIORITIES							( 32 )
#define configMINIMAL_STACK_SIZE						( 128 )		//	单位word，32位架构下，4Byte
#define configMAX_TASK_NAME_LEN							( 16 )
#define configUSE_16_BIT_TICKS							0	//	TickType_t --> 16bit <-> 32bit
#define configIDLE_SHOULD_YIELD							1	//	1:空闲任务放弃其分配的时间片的剩余部分
#define configUSE_TASK_NOTIFICATIONS                    1	//	开启任务通知，每个任务增加 8 Byte RAM
#define configTASK_NOTIFICATION_ARRAY_ENTRIES           1	//	定义任务通知数组的大小, 默认: 1 
#define configUSE_MUTEXES								1
#define configUSE_RECURSIVE_MUTEXES						1	//	启用递归互斥量
#define configUSE_COUNTING_SEMAPHORES					1
#define configUSE_ALTERNATIVE_API                       0	//	弃用
#define configQUEUE_REGISTRY_SIZE						8	//	设置可以注册的信号量和消息队列个数。队列记录有两个目的，都涉及到 RTOS 内核的调试，除了进行内核调试外，队列记录没有其它任何目的
#define configUSE_QUEUE_SETS                            0	//	使能队列集
#define configUSE_TIME_SLICING                          1	//	1:FreeRTOS 使用基于时间片的优先级抢占式调度器。这意味着 FreeRTOS 调度器总是运行处于最高优先级的就绪任务
															//	在每个 FreeRTOS 系统节拍中断时在相同优先级的多个任务间进行任务切换。
															//	0:RTOS 调度程序仍将运行处于就绪状态的最高优先级任务，但是不会因为发生滴答中断而在相同优先级的任务之间切换
#define configUSE_NEWLIB_REENTRANT                      0	//	1:每一个创建的任务会分配一个newlib（一个嵌入式 C 库）reent 结构,默认0
#define configENABLE_BACKWARD_COMPATIBILITY             0	//	使能兼容老版本，默认0
#define configNUM_THREAD_LOCAL_STORAGE_POINTERS         0 	//	设置每个任务的线程本地存储指针数组大小，默认0
#define configSTACK_DEPTH_TYPE                          uint16_t	//	用于指定堆栈深度的数据类型，默认：uint16_t
#define configMESSAGE_BUFFER_LENGTH_TYPE                size_t		//	定义消息缓冲区中消息长度的数据类型,size_t:存储在消息缓冲区中的消息不会超过 255 字节; uint16_t:65535

/*Memory*/
#define configSUPPORT_STATIC_ALLOCATION                 0	//	静态分配，定义两个函数
#define configSUPPORT_DYNAMIC_ALLOCATION                1	//	动态内存分配
#define configTOTAL_HEAP_SIZE	   			            ( ( size_t ) ( 20 * 1024 ) )
#define configAPPLICATION_ALLOCATED_HEAP                0	//	0:堆由 FreeRTOS 声明; 1:用户声明   static/extern uint8_t ucHeap[configTOTAL_HEAP_SIZE]
#define configSTACK_ALLOCATION_FROM_SEPARATE_HEAP       0	//	1: 用户自行实现任务创建时使用的内存申请与释放函数, 默认: 0 */

/*Hook*/
#define configUSE_IDLE_HOOK								0	//	void vApplicationIdleHook(void)
#define configUSE_TICK_HOOK								0	//	void vApplicationTickHook(void)
#define configCHECK_FOR_STACK_OVERFLOW					0	//	定义void vApplicationStackOverflowHook(TaskHandle_t xTask, signed char *pcTaskName );
															//	1:任务切换出去后，该任务的上下文环境被保存到自己的堆栈空间，这时很可能堆栈的使用量达到了最大（最深）值。
															//	在这个时候，RTOS 内核会检测堆栈指针是否还指向有效的堆栈空间。如果堆栈指针指向了有效堆栈空间之外的地方，堆栈溢出钩子函数会被调用。
															//	这个方法速度很快，但是不能检测到所有堆栈溢出情况（比如，堆栈溢出没有发生在上下文切换时）
															//	2:当堆栈首次创建时，在它的堆栈区中填充一些已知值（标记）。当任务切换时，RTOS 内核会检测堆栈最后的 16 个字节，
															//	确保标记数据没有被覆盖。如果这 16 个字节有任何一个被改变，则调用堆栈溢出钩子函数
#define configUSE_MALLOC_FAILED_HOOK					0
#define configUSE_DAEMON_TASK_STARTUP_HOOK              0	//	如果 configUSE_TIMERS 和 configUSE_DAEMON_TASK_STARTUP_HOOK 都设置为 1,
															//	定义一个钩子函数，其名称和原型为 void vApplicationDaemonTaskStartupHook( void );
															//	当 RTOS 守护程序任务（也称为定时器服务任务）第一次执行时，钩子函数将被精确调用一次。
															//	需要 RTOS 运行的任何应用程序初始化代码都可以放在 hook 函数中。

/*Run time and task stats ...debug*/
#define configUSE_TRACE_FACILITY                        0	//	使能可视化跟踪调试，真正发布程序时必须将其关闭，会影响性能
#define configUSE_STATS_FORMATTING_FUNCTIONS            0	//	设置configUSE_TRACE_FACILITY 和 configUSE_STATS_FORMATTING_FUNCTIONS为 1
															//	会编译 vTaskList() 和vTaskGetRunTimeStats() 函数。如果将这两个宏任意一个设置为 0，上述两个函数将被忽略。
															//	通常也是在调试时才使用，用来观察各任务
#define configGENERATE_RUN_TIME_STATS                   0	//	依赖：configUSE_TRACE_FACILITY configUSE_STATS_FORMATTING_FUNCTIONS configSUPPORT_DYNAMIC_ALLOCATION 为1 
															//	仅限调试阶段，严重影响性能，设置为1，需要定义一下两个函数 vTaskStartScheduler()中根据宏配置调用
															//	宏：portCONFIGURE_TIMER_FOR_RUN_TIME_STATS() ，提供基准时钟函数定义到此宏，要比系统节拍中断更准
															//	宏：portGET_RUN_TIME_COUNTER_VALUE() 提供一个返回基准时钟当前时间（通常是计数值）的函数，define 到宏


/*Co-routine related definitions*/
#define configUSE_CO_ROUTINES                           0	//	1: 启用协程, 默认: 0
#define configMAX_CO_ROUTINE_PRIORITIES                 2	//	定义协程的最大优先级, 最大优先级=configMAX_CO_ROUTINE_PRIORITIES-1, 无默认configUSE_CO_ROUTINES为1时需定义 */

/* Software timer definitions. */
#define configUSE_TIMERS				1
#define configTIMER_TASK_PRIORITY		( 2 )
#define configTIMER_QUEUE_LENGTH		10
#define configTIMER_TASK_STACK_DEPTH	( configMINIMAL_STACK_SIZE * 2 )

/* FreeRTOS MPU specific definitions. */
#define configINCLUDE_APPLICATION_DEFINED_PRIVILEGED_FUNCTIONS 		0



/* Optional functions - most linkers will remove unused functions anyway. */
#define INCLUDE_vTaskPrioritySet                		1
#define INCLUDE_uxTaskPriorityGet               		1
#define INCLUDE_vTaskDelete                     		1
#define INCLUDE_vTaskSuspend                    		1
#define INCLUDE_xResumeFromISR                  		1
#define INCLUDE_xTaskResumeFromISR              		1	//	恢复在中断中挂起的任务
#define INCLUDE_vTaskDelayUntil                 		1
#define INCLUDE_vTaskDelay                      		1
#define INCLUDE_xTaskGetSchedulerState          		1	//	获取任务调度器状态
#define INCLUDE_xTaskGetCurrentTaskHandle       		1
#define INCLUDE_uxTaskGetStackHighWaterMark     		0	//	获取任务堆栈历史剩余最小值
#define INCLUDE_xTaskGetIdleTaskHandle          		0
#define INCLUDE_eTaskGetState                   		0	//	获取任务状态
#define INCLUDE_xEventGroupSetBitFromISR        		1
#define INCLUDE_xTimerPendFunctionCall          		0	//	将函数的执行挂到定时器服务任务
#define INCLUDE_xTaskAbortDelay                 		0
#define INCLUDE_xTaskGetHandle                  		0	//	通过任务名获取任务句柄

/*CubeMX*/
#define INCLUDE_vTaskCleanUpResources					1
#define INCLUDE_xQueueGetMutexHolder         			1


/* Cortex-M specific definitions. */
#ifdef __NVIC_PRIO_BITS
	/* __BVIC_PRIO_BITS will be specified when CMSIS is being used. */
	#define configPRIO_BITS       		__NVIC_PRIO_BITS
#else
	#define configPRIO_BITS       		4        /* 15 priority levels */
#endif

#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY			15
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY	5

#define configKERNEL_INTERRUPT_PRIORITY 				( configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )
#define configMAX_SYSCALL_INTERRUPT_PRIORITY 			( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )
#define configMAX_API_CALL_INTERRUPT_PRIORITY           configMAX_SYSCALL_INTERRUPT_PRIORITY

#define configASSERT( x ) if( ( x ) == 0 ) { taskDISABLE_INTERRUPTS(); printf("err at line %d of file \"%s\". \r\n ",__LINE__,__FILE__); for( ;; ); }

#define vPortSVCHandler SVC_Handler
#define xPortPendSVHandler PendSV_Handler
#define xPortSysTickHandler SysTick_Handler

#endif /* FREERTOS_CONFIG_H */

