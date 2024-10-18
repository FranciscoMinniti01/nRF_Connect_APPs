/*---------------------------------------- INCLUDES --------------------------------------------------------------------------------*/

#include "uart_machine.h"

/*---------------------------------------- VARIABLES --------------------------------------------------------------------------------*/

static uint8_t uart_machine_state    = UART_INIT;
static uint8_t uart_machine_state_CB = NOT_USET;

static uint8_t current_buffer_index  = 0;

static uint16_t uart_data_index  = 0;
static uint16_t app_data_index   = 0;

const struct device *uart = DEVICE_DT_GET(DT_NODELABEL(uart1));

const struct uart_config uart_cfg = {   .baudrate   = 9600,
		                                .parity     = UART_CFG_PARITY_NONE,
		                                .stop_bits  = UART_CFG_STOP_BITS_1,
		                                .data_bits  = UART_CFG_DATA_BITS_8,
		                                .flow_ctrl  = UART_CFG_FLOW_CTRL_NONE
	                                };

uint8_t uart_rx_buffers[RECEIVE_BUFF_NUMBER][RECEIVE_BUFF_SIZE];

bool app_notifi_error = false;
bool app_notifi_ready = false;

/*---------------------------------------- Functions --------------------------------------------------------------------------------*/

bool get_UART_notifi_error(void)
{
    return app_notifi_error;
}

bool get_UART_notifi_ready(void)
{
    return app_notifi_ready;
}

uint8_t get_rdy_data() //Esta funcion retorna de un caracter a la vex, para aumentar la velocidad quizas pueda dar como parametro el largo maximo de info que quiero y retornar el largo de la info que mando.
{
    uint8_t data;
    if(app_data_index != uart_data_index)
    {
        //printk("UART get_rdy_data : uart index=%d - app index=%d\n",uart_data_index,app_data_index);
        data = * ( *((uint8_t (*)[RECEIVE_BUFF_NUMBER*RECEIVE_BUFF_SIZE])uart_rx_buffers) + app_data_index);
        app_data_index++;
        app_data_index%=(RECEIVE_BUFF_NUMBER*RECEIVE_BUFF_SIZE);
        return data;
    }
    return 0;
}

/*---------------------------------------- CALLBACK --------------------------------------------------------------------------------*/

static void uart_cb(const struct device *dev, struct uart_event *evt, void *user_data)
{
	switch (evt->type) 
	{
        case UART_RX_BUF_REQUEST:
        {
            //printk("UART_RX_BUF_REQUEST\n");
            uart_rx_buf_rsp(uart,
                            uart_rx_buffers[(current_buffer_index+1)%RECEIVE_BUFF_NUMBER],//&uart_rx_buffers[(current_buffer_index+1)%RECEIVE_BUFF_NUMBER],
                            RECEIVE_BUFF_SIZE);
            break;
        }

		case UART_RX_RDY:
		{
            printk("UART_RX_RDY\n");
            uart_data_index = uart_data_index + evt->data.rx.len;
            uart_data_index %= (RECEIVE_BUFF_NUMBER*RECEIVE_BUFF_SIZE);
			break;
		}

        case UART_RX_BUF_RELEASED:
        {
            //printk("UART_RX_BUF_RELEASED\n");
            current_buffer_index ++;
            current_buffer_index %= RECEIVE_BUFF_NUMBER;
            break;
        }

		case UART_RX_DISABLED:
		{
            printk("UART_RX_DISABLED\n");
            uart_machine_state_CB = WAIT_FOT_REINIT;
            app_notifi_error = true;
            break;
		}

        case UART_RX_STOPPED:
        {
            printk("UART_RX_STOP: ");
            app_notifi_error = true;
            switch (evt->data.rx_stop.reason)
            {
                case UART_ERROR_OVERRUN:
                    printk("OVERRUN\n");
                    break;
            
                case UART_ERROR_PARITY:
                    printk("PARITY\n");
                    break;

                case UART_ERROR_FRAMING:
                    printk("FRAMING\n");
                    break;

                case UART_BREAK:
                    printk("BREAK\n");
                    break;

                case UART_ERROR_COLLISION:
                    printk("COLLISION\n");
                    break;

                case UART_ERROR_NOISE:
                    printk("NOISE\n");
                    break;
            }
            break;
        }

		default:
		{
			printk("Default - Event: %d\n" , evt->type);
			break;
		}
	}
}

/*---------------------------------------- MACHINE --------------------------------------------------------------------------------*/

void uart_machine()
{
    uint8_t err;
    if(uart_machine_state_CB != NOT_USET)
    {
        uart_machine_state = uart_machine_state_CB;
        uart_machine_state_CB = NOT_USET;
    }
    switch (uart_machine_state)
    {
        case UART_INIT:
        {
            if (!device_is_ready(uart)) {
		        printk("UART device is not ready\n");
		        break;
	        }

	        err = uart_configure(uart, &uart_cfg);
	        if (err == -ENOSYS) {
		        printk("Cannot init UART\n");
	        }

	        err = uart_callback_set(uart, uart_cb, NULL);
	        if (err) {
		        printk("Cannot init UART Callback\n");
	        }

            uart_machine_state = UART_RX_INIT;
            app_notifi_ready = false;
            app_notifi_error = false;
            break;
        }

        case UART_RX_INIT:
        {
            current_buffer_index = 0;
            uart_data_index  = 0;
            app_data_index   = 0;
            if (uart_rx_enable(uart ,uart_rx_buffers[current_buffer_index],RECEIVE_BUFF_SIZE,RECEIVE_TIMEOUT_HAR)){
		        printk("UART RX can not be enabled\n");
                break;
	        }

            printk("UART STATE WORKING\n");
            uart_machine_state = UART_WORKING;
            app_notifi_ready = true;
            break;
        }
    
        case UART_WORKING:
        {

            break;
        }

        case UART_DEACTIVATE:
        {
            printk("UART_DEACTIVATE\n");
            uart_rx_disable(uart);
            uart_machine_state = UART_INIT;
            app_notifi_ready = false;
            break;
        }

        case WAIT_FOT_REINIT:
        {
            printk("UART WAIT_FOT_REINIT\n");
            k_sleep(K_MSEC(K_INTERVAL_TO_REINIT));
            uart_machine_state = UART_INIT;
            app_notifi_ready = false;
            break;
        }

        default:
        {
            printk("UART default\n");
            uart_machine_state = UART_DEACTIVATE;
            break;
        }
    }
}   